"""Read-only queries against the checkout's canonical Ghidra project.

The module keeps its historical name temporarily so callers do not need a
pointless rename-only migration. There is no daemon, socket, lifecycle state or
implicit content-addressed materialization: a query starts PyGhidra, opens the
existing project once, executes one ordered batch, and closes it.
"""

from __future__ import annotations

from contextlib import contextmanager
from typing import Any

from ..config import Settings
from .environment import start_pyghidra
from .project import resolve_program_name
from .query import execute_query, validate_query_arguments


@contextmanager
def open_program(settings: Settings, selector: str):
    """Open one existing program for an ordered read-only query batch."""

    program_name = resolve_program_name(settings, selector)
    start_pyghidra(settings)
    import pyghidra

    try:
        project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    except FileNotFoundError as error:
        raise RuntimeError(
            f"Ghidra project is missing at {settings.project_dir}; "
            f"run `just ghidra rebuild {program_name}` first"
        ) from error
    try:
        domain_file = project.getProjectData().getFile("/" + program_name)
        if domain_file is None:
            raise RuntimeError(
                f"Ghidra program {program_name} is missing from {settings.project_dir}; "
                f"run `just ghidra rebuild {program_name}` first"
            )
        with pyghidra.program_context(project, "/" + program_name) as program:
            yield program_name, program
    finally:
        project.close()


def query(
    settings: Settings, selector: str, command: str, arguments: list[str]
) -> tuple[dict[str, Any], str]:
    validate_query_arguments(command, arguments)
    with open_program(settings, selector) as (_program_name, program):
        return execute_query(program, command, arguments), "one-shot"


def query_many(
    settings: Settings, selector: str, queries: list[tuple[str, list[str]]]
) -> tuple[list[dict[str, Any]], str]:
    """Execute an ordered query batch while opening the project only once."""

    if not queries:
        raise ValueError("at least one query is required")
    for command, arguments in queries:
        validate_query_arguments(command, arguments)
    with open_program(settings, selector) as (_program_name, program):
        return [
            {
                "command": command,
                "arguments": arguments,
                "result": execute_query(program, command, arguments),
            }
            for command, arguments in queries
        ], "one-shot"


def daemon_status(_settings: Settings) -> dict[str, Any]:
    """Compatibility result for callers being migrated away from daemon plumbing."""

    return {"running": False, "removed": True}


def stop_daemon(_settings: Settings, *, quiet: bool = False) -> dict[str, Any]:
    """Compatibility no-op; the persistent query process no longer exists."""

    del quiet
    return {"running": False, "stopped": False, "removed": True}
