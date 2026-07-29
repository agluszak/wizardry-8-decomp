"""Apply source-owned function names to the reviewed Ghidra workspace."""

from __future__ import annotations

from typing import Any

from ..config import Settings
from ..source_model import build_source_model
from .environment import start_pyghidra
from .workspace import ensure_seed


def sync_source_names(
    settings: Settings, selector: str = "wiz8", *, apply: bool = False
) -> dict[str, Any]:
    """Report or apply source marker names without touching unclaimed entities."""

    program_name = ensure_seed(settings, selector)
    model = build_source_model(settings.repo_dir)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd
    from ghidra.program.model.symbol import SourceType

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    changed = 0
    current = 0
    missing: list[str] = []
    transaction = None
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            try:
                functions = program.getFunctionManager()
                symbols = program.getSymbolTable()
                address_space = program.getAddressFactory().getDefaultAddressSpace()
                listing = program.getListing()
                global_namespace = program.getGlobalNamespace()

                def namespace(parts: list[str]):
                    parent = global_namespace
                    for part in parts:
                        existing = symbols.getNamespace(part, parent)
                        parent = (
                            existing
                            if existing is not None
                            else symbols.createClass(parent, part, SourceType.USER_DEFINED)
                        )
                    return parent

                if apply:
                    transaction = program.startTransaction("synchronize source function names")
                for address, source in model.functions.items():
                    target = address_space.getAddress(address)
                    function = functions.getFunctionAt(target)
                    if function is None:
                        if not apply:
                            missing.append(f"{address:08x}")
                            continue
                        if listing.getInstructionAt(target) is None:
                            DisassembleCommand(target, None, True).applyTo(
                                program, pyghidra.task_monitor()
                            )
                        command = CreateFunctionCmd(target)
                        if command.applyTo(program, pyghidra.task_monitor()):
                            function = functions.getFunctionAt(target)
                        if function is None:
                            missing.append(f"{address:08x}")
                            continue
                    if function.getName(True) == source.name:
                        current += 1
                        continue
                    changed += 1
                    if apply:
                        parts = source.name.split("::")
                        function.setParentNamespace(namespace(parts[:-1]))
                        if function.getName() != parts[-1]:
                            function.setName(parts[-1], SourceType.USER_DEFINED)
                if missing and apply:
                    raise ValueError(
                        "source markers do not resolve to Ghidra functions: " + ", ".join(missing)
                    )
                if apply:
                    assert transaction is not None
                    program.endTransaction(transaction, True)
                    transaction = None
                    program.save("synchronize source function names", pyghidra.task_monitor())
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
        "source_functions": len(model.functions),
        "already_current": current,
        "changed": changed,
        "missing": missing,
    }
