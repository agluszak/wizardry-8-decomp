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
    return {
        "schema": "wiz8.semantic-debt-v1",
        "non_gating": True,
        "summary": {
            "unresolved_fragments": len(fragment_paths),
            "unresolved_function_declarations": len(unresolved),
            "address_suffixed_names": len(suffixed),
            "high_reference_provisional_fields": len(fields),
            "provisional_tu_placements": len(provisional),
        },
        "unresolved_fragments": provisional,
        "unresolved_function_declarations": unresolved,
        "address_suffixed_names": suffixed,
        "high_reference_provisional_fields": fields,
        "provisional_tu_placements": provisional,
    }
