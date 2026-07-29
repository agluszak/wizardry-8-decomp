"""Derive recovered-function ownership from address-marked C++ source.

The source model is intentionally generated, never reviewed by hand.  A
``FUNCTION`` marker and the declaration immediately following it own the
address, name, prototype, and translation unit for recovered code. ``TEMPLATE``,
``SYNTHETIC``, and ``LIBRARY`` markers use the conventional symbol comment below
the marker.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .markers import (
    FUNCTION_MARKER,
    LIBRARY_MARKER,
    SYNTHETIC_MARKER,
    TEMPLATE_MARKER,
    iter_source_files,
    normalize_offset,
)

_CLASS_DECLARATION = re.compile(
    r"^\s*(?:class|struct)\s+(?:__declspec\s*\([^)]*\)\s*)?([A-Za-z_]\w*)"
)
_COMMENT_NAME = re.compile(r"^//\s*(\S(?:.*\S)?)\s*$")
_NON_NAMES = frozenset({"__declspec", "if", "while", "for", "switch", "sizeof"})
_BLOCK_COMMENT = re.compile(r"/\*.*?\*/")
_SOURCE_QUALIFIER = re.compile(
    r"\b(?:static|virtual|inline|__inline|__forceinline|explicit|friend|"
    r"__cdecl|__stdcall|__fastcall|WINAPI|CALLBACK)\b\s*"
)
_SCOPED_TYPE = re.compile(r"\b(?:[A-Za-z_]\w*::)+([A-Za-z_]\w*)")
_DLLIMPORT = re.compile(r"__declspec\s*\(\s*dllimport\s*\)")


class SourceModelError(ValueError):
    """Address-marked source cannot be represented unambiguously."""


@dataclass(frozen=True)
class SourceFunction:
    address: int
    kind: str
    file: str
    line: int
    name: str
    prototype: str
    ghidra_prototype: str
    calling_convention: str


@dataclass(frozen=True)
class SourceModel:
    functions: dict[int, SourceFunction]
    externals: dict[str, SourceFunction] = field(default_factory=dict)


def _enclosing_class(lines: list[str], marker_index: int) -> str:
    """Return the nearest lexical class/struct containing a marker."""

    scopes: list[str] = []
    for line in lines[:marker_index]:
        declaration = _CLASS_DECLARATION.match(line)
        class_name = declaration.group(1) if declaration is not None else ""
        for character in line:
            if character == "{":
                scopes.append(class_name)
                class_name = ""
            elif character == "}" and scopes:
                scopes.pop()
    return next((name for name in reversed(scopes) if name), "")


def _token_before_open_paren(text: str, offset: int) -> str:
    index = offset - 1
    while index >= 0 and text[index].isspace():
        index -= 1
    end = index + 1
    angle_depth = 0
    while index >= 0:
        character = text[index]
        if character == ">":
            angle_depth += 1
        elif character == "<":
            angle_depth -= 1
        elif character.isspace() and angle_depth == 0:
            break
        index -= 1
    return text[index + 1 : end]


def _function_name(prototype: str) -> str:
    for match in re.finditer(r"\(", prototype):
        candidate = _token_before_open_paren(prototype, match.start())
        leaf = candidate.rsplit("::", 1)[-1].lstrip("~")
        if re.fullmatch(r"[A-Za-z_]\w*", leaf) and leaf not in _NON_NAMES:
            return candidate
    return ""


def _declaration(lines: list[str], start: int) -> tuple[str, str]:
    pieces: list[str] = []
    opened = False
    for index in range(start, min(len(lines), start + 16)):
        stripped = lines[index].strip()
        if not stripped or stripped.startswith("//"):
            continue
        if stripped.startswith("template") and not opened:
            continue
        pieces.append(stripped)
        opened = opened or "(" in stripped
        joined = " ".join(pieces)
        if opened and joined.count("(") <= joined.count(")"):
            break
    prototype = " ".join(" ".join(pieces).split())
    prototype = prototype.split("{", 1)[0].rstrip().rstrip(";").rstrip()
    return _function_name(prototype), prototype


def _comment_name(lines: list[str], marker_index: int) -> str:
    if marker_index + 1 >= len(lines):
        return ""
    match = _COMMENT_NAME.match(lines[marker_index + 1].strip())
    return match.group(1).strip() if match is not None else ""


def _calling_convention(name: str, prototype: str) -> str:
    if "__stdcall" in prototype or "WINAPI" in prototype or "CALLBACK" in prototype:
        return "__stdcall"
    if "__fastcall" in prototype:
        return "__fastcall"
    if "__cdecl" in prototype:
        return "__cdecl"
    return "__thiscall" if "::" in name else "__cdecl"


def _ghidra_prototype(prototype: str) -> str:
    """Translate one C++ declaration into Ghidra's C signature grammar.

    Ghidra's native parser supplies the actual data types.  This normalization
    only removes C++ syntax that the parser cannot represent: method
    qualification, cv qualifiers, references, and source-level calling
    convention spellings.  Unresolvable types remain unresolvable and are
    reported by the synchronization pass rather than guessed here.
    """

    if not prototype:
        return ""
    text = _BLOCK_COMMENT.sub("", prototype)
    name = _function_name(text)
    if not name:
        return ""
    text = text.replace(name, "source_function", 1)
    text = _DLLIMPORT.sub("", text)
    text = re.sub(r'extern(?:al)?\s+"C"\s*', "", text)
    text = _SOURCE_QUALIFIER.sub("", text)
    before_name = text.split("source_function", 1)[0].strip()
    if not before_name:
        text = "void " + text
    text = re.sub(r"\s+(?:const|volatile|override|final)\s*$", "", text)
    text = re.sub(r"\b(?:const|volatile)\s+", "", text)
    text = text.replace("&", "*")
    text = _SCOPED_TYPE.sub(r"\1", text)
    return " ".join(text.split())


def target_for_program(program_name: str) -> str:
    """Return the source marker target owned by a configured Ghidra program."""

    return "SURRENDER" if "--sr--" in program_name else "WIZ8"


def build_source_model(repository: Path, target: str = "WIZ8") -> SourceModel:
    source_stem = {"WIZ8": "wiz8", "SURRENDER": "surrender"}.get(target.upper())
    if source_stem is None:
        raise SourceModelError(f"unsupported source-model target: {target}")
    roots = [repository / "src" / source_stem, repository / "include" / source_stem]
    functions: dict[int, SourceFunction] = {}
    externals: dict[str, SourceFunction] = {}
    locations: dict[int, str] = {}

    for path in iter_source_files(roots):
        relative = path.relative_to(repository).as_posix()
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        for index, line in enumerate(lines):
            if _DLLIMPORT.search(line) and not _CLASS_DECLARATION.match(line):
                external_name, external_prototype = _declaration(lines, index)
                if external_name and external_name not in externals:
                    externals[external_name] = SourceFunction(
                        address=0,
                        kind="EXTERNAL",
                        file=relative,
                        line=index + 1,
                        name=external_name,
                        prototype=external_prototype,
                        ghidra_prototype=_ghidra_prototype(external_prototype),
                        calling_convention=_calling_convention(external_name, external_prototype),
                    )
            marker = FUNCTION_MARKER.match(line)
            kind = "FUNCTION"
            if marker is None:
                marker = TEMPLATE_MARKER.match(line)
                kind = "TEMPLATE"
            if marker is None:
                marker = LIBRARY_MARKER.match(line)
                kind = "LIBRARY"
            if marker is None:
                marker = SYNTHETIC_MARKER.match(line)
                kind = "SYNTHETIC"
            if marker is None or marker.group("module").upper() != target.upper():
                continue

            address = int(normalize_offset(marker.group("offset")), 16)
            if address in functions:
                raise SourceModelError(
                    f"{target} 0x{address:08x} is claimed by both "
                    f"{locations[address]} and {relative}:{index + 1}"
                )

            if kind == "FUNCTION":
                name, prototype = _declaration(lines, index + 1)
                enclosing = _enclosing_class(lines, index)
                if enclosing and name and "::" not in name:
                    name = f"{enclosing}::{name}"
            elif kind == "TEMPLATE":
                name = _comment_name(lines, index)
                _parsed_name, prototype = _declaration(lines, index + 2)
            else:
                name = _comment_name(lines, index)
                prototype = ""

            if not name:
                raise SourceModelError(
                    f"{relative}:{index + 1}: {kind} marker at 0x{address:08x} "
                    "does not resolve to a source name"
                )
            item = SourceFunction(
                address=address,
                kind=kind,
                file=relative,
                line=index + 1,
                name=name,
                prototype=prototype,
                ghidra_prototype=_ghidra_prototype(prototype),
                calling_convention=_calling_convention(name, prototype),
            )
            functions[address] = item
            locations[address] = f"{relative}:{index + 1}"

    return SourceModel(
        functions=dict(sorted(functions.items())), externals=dict(sorted(externals.items()))
    )


def validate_source_names_against_index(
    repository: Path, document: dict[str, Any], target: str = "WIZ8"
) -> int:
    """Require Ghidra's primary names to agree with source-owned identities."""

    indexed_functions = {int(item["entry"], 16): item for item in document["functions"]}
    model = build_source_model(repository, target)
    mismatches = []
    checked = 0
    for address, source in model.functions.items():
        # Compiler-generated names use MSVC spellings such as backticks and
        # spaces that are not legal Ghidra symbols. Their SYNTHETIC marker owns
        # the reccmp identity, but there is no source declaration to synchronize
        # into the ProgramDB.
        if source.kind == "SYNTHETIC":
            continue
        checked += 1
        indexed = indexed_functions.get(address)
        if indexed is None:
            mismatches.append(f"0x{address:08x}: missing")
            continue
        qualified = str(indexed.get("qualified_name") or indexed["name"])
        if qualified != source.name:
            mismatches.append(f"0x{address:08x}: source={source.name!r}, ghidra={qualified!r}")
    if mismatches:
        raise ValueError(
            "Ghidra source-name synchronization is pending; run "
            "`uv run wiz8 ghidra sync-source --apply`: " + "; ".join(mismatches[:20])
        )
    return checked
