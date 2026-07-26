from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data, _function_type, _structure
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/sgp"


def apply_sgp_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Install the reviewed source-backed SGP types, globals, and prototypes."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        CharDataType,
        DataTypeConflictHandler,
        DWordDataType,
        EnumDataType,
        IntegerDataType,
        PointerDataType,
        QWordDataType,
        ShortDataType,
        SignedByteDataType,
        TypedefDataType,
        VoidDataType,
        WordDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply source-backed SGP model")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                category = CategoryPath(CATEGORY)
                byte = ByteDataType.dataType
                char = CharDataType.dataType
                dword = DWordDataType.dataType
                integer = IntegerDataType.dataType
                qword = QWordDataType.dataType
                signed_byte = SignedByteDataType.dataType
                short = ShortDataType.dataType
                void = VoidDataType.dataType
                word = WordDataType.dataType
                generic_pointer = PointerDataType(dtm)

                def com_interface(name: str) -> Any:
                    return _structure(
                        dtm,
                        category,
                        name,
                        4,
                        [(0x00, generic_pointer, "lpVtbl", "COM interface vtable")],
                    )

                direct_draw = com_interface("IDirectDraw2")
                surface1 = com_interface("IDirectDrawSurface")
                surface2 = com_interface("IDirectDrawSurface2")
                palette = com_interface("IDirectDrawPalette")
                iunknown = com_interface("IUnknown")

                rect = _structure(
                    dtm,
                    category,
                    "RECT",
                    0x10,
                    [
                        (0x00, integer, "left", "left edge"),
                        (0x04, integer, "top", "top edge"),
                        (0x08, integer, "right", "right edge"),
                        (0x0C, integer, "bottom", "bottom edge"),
                    ],
                )
                color_key = _structure(
                    dtm,
                    category,
                    "DDCOLORKEY",
                    0x08,
                    [
                        (0x00, dword, "dwColorSpaceLowValue", "low color-space value"),
                        (0x04, dword, "dwColorSpaceHighValue", "high color-space value"),
                    ],
                )
                pixel_format = _structure(
                    dtm,
                    category,
                    "DDPIXELFORMAT",
                    0x20,
                    [
                        (0x00, dword, "dwSize", "structure size"),
                        (0x04, dword, "dwFlags", "pixel-format flags"),
                        (0x08, dword, "dwFourCC", "FourCC or union member"),
                        (0x0C, dword, "dwRGBBitCount", "RGB bit count or union member"),
                        (0x10, dword, "dwRBitMask", "red or union mask"),
                        (0x14, dword, "dwGBitMask", "green or union mask"),
                        (0x18, dword, "dwBBitMask", "blue or union mask"),
                        (0x1C, dword, "dwRGBAlphaBitMask", "alpha or union mask"),
                    ],
                )
                caps = _structure(
                    dtm,
                    category,
                    "DDSCAPS",
                    0x04,
                    [(0x00, dword, "dwCaps", "surface capability flags")],
                )
                surface_desc = _structure(
                    dtm,
                    category,
                    "DDSURFACEDESC",
                    0x6C,
                    [
                        (0x00, dword, "dwSize", "structure size"),
                        (0x04, dword, "dwFlags", "valid-field flags"),
                        (0x08, dword, "dwHeight", "surface height"),
                        (0x0C, dword, "dwWidth", "surface width"),
                        (0x10, integer, "lPitch", "pitch or linear-size union"),
                        (0x14, dword, "dwBackBufferCount", "back-buffer count"),
                        (0x18, dword, "dwZBufferBitDepth", "z-buffer depth"),
                        (0x1C, dword, "dwAlphaBitDepth", "alpha depth"),
                        (0x20, dword, "dwReserved", "reserved"),
                        (0x24, generic_pointer, "lpSurface", "surface memory"),
                        (0x28, color_key, "ddckCKDestOverlay", "destination overlay key"),
                        (0x30, color_key, "ddckCKDestBlt", "destination blit key"),
                        (0x38, color_key, "ddckCKSrcOverlay", "source overlay key"),
                        (0x40, color_key, "ddckCKSrcBlt", "source blit key"),
                        (0x48, pixel_format, "ddpfPixelFormat", "pixel format"),
                        (0x68, caps, "ddsCaps", "surface capabilities"),
                    ],
                )
                palette_entry = _structure(
                    dtm,
                    category,
                    "PALETTEENTRY",
                    0x04,
                    [
                        (0x00, byte, "peRed", "red component"),
                        (0x01, byte, "peGreen", "green component"),
                        (0x02, byte, "peBlue", "blue component"),
                        (0x03, byte, "peFlags", "entry flags"),
                    ],
                )
                blt_fx = _structure(
                    dtm,
                    category,
                    "DDBLTFX",
                    0x64,
                    [(0x00, ArrayDataType(byte, 0x64, 1), "raw", "DirectDraw blit effects")],
                )

                boolean = dtm.addDataType(
                    TypedefDataType(category, "BOOLEAN", byte, dtm),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                hcontainer = dtm.addDataType(
                    TypedefDataType(category, "HCONTAINER", generic_pointer, dtm),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                container_handles = {
                    name: dtm.addDataType(
                        TypedefDataType(category, name, hcontainer, dtm),
                        DataTypeConflictHandler.REPLACE_HANDLER,
                    )
                    for name in ("HSTACK", "HQUEUE", "HLIST", "HORDLIST")
                }
                compare_function = _function_type(
                    dtm,
                    category,
                    "ContainerCompareFunction",
                    signed_byte,
                    [
                        ("left", generic_pointer),
                        ("right", generic_pointer),
                        ("size", dword),
                    ],
                    "__cdecl",
                )
                compare_pointer = PointerDataType(compare_function, dtm)
                stack_header = _structure(
                    dtm,
                    category,
                    "StackHeader",
                    0x0C,
                    [
                        (0x00, dword, "uiTotal_items", "source item count"),
                        (0x04, dword, "uiSiz_of_elem", "source element size"),
                        (0x08, dword, "uiMax_size", "source allocation size"),
                    ],
                )
                queue_header = _structure(
                    dtm,
                    category,
                    "QueueHeader",
                    0x14,
                    [
                        (0x00, dword, "uiTotal_items", "source item count"),
                        (0x04, dword, "uiSiz_of_elem", "source element size"),
                        (0x08, dword, "uiMax_size", "source allocation size"),
                        (0x0C, dword, "uiHead", "source circular-buffer head"),
                        (0x10, dword, "uiTail", "source circular-buffer tail"),
                    ],
                )
                list_header = _structure(
                    dtm,
                    category,
                    "ListHeader",
                    0x14,
                    [
                        (0x00, dword, "uiTotal_items", "source item count"),
                        (0x04, dword, "uiSiz_of_elem", "source element size"),
                        (0x08, dword, "uiMax_size", "source allocation size"),
                        (0x0C, dword, "uiHead", "source circular-buffer head"),
                        (0x10, dword, "uiTail", "source circular-buffer tail"),
                    ],
                )
                ord_list_header = _structure(
                    dtm,
                    category,
                    "OrdListHeader",
                    0x18,
                    [
                        (0x00, dword, "uiTotal_items", "source item count"),
                        (0x04, dword, "uiSiz_of_elem", "source element size"),
                        (0x08, dword, "uiMax_size", "source allocation size"),
                        (0x0C, dword, "uiHead", "source circular-buffer head"),
                        (0x10, dword, "uiTail", "source circular-buffer tail"),
                        (0x14, compare_pointer, "pCompare", "source ordering callback"),
                    ],
                )
                hwfile = dtm.addDataType(
                    TypedefDataType(category, "HWFILE", dword, dtm),
                    DataTypeConflictHandler.REPLACE_HANDLER,
                )
                sgp_filetime = _structure(
                    dtm,
                    category,
                    "SGP_FILETIME",
                    0x08,
                    [
                        (0x00, dword, "dwLowDateTime", "low 32 bits of Win32 file time"),
                        (0x04, dword, "dwHighDateTime", "high 32 bits of Win32 file time"),
                    ],
                )
                get_file = _structure(
                    dtm,
                    category,
                    "GETFILESTRUCT",
                    0x110,
                    [
                        (0x000, integer, "iFindHandle", "index in FileMan's 20-slot find table"),
                        (0x004, ArrayDataType(char, 260, 1), "zFileName", "Win32 file name"),
                        (0x108, dword, "uiFileSize", "low 32-bit file size or -1 for directories"),
                        (0x10C, dword, "uiFileAttribs", "SGP file-attribute flags"),
                    ],
                )
                win32_find_data = _structure(
                    dtm,
                    category,
                    "WIN32_FIND_DATAA",
                    0x140,
                    [
                        (0x000, dword, "dwFileAttributes", "Win32 file attributes"),
                        (0x004, sgp_filetime, "ftCreationTime", "creation time"),
                        (0x00C, sgp_filetime, "ftLastAccessTime", "last access time"),
                        (0x014, sgp_filetime, "ftLastWriteTime", "last write time"),
                        (0x01C, dword, "nFileSizeHigh", "high file-size word"),
                        (0x020, dword, "nFileSizeLow", "low file-size word"),
                        (0x024, dword, "dwReserved0", "reserved"),
                        (0x028, dword, "dwReserved1", "reserved"),
                        (0x02C, ArrayDataType(char, 260, 1), "cFileName", "long file name"),
                        (0x130, ArrayDataType(char, 14, 1), "cAlternateFileName", "8.3 file name"),
                    ],
                )

                # LibraryDataBase.h is the source authority for the SLF disk
                # records and the common prefix of Wizardry's archive state.
                # Wizardry extends the released 0x20-byte LibraryHeaderStruct
                # with its independently observed mapping handle and view.
                lib_header = _structure(
                    dtm,
                    category,
                    "LIBHEADER",
                    0x214,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "sLibName", "source name"),
                        (
                            0x100,
                            ArrayDataType(char, 256, 1),
                            "sPathToLibrary",
                            "source base path",
                        ),
                        (0x200, integer, "iEntries", "source directory count"),
                        (0x204, integer, "iUsed", "source used count"),
                        (0x208, word, "iSort", "source sort mode"),
                        (0x20A, word, "iVersion", "source archive version"),
                        (
                            0x20C,
                            boolean,
                            "fContainsSubDirectories",
                            "source subdirectory flag",
                        ),
                        (0x210, integer, "iReserved", "source reserved field"),
                    ],
                )
                dir_entry = _structure(
                    dtm,
                    category,
                    "DIRENTRY",
                    0x118,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "sFileName", "source path"),
                        (0x100, dword, "uiOffset", "source payload offset"),
                        (0x104, dword, "uiLength", "source payload length"),
                        (0x108, byte, "ubState", "source FILE_OK state"),
                        (0x109, byte, "ubReserved", "source reserved byte"),
                        (0x10C, qword, "sFileTime", "source Win32 FILETIME"),
                        (0x114, word, "usReserved2", "source reserved word"),
                    ],
                )
                file_header = _structure(
                    dtm,
                    category,
                    "FileHeaderStruct",
                    0x0C,
                    [
                        (0x00, PointerDataType(char, dtm), "pFileName", "source file name"),
                        (0x04, dword, "uiFileLength", "source file length"),
                        (0x08, dword, "uiFileOffset", "source archive offset"),
                    ],
                )
                file_open = _structure(
                    dtm,
                    category,
                    "FileOpenStruct",
                    0x10,
                    [
                        (0x00, dword, "uiFileID", "source one-based file ID"),
                        (0x04, dword, "uiFilePosInFile", "source logical position"),
                        (
                            0x08,
                            dword,
                            "uiActualPositionInLibrary",
                            "source archive position",
                        ),
                        (
                            0x0C,
                            PointerDataType(file_header, dtm),
                            "pFileHeader",
                            "source live entry",
                        ),
                    ],
                )
                library_header = _structure(
                    dtm,
                    category,
                    "LibraryHeaderStruct",
                    0x28,
                    [
                        (0x00, PointerDataType(char, dtm), "sLibraryPath", "source path"),
                        (0x04, generic_pointer, "hLibraryHandle", "source Win32 handle"),
                        (0x08, word, "usNumberOfEntries", "source active entry count"),
                        (0x0A, boolean, "fLibraryOpen", "source open flag"),
                        (
                            0x0C,
                            dword,
                            "uiIdOfOtherFileAlreadyOpenedLibrary",
                            "source serialization field",
                        ),
                        (0x10, integer, "iNumFilesOpen", "source open count"),
                        (0x14, integer, "iSizeOfOpenFileArray", "source capacity"),
                        (
                            0x18,
                            PointerDataType(file_header, dtm),
                            "pFileHeader",
                            "source entries",
                        ),
                        (
                            0x1C,
                            PointerDataType(file_open, dtm),
                            "pOpenFiles",
                            "source open slots",
                        ),
                        (0x20, generic_pointer, "hFileMapping", "Wizardry extension"),
                        (0x24, generic_pointer, "pMappedFile", "Wizardry extension"),
                    ],
                )
                real_file_open = _structure(
                    dtm,
                    category,
                    "RealFileOpenStruct",
                    0x08,
                    [
                        (0x00, dword, "uiFileID", "source one-based file ID"),
                        (0x04, generic_pointer, "hRealFileHandle", "source Win32 handle"),
                    ],
                )
                real_file_header = _structure(
                    dtm,
                    category,
                    "RealFileHeaderStruct",
                    0x0C,
                    [
                        (0x00, integer, "iNumFilesOpen", "source open count"),
                        (0x04, integer, "iSizeOfOpenFileArray", "source capacity"),
                        (
                            0x08,
                            PointerDataType(real_file_open, dtm),
                            "pRealFilesOpen",
                            "source slots",
                        ),
                    ],
                )
                database_manager = _structure(
                    dtm,
                    category,
                    "DatabaseManagerHeaderStruct",
                    0x18,
                    [
                        (0x00, PointerDataType(char, dtm), "sManagerName", "source name"),
                        (
                            0x04,
                            PointerDataType(library_header, dtm),
                            "pLibraries",
                            "source archive states",
                        ),
                        (0x08, word, "usNumberOfLibraries", "source library count"),
                        (0x0A, boolean, "fInitialized", "source initialized flag"),
                        (0x0C, real_file_header, "RealFiles", "source physical file state"),
                    ],
                )

                file_open_flags = EnumDataType(category, "SGP_FILE_OPEN_FLAGS", 4, dtm)
                for name, value in (
                    ("FILE_ACCESS_READ", 0x01),
                    ("FILE_ACCESS_WRITE", 0x02),
                    ("FILE_ACCESS_READWRITE", 0x03),
                    ("FILE_CREATE_NEW", 0x10),
                    ("FILE_CREATE_ALWAYS", 0x20),
                    ("FILE_OPEN_EXISTING", 0x40),
                    ("FILE_OPEN_ALWAYS", 0x80),
                    ("FILE_TRUNCATE_EXISTING", 0x100),
                ):
                    file_open_flags.add(name, value)
                file_open_flags = dtm.addDataType(
                    file_open_flags, DataTypeConflictHandler.REPLACE_HANDLER
                )

                file_seek_origin = EnumDataType(category, "SGP_FILE_SEEK_ORIGIN", 1, dtm)
                file_seek_origin.add("FILE_SEEK_FROM_START", 0x01)
                file_seek_origin.add("FILE_SEEK_FROM_END", 0x02)
                file_seek_origin.add("FILE_SEEK_FROM_CURRENT", 0x04)
                file_seek_origin = dtm.addDataType(
                    file_seek_origin, DataTypeConflictHandler.REPLACE_HANDLER
                )

                file_attributes = EnumDataType(category, "SGP_FILE_ATTRIBUTES", 4, dtm)
                for name, value in (
                    ("FILE_IS_READONLY", 0x001),
                    ("FILE_IS_DIRECTORY", 0x002),
                    ("FILE_IS_HIDDEN", 0x004),
                    ("FILE_IS_NORMAL", 0x008),
                    ("FILE_IS_ARCHIVE", 0x010),
                    ("FILE_IS_SYSTEM", 0x020),
                    ("FILE_IS_TEMPORARY", 0x040),
                    ("FILE_IS_COMPRESSED", 0x080),
                    ("FILE_IS_OFFLINE", 0x100),
                ):
                    file_attributes.add(name, value)
                file_attributes = dtm.addDataType(
                    file_attributes, DataTypeConflictHandler.REPLACE_HANDLER
                )

                input_atom = _structure(
                    dtm,
                    category,
                    "InputAtom",
                    0x10,
                    [
                        (0x00, dword, "uiTimeStamp", "event timestamp"),
                        (0x04, word, "usKeyState", "shift control and alt mask"),
                        (0x06, word, "usEvent", "input event mask"),
                        (0x08, dword, "usParam", "key or event parameter"),
                        (0x0C, dword, "uiParam", "packed mouse or event parameter"),
                    ],
                )
                dd_pointer = PointerDataType(direct_draw, dtm)
                surface1_pointer = PointerDataType(surface1, dtm)
                surface2_pointer = PointerDataType(surface2, dtm)
                palette_pointer = PointerDataType(palette, dtm)
                rect_pointer = PointerDataType(rect, dtm)
                desc_pointer = PointerDataType(surface_desc, dtm)
                color_key_pointer = PointerDataType(color_key, dtm)
                palette_entry_pointer = PointerDataType(palette_entry, dtm)
                blt_fx_pointer = PointerDataType(blt_fx, dtm)
                surface1_pointer_pointer = PointerDataType(surface1_pointer, dtm)
                surface2_pointer_pointer = PointerDataType(surface2_pointer, dtm)
                palette_pointer_pointer = PointerDataType(palette_pointer, dtm)
                iunknown_pointer = PointerDataType(iunknown, dtm)
                char_pointer = PointerDataType(char, dtm)
                byte_pointer = PointerDataType(byte, dtm)
                char_pointer_pointer = PointerDataType(char_pointer, dtm)
                dword_pointer = PointerDataType(dword, dtm)
                get_file_pointer = PointerDataType(get_file, dtm)
                win32_find_data_pointer = PointerDataType(win32_find_data, dtm)
                short_pointer = PointerDataType(short, dtm)
                dir_entry_pointer_pointer = PointerDataType(PointerDataType(dir_entry, dtm), dtm)
                input_atom_pointer = PointerDataType(input_atom, dtm)

                signatures: dict[int, list[tuple[str, Any]]] = {
                    0x0040F0B0: [
                        ("pExistingDirectDraw", dd_pointer),
                        ("pNewSurfaceDesc", desc_pointer),
                        ("ppNewSurface1", surface1_pointer_pointer),
                        ("ppNewSurface2", surface2_pointer_pointer),
                    ],
                    0x0040F100: [
                        ("pSurface", surface2_pointer),
                        ("pDestRect", rect_pointer),
                        ("pSurfaceDesc", desc_pointer),
                        ("uiFlags", dword),
                        ("hEvent", generic_pointer),
                    ],
                    0x0040F150: [("pSurface", surface2_pointer), ("pSurfaceData", generic_pointer)],
                    0x0040F180: [
                        ("pSurface", surface2_pointer),
                        ("pSurfaceDesc", desc_pointer),
                    ],
                    0x0040F1C0: [
                        ("ppOldSurface1", surface1_pointer_pointer),
                        ("ppOldSurface2", surface2_pointer_pointer),
                    ],
                    0x0040F210: [("pSurface", surface2_pointer)],
                    0x0040F230: [
                        ("pDestSurface", surface2_pointer),
                        ("uiX", dword),
                        ("uiY", dword),
                        ("pSrcSurface", surface2_pointer),
                        ("pSrcRect", rect_pointer),
                        ("uiTrans", dword),
                    ],
                    0x0040F290: [
                        ("pDestSurface", surface2_pointer),
                        ("pDestRect", rect_pointer),
                        ("pSrcSurface", surface2_pointer),
                        ("pSrcRect", rect_pointer),
                        ("uiFlags", dword),
                        ("pDDBltFx", blt_fx_pointer),
                    ],
                    0x0040F300: [
                        ("pDirectDraw", dd_pointer),
                        ("uiFlags", dword),
                        ("pColorTable", palette_entry_pointer),
                        ("ppDDPalette", palette_pointer_pointer),
                        ("pUnkOuter", iunknown_pointer),
                    ],
                    0x0040F340: [
                        ("pPalette", palette_pointer),
                        ("uiFlags", dword),
                        ("uiStartingEntry", dword),
                        ("uiCount", dword),
                        ("pEntries", palette_entry_pointer),
                    ],
                    0x0040F380: [
                        ("pPalette", palette_pointer),
                        ("uiFlags", dword),
                        ("uiBase", dword),
                        ("uiNumEntries", dword),
                        ("pEntries", palette_entry_pointer),
                    ],
                    0x0040F3C0: [("pPalette", palette_pointer)],
                    0x0040F3E0: [
                        ("pSurface", surface2_pointer),
                        ("uiFlags", dword),
                        ("pDDColorKey", color_key_pointer),
                    ],
                }

                address_space = program.getAddressFactory().getDefaultAddressSpace()
                applied: list[str] = []
                for raw_address, arguments in signatures.items():
                    address = address_space.getAddress(raw_address)
                    function = program.getFunctionManager().getFunctionAt(address)
                    if function is None:
                        raise RuntimeError(f"no function at 0x{raw_address:08x}")
                    signature = _function_type(
                        dtm,
                        category,
                        f"signature_{function.getName()}",
                        void,
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
                    applied.append(f"0x{raw_address:08x}")

                string_signature = _function_type(
                    dtm,
                    category,
                    "signature_String",
                    byte_pointer,
                    [("format", char_pointer)],
                    "__cdecl",
                )
                string_signature.setVarArgs(True)
                string_address = address_space.getAddress(0x00404B50)
                if program.getFunctionManager().getFunctionAt(string_address) is None:
                    raise RuntimeError("no function at 0x00404b50")
                string_command = ApplyFunctionSignatureCmd(
                    string_address,
                    string_signature,
                    SourceType.USER_DEFINED,
                    True,
                    FunctionRenameOption.NO_CHANGE,
                )
                if not string_command.applyTo(program):
                    raise RuntimeError(
                        f"failed to apply signature at 0x00404b50: {string_command.getStatusMsg()}"
                    )
                applied.append("0x00404b50")

                file_signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x00404BF0: (boolean, [("strFilename", char_pointer)]),
                    0x00404C40: (boolean, [("strFilename", char_pointer)]),
                    0x00404C80: (
                        hwfile,
                        [
                            ("strFilename", char_pointer),
                            ("uiOptions", file_open_flags),
                            ("fDeleteOnClose", boolean),
                        ],
                    ),
                    0x00404FB0: (
                        boolean,
                        [
                            ("hFile", hwfile),
                            ("pDest", generic_pointer),
                            ("uiBytesToWrite", dword),
                            ("puiBytesWritten", dword_pointer),
                        ],
                    ),
                    0x00405030: (
                        boolean,
                        [
                            ("hFile", hwfile),
                            ("uiDistance", dword),
                            ("uiHow", file_seek_origin),
                        ],
                    ),
                    0x004051D0: (boolean, [("pcDirectory", char_pointer)]),
                    0x004051F0: (boolean, [("pcDirectory", char_pointer)]),
                    0x00405200: (boolean, [("pcDirectory", char_pointer)]),
                    0x00405270: (
                        boolean,
                        [("pSpec", char_pointer), ("pGFStruct", get_file_pointer)],
                    ),
                    0x00405300: (boolean, [("pGFStruct", get_file_pointer)]),
                    0x00405350: (void, [("pGFStruct", get_file_pointer)]),
                    0x00405390: (
                        void,
                        [
                            ("pGFStruct", get_file_pointer),
                            ("pW32Struct", win32_find_data_pointer),
                        ],
                    ),
                    0x004054D0: (
                        boolean,
                        [
                            ("strSrcFile", char_pointer),
                            ("strDstFile", char_pointer),
                            ("fFailIfExists", boolean),
                        ],
                    ),
                    0x004054F0: (file_attributes, [("strFilename", char_pointer)]),
                    0x00405550: (boolean, [("strFilename", char_pointer)]),
                }
                for raw_address, (return_type, arguments) in file_signatures.items():
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
                    applied.append(f"0x{raw_address:08x}")

                library_signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x00412B10: (boolean, []),
                    0x00413680: (hwfile, [("hFile", generic_pointer)]),
                    0x00413730: (
                        boolean,
                        [
                            ("hlibFile", hwfile),
                            ("pLibraryID", short_pointer),
                            ("pFileNum", dword_pointer),
                        ],
                    ),
                    0x00413D00: (
                        integer,
                        [("arg1", char_pointer_pointer), ("arg2", dir_entry_pointer_pointer)],
                    ),
                }
                for raw_address, (return_type, arguments) in library_signatures.items():
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
                    applied.append(f"0x{raw_address:08x}")

                hstack = container_handles["HSTACK"]
                hlist = container_handles["HLIST"]
                container_signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x00405970: (
                        hstack,
                        [("uiNum_items", dword), ("uiSiz_each", dword)],
                    ),
                    0x004059B0: (
                        hlist,
                        [("uiNum_items", dword), ("uiSiz_each", dword)],
                    ),
                    0x00405A00: (
                        hstack,
                        [("hStack", hstack), ("pdata", generic_pointer)],
                    ),
                    0x00405A70: (
                        boolean,
                        [("hStack", hstack), ("pdata", generic_pointer)],
                    ),
                    0x00405AC0: (
                        boolean,
                        [("hStack", hstack), ("pdata", generic_pointer)],
                    ),
                    0x00405B00: (boolean, [("hContainer", hcontainer)]),
                    0x00405B20: (
                        boolean,
                        [
                            ("hList", hlist),
                            ("pdata", generic_pointer),
                            ("uiPos", dword),
                        ],
                    ),
                    0x00405B90: (
                        boolean,
                        [
                            ("hList", hlist),
                            ("pdata", generic_pointer),
                            ("uiPos", dword),
                        ],
                    ),
                    0x00405C00: (dword, [("hContainer", hcontainer)]),
                    0x00405C10: (
                        hlist,
                        [
                            ("hList", hlist),
                            ("pdata", generic_pointer),
                            ("uiPos", dword),
                        ],
                    ),
                }
                for raw_address, (return_type, arguments) in container_signatures.items():
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
                    applied.append(f"0x{raw_address:08x}")

                startup_signatures: dict[int, tuple[Any, list[tuple[str, Any]], str]] = {
                    0x004018C0: (void, [], "__cdecl"),
                    0x00401950: (void, [("pCommandLine", char_pointer)], "__cdecl"),
                    0x00401B30: (
                        integer,
                        [("Code", integer), ("wParam", dword), ("lParam", integer)],
                        "__stdcall",
                    ),
                    0x00401C70: (
                        integer,
                        [("Code", integer), ("wParam", dword), ("lParam", integer)],
                        "__stdcall",
                    ),
                    0x00401EA0: (boolean, [], "__cdecl"),
                    0x00401F70: (void, [], "__cdecl"),
                    0x00401F90: (
                        void,
                        [("ubInputEvent", word), ("usParam", dword), ("uiParam", dword)],
                        "__cdecl",
                    ),
                    0x00402140: (boolean, [("Event", input_atom_pointer)], "__cdecl"),
                    0x00402750: (void, [], "__cdecl"),
                    0x00402760: (short, [("wParam", dword)], "__cdecl"),
                    0x00406B70: (
                        void,
                        [
                            ("hWindow", generic_pointer),
                            ("uMessage", dword),
                            ("idEvent", dword),
                            ("dwTime", dword),
                        ],
                        "__stdcall",
                    ),
                    0x00406BA0: (boolean, [], "__cdecl"),
                    0x00406BD0: (void, [], "__cdecl"),
                    0x00406BE0: (dword, [], "__cdecl"),
                    0x00406BF0: (dword, [("uiTimeToElapse", dword)], "__cdecl"),
                    0x00406C00: (dword, [("uiTimer", dword)], "__cdecl"),
                }
                for raw_address, (return_type, arguments, convention) in startup_signatures.items():
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
                        convention,
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
                    applied.append(f"0x{raw_address:08x}")

                database_address = address_space.getAddress(0x006EB720)
                _apply_data(program, database_address, database_manager)
                symbol_table = program.getSymbolTable()
                database_symbol = symbol_table.getPrimarySymbol(database_address)
                if database_symbol is None:
                    symbol_table.createLabel(
                        database_address, "gFileDataBase", SourceType.USER_DEFINED
                    )
                elif database_symbol.getName() != "gFileDataBase":
                    database_symbol.setName("gFileDataBase", SourceType.USER_DEFINED)

                debug_globals = {
                    0x00650DEC: (byte, "gubStringIndex"),
                    0x006EE440: (
                        ArrayDataType(ArrayDataType(byte, 512, 1), 8, 512),
                        "gbTmpDebugString",
                    ),
                }
                for raw_address, (data_type, name) in debug_globals.items():
                    address = address_space.getAddress(raw_address)
                    _apply_data(program, address, data_type)
                    symbol = symbol_table.getPrimarySymbol(address)
                    if symbol is None:
                        symbol_table.createLabel(address, name, SourceType.USER_DEFINED)
                    elif symbol.getName() != name:
                        symbol.setName(name, SourceType.USER_DEFINED)

                startup_globals = {
                    0x005FF450: (byte, "gbPixelDepth"),
                    0x006505A0: (boolean, "gfLoadAtStartup"),
                    0x006505A1: (boolean, "gfUsingBoundsChecker"),
                    0x006505A4: (char_pointer, "gzStringDataOverride"),
                    0x006505A8: (boolean, "gfCapturingVideo"),
                    0x00650DB8: (boolean, "fCursorWasClipped"),
                    0x00650DB9: (boolean, "gfSGPInputReceived"),
                    0x006596CC: (generic_pointer, "ghWindow"),
                    0x006EB708: (dword, "guiStartupTime"),
                    0x006EB70C: (dword, "guiCurrentTime"),
                    0x006EF4E0: (ArrayDataType(input_atom, 256, 0x10), "gEventQueue"),
                    0x006F04E0: (word, "gusTailIndex"),
                    0x006F04E2: (word, "gusHeadIndex"),
                    0x006F04E4: (dword, "guiDoubleClkDelay"),
                    0x006F04E8: (boolean, "gfRightButtonState"),
                    0x006F04E9: (boolean, "gfRecordedLeftButtonUp"),
                    0x006F04EA: (word, "gfShiftState"),
                    0x006F04EC: (boolean, "gfTrackMousePos"),
                    0x006F04ED: (boolean, "gfLeftButtonState"),
                    0x006F04F0: (dword, "guiRightButtonRepeatTimer"),
                    0x006F04F4: (boolean, "gfTrackDblClick"),
                    0x006F04F6: (word, "gusQueueCount"),
                    0x006F04F8: (word, "gusMouseYPos"),
                    0x006F04FC: (generic_pointer, "ghKeyboardHook"),
                    0x006F0500: (boolean, "gfCurrentStringInputState"),
                    0x006F0504: (dword, "guiSingleClickTimer"),
                    0x006F0508: (word, "gfCtrlState"),
                    0x006F050A: (word, "gusMouseXPos"),
                    0x006F050C: (generic_pointer, "ghMouseHook"),
                    0x006F0510: (generic_pointer, "gpCurrentStringDescriptor"),
                    0x006F0514: (word, "gusRecordedKeyState"),
                    0x006F0518: (dword, "guiLeftButtonRepeatTimer"),
                    0x006F051C: (word, "gfAltState"),
                    0x006F0520: (ArrayDataType(boolean, 256, 1), "gfKeyState"),
                    0x006F0630: (boolean, "gfApplicationActive"),
                }
                for raw_address, (data_type, name) in startup_globals.items():
                    address = address_space.getAddress(raw_address)
                    _apply_data(program, address, data_type)
                    symbol = symbol_table.getPrimarySymbol(address)
                    if symbol is None:
                        symbol_table.createLabel(address, name, SourceType.USER_DEFINED)
                    elif symbol.getName() != name:
                        symbol.setName(name, SourceType.USER_DEFINED)

                commit = True
                result.update(
                    {
                        "structures": [
                            str(data_type.getPathName())
                            for data_type in (
                                rect,
                                color_key,
                                pixel_format,
                                caps,
                                surface_desc,
                                palette_entry,
                                blt_fx,
                                sgp_filetime,
                                get_file,
                                win32_find_data,
                                lib_header,
                                dir_entry,
                                file_header,
                                file_open,
                                library_header,
                                real_file_open,
                                real_file_header,
                                database_manager,
                                stack_header,
                                queue_header,
                                list_header,
                                ord_list_header,
                                input_atom,
                            )
                        ],
                        "typed_functions": applied,
                        "typed_globals": [
                            "0x00650dec",
                            "0x006eb720",
                            "0x006ee440",
                            *[f"0x{address:08x}" for address in startup_globals],
                        ],
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply source-backed SGP model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
