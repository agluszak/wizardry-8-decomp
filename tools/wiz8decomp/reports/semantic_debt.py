"""Rank non-gating recovery debt from the existing source model."""

from __future__ import annotations

import re
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from ..ghidra.unit_intervals import TranslationUnitLayout, assertion_anchors, read_assertions
from ..source_index import load_source_index, source_functions
from ..source_units import UNRESOLVED_FRAGMENT, source_unit_records

_PLACEHOLDER = re.compile(r"^Function[0-9A-Fa-f]{6,8}$")
_ADDRESS_SUFFIX = re.compile(r"[A-Za-z_][A-Za-z0-9_:<>]*[0-9A-Fa-f]{6,8}$")
_PROVISIONAL_FIELD = re.compile(r"\b(?:unknown|value|field)_[0-9A-Fa-f]{2,}\b")
_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"})

_STALE_CLAIM = re.compile(
    r"unrecovered|unported|not\s+yet\s+identified|not\s+yet\s+ported|"
    r"not\s+yet\s+recovered|not\s+identified|body\s+is\s+not\s+ported|"
    r"unresolved\s+at\s+link|stays\s+unresolved|named\s+by\s+address|"
    r"identity\s+ceiling",
    re.IGNORECASE,
)
_EXPLICIT_ADDRESS = re.compile(r"\b0x([0-9A-Fa-f]{6,8})\b")
_ADDRESS_SUFFIXED_TOKEN = re.compile(r"\b[A-Za-z_][A-Za-z0-9_:<>]*[0-9A-Fa-f]{6,8}\b")
_TRAILING_HEX = re.compile(r"([0-9A-Fa-f]{6,8})$")


def _comment_regions(lines: list[str]) -> list[tuple[int, int, str]]:
    """Return (start_line, end_line, text) for ``/* */`` blocks and ``//`` runs.

    Lines are 1-based and the end line is exclusive. ``//``-only consecutive
    lines merge into one region; a trailing ``//`` after code forms its own
    single-line region. Only the first block comment on a line is captured.
    """

    regions: list[tuple[int, int, str]] = []
    index = 0
    total = len(lines)
    while index < total:
        line = lines[index]
        if line.lstrip().startswith("//"):
            start = index
            parts = []
            while index < total and lines[index].lstrip().startswith("//"):
                parts.append(lines[index].lstrip()[2:])
                index += 1
            regions.append((start + 1, index, "\n".join(parts)))
            continue
        block = line.find("/*")
        slash = line.find("//")
        if slash != -1 and (block == -1 or slash < block):
            regions.append((index + 1, index + 1, line[slash:]))
            index += 1
            continue
        if block == -1:
            index += 1
            continue
        start = index
        parts = [line[block + 2 :]]
        closing = parts[0].find("*/")
        if closing != -1:
            parts[0] = parts[0][:closing]
        else:
            index += 1
            while index < total:
                closing = lines[index].find("*/")
                if closing != -1:
                    parts.append(lines[index][:closing])
                    break
                parts.append(lines[index])
                index += 1
        regions.append((start + 1, index + 1, "\n".join(parts)))
        index += 1
    return regions


def _candidate_addresses(text: str) -> set[int]:
    """Collect retail VAs cited by a comment: ``0x`` literals and the hex
    suffixes positional names embed, accepting dropped leading zeros."""

    addresses = {int(match.group(1), 16) for match in _EXPLICIT_ADDRESS.finditer(text)}
    for token in _ADDRESS_SUFFIXED_TOKEN.finditer(text):
        digits_match = _TRAILING_HEX.search(token.group(0))
        if not digits_match:
            continue
        digits = digits_match.group(1)
        addresses.add(int(digits, 16))
        if len(digits) > 6:
            addresses.add(int(digits[-6:], 16))
    return addresses


def _stale_recovery_claims(repository: Path, functions: dict[int, Any]) -> list[dict[str, Any]]:
    """Flag prose debt claims whose cited address is a defined FUNCTION.

    A claim phrase alone is not flagged: plenty of comments legitimately call
    out still-unrecovered fields or routines. A claim becomes stale when the
    retail address it cites (or an address-suffixed name it mentions) already
    carries a FUNCTION marker in the source index.
    """

    rows: list[dict[str, Any]] = []
    for root_name in ("include/wiz8", "src/wiz8"):
        root = repository / root_name
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix.casefold() not in _SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository).as_posix()
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
            for start, end, comment in _comment_regions(lines):
                phrase = _STALE_CLAIM.search(comment)
                if not phrase:
                    continue
                context = [comment]
                for line in lines[end : min(end + 8, len(lines))]:
                    if not line.strip():
                        break
                    context.append(line)
                for address in sorted(_candidate_addresses("\n".join(context))):
                    function = functions.get(address)
                    if function is None:
                        continue
                    rows.append(
                        {
                            "source_file": relative,
                            "line": start,
                            "phrase": phrase.group(0),
                            "address": f"0x{address:08x}",
                            "resolved_name": function.name,
                            "resolved_source_file": function.source_file,
                        }
                    )
    rows.sort(key=lambda row: (row["source_file"], row["line"], row["address"]))
    return rows


def _unresolved_declarations(index: dict[str, Any]) -> list[dict[str, Any]]:
    definitions = {
        str(item.get("semantic_id") or "")
        for item in index.get("declarations", [])
        if item.get("is_definition")
    }
    rows: dict[str, dict[str, Any]] = {}
    for item in index.get("declarations", []):
        name = str(item.get("qualified_name") or "")
        semantic_id = str(item.get("semantic_id") or "")
        if not _PLACEHOLDER.fullmatch(name) or semantic_id in definitions:
            continue
        rows.setdefault(
            semantic_id,
            {
                "name": name,
                "semantic_id": semantic_id,
                "source_file": str(item.get("source_file") or ""),
                "line": int(item.get("line") or 0),
            },
        )
    return sorted(rows.values(), key=lambda row: (row["name"], row["source_file"], row["line"]))


def semantic_name_opportunity_report(repository: Path) -> dict[str, Any]:
    """Rank unresolved placeholder names by their checked-in call-site footprint."""

    index = load_source_index(repository)
    candidates = _unresolved_declarations(index)
    sources = []
    for root_name in ("include/wiz8", "src/wiz8"):
        root = repository / root_name
        sources.extend(
            path.read_text(encoding="utf-8", errors="replace")
            for path in root.rglob("*")
            if path.is_file() and path.suffix.casefold() in _SOURCE_SUFFIXES
        )
    corpus = "\n".join(sources)
    rows = []
    for candidate in candidates:
        references = len(re.findall(rf"\b{re.escape(candidate['name'])}\s*\(", corpus))
        if references < 2:
            continue
        rows.append({**candidate, "references": references})
    rows.sort(key=lambda row: (-row["references"], row["name"]))
    return {
        "schema": "wiz8.semantic-name-opportunities-v1",
        "non_gating": True,
        "count": len(rows),
        "functions": rows,
    }


def _field_references(repository: Path) -> list[dict[str, Any]]:
    counts: Counter[str] = Counter()
    files: dict[str, set[str]] = defaultdict(set)
    for root_name in ("include/wiz8", "src/wiz8"):
        root = repository / root_name
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.casefold() not in _SOURCE_SUFFIXES:
                continue
            relative = path.relative_to(repository).as_posix()
            for name in _PROVISIONAL_FIELD.findall(
                path.read_text(encoding="utf-8", errors="replace")
            ):
                counts[name] += 1
                files[name].add(relative)
    return [
        {"name": name, "references": count, "files": sorted(files[name])}
        for name, count in counts.most_common()
        if count >= 3
    ]


def semantic_debt_report(repository: Path, target: str = "WIZ8") -> dict[str, Any]:
    """Return a read-only recovery queue; none of its rows are validation failures."""

    index = load_source_index(repository)
    units = source_unit_records(repository)
    fragment_paths = [
        path for path, record in units.items() if record["class"] == UNRESOLVED_FRAGMENT
    ]
    functions = source_functions(repository, target)
    suffixed = [
        {
            "address": f"0x{address:08x}",
            "name": function.name,
            "source_file": function.source_file,
        }
        for address, function in sorted(functions.items())
        if _ADDRESS_SUFFIX.fullmatch(function.name) and not _PLACEHOLDER.fullmatch(function.name)
    ]

    unit_set = set(fragment_paths)
    anchors, headers = assertion_anchors(read_assertions(repository))
    layout = TranslationUnitLayout(anchors, header_anchors=headers)
    provisional_by_file: dict[str, dict[str, Any]] = {
        path: {"source_file": path, "function_count": 0, "candidate_original_units": set()}
        for path in fragment_paths
    }
    for marker in index.get("markers", []):
        current = str(marker.get("source_file") or "")
        if marker.get("marker_kind") != "FUNCTION" or current not in unit_set:
            continue
        provisional_by_file[current]["function_count"] += 1
        address = int(marker["address"])
        owner = layout.owner(address)
        if owner.get("attribution") not in {"direct", "bounded", "cross-build"}:
            continue
        candidate = str(owner.get("source_path") or "")
        if candidate:
            provisional_by_file[current]["candidate_original_units"].add(candidate)

    provisional = [
        {**row, "candidate_original_units": sorted(row["candidate_original_units"])}
        for row in provisional_by_file.values()
    ]
    provisional.sort(key=lambda row: (-row["function_count"], row["source_file"]))

    fields = _field_references(repository)
    unresolved = _unresolved_declarations(index)
    stale = _stale_recovery_claims(repository, functions)
    return {
        "schema": "wiz8.semantic-debt-v1",
        "non_gating": True,
        "summary": {
            "unresolved_fragments": len(fragment_paths),
            "unresolved_function_declarations": len(unresolved),
            "address_suffixed_names": len(suffixed),
            "high_reference_provisional_fields": len(fields),
            "provisional_tu_placements": len(provisional),
            "stale_recovery_claims": len(stale),
        },
        "unresolved_fragments": provisional,
        "unresolved_function_declarations": unresolved,
        "address_suffixed_names": suffixed,
        "high_reference_provisional_fields": fields,
        "provisional_tu_placements": provisional,
        "stale_recovery_claims": stale,
    }
