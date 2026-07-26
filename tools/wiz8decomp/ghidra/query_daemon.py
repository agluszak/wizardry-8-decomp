from __future__ import annotations

import json
import os
import signal
import socket
import subprocess
import sys
import time
from collections.abc import Callable
from contextlib import contextmanager
from pathlib import Path
from typing import Any

import typer

from ..config import Settings, load_settings
from ..paths import atomic_json
from .environment import start_pyghidra
from .project import resolve_program_name
from .query import execute_query, validate_query_arguments

internal_app = typer.Typer(add_completion=False, hidden=True)


class DaemonProgramMismatchError(ConnectionError):
    """The daemon changed programs between lifecycle selection and query."""


def _materialize(settings: Settings, program: str) -> tuple[Settings, dict[str, Any]]:
    # Local import avoids the replay -> import_programs -> query_daemon cycle.
    from .cache import materialize_program

    return materialize_program(settings, program)


def state_dir(settings: Settings) -> Path:
    return settings.ghidra_runtime_dir


def state_path(settings: Settings) -> Path:
    return state_dir(settings) / "state.json"


def socket_path(settings: Settings) -> Path:
    return state_dir(settings) / "query.sock"


def lock_path(settings: Settings) -> Path:
    return state_dir(settings) / "lifecycle.lock"


@contextmanager
def lifecycle_lock(settings: Settings) -> Any:
    """Serialize daemon replacement across agents sharing WIZ8_WORK_DIR."""

    import fcntl

    directory = state_dir(settings)
    directory.mkdir(parents=True, exist_ok=True)
    with lock_path(settings).open("a+", encoding="utf-8") as stream:
        fcntl.flock(stream.fileno(), fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def daemon_status(settings: Settings) -> dict[str, Any]:
    path = state_path(settings)
    if not path.is_file():
        return {"running": False}
    try:
        state = json.loads(path.read_text(encoding="utf-8"))
        os.kill(state["pid"], 0)
        state["running"] = socket_path(settings).exists()
        return state
    except (OSError, ValueError, KeyError):
        return {"running": False, "stale_state": True}


def _start_daemon(settings: Settings, program: str) -> dict[str, Any]:
    status = daemon_status(settings)
    if status.get("running"):
        if status.get("program") != program:
            raise RuntimeError(
                f"daemon already serves {status.get('program')}; stop it before selecting {program}"
            )
        return status
    directory = state_dir(settings)
    directory.mkdir(parents=True, exist_ok=True)
    log = settings.build_dir / "logs" / "ghidra-daemon.log"
    log.parent.mkdir(parents=True, exist_ok=True)
    stream = log.open("ab")
    environment = os.environ.copy()
    environment["WIZ8_GHIDRA_PROJECT_DIR"] = str(settings.project_dir)
    environment["WIZ8_GHIDRA_RUNTIME_DIR"] = str(settings.ghidra_runtime_dir)
    process = subprocess.Popen(
        [sys.executable, "-m", "wiz8decomp.ghidra.query_daemon", "--program", program],
        cwd=settings.repo_dir,
        stdin=subprocess.DEVNULL,
        stdout=stream,
        stderr=subprocess.STDOUT,
        start_new_session=True,
        env=environment,
    )
    stream.close()
    for _ in range(200):
        time.sleep(0.05)
        status = daemon_status(settings)
        if status.get("running"):
            return status
        if process.poll() is not None:
            raise RuntimeError(f"Ghidra daemon exited with status {process.returncode}; see {log}")
    process.terminate()
    raise RuntimeError(f"Ghidra daemon did not become ready; see {log}")


def start_daemon(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    program = resolve_program_name(settings, selector)
    effective, _ = _materialize(settings, program)
    with lifecycle_lock(effective):
        return _start_daemon(effective, program)


def _stop_daemon(settings: Settings) -> dict[str, Any]:
    status = daemon_status(settings)
    if not status.get("running"):
        return {"running": False, "stopped": False}
    try:
        response = daemon_query(settings, {"action": "stop"})
    except OSError:
        os.kill(status["pid"], signal.SIGTERM)
        response = {"stopped": True, "forced": True}
    for _ in range(100):
        try:
            os.kill(status["pid"], 0)
        except OSError:
            break
        time.sleep(0.05)
    return {"running": False, **response}


def stop_daemon(settings: Settings, *, quiet: bool = False) -> dict[str, Any]:
    del quiet
    with lifecycle_lock(settings):
        return _stop_daemon(settings)


def ensure_daemon(settings: Settings, program: str) -> dict[str, Any]:
    """Return a daemon for PROGRAM, replacing a daemon for another program."""

    with lifecycle_lock(settings):
        status = daemon_status(settings)
        if status.get("running") and status.get("program") != program:
            _stop_daemon(settings)
            status = {"running": False}
        if not status.get("running"):
            status = _start_daemon(settings, program)
        return status


def daemon_query(settings: Settings, request: dict[str, Any]) -> Any:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(180)
        client.connect(str(socket_path(settings)))
        client.sendall(json.dumps(request).encode("utf-8") + b"\n")
        chunks = bytearray()
        while True:
            chunk = client.recv(1024 * 1024)
            if not chunk:
                break
            chunks.extend(chunk)
            if b"\n" in chunk:
                break
    response = json.loads(bytes(chunks).decode("utf-8"))
    if not response.get("ok"):
        if response.get("type") == "DaemonProgramMismatchError":
            raise DaemonProgramMismatchError(response.get("error", "daemon program changed"))
        raise RuntimeError(response.get("error", "daemon query failed"))
    return response.get("result", {})


def one_shot_query(
    settings: Settings, program_name: str, command: str, arguments: list[str]
) -> dict[str, Any]:
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            return execute_query(program, command, arguments)
    finally:
        project.close()


def one_shot_queries(
    settings: Settings, program_name: str, queries: list[tuple[str, list[str]]]
) -> list[dict[str, Any]]:
    """Execute an ordered query batch while opening the Ghidra project only once."""

    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            return [
                {
                    "command": command,
                    "arguments": arguments,
                    "result": execute_query(program, command, arguments),
                }
                for command, arguments in queries
            ]
    finally:
        project.close()


def _query_with_transport(
    settings: Settings,
    program: str,
    request: dict[str, Any],
    fallback: Callable[[], Any],
) -> tuple[Any, str]:
    """Use the selected daemon, retry it once, then use the supplied one-shot query."""

    try:
        ensure_daemon(settings, program)
    except (ConnectionError, json.JSONDecodeError, OSError, RuntimeError, TimeoutError):
        return fallback(), "one-shot-fallback"

    try:
        return daemon_query(settings, request), "daemon"
    except (ConnectionError, json.JSONDecodeError, OSError):
        # The daemon may have exited between the lifecycle check and the
        # query. Replace it once; a persistent lifecycle failure uses the safe
        # one-shot path below. Query errors returned by a healthy daemon are
        # deliberately not caught here.
        try:
            stop_daemon(settings, quiet=True)
            ensure_daemon(settings, program)
        except (ConnectionError, json.JSONDecodeError, OSError, RuntimeError, TimeoutError):
            return fallback(), "one-shot-fallback"
        try:
            return daemon_query(settings, request), "daemon"
        except (ConnectionError, json.JSONDecodeError, OSError, TimeoutError):
            return fallback(), "one-shot-fallback"


def query(
    settings: Settings, selector: str, command: str, arguments: list[str]
) -> tuple[dict[str, Any], str]:
    validate_query_arguments(command, arguments)
    program = resolve_program_name(settings, selector)
    effective, _ = _materialize(settings, program)
    request = {
        "action": "query",
        "program": program,
        "command": command,
        "arguments": arguments,
    }
    return _query_with_transport(
        effective,
        program,
        request,
        lambda: one_shot_query(effective, program, command, arguments),
    )


def query_many(
    settings: Settings, selector: str, queries: list[tuple[str, list[str]]]
) -> tuple[list[dict[str, Any]], str]:
    """Execute several validated queries against one materialized program in order."""

    if not queries:
        raise ValueError("at least one query is required")
    for command, arguments in queries:
        validate_query_arguments(command, arguments)
    program = resolve_program_name(settings, selector)
    effective, _ = _materialize(settings, program)
    request = {
        "action": "query-many",
        "program": program,
        "queries": [{"command": command, "arguments": arguments} for command, arguments in queries],
    }
    return _query_with_transport(
        effective,
        program,
        request,
        lambda: one_shot_queries(effective, program, queries),
    )


@internal_app.command()
def serve(program: str = typer.Option(..., "--program")) -> None:
    settings = load_settings()
    assert settings is not None
    start_pyghidra(settings)
    import pyghidra

    directory = state_dir(settings)
    directory.mkdir(parents=True, exist_ok=True)
    sock_path = socket_path(settings)
    if sock_path.exists():
        sock_path.unlink()
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        with pyghidra.program_context(project, "/" + program) as ghidra_program:
            server.bind(str(sock_path))
            os.chmod(sock_path, 0o600)
            server.listen(8)
            atomic_json(
                state_path(settings),
                {"pid": os.getpid(), "program": program, "socket": str(sock_path), "running": True},
            )
            stopping = False
            while not stopping:
                connection, _ = server.accept()
                with connection:
                    payload = bytearray()
                    while b"\n" not in payload:
                        chunk = connection.recv(1024 * 1024)
                        if not chunk:
                            break
                        payload.extend(chunk)
                    try:
                        request = json.loads(bytes(payload).decode("utf-8"))
                        if request.get("action") == "stop":
                            result, stopping = {"stopped": True}, True
                        else:
                            requested_program = request.get("program")
                            if requested_program != program:
                                raise DaemonProgramMismatchError(
                                    f"daemon serves {program}, request targets {requested_program}"
                                )
                            if request.get("action") == "query-many":
                                queries = request.get("queries", [])
                                if not isinstance(queries, list) or not queries:
                                    raise ValueError("query-many requires at least one query")
                                normalized = []
                                for query_request in queries:
                                    if not isinstance(query_request, dict):
                                        raise TypeError("each query must be an object")
                                    command = query_request.get("command")
                                    arguments = query_request.get("arguments", [])
                                    if (
                                        not isinstance(command, str)
                                        or not isinstance(arguments, list)
                                        or not all(isinstance(item, str) for item in arguments)
                                    ):
                                        raise ValueError(
                                            "each query needs a string command and string arguments"
                                        )
                                    validate_query_arguments(command, arguments)
                                    normalized.append((command, arguments))
                                result = [
                                    {
                                        "command": command,
                                        "arguments": arguments,
                                        "result": execute_query(ghidra_program, command, arguments),
                                    }
                                    for command, arguments in normalized
                                ]
                            elif request.get("action") == "query":
                                command = request["command"]
                                arguments = request.get("arguments", [])
                                validate_query_arguments(command, arguments)
                                result = execute_query(ghidra_program, command, arguments)
                            else:
                                raise ValueError(
                                    f"unsupported daemon action: {request.get('action')}"
                                )
                        response = {"ok": True, "result": result}
                    except Exception as error:
                        response = {"ok": False, "error": str(error), "type": type(error).__name__}
                    connection.sendall(
                        json.dumps(response, ensure_ascii=False).encode("utf-8") + b"\n"
                    )
    finally:
        server.close()
        project.close()
        if sock_path.exists():
            sock_path.unlink()
        if state_path(settings).exists():
            state_path(settings).unlink()


if __name__ == "__main__":
    internal_app()
