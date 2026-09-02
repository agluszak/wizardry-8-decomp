from __future__ import annotations

import csv
import io
from pathlib import Path
from typing import Any

# The interval derivation is shared with Ghidra-backed recovery reports.
from ..ghidra.unit_intervals import (
    TranslationUnitInterval,
    _address,
    derive_intervals,
)
from ..ghidra.unit_intervals import source_path as _source_path
from ..paths import atomic_write

__all__ = [
    "TranslationUnitInterval",
    "derive_intervals",
    "function_inventory",
    "render_gameplay_map_csv",
    "render_interval_csv",
    "translation_unit_report",
]


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
    from ..source_model import build_source_model

    by_address = {int(item["address"], 16): item for item in values}
    for address, function in build_source_model(repo_dir).functions.items():
        by_address[address] = {
            "address": f"{address:08x}",
            "symbol": function.name,
            "owner": "surrender-template" if function.kind == "TEMPLATE" else "source",
            "source_path": function.file,
        }
    return list(by_address.values())


def _read_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


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


def render_gameplay_map_csv(
    assertions: list[dict[str, str]],
    gameplay: list[dict[str, str]],
    intervals: list[TranslationUnitInterval],
    extra_anchors: dict[int, str] | None = None,
) -> tuple[str, dict[str, int]]:
    direct: dict[int, str] = {}
    reviewed: set[int] = set()
    for row in assertions:
        source_path = _source_path(row["source_path"])
        if source_path is not None and row["containing_function"]:
            anchor = int(row["containing_function"], 16)
            direct[anchor] = source_path
            reviewed.add(anchor)
    for anchor, source_path in (extra_anchors or {}).items():
        direct.setdefault(anchor, source_path)

    rows: list[dict[str, str]] = []
    counts = {"direct": 0, "inferred": 0, "gap": 0, "external": 0}
    for function in sorted(gameplay, key=lambda row: int(row["address"], 16)):
        address = int(function["address"], 16)
        containing = next(
            (interval for interval in intervals if interval.lower <= address <= interval.upper),
            None,
        )
        if function.get("source_path"):
            attribution = "direct"
            source_path = function["source_path"]
            lower = address
            upper = address
            bounds = "source-marker"
            evidence = "physical source file owning the compiler-bound marker"
        elif function["owner"] == "surrender-template":
            attribution = "external"
            source_path = ""
            lower = 0
            upper = 0
            bounds = ""
            evidence = (
                "SurRender template body; reviewed vendor ownership overrides "
                "address-range inference"
            )
        elif address in direct:
            assert containing is not None and containing.source_path == direct[address]
            attribution = "direct"
            source_path = direct[address]
            lower = containing.lower
            upper = containing.upper
            bounds = "inclusive"
            evidence = "the function itself contains an assertion naming this source path"
            if address not in reviewed:
                evidence += "; anchor recovered statically from the call-site snapshot"
        elif containing is not None:
            attribution = "inferred"
            source_path = containing.source_path
            lower = containing.lower
            upper = containing.upper
            bounds = "inclusive"
            evidence = "function start lies inside this unit's assertion-bounded interval"
        else:
            attribution = "gap"
            source_path = ""
            previous = next((item for item in reversed(intervals) if item.upper < address), None)
            following = next((item for item in intervals if item.lower > address), None)
            lower = previous.upper if previous is not None else 0
            upper = following.lower if following is not None else 0
            bounds = "exclusive"
            previous_path = previous.source_path if previous is not None else "start of .text"
            following_path = following.source_path if following is not None else "end of .text"
            evidence = (
                "outside every assertion-bounded interval; between "
                f"{previous_path} and {following_path}"
            )
        counts[attribution] += 1
        rows.append(
            {
                "address": _address(address),
                "symbol": function["symbol"],
                "source_path": source_path,
                "attribution": attribution,
                "interval_lower": _address(lower) if lower else "",
                "interval_upper": _address(upper) if upper else "",
                "bounds": bounds,
                "evidence": evidence,
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


def translation_unit_report(settings: Any) -> dict[str, Any]:
    from ..ghidra.query import function_inventory as ghidra_function_inventory

    assertions = _read_rows(
        settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    )
    gameplay = function_inventory(settings.repo_dir, ghidra_function_inventory(settings))
    intervals = derive_intervals(assertions)
    interval_csv = render_interval_csv(intervals)
    gameplay_csv, counts = render_gameplay_map_csv(assertions, gameplay, intervals)

    report_dir = settings.build_dir / "reports" / "translation-units"
    interval_path = report_dir / "translation-unit-intervals.csv"
    gameplay_path = report_dir / "gameplay-translation-units.csv"
    atomic_write(interval_path, interval_csv)
    atomic_write(gameplay_path, gameplay_csv)
    reviewed_anchors = {
        int(row["containing_function"], 16)
        for row in assertions
        if _source_path(row["source_path"]) is not None and row["containing_function"]
    }
    return {
        "translation_units": len(intervals),
        "gaps": max(len(intervals) - 1, 0),
        "gameplay_functions": len(gameplay),
        "attribution": counts,
        "anchors": {
            "reviewed": len(reviewed_anchors),
        },
        "outputs": [
            str(interval_path.relative_to(settings.repo_dir)),
            str(gameplay_path.relative_to(settings.repo_dir)),
        ],
    }
