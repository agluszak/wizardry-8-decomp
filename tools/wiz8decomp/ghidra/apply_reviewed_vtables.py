"""Apply receiver-specific reviewed vtable datatypes after signatures replay."""

from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data
from .project import resolve_program_name
from .reviewed_class_model import load_reviewed_class_model


def apply_reviewed_vtables(
    settings: Settings,
    selector: str,
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    """Materialize exact table identities and slot prototypes without retyping bodies."""

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
        StructureDataType,
    )

    from .overlay import _slot_prototype

    program_name = resolve_program_name(settings, selector)
    model = load_reviewed_class_model(settings.repo_dir, "wiz8")
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    report: dict[str, Any] = {
        "program": program_name,
        "tables": 0,
        "slots": 0,
        "prototype_sources": {},
        "skipped_owner_types": [],
        "unbound_owner_fields": [],
    }
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply reviewed typed vtables")
            try:
                dtm = program.getDataTypeManager()
                space = program.getAddressFactory().getDefaultAddressSpace()
                functions = program.getFunctionManager()
                class_category = CategoryPath("/wiz8/classes")
                subobject_category = CategoryPath("/wiz8/classes/subobjects")
                table_category = CategoryPath("/wiz8/vtables")
                slots_by_table: dict[str, dict[int, Any]] = {}
                for slot in model.slots:
                    slots_by_table.setdefault(slot.vtable_id, {})[slot.index] = slot
                for reviewed in model.vtables:
                    if reviewed.slot_count is None or reviewed.subobject_offset is None:
                        continue
                    owner = dtm.getDataType(class_category, reviewed.class_name)
                    if owner is None:
                        report["skipped_owner_types"].append(reviewed.vtable_id)
                        continue
                    receiver_type = owner
                    if reviewed.subobject_offset:
                        receiver_type = dtm.addDataType(
                            StructureDataType(
                                subobject_category, f"{reviewed.vtable_id}.receiver", 4, dtm
                            ),
                            DataTypeConflictHandler.REPLACE_HANDLER,
                        )
                    receiver = PointerDataType(receiver_type, dtm)
                    table = StructureDataType(table_category, reviewed.vtable_id, 0, dtm)
                    for index in range(reviewed.slot_count):
                        slot = slots_by_table.get(reviewed.vtable_id, {}).get(index)
                        target = (
                            None
                            if slot is None
                            else functions.getFunctionAt(space.getAddress(slot.target))
                        )
                        name = slot.name if slot is not None and slot.name else f"slot{index}"
                        definition, source = _slot_prototype(
                            program,
                            target,
                            receiver,
                            table_category,
                            f"{reviewed.vtable_id}.{name}",
                            ParameterDefinitionImpl,
                            FunctionDefinitionDataType,
                            allow_inferred=False,
                        )
                        added = dtm.addDataType(
                            definition, DataTypeConflictHandler.REPLACE_HANDLER
                        )
                        table.add(PointerDataType(added, dtm), 4, name, None)
                        report["prototype_sources"][source] = (
                            report["prototype_sources"].get(source, 0) + 1
                        )
                        report["slots"] += 1
                    table_type = dtm.addDataType(
                        table, DataTypeConflictHandler.REPLACE_HANDLER
                    )
                    _apply_data(program, space.getAddress(reviewed.address), table_type)
                    component = owner.getComponentAt(reviewed.subobject_offset)
                    explicit_vptr = (
                        component is not None
                        and component.getOffset() == reviewed.subobject_offset
                        and component.getLength() == 4
                        and "vptr" in str(component.getFieldName() or "")
                    )
                    if not explicit_vptr:
                        # The table and receiver-specific slot definitions are
                        # still reviewed state. Do not split an opaque base span
                        # merely to make the owner display a secondary vptr.
                        report["unbound_owner_fields"].append(reviewed.vtable_id)
                        report["tables"] += 1
                        continue
                    field_name = (
                        component.getFieldName() if component is not None else None
                    ) or (
                        "vptr"
                        if reviewed.subobject_offset == 0
                        else f"secondary_vptr_{reviewed.subobject_offset:x}"
                    )
                    comment = component.getComment() if component is not None else None
                    owner.replaceAtOffset(
                        reviewed.subobject_offset,
                        PointerDataType(table_type, dtm),
                        4,
                        field_name,
                        comment or "reviewed exact vtable identity",
                    )
                    report["tables"] += 1
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            program.save("apply reviewed typed vtables", None)
    finally:
        from .semantic import dispose_sessions

        dispose_sessions()
        project.close()
    return report
