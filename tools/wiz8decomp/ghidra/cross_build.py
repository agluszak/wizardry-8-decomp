"""Project translation-unit anchors across Wizardry builds.

Retail only retains the source paths the compiler embedded in retail code.
Other official builds (demo, 1.2.6, 1.2.8) embed paths for translation units
whose retail filenames survive nowhere. This module matches functions between
builds with a relocation-insensitive instruction fingerprint and keeps only
signature groups that are unique on both sides; when a variant was compiled
with assertions and retail without them, a unique near-identical instruction
sequence is accepted as a weaker ``similar-body`` match with its score. Those
fuzzy matches stay advisory: only unique body matches may extend hard layout
hulls (see :mod:`unit_intervals`).

The matcher is deliberately narrow: every accepted pair must be unique, and an
ambiguous or non-unique function stays unknown. Nothing is persisted; the
projection is derived from the live project on demand.
"""

from __future__ import annotations

import difflib
import hashlib
from collections import defaultdict
from collections.abc import Iterable, Sequence
from dataclasses import dataclass
from typing import Any

from .unit_intervals import (
    CROSS_BUILD,
    DEFAULT_ROOTS,
    SIMILAR_MATCH,
    UnitAnchor,
    collect_program_anchors,
)

DEFAULT_VARIANTS = ("demo", "gog-1261", "gog-128")
_VARIANT_MODULES = {"wiz8.exe", "wiz8new.exe", "wiz8_v128.exe"}
_UNIQUE_MATCH = "unique-body"
_SIMILAR_MATCH = SIMILAR_MATCH
_MIN_RATIO = 0.85
_MIN_MARGIN = 0.06


@dataclass(frozen=True)
class FunctionSignature:
    size: int
    fingerprint: str
    sequence: tuple[str, ...] = ()


def program_function_signatures(program: Any) -> dict[int, FunctionSignature]:
    """One relocation-insensitive fingerprint per ordinary function body.

    The fingerprint is the mnemonic plus operand types of every instruction,
    which ignores absolute addresses (relocations) but keeps instruction shape.
    The body size comes along so a unique group must agree on both; the raw
    mnemonic sequence supports the conservative near-identical fallback.
    """

    listing = program.getListing()
    manager = program.getFunctionManager()
    signatures: dict[int, FunctionSignature] = {}
    iterator = manager.getFunctions(True)
    while iterator.hasNext():
        function = iterator.next()
        body = function.getBody()
        parts = []
        mnemonics = []
        for instruction in listing.getInstructions(body, True):
            operands = ",".join(
                str(instruction.getOperandType(index))
                for index in range(instruction.getNumOperands())
            )
            parts.append(f"{instruction.getMnemonicString()}:{operands}")
            mnemonics.append(instruction.getMnemonicString())
        signatures[int(function.getEntryPoint().getOffset())] = FunctionSignature(
            size=int(body.getNumAddresses()),
            fingerprint=hashlib.sha256("\n".join(parts).encode()).hexdigest(),
            sequence=tuple(mnemonics),
        )
    return signatures


def unique_signature_matches(
    target: dict[int, FunctionSignature], source: dict[int, FunctionSignature]
) -> dict[int, int]:
    """Map source entries to target entries whose signature group is unique both ways."""

    target_groups: dict[FunctionSignature, list[int]] = defaultdict(list)
    for entry, signature in target.items():
        target_groups[signature].append(entry)
    source_groups: dict[FunctionSignature, list[int]] = defaultdict(list)
    for entry, signature in source.items():
        source_groups[signature].append(entry)
    matches: dict[int, int] = {}
    for signature, entries in source_groups.items():
        if len(entries) != 1:
            continue
        candidates = target_groups.get(signature)
        if candidates is None or len(candidates) != 1:
            continue
        matches[entries[0]] = candidates[0]
    return matches


def fuzzy_signature_matches(
    target: dict[int, FunctionSignature],
    source: dict[int, FunctionSignature],
    *,
    exact: dict[int, int],
    source_entries: Iterable[int],
) -> dict[int, tuple[int, float]]:
    """A conservative fallback for debug/release bodies with compiled-out asserts.

    Only source functions with no exact match are considered, only targets with
    the same first mnemonic and a comparable size, and a pair is accepted only
    when the best sequence ratio clears ``_MIN_RATIO`` by ``_MIN_MARGIN`` over
    the runner-up and no other source claims the same target.
    """

    reserved = set(exact.values())
    by_first: dict[str, list[tuple[int, FunctionSignature]]] = defaultdict(list)
    for entry, signature in target.items():
        if entry in reserved or not signature.sequence:
            continue
        by_first[signature.sequence[0]].append((entry, signature))

    proposals: dict[int, tuple[float, int]] = {}
    for source_entry in source_entries:
        if source_entry in exact:
            continue
        source_signature = source.get(source_entry)
        if source_signature is None or not source_signature.sequence:
            continue
        candidates: list[tuple[float, int]] = []
        for target_entry, target_signature in by_first.get(source_signature.sequence[0], ()):
            if not (
                source_signature.size * 0.5 <= target_signature.size <= source_signature.size * 1.8
            ):
                continue
            matcher = difflib.SequenceMatcher(
                None, source_signature.sequence, target_signature.sequence, autojunk=False
            )
            if matcher.real_quick_ratio() < _MIN_RATIO or matcher.quick_ratio() < _MIN_RATIO:
                continue
            ratio = matcher.ratio()
            if ratio >= _MIN_RATIO:
                candidates.append((ratio, target_entry))
        if not candidates:
            continue
        candidates.sort(key=lambda item: (-item[0], item[1]))
        best = candidates[0]
        if len(candidates) > 1 and best[0] - candidates[1][0] < _MIN_MARGIN:
            continue
        proposals[source_entry] = best

    claimed: dict[int, list[int]] = defaultdict(list)
    for source_entry, (_ratio, target_entry) in proposals.items():
        claimed[target_entry].append(source_entry)
    return {
        source_entry: (target_entry, ratio)
        for source_entry, (ratio, target_entry) in proposals.items()
        if len(claimed[target_entry]) == 1
    }


def variant_program_names(settings: Any) -> dict[str, str]:
    from .project import configured_modules

    names: dict[str, str] = {}
    for module in configured_modules(settings, all_modules=True):
        if str(module["module_name"]).casefold() in _VARIANT_MODULES:
            names.setdefault(str(module["variant"]), str(module["program_name"]))
    return names


def collect_cross_build_anchors(
    settings: Any,
    target_signatures: dict[int, FunctionSignature],
    *,
    variants: Sequence[str] = DEFAULT_VARIANTS,
    roots: Sequence[str] = DEFAULT_ROOTS,
) -> tuple[list[UnitAnchor], list[dict[str, Any]]]:
    """Reproject variant anchors onto retail functions with unique body matches."""

    from .env import open_live_program

    names = variant_program_names(settings)
    projected: list[UnitAnchor] = []
    details: list[dict[str, Any]] = []
    for variant in variants:
        program_name = names.get(variant)
        if program_name is None:
            continue
        try:
            context = open_live_program(settings, program_name)
            with context as program:
                anchors, _ = collect_program_anchors(program, variant=variant, roots=roots)
                signatures = program_function_signatures(program)
        except FileNotFoundError:
            details.append({"variant": variant, "program": program_name, "status": "missing"})
            continue
        anchored = sorted({anchor.function for anchor in anchors})
        exact = unique_signature_matches(target_signatures, signatures)
        fuzzy = fuzzy_signature_matches(
            target_signatures,
            signatures,
            exact=exact,
            source_entries=anchored,
        )
        by_unit: dict[str, int] = defaultdict(int)
        match_kinds = defaultdict(int)
        for anchor in anchors:
            target_entry = exact.get(anchor.function)
            kind = _UNIQUE_MATCH
            score: float | None = None
            if target_entry is None:
                fuzzy_match = fuzzy.get(anchor.function)
                if fuzzy_match is None:
                    continue
                target_entry, score = fuzzy_match
                kind = _SIMILAR_MATCH
            projected.append(
                UnitAnchor(
                    function=target_entry,
                    source_path=anchor.source_path,
                    evidence=CROSS_BUILD,
                    line=anchor.line,
                    origin_variant=variant,
                    origin_function=anchor.function,
                    match_kind=kind,
                    score=score,
                )
            )
            by_unit[anchor.source_path] += 1
            match_kinds[kind] += 1
        details.append(
            {
                "variant": variant,
                "program": program_name,
                "status": "projected",
                "variant_anchors": len(anchors),
                "unique_matches": len(exact),
                "projected_anchors": sum(by_unit.values()),
                "match_kinds": dict(sorted(match_kinds.items())),
                "projected_units": dict(sorted(by_unit.items())),
            }
        )
    return projected, details


def cross_build_summary(anchors: Iterable[UnitAnchor]) -> dict[str, Any]:
    values = list(anchors)
    units: dict[str, set[str]] = defaultdict(set)
    for anchor in values:
        units[anchor.source_path].add(anchor.origin_variant)
    return {
        "anchors": len(values),
        "units": {unit: sorted(variants) for unit, variants in sorted(units.items())},
    }
