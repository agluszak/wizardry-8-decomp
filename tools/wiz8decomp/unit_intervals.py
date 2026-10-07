"""Translation-unit layout derived from reviewed retail assertion paths."""

from __future__ import annotations

import bisect
import csv
from collections import defaultdict
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from itertools import pairwise
from pathlib import Path
from typing import Any

DEFAULT_ROOTS: tuple[str, ...] = ("C:\\Projects\\Wizardry 8\\", "E:\\Wizardry 8\\")
UNIT_SUFFIXES = (".cpp", ".c")
HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")


class TranslationUnitContradiction(ValueError):
    """The anchor set violates the contiguous translation-unit invariant."""


@dataclass(frozen=True)
class UnitAnchor:
    function: int
    source_path: str
    line: int | None = None

    def describe(self) -> dict[str, Any]:
        value: dict[str, Any] = {"evidence": "assertion", "function": _address(self.function)}
        if self.line is not None:
            value["line"] = self.line
        return value


@dataclass(frozen=True)
class TranslationUnitInterval:
    source_path: str
    lower: int
    upper: int
    anchors: tuple[int, ...]


def _address(value: int) -> str:
    return f"{value:08x}"


def classify_path(value: str, roots: Sequence[str] = DEFAULT_ROOTS) -> tuple[str, str] | None:
    """Classify an assertion path as a unit or a header, relative to the source root.

    Relative include spellings such as ``..\\Engine Code\\Include\\AnimRep.hpp`` are
    header-origin evidence, never translation-unit anchors.
    """

    relative = next((value[len(root) :] for root in roots if value.startswith(root)), None)
    if relative is None and value.startswith(("..\\", "../")):
        relative = value
    if relative is None:
        return None
    folded = relative.casefold()
    if folded.endswith(UNIT_SUFFIXES):
        return ("unit", relative)
    if folded.endswith(HEADER_SUFFIXES):
        return ("header", relative)
    return None


def assertion_anchors(assertions: Iterable[dict[str, str]]) -> list[UnitAnchor]:
    """Reviewed assertion rows naming a ``.cpp``/``.c`` unit, as direct anchors."""

    units: list[UnitAnchor] = []
    for row in assertions:
        value = (row.get("source_path") or "").strip()
        if not value or not row.get("containing_function"):
            continue
        classified = classify_path(value)
        if classified is None or classified[0] != "unit":
            continue
        function = int(row["containing_function"], 16)
        line = int(row["line"]) if (row.get("line") or "").strip() else None
        units.append(UnitAnchor(function, classified[1], line))
    return units


def read_assertions(repo_dir: Path) -> list[dict[str, str]]:
    path = repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


class TranslationUnitLayout:
    """Direct anchors, hard hulls and gaps over one program's function entries.

    Ordinary non-COMDAT functions of one translation unit occupy one contiguous
    ``.text`` contribution, so the convex hull of a unit's anchors is owned by
    that unit. Everything outside every hull is an explicit gap.
    """

    def __init__(self, anchors: Iterable[UnitAnchor]) -> None:
        self.unit_anchors = tuple(sorted(anchors, key=lambda a: (a.function, a.source_path)))
        by_function: dict[int, list[UnitAnchor]] = defaultdict(list)
        for anchor in self.unit_anchors:
            by_function[anchor.function].append(anchor)
        self.anchors_by_function = dict(by_function)
        self.conflicts = {
            function: tuple(sorted({anchor.source_path for anchor in anchors}))
            for function, anchors in by_function.items()
            if len({anchor.source_path for anchor in anchors}) > 1
        }
        by_unit: dict[str, list[int]] = defaultdict(list)
        for anchor in self.unit_anchors:
            if anchor.function not in self.conflicts:
                by_unit[anchor.source_path].append(anchor.function)
        self.intervals = sorted(
            (
                TranslationUnitInterval(unit, min(entries), max(entries), tuple(sorted(entries)))
                for unit, entries in by_unit.items()
            ),
            key=lambda interval: (interval.lower, interval.source_path.casefold()),
        )
        self._lowers = [interval.lower for interval in self.intervals]
        for previous, current in pairwise(self.intervals):
            if previous.upper >= current.lower:
                raise TranslationUnitContradiction(
                    "translation-unit anchors violate the contiguous-.text invariant: "
                    f"{previous.source_path} is hard-bounded at "
                    f"{_address(previous.lower)}-{_address(previous.upper)} but "
                    f"{current.source_path} anchors at {_address(current.lower)}"
                )

    def owner(self, entry: int) -> dict[str, Any]:
        """Resolve one function entry to its original translation unit."""

        if entry in self.conflicts:
            return {
                "source_path": "",
                "attribution": "inlined-or-conflicting",
                "alternatives": list(self.conflicts[entry]),
                "evidence": [anchor.describe() for anchor in self.anchors_by_function[entry]],
            }
        anchors = self.anchors_by_function.get(entry, ())
        if anchors:
            return {
                "source_path": anchors[0].source_path,
                "attribution": "direct",
                "alternatives": [],
                "evidence": [anchor.describe() for anchor in anchors],
            }
        index = bisect.bisect_right(self._lowers, entry) - 1
        if index >= 0 and self.intervals[index].lower <= entry <= self.intervals[index].upper:
            interval = self.intervals[index]
            return {
                "source_path": interval.source_path,
                "attribution": "bounded",
                "alternatives": [],
                "evidence": [
                    {
                        "evidence": "hard-hull",
                        "lower": _address(interval.lower),
                        "upper": _address(interval.upper),
                    }
                ],
            }
        return {"source_path": "", "attribution": "gap", "alternatives": [], "evidence": []}


def assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    return TranslationUnitLayout(assertion_anchors(read_assertions(repo_dir)))
