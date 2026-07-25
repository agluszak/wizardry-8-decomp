from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data, _function_type, _structure
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/formats/slf"


def apply_wiz8_format_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Install the binary-grounded SLF archive types and parser signatures."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        CharDataType,
        DWordDataType,
        PointerDataType,
        QWordDataType,
        WordDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply Wizardry 8 SLF format model")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                category = CategoryPath(CATEGORY)
                byte = ByteDataType.dataType
                char = CharDataType.dataType
                word = WordDataType.dataType
                dword = DWordDataType.dataType
                qword = QWordDataType.dataType
                generic_pointer = PointerDataType(dtm)
                char_pointer = PointerDataType(char, dtm)

                header = _structure(
                    dtm,
                    category,
                    "W8SlfHeader",
                    0x214,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "archive_name", "archive name"),
                        (0x100, ArrayDataType(char, 256, 1), "base_path", "payload base path"),
                        (0x200, dword, "file_count", "number of EOF directory records"),
                        (0x204, dword, "second_count", "meaning not yet established"),
                        (0x208, dword, "unknown_208", "unreviewed header field"),
                        (0x20C, dword, "unknown_20c", "unreviewed header field"),
                        (0x210, dword, "unknown_210", "unreviewed header field"),
                    ],
                )
                directory_entry = _structure(
                    dtm,
                    category,
                    "W8SlfDirectoryEntry",
                    0x118,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "path", "payload path"),
                        (0x100, dword, "data_offset", "payload offset from archive start"),
                        (0x104, dword, "data_size", "payload byte size"),
                        (0x108, dword, "status_108", "low byte zero means active"),
                        (0x10C, qword, "file_time", "Windows FILETIME representation"),
                        (0x114, dword, "unknown_114", "unreviewed directory field"),
                    ],
                )
                live_entry = _structure(
                    dtm,
                    category,
                    "W8SlfLiveEntry",
                    0x0C,
                    [
                        (0x00, char_pointer, "path", "separately allocated path"),
                        (0x04, dword, "data_size", "payload byte size"),
                        (0x08, dword, "data_offset", "payload offset"),
                    ],
                )
                live_entry_pointer = PointerDataType(live_entry, dtm)
                archive_state = _structure(
                    dtm,
                    category,
                    "W8SlfArchiveState",
                    0x28,
                    [
                        (0x00, char_pointer, "base_path", "copied from W8SlfHeader"),
                        (0x04, generic_pointer, "archive_file", "Win32 file handle"),
                        (0x08, word, "active_entry_count", "retained directory records"),
                        (0x0A, byte, "is_open", "nonzero after successful initialization"),
                        (0x0B, byte, "unknown_0b", "unreviewed state byte"),
                        (0x0C, dword, "unknown_0c", "unreviewed state field"),
                        (0x10, dword, "unknown_10", "initialized to zero"),
                        (0x14, dword, "lookup_bucket_count", "initialized to 0x14"),
                        (0x18, live_entry_pointer, "entries", "active entry array"),
                        (0x1C, generic_pointer, "lookup_buckets", "0x140-byte allocation"),
                        (0x20, generic_pointer, "mapping_handle", "optional mapping handle"),
                        (0x24, generic_pointer, "mapping_view", "optional read-only view"),
                    ],
                )
                archive_state_pointer = PointerDataType(archive_state, dtm)
                configuration = _structure(
                    dtm,
                    category,
                    "W8SlfConfiguration",
                    0x103,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "archive_path", "configured path"),
                        (0x100, byte, "enabled", "whether this slot is considered"),
                        (0x101, byte, "allow_fallback", "try the alternate base path"),
                        (0x102, byte, "map_file", "create a read-only file mapping"),
                    ],
                )

                address_space = program.getAddressFactory().getDefaultAddressSpace()
                _apply_data(
                    program,
                    address_space.getAddress(0x006000C8),
                    ArrayDataType(configuration, 6, 0x103),
                )
                _apply_data(
                    program,
                    address_space.getAddress(0x006EB724),
                    archive_state_pointer,
                )

                signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x004126F0: (dword, []),
                    0x00412A10: (dword, []),
                    0x00412BB0: (
                        dword,
                        [
                            ("archive_path", char_pointer),
                            ("state", archive_state_pointer),
                            ("allow_fallback", byte),
                        ],
                    ),
                }
                typed_functions: list[str] = []
                for raw_address, (return_type, arguments) in signatures.items():
                    address = address_space.getAddress(raw_address)
                    function = program.getFunctionManager().getFunctionAt(address)
                    if function is None:
                        raise RuntimeError(f"no function at 0x{raw_address:08x}")
                    signature = _function_type(
                        dtm,
                        category,
                        f"signature_{function.getName()}",
                        return_type,
                        arguments,
                        "__cdecl",
                    )
                    command = ApplyFunctionSignatureCmd(
                        address,
                        signature,
                        SourceType.USER_DEFINED,
                        True,
                        FunctionRenameOption.NO_CHANGE,
                    )
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to apply signature at 0x{raw_address:08x}: "
                            f"{command.getStatusMsg()}"
                        )
                    typed_functions.append(f"0x{raw_address:08x}")

                symbol_table = program.getSymbolTable()
                for raw_address, name in (
                    (0x006000C8, "g_slf_configurations"),
                    (0x006EB724, "g_slf_archives"),
                ):
                    address = address_space.getAddress(raw_address)
                    symbol = symbol_table.getPrimarySymbol(address)
                    if symbol is None:
                        symbol = symbol_table.createLabel(address, name, SourceType.USER_DEFINED)
                    elif symbol.getName() != name:
                        symbol.setName(name, SourceType.USER_DEFINED)

                commit = True
                result.update(
                    {
                        "structures": [
                            str(data_type.getPathName())
                            for data_type in (
                                header,
                                directory_entry,
                                live_entry,
                                archive_state,
                                configuration,
                            )
                        ],
                        "typed_functions": typed_functions,
                        "typed_globals": ["0x006000c8", "0x006eb724"],
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply Wizardry 8 SLF format model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
