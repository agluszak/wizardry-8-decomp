"""Segment the data sections by translation unit with an order-constrained fit.

The linker preserved object order, so a translation unit's globals cluster in
each data section in the same order its functions cluster in ``.text``. This
report fits the known ``.text`` unit order to each data storage class and
attributes every censused global to a unit interval:

1. Every global's referencing functions come from the per-reference globals
   report; each function maps to a unit through the assertion-anchored
   ``.text`` interval map. A global whose referencing functions name exactly
   one unit is a **single-unit baseline** observation.
2. Per storage class (``.rdata`` initialized, ``.data`` initialized, ``.data``
   bss), baseline globals sorted by address should visit units in ``.text``
   order. A longest non-decreasing subsequence keeps the order-consistent
   majority and drops extern-reference outliers.
3. Units whose baseline globals are mostly dropped are utility units
   referenced from everywhere; they are excluded and the fit reruns without
   them.
4. Surviving globals bound one address interval per unit and storage class,
   and every censused global falling inside an interval is attributed to it.

Known failure modes, deliberate and reported rather than smoothed away:

- **Extern outliers.** A global referenced from one *other* unit attributes
  to the consumer, not the definer; the LIS drop is what catches most.
- **Utility units.** Units like UtilityFunctions.cpp own few data globals but
  reference everything; the exclusion pass keeps them from shredding the fit.
- **Anchorless units.** Units with no asserting functions have no ``.text``
  interval, so their functions attribute no globals and their data intervals
  are silently absorbed by their neighbours' gaps - attribution confidence is
  ``interval`` (bounded), never ``proven``.
- **Merged and pooled data.** VC6 pools identical string literals and merges
  some constants across objects; a pooled address genuinely belongs to more
  than one unit, and single-unit baselines misstate it.

Everything here is a generated projection under ``build/reports/``; nothing
is written into ``evidence/``.
"""

from __future__ import annotations

import csv
import io
from bisect import bisect_right
from collections import defaultdict
from itertools import pairwise
from typing import Any

from ..paths import atomic_json, atomic_write

_REPORT_NAME = "data-segmentation"


def storage_class(row: dict[str, str]) -> str:
    return f"{row['section']}/{row['storage']}"


def unit_lookup(intervals: list[Any]) -> Any:
    """Map a function entry to its owning unit through the interval list."""

    lows = [interval.lower for interval in intervals]

    def lookup(entry: int) -> str | None:
        index = bisect_right(lows, entry) - 1
        if index >= 0 and entry <= intervals[index].upper:
            return intervals[index].source_path
        return None

    return lookup


def single_unit_baseline(
    references: list[dict[str, str]], lookup: Any
) -> dict[int, str]:
    """Globals whose referencing functions all sit in exactly one unit."""

    units_by_global: dict[int, set[str]] = defaultdict(set)
    for row in references:
        if not row["function_start"]:
            continue
        unit = lookup(int(row["function_start"], 16))
        if unit is not None:
            units_by_global[int(row["target"], 16)].add(unit)
    return {
        target: next(iter(units))
        for target, units in units_by_global.items()
        if len(units) == 1
    }


def longest_non_decreasing(
    indices: list[int], weights: list[int] | None = None
) -> list[int]:
    """Positions of one longest non-decreasing subsequence.

    When two chains keep the same number of elements, the one whose kept
    elements carry more total weight wins - the fit passes each element's
    unit population, so ties resolve toward the units that actually own the
    surrounding run instead of an arbitrary interleaved minority.
    """

    if not indices:
        return []
    if weights is None:
        weights = [1] * len(indices)
    length = [1] * len(indices)
    weight = list(weights)
    parents = [-1] * len(indices)
    for position, value in enumerate(indices):
        for earlier in range(position):
            if indices[earlier] <= value:
                candidate = (length[earlier] + 1, weight[earlier] + weights[position])
                if candidate > (length[position], weight[position]):
                    length[position], weight[position] = candidate
                    parents[position] = earlier
    best = max(range(len(indices)), key=lambda index: (length[index], weight[index]))
    chain: list[int] = []
    while best >= 0:
        chain.append(best)
        best = parents[best]
    return chain[::-1]


def fit_storage_class(
    baseline: list[tuple[int, str]], unit_order: dict[str, int]
) -> dict[str, Any]:
    """Order-constrained fit for one storage class.

    ``baseline`` is (address, unit) sorted by address. Returns surviving
    per-unit intervals, dropped outliers, and the utility units excluded
    because most of their baseline observations fought the global order.
    """

    excluded: set[str] = set()
    while True:
        candidates = [
            (address, unit)
            for address, unit in baseline
            if unit not in excluded
        ]
        population: dict[str, int] = defaultdict(int)
        for _, unit in candidates:
            population[unit] += 1
        indices = [unit_order[unit] for _, unit in candidates]
        kept_positions = set(
            longest_non_decreasing(
                indices, [population[unit] for _, unit in candidates]
            )
        )
        dropped_by_unit: dict[str, int] = defaultdict(int)
        total_by_unit: dict[str, int] = defaultdict(int)
        for position, (_, unit) in enumerate(candidates):
            total_by_unit[unit] += 1
            if position not in kept_positions:
                dropped_by_unit[unit] += 1
        newly_excluded = {
            unit
            for unit, total in total_by_unit.items()
            if total >= 4 and dropped_by_unit[unit] / total > 0.5
        }
        if not newly_excluded:
            break
        excluded |= newly_excluded

    intervals: dict[str, list[int]] = {}
    outliers: list[tuple[int, str]] = []
    kept_by_unit: dict[str, int] = defaultdict(int)
    for position, (address, unit) in enumerate(candidates):
        if position in kept_positions:
            bounds = intervals.setdefault(unit, [address, address])
            bounds[0] = min(bounds[0], address)
            bounds[1] = max(bounds[1], address)
            kept_by_unit[unit] += 1
        else:
            outliers.append((address, unit))
    ordered = sorted(intervals.items(), key=lambda item: item[1][0])
    for (_, left), (unit, right) in pairwise(ordered):
        if right[0] <= left[1]:
            raise ValueError(f"fitted unit intervals overlap at {unit}")
    return {
        "intervals": ordered,
        "outliers": outliers,
        "excluded_units": sorted(excluded),
        "kept": len(kept_positions),
        "kept_by_unit": dict(kept_by_unit),
        "dropped": len(candidates) - len(kept_positions),
    }


def attribute_globals(
    globals_rows: list[dict[str, str]],
    fits: dict[str, dict[str, Any]],
    baseline: dict[int, str],
) -> tuple[list[dict[str, str]], dict[str, int]]:
    """Attribute every censused global to a fitted unit interval."""

    lookup_by_class: dict[str, tuple[list[int], list[tuple[str, list[int]]]]] = {}
    for name, fit in fits.items():
        ordered = fit["intervals"]
        lookup_by_class[name] = ([bounds[0] for _, bounds in ordered], ordered)

    rows: list[dict[str, str]] = []
    counts = {"attributed": 0, "unattributed": 0, "excluded-import-slot": 0}
    agreement = {"agree": 0, "disagree": 0}
    for row in globals_rows:
        address = int(row["address"], 16)
        if row["kind"] == "import-slot":
            counts["excluded-import-slot"] += 1
            continue
        name = storage_class(row)
        unit = ""
        if name in lookup_by_class:
            lows, ordered = lookup_by_class[name]
            index = bisect_right(lows, address) - 1
            if index >= 0 and address <= ordered[index][1][1]:
                unit = ordered[index][0]
        counts["attributed" if unit else "unattributed"] += 1
        observed = baseline.get(address, "")
        if unit and observed:
            agreement["agree" if observed == unit else "disagree"] += 1
        rows.append(
            {
                "address": f"{address:08x}",
                "storage_class": name,
                "kind": row["kind"],
                "unit": unit,
                "confidence": "interval" if unit else "",
                "baseline_unit": observed,
            }
        )
    counts.update(agreement)
    return rows, counts


_SNAPSHOT_FILE = "unit-data-intervals.csv"


def _snapshot_readme() -> str:
    return """# Data-segmentation snapshot

`unit-data-intervals.csv` holds the fitted per-translation-unit address
intervals for each data storage class of the canonical retail program, keyed
`program,storage_class,unit,lower,upper,baseline_globals`.

Producer: `uv run wiz8 report data-segmentation --update-snapshot`. The fit
joins the per-reference globals report (which needs the proprietary binaries in
`WIZ8_WORK_DIR`) with the assertion-anchored `.text` interval map; the snapshot
exists so the candidate replay can attribute globals to units from tracked
inputs alone. Every run without `--update-snapshot` regenerates the table under
`build/reports/data-segmentation/` and fails if it differs from this snapshot.

The intervals are an order-constrained fit, not reviewed identity: attribution
derived from them is bounded evidence, and the per-global table plus outlier
and exclusion detail stay under `build/reports/data-segmentation/`.
"""


def data_segmentation_report(
    settings: Any, *, update_snapshot: bool = False
) -> dict[str, Any]:
    from ..data_globals import sweep_globals
    from .translation_units import call_site_anchors, derive_intervals

    # Regenerate the per-reference report and verify snapshot freshness.
    sweep = sweep_globals(settings)
    references_path = (
        settings.build_dir / "reports" / "globals" / "references.csv"
    )
    with references_path.open(newline="", encoding="utf-8") as stream:
        all_references = list(csv.DictReader(stream))

    assertions_path = (
        settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    )
    snapshot_path = (
        settings.repo_dir / "evidence" / "snapshots" / "call-sites" / "assertions.csv"
    )
    with assertions_path.open(newline="", encoding="utf-8") as stream:
        assertions = list(csv.DictReader(stream))
    with snapshot_path.open(newline="", encoding="utf-8") as stream:
        call_sites = list(csv.DictReader(stream))
    program = next(
        row["program"] for row in call_sites if "--gog-base--" in row["program"]
    )
    intervals = derive_intervals(assertions, call_site_anchors(call_sites, program))
    order = {
        interval.source_path: index for index, interval in enumerate(intervals)
    }
    lookup = unit_lookup(intervals)

    references = [row for row in all_references if row["program"] == program]
    globals_path = (
        settings.repo_dir / "evidence" / "snapshots" / "globals" / "globals.csv"
    )
    with globals_path.open(newline="", encoding="utf-8") as stream:
        globals_rows = [
            row for row in csv.DictReader(stream) if row["program"] == program
        ]
    by_address = {int(row["address"], 16): row for row in globals_rows}

    baseline = single_unit_baseline(references, lookup)
    per_class: dict[str, list[tuple[int, str]]] = defaultdict(list)
    for address, unit in baseline.items():
        row = by_address.get(address)
        if row is None or row["kind"] == "import-slot":
            continue
        per_class[storage_class(row)].append((address, unit))

    fits = {
        name: fit_storage_class(sorted(observations), order)
        for name, observations in sorted(per_class.items())
    }
    attribution, counts = attribute_globals(globals_rows, fits, baseline)

    report_dir = settings.build_dir / "reports" / _REPORT_NAME
    interval_stream = io.StringIO(newline="")
    writer = csv.writer(interval_stream, lineterminator="\n")
    writer.writerow(
        ["program", "storage_class", "unit", "lower", "upper", "baseline_globals"]
    )
    for name, fit in fits.items():
        for unit, (lower, upper) in fit["intervals"]:
            writer.writerow(
                [
                    program,
                    name,
                    unit,
                    f"{lower:08x}",
                    f"{upper:08x}",
                    fit["kept_by_unit"].get(unit, 0),
                ]
            )
    interval_csv = interval_stream.getvalue()
    atomic_write(report_dir / _SNAPSHOT_FILE, interval_csv)

    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / _REPORT_NAME
    if update_snapshot:
        atomic_write(snapshot_dir / _SNAPSHOT_FILE, interval_csv)
        atomic_write(snapshot_dir / "README.md", _snapshot_readme())
    interval_snapshot_fresh = (
        snapshot_dir / _SNAPSHOT_FILE
    ).is_file() and (snapshot_dir / _SNAPSHOT_FILE).read_text(
        encoding="utf-8"
    ) == interval_csv
    if not update_snapshot and not interval_snapshot_fresh:
        raise RuntimeError(
            "unit data intervals differ from the tracked snapshot; review "
            f"build/reports/{_REPORT_NAME} and rerun with --update-snapshot"
        )

    attribution_stream = io.StringIO(newline="")
    writer = csv.DictWriter(
        attribution_stream,
        fieldnames=[
            "address",
            "storage_class",
            "kind",
            "unit",
            "confidence",
            "baseline_unit",
        ],
        lineterminator="\n",
    )
    writer.writeheader()
    writer.writerows(attribution)
    atomic_write(report_dir / "global-attribution.csv", attribution_stream.getvalue())

    summary = {
        "schema": "wiz8.data-segmentation",
        "program": program,
        "globals_snapshot_fresh": bool(sweep.get("snapshot_fresh", True)),
        "interval_snapshot_fresh": interval_snapshot_fresh,
        "interval_snapshot_updated": update_snapshot,
        "baseline_globals": len(baseline),
        "storage_classes": {
            name: {
                "units": len(fit["intervals"]),
                "kept": fit["kept"],
                "dropped_outliers": fit["dropped"],
                "excluded_units": fit["excluded_units"],
            }
            for name, fit in fits.items()
        },
        "attribution": counts,
        "outputs": [
            f"build/reports/{_REPORT_NAME}/unit-data-intervals.csv",
            f"build/reports/{_REPORT_NAME}/global-attribution.csv",
        ],
    }
    atomic_json(report_dir / "summary.json", summary)
    return summary
