from __future__ import annotations

from typing import Any

from ..config import Settings
from .environment import start_pyghidra
from .query import execute_query, validate_query_arguments
from .workspace import ensure_seed


def query_many(
    settings: Settings,
    selector: str,
    queries: list[tuple[str, list[str]]],
    *,
    function_seeds: list[str] | None = None,
) -> tuple[list[dict[str, Any]], str]:
    """Run one ordered batch in one ordinary PyGhidra project session.

    Explicit function seeds are speculative analysis inputs. They are created
    inside a transaction that is always rolled back, so a data-pointer target
    can be decompiled without silently changing the reviewed Ghidra seed.
    """

    if not queries:
        raise ValueError("at least one query is required")
    for command, arguments in queries:
        validate_query_arguments(command, arguments)
    program_name = ensure_seed(settings, selector)
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = None
            try:
                if function_seeds:
                    from ghidra.app.cmd.disassemble import DisassembleCommand
                    from ghidra.app.cmd.function import CreateFunctionCmd

                    from .query import _address

                    transaction = program.startTransaction("disposable function seeds")
                    for seed in function_seeds:
                        address = _address(program, seed)
                        if program.getFunctionManager().getFunctionContaining(address) is not None:
                            continue
                        if program.getListing().getInstructionAt(address) is None:
                            command = DisassembleCommand(address, None, True)
                            if not command.applyTo(program):
                                raise ValueError(f"could not disassemble function seed {address}")
                        command = CreateFunctionCmd(address)
                        if not command.applyTo(program):
                            raise ValueError(f"could not create function seed {address}")
                results = [
                    {
                        "command": command,
                        "arguments": arguments,
                        "result": execute_query(program, command, arguments),
                    }
                    for command, arguments in queries
                ]
            finally:
                from .semantic import dispose_sessions

                dispose_sessions()
                if transaction is not None:
                    program.endTransaction(transaction, False)
    finally:
        project.close()
    return results, "pyghidra"


def query(
    settings: Settings,
    selector: str,
    command: str,
    arguments: list[str],
    *,
    function_seeds: list[str] | None = None,
) -> tuple[dict[str, Any], str]:
    results, transport = query_many(
        settings,
        selector,
        [(command, arguments)],
        function_seeds=function_seeds,
    )
    return results[0]["result"], transport
