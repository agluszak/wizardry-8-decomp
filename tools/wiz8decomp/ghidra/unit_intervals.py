"""Assertion-anchored translation-unit intervals over the ``.text`` order.

This is the pure core behind the translation-units report. It lives under
``ghidra/`` because the candidate replay derives bounded unit attribution from
it, and everything a replay imports must feed the materialization key. The
inputs are tracked: the reviewed assertion observations and the call-sites
snapshot.
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


def call_site_anchors(rows: list[dict[str, str]], program: str) -> dict[int, str]:
    """Function-to-unit anchors recovered statically from assertion call sites.

    The reviewed assertion table resolves its containing function through Ghidra
    and so covers only what has been imported; the call-site snapshot derives the
    enclosing function from inter-function padding and therefore covers the whole
    image. A function whose assertions name two units has been inlined into, so
    it anchors neither.
    """
    units_by_anchor: dict[int, set[str]] = defaultdict(set)
    for row in rows:
        if row.get("program") != program or not row.get("function_start"):
            continue
        unit = source_path(row["source_path"])
        if unit is None:
            continue
        units_by_anchor[int(row["function_start"], 16)].add(unit)
    return {
        anchor: next(iter(units)) for anchor, units in units_by_anchor.items() if len(units) == 1
    }


def derive_intervals(
    assertions: list[dict[str, str]], extra_anchors: dict[int, str] | None = None
) -> list[TranslationUnitInterval]:
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
    for anchor, unit in (extra_anchors or {}).items():
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
