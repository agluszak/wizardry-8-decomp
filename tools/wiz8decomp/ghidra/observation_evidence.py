"""Read-only comparison between canonical observations and a Ghidra program.

The snapshot files contain machine observations, not reviewed semantic names.
This module reports what Ghidra already contains; it never mutates the program.
"""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..config import repository_root


def _rows(relative: str, program_name: str, repo_dir: Path) -> list[dict[str, str]]:
    path = repo_dir / relative
    with path.open(newline="", encoding="utf-8") as stream:
        return [row for row in csv.DictReader(stream) if row.get("program") == program_name]


def strict_scalar_observation(row: dict[str, str]) -> bool:
    """Whether a global row proves a standalone scalar width conservatively."""

    access = set(row.get("access_kinds", "").split())
    widths = row.get("widths", "").split()
    return (
        row.get("kind") == "data"
        and "write" in access
        and "address-taken" not in access
        and len(widths) == 1
        and widths[0] in {"1", "2", "4"}
    )


def load_observation_bundle(
    program_name: str, repo_dir: Path | None = None
) -> dict[str, list[dict[str, str]]]:
    """Load only the observations whose addresses belong to PROGRAM_NAME."""

    root = repo_dir or repository_root()
    return {
        "eh_functions": _rows("evidence/snapshots/eh-metadata/functions.csv", program_name, root),
        "eh_unwind": _rows("evidence/snapshots/eh-metadata/unwind.csv", program_name, root),
        "assertions": _rows("evidence/snapshots/call-sites/assertions.csv", program_name, root),
        "runtime_class_names": _rows(
            "evidence/snapshots/call-sites/runtime-class-names.csv", program_name, root
        ),
        "vtables": _rows("evidence/snapshots/polymorphism/vtables.csv", program_name, root),
        "vtable_slots": _rows("evidence/snapshots/polymorphism/slots.csv", program_name, root),
        "vptr_writes": _rows("evidence/snapshots/polymorphism/vptr-writes.csv", program_name, root),
        "globals": _rows("evidence/snapshots/globals/globals.csv", program_name, root),
    }


def _examples(values: list[int], limit: int = 12) -> list[str]:
    return [f"0x{value:08x}" for value in sorted(set(values))[:limit]]


def _defined_data_covering(listing: Any, address: Any) -> Any | None:
    data = listing.getDataContaining(address)
    return data if data is not None and data.isDefined() else None


def defined_overlap(listing: Any, address: Any, length: int) -> bool:
    """Whether defined data or an instruction overlaps an observed byte range."""

    return any(
        _defined_data_covering(listing, address.add(offset)) is not None
        or listing.getInstructionContaining(address.add(offset)) is not None
        for offset in range(length)
    )


def _has_owned_comment(listing: Any, address: Any, key: str) -> bool:
    from ghidra.program.model.listing import CodeUnit

    comment = listing.getComment(CodeUnit.PRE_COMMENT, address) or ""
    return f"[wiz8 observation:{key}:begin]" in comment


def audit_observation_evidence(program: Any) -> dict[str, Any]:
    """Compare canonical snapshot facts with one open, read-only Ghidra program."""

    # Program.getName() is the PE's original name (``Wiz8.exe``), while the
    # project domain file carries the corpus identity used by every snapshot.
    program_name = str(program.getDomainFile().getName())
    bundle = load_observation_bundle(program_name)
    if not any(bundle.values()):
        raise ValueError(f"no symbol-evidence snapshots exist for {program_name}")

    address_space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    listing = program.getListing()

    eh_rows = [row for row in bundle["eh_functions"] if row["frame_setup"]]
    eh_unresolved: list[int] = []
    eh_entries: set[int] = set()
    for row in eh_rows:
        raw = int(row["frame_setup"], 16)
        owner = functions.getFunctionContaining(address_space.getAddress(raw))
        if owner is None:
            eh_unresolved.append(raw)
        else:
            eh_entries.add(int(str(owner.getEntryPoint()), 16))

    tables_fully_defined = 0
    tables_partly_defined = 0
    table_missing_slots = 0
    table_conflict_slots = 0
    for row in bundle["vtables"]:
        if row["kind"] != "vftable":
            continue
        start = int(row["address"], 16)
        count = int(row["slot_count"])
        defined = 0
        missing = 0
        conflicts = 0
        for index in range(count):
            address = address_space.getAddress(start + index * 4)
            if _defined_data_covering(listing, address) is not None:
                defined += 1
            elif defined_overlap(listing, address, 4):
                conflicts += 1
            else:
                missing += 1
        table_missing_slots += missing
        table_conflict_slots += conflicts
        if defined == count:
            tables_fully_defined += 1
        elif defined:
            tables_partly_defined += 1

    missing_slot_functions: list[int] = []
    slot_targets_inside_functions: list[int] = []
    uncovered_slot_targets: list[int] = []
    for row in bundle["vtable_slots"]:
        if row["kind"] == "base-displacement" or not row["target"]:
            continue
        target = int(row["target"], 16)
        address = address_space.getAddress(target)
        if functions.getFunctionAt(address) is None:
            missing_slot_functions.append(target)
            if functions.getFunctionContaining(address) is None:
                uncovered_slot_targets.append(target)
            else:
                slot_targets_inside_functions.append(target)

    scalar_rows = [row for row in bundle["globals"] if strict_scalar_observation(row)]
    scalar_exact = 0
    scalar_aggregate = 0
    scalar_missing: list[int] = []
    scalar_conflicts: list[int] = []
    for row in scalar_rows:
        raw = int(row["address"], 16)
        width = int(row["widths"])
        address = address_space.getAddress(raw)
        data = _defined_data_covering(listing, address)
        if data is None:
            if defined_overlap(listing, address, width):
                scalar_conflicts.append(raw)
            else:
                scalar_missing.append(raw)
            continue
        offset = address.subtract(data.getAddress())
        if offset == 0 and data.getLength() == width:
            scalar_exact += 1
        elif 0 <= offset and offset + width <= data.getLength():
            scalar_aggregate += 1
        else:
            scalar_conflicts.append(raw)

    commented_calls = 0
    for row in bundle["assertions"]:
        address = address_space.getAddress(int(row["call_site"], 16))
        if _has_owned_comment(listing, address, "assertion"):
            commented_calls += 1

    commented_eh = sum(
        _has_owned_comment(
            listing,
            address_space.getAddress(int(row["frame_setup"], 16)),
            "eh",
        )
        for row in eh_rows
    )
    commented_runtime_names = sum(
        _has_owned_comment(
            listing,
            address_space.getAddress(int(row["call_site"], 16)),
            "runtime-class-name",
        )
        for row in bundle["runtime_class_names"]
    )

    return {
        "program": program_name,
        "mode": "read-only",
        "eh": {
            "records": len(bundle["eh_functions"]),
            "setups": len(eh_rows),
            "resolved_setups": len(eh_rows) - len(eh_unresolved),
            "owning_functions": len(eh_entries),
            "unresolved_setups": len(eh_unresolved),
            "unresolved_examples": _examples(eh_unresolved),
            "commented_setups": commented_eh,
            "without_comments": len(eh_rows) - commented_eh,
        },
        "polymorphism": {
            "vtables": sum(row["kind"] == "vftable" for row in bundle["vtables"]),
            "fully_defined_tables": tables_fully_defined,
            "partly_defined_tables": tables_partly_defined,
            "missing_table_slots": table_missing_slots,
            "conflicting_table_slots": table_conflict_slots,
            "slot_targets": len(bundle["vtable_slots"]),
            "missing_slot_functions": len(set(missing_slot_functions)),
            "missing_function_examples": _examples(missing_slot_functions),
            "targets_inside_existing_functions": len(set(slot_targets_inside_functions)),
            "inside_function_examples": _examples(slot_targets_inside_functions),
            "uncovered_slot_targets": len(set(uncovered_slot_targets)),
            "uncovered_target_examples": _examples(uncovered_slot_targets),
        },
        "globals": {
            "strict_scalar_candidates": len(scalar_rows),
            "exact_width_data": scalar_exact,
            "covered_by_aggregate": scalar_aggregate,
            "undefined": len(scalar_missing),
            "conflicts": len(scalar_conflicts),
            "undefined_examples": _examples(scalar_missing),
            "conflict_examples": _examples(scalar_conflicts),
        },
        "assertions": {
            "call_sites": len(bundle["assertions"]),
            "already_commented": commented_calls,
            "without_comments": len(bundle["assertions"]) - commented_calls,
        },
        "runtime_class_names": {
            "call_sites": len(bundle["runtime_class_names"]),
            "literal_names": sum(bool(row["name"]) for row in bundle["runtime_class_names"]),
            "already_commented": commented_runtime_names,
            "without_comments": len(bundle["runtime_class_names"]) - commented_runtime_names,
        },
    }
