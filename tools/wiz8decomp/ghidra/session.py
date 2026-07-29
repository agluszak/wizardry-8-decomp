from __future__ import annotations

from typing import Any

from ..config import Settings
from .environment import start_pyghidra
from .query import execute_query, validate_query_arguments
from .workspace import ensure_seed


def query_many(
    settings: Settings, selector: str, queries: list[tuple[str, list[str]]]
) -> tuple[list[dict[str, Any]], str]:
    """Run one ordered batch in one ordinary PyGhidra project session."""

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
        project.close()
    return results, "pyghidra"


def query(
    settings: Settings, selector: str, command: str, arguments: list[str]
) -> tuple[dict[str, Any], str]:
    results, transport = query_many(settings, selector, [(command, arguments)])
    return results[0]["result"], transport
