"""Materialize neutral, machine-observed facts without semantic renaming."""

from __future__ import annotations

import re
from typing import Any

from ..config import Settings
from .observation_evidence import (
    defined_overlap,
    load_observation_bundle,
    strict_scalar_observation,
)
from .project import resolve_program_name


def merge_observation_comment(existing: str | None, key: str, body: str) -> str:
    """Insert or replace one owned observation block while preserving other comments."""

    begin = f"[wiz8 observation:{key}:begin]"
    end = f"[wiz8 observation:{key}:end]"
    block = f"{begin}\n{body.rstrip()}\n{end}"
    current = existing or ""
    pattern = re.compile(re.escape(begin) + r".*?" + re.escape(end), re.DOTALL)
    if pattern.search(current):
        return pattern.sub(lambda _match: block, current)
    return f"{current.rstrip()}\n\n{block}".strip()


def _destructor(row: dict[str, str]) -> str:
    return row["import_signature"] or row["import_name"] or row["target"] or "unresolved"


def apply_observation_evidence(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    """Apply only neutral facts whose address and width come directly from snapshots."""

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.data import (
        ByteDataType,
        DataUtilities,
        DWordDataType,
        PointerDataType,
        WordDataType,
    )
    from ghidra.program.model.listing import CodeUnit
    from ghidra.program.model.util import CodeUnitInsertionException

    program_name = resolve_program_name(settings, selector)
    bundle = load_observation_bundle(program_name, settings.repo_dir)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name, "mode": "neutral-observations"}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply canonical neutral observations")
            commit = False
            try:
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                functions = program.getFunctionManager()
                listing = program.getListing()
                dtm = program.getDataTypeManager()

                created_functions: list[str] = []
                contained_targets: list[str] = []
                slot_targets = {
                    int(row["target"], 16)
                    for row in bundle["vtable_slots"]
                    if row["kind"] != "base-displacement" and row["target"]
                }
                for target in sorted(slot_targets):
                    address = address_space.getAddress(target)
                    if functions.getFunctionAt(address) is not None:
                        continue
                    if functions.getFunctionContaining(address) is not None:
                        contained_targets.append(f"0x{target:08x}")
                        continue
                    command = CreateFunctionCmd(address)
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to create observed vtable target 0x{target:08x}: "
                            f"{command.getStatusMsg()}"
                        )
                    created_functions.append(f"0x{target:08x}")

                pointer = PointerDataType(dtm)
                defined_slots = 0
                skipped_slot_conflicts: list[str] = []
                for table in bundle["vtables"]:
                    if table["kind"] != "vftable":
                        continue
                    start = int(table["address"], 16)
                    for index in range(int(table["slot_count"])):
                        address = address_space.getAddress(start + index * 4)
                        data = listing.getDataContaining(address)
                        if data is not None and data.isDefined():
                            continue
                        if defined_overlap(listing, address, 4):
                            skipped_slot_conflicts.append(str(address))
                            continue
                        try:
                            DataUtilities.createData(
                                program,
                                address,
                                pointer,
                                -1,
                                False,
                                DataUtilities.ClearDataMode.CLEAR_ALL_UNDEFINED_CONFLICT_DATA,
                            )
                        except CodeUnitInsertionException as error:
                            raise RuntimeError(
                                f"cannot define observed vtable slot at {address}"
                            ) from error
                        defined_slots += 1

                scalar_types = {
                    "1": ByteDataType.dataType,
                    "2": WordDataType.dataType,
                    "4": DWordDataType.dataType,
                }
                typed_scalars = 0
                existing_scalars = 0
                skipped_scalar_conflicts: list[str] = []
                for row in bundle["globals"]:
                    if not strict_scalar_observation(row):
                        continue
                    address = address_space.getAddress(int(row["address"], 16))
                    data = listing.getDataContaining(address)
                    if data is not None and data.isDefined():
                        existing_scalars += 1
                        continue
                    width = int(row["widths"])
                    if defined_overlap(listing, address, width):
                        skipped_scalar_conflicts.append(str(address))
                        continue
                    try:
                        DataUtilities.createData(
                            program,
                            address,
                            scalar_types[row["widths"]],
                            -1,
                            False,
                            DataUtilities.ClearDataMode.CLEAR_ALL_UNDEFINED_CONFLICT_DATA,
                        )
                    except CodeUnitInsertionException as error:
                        raise RuntimeError(
                            f"cannot define observed {row['widths']}-byte scalar at {address}"
                        ) from error
                    typed_scalars += 1

                assertion_comments = 0
                for row in bundle["assertions"]:
                    address = address_space.getAddress(int(row["call_site"], 16))
                    if listing.getCodeUnitAt(address) is None:
                        continue
                    body = f"{row['source_path']}:{row['line']}\nassert({row['expression']})" + (
                        f"\nmessage: {row['message']}" if row["message"] else ""
                    )
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(existing, "assertion", body),
                    )
                    assertion_comments += 1

                runtime_class_comments = 0
                for row in bundle["runtime_class_names"]:
                    address = address_space.getAddress(int(row["call_site"], 16))
                    if listing.getCodeUnitAt(address) is None:
                        continue
                    body = (
                        f'srRuntimeClass::setName("{row["name"]}")'
                        if row["name"]
                        else "srRuntimeClass::setName(non-literal value)"
                    )
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(existing, "runtime-class-name", body),
                    )
                    runtime_class_comments += 1

                unwind_by_funcinfo: dict[str, list[dict[str, str]]] = {}
                for row in bundle["eh_unwind"]:
                    unwind_by_funcinfo.setdefault(row["funcinfo"], []).append(row)
                eh_comments = 0
                for row in bundle["eh_functions"]:
                    if not row["frame_setup"]:
                        continue
                    address = address_space.getAddress(int(row["frame_setup"], 16))
                    if listing.getCodeUnitAt(address) is None:
                        continue
                    cleanups = unwind_by_funcinfo.get(row["funcinfo"], [])
                    body_lines = [f"FuncInfo {row['funcinfo']}; exact EH setup anchor"]
                    body_lines.extend(
                        f"state {item['state']}: [ebp{int(item['frame_offset']):+d}] "
                        f"{item['kind']} -> {_destructor(item)}"
                        for item in cleanups
                        if item["frame_offset"]
                    )
                    existing = listing.getComment(CodeUnit.PRE_COMMENT, address)
                    listing.setComment(
                        address,
                        CodeUnit.PRE_COMMENT,
                        merge_observation_comment(existing, "eh", "\n".join(body_lines)),
                    )
                    eh_comments += 1

                commit = True
                result.update(
                    {
                        "created_slot_functions": created_functions,
                        "slot_targets_inside_existing_functions": contained_targets,
                        "defined_vtable_slots": defined_slots,
                        "skipped_vtable_slot_conflicts": skipped_slot_conflicts,
                        "typed_scalars": typed_scalars,
                        "existing_scalar_data": existing_scalars,
                        "skipped_scalar_conflicts": skipped_scalar_conflicts,
                        "assertion_comments": assertion_comments,
                        "runtime_class_comments": runtime_class_comments,
                        "eh_comments": eh_comments,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply canonical neutral observations", pyghidra.task_monitor())
    finally:
        project.close()
    return result
