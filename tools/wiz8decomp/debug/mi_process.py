"""Asynchronous subprocess and command correlation for GDB/MI3."""

from __future__ import annotations

import asyncio
from dataclasses import dataclass
from pathlib import Path

from .mi_protocol import MiRecord, parse_mi_record, stream_text


class DebuggerTransportError(RuntimeError):
    """The debugger transport failed independently of the inferior."""


@dataclass(frozen=True)
class MiCommandResult:
    result: MiRecord
    output: tuple[str, ...]


class GdbMiProcess:
    """Own one GDB process and continuously service its MI streams."""

    def __init__(
        self,
        cwd: Path,
        transcript_path: Path,
        command: tuple[str, ...] = ("gdb", "-q", "-nx", "--interpreter=mi3"),
    ) -> None:
        self.cwd = cwd
        self.transcript_path = transcript_path
        self.command_line = command
        self.process: asyncio.subprocess.Process | None = None
        self.events: asyncio.Queue[MiRecord] = asyncio.Queue()
        self._next_token = 1
        self._pending: dict[int, asyncio.Future[MiRecord]] = {}
        self._command_output: dict[int, list[str]] = {}
        self._active_token: int | None = None
        self._command_lock = asyncio.Lock()
        self._tasks: list[asyncio.Task[None]] = []
        self._transcript = None

    async def start(self, executable: Path) -> None:
        self._transcript = self.transcript_path.open("w", encoding="utf-8")
        try:
            self.process = await asyncio.create_subprocess_exec(
                *self.command_line,
                str(executable),
                cwd=self.cwd,
                stdin=asyncio.subprocess.PIPE,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
        except OSError as error:
            self._transcript.close()
            self._transcript = None
            raise DebuggerTransportError(f"could not start GDB: {error}") from error
        self._tasks = [
            asyncio.create_task(self._read_stdout()),
            asyncio.create_task(self._read_stderr()),
            asyncio.create_task(self._watch_exit()),
        ]

    @property
    def pid(self) -> int | None:
        return self.process.pid if self.process is not None else None

    @property
    def returncode(self) -> int | None:
        return self.process.returncode if self.process is not None else None

    async def command(self, command: str, timeout: float = 30.0) -> MiCommandResult:
        process = self._require_process()
        if process.returncode is not None or process.stdin is None:
            raise DebuggerTransportError("GDB is not running")
        async with self._command_lock:
            token = self._next_token
            self._next_token += 1
            future = asyncio.get_running_loop().create_future()
            self._pending[token] = future
            self._command_output[token] = []
            self._active_token = token
            try:
                process.stdin.write(f"{token}{command}\n".encode())
                await process.stdin.drain()
                try:
                    result = await asyncio.wait_for(future, timeout)
                except TimeoutError as error:
                    raise DebuggerTransportError(f"GDB command timed out: {command}") from error
                output = tuple(self._command_output[token])
                if result.message == "error":
                    raise DebuggerTransportError(f"GDB command failed: {command}: {result.raw}")
                return MiCommandResult(result, output)
            finally:
                self._pending.pop(token, None)
                self._command_output.pop(token, None)
                if self._active_token == token:
                    self._active_token = None

    async def drain_events(self) -> list[MiRecord]:
        records: list[MiRecord] = []
        while True:
            try:
                records.append(self.events.get_nowait())
            except asyncio.QueueEmpty:
                return records

    async def next_event(self, timeout: float) -> MiRecord | None:
        try:
            return await asyncio.wait_for(self.events.get(), timeout)
        except TimeoutError:
            return None

    async def close(self) -> None:
        process = self.process
        if process is not None and process.returncode is None:
            try:
                await self.command("-gdb-exit", timeout=5.0)
            except DebuggerTransportError:
                process.kill()
            try:
                await asyncio.wait_for(process.wait(), 5.0)
            except TimeoutError:
                process.kill()
                await process.wait()
        for task in self._tasks:
            if not task.done():
                task.cancel()
        if self._tasks:
            await asyncio.gather(*self._tasks, return_exceptions=True)
        self._tasks = []
        if self._transcript is not None:
            self._transcript.close()
            self._transcript = None

    async def _read_stdout(self) -> None:
        process = self._require_process()
        assert process.stdout is not None
        while line_bytes := await process.stdout.readline():
            line = line_bytes.decode(errors="replace").rstrip("\r\n")
            if not line or line == "(gdb)":
                continue
            self._log(line)
            record = parse_mi_record(line)
            if record.record_type == "result" and record.token is not None:
                future = self._pending.get(record.token)
                if future is not None and not future.done():
                    future.set_result(record)
                    continue
            text = stream_text(record)
            if text is not None and self._active_token is not None:
                self._command_output[self._active_token].append(text)
            elif record.record_type in {"notify", "exec", "status", "unparsed"}:
                await self.events.put(record)

    async def _read_stderr(self) -> None:
        process = self._require_process()
        assert process.stderr is not None
        while line_bytes := await process.stderr.readline():
            line = line_bytes.decode(errors="replace").rstrip("\r\n")
            self._log(f"&stderr {line}")
            record = MiRecord(line, "gdb-stderr", None, line, None)
            if self._active_token is not None:
                self._command_output[self._active_token].append(line)
            else:
                await self.events.put(record)

    async def _watch_exit(self) -> None:
        process = self._require_process()
        returncode = await process.wait()
        error = DebuggerTransportError(f"GDB exited with {returncode}")
        for future in self._pending.values():
            if not future.done():
                future.set_exception(error)
        await self.events.put(
            MiRecord(
                raw=f"GDB exited with {returncode}",
                record_type="debugger-exit",
                message="debugger-exited",
                payload={"returncode": returncode},
                token=None,
            )
        )

    def _log(self, line: str) -> None:
        if self._transcript is not None:
            self._transcript.write(line + "\n")
            self._transcript.flush()

    def _require_process(self) -> asyncio.subprocess.Process:
        if self.process is None:
            raise DebuggerTransportError("GDB has not been started")
        return self.process
