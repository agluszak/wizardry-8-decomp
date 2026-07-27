"""Plan typed stack variables from exception-metadata unwind observations.

``unwind.csv`` states that a frame slot holds (or points at) an object a known
destructor runs on. When that destructor identifies a class - a demangled
library import, or a reviewed first-party destructor - the slot's type is
evidence, not inference, and can be materialized as a typed stack variable.

This module is pure planning: it joins the snapshots against the reviewed
class identities and reports exactly what an apply pass may do and what it
must skip. It never imports Ghidra.

Frame-offset semantics: a cleanup funclet addresses its slot relative to the
frame pointer the EH runtime hands it, which is the stack pointer value just
before the owning function pushed its EH registration node (the ``PUSH -0x1``
at ``eh_setup_start``). In Ghidra's entry-relative stack frame that base sits
at the stack depth of ``eh_setup_start``, so a funclet offset ``N`` (negative)
lands at ``depth + N``. This holds for both prologue shapes VC6 emits: a
classic ``push ebp`` frame (depth -4 at the state push) and the frameless
ESP-relative shape (depth 0), both verified against the canonical image.
"""

from __future__ import annotations

import csv
import re
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

from .observation_evidence import load_observation_bundle

_POINTER_KINDS = frozenset({"pointer", "pointer-import"})
_OBJECT_KINDS = frozenset({"object", "object-import"})

# "public: virtual __thiscall srBinStream::~srBinStream(void)" -> srBinStream;
# nested classes keep their full path ("srHuffman::BitIStream"). Only chained
# identifier segments (with optional template arguments) directly before the
# "::~" belong to the class; access and convention prefixes do not.
_IMPORT_DESTRUCTOR = re.compile(
    r"([A-Za-z_]\w*(?:<[^>]*>)?(?:::[A-Za-z_]\w*(?:<[^>]*>)?)*)::~"
)


@dataclass(frozen=True)
class FrameSlotPlan:
    """One typed stack variable an apply pass may create."""

    funcinfo: int
    eh_setup_start: int
    frame_setup: int
    frame_offset: int
    states: tuple[int, ...]
    kind: str
    class_name: str
    type_source: str
    destructor: int | None
    variable_name: str

    @property
    def is_pointer(self) -> bool:
        return self.kind in _POINTER_KINDS


@dataclass(frozen=True)
class FrameSlotPlanReport:
    """The plan plus everything that was deliberately not planned."""

    plans: tuple[FrameSlotPlan, ...]
    skipped_unresolved_destructor: int
    unresolved_destructor_counts: tuple[tuple[str, int], ...]
    skipped_conflicting_reuse: int
    skipped_unparsed_import: int


def ghidra_stack_offset(setup_depth: int, frame_offset: int) -> int:
    """Entry-relative Ghidra stack offset for a funclet frame offset."""

    return setup_depth + frame_offset


def import_destructor_class(import_signature: str) -> str | None:
    """Class named by a demangled library destructor signature."""

    match = _IMPORT_DESTRUCTOR.search(import_signature)
    if match is None:
        return None
    return match.group(1).strip()


def variable_name(class_name: str, frame_offset: int) -> str:
    """Deterministic, listing-safe name for a planned slot."""

    tail = class_name.rsplit("::", 1)[-1]
    tail = re.sub(r"[^0-9A-Za-z_]", "_", tail).strip("_")
    return f"eh_{tail}_{abs(frame_offset):x}"


def reviewed_destructor_classes(repo_dir: Path, evidence_program: str) -> dict[int, str]:
    """Destructor address -> class name, from the reviewed identity layer only."""

    directory = repo_dir / "evidence" / "reviewed" / evidence_program
    mapping: dict[int, str] = {}
    with (directory / "classes.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row.get("program", "").strip() != evidence_program:
                continue
            for column in ("destructor", "scalar_deleting_destructor"):
                value = row.get(column, "").strip()
                if value:
                    mapping[int(value, 16)] = row["class_name"].strip()
    with (directory / "functions.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row.get("program", "").strip() != evidence_program:
                continue
            name = row.get("provisional_name", "").strip()
            if "::~" in name:
                mapping.setdefault(int(row["address"], 16), name.split("::~", 1)[0])
    return mapping


def _identify(
    row: dict[str, str], reviewed: dict[int, str]
) -> tuple[str, str, int | None] | str:
    """Resolve one unwind row to (kind, class, destructor) or a skip reason."""

    kind = row["kind"]
    if kind.endswith("-import"):
        signature = row.get("import_signature", "").strip()
        class_name = import_destructor_class(signature) if signature else None
        if class_name is None:
            return "unparsed-import"
        return (kind, class_name, None)
    target = row.get("target", "").strip()
    if not target:
        return "unresolved"
    destructor = int(target, 16)
    class_name = reviewed.get(destructor)
    if class_name is None:
        return f"unresolved:0x{destructor:08x}"
    return (kind, class_name, destructor)


def plan_frame_slots(
    program_name: str,
    repo_dir: Path,
    *,
    evidence_program: str = "wiz8",
) -> FrameSlotPlanReport:
    """Join unwind observations with reviewed identities into a typed-slot plan."""

    bundle = load_observation_bundle(program_name, repo_dir)
    reviewed = reviewed_destructor_classes(repo_dir, evidence_program)
    setups = {
        row["funcinfo"]: row
        for row in bundle["eh_functions"]
        if row.get("frame_setup") and row.get("eh_setup_start")
    }

    slots: dict[tuple[str, int], list[dict[str, str]]] = {}
    for row in bundle["eh_unwind"]:
        if row["funcinfo"] not in setups or not row.get("frame_offset"):
            continue
        if row["kind"] not in _POINTER_KINDS | _OBJECT_KINDS:
            continue
        slots.setdefault((row["funcinfo"], int(row["frame_offset"])), []).append(row)

    plans: list[FrameSlotPlan] = []
    skipped_unresolved = 0
    skipped_unparsed = 0
    skipped_reuse = 0
    unresolved = Counter()
    for (funcinfo, offset), rows in sorted(slots.items()):
        identities = [_identify(row, reviewed) for row in rows]
        resolutions = {item for item in identities if isinstance(item, tuple)}
        reasons = [item for item in identities if isinstance(item, str)]
        for reason in reasons:
            if reason == "unparsed-import":
                skipped_unparsed += 1
            else:
                skipped_unresolved += 1
                if reason.startswith("unresolved:"):
                    unresolved[reason.split(":", 1)[1]] += 1
        if not resolutions:
            continue
        if len(resolutions) > 1 or reasons:
            # VC6 reuses one slot for successive temporaries. A slot whose
            # states disagree on kind or class - or where any co-located
            # state's class is unknown - has no single honest type.
            skipped_reuse += len(resolutions)
            continue
        setup = setups[funcinfo]
        kind, class_name, destructor = next(iter(resolutions))
        plans.append(
            FrameSlotPlan(
                funcinfo=int(funcinfo, 16),
                eh_setup_start=int(setup["eh_setup_start"], 16),
                frame_setup=int(setup["frame_setup"], 16),
                frame_offset=offset,
                states=tuple(sorted(int(row["state"]) for row in rows)),
                kind=kind,
                class_name=class_name,
                type_source="library-import" if kind.endswith("-import") else "reviewed-class",
                destructor=destructor,
                variable_name=variable_name(class_name, offset),
            )
        )

    return FrameSlotPlanReport(
        plans=tuple(plans),
        skipped_unresolved_destructor=skipped_unresolved,
        unresolved_destructor_counts=tuple(unresolved.most_common()),
        skipped_conflicting_reuse=skipped_reuse,
        skipped_unparsed_import=skipped_unparsed,
    )
