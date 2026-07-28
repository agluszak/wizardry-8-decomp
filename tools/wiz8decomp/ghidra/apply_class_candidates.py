"""Materialize candidate classes and bounded unit attribution.

This is observation-layer replay: everything it writes is machine-derived,
candidate-marked, and separable from the reviewed model at a glance.

- Skeleton structures for every unreviewed constructor-written vftable land
  in the ``/wiz8/candidates`` category as ``Candidate_<vtable>`` - vptrs
  typed through the shared ``virtual_function`` definition, sized by the
  census allocation hints. They are data types only, never applied to memory
  or bound to parameters; a reviewer picks one in the decompiler while
  triaging a writer.
- Every candidate writer function gets a ``candidate-class`` pre-comment
  stating its role (constructor-or-destructor vs scalar deleting destructor)
  for each vtable it installs.
- Every function whose entry falls inside an assertion-anchored translation
  unit interval, and every censused global inside a fitted data interval,
  gets a ``translation-unit`` pre-comment. Both are bounded interval
  attribution, not reviewed identity, and the comment says so.

All inputs are tracked (snapshots plus reviewed evidence), so the phase is
deterministic under the materialization key.
"""

from __future__ import annotations

import csv
from bisect import bisect_right
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..config import Settings
from ..evidence.classes import VIRTUAL_SLOT_TYPE_NAME
from .apply_observation_evidence import merge_observation_comment
from .apply_unzip_model import _structure
from .candidate_model import candidate_name, classify_candidates, derive_skeletons
from .observation_evidence import load_observation_bundle
from .project import resolve_program_name
from .unit_intervals import TranslationUnitInterval, call_site_anchors, derive_intervals

CANDIDATE_CATEGORY = "/wiz8/candidates"


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def load_candidate_inputs(
    repo_dir: Path, program_name: str, resolve_function: Any = None
) -> dict[str, Any]:
    """Everything the candidate phase derives, from tracked inputs only.

    ``resolve_function`` maps a write site to its containing function; the
    replay and the validator pass Ghidra's function manager so writer roles
    land on the real bodies rather than the census's padding heuristic.
    """

    bundle = load_observation_bundle(program_name, repo_dir)
    reviewed_vtables = {
        int(row["address"], 16)
        for row in _rows(repo_dir / "evidence" / "reviewed" / "wiz8" / "vtables.csv")
    }
    candidates = classify_candidates(
        bundle["vtables"],
        bundle["vtable_slots"],
        bundle["vptr_writes"],
        reviewed_vtables,
        resolve_function,
    )
    skeletons = derive_skeletons(candidates)

    # The reviewed assertion observations carry canonical-program addresses,
    # so only the canonical program may use them as interval anchors; every
    # program still gets the call-site snapshot anchors filtered to itself.
    assertions: list[dict[str, str]] = []
    if "--gog-base--" in program_name:
        assertions = _rows(repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv")
    anchors = call_site_anchors(bundle["assertions"], program_name)
    intervals = derive_intervals(assertions, anchors)

    data_intervals = [
        row
        for row in _rows(
            repo_dir / "evidence" / "snapshots" / "data-segmentation" / "unit-data-intervals.csv"
        )
        if row["program"] == program_name
    ]
    return {
        "candidates": candidates,
        "skeletons": skeletons,
        "intervals": intervals,
        "data_intervals": data_intervals,
    }


def function_resolver(program: Any) -> Any:
    """Authoritative site-to-function mapping backed by one open program."""

    address_space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()

    def resolve(site: int) -> int | None:
        function = functions.getFunctionContaining(address_space.getAddress(site))
        return None if function is None else int(str(function.getEntryPoint()), 16)

    return resolve


def writer_comment_bodies(candidates: list[dict[str, Any]]) -> dict[int, str]:
    """One aggregated candidate-class comment body per writer function."""

    roles: dict[int, list[str]] = defaultdict(list)
    for candidate in candidates:
        if candidate["reviewed"]:
            continue
        name = candidate_name(candidate["vtable"])
        hints = ", ".join(f"0x{size:x}" for size in candidate["allocation_sizes"])
        suffix = f"; allocation hints {hints}" if hints else ""
        deleting = candidate["scalar_deleting_destructor"]
        if deleting is not None:
            roles[deleting].append(f"candidate scalar deleting destructor of {name}{suffix}")
        elif candidate["slot0_target"] is not None:
            # Writes no vtable itself, so it is only positional evidence:
            # MSVC puts the scalar deleting destructor in slot 0 when the
            # class has a virtual destructor, and such a destructor usually
            # delegates the vtable restore to the complete destructor.
            roles[candidate["slot0_target"]].append(
                f"candidate vtable slot 0 of {name} "
                "(scalar deleting destructor position; writes no vtable itself)"
            )
        for writer in candidate["constructor_or_destructor"]:
            roles[writer].append(f"candidate constructor-or-destructor of {name}{suffix}")
    return {writer: "\n".join(sorted(lines)) for writer, lines in roles.items()}


def interval_lookup(intervals: list[TranslationUnitInterval]):
    lows = [interval.lower for interval in intervals]

    def lookup(entry: int) -> str | None:
        index = bisect_right(lows, entry) - 1
        if index >= 0 and entry <= intervals[index].upper:
            return intervals[index].source_path
        return None

    return lookup


def data_interval_lookup(rows: list[dict[str, str]]):
    by_class: dict[str, tuple[list[int], list[tuple[int, str]]]] = {}
    for name in {row["storage_class"] for row in rows}:
        spans = sorted(
            (int(row["lower"], 16), int(row["upper"], 16), row["unit"])
            for row in rows
            if row["storage_class"] == name
        )
        by_class[name] = (
            [lower for lower, _, _ in spans],
            [(upper, unit) for _, upper, unit in spans],
        )

    def lookup(storage: str, address: int) -> str | None:
        entry = by_class.get(storage)
        if entry is None:
            return None
        lows, uppers = entry
        index = bisect_right(lows, address) - 1
        if index >= 0 and address <= uppers[index][0]:
            return uppers[index][1]
        return None

    return lookup


def apply_class_candidates(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.program.model.data import CategoryPath, PointerDataType
    from ghidra.program.model.listing import CodeUnit

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name, "mode": "candidate-observations"}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply candidate class observations")
            commit = False
            try:
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                listing = program.getListing()
                functions = program.getFunctionManager()
                dtm = program.getDataTypeManager()

                inputs = load_candidate_inputs(
                    settings.repo_dir,
                    program_name,
                    function_resolver(program),
                )

                virtual_function = dtm.getDataType(
                    CategoryPath("/wiz8/classes"), VIRTUAL_SLOT_TYPE_NAME
                )
                vptr_type = (
                    PointerDataType(PointerDataType(virtual_function, dtm), dtm)
                    if virtual_function is not None
                    else PointerDataType(PointerDataType(dtm), dtm)
                )

                category = CategoryPath(CANDIDATE_CATEGORY)
                structures = 0
                for skeleton in inputs["skeletons"]:
                    fields = [
                        (
                            offset,
                            vptr_type,
                            "vptr" if offset == 0 else f"vptr_{offset:x}",
                            f"candidate vtable 0x{vtable:08x}",
                        )
                        for offset, vtable in skeleton["vptr_offsets"]
                    ]
                    _structure(dtm, category, skeleton["name"], skeleton["size"], fields)
                    structures += 1

                writer_comments = 0
                for writer, body in sorted(writer_comment_bodies(inputs["candidates"]).items()):
                    address = address_space.getAddress(writer)
                    if listing.getCodeUnitAt(address) is None:
                        continue
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(existing, "candidate-class", body),
                    )
                    writer_comments += 1

                lookup = interval_lookup(inputs["intervals"])
                function_comments = 0
                iterator = functions.getFunctions(True)
                while iterator.hasNext():
                    function = iterator.next()
                    entry = int(str(function.getEntryPoint()), 16)
                    unit = lookup(entry)
                    if unit is None:
                        continue
                    address = function.getEntryPoint()
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(
                            existing,
                            "translation-unit",
                            f"{unit} (bounded interval attribution, not reviewed identity)",
                        ),
                    )
                    function_comments += 1

                data_lookup = data_interval_lookup(inputs["data_intervals"])
                bundle = load_observation_bundle(program_name, settings.repo_dir)
                global_comments = 0
                for row in bundle["globals"]:
                    if row["kind"] == "import-slot":
                        continue
                    raw = int(row["address"], 16)
                    unit = data_lookup(f"{row['section']}/{row['storage']}", raw)
                    if unit is None:
                        continue
                    address = address_space.getAddress(raw)
                    if listing.getCodeUnitAt(address) is None:
                        continue
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(
                            existing,
                            "translation-unit",
                            f"{unit} ({row['section']}/{row['storage']} fit; "
                            "bounded interval attribution, not reviewed identity)",
                        ),
                    )
                    global_comments += 1

                commit = True
                result.update(
                    {
                        "candidate_structures": structures,
                        "writer_comments": writer_comments,
                        "function_unit_comments": function_comments,
                        "global_unit_comments": global_comments,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply candidate class observations", pyghidra.task_monitor())
    finally:
        project.close()
    return result
