from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from ..config import Settings
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

ACCEPTED_CONFIDENCE = frozenset({"exact", "high", "strong"})


@dataclass(frozen=True)
class FunctionIdentity:
    address: int
    name: str
    owner: str
    confidence: str
    evidence: str


def load_function_identities(path: Path) -> list[FunctionIdentity]:
    identities: list[FunctionIdentity] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row_number, row in enumerate(csv.DictReader(stream), start=2):
            missing = {
                field
                for field in ("address", "provisional_name", "owner", "confidence", "evidence")
                if field not in row
            }
            if missing:
                raise ValueError(
                    f"{path}:{row_number}: missing columns: {', '.join(sorted(missing))}"
                )
            name = row["provisional_name"].strip()
            confidence = row["confidence"].strip()
            if not name or confidence not in ACCEPTED_CONFIDENCE:
                continue
            identities.append(
                FunctionIdentity(
                    address=int(row["address"], 16),
                    name=name,
                    owner=row["owner"].strip(),
                    confidence=confidence,
                    evidence=row["evidence"].strip(),
                )
            )
    identities.sort(key=lambda identity: identity.address)
    if len({identity.address for identity in identities}) != len(identities):
        raise ValueError(f"{path}: duplicate accepted function addresses")
    return identities


def _namespace_and_name(symbol_table: Any, program: Any, qualified_name: str) -> tuple[Any, str]:
    parts = qualified_name.split("::")
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
) -> dict[str, Any]:
    mapping_path = mapping_path.resolve()
    if not mapping_path.is_file():
        raise ValueError(f"function map does not exist: {mapping_path}")
    identities = load_function_identities(mapping_path)
    if not identities:
        raise ValueError(f"function map has no accepted identities: {mapping_path}")

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.listing import CodeUnit
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    stats = {"created": 0, "renamed": 0, "already_applied": 0, "failed": 0}
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
                    if function is None:
                        if dry_run:
                            stats["created"] += 1
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
                        conflicting_symbol = symbol_table.getSymbol(
                            simple_name, address, namespace
                        )
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
                        listing.setComment(
                            address,
                            CodeUnit.PLATE_COMMENT,
                            f"Owner: {identity.owner}\nConfidence: {identity.confidence}\nEvidence: {identity.evidence}",
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
