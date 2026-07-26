from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _function_type, _structure
from .project import resolve_program_name

CATEGORY = "/wiz8/zlib_1_0_4"


def apply_zlib_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    """Install the reviewed zlib 1.0.4 layouts and function prototypes."""

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
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
            transaction = program.startTransaction("apply zlib 1.0.4 model")
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
                generic_pointer = PointerDataType(dtm)
                byte_pointer = PointerDataType(byte, dtm)
                char_pointer = PointerDataType(char, dtm)
                dword_pointer = PointerDataType(dword, dtm)
                word_pointer = PointerDataType(word, dtm)

                alloc_func = _function_type(
                    dtm,
                    category,
                    "alloc_func",
                    generic_pointer,
                    [("opaque", generic_pointer), ("items", dword), ("size", dword)],
                    "__cdecl",
                )
                free_func = _function_type(
                    dtm,
                    category,
                    "free_func",
                    void,
                    [("opaque", generic_pointer), ("allocation", generic_pointer)],
                    "__cdecl",
                )
                check_func = _function_type(
                    dtm,
                    category,
                    "check_func",
                    dword,
                    [("check", dword), ("buffer", byte_pointer), ("length", dword)],
                    "__cdecl",
                )

                inflate_huft = _structure(
                    dtm,
                    category,
                    "inflate_huft",
                    0x08,
                    [
                        (0x00, byte, "exop", "extra bits or operation"),
                        (0x01, byte, "bits", "bits in this code or subcode"),
                        (0x02, word, "word_padding", "32-bit union padding"),
                        (0x04, dword, "base_or_next", "literal/base value or next-table pointer"),
                    ],
                )
                huft_pointer = PointerDataType(inflate_huft, dtm)
                huft_pointer_pointer = PointerDataType(huft_pointer, dtm)

                inflate_codes_state = _structure(
                    dtm,
                    category,
                    "inflate_codes_state",
                    0x1C,
                    [
                        (0x00, integer, "mode", "inflate code decoder mode"),
                        (0x04, dword, "length", "current match length"),
                        (0x08, dword, "sub_value_08", "tree pointer, literal, or extra-bit count"),
                        (0x0C, dword, "sub_value_0c", "tree depth or match distance"),
                        (0x10, byte, "literal_bits", "literal-tree branch width"),
                        (0x11, byte, "distance_bits", "distance-tree branch width"),
                        (0x12, word, "alignment", "VC6 structure alignment"),
                        (0x14, huft_pointer, "literal_tree", "literal/length/eob tree"),
                        (0x18, huft_pointer, "distance_tree", "distance tree"),
                    ],
                )
                codes_pointer = PointerDataType(inflate_codes_state, dtm)

                inflate_blocks_state = _structure(
                    dtm,
                    category,
                    "inflate_blocks_state",
                    0x3C,
                    [
                        (0x00, integer, "mode", "inflate block mode"),
                        (
                            0x04,
                            ArrayDataType(byte, 0x14, 1),
                            "sub",
                            "stored/tree/code mode union",
                        ),
                        (0x18, dword, "last", "last-block flag"),
                        (0x1C, dword, "bit_count", "bits in bit buffer"),
                        (0x20, dword, "bit_buffer", "input bit buffer"),
                        (0x24, byte_pointer, "window", "sliding-window start"),
                        (0x28, byte_pointer, "end", "one byte after window"),
                        (0x2C, byte_pointer, "read", "window read cursor"),
                        (0x30, byte_pointer, "write", "window write cursor"),
                        (0x34, PointerDataType(check_func, dtm), "check", "checksum callback"),
                        (0x38, dword, "check_value", "current output checksum"),
                    ],
                )
                blocks_pointer = PointerDataType(inflate_blocks_state, dtm)

                inflate_state = _structure(
                    dtm,
                    category,
                    "inflate_state",
                    0x18,
                    [
                        (0x00, integer, "mode", "zlib wrapper mode"),
                        (
                            0x04,
                            ArrayDataType(byte, 0x08, 1),
                            "sub",
                            "method/check/marker union",
                        ),
                        (0x0C, integer, "nowrap", "raw-deflate flag"),
                        (0x10, dword, "window_bits", "base-two window size"),
                        (0x14, blocks_pointer, "blocks", "inflate block decoder"),
                    ],
                )
                inflate_state_pointer = PointerDataType(inflate_state, dtm)

                z_stream = _structure(
                    dtm,
                    category,
                    "z_stream",
                    0x38,
                    [
                        (0x00, byte_pointer, "next_in", "next input byte"),
                        (0x04, dword, "avail_in", "available input bytes"),
                        (0x08, dword, "total_in", "total input consumed"),
                        (0x0C, byte_pointer, "next_out", "next output byte"),
                        (0x10, dword, "avail_out", "remaining output capacity"),
                        (0x14, dword, "total_out", "total output produced"),
                        (0x18, char_pointer, "msg", "last error message"),
                        (0x1C, inflate_state_pointer, "state", "inflate private state"),
                        (0x20, PointerDataType(alloc_func, dtm), "zalloc", "allocator callback"),
                        (0x24, PointerDataType(free_func, dtm), "zfree", "deallocator callback"),
                        (0x28, generic_pointer, "opaque", "callback context"),
                        (0x2C, integer, "data_type", "binary, text, or unknown"),
                        (0x30, dword, "adler", "Adler-32 value"),
                        (0x34, dword, "reserved", "reserved by zlib 1.0.4"),
                    ],
                )
                stream_pointer = PointerDataType(z_stream, dtm)

                ct_data = _structure(
                    dtm,
                    category,
                    "ct_data",
                    0x04,
                    [
                        (0x00, word, "frequency_or_code", "frequency count or bit string"),
                        (0x02, word, "parent_or_length", "parent node or bit length"),
                    ],
                )
                ct_pointer = PointerDataType(ct_data, dtm)
                tree_desc = _structure(
                    dtm,
                    category,
                    "tree_desc",
                    0x0C,
                    [
                        (0x00, ct_pointer, "dynamic_tree", "dynamic Huffman tree"),
                        (0x04, integer, "max_code", "largest non-zero code"),
                        (0x08, generic_pointer, "static_description", "static tree metadata"),
                    ],
                )
                tree_pointer = PointerDataType(tree_desc, dtm)
                deflate_state = _structure(
                    dtm,
                    category,
                    "deflate_state",
                    1,
                    [(0x00, byte, "opaque", "full layout is not needed by the retained API model")],
                )
                deflate_pointer = PointerDataType(deflate_state, dtm)

                signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x00415820: (
                        generic_pointer,
                        [("opaque", generic_pointer), ("items", dword), ("size", dword)],
                    ),
                    0x00415840: (
                        void,
                        [("opaque", generic_pointer), ("allocation", generic_pointer)],
                    ),
                    0x00415850: (stream_pointer, [("input", byte_pointer), ("input_size", dword)]),
                    0x004158B0: (
                        dword,
                        [
                            ("stream", stream_pointer),
                            ("output", byte_pointer),
                            ("output_size", dword),
                        ],
                    ),
                    0x004158F0: (void, [("stream", stream_pointer)]),
                    0x00415910: (integer, [("stream", stream_pointer)]),
                    0x00415960: (integer, [("stream", stream_pointer)]),
                    0x004159C0: (
                        integer,
                        [
                            ("stream", stream_pointer),
                            ("window_bits", integer),
                            ("version", char_pointer),
                            ("stream_size", integer),
                        ],
                    ),
                    0x00415AD0: (
                        integer,
                        [
                            ("stream", stream_pointer),
                            ("version", char_pointer),
                            ("stream_size", integer),
                        ],
                    ),
                    0x00415AF0: (integer, [("stream", stream_pointer), ("flush", integer)]),
                    0x00415F10: (void, [("stream", stream_pointer)]),
                    0x00415F60: (integer, [("state", deflate_pointer), ("flush", integer)]),
                    0x00416070: (void, [("state", deflate_pointer)]),
                    0x00416180: (
                        integer,
                        [("stream", stream_pointer), ("buffer", char_pointer), ("size", dword)],
                    ),
                    0x004161F0: (integer, [("state", deflate_pointer), ("flush", integer)]),
                    0x00416450: (dword, [("state", deflate_pointer), ("current_match", dword)]),
                    0x004165C0: (integer, [("state", deflate_pointer), ("flush", integer)]),
                    0x004168B0: (
                        void,
                        [
                            ("state", blocks_pointer),
                            ("stream", stream_pointer),
                            ("check", dword_pointer),
                        ],
                    ),
                    0x00416940: (
                        blocks_pointer,
                        [
                            ("stream", stream_pointer),
                            ("check", PointerDataType(check_func, dtm)),
                            ("window_size", dword),
                        ],
                    ),
                    0x004169B0: (
                        integer,
                        [
                            ("state", blocks_pointer),
                            ("stream", stream_pointer),
                            ("result", integer),
                        ],
                    ),
                    0x004177D0: (
                        integer,
                        [
                            ("state", blocks_pointer),
                            ("stream", stream_pointer),
                            ("check", dword_pointer),
                        ],
                    ),
                    0x00417810: (
                        dword,
                        [("adler", dword), ("buffer", byte_pointer), ("length", dword)],
                    ),
                    0x00417940: (
                        generic_pointer,
                        [("opaque", generic_pointer), ("items", dword), ("size", dword)],
                    ),
                    0x00417960: (
                        void,
                        [("opaque", generic_pointer), ("allocation", generic_pointer)],
                    ),
                    0x00417970: (void, [("state", deflate_pointer)]),
                    0x004179E0: (
                        void,
                        [("tree", ct_pointer), ("max_code", integer), ("bit_counts", word_pointer)],
                    ),
                    0x00417A60: (
                        void,
                        [
                            ("state", deflate_pointer),
                            ("buffer", char_pointer),
                            ("stored_length", dword),
                            ("end_of_file", integer),
                        ],
                    ),
                    0x00417B20: (
                        dword,
                        [
                            ("state", deflate_pointer),
                            ("buffer", char_pointer),
                            ("stored_length", dword),
                            ("end_of_file", integer),
                        ],
                    ),
                    0x00417D40: (void, [("state", deflate_pointer), ("description", tree_pointer)]),
                    0x00417F50: (
                        void,
                        [("state", deflate_pointer), ("tree", ct_pointer), ("heap_index", integer)],
                    ),
                    0x00418000: (void, [("state", deflate_pointer), ("description", tree_pointer)]),
                    0x004181E0: (integer, [("state", deflate_pointer)]),
                    0x00418250: (
                        void,
                        [("state", deflate_pointer), ("tree", ct_pointer), ("max_code", integer)],
                    ),
                    0x00418340: (
                        void,
                        [
                            ("state", deflate_pointer),
                            ("literal_codes", integer),
                            ("distance_codes", integer),
                            ("bit_length_codes", integer),
                        ],
                    ),
                    0x004185A0: (
                        void,
                        [("state", deflate_pointer), ("tree", ct_pointer), ("max_code", integer)],
                    ),
                    0x00418B20: (
                        integer,
                        [
                            ("state", deflate_pointer),
                            ("distance", dword),
                            ("literal_or_length", dword),
                        ],
                    ),
                    0x00418C30: (
                        void,
                        [
                            ("state", deflate_pointer),
                            ("literal_tree", ct_pointer),
                            ("distance_tree", ct_pointer),
                        ],
                    ),
                    0x004190A0: (void, [("state", deflate_pointer)]),
                    0x00419110: (dword, [("code", dword), ("length", integer)]),
                    0x00419140: (void, [("state", deflate_pointer)]),
                    0x004191A0: (
                        void,
                        [
                            ("state", deflate_pointer),
                            ("buffer", char_pointer),
                            ("length", dword),
                            ("with_header", integer),
                        ],
                    ),
                    0x00419230: (
                        integer,
                        [
                            ("code_lengths", dword_pointer),
                            ("tree_bits", dword_pointer),
                            ("tree", huft_pointer_pointer),
                            ("stream", stream_pointer),
                        ],
                    ),
                    0x00419290: (
                        integer,
                        [
                            ("code_lengths", dword_pointer),
                            ("code_count", dword),
                            ("simple_count", dword),
                            ("base_values", dword_pointer),
                            ("extra_bits", dword_pointer),
                            ("tree", huft_pointer_pointer),
                            ("max_bits", dword_pointer),
                            ("stream", stream_pointer),
                        ],
                    ),
                    0x00419760: (
                        integer,
                        [
                            ("literal_count", dword),
                            ("distance_count", dword),
                            ("code_lengths", dword_pointer),
                            ("literal_bits", dword_pointer),
                            ("distance_bits", dword_pointer),
                            ("literal_tree", huft_pointer_pointer),
                            ("distance_tree", huft_pointer_pointer),
                            ("stream", stream_pointer),
                        ],
                    ),
                    0x00419850: (
                        integer,
                        [
                            ("literal_bits", dword_pointer),
                            ("distance_bits", dword_pointer),
                            ("literal_tree", huft_pointer_pointer),
                            ("distance_tree", huft_pointer_pointer),
                        ],
                    ),
                    0x004199C0: (integer, [("tree", huft_pointer), ("stream", stream_pointer)]),
                    0x00419A00: (
                        codes_pointer,
                        [
                            ("literal_bits", dword),
                            ("distance_bits", dword),
                            ("literal_tree", huft_pointer),
                            ("distance_tree", huft_pointer),
                            ("stream", stream_pointer),
                        ],
                    ),
                    0x00419A40: (
                        integer,
                        [
                            ("blocks", blocks_pointer),
                            ("stream", stream_pointer),
                            ("result", integer),
                        ],
                    ),
                    0x0041A340: (void, [("codes", codes_pointer), ("stream", stream_pointer)]),
                    0x0041A360: (
                        integer,
                        [
                            ("blocks", blocks_pointer),
                            ("stream", stream_pointer),
                            ("result", integer),
                        ],
                    ),
                    0x0041A4A0: (
                        integer,
                        [
                            ("literal_bits", dword),
                            ("distance_bits", dword),
                            ("literal_tree", huft_pointer),
                            ("distance_tree", huft_pointer),
                            ("blocks", blocks_pointer),
                            ("stream", stream_pointer),
                        ],
                    ),
                }

                address_space = program.getAddressFactory().getDefaultAddressSpace()
                applied: list[str] = []
                for raw_address, (return_type, arguments) in signatures.items():
                    function = program.getFunctionManager().getFunctionAt(
                        address_space.getAddress(raw_address)
                    )
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
                        address_space.getAddress(raw_address),
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
                    applied.append(f"0x{raw_address:08x}")

                commit = True
                result.update(
                    {
                        "structures": [
                            str(data_type.getPathName())
                            for data_type in (
                                z_stream,
                                inflate_state,
                                inflate_blocks_state,
                                inflate_codes_state,
                                inflate_huft,
                                ct_data,
                                tree_desc,
                            )
                        ],
                        "typed_functions": applied,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply zlib 1.0.4 model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
