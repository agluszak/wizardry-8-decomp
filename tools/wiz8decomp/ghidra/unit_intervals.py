"""Translation-unit layout derived from source-location evidence in the image.

The reviewed assertion table used to be the sole anchor source. Ghidra also
holds the original ``__FILE__``-style source-path strings and the code that
references them; a string reference from a function is a direct anchor for that
translation unit whether it feeds an assertion, an allocation macro or any other
diagnostic. This module collects those anchors, builds the conservative hard
hulls over them, and exposes one resolver for the report projections, recovery
context and source placement.

Only absolute paths below a known Wizardry source root anchor a translation
unit. Header paths (including relative ``..\\Engine Code\\Include\\*.hpp``
spellings) are header-origin inline code and are reported separately, never as
unit anchors. A function referencing two distinct ``.cpp`` paths is
``inlined-or-conflicting`` rather than silently assigned one unit.

The layout records the project's contiguous-TU invariant: ordinary non-COMDAT
functions emitted by one translation unit occupy one contiguous ``.text``
contribution, so the convex hull of a unit's direct anchors is hard-owned while
everything outside every hull stays an explicit gap. Hulls of distinct units
must not overlap; an overlap is a model contradiction, not something to paper
over.

Cross-build projection only extends a hard hull when the body match is unique.
The mnemonic-similarity fallback stays advisory: it can name a likely owner for
one function, but it never shrinks or grows an interval.
"""

from __future__ import annotations

import csv
from collections import defaultdict
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from itertools import pairwise
from pathlib import Path
from typing import Any

SOURCE_PREFIX = "C:\\Projects\\Wizardry 8\\"
DEFAULT_ROOTS: tuple[str, ...] = (SOURCE_PREFIX, "E:\\Wizardry 8\\")
UNIT_SUFFIXES = (".cpp", ".c")
HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")

ASSERTION = "assertion"
SOURCE_REFERENCE = "source-path-reference"
CROSS_BUILD = "cross-build"
SIMILAR_MATCH = "similar-body"

_DIRECT_EVIDENCE = (ASSERTION, SOURCE_REFERENCE)


class TranslationUnitContradiction(ValueError):
    """The anchor set violates the contiguous translation-unit invariant."""


@dataclass(frozen=True)
class UnitAnchor:
    function: int
    source_path: str
    evidence: str
    line: int | None = None
    origin_variant: str = ""
    origin_function: int | None = None
    match_kind: str = ""
    score: float | None = None

    def describe(self) -> dict[str, Any]:
        value: dict[str, Any] = {
            "evidence": self.evidence,
            "function": _address(self.function),
        }
        if self.line is not None:
            value["line"] = self.line
        if self.origin_variant:
            value["origin_variant"] = self.origin_variant
            if self.origin_function is not None:
                value["origin_function"] = _address(self.origin_function)
            if self.match_kind:
                value["match"] = self.match_kind
            if self.score is not None:
                value["score"] = round(self.score, 3)
        return value


@dataclass(frozen=True)
class HeaderAnchor:
    function: int
    header_path: str
    evidence: str
    line: int | None = None

    def describe(self) -> dict[str, Any]:
        value: dict[str, Any] = {
            "evidence": self.evidence,
            "function": _address(self.function),
        }
        if self.line is not None:
            value["line"] = self.line
        return value


@dataclass(frozen=True)
class TranslationUnitInterval:
    source_path: str
    lower: int
    upper: int
    anchors: tuple[int, ...]
    evidence: tuple[str, ...] = ()


def _address(value: int) -> str:
    return f"{value:08x}"


def source_path(value: str, roots: Sequence[str] = DEFAULT_ROOTS) -> str | None:
    """The unit-relative spelling of an absolute canonical ``.cpp`` path."""

    relative = _relative_source_path(value, roots)
    if relative is None or not relative.casefold().endswith(UNIT_SUFFIXES):
        return None
    return relative


def header_path(value: str, roots: Sequence[str] = DEFAULT_ROOTS) -> str | None:
    """The unit-relative spelling of a header/relative source path."""

    relative = _relative_source_path(value, roots)
    if relative is None or not relative.casefold().endswith(HEADER_SUFFIXES):
        return None
    return relative


def _relative_source_path(value: str, roots: Sequence[str]) -> str | None:
    for root in roots:
        if value.startswith(root):
            return value[len(root) :]
    # Relative include spellings such as ``..\Engine Code\Include\AnimRep.hpp``
    # are header-origin evidence, not translation-unit anchors.
    if value.startswith(("..\\", "../")):
        return value
    return None


def classify_path(value: str, roots: Sequence[str] = DEFAULT_ROOTS) -> tuple[str, str] | None:
    relative = _relative_source_path(value, roots)
    if relative is None:
        return None
    folded = relative.casefold()
    if folded.endswith(UNIT_SUFFIXES):
        return ("unit", relative)
    if folded.endswith(HEADER_SUFFIXES):
        return ("header", relative)
    return None


def assertion_anchors(
    assertions: Iterable[dict[str, str]],
) -> tuple[list[UnitAnchor], list[HeaderAnchor]]:
    """Reviewed assertion rows as direct anchors with their source lines."""

    units: list[UnitAnchor] = []
    headers: list[HeaderAnchor] = []
    for row in assertions:
        value = (row.get("source_path") or "").strip()
        if not value or not row.get("containing_function"):
            continue
        classified = classify_path(value)
        if classified is None:
            continue
        kind, relative = classified
        function = int(row["containing_function"], 16)
        line = int(row["line"]) if (row.get("line") or "").strip() else None
        if kind == "unit":
            units.append(UnitAnchor(function, relative, ASSERTION, line))
        else:
            headers.append(HeaderAnchor(function, relative, ASSERTION, line))
    return units, headers


def _small_immediate(instruction: Any) -> int | None:
    from ghidra.program.model.scalar import Scalar

    value: int | None = None
    for index in range(instruction.getNumOperands()):
        for item in instruction.getOpObjects(index):
            if isinstance(item, Scalar) and 0 < item.getValue() <= 0xFFFF:
                if value is not None:
                    return None
                value = int(item.getValue())
    return value


def _recover_line(listing: Any, reference_instruction: Any) -> int | None:
    """Recover an adjacent ``__LINE__`` argument through Ghidra instructions.

    The reviewed VC6 assertion form pushes the message, the line and the file
    immediately before the call, so the instruction directly before the string
    reference is the line push. No other form is guessed at; a missing line
    never discards the anchor.
    """

    previous = reference_instruction.getPrevious()
    if previous is None:
        return None
    return _small_immediate(previous)


def collect_program_anchors(
    program: Any,
    *,
    variant: str = "",
    roots: Sequence[str] = DEFAULT_ROOTS,
) -> tuple[list[UnitAnchor], list[HeaderAnchor]]:
    """Extract every source-path string reference in one Ghidra program.

    A reference to an absolute Wizardry source path from a function establishes
    that unit's anchor for the containing function, regardless of what consumes
    the string. Header spellings are collected separately.
    """

    listing = program.getListing()
    references = program.getReferenceManager()
    functions = program.getFunctionManager()
    units: list[UnitAnchor] = []
    headers: list[HeaderAnchor] = []
    seen: set[tuple[str, int, str]] = set()
    data_iterator = listing.getDefinedData(True)
    while data_iterator.hasNext():
        data = data_iterator.next()
        if not data.hasStringValue():
            continue
        value = str(data.getValue())
        if "\\" not in value:
            continue
        classified = classify_path(value, roots)
        if classified is None:
            continue
        kind, relative = classified
        target = data.getAddress()
        for reference in references.getReferencesTo(target):
            site = reference.getFromAddress()
            instruction = listing.getInstructionContaining(site)
            if instruction is None:
                continue
            function = functions.getFunctionContaining(site)
            if function is None:
                continue
            entry = int(function.getEntryPoint().getOffset())
            key = (kind, entry, relative)
            if key in seen:
                continue
            seen.add(key)
            line = _recover_line(listing, instruction)
            if kind == "unit":
                anchor = UnitAnchor(entry, relative, SOURCE_REFERENCE, line, variant, entry)
                units.append(anchor)
            else:
                headers.append(HeaderAnchor(entry, relative, SOURCE_REFERENCE, line))
    units.sort(key=lambda anchor: (anchor.function, anchor.source_path, anchor.evidence))
    headers.sort(key=lambda anchor: (anchor.function, anchor.header_path, anchor.evidence))
    return units, headers


def _conflicting_functions(anchors: Iterable[UnitAnchor]) -> dict[int, tuple[str, ...]]:
    by_function: dict[int, list[UnitAnchor]] = defaultdict(list)
    for anchor in anchors:
        by_function[anchor.function].append(anchor)
    return {
        function: tuple(sorted({anchor.source_path for anchor in unit_anchors}))
        for function, unit_anchors in by_function.items()
        if len({anchor.source_path for anchor in unit_anchors}) > 1
    }


def _group_by_unit(anchors: Iterable[UnitAnchor]) -> dict[str, list[UnitAnchor]]:
    grouped: dict[str, list[UnitAnchor]] = defaultdict(list)
    for anchor in anchors:
        grouped[anchor.source_path].append(anchor)
    return grouped


def _group_by_function(anchors: Iterable[UnitAnchor]) -> dict[int, list[UnitAnchor]]:
    grouped: dict[int, list[UnitAnchor]] = defaultdict(list)
    for anchor in anchors:
        grouped[anchor.function].append(anchor)
    return grouped


def _hulls(grouped: dict[str, list[UnitAnchor]]) -> list[TranslationUnitInterval]:
    intervals = []
    for unit, anchors in grouped.items():
        addresses = sorted({anchor.function for anchor in anchors})
        evidence = tuple(sorted({anchor.evidence for anchor in anchors}, key=_EVIDENCE_ORDER.index))
        intervals.append(
            TranslationUnitInterval(
                source_path=unit,
                lower=addresses[0],
                upper=addresses[-1],
                anchors=tuple(addresses),
                evidence=evidence,
            )
        )
    intervals.sort(key=lambda interval: (interval.lower, interval.source_path.casefold()))
    return intervals


def _inside_other_hull(entry: int, unit: str, intervals: Iterable[TranslationUnitInterval]) -> bool:
    return any(
        interval.source_path != unit and interval.lower <= entry <= interval.upper
        for interval in intervals
    )


class TranslationUnitLayout:
    """One resolver over direct anchors, hard hulls, gaps and header evidence."""

    def __init__(
        self,
        anchors: Iterable[UnitAnchor],
        *,
        header_anchors: Iterable[HeaderAnchor] = (),
        external_entries: Iterable[int] = (),
        cross_build_details: Iterable[dict[str, Any]] = (),
    ) -> None:
        collected = tuple(sorted(anchors, key=lambda a: (a.function, a.source_path)))
        self.header_anchors = tuple(
            sorted(header_anchors, key=lambda a: (a.function, a.header_path))
        )
        self.external_entries = frozenset(external_entries)
        self.cross_build_details = tuple(cross_build_details)
        native = [anchor for anchor in collected if anchor.evidence in _DIRECT_EVIDENCE]
        cross = [anchor for anchor in collected if anchor.evidence == CROSS_BUILD]
        native_conflicts = _conflicting_functions(native)
        native_functions = {anchor.function for anchor in native}
        native_hulls = _hulls(
            _group_by_unit(anchor for anchor in native if anchor.function not in native_conflicts)
        )
        accepted_cross = [
            anchor
            for anchor in cross
            if anchor.function not in native_functions
            and anchor.function not in native_conflicts
            and not _inside_other_hull(anchor.function, anchor.source_path, native_hulls)
        ]
        self.cross_build_rejected = len(cross) - len(accepted_cross)
        advisory_cross = [anchor for anchor in accepted_cross if anchor.match_kind == SIMILAR_MATCH]
        self.cross_build_advisory = len(advisory_cross)
        self.unit_anchors = tuple(
            sorted([*native, *accepted_cross], key=lambda a: (a.function, a.source_path))
        )
        self.conflicts = _conflicting_functions(self.unit_anchors)
        self.anchors_by_function = _group_by_function(self.unit_anchors)
        self.anchors_by_unit = _group_by_unit(
            anchor for anchor in self.unit_anchors if anchor.function not in self.conflicts
        )
        # Only direct evidence and unique cross-build matches shape hard hulls.
        # Advisory similarity matches still report a likely owner through
        # anchors_by_function, but they cannot resize an interval.
        self.hull_anchors_by_unit = _group_by_unit(
            anchor
            for anchor in (*native, *accepted_cross)
            if anchor.match_kind != SIMILAR_MATCH and anchor.function not in self.conflicts
        )
        self.intervals = _hulls(self.hull_anchors_by_unit)
        self.interval_lowers = [interval.lower for interval in self.intervals]
        self._validate_hulls()

    def interval_at(self, entry: int) -> TranslationUnitInterval | None:
        """The hard hull containing ``entry``, if any."""
        return self._interval_at(entry)[1]

    def _interval_at(self, entry: int) -> tuple[int, TranslationUnitInterval | None]:
        import bisect

        index = bisect.bisect_right(self.interval_lowers, entry) - 1
        if index >= 0:
            interval = self.intervals[index]
            if interval.lower <= entry <= interval.upper:
                return index, interval
        return index, None

    def _validate_hulls(self) -> None:
        for previous, current in pairwise(self.intervals):
            if previous.upper >= current.lower:
                raise TranslationUnitContradiction(
                    "translation-unit anchors violate the contiguous-.text invariant: "
                    f"{previous.source_path} is hard-bounded at "
                    f"{_address(previous.lower)}-{_address(previous.upper)} but "
                    f"{current.source_path} anchors at {_address(current.lower)}"
                )

    def _conflict(self, entry: int) -> dict[str, Any]:
        paths = list(self.conflicts[entry])
        return {
            "source_path": "",
            "attribution": "inlined-or-conflicting",
            "alternatives": paths,
            "evidence": [anchor.describe() for anchor in self.anchors_by_function.get(entry, ())],
        }

    def _direct(self, entry: int) -> dict[str, Any]:
        anchors = [
            anchor
            for anchor in self.anchors_by_function.get(entry, ())
            if entry not in self.conflicts
        ]
        units = sorted({anchor.source_path for anchor in anchors}, key=str.casefold)
        if len(units) != 1:
            return {
                "source_path": "",
                "attribution": "gap",
                "alternatives": [],
                "evidence": [],
            }
        unit = units[0]
        unit_anchors = [anchor for anchor in anchors if anchor.source_path == unit]
        evidence = sorted(unit_anchors, key=lambda anchor: _EVIDENCE_ORDER.index(anchor.evidence))
        direct = {anchor.evidence for anchor in unit_anchors}
        if direct & set(_DIRECT_EVIDENCE):
            attribution = "direct"
        elif any(anchor.match_kind != SIMILAR_MATCH for anchor in unit_anchors):
            attribution = CROSS_BUILD
        else:
            attribution = "cross-build-similar"
        result: dict[str, Any] = {
            "source_path": unit,
            "attribution": attribution,
            "alternatives": [],
            "evidence": [anchor.describe() for anchor in evidence],
        }
        lines = sorted({anchor.line for anchor in unit_anchors if anchor.line is not None})
        if len(lines) == 1:
            result["line"] = lines[0]
        return result

    def _bounded(self, entry: int, interval: TranslationUnitInterval) -> dict[str, Any]:
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
            "interval_lower": _address(interval.lower),
            "interval_upper": _address(interval.upper),
        }

    def _nearest_assertion_anchor(self, entry: int, *, before: bool) -> dict[str, Any] | None:
        candidates = [
            anchor
            for anchor in self.unit_anchors
            if anchor.evidence == ASSERTION
            and (anchor.function < entry if before else anchor.function > entry)
        ]
        if not candidates:
            return None
        anchor = (max if before else min)(candidates, key=lambda item: item.function)
        return {
            "function": _address(anchor.function),
            "source_path": anchor.source_path,
            "line": anchor.line,
        }

    def _gap(self, entry: int) -> dict[str, Any]:
        index, _interval = self._interval_at(entry)
        previous = self.intervals[index] if index >= 0 else None
        following = self.intervals[index + 1] if index + 1 < len(self.intervals) else None
        return {
            "source_path": "",
            "attribution": "gap",
            "alternatives": [],
            "evidence": [],
            "gap_size": (
                following.lower - previous.upper
                if previous is not None and following is not None
                else None
            ),
            "nearest_anchors": {
                "previous": self._nearest_assertion_anchor(entry, before=True),
                "next": self._nearest_assertion_anchor(entry, before=False),
            },
            "previous_hard_unit": (
                {"source_path": previous.source_path, "upper": _address(previous.upper)}
                if previous is not None
                else None
            ),
            "next_hard_unit": (
                {"source_path": following.source_path, "lower": _address(following.lower)}
                if following is not None
                else None
            ),
        }

    def owner(self, entry: int) -> dict[str, Any]:
        """Resolve one function entry to its original translation unit."""

        if entry in self.external_entries:
            return {
                "source_path": "",
                "attribution": "external/synthetic",
                "alternatives": [],
                "evidence": [],
            }
        if entry in self.conflicts:
            return self._conflict(entry)
        direct = self._direct(entry)
        if direct["source_path"]:
            return direct
        _index, interval = self._interval_at(entry)
        if interval is not None:
            return self._bounded(entry, interval)
        return self._gap(entry)

    def context(self, entry: int) -> dict[str, Any]:
        """Owner plus the nearby source-line anchors a recovery agent needs."""

        owner = self.owner(entry)
        unit = str(owner.get("source_path") or "")
        line_anchors: dict[str, Any] = {
            "this_function": [
                {"line": value}
                for value in sorted(
                    {
                        anchor.line
                        for anchor in self.anchors_by_function.get(entry, ())
                        if anchor.line is not None
                    }
                )
            ],
            "previous": None,
            "next": None,
        }
        if unit:
            unit_anchors = [a for a in self.anchors_by_unit.get(unit, []) if a.line is not None]
            previous = [a for a in unit_anchors if a.function <= entry]
            following = [a for a in unit_anchors if a.function > entry]
            if previous:
                anchor = max(previous, key=lambda item: item.function)
                line_anchors["previous"] = {
                    "function": _address(anchor.function),
                    "line": anchor.line,
                }
            if following:
                anchor = min(following, key=lambda item: item.function)
                line_anchors["next"] = {
                    "function": _address(anchor.function),
                    "line": anchor.line,
                }
        return {**owner, "source_line_anchors": line_anchors}

    def projection(self) -> list[TranslationUnitInterval]:
        return list(self.intervals)

    def header_owner(self, entry: int) -> dict[str, Any] | None:
        headers = [anchor for anchor in self.header_anchors if anchor.function == entry]
        if not headers:
            return None
        return {
            "header_paths": sorted({anchor.header_path for anchor in headers}, key=str.casefold),
            "attribution": "header-origin",
            "evidence": [anchor.describe() for anchor in headers],
        }


_EVIDENCE_ORDER = (ASSERTION, SOURCE_REFERENCE, CROSS_BUILD)


def derive_intervals(assertions: list[dict[str, str]]) -> list[TranslationUnitInterval]:
    """The assertion-only projection retained for existing report callers."""

    units, _ = assertion_anchors(assertions)
    return TranslationUnitLayout(units).projection()


class TranslationUnitResolver:
    """Resolve one function entry from the reviewed assertion table only."""

    def __init__(self, assertions: list[dict[str, str]]) -> None:
        units, headers = assertion_anchors(assertions)
        self.layout = TranslationUnitLayout(units, header_anchors=headers)

    def resolve(self, entry: int) -> dict[str, object]:
        return self.layout.owner(entry)


def read_assertions(repo_dir: Path) -> list[dict[str, str]]:
    path = repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def translation_unit_layout(
    settings: Any,
    selector: str = "wiz8",
    *,
    assertions: list[dict[str, str]] | None = None,
    roots: Sequence[str] = DEFAULT_ROOTS,
    external_entries: Iterable[int] = (),
    include_cross_build: bool = True,
    variants: Sequence[str] = (),
) -> TranslationUnitLayout:
    """Collect the reviewed, live and cross-build anchors for one program."""

    from .cross_build import DEFAULT_VARIANTS, collect_cross_build_anchors
    from .cross_build import program_function_signatures as signatures_for
    from .env import open_program

    if assertions is None:
        assertions = read_assertions(settings.repo_dir)
    units, headers = assertion_anchors(assertions)
    with open_program(settings, selector) as program:
        live_units, live_headers = collect_program_anchors(program, roots=roots)
        signatures = signatures_for(program) if include_cross_build else {}
    projected: list[UnitAnchor] = []
    cross_details: list[dict[str, Any]] = []
    if signatures:
        projected, cross_details = collect_cross_build_anchors(
            settings,
            signatures,
            variants=variants or DEFAULT_VARIANTS,
            roots=roots,
        )
    return TranslationUnitLayout(
        [*units, *live_units, *projected],
        header_anchors=[*headers, *live_headers],
        external_entries=external_entries,
        cross_build_details=cross_details,
    )


def translation_unit_layout_if_available(
    settings: Any,
    selector: str = "wiz8",
    **kwargs: Any,
) -> TranslationUnitLayout | None:
    """The live layout when this environment can open the Ghidra program.

    Callers that must also work without a Ghidra checkout (unit tests, CI)
    fall back to the reviewed assertion anchors. A layout contradiction is a
    real model error and is never swallowed.
    """

    try:
        return translation_unit_layout(settings, selector, **kwargs)
    except (OSError, RuntimeError):
        return None
