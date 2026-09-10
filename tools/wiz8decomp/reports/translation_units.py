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


def _owner_evidence(owner: dict[str, Any]) -> str:
    evidence = owner.get("evidence") or []
    if not evidence:
        return str(owner.get("attribution") or "")
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

    report_dir = settings.build_dir / "reports" / "translation-units"
    interval_path = report_dir / "translation-unit-intervals.csv"
    gameplay_path = report_dir / "gameplay-translation-units.csv"
    atomic_write(interval_path, interval_csv)
    atomic_write(gameplay_path, gameplay_csv)
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
        "outputs": [
            str(interval_path.relative_to(settings.repo_dir)),
            str(gameplay_path.relative_to(settings.repo_dir)),
        ],
    }
