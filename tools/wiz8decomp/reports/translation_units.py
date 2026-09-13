from __future__ import annotations

import csv
import io
from pathlib import Path
from typing import Any

from ..ghidra.unit_intervals import (
    TranslationUnitInterval,
    TranslationUnitLayout,
    _address,
    assertion_anchors,
    derive_intervals,
)
from ..paths import atomic_write

__all__ = [
    "TranslationUnitInterval",
    "derive_intervals",
    "function_inventory",
    "misplaced_function_rows",
    "original_unit_rows",
    "render_gameplay_map_csv",
    "render_interval_csv",
    "translation_unit_report",
]

ATTRIBUTION_KEYS = (
    "direct",
    "bounded",
    "cross-build",
    "inlined-or-conflicting",
    "external/synthetic",
    "gap",
)


def function_inventory(
    repo_dir: Path, ghidra_functions: list[dict[str, str]]
) -> list[dict[str, str]]:
    """Generated original-function inventory with source marker ownership overlaid."""

    values = [
        {
            "address": item["entry"],
            "symbol": item["name"],
            "owner": "",
            "source_path": "",
        }
        for item in ghidra_functions
    ]
    from ..source_index import source_functions

    by_address = {int(item["address"], 16): item for item in values}
    for address, function in source_functions(repo_dir).items():
        by_address[address] = {
            "address": f"{address:08x}",
            "symbol": function.name,
            "owner": "surrender-template" if function.marker_kind == "TEMPLATE" else "source",
            "source_path": function.source_file,
        }
    return list(by_address.values())


def _csv(rows: list[dict[str, str]], fields: list[str]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def render_interval_csv(intervals: list[TranslationUnitInterval]) -> str:
    rows: list[dict[str, str]] = []
    for index, interval in enumerate(intervals):
        rows.append(
            {
                "record_type": "translation-unit",
                "lower_address": _address(interval.lower),
                "upper_address": _address(interval.upper),
                "bounds": "inclusive",
                "source_path": interval.source_path,
                "anchor_function_count": str(len(interval.anchors)),
                "lower_anchor": _address(interval.lower),
                "upper_anchor": _address(interval.upper),
                "previous_source_path": "",
                "next_source_path": "",
            }
        )
        if index + 1 == len(intervals):
            continue
        following = intervals[index + 1]
        rows.append(
            {
                "record_type": "gap",
                "lower_address": _address(interval.upper),
                "upper_address": _address(following.lower),
                "bounds": "exclusive",
                "source_path": "",
                "anchor_function_count": "0",
                "lower_anchor": _address(interval.upper),
                "upper_anchor": _address(following.lower),
                "previous_source_path": interval.source_path,
                "next_source_path": following.source_path,
            }
        )
    return _csv(
        rows,
        [
            "record_type",
            "lower_address",
            "upper_address",
            "bounds",
            "source_path",
            "anchor_function_count",
            "lower_anchor",
            "upper_anchor",
            "previous_source_path",
            "next_source_path",
        ],
    )


def _anchor_label(anchor: dict[str, Any] | None) -> str:
    if not anchor:
        return "-"
    label = f"{anchor['function']} {anchor['source_path'].rsplit('\\', 1)[-1]}"
    if anchor.get("line") is not None:
        label += f":{anchor['line']}"
    return label


def _owner_evidence(owner: dict[str, Any]) -> str:
    evidence = owner.get("evidence") or []
    if not evidence:
        if owner.get("attribution") != "gap":
            return str(owner.get("attribution") or "")
        previous = owner.get("previous_hard_unit") or {}
        following = owner.get("next_hard_unit") or {}
        anchors = owner.get("nearest_anchors") or {}
        size = owner.get("gap_size")
        return (
            f"gap 0x{previous.get('upper', '?')}..0x{following.get('lower', '?')} "
            f"({'-' if size is None else hex(size)} bytes) "
            f"after {previous.get('source_path', '?')}, before {following.get('source_path', '?')}; "
            f"nearest anchors {_anchor_label(anchors.get('previous'))} / "
            f"{_anchor_label(anchors.get('next'))}"
        )
    parts = []
    for item in evidence:
        if not isinstance(item, dict):
            continue
        if item.get("evidence") == "hard-hull":
            parts.append(f"hard hull {item['lower']}-{item['upper']}")
            continue
        text = str(item.get("evidence"))
        if item.get("function"):
            text += f" at {item['function']}"
        if item.get("line") is not None:
            text += f":{item['line']}"
        if item.get("origin_variant"):
            text += f" from {item['origin_variant']} {item.get('origin_function', '')}".rstrip()
            if item.get("match"):
                text += f" ({item['match']}"
                if item.get("score") is not None:
                    text += f", {item['score']}"
                text += ")"
        parts.append(text)
    return "; ".join(parts)


def _bounds(owner: dict[str, Any], address: int) -> tuple[str, str, str]:
    if owner.get("interval_lower") and owner.get("interval_upper"):
        return str(owner["interval_lower"]), str(owner["interval_upper"]), "inclusive"
    if owner.get("attribution") == "gap":
        previous = owner.get("previous_hard_unit") or {}
        following = owner.get("next_hard_unit") or {}
        return str(previous.get("upper") or ""), str(following.get("lower") or ""), "exclusive"
    return _address(address), _address(address), "source-marker"


def render_gameplay_map_csv(
    layout: TranslationUnitLayout,
    gameplay: list[dict[str, str]],
) -> tuple[str, dict[str, int]]:
    rows: list[dict[str, str]] = []
    counts = {key: 0 for key in ATTRIBUTION_KEYS}
    for function in sorted(gameplay, key=lambda row: int(row["address"], 16)):
        address = int(function["address"], 16)
        if function.get("source_path"):
            owner: dict[str, Any] = {
                "source_path": function["source_path"],
                "attribution": "direct",
                "evidence": [{"evidence": "physical source file owning the compiler-bound marker"}],
            }
        elif function["owner"] == "surrender-template":
            owner = {
                "source_path": "",
                "attribution": "external/synthetic",
                "evidence": [
                    {
                        "evidence": (
                            "SurRender template body; reviewed vendor ownership overrides "
                            "address-range inference"
                        )
                    }
                ],
            }
        else:
            owner = layout.owner(address)
        attribution = str(owner.get("attribution") or "gap")
        lower, upper, bounds = _bounds(owner, address)
        counts[attribution] = counts.get(attribution, 0) + 1
        rows.append(
            {
                "address": _address(address),
                "symbol": function["symbol"],
                "source_path": str(owner.get("source_path") or ""),
                "attribution": attribution,
                "interval_lower": lower,
                "interval_upper": upper,
                "bounds": bounds,
                "evidence": _owner_evidence(owner),
            }
        )
    return (
        _csv(
            rows,
            [
                "address",
                "symbol",
                "source_path",
                "attribution",
                "interval_lower",
                "interval_upper",
                "bounds",
                "evidence",
            ],
        ),
        counts,
    )


ORIGINAL_UNIT_FIELDS = [
    "rank",
    "original_path",
    "status",
    "recovered_file",
    "hull_lower",
    "hull_upper",
    "span_bytes",
    "anchor_functions",
    "hull_functions",
    "recovered_in_unit",
    "recovered_anchored",
    "misplaced",
    "unrecovered",
    "claimed_outside_hull",
]

MISPLACED_FIELDS = [
    "address",
    "symbol",
    "current_source_path",
    "current_class",
    "owner_original_path",
    "expected_recovered_file",
    "hull_lower",
    "hull_upper",
    "attribution",
]


def original_unit_rows(
    repo_dir: Path,
    layout: TranslationUnitLayout,
    gameplay: list[dict[str, str]],
) -> list[dict[str, str]]:
    """One status row per original ``.cpp`` in the evidence source tree."""
    from ..source_units import SOURCE_TREE_PATH, mapped_repository_source_file

    tree = repo_dir / SOURCE_TREE_PATH
    originals: list[str] = []
    if tree.is_file():
        with tree.open(newline="", encoding="utf-8") as stream:
            for row in csv.DictReader(stream):
                relative = (row.get("relative_path") or "").strip().replace("/", "\\")
                if relative.casefold().endswith(".cpp"):
                    originals.append(relative)
    hulls = {interval.source_path: interval for interval in layout.intervals}
    assertion_functions = {
        unit: {anchor.function for anchor in anchors if anchor.evidence == "assertion"}
        for unit, anchors in layout.anchors_by_unit.items()
    }
    by_recovered_file: dict[str, list[dict[str, str]]] = {}
    for function in gameplay:
        if function.get("source_path"):
            by_recovered_file.setdefault(function["source_path"], []).append(function)

    rows: list[dict[str, str]] = []
    for original_path in originals:
        recovered_file = mapped_repository_source_file(repo_dir, original_path) or ""
        interval = hulls.get(original_path)
        members = []
        if interval is not None:
            members = [
                function
                for function in gameplay
                if function["owner"] != "surrender-template"
                and interval.lower <= int(function["address"], 16) <= interval.upper
            ]
        recovered_in_unit = [
            f for f in members if recovered_file and f.get("source_path") == recovered_file
        ]
        unit_assertions = assertion_functions.get(original_path, set())
        recovered_anchored = [
            f for f in recovered_in_unit if int(f["address"], 16) in unit_assertions
        ]
        misplaced = [
            f for f in members if f.get("source_path") and f["source_path"] != recovered_file
        ]
        unrecovered = [f for f in members if not f.get("source_path")]
        if recovered_file:
            status = "recovered-original-tu"
        elif interval is None:
            status = "evidence-insufficient"
        elif misplaced:
            status = "partially-represented"
        else:
            status = "absent"
        claimed = by_recovered_file.get(recovered_file, []) if recovered_file else []
        if interval is not None:
            claimed = [
                f
                for f in claimed
                if not (interval.lower <= int(f["address"], 16) <= interval.upper)
            ]
        claimed_outside = len(claimed)
        rows.append(
            {
                "rank": "",
                "original_path": original_path,
                "status": status,
                "recovered_file": recovered_file,
                "hull_lower": _address(interval.lower) if interval is not None else "",
                "hull_upper": _address(interval.upper) if interval is not None else "",
                "span_bytes": (
                    str(interval.upper - interval.lower) if interval is not None else ""
                ),
                "anchor_functions": str(len(interval.anchors) if interval is not None else 0),
                "hull_functions": str(len(members)),
                "recovered_in_unit": str(len(recovered_in_unit)),
                "recovered_anchored": str(len(recovered_anchored)),
                "misplaced": str(len(misplaced)),
                "unrecovered": str(len(unrecovered)),
                "claimed_outside_hull": str(claimed_outside),
            }
        )
    missing = [row for row in rows if row["status"] != "recovered-original-tu"]
    missing.sort(
        key=lambda row: (
            -(int(row["misplaced"]) + int(row["unrecovered"])),
            -int(row["anchor_functions"]),
            -int(row["span_bytes"] or 0),
            row["original_path"].casefold(),
        )
    )
    for rank, row in enumerate(missing, start=1):
        row["rank"] = str(rank)
    recovered = sorted(
        (row for row in rows if row["status"] == "recovered-original-tu"),
        key=lambda row: row["original_path"].casefold(),
    )
    return [*missing, *recovered]


def misplaced_function_rows(
    repo_dir: Path,
    layout: TranslationUnitLayout,
    gameplay: list[dict[str, str]],
) -> list[dict[str, str]]:
    """Gameplay functions inside a hard hull whose recovered file is elsewhere."""
    from ..source_units import mapped_repository_source_file, source_unit_records

    records = source_unit_records(repo_dir)
    rows: list[dict[str, str]] = []
    for function in gameplay:
        if function["owner"] == "surrender-template" or not function.get("source_path"):
            continue
        address = int(function["address"], 16)
        interval = layout.interval_at(address)
        if interval is None:
            continue
        expected = mapped_repository_source_file(repo_dir, interval.source_path) or ""
        if function["source_path"] == expected:
            continue
        rows.append(
            {
                "address": function["address"],
                "symbol": function["symbol"],
                "current_source_path": function["source_path"],
                "current_class": records.get(function["source_path"], {}).get("class", ""),
                "owner_original_path": interval.source_path,
                "expected_recovered_file": expected,
                "hull_lower": _address(interval.lower),
                "hull_upper": _address(interval.upper),
                "attribution": str(layout.owner(address).get("attribution") or ""),
            }
        )
    rows.sort(key=lambda row: int(row["address"], 16))
    return rows


def assertion_only_layout(repo_dir: Path) -> TranslationUnitLayout:
    assertions = _read_rows(repo_dir / "evidence/observations/wiz8/assertions.csv")
    units, headers = assertion_anchors(assertions)
    return TranslationUnitLayout(units, header_anchors=headers)


def _read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def translation_unit_report(
    settings: Any, layout: TranslationUnitLayout | None = None
) -> dict[str, Any]:
    from ..ghidra.query import function_inventory as ghidra_function_inventory

    if layout is None:
        layout = assertion_only_layout(settings.repo_dir)
    gameplay = function_inventory(settings.repo_dir, ghidra_function_inventory(settings))
    intervals = layout.projection()
    interval_csv = render_interval_csv(intervals)
    gameplay_csv, counts = render_gameplay_map_csv(layout, gameplay)

    unit_rows = original_unit_rows(settings.repo_dir, layout, gameplay)
    misplaced_rows = misplaced_function_rows(settings.repo_dir, layout, gameplay)

    report_dir = settings.build_dir / "reports" / "translation-units"
    interval_path = report_dir / "translation-unit-intervals.csv"
    gameplay_path = report_dir / "gameplay-translation-units.csv"
    original_path = report_dir / "original-translation-units.csv"
    misplaced_path = report_dir / "misplaced-functions.csv"
    atomic_write(interval_path, interval_csv)
    atomic_write(gameplay_path, gameplay_csv)
    atomic_write(original_path, _csv(unit_rows, ORIGINAL_UNIT_FIELDS))
    atomic_write(misplaced_path, _csv(misplaced_rows, MISPLACED_FIELDS))
    status_counts = {
        key: len([row for row in unit_rows if row["status"] == key])
        for key in (
            "recovered-original-tu",
            "partially-represented",
            "absent",
            "evidence-insufficient",
        )
    }
    return {
        "translation_units": len(intervals),
        "gaps": max(len(intervals) - 1, 0),
        "gameplay_functions": len(gameplay),
        "attribution": counts,
        "anchors": {
            "unit_anchors": len(layout.unit_anchors),
            "conflicting_functions": len(layout.conflicts),
            "header_anchors": len(layout.header_anchors),
        },
        "original_units": status_counts,
        "misplaced_functions": len(misplaced_rows),
        "top_missing": [row["original_path"] for row in unit_rows if row["rank"]][:10],
        "outputs": [
            str(interval_path.relative_to(settings.repo_dir)),
            str(gameplay_path.relative_to(settings.repo_dir)),
            str(original_path.relative_to(settings.repo_dir)),
            str(misplaced_path.relative_to(settings.repo_dir)),
        ],
    }
