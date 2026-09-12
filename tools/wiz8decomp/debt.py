"""Ranked source-model inconsistencies for recovery agents.

One command. JSON only. No human-report mode.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from .global_model import (
    classify_status_region,
    overlapping_globals,
    parse_global_definitions,
    status_member_accesses,
    type_consistency_violations,
)
from .identity_lint import identity_violations
from .paths import atomic_json
from .source_units import (
    UNRESOLVED_FRAGMENT,
    cmake_source_units,
    file_emits_code_or_data,
    source_unit_records,
)

_PROVISIONAL_COMMENT = re.compile(
    r"original translation-unit name is unknown|descriptive name is provisional|"
    r"reconstructed logical owner|owner not proved",
    re.IGNORECASE,
)
_VOID_STAR_CALL = re.compile(
    r"(?:reinterpret_cast|static_cast)\s*<\s*([A-Za-z_][\w:]*)\s*\*\s*>\s*\(\s*([A-Za-z_]\w*)\s*\("
)
_BYTE_OFFSET = re.compile(
    r"reinterpret_cast\s*<\s*char\s*\*\s*>\s*\(([^)]+)\)\s*\+\s*(0x[0-9a-fA-F]+|\d+)"
)


def _function_markers(repo_dir: Path) -> list[dict[str, Any]]:
    path = repo_dir / "build/source-index.json"
    if not path.is_file():
        return []
    document = json.loads(path.read_text(encoding="utf-8"))
    return [
        marker
        for marker in document.get("markers") or []
        if marker.get("marker_kind") == "FUNCTION"
    ]


def _void_star_uniform_consumers(repo_dir: Path) -> list[dict[str, Any]]:
    casts: dict[str, set[str]] = {}
    for root in (repo_dir / "src/wiz8", repo_dir / "include/wiz8"):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
                continue
            relative = str(path.relative_to(repo_dir))
            for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                for match in _VOID_STAR_CALL.finditer(line):
                    casts.setdefault(match.group(2), set()).add(f"{match.group(1)}*@{relative}")
    rows: list[dict[str, Any]] = []
    for name, uses in sorted(casts.items()):
        types = {item.split("@", 1)[0] for item in uses}
        if len(types) != 1 or len(uses) < 2:
            continue
        rows.append(
            {
                "function": name,
                "type": next(iter(types)),
                "sites": sorted(uses),
            }
        )
    return rows


def _raw_modeled_offsets(repo_dir: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for root in (repo_dir / "src/wiz8", repo_dir / "include/wiz8"):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix.lower() not in {".h", ".hpp", ".cpp"}:
                continue
            relative = str(path.relative_to(repo_dir))
            for number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), 1
            ):
                match = _BYTE_OFFSET.search(line)
                if match:
                    rows.append(
                        {
                            "file": relative,
                            "line": number,
                            "base": match.group(1).strip(),
                            "offset": match.group(2),
                        }
                    )
    return rows


def _provisional_units(
    repo_dir: Path,
    records: dict[str, dict[str, str]],
    layout: Any | None = None,
) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for path, record in records.items():
        full = repo_dir / path
        if not full.is_file():
            continue
        text = full.read_text(encoding="utf-8", errors="replace")
        if record["class"] != UNRESOLVED_FRAGMENT and not _PROVISIONAL_COMMENT.search(text):
            continue
        addresses = [
            int(match.group(1), 16)
            for match in re.finditer(
                r"//\s*(?:FUNCTION|TEMPLATE|GLOBAL|VTABLE):\s+WIZ8\s+(0x[0-9a-fA-F]+)",
                text,
                re.IGNORECASE,
            )
        ]
        row: dict[str, Any] = {
            "path": path,
            "class": record["class"],
            "original_path": record.get("original_path", ""),
            "address_min": f"0x{min(addresses):08x}" if addresses else "",
            "address_max": f"0x{max(addresses):08x}" if addresses else "",
            "markers": len(addresses),
            "functions": [
                f"0x{value:08x}"
                for value in sorted(
                    int(match.group(1), 16)
                    for match in re.finditer(
                        r"//\s*FUNCTION:\s+WIZ8\s+(0x[0-9a-fA-F]+)",
                        text,
                        re.IGNORECASE,
                    )
                )
            ],
        }
        if layout is not None and addresses:
            low = layout.owner(min(addresses))
            high = layout.owner(max(addresses))
            row["low_original_tu"] = low.get("source_path") or ""
            row["low_attribution"] = low.get("attribution") or ""
            row["high_original_tu"] = high.get("source_path") or ""
            row["high_attribution"] = high.get("attribution") or ""
            row["previous_hard_unit"] = low.get("previous_hard_unit")
            row["next_hard_unit"] = high.get("next_hard_unit") or low.get("next_hard_unit")
        rows.append(row)
    return rows


def structural_debt_report(repo_dir: Path, *, layout: Any | None = None) -> dict[str, Any]:
    from .placement import placement_violations
    from .ghidra.unit_intervals import assertion_anchors, read_assertions, TranslationUnitLayout

    records = source_unit_records(repo_dir)
    definitions = parse_global_definitions(repo_dir)
    overlaps = overlapping_globals(definitions)
    types = type_consistency_violations(definitions)
    identities = (
        identity_violations(repo_dir) if (repo_dir / "build/source-index.json").is_file() else []
    )
    empty = [
        path
        for path, record in records.items()
        if record["class"] != "compiler-emission" and not file_emits_code_or_data(repo_dir / path)
    ]
    if layout is None:
        units, headers = assertion_anchors(read_assertions(repo_dir))
        layout = TranslationUnitLayout(units, header_anchors=headers)
    placement = placement_violations(repo_dir, layout, _function_markers(repo_dir))
    counts = {"original-tu": 0, "unresolved-fragment": 0, "compiler-emission": 0}
    for record in records.values():
        counts[record["class"]] += 1
    report = {
        "schema": "wiz8.structural-debt-v1",
        "source_units": counts,
        "empty_tus": empty,
        "unresolved_fragments": [
            item for item in records.values() if item["class"] == UNRESOLVED_FRAGMENT
        ],
        "overlapping_globals": overlaps,
        "type_consistency": types,
        "placement": placement,
        "duplicate_identities": identities,
        "void_star_uniform_consumers": _void_star_uniform_consumers(repo_dir),
        "raw_struct_offsets": _raw_modeled_offsets(repo_dir),
        "provisional_tus": _provisional_units(repo_dir, records, layout),
        "status_region_globals": classify_status_region(definitions),
        "status_member_accesses": status_member_accesses(repo_dir),
        "cmake_units": cmake_source_units(repo_dir),
    }
    destination = repo_dir / "build/reports/structural-debt.json"
    destination.parent.mkdir(parents=True, exist_ok=True)
    atomic_json(destination, report)
    report["report"] = str(destination.relative_to(repo_dir))
    return report
