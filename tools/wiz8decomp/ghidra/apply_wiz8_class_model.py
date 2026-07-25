from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data, _structure
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/classes"
GR_CYCLE_VTABLE = 0x005ECE78
GR_CYCLE_TARGETS = (
    0x004A5F00,
    0x004A7140,
    0x004A6E20,
    0x004A6FC0,
    0x004A7470,
    0x005E1D9A,
    0x005E1D9A,
    0x005E1D9A,
    0x005E1D9A,
    0x005E1D9A,
    0x004A7DD0,
    0x004A7E10,
    0x005E1D9A,
    0x005E1D9A,
    0x004A72F0,
    0x005E1D9A,
)
GR_CYCLE_SECONDARY_VTABLE = 0x005ECEB8
GR_CYCLE_SECONDARY_TARGETS = (
    0x004A9100,
    0x004538B0,
    0x004538C0,
    0x004A7140,
    0x00456020,
    0x004A8FE0,
    0x004A8FB0,
    0x004A8F70,
    0x004A8F40,
    0x004A9050,
    0x004A9020,
    0x004A8F20,
    0x004A8EF0,
)
MONSTER_VTABLE = 0x005ED200
MONSTER_TARGETS = (
    0x004BEBA0,
    0x004CA9E0,
    0x004BF8C0,
    0x004BF970,
    0x004BF920,
    0x004BF0F0,
    0x004CAE30,
    0x004538B0,
    0x004538C0,
    0x004A7140,
    0x004CA840,
    0x004BFDE0,
    0x004C2BF0,
    0x004A6E20,
    0x004A6FC0,
    0x004C2E60,
    0x004CAA40,
    0x004C3740,
    0x004C3DD0,
    0x004CAA90,
    0x004CAB00,
    0x004C3DF0,
    0x004C3ED0,
    0x004C3790,
    0x004CAB10,
    0x004C32E0,
    0x004C3F00,
    0x004C2100,
    0x004CAAC0,
    0x004C72A0,
    0x004C3E60,
)
MONSTER_INFO_DIALOG_VTABLE = 0x005EF910
MONSTER_INFO_DIALOG_TARGETS = (
    0x005D5EE0,
    0x005D5F90,
    0x005DBDE0,
    0x005D6080,
    0x005D6FA0,
    0x005DC940,
    0x005DC9C0,
    0x005DC9F0,
    0x005DCA70,
    0x005DCCE0,
    0x005B1BE0,
    0x005AD270,
    0x005D6E60,
    0x005D6E70,
)


def apply_wiz8_class_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Install reviewed vtable starts and the first source-backed game class."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        DataTypeConflictHandler,
        PointerDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply reviewed Wiz8 class model")
            commit = False
            try:
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                function_manager = program.getFunctionManager()
                created: list[str] = []
                existing: list[str] = []
                for raw_address in (
                    *GR_CYCLE_TARGETS,
                    *GR_CYCLE_SECONDARY_TARGETS,
                    0x004A6610,
                    *MONSTER_TARGETS,
                    0x004BEE50,
                    *MONSTER_INFO_DIALOG_TARGETS,
                    0x005D5F00,
                ):
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
                category = CategoryPath(CATEGORY)
                byte = ByteDataType.dataType
                generic_pointer = PointerDataType(dtm)
                gr_cycle = _structure(
                    dtm,
                    category,
                    "GrCycle",
                    0x1D8,
                    [
                        (
                            0x00,
                            generic_pointer,
                            "vptr",
                            "primary vtable 0x005ece78",
                        ),
                        (
                            0x04,
                            ArrayDataType(byte, 0x14, 1),
                            "base_and_fields",
                            "primary base-class storage not yet reviewed",
                        ),
                        (
                            0x18,
                            generic_pointer,
                            "secondary_vptr",
                            "secondary vtable 0x005eceb8",
                        ),
                        (
                            0x1C,
                            ArrayDataType(byte, 0x1BC, 1),
                            "fields",
                            "owned graphics-cycle state; fields observed through 0x1d4",
                        ),
                    ],
                )
                monster = _structure(
                    dtm,
                    category,
                    "Monster",
                    0x628,
                    [
                        (0x00, generic_pointer, "vptr", "vtable 0x005ed200"),
                        (
                            0x04,
                            ArrayDataType(byte, 0xA8, 1),
                            "base_and_fields",
                            "base-class and fields not yet reviewed",
                        ),
                        (
                            0xAC,
                            ArrayDataType(byte, 0x1B0, 1),
                            "subobject_array_ac",
                            "27 adjacent 0x10-byte subobjects",
                        ),
                        (
                            0x25C,
                            ArrayDataType(byte, 0x1B0, 1),
                            "subobject_array_25c",
                            "27 adjacent 0x10-byte subobjects",
                        ),
                        (
                            0x40C,
                            ArrayDataType(byte, 0x1B0, 1),
                            "subobject_array_40c",
                            "27 adjacent 0x10-byte subobjects",
                        ),
                        (
                            0x5BC,
                            ArrayDataType(byte, 0x6C, 1),
                            "tail_fields",
                            "fields observed through offset 0x624",
                        ),
                    ],
                )
                dialog = _structure(
                    dtm,
                    category,
                    "MonsterInfoDialog",
                    0x130,
                    [
                        (0x00, generic_pointer, "vptr", "vtable 0x005ef910"),
                        (
                            0x04,
                            ArrayDataType(byte, 0x50, 1),
                            "base_storage",
                            "base-class storage; fields not yet reviewed",
                        ),
                        (
                            0x54,
                            generic_pointer,
                            "constructor_argument_54",
                            "second constructor argument retained at offset 0x54",
                        ),
                        (
                            0x58,
                            ArrayDataType(byte, 0x4C, 1),
                            "member_58",
                            "embedded member constructed and destroyed as one subobject",
                        ),
                        (
                            0xA4,
                            ArrayDataType(byte, 0x48, 1),
                            "member_a4",
                            "embedded member constructed and destroyed as one subobject",
                        ),
                        (
                            0xEC,
                            ArrayDataType(byte, 0x44, 1),
                            "member_ec",
                            "embedded member; minimum extent follows access at 0x129",
                        ),
                    ],
                )
                gr_cycle_vtable_type = dtm.addDataType(
                    ArrayDataType(generic_pointer, len(GR_CYCLE_TARGETS), 4),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                gr_cycle_vtable_address = address_space.getAddress(GR_CYCLE_VTABLE)
                _apply_data(program, gr_cycle_vtable_address, gr_cycle_vtable_type)

                gr_cycle_secondary_vtable_type = dtm.addDataType(
                    ArrayDataType(generic_pointer, len(GR_CYCLE_SECONDARY_TARGETS), 4),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                gr_cycle_secondary_vtable_address = address_space.getAddress(
                    GR_CYCLE_SECONDARY_VTABLE
                )
                _apply_data(
                    program,
                    gr_cycle_secondary_vtable_address,
                    gr_cycle_secondary_vtable_type,
                )

                monster_vtable_type = dtm.addDataType(
                    ArrayDataType(generic_pointer, len(MONSTER_TARGETS), 4),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                monster_vtable_address = address_space.getAddress(MONSTER_VTABLE)
                _apply_data(program, monster_vtable_address, monster_vtable_type)

                dialog_vtable_type = dtm.addDataType(
                    ArrayDataType(generic_pointer, len(MONSTER_INFO_DIALOG_TARGETS), 4),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                dialog_vtable_address = address_space.getAddress(MONSTER_INFO_DIALOG_VTABLE)
                _apply_data(program, dialog_vtable_address, dialog_vtable_type)

                symbol_table = program.getSymbolTable()
                for class_name, symbol_name, vtable_address in (
                    ("GrCycle", "vftable", gr_cycle_vtable_address),
                    (
                        "GrCycle",
                        "vftable_secondary_18",
                        gr_cycle_secondary_vtable_address,
                    ),
                    ("Monster", "vftable", monster_vtable_address),
                    ("MonsterInfoDialog", "vftable", dialog_vtable_address),
                ):
                    namespace = symbol_table.getNamespace(class_name, program.getGlobalNamespace())
                    if namespace is None:
                        namespace = symbol_table.createNameSpace(
                            program.getGlobalNamespace(),
                            class_name,
                            SourceType.USER_DEFINED,
                        )
                    vtable_symbol = next(
                        (
                            symbol
                            for symbol in symbol_table.getSymbols(vtable_address)
                            if symbol.getName() == symbol_name
                            and symbol.getParentNamespace() == namespace
                        ),
                        None,
                    )
                    if vtable_symbol is None:
                        vtable_symbol = symbol_table.createLabel(
                            vtable_address,
                            symbol_name,
                            namespace,
                            SourceType.USER_DEFINED,
                        )
                    vtable_symbol.setPrimary()

                commit = True
                result.update(
                    {
                        "classes": [
                            str(gr_cycle.getPathName()),
                            str(monster.getPathName()),
                            str(dialog.getPathName()),
                        ],
                        "vtables": {
                            f"0x{GR_CYCLE_VTABLE:08x}": len(GR_CYCLE_TARGETS),
                            f"0x{GR_CYCLE_SECONDARY_VTABLE:08x}": len(GR_CYCLE_SECONDARY_TARGETS),
                            f"0x{MONSTER_VTABLE:08x}": len(MONSTER_TARGETS),
                            f"0x{MONSTER_INFO_DIALOG_VTABLE:08x}": len(MONSTER_INFO_DIALOG_TARGETS),
                        },
                        "created_functions": created,
                        "existing_functions": existing,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply reviewed Wiz8 class model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
