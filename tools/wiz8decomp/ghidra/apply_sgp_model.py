from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _function_type, _structure
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/sgp"


def apply_sgp_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Install the source-backed DirectDraw wrapper types and prototypes."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        DWordDataType,
        IntegerDataType,
        PointerDataType,
        VoidDataType,
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
                dword = DWordDataType.dataType
                integer = IntegerDataType.dataType
                void = VoidDataType.dataType
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
                            )
                        ],
                        "typed_functions": applied,
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply source-backed SGP model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
