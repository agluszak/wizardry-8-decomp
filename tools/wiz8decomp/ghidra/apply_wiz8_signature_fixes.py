from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _function_type
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon
from .reviewed_signatures import load_reviewed_signatures


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
    try:
        return base_types[spec]
    except KeyError as error:
        raise ValueError(f"unsupported reviewed signature type: {spec}") from error


def apply_reviewed_signatures(
    settings: Settings,
    selector: str,
    *,
    evidence_program: str,
) -> dict[str, Any]:
    """Materialize canonical reviewed function signatures in Ghidra."""

    signatures = load_reviewed_signatures(settings.repo_dir, evidence_program)
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import CategoryPath
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name, "evidence_program": evidence_program}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply canonical reviewed signatures")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                applied: list[dict[str, str]] = []
                for reviewed in signatures:
                    address = address_space.getAddress(reviewed.address)
                    function = program.getFunctionManager().getFunctionAt(address)
                    if function is None:
                        raise RuntimeError(f"no function at 0x{reviewed.address:08x}")
                    before = function.getPrototypeString(False, False)
                    signature = _function_type(
                        dtm,
                        CategoryPath(f"/{evidence_program}/signatures"),
                        f"signature_{reviewed.address:08x}",
                        _type_for(dtm, reviewed.return_type),
                        [
                            (argument_name, _type_for(dtm, type_spec))
                            for argument_name, type_spec in reviewed.parameters
                        ],
                        reviewed.calling_convention,
                    )
                    signature.setVarArgs(reviewed.variadic)
                    command = ApplyFunctionSignatureCmd(
                        address,
                        signature,
                        SourceType.USER_DEFINED,
                        True,
                        FunctionRenameOption.NO_CHANGE,
                    )
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to apply signature at 0x{reviewed.address:08x}: "
                            f"{command.getStatusMsg()}"
                        )
                    applied.append(
                        {
                            "address": f"0x{reviewed.address:08x}",
                            "before": before,
                            "after": function.getPrototypeString(False, False),
                            "evidence_id": reviewed.evidence_id,
                        }
                    )

                commit = True
                result["applied"] = applied
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply canonical reviewed signatures", pyghidra.task_monitor())
    finally:
        project.close()
    return result


def apply_wiz8_signature_fixes(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    return apply_reviewed_signatures(settings, selector, evidence_program="wiz8")
