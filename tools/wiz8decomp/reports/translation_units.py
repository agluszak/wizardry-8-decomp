from __future__ import annotations

import csv
import io
from pathlib import Path
from typing import Any

# The interval derivation is shared with Ghidra-backed recovery reports.
from ..ghidra.unit_intervals import (
    TranslationUnitInterval,
    _address,
    call_site_anchors,
    derive_intervals,
)
from ..ghidra.unit_intervals import source_path as _source_path
from ..paths import atomic_write

__all__ = [
    "TranslationUnitInterval",
    "call_site_anchors",
    "derive_intervals",
    "load_call_site_anchors",
    "render_gameplay_map_csv",
    "render_interval_csv",
    "translation_unit_report",
]


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
        if function["owner"] == "surrender-template":
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


def _canonical_program(repo_dir: Path) -> str | None:
    """The program name whose addresses this report is written in terms of.

    Other builds carry the same source paths at different addresses, so mixing
    them would corrupt the interval map rather than extend it.
    """
    import yaml

    variants = repo_dir / "config" / "variants.yml"
    inventory = repo_dir / "build" / "manifests" / "modules.json"
    if not variants.is_file() or not inventory.is_file():
        return None
    import json

    variant = yaml.safe_load(variants.read_text(encoding="utf-8"))["canonical_matching_target"][
        "variant"
    ]
    from ..ghidra.project import program_name

    for module in json.loads(inventory.read_text(encoding="utf-8"))["modules"]:
        if module["variant"] == variant and module.get("classification") == "first-party-game":
            return program_name(module)
    return None


def load_call_site_anchors(repo_dir: Path) -> dict[int, str]:
    """Anchors contributed by the call-site snapshot, if it is available.

    Optional by design: the report predates the snapshot and still works without
    it. Shared so that every consumer of the attribution counts derives them from
    the same anchor set rather than reporting two numbers for one fact.
    """
    snapshot = repo_dir / "evidence" / "snapshots" / "call-sites" / "assertions.csv"
    program = _canonical_program(repo_dir)
    if not snapshot.is_file() or not program:
        return {}
    return call_site_anchors(_read_rows(snapshot), program)


def translation_unit_report(settings: Any) -> dict[str, Any]:
    assertions = _read_rows(
        settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    )
    gameplay = _read_rows(settings.repo_dir / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv")
    extra_anchors = load_call_site_anchors(settings.repo_dir)

    intervals = derive_intervals(assertions, extra_anchors)
    interval_csv = render_interval_csv(intervals)
    gameplay_csv, counts = render_gameplay_map_csv(assertions, gameplay, intervals, extra_anchors)

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
            "call_site_snapshot": len(extra_anchors),
            "added_by_snapshot": len(set(extra_anchors) - reviewed_anchors),
        },
        "outputs": [
            str(interval_path.relative_to(settings.repo_dir)),
            str(gameplay_path.relative_to(settings.repo_dir)),
        ],
    }
