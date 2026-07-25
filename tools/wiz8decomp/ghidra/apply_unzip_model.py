from __future__ import annotations

from typing import Any

from ..config import Settings
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/srext_unzip"


def _structure(
    dtm: Any, category: Any, name: str, size: int, fields: list[tuple[int, Any, str, str]]
) -> Any:
    from ghidra.program.model.data import DataTypeConflictHandler, StructureDataType

    structure = StructureDataType(category, name, size, dtm)
    for offset, data_type, field_name, comment in fields:
        structure.replaceAtOffset(offset, data_type, data_type.getLength(), field_name, comment)
    return dtm.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def _function_type(
    dtm: Any,
    category: Any,
    name: str,
    return_type: Any,
    arguments: list[tuple[str, Any]],
    calling_convention: str,
) -> Any:
    import jpype
    from ghidra.program.model.data import (
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinition,
        ParameterDefinitionImpl,
    )

    definition = FunctionDefinitionDataType(category, name, dtm)
    definition.setReturnType(return_type)
    definition.setArguments(
        jpype.JArray(ParameterDefinition)(
            [
                ParameterDefinitionImpl(argument_name, data_type, None)
                for argument_name, data_type in arguments
            ]
        )
    )
    definition.setCallingConvention(calling_convention)
    return dtm.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)


def _apply_data(program: Any, address: Any, data_type: Any) -> None:
    from ghidra.program.model.data import DataUtilities

    DataUtilities.createData(
        program,
        address,
        data_type,
        -1,
        False,
        DataUtilities.ClearDataMode.CLEAR_ALL_CONFLICT_DATA,
    )


def apply_unzip_model(settings: Settings, selector: str = "srEXT_Unzip.dll") -> dict[str, Any]:
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
        IntegerDataType,
        PointerDataType,
        VoidDataType,
        WordDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply srEXT_Unzip ABI model")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                category = CategoryPath(CATEGORY)
                byte = ByteDataType.dataType
                char = CharDataType.dataType
                dword = DWordDataType.dataType
                integer = IntegerDataType.dataType
                word = WordDataType.dataType
                void = VoidDataType.dataType
                byte_pointer = PointerDataType(byte, dtm)
                char_pointer = PointerDataType(char, dtm)
                const_char_pointer = char_pointer
                generic_pointer = PointerDataType(dtm)

                print_callback = _function_type(
                    dtm,
                    category,
                    "DLLPRNT",
                    integer,
                    [("text", char_pointer), ("size", dword)],
                    "__stdcall",
                )
                sound_callback = _function_type(dtm, category, "DLLSND", void, [], "__stdcall")
                replace_callback = _function_type(
                    dtm,
                    category,
                    "DLLREPLACE",
                    integer,
                    [("filename", char_pointer)],
                    "__stdcall",
                )
                password_callback = _function_type(
                    dtm,
                    category,
                    "WIZ_DLLPASSWORD",
                    integer,
                    [
                        ("destination", char_pointer),
                        ("destination_size", integer),
                        ("adapter", generic_pointer),
                        ("entry", const_char_pointer),
                        ("archive", const_char_pointer),
                    ],
                    "__stdcall",
                )
                message_callback = _function_type(
                    dtm,
                    category,
                    "DLLMESSAGE",
                    void,
                    [
                        ("uncompressed_size", dword),
                        ("compressed_size", dword),
                        ("factor", dword),
                        ("month", dword),
                        ("day", dword),
                        ("year", dword),
                        ("hour", dword),
                        ("minute", dword),
                        ("code", char),
                        ("filename", char_pointer),
                        ("method", char_pointer),
                        ("crc", dword),
                        ("encrypted", char),
                    ],
                    "__stdcall",
                )
                service_callback = _function_type(
                    dtm,
                    category,
                    "DLLSERVICE",
                    integer,
                    [("filename", const_char_pointer), ("size", dword)],
                    "__stdcall",
                )

                callback_fields = [
                    (
                        0x00,
                        PointerDataType(print_callback, dtm),
                        "print",
                        "Info-ZIP print callback",
                    ),
                    (0x04, PointerDataType(sound_callback, dtm), "sound", "unused sound callback"),
                    (0x08, PointerDataType(replace_callback, dtm), "replace", "overwrite callback"),
                    (
                        0x0C,
                        PointerDataType(password_callback, dtm),
                        "password",
                        "Wizardry five-argument archive callback",
                    ),
                    (
                        0x10,
                        PointerDataType(message_callback, dtm),
                        "SendApplicationMessage",
                        "entry-progress callback",
                    ),
                    (
                        0x14,
                        PointerDataType(service_callback, dtm),
                        "ServCallBk",
                        "service callback",
                    ),
                    (0x18, dword, "TotalSizeComp", "compressed size accumulator"),
                    (0x1C, dword, "TotalSize", "uncompressed size accumulator"),
                    (0x20, integer, "CompFactor", "compression factor"),
                    (0x24, dword, "NumMembers", "member count"),
                    (0x28, word, "cchComment", "archive comment length"),
                ]
                user_functions = _structure(dtm, category, "USERFUNCTIONS", 0x2C, callback_fields)
                callbacks = _structure(
                    dtm,
                    category,
                    "srZipCallbacks",
                    0x30,
                    [
                        (0x00, user_functions, "infozip", "Info-ZIP 5.4 DLL callback record"),
                        (0x2C, generic_pointer, "adapter", "owning srZipAdapter"),
                    ],
                )
                inline_string = _structure(
                    dtm,
                    category,
                    "srInlineString",
                    0x0C,
                    [
                        (
                            0x00,
                            ArrayDataType(byte, 4, 1),
                            "inline_data",
                            "embedded empty/small-string storage",
                        ),
                        (0x04, dword, "size", "allocated size including terminator"),
                        (0x08, char_pointer, "data", "inline_data or srHeap allocation"),
                    ],
                )
                adapter = _structure(
                    dtm,
                    category,
                    "srZipAdapter",
                    0x20,
                    [
                        (
                            0x00,
                            inline_string,
                            "archive_path",
                            "archive path reported through the callback",
                        ),
                        (
                            0x0C,
                            PointerDataType(callbacks, dtm),
                            "callbacks",
                            "owned 0x30-byte callback record",
                        ),
                        (0x10, dword, "callback_state_10", "unresolved callback state"),
                        (0x14, dword, "callback_state_14", "unresolved callback state"),
                        (0x18, dword, "callback_count", "matched archive-member callback count"),
                        (0x1C, integer, "case_insensitive", "ZIP_CASE_INSENSITIVE option"),
                    ],
                )
                opener = _structure(
                    dtm,
                    category,
                    "srZipOpener",
                    0x24,
                    [
                        (0x00, generic_pointer, "vftable", "srIStreamOpener::Opener virtual table"),
                        (0x04, adapter, "adapter", "owned ZIP-path and callback adapter"),
                    ],
                )
                plugin = _structure(
                    dtm,
                    category,
                    "srUnzipPlugin",
                    0x08,
                    [
                        (0x00, generic_pointer, "vftable", "srPlugin virtual table"),
                        (
                            0x04,
                            PointerDataType(opener, dtm),
                            "opener",
                            "registered .zip stream opener",
                        ),
                    ],
                )
                owned_stream = _structure(
                    dtm,
                    category,
                    "srOwnedBinIMStream",
                    0x2C,
                    [
                        (0x00, generic_pointer, "vftable", "local virtual-base adjustment table"),
                        (0x04, generic_pointer, "vbtable", "VC6 virtual-base displacement table"),
                        (
                            0x08,
                            ArrayDataType(byte, 0x0C, 1),
                            "imported_state",
                            "srBinIMStream-owned prefix",
                        ),
                        (
                            0x14,
                            byte_pointer,
                            "allocation",
                            "malloc buffer freed by the local destructor",
                        ),
                        (0x18, dword, "virtual_base_displacement", "VC6 virtual-base state"),
                        (
                            0x1C,
                            ArrayDataType(byte, 0x10, 1),
                            "srBinStream",
                            "virtual srBinStream base",
                        ),
                    ],
                )

                vtable_types = {
                    0x10015030: _structure(
                        dtm,
                        category,
                        "srOwnedBinIMStream_vftable",
                        0x1C,
                        [
                            (0x00, generic_pointer, "scalar_deleting_destructor", "0x10011310"),
                            (0x04, generic_pointer, "getSize", "0x10011380 adjustment thunk"),
                            (
                                0x08,
                                generic_pointer,
                                "seek_from_start",
                                "0x10011390 adjustment thunk",
                            ),
                            (0x0C, generic_pointer, "seek", "0x100113a0 adjustment thunk"),
                            (0x10, generic_pointer, "tell", "0x100113b0 adjustment thunk"),
                            (0x14, generic_pointer, "vget", "0x10011646 import thunk"),
                            (0x18, generic_pointer, "vread", "0x10011640 import thunk"),
                        ],
                    ),
                    0x1001504C: _structure(
                        dtm,
                        category,
                        "srOwnedBinIMStream_vbtable",
                        0x08,
                        [
                            (0x00, integer, "self_displacement", "-4 from the vbptr"),
                            (0x04, integer, "srBinStream_displacement", "+0x18 from the vbptr"),
                        ],
                    ),
                    0x10015054: _structure(
                        dtm,
                        category,
                        "srZipOpener_vftable",
                        0x0C,
                        [
                            (0x00, generic_pointer, "scalar_deleting_destructor", "0x10011220"),
                            (0x04, generic_pointer, "open", "0x100106b0"),
                            (0x08, generic_pointer, "getDescription", "0x10011200"),
                        ],
                    ),
                    0x10015060: _structure(
                        dtm,
                        category,
                        "srUnzipPlugin_vftable",
                        0x08,
                        [
                            (0x00, generic_pointer, "scalar_deleting_destructor", "0x10011280"),
                            (0x04, generic_pointer, "getDescription", "0x10011210"),
                        ],
                    ),
                }

                address_space = program.getAddressFactory().getDefaultAddressSpace()
                for raw_address, data_type in vtable_types.items():
                    _apply_data(program, address_space.getAddress(raw_address), data_type)

                symbol_table = program.getSymbolTable()
                labels = {
                    0x10015030: "srOwnedBinIMStream_vftable",
                    0x1001504C: "srOwnedBinIMStream_vbtable",
                    0x10015054: "srZipOpener_vftable",
                    0x10015060: "srUnzipPlugin_vftable",
                }
                for raw_address, label in labels.items():
                    address = address_space.getAddress(raw_address)
                    primary = symbol_table.getPrimarySymbol(address)
                    if primary is None or primary.getName() != label:
                        symbol_table.createLabel(
                            address, label, SourceType.USER_DEFINED
                        ).setPrimary()

                local_signatures = {
                    0x100115C0: print_callback,
                    0x100115D0: replace_callback,
                    0x100115E0: message_callback,
                    0x100115F0: password_callback,
                }
                for raw_address, signature in local_signatures.items():
                    command = ApplyFunctionSignatureCmd(
                        address_space.getAddress(raw_address),
                        signature,
                        SourceType.USER_DEFINED,
                        True,
                        FunctionRenameOption.NO_CHANGE,
                    )
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to apply signature at 0x{raw_address:08x}: {command.getStatusMsg()}"
                        )

                commit = True
                result.update(
                    {
                        "structures": [
                            str(data_type.getPathName())
                            for data_type in (
                                user_functions,
                                callbacks,
                                inline_string,
                                adapter,
                                opener,
                                plugin,
                                owned_stream,
                                *vtable_types.values(),
                            )
                        ],
                        "applied_vtables": [f"0x{address:08x}" for address in sorted(vtable_types)],
                        "typed_callbacks": [
                            f"0x{address:08x}" for address in sorted(local_signatures)
                        ],
                        "sr_imports": {
                            "functions": 22,
                            "global_objects": 3,
                            "status": "loader-demangled-and-typed",
                        },
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply srEXT_Unzip ABI model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
