"""Materialize typed stack variables from exception-metadata unwind evidence.

Each planned slot comes from ``eh_frame_types.plan_frame_slots``: a frame
offset a cleanup funclet destroys, joined to a class either named by a
demangled library destructor import (abi-backed) or by a reviewed first-party
destructor identity. Slots whose destructor has no known class, and slots VC6
reuses for differently-typed temporaries, are already excluded by the plan.

The funclet frame base is the stack pointer just before the owning function
pushes its EH registration node, so the Ghidra stack offset of a slot is the
stack depth at ``eh_setup_start`` plus the (negative) funclet offset. The
depth is read from Ghidra's own stack-depth analysis rather than re-derived
from prologue pattern matching.
"""

from __future__ import annotations

from typing import Any

from ..config import Settings
from .eh_frame_types import FrameSlotPlan, ghidra_stack_offset, plan_frame_slots
from .evidence_index import load_evidence_index
from .project import resolve_program_name
from .type_specs import resolve_type_spec


def _library_type(dtm: Any, class_name: str, imported_types: frozenset[str]) -> Any:
    """An existing or newly created opaque structure for a library class."""

    return resolve_type_spec(
        dtm,
        class_name,
        "wiz8",
        imported_types=imported_types,
        allow_opaque_imported=True,
    )


def _reviewed_type(dtm: Any, evidence_program: str, class_name: str) -> Any | None:
    try:
        return resolve_type_spec(dtm, class_name, evidence_program)
    except ValueError:
        return None


def _clear_replaceable(frame: Any, offset: int, length: int) -> bool:
    """Remove non-user variables overlapping the slot; False on a user conflict."""

    from ghidra.program.model.symbol import SourceType

    for variable in list(frame.getStackVariables()):
        start = variable.getStackOffset()
        if start + variable.getLength() <= offset or start >= offset + length:
            continue
        symbol = variable.getSymbol()
        source = symbol.getSource() if symbol is not None else variable.getSource()
        if source == SourceType.USER_DEFINED:
            return False
        frame.clearVariable(start)
    return True


def apply_eh_frame_types(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
    *,
    evidence_program: str = "wiz8",
    materialize: bool = True,
) -> dict[str, Any]:
    """Create typed stack variables for every plannable unwind slot."""

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.app.cmd.function import CallDepthChangeInfo
    from ghidra.program.model.data import PointerDataType
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    report = plan_frame_slots(program_name, settings.repo_dir, evidence_program=evidence_program)
    index = load_evidence_index(settings.repo_dir, evidence_program, program_name)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {
        "program": program_name,
        "mode": "eh-frame-types",
        "planned": len(report.plans),
        "plan_skips": {
            "unresolved_destructor": report.skipped_unresolved_destructor,
            "conflicting_reuse": report.skipped_conflicting_reuse,
            "unparsed_import": report.skipped_unparsed_import,
        },
        "unresolved_destructors": dict(report.unresolved_destructor_counts[:10]),
    }
    applied: list[dict[str, str]] = []
    skipped: list[dict[str, str]] = []

    def skip(plan: FrameSlotPlan, reason: str) -> None:
        skipped.append(
            {
                "funcinfo": f"0x{plan.funcinfo:08x}",
                "frame_offset": str(plan.frame_offset),
                "class": plan.class_name,
                "reason": reason,
            }
        )

    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply EH unwind frame slot types")
            commit = False
            try:
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                functions = program.getFunctionManager()
                dtm = program.getDataTypeManager()
                depth_cache: dict[int, Any] = {}

                for plan in report.plans:
                    setup = address_space.getAddress(plan.frame_setup)
                    owner = functions.getFunctionContaining(setup)
                    if owner is None:
                        skip(plan, "no-owning-function")
                        continue
                    start = address_space.getAddress(plan.eh_setup_start)
                    if not owner.getBody().contains(start):
                        skip(plan, "setup-outside-owner")
                        continue
                    entry = int(str(owner.getEntryPoint()), 16)
                    info = depth_cache.get(entry)
                    if info is None:
                        info = CallDepthChangeInfo(owner)
                        depth_cache[entry] = info
                    depth = int(info.getDepth(start))
                    if depth > 0 or depth < -0x10000:
                        skip(plan, f"unknown-stack-depth:{depth}")
                        continue
                    offset = ghidra_stack_offset(depth, plan.frame_offset)

                    if plan.type_source == "reviewed-class":
                        data_type = _reviewed_type(dtm, evidence_program, plan.class_name)
                        if data_type is None:
                            skip(plan, "no-reviewed-structure")
                            continue
                    else:
                        data_type = _library_type(dtm, plan.class_name, index.imported_types)
                    if plan.is_pointer:
                        data_type = PointerDataType(data_type, dtm)

                    frame = owner.getStackFrame()
                    existing = frame.getVariableContaining(offset)
                    if (
                        existing is not None
                        and existing.getStackOffset() == offset
                        and str(existing.getName()) == plan.variable_name
                    ):
                        applied.append(
                            {
                                "function": str(owner.getEntryPoint()),
                                "stack_offset": hex(offset),
                                "name": plan.variable_name,
                                "type": str(existing.getDataType().getDisplayName()),
                                "already_present": "true",
                            }
                        )
                        continue
                    length = max(int(data_type.getLength()), 1)
                    if not _clear_replaceable(frame, offset, length):
                        skip(plan, "user-variable-conflict")
                        continue
                    try:
                        variable = frame.createVariable(
                            plan.variable_name, offset, data_type, SourceType.USER_DEFINED
                        )
                    except Exception as error:  # noqa: BLE001 - Ghidra Java exceptions
                        skip(plan, f"create-failed:{error.__class__.__name__}")
                        continue
                    held = "pointer to" if plan.is_pointer else "object of"
                    destructor = f" destructor 0x{plan.destructor:08x}" if plan.destructor else ""
                    variable.setComment(
                        f"EH unwind evidence: {held} {plan.class_name};"
                        f" FuncInfo 0x{plan.funcinfo:08x} states {list(plan.states)};"
                        f"{destructor} [{plan.type_source}]"
                    )
                    applied.append(
                        {
                            "function": str(owner.getEntryPoint()),
                            "stack_offset": hex(offset),
                            "name": plan.variable_name,
                            "type": str(data_type.getDisplayName()),
                        }
                    )

                commit = True
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply EH unwind frame slot types", pyghidra.task_monitor())
    finally:
        project.close()

    result["applied"] = applied
    result["applied_count"] = len(applied)
    result["skipped"] = skipped
    result["skipped_count"] = len(skipped)
    return result
