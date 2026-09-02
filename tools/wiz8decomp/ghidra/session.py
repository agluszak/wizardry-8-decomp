from __future__ import annotations

import hashlib
import json
import os
import socket
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

from ..config import REQUIRED_GHIDRA_VERSION, REQUIRED_PYGHIDRA_VERSION, Settings
from .environment import start_pyghidra
from .query import execute_query, validate_query_arguments
from .workspace import ensure_seed

_IDLE_SECONDS = 15 * 60


def _runtime_path(settings: Settings, selector: str) -> tuple[Path, str]:
    identity = "|".join(
        (
            str(settings.repo_dir.resolve()),
            str(settings.project_dir.resolve()),
            str(settings.ghidra_install_dir.resolve()),
            selector,
        )
    )
    token = hashlib.sha256(identity.encode()).hexdigest()[:20]
    directory = settings.build_dir / "ghidra"
    return directory / f"warm-session-{token}.json", token


def _worker_identity(settings: Settings, selector: str) -> dict[str, str]:
    code_path = settings.repo_dir / "tools/wiz8decomp/ghidra/session.py"
    return {
        "repo_dir": str(settings.repo_dir.resolve()),
        "project_dir": str(settings.project_dir.resolve()),
        "ghidra_install_dir": str(settings.ghidra_install_dir.resolve()),
        "selector": selector,
        "ghidra_version": REQUIRED_GHIDRA_VERSION,
        "pyghidra_version": REQUIRED_PYGHIDRA_VERSION,
        "session_code_mtime": str(
            code_path.stat().st_mtime_ns if code_path.exists() else "missing"
        ),
    }


class _WarmSession:
    """Small socket client for one checkout/project/selector worker."""

    def __init__(self, settings: Settings, selector: str) -> None:
        self.settings = settings
        self.selector = selector
        self.runtime_path, token = _runtime_path(settings, selector)
        self.socket_path = settings.build_dir / "ghidra" / f"warm-session-{token}.sock"

    def _metadata(self) -> dict[str, Any] | None:
        try:
            metadata = json.loads(self.runtime_path.read_text(encoding="utf-8"))
            if metadata.get("identity") != _worker_identity(self.settings, self.selector):
                return None
            os.kill(int(metadata["pid"]), 0)
            if not self.socket_path.exists():
                return None
            return metadata
        except (OSError, ValueError, KeyError, TypeError):
            return None

    def _start(self) -> None:
        self.runtime_path.parent.mkdir(parents=True, exist_ok=True)
        self.runtime_path.unlink(missing_ok=True)
        self.socket_path.unlink(missing_ok=True)
        metadata = {
            "identity": _worker_identity(self.settings, self.selector),
            "socket": str(self.socket_path),
            "runtime": {
                "GHIDRA_INSTALL_DIR": str(self.settings.ghidra_install_dir),
                "WIZ8_INPUT_DIR": str(self.settings.input_dir),
                "WIZ8_WORK_DIR": str(self.settings.work_dir),
                "WIZ8_GHIDRA_PROJECT_DIR": str(self.settings.project_dir),
            },
        }
        self.runtime_path.write_text(json.dumps(metadata) + "\n", encoding="utf-8")
        log = self.settings.build_dir / "logs" / "ghidra-warm-session.json"
        log.parent.mkdir(parents=True, exist_ok=True)
        stream = log.open("ab")
        subprocess.Popen(
            [sys.executable, "-m", "wiz8decomp.ghidra.session", "--worker", str(self.runtime_path)],
            cwd=self.settings.repo_dir,
            stdin=subprocess.DEVNULL,
            stdout=stream,
            stderr=subprocess.STDOUT,
            start_new_session=True,
            close_fds=True,
        )
        stream.close()
        deadline = time.monotonic() + 20
        while time.monotonic() < deadline:
            if self._metadata() is not None:
                return
            time.sleep(0.1)
        raise RuntimeError("Ghidra warm session did not become ready")

    def request(self, payload: dict[str, Any]) -> tuple[list[dict[str, Any]], str]:
        for attempt in range(2):
            if self._metadata() is None:
                self._start()
            try:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
                    connection.settimeout(120)
                    connection.connect(str(self.socket_path))
                    connection.sendall(json.dumps(payload).encode() + b"\n")
                    chunks: list[bytes] = []
                    while not chunks or not chunks[-1].endswith(b"\n"):
                        chunk = connection.recv(65536)
                        if not chunk:
                            raise ConnectionError("Ghidra warm session closed the connection")
                        chunks.append(chunk)
                result = json.loads(b"".join(chunks).decode())
                if "error" in result:
                    raise RuntimeError(str(result["error"]))
                return result["results"], str(result.get("transport", "pyghidra-warm"))
            except (OSError, ValueError, KeyError, TypeError, ConnectionError, RuntimeError):
                if attempt:
                    raise
                self.runtime_path.unlink(missing_ok=True)
                self.socket_path.unlink(missing_ok=True)
        raise AssertionError("unreachable")


def _query_many_cold(
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


def query_many(
    settings: Settings,
    selector: str,
    queries: list[tuple[str, list[str]]],
    *,
    function_seeds: list[str] | None = None,
) -> tuple[list[dict[str, Any]], str]:
    """Run a batch through a transparent checkout-scoped warm Ghidra session."""

    if not queries:
        raise ValueError("at least one query is required")
    for command, arguments in queries:
        validate_query_arguments(command, arguments)
    try:
        return _WarmSession(settings, selector).request(
            {
                "selector": selector,
                "queries": queries,
                "function_seeds": function_seeds or [],
            }
        )
    except (OSError, RuntimeError, ConnectionError, TimeoutError):
        # Keep the existing one-shot path as a safe fallback when the runtime
        # cannot host a persistent worker (for example in a headless CI job).
        return _query_many_cold(settings, selector, queries, function_seeds=function_seeds)


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


def _worker_main(runtime_path: str) -> None:
    metadata = json.loads(Path(runtime_path).read_text(encoding="utf-8"))
    runtime = metadata["runtime"]
    settings = Settings.model_validate(runtime)
    selector = str(metadata["identity"]["selector"])
    socket_path = Path(metadata["socket"])
    socket_path.unlink(missing_ok=True)
    program_name = ensure_seed(settings, selector)
    start_pyghidra(settings)
    import pyghidra

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as server:
        server.bind(str(socket_path))
        server.listen(4)
        metadata["pid"] = os.getpid()
        Path(runtime_path).write_text(json.dumps(metadata) + "\n", encoding="utf-8")
        server.settimeout(30)
        last_request = time.monotonic()
        try:
            while time.monotonic() - last_request < _IDLE_SECONDS:
                try:
                    connection, _ = server.accept()
                except TimeoutError:
                    continue
                last_request = time.monotonic()
                with connection:
                    try:
                        data = b""
                        while not data.endswith(b"\n"):
                            chunk = connection.recv(65536)
                            if not chunk:
                                raise ConnectionError("empty Ghidra request")
                            data += chunk
                        request = json.loads(data.decode())
                        with pyghidra.program_context(project, "/" + program_name) as program:
                            transaction = None
                            try:
                                function_seeds = request.get("function_seeds") or []
                                if function_seeds:
                                    from ghidra.app.cmd.disassemble import DisassembleCommand
                                    from ghidra.app.cmd.function import CreateFunctionCmd

                                    from .query import _address

                                    transaction = program.startTransaction(
                                        "disposable function seeds"
                                    )
                                    for seed in function_seeds:
                                        address = _address(program, seed)
                                        if (
                                            program.getFunctionManager().getFunctionContaining(
                                                address
                                            )
                                            is not None
                                        ):
                                            continue
                                        if program.getListing().getInstructionAt(
                                            address
                                        ) is None and not DisassembleCommand(
                                            address, None, True
                                        ).applyTo(program):
                                            raise ValueError(
                                                f"could not disassemble function seed {address}"
                                            )
                                        if not CreateFunctionCmd(address).applyTo(program):
                                            raise ValueError(
                                                f"could not create function seed {address}"
                                            )
                                results = [
                                    {
                                        "command": command,
                                        "arguments": arguments,
                                        "result": execute_query(program, command, arguments),
                                    }
                                    for command, arguments in request["queries"]
                                ]
                            finally:
                                from .semantic import dispose_sessions

                                dispose_sessions()
                                if transaction is not None:
                                    program.endTransaction(transaction, False)
                        connection.sendall(
                            json.dumps({"results": results, "transport": "pyghidra-warm"}).encode()
                            + b"\n"
                        )
                    except BaseException as error:  # noqa: BLE001 - Java exceptions cross this boundary.
                        connection.sendall(json.dumps({"error": str(error)}).encode() + b"\n")
        finally:
            project.close()
            socket_path.unlink(missing_ok=True)
            Path(runtime_path).unlink(missing_ok=True)


if __name__ == "__main__" and len(sys.argv) == 3 and sys.argv[1] == "--worker":
    _worker_main(sys.argv[2])
