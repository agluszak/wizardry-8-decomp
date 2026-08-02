"""Assertion-anchored translation-unit intervals over the ``.text`` order.

This is the pure core behind the translation-units report. It lives under
``ghidra/`` because recovery reports derive bounded unit attribution from it. The
sole input is the reviewed assertion observation table. Ghidra owns each call
site's containing function; no byte-scanned boundary projection is merged.
"""

from __future__ import annotations

from collections import defaultdict
from dataclasses import dataclass
from itertools import pairwise

SOURCE_PREFIX = "C:\\Projects\\Wizardry 8\\"


@dataclass(frozen=True)
class TranslationUnitInterval:
    source_path: str
    lower: int
    upper: int
    anchors: tuple[int, ...]


def _address(value: int) -> str:
    return f"{value:08x}"


def source_path(value: str) -> str | None:
    """The unit-relative spelling of an absolute canonical ``.cpp`` path."""

    if not value.startswith(SOURCE_PREFIX) or not value.casefold().endswith(".cpp"):
        return None
    return value[len(SOURCE_PREFIX) :]


def derive_intervals(assertions: list[dict[str, str]]) -> list[TranslationUnitInterval]:
    anchors_by_unit: dict[str, set[int]] = defaultdict(set)
    owners_by_anchor: dict[int, set[str]] = defaultdict(set)
    for row in assertions:
        unit = source_path(row["source_path"])
        # Sites outside any defined function record an empty containing
        # function and anchor nothing.
        if unit is None or not row["containing_function"]:
            continue
        anchor = int(row["containing_function"], 16)
        anchors_by_unit[unit].add(anchor)
        owners_by_anchor[anchor].add(unit)
    conflicting = {anchor: owners for anchor, owners in owners_by_anchor.items() if len(owners) > 1}
    if conflicting:
        formatted = ", ".join(
            f"{_address(anchor)}={sorted(owners)}" for anchor, owners in sorted(conflicting.items())
        )
        raise ValueError(f"assertion anchors have conflicting translation-unit owners: {formatted}")

    intervals = sorted(
        (
            TranslationUnitInterval(
                source_path=unit,
                lower=min(anchors),
                upper=max(anchors),
                anchors=tuple(sorted(anchors)),
            )
            for unit, anchors in anchors_by_unit.items()
        ),
        key=lambda interval: interval.lower,
    )
    for previous, current in pairwise(intervals):
        if previous.upper >= current.lower:
            raise ValueError(
                "assertion-bounded translation-unit intervals overlap: "
                f"{previous.source_path} ends at {_address(previous.upper)}, "
                f"{current.source_path} starts at {_address(current.lower)}"
            )
    return intervals


class TranslationUnitResolver:
    """Resolve one function entry to a source owner and explicit provenance."""

    def __init__(self, assertions: list[dict[str, str]]) -> None:
        self.assertions = assertions
        units_by_anchor: dict[int, set[str]] = defaultdict(set)
        for row in assertions:
            unit = source_path(row["source_path"])
            if unit is not None and row.get("containing_function"):
                units_by_anchor[int(row["containing_function"], 16)].add(unit)
        unambiguous = {anchor for anchor, units in units_by_anchor.items() if len(units) == 1}
        self.intervals = derive_intervals(
            [
                row
                for row in assertions
                if row.get("containing_function")
                and int(row["containing_function"], 16) in unambiguous
            ]
        )

    def resolve(self, entry: int) -> dict[str, object]:
        direct = sorted(
            {
                unit
                for row in self.assertions
                if row.get("containing_function")
                and int(row["containing_function"], 16) == entry
                and (unit := source_path(row["source_path"])) is not None
            },
            key=str.casefold,
        )
        if len(direct) == 1:
            return {"source_path": direct[0], "attribution": "direct", "alternatives": []}
        if len(direct) > 1:
            return {
                "source_path": "",
                "attribution": "inlined-or-conflicting",
                "alternatives": direct,
            }
        interval = next(
            (value for value in self.intervals if value.lower <= entry <= value.upper), None
        )
        if interval is None:
            return {"source_path": "", "attribution": "gap", "alternatives": []}
        return {
            "source_path": interval.source_path,
            "attribution": "interval-inference",
            "alternatives": [],
            "interval_lower": _address(interval.lower),
            "interval_upper": _address(interval.upper),
        }
