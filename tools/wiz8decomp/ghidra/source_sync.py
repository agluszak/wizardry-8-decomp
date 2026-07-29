"""Synchronize source-owned function identities and signatures with Ghidra."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from ..config import Settings
from ..source_model import SourceModel, build_source_model
from .environment import start_pyghidra
from .workspace import ensure_seed


def _data_type_name(data_type: Any) -> str:
    path = data_type.getDataTypePath()
    return str(path) if path is not None else data_type.getDisplayName()


def _signature_key(signature: Any, *, function: bool) -> tuple[Any, ...]:
    arguments = signature.getParameters() if function else signature.getArguments()
    if function:
        arguments = [argument for argument in arguments if not argument.isAutoParameter()]
    return (
        signature.getCallingConventionName(),
        _data_type_name(signature.getReturnType()),
        bool(signature.hasVarArgs()),
        tuple(
            (
                str(argument.getName()) or f"param_{index + 1}",
                _data_type_name(argument.getDataType()),
            )
            for index, argument in enumerate(arguments)
        ),
    )


def _signature_matches(function: Any, definition: Any, calling_convention: str) -> bool:
    if _signature_key(function, function=True) != _signature_key(definition, function=False):
        return False
    return calling_convention != "__cdecl" or function.getStackPurgeSize() == 0


def _data_type_query_service(data_types: Any) -> Any:
    """Resolve a parser type only when the Program DTM has one unique match."""

    import jpype
    from ghidra.app.services import DataTypeQueryService
    from java.util import ArrayList

    def matches(name: Any) -> Any:
        result = ArrayList()
        data_types.findDataTypes(str(name), result)
        return result

    def unique(name: Any) -> Any:
        result = matches(name)
        return result.get(0) if result.size() == 1 else None

    def by_path(path: Any) -> Any:
        result = ArrayList()
        data_type = data_types.getDataType(path)
        if data_type is not None:
            result.add(data_type)
        return result

    return jpype.JProxy(
        DataTypeQueryService,
        dict={
            "getSortedDataTypeList": lambda: ArrayList(),
            "getSortedCategoryPathList": lambda: ArrayList(),
            "getDataType": unique,
            "promptForDataType": unique,
            "findDataTypes": lambda name, _monitor: matches(name),
            "getDataTypesByPath": by_path,
            "getProgramDataTypeByPath": data_types.getDataType,
        },
    )


def synchronize_source_program(
    program: Any, model: SourceModel, *, apply: bool = False
) -> dict[str, Any]:
    """Audit or apply source state to one already-open Ghidra Program."""

    import pyghidra
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import (
        ApplyFunctionSignatureCmd,
        CreateFunctionCmd,
        FunctionRenameOption,
    )
    from ghidra.app.util import NamespaceUtils
    from ghidra.app.util.parser import FunctionSignatureParser
    from ghidra.program.model.symbol import SourceType, SymbolType

    functions = program.getFunctionManager()
    symbols = program.getSymbolTable()
    address_space = program.getAddressFactory().getDefaultAddressSpace()
    listing = program.getListing()
    global_namespace = program.getGlobalNamespace()
    monitor = pyghidra.task_monitor()
    parser = FunctionSignatureParser(
        program.getDataTypeManager(), _data_type_query_service(program.getDataTypeManager())
    )

    name_current = 0
    name_changed = 0
    signature_current = 0
    signature_changed = 0
    signature_unavailable = 0
    unsupported: list[dict[str, str]] = []
    unsupported_addresses: set[int] = set()
    missing: list[str] = []
    missing_addresses: set[int] = set()
    changed_addresses: set[int] = set()
    external_current = 0
    external_changed: set[str] = set()
    external_unsupported: set[str] = set()
    external_missing: list[str] = []

    def namespace(parts: list[str]) -> Any:
        parent = global_namespace
        for index, part in enumerate(parts):
            existing = symbols.getNamespace(part, parent)
            final = index == len(parts) - 1
            if existing is not None:
                if final and existing.getSymbol().getSymbolType() != SymbolType.CLASS:
                    existing = NamespaceUtils.convertNamespaceToClass(existing)
                parent = existing
            elif final:
                parent = symbols.createClass(parent, part, SourceType.USER_DEFINED)
            else:
                parent = symbols.createNameSpace(parent, part, SourceType.USER_DEFINED)
        return parent

    for address, source in model.functions.items():
        target = address_space.getAddress(address)
        item = functions.getFunctionAt(target)
        if item is None:
            if not apply:
                missing.append(f"{address:08x}")
                missing_addresses.add(address)
                continue
            if listing.getInstructionAt(target) is None:
                DisassembleCommand(target, None, True).applyTo(program, monitor)
            command = CreateFunctionCmd(target)
            if command.applyTo(program, monitor):
                item = functions.getFunctionAt(target)
            if item is None:
                missing.append(f"{address:08x}")
                missing_addresses.add(address)
                continue

        parts = source.name.split("::")
        namespace_drift = (
            len(parts) > 1
            and item.getParentNamespace().getSymbol().getSymbolType() != SymbolType.CLASS
        )
        if item.getName(True) == source.name and not namespace_drift:
            name_current += 1
        else:
            name_changed += 1
            changed_addresses.add(address)
            if apply:
                item.setParentNamespace(namespace(parts[:-1]))
                if item.getName() != parts[-1]:
                    item.setName(parts[-1], SourceType.USER_DEFINED)

        if not source.ghidra_prototype:
            signature_unavailable += 1
            continue
        try:
            definition = parser.parse(item.getSignature(), source.ghidra_prototype)
            definition.setCallingConvention(source.calling_convention)
        except Exception as error:  # noqa: BLE001 - Java parser exceptions vary by declaration
            unsupported_addresses.add(address)
            unsupported.append(
                {
                    "address": f"{address:08x}",
                    "name": source.name,
                    "prototype": source.prototype,
                    "error": str(error).splitlines()[0],
                }
            )
            continue

        if not namespace_drift and _signature_matches(item, definition, source.calling_convention):
            signature_current += 1
            continue
        signature_changed += 1
        changed_addresses.add(address)
        if apply:
            command = ApplyFunctionSignatureCmd(
                target,
                definition,
                SourceType.USER_DEFINED,
                False,
                FunctionRenameOption.NO_CHANGE,
            )
            if not command.applyTo(program, monitor):
                raise ValueError(
                    f"failed to apply source signature at 0x{address:08x}: "
                    f"{command.getStatusMsg() or source.prototype}"
                )
            if source.calling_convention == "__cdecl":
                item.setStackPurgeSize(0)
            if not _signature_matches(item, definition, source.calling_convention):
                raise ValueError(f"Ghidra did not retain the source signature at 0x{address:08x}")

    external_functions: dict[str, list[Any]] = {}
    for item in functions.getExternalFunctions():
        external_functions.setdefault(str(item.getName()), []).append(item)
    for name, source in model.externals.items():
        candidates = external_functions.get(name, [])
        if len(candidates) != 1:
            external_missing.append(name)
            continue
        item = candidates[0]
        try:
            definition = parser.parse(item.getSignature(), source.ghidra_prototype)
            definition.setCallingConvention(source.calling_convention)
        except Exception as error:  # noqa: BLE001 - Java parser exceptions vary by declaration
            external_unsupported.add(name)
            unsupported.append(
                {
                    "external": name,
                    "name": name,
                    "prototype": source.prototype,
                    "error": str(error).splitlines()[0],
                }
            )
            continue
        if _signature_matches(item, definition, source.calling_convention):
            external_current += 1
            continue
        external_changed.add(name)
        if apply:
            command = ApplyFunctionSignatureCmd(
                item.getEntryPoint(),
                definition,
                SourceType.USER_DEFINED,
                False,
                FunctionRenameOption.NO_CHANGE,
            )
            if not command.applyTo(program, monitor):
                raise ValueError(
                    f"failed to apply source signature for external {name}: "
                    f"{command.getStatusMsg() or source.prototype}"
                )
            if source.calling_convention == "__cdecl":
                item.setStackPurgeSize(0)
            if not _signature_matches(item, definition, source.calling_convention):
                raise ValueError(f"Ghidra did not retain the source signature for external {name}")

    return {
        "source_functions": len(model.functions),
        "source_externals": len(model.externals),
        "already_current": len(model.functions)
        + len(model.externals)
        - len(changed_addresses | missing_addresses | unsupported_addresses)
        - len(external_changed | external_unsupported | set(external_missing)),
        "changed": len(changed_addresses) + len(external_changed),
        "missing": missing,
        "external_missing": external_missing,
        "names": {"current": name_current, "changed": name_changed},
        "signatures": {
            "current": signature_current,
            "changed": signature_changed,
            "unavailable": signature_unavailable,
            "unsupported": len(unsupported_addresses),
        },
        "external_signatures": {
            "current": external_current,
            "changed": len(external_changed),
            "unsupported": len(external_unsupported),
        },
        "unsupported": unsupported,
    }


def require_source_synchronized(report: dict[str, Any]) -> None:
    if not report["changed"] and not report["missing"] and not report["external_missing"]:
        return
    details = []
    if report["changed"]:
        details.append(f"{report['changed']} changed")
    if report["missing"]:
        details.append(f"{len(report['missing'])} missing")
    if report["external_missing"]:
        details.append(f"{len(report['external_missing'])} external missing")
    raise ValueError(
        "Ghidra source synchronization is pending ("
        + ", ".join(details)
        + "); run `uv run wiz8 ghidra sync-source --apply`"
    )


def audit_source_program(program: Any, repository: Path) -> dict[str, Any]:
    report = synchronize_source_program(program, build_source_model(repository), apply=False)
    require_source_synchronized(report)
    report["unsupported_count"] = len(report.pop("unsupported"))
    return report


def sync_source_names(
    settings: Settings, selector: str = "wiz8", *, apply: bool = False
) -> dict[str, Any]:
    """Report or apply source-owned names and resolvable signatures."""

    program_name = ensure_seed(settings, selector)
    model = build_source_model(settings.repo_dir)
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    transaction = None
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            try:
                if apply:
                    transaction = program.startTransaction("synchronize source functions")
                reports = []
                for _pass in range(4 if apply else 1):
                    report = synchronize_source_program(program, model, apply=apply)
                    reports.append(report)
                    if not apply or not report["changed"]:
                        break
                else:
                    raise ValueError("source signature synchronization did not stabilize")
                if (report["missing"] or report["external_missing"]) and apply:
                    raise ValueError(
                        "source declarations do not resolve to Ghidra functions: "
                        + ", ".join(report["missing"] + report["external_missing"])
                    )
                if apply:
                    assert transaction is not None
                    program.endTransaction(transaction, True)
                    transaction = None
                    program.save("synchronize source functions", pyghidra.task_monitor())
            except Exception:
                if transaction is not None:
                    program.endTransaction(transaction, False)
                    transaction = None
                raise
    finally:
        project.close()

    return {
        "schema": "wiz8.ghidra-source-sync",
        "program": program_name,
        "mode": "apply" if apply else "dry-run",
        "passes": len(reports),
        "applied": sum(int(item["changed"]) for item in reports[:-1]) if apply else 0,
        **report,
    }
