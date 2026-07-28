from __future__ import annotations

from typing import Any

from ..config import Settings
from . import type_specs
from .apply_unzip_model import _function_type
from .project import resolve_program_name
from .reviewed_signatures import load_reviewed_signatures


def type_category_paths(type_name: str) -> tuple[str, ...]:
    """Compatibility surface for callers that validate reviewed type paths."""

    return type_specs.type_category_paths(type_name)[:5]


def _type_for(dtm: Any, spec: str, evidence_program: str) -> Any:
    return type_specs.resolve_type_spec(dtm, spec, evidence_program)


def apply_reviewed_signatures(
    settings: Settings,
    selector: str,
    *,
    evidence_program: str,
    materialize: bool = True,
) -> dict[str, Any]:
    """Materialize canonical reviewed function signatures in Ghidra."""

    signatures = load_reviewed_signatures(settings.repo_dir, evidence_program)
    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
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
                    if reviewed.this_type is not None:
                        from ghidra.program.model.listing import GhidraClass

                        namespace = function.getParentNamespace()
                        if not isinstance(namespace, GhidraClass):
                            program.getSymbolTable().convertNamespaceToClass(namespace)
                    before = function.getPrototypeString(False, False)
                    parameters = [
                        (
                            argument_name,
                            _type_for(dtm, type_spec, evidence_program),
                        )
                        for argument_name, type_spec in reviewed.parameters
                    ]
                    signature = _function_type(
                        dtm,
                        CategoryPath(f"/{evidence_program}/signatures"),
                        f"signature_{reviewed.address:08x}",
                        _type_for(dtm, reviewed.return_type, evidence_program),
                        parameters,
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
                    if reviewed.this_type is not None:
                        # Ghidra derives an auto-parameter's datatype from a
                        # GhidraClass namespace and will not let callers edit it.
                        # Canonical class layouts live in our reviewed datatype
                        # category instead, so preserve the ABI-selected storage
                        # as an explicit custom parameter before applying the
                        # reviewed this type.
                        function.setCustomVariableStorage(True)
                        function_parameters = function.getParameters()
                        if (
                            not function_parameters
                            or function_parameters[0].getName() != "this"
                        ):
                            raise RuntimeError(
                                f"__thiscall signature at 0x{reviewed.address:08x} "
                                "did not materialize a this parameter"
                            )
                        function_parameters[0].setDataType(
                            _type_for(dtm, reviewed.this_type, evidence_program),
                            SourceType.USER_DEFINED,
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
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    return apply_reviewed_signatures(
        settings, selector, evidence_program="wiz8", materialize=materialize
    )
