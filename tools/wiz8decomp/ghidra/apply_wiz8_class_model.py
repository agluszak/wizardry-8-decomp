from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data, _structure
from .project import resolve_program_name
from .reviewed_class_model import load_reviewed_class_model


def apply_reviewed_class_model(
    settings: Settings,
    selector: str,
    *,
    evidence_program: str,
    materialize: bool = True,
) -> dict[str, Any]:
    """Materialize canonical class, field, and vtable records in Ghidra."""

    model = load_reviewed_class_model(settings.repo_dir, evidence_program)
    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        DataTypeConflictHandler,
        IntegerDataType,
        PointerDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name, "evidence_program": evidence_program}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply canonical reviewed class model")
            commit = False
            try:
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                function_manager = program.getFunctionManager()
                created: list[str] = []
                existing: list[str] = []
                for raw_address in sorted({slot.target for slot in model.slots}):
                    address = address_space.getAddress(raw_address)
                    if function_manager.getFunctionAt(address) is not None:
                        existing.append(f"0x{raw_address:08x}")
                        continue
                    command = CreateFunctionCmd(address)
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to create function at 0x{raw_address:08x}: "
                            f"{command.getStatusMsg()}"
                        )
                    created.append(f"0x{raw_address:08x}")

                dtm = program.getDataTypeManager()
                category = CategoryPath(f"/{evidence_program}/classes")
                byte = ByteDataType.dataType
                generic_pointer = PointerDataType(dtm)
                fields_by_class: dict[str, list[Any]] = {}
                for field in model.fields:
                    if field.data_type == "pointer":
                        data_type = generic_pointer
                    elif field.data_type == "int32":
                        data_type = IntegerDataType.dataType
                    else:
                        data_type = ArrayDataType(byte, field.size, 1)
                    fields_by_class.setdefault(field.class_name, []).append(
                        (field.offset, data_type, field.name, field.description)
                    )

                structures: list[str] = []
                for reviewed_class in model.classes:
                    fields = fields_by_class.get(reviewed_class.name)
                    if not fields:
                        continue
                    if reviewed_class.size is None:
                        raise RuntimeError(f"{reviewed_class.name} has fields but no reviewed size")
                    structure = _structure(
                        dtm,
                        category,
                        reviewed_class.name,
                        reviewed_class.size,
                        fields,
                    )
                    structures.append(str(structure.getPathName()))

                typed_ranges = sorted(
                    (
                        vtable.address,
                        vtable.address + vtable.slot_count * 4,
                    )
                    for vtable in model.vtables
                    if vtable.slot_count is not None
                )
                merged_ranges: list[list[int]] = []
                for start, end in typed_ranges:
                    if merged_ranges and start < merged_ranges[-1][1]:
                        merged_ranges[-1][1] = max(merged_ranges[-1][1], end)
                    else:
                        merged_ranges.append([start, end])
                for start, end in merged_ranges:
                    region_type = dtm.addDataType(
                        ArrayDataType(generic_pointer, (end - start) // 4, 4),
                        DataTypeConflictHandler.REPLACE_HANDLER,
                    )
                    _apply_data(program, address_space.getAddress(start), region_type)

                for vtable in model.vtables:
                    address = address_space.getAddress(vtable.address)

                    symbol_table = program.getSymbolTable()
                    namespace = symbol_table.getNamespace(
                        vtable.class_name, program.getGlobalNamespace()
                    )
                    if namespace is None:
                        namespace = symbol_table.createNameSpace(
                            program.getGlobalNamespace(),
                            vtable.class_name,
                            SourceType.USER_DEFINED,
                        )
                    if vtable.kind == "primary":
                        symbol_name = "vftable"
                    elif vtable.subobject_offset is not None:
                        symbol_name = f"vftable_secondary_{vtable.subobject_offset:x}"
                    else:
                        symbol_name = "vftable_secondary_unknown"
                    symbol = next(
                        (
                            candidate
                            for candidate in symbol_table.getSymbols(address)
                            if candidate.getName() == symbol_name
                            and candidate.getParentNamespace() == namespace
                        ),
                        None,
                    )
                    if symbol is None:
                        symbol = symbol_table.createLabel(
                            address, symbol_name, namespace, SourceType.USER_DEFINED
                        )
                    symbol.setPrimary()

                commit = True
                result.update(
                    {
                        "classes": structures,
                        "vtables": {
                            f"0x{item.address:08x}": item.slot_count for item in model.vtables
                        },
                        "created_functions": created,
                        "existing_functions": existing,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply canonical reviewed class model", pyghidra.task_monitor())
    finally:
        project.close()
    return result


def apply_wiz8_class_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    return apply_reviewed_class_model(
        settings, selector, evidence_program="wiz8", materialize=materialize
    )
