from __future__ import annotations

from pathlib import Path
from typing import Any

from ..config import Settings
from ..evidence.classes import ghidra_namespace_name
from ..evidence.functions import load_function_identities
from ..provenance import format_name_origin
from .project import resolve_program_name


def _namespace_and_name(symbol_table: Any, program: Any, qualified_name: str) -> tuple[Any, str]:
    # A template instantiation over a pointer carries a space -- the demangled
    # `W8GrowableVector<W8WorldItem *>` is the spelling that joins a reviewed row
    # to its COFF symbol -- and Ghidra refuses whitespace in a symbol name. The
    # replay adapts; the reviewed name does not change.
    parts = [ghidra_namespace_name(part) for part in qualified_name.split("::")]
    parent = program.getGlobalNamespace()
    for part in parts[:-1]:
        namespace = symbol_table.getNamespace(part, parent)
        if namespace is None:
            from ghidra.program.model.symbol import SourceType

            namespace = symbol_table.createNameSpace(parent, part, SourceType.USER_DEFINED)
        parent = namespace
    return parent, parts[-1]


def apply_function_map(
    settings: Settings,
    selector: str,
    mapping_path: Path,
    *,
    dry_run: bool = False,
    materialize: bool = True,
) -> dict[str, Any]:
    mapping_path = mapping_path.resolve()
    if not mapping_path.is_file():
        raise ValueError(f"function map does not exist: {mapping_path}")
    identities = load_function_identities(mapping_path)
    if not identities:
        raise ValueError(f"function map has no accepted identities: {mapping_path}")

    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.address import AddressSet
    from ghidra.program.model.listing import CodeUnit
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    stats = {
        "created": 0,
        "boundaries_adjusted": 0,
        "renamed": 0,
        "already_applied": 0,
        "aliased": 0,
        "failed": 0,
    }
    failures: list[dict[str, str]] = []
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            function_manager = program.getFunctionManager()
            symbol_table = program.getSymbolTable()
            listing = program.getListing()
            address_factory = program.getAddressFactory().getDefaultAddressSpace()
            transaction = (
                None if dry_run else program.startTransaction("apply reviewed function identities")
            )
            commit = False
            try:
                for identity in identities:
                    address = address_factory.getAddress(identity.address)
                    if not program.getMemory().contains(address):
                        stats["failed"] += 1
                        failures.append(
                            {
                                "address": f"0x{identity.address:08x}",
                                "error": "outside program memory",
                            }
                        )
                        continue
                    function = function_manager.getFunctionAt(address)
                    instruction = listing.getInstructionAt(address)
                    if (
                        function is not None
                        and function.getBody().getNumAddresses() <= 1
                        and instruction is None
                        and not dry_run
                    ):
                        function_manager.removeFunction(address)
                        function = None
                    if function is None:
                        if dry_run:
                            stats["created"] += 1
                            continue
                        if listing.getInstructionAt(address) is None:
                            disassemble = DisassembleCommand(address, None, True)
                            if not disassemble.applyTo(program):
                                stats["failed"] += 1
                                failures.append(
                                    {
                                        "address": f"0x{identity.address:08x}",
                                        "error": str(disassemble.getStatusMsg()),
                                    }
                                )
                                continue
                        command = CreateFunctionCmd(address)
                        if not command.applyTo(program):
                            stats["failed"] += 1
                            failures.append(
                                {
                                    "address": f"0x{identity.address:08x}",
                                    "error": str(command.getStatusMsg()),
                                }
                            )
                            continue
                        function = function_manager.getFunctionAt(address)
                        if function is None:
                            stats["failed"] += 1
                            failures.append(
                                {
                                    "address": f"0x{identity.address:08x}",
                                    "error": "creation produced no function",
                                }
                            )
                            continue
                        stats["created"] += 1

                    if identity.size is not None:
                        expected_body = AddressSet(
                            address, address.add(identity.size - 1)
                        )
                        if function.getBody() != expected_body:
                            if dry_run:
                                stats["boundaries_adjusted"] += 1
                            else:
                                overlapping = list(function_manager.getFunctions(expected_body, True))
                                for other in overlapping:
                                    if other.getEntryPoint() != address:
                                        function_manager.removeFunction(other.getEntryPoint())
                                disassemble = DisassembleCommand(address, expected_body, True)
                                if not disassemble.applyTo(program):
                                    raise RuntimeError(
                                        f"failed to disassemble reviewed extent at "
                                        f"0x{identity.address:08x}: {disassemble.getStatusMsg()}"
                                    )
                                function.setBody(expected_body)
                                stats["boundaries_adjusted"] += 1

                    namespace, simple_name = _namespace_and_name(
                        symbol_table, program, identity.name
                    )
                    already_named = (
                        function.getName() == simple_name
                        and function.getParentNamespace() == namespace
                    )
                    if already_named:
                        stats["already_applied"] += 1
                    elif not dry_run:
                        conflicting_symbol = symbol_table.getSymbol(simple_name, address, namespace)
                        if (
                            conflicting_symbol is not None
                            and conflicting_symbol != function.getSymbol()
                            and not conflicting_symbol.delete()
                        ):
                            raise RuntimeError(
                                f"could not remove same-address label {identity.name}"
                            )
                        function.setParentNamespace(namespace)
                        function.setName(simple_name, SourceType.USER_DEFINED)
                        stats["renamed"] += 1
                    else:
                        stats["renamed"] += 1

                    if not dry_run:
                        for alias in identity.aliases:
                            alias_namespace, alias_simple = _namespace_and_name(
                                symbol_table, program, alias
                            )
                            if (
                                symbol_table.getSymbol(alias_simple, address, alias_namespace)
                                is None
                            ):
                                symbol_table.createLabel(
                                    address, alias_simple, alias_namespace, SourceType.USER_DEFINED
                                )
                                stats["aliased"] += 1
                        listing.setComment(
                            address,
                            CodeUnit.PLATE_COMMENT,
                            "\n".join(
                                [
                                    f"Owner: {identity.owner}",
                                    f"Identity: {identity.identity_id}",
                                    f"Confidence: {identity.confidence}",
                                    f"Name origin: {format_name_origin(identity.name_origin)}",
                                    f"Authority: {identity.authority}",
                                    *(
                                        [f"Source unit: {identity.source_unit}"]
                                        if identity.source_unit
                                        else []
                                    ),
                                    *(
                                        [f"Aliases: {', '.join(identity.aliases)}"]
                                        if identity.aliases
                                        else []
                                    ),
                                    *(
                                        [f"Evidence: {'; '.join(identity.evidence_ids)}"]
                                        if identity.evidence_ids
                                        else []
                                    ),
                                ]
                            ),
                        )
                commit = stats["failed"] == 0
            finally:
                if transaction is not None:
                    program.endTransaction(transaction, commit)
            if not dry_run and commit:
                program.save("apply reviewed function identities", pyghidra.task_monitor())
    finally:
        project.close()

    if failures:
        details = "; ".join(f"{item['address']}: {item['error']}" for item in failures)
        raise RuntimeError(f"function-map application failed and was rolled back: {details}")
    return {
        "program": program_name,
        "mapping": str(mapping_path.relative_to(settings.repo_dir)),
        "dry_run": dry_run,
        "accepted_identities": len(identities),
        **stats,
    }
