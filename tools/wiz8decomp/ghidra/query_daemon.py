from __future__ import annotations

import json
import os
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

import typer

from ..config import Settings, load_settings
from ..paths import atomic_json
from .environment import start_pyghidra
from .project import resolve_program_name
from .query import execute_query, validate_query_arguments

internal_app = typer.Typer(add_completion=False, hidden=True)


def state_dir(settings: Settings) -> Path:
    return settings.work_dir / "daemon"


def state_path(settings: Settings) -> Path:
    return state_dir(settings) / "state.json"


def socket_path(settings: Settings) -> Path:
    return state_dir(settings) / "query.sock"


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


def start_daemon(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    status = daemon_status(settings)
    program = resolve_program_name(settings, selector)
    if status.get("running"):
        if status.get("program") != program:
            raise RuntimeError(f"daemon already serves {status.get('program')}; stop it before selecting {program}")
        return status
    directory = state_dir(settings)
    directory.mkdir(parents=True, exist_ok=True)
    log = (settings.build_dir / "logs" / "ghidra-daemon.log")
    log.parent.mkdir(parents=True, exist_ok=True)
    stream = log.open("ab")
    process = subprocess.Popen(
        [sys.executable, "-m", "wiz8decomp.ghidra.query_daemon", "--program", program],
        cwd=settings.repo_dir,
        stdin=subprocess.DEVNULL,
        stdout=stream,
        stderr=subprocess.STDOUT,
        start_new_session=True,
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


def stop_daemon(settings: Settings, *, quiet: bool = False) -> dict[str, Any]:
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


def daemon_query(settings: Settings, request: dict[str, Any]) -> dict[str, Any]:
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
        raise RuntimeError(response.get("error", "daemon query failed"))
    return response.get("result", {})


def one_shot_query(settings: Settings, program_name: str, command: str, arguments: list[str]) -> dict[str, Any]:
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            return execute_query(program, command, arguments)
    finally:
        project.close()


def query(settings: Settings, selector: str, command: str, arguments: list[str]) -> tuple[dict[str, Any], str]:
    validate_query_arguments(command, arguments)
    program = resolve_program_name(settings, selector)
    status = daemon_status(settings)
    if status.get("running") and status.get("program") == program:
        return daemon_query(settings, {"action": "query", "command": command, "arguments": arguments}), "daemon"
    return one_shot_query(settings, program, command, arguments), "one-shot"


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
            atomic_json(state_path(settings), {"pid": os.getpid(), "program": program, "socket": str(sock_path), "running": True})
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
                            command = request["command"]
                            arguments = request.get("arguments", [])
                            validate_query_arguments(command, arguments)
                            result = execute_query(ghidra_program, command, arguments)
                        response = {"ok": True, "result": result}
                    except Exception as error:
                        response = {"ok": False, "error": str(error), "type": type(error).__name__}
                    connection.sendall(json.dumps(response, ensure_ascii=False).encode("utf-8") + b"\n")
    finally:
        server.close()
        project.close()
        if sock_path.exists():
            sock_path.unlink()
        if state_path(settings).exists():
            state_path(settings).unlink()


if __name__ == "__main__":
    internal_app()
