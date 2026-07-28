"""Gate the hygiene of reccmp address markers in owned sources.

Ported from imperialism-decomp's `check_marker_hygiene`, which enforces the same
two rules against the same reccmp marker convention.

A `// FUNCTION: <MODULE> 0x...` marker must be *immediately* followed by the
declaration it names, with no blank line and no comment between them. reccmp
attributes a marker to whatever declaration follows it, so an explanatory
comment in the gap does not merely look untidy: it is the difference between a
marker that binds to its function and one that binds to nothing. Prose belongs
above the marker, where it reads the same and cannot separate the two.

Each address may also own at most one marker across the tree. Two definitions
carrying one original address means one original function modelled twice;
reccmp silently keeps one of them, so the duplicate is invisible in its report.
Addresses are keyed by module, unlike the single-module original this is ported
from: this tree builds WIZ8 alongside the srEXT DLLs, which are separate images
that legitimately reuse the same offsets.

`// LIBRARY:` markers are deliberately exempt from the adjacency rule. They name
linked library code that has no owned definition to sit against, and the
established convention in vc6_runtime.cpp is to follow one with the symbol's
name as a comment. They are still checked for duplicate addresses.
"""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path
from typing import Any

FUNCTION_MARKER = re.compile(
    r"^\s*//\s*FUNCTION\s*:\s*(?P<module>[A-Za-z0-9_]+)\s+(?P<offset>(?:0x)?[0-9a-fA-F]+)"
)
LIBRARY_MARKER = re.compile(
    r"^\s*//\s*LIBRARY\s*:\s*(?P<module>[A-Za-z0-9_]+)\s+(?P<offset>(?:0x)?[0-9a-fA-F]+)"
)
SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
EXTERN_DECLARATION = re.compile(r'^\s*extern\s+(?:"C"\s+)?(?P<body>[^;]+);', re.MULTILINE)
DATA_SYMBOL = re.compile(r"(?P<symbol>[A-Za-z_]\w*)\s*(?:\[[^]]*\])?$")


class MarkerHygieneError(RuntimeError):
    """A marker does not name the declaration it is supposed to name."""


def normalize_offset(raw: str) -> str:
    """Canonicalise an offset so padding cannot hide a duplicate.

    Markers are written both zero-padded and bare, so comparing the text would
    treat 0x00401000 and 0x401000 as different addresses and let a genuine
    duplicate through.
    """

    return f"0x{int(raw, 16):08x}"


def following_declaration_offset(lines: list[str], marker_index: int) -> tuple[int, str] | None:
    """Find what actually follows a marker, skipping over whole comments.

    Block comments have to be tracked rather than pattern-matched line by line.
    A continuation line inside one is ordinary prose - "never see a live entry
    because..." starts with a letter, not with an asterisk - so testing lines
    individually reads the middle of a comment as a declaration. That
    misjudgement is not academic: acting on it splices the marker into the
    comment, which is exactly the detachment this module exists to prevent.

    Returns the offset of the first real line after the marker and what
    separates it, or None when nothing follows.
    """

    index = marker_index + 1
    separator = ""
    while index < len(lines):
        stripped = lines[index].strip()
        if not stripped:
            separator = separator or "blank line"
            index += 1
            continue
        if stripped.startswith("//"):
            separator = "comment"
            index += 1
            continue
        if stripped.startswith("/*"):
            separator = "comment"
            while index < len(lines) and "*/" not in lines[index]:
                index += 1
            index += 1
            continue
        return index, separator
    return None


def iter_source_files(roots: list[Path]) -> list[Path]:
    files: list[Path] = []
    for root in roots:
        if root.is_file():
            files.append(root)
            continue
        files.extend(path for path in root.rglob("*") if path.suffix in SOURCE_SUFFIXES)
    return sorted(set(files))


def check_marker_hygiene(roots: list[Path], repo_dir: Path) -> dict[str, Any]:
    """Report every marker that does not sit against its declaration.

    Raises when anything is wrong, so the gate fails loudly rather than
    returning a report a caller might not read.
    """

    detached: list[str] = []
    owners: dict[tuple[str, str], list[str]] = defaultdict(list)
    library_owners: dict[tuple[str, str], list[str]] = defaultdict(list)
    header_externs: set[str] = set()
    source_externs: dict[str, set[str]] = defaultdict(set)

    for path in iter_source_files(roots):
        try:
            relative = path.relative_to(repo_dir).as_posix()
        except ValueError:
            relative = path.as_posix()
        text = path.read_text(encoding="utf-8", errors="ignore")
        lines = text.splitlines()
        for declaration in EXTERN_DECLARATION.finditer(text):
            body = " ".join(declaration.group("body").split())
            if "(" in body:
                continue
            symbol = DATA_SYMBOL.search(body)
            if symbol is None:
                continue
            name = symbol.group("symbol")
            if path.suffix in {".h", ".hpp"}:
                header_externs.add(name)
            else:
                source_externs[name].add(relative)
        for index, line in enumerate(lines):
            library = LIBRARY_MARKER.match(line)
            if library is not None:
                key = (library.group("module"), normalize_offset(library.group("offset")))
                library_owners[key].append(f"{relative}:{index + 1}")
                continue
            match = FUNCTION_MARKER.match(line)
            if match is None:
                continue
            offset = normalize_offset(match.group("offset"))
            owners[(match.group("module"), offset)].append(f"{relative}:{index + 1}")
            found = following_declaration_offset(lines, index)
            if found is not None and found[1]:
                detached.append(
                    f"{relative}:{index + 1}: // FUNCTION {offset} is separated from its "
                    f"declaration by a {found[1]}"
                )

    duplicated = {offset: places for offset, places in owners.items() if len(places) > 1}
    duplicated_library = {
        offset: places for offset, places in library_owners.items() if len(places) > 1
    }

    problems: list[str] = list(detached)
    problems += [
        f"{module} {offset} is claimed by {len(places)} FUNCTION markers: {', '.join(places)}"
        for (module, offset), places in sorted(duplicated.items())
    ]
    shadowed_externs = {
        symbol: sorted(paths)
        for symbol, paths in source_externs.items()
        if symbol in header_externs and len(paths) > 1
    }
    problems += [
        f"{symbol} has a canonical header declaration but is redeclared in "
        f"{len(paths)} source files: {', '.join(paths)}"
        for symbol, paths in sorted(shadowed_externs.items())
    ]
    problems += [
        f"{module} {offset} is claimed by {len(places)} LIBRARY markers: {', '.join(places)}"
        for (module, offset), places in sorted(duplicated_library.items())
    ]
    if problems:
        raise MarkerHygieneError("marker hygiene failed:\n  " + "\n  ".join(sorted(problems)))

    return {
        "function_markers": sum(len(places) for places in owners.values()),
        "function_addresses": len(owners),
        "library_markers": sum(len(places) for places in library_owners.values()),
        "library_addresses": len(library_owners),
        "shadowed_externs": len(shadowed_externs),
        "files": len(iter_source_files(roots)),
    }
