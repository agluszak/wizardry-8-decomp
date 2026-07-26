from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _function_type
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/signature_fixes"

# Each entry documents the wrong signature Ghidra's own auto-analysis produced
# at this address, and the evidence for the corrected one. Both were found
# while porting WIZ8_GAMEPLAY_BOUNDARIES functions that call these addresses:
# Ghidra's decompile of the callee itself disagreed with its own decompile of
# the call sites, and the raw disassembly's ESP-relative reads (checked
# against the actual push count at each read) settled which one was right.
SIGNATURE_FIXES = {
    0x005E2890: {
        "name": "PListIndexOf",
        "return_type": "int",
        "arguments": [("list", "void *"), ("target", "void *")],
        "wrong": "undefined4 FUN_005e2890(undefined4 *param_1, undefined4 param_2, int param_3)",
        "evidence": (
            "Every call site (e.g. FindNextExistingMonsterByID at 0x00510BF0) passes exactly "
            "two arguments. The standalone three-parameter decompile is an artifact of this "
            "function's own srAssertFail prologue, whose two inline-pushed assertion constants "
            "Ghidra's parameter recovery mistook for a third caller-supplied argument."
        ),
    },
    0x005222D0: {
        "name": "GetOriginOfCharacterItem",
        "return_type": "void",
        "arguments": [
            ("character_index", "int"),
            ("item", "void *"),
            ("origin", "unsigned char *"),
            ("slot", "unsigned short *"),
        ],
        "wrong": (
            "void FUN_005222d0(undefined4 param_1, undefined4 *param_2, undefined4 param_3, "
            "undefined1 *param_4, undefined2 *param_5)"
        ),
        "evidence": (
            "Ghidra reports five parameters and multiplies the *second* one (the item pointer, "
            "compared by identity a few lines later) by 0x1862 -- impossible for a pointer. "
            "Walking the raw disassembly's ESP-relative reads against the actual push count at "
            "each point shows four parameters, with the *first* (character_index) driving the "
            "0x1862 (W8Character stride) strength-reduction multiply and the second (item) used "
            "only for comparisons; the embedded 'pPCItem != NULL' assertion text and "
            "'PC Item.cpp' source path independently confirm this reading."
        ),
    },
}


def _type_for(dtm: Any, spec: str) -> Any:
    from ghidra.program.model.data import (
        CharDataType,
        IntegerDataType,
        PointerDataType,
        ShortDataType,
        UnsignedCharDataType,
        UnsignedShortDataType,
        VoidDataType,
    )

    base_types = {
        "void": VoidDataType.dataType,
        "int": IntegerDataType.dataType,
        "char": CharDataType.dataType,
        "short": ShortDataType.dataType,
        "unsigned char": UnsignedCharDataType.dataType,
        "unsigned short": UnsignedShortDataType.dataType,
    }
    spec = spec.strip()
    if spec.endswith("*"):
        return PointerDataType(_type_for(dtm, spec[:-1]), dtm)
    return base_types[spec]


def apply_wiz8_signature_fixes(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Correct known-wrong Ghidra auto-analysis signatures found while porting owned functions."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import CategoryPath
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply wiz8 signature fixes")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                applied: list[dict[str, str]] = []
                for raw_address, fix in SIGNATURE_FIXES.items():
                    address = address_space.getAddress(raw_address)
                    function = program.getFunctionManager().getFunctionAt(address)
                    if function is None:
                        raise RuntimeError(f"no function at 0x{raw_address:08x}")
                    before = function.getPrototypeString(False, False)
                    signature = _function_type(
                        dtm,
                        CategoryPath(CATEGORY),
                        f"signature_{fix['name']}",
                        _type_for(dtm, fix["return_type"]),
                        [
                            (argument_name, _type_for(dtm, type_spec))
                            for argument_name, type_spec in fix["arguments"]
                        ],
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
                    function.setName(fix["name"], SourceType.USER_DEFINED)
                    applied.append(
                        {
                            "address": f"0x{raw_address:08x}",
                            "before": before,
                            "after": function.getPrototypeString(False, False),
                            "evidence": fix["evidence"],
                        }
                    )

                commit = True
                result["applied"] = applied
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply wiz8 signature fixes", pyghidra.task_monitor())
    finally:
        project.close()
    return result
