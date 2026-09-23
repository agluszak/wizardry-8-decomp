"""Wine GDB proxy lifecycle and persistent GDB/MI supervision."""

from __future__ import annotations

import asyncio
import json
import re
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path

from .mi_process import DebuggerTransportError, GdbMiProcess
from .mi_protocol import MiRecord

REPO_ROOT = Path(__file__).resolve().parents[3]
PROXY_START_TIMEOUT_SECONDS = 45.0

__all__ = [
    "CrashSnapshot",
    "DebuggerTransportError",
    "GdbSession",
    "StopEvent",
    "WineGdbProxy",
    "allocate_port",
    "is_terminal_stop",
    "stop_event_from_record",
    "terminal_stop_summary",
]

REGISTER_LINE = re.compile(
    r"^(?P<name>eip|esp|ebp|eax|ebx|ecx|edx|esi|edi)\s+0x(?P<value>[0-9a-fA-F]+)\b"
)
MEMORY_WORD_LINE = re.compile(r"^0x(?P<address>[0-9a-fA-F]+):\s+(?P<words>(?:0x[0-9a-fA-F]+\s*)+)$")
HEX_VALUE = re.compile(r"=.*?0x(?P<value>[0-9a-fA-F]+)\b")


@dataclass(frozen=True)
class StopEvent:
    reason: str
    signal: str | None
    exit_code: int | None
    raw: str
    thread_id: str | None = None


@dataclass(frozen=True)
class CrashSnapshot:
    event: StopEvent
    registers: dict[str, int]
    frame_addresses: tuple[int, ...]
    stack_words: tuple[tuple[int, int], ...]
    fault_address: int | None
    raw_path: Path


def _parse_frame_addresses(lines: list[str]) -> tuple[int, ...]:
    return tuple(
        int(match.group(1), 16)
        for line in lines
        if (match := re.match(r"^\s*#\d+\s+0x([0-9a-fA-F]+)\b", line))
    )


def _parse_registers(lines: list[str]) -> dict[str, int]:
    registers: dict[str, int] = {}
    for line in lines:
        match = REGISTER_LINE.match(line)
        if match is not None:
            registers[match.group("name")] = int(match.group("value"), 16)
    return registers


def _parse_stack_words(lines: list[str]) -> tuple[tuple[int, int], ...]:
    words: list[tuple[int, int]] = []
    for line in lines:
        match = MEMORY_WORD_LINE.match(line)
        if match is None:
            continue
        row = int(match.group("address"), 16)
        for index, value in enumerate(match.group("words").split()):
            words.append((row + index * 4, int(value, 16)))
    return tuple(words)


def stop_event_from_record(record: MiRecord) -> StopEvent | None:
    if record.message != "stopped" or not isinstance(record.payload, dict):
        return None
    reason = record.payload.get("reason")
    signal_name = record.payload.get("signal-name")
    thread_id = record.payload.get("thread-id")
    return StopEvent(
        reason=reason if isinstance(reason, str) else "stopped",
        signal=signal_name if isinstance(signal_name, str) else None,
        exit_code=_parse_exit_code(record.payload.get("exit-code")),
        raw=record.raw,
        thread_id=thread_id if isinstance(thread_id, str) else None,
    )


def is_terminal_stop(event: StopEvent) -> bool:
    return event.reason in {"exited", "exited-normally", "exited-signalled"}


def terminal_stop_summary(event: StopEvent) -> tuple[str, str]:
    """Classify a terminal inferior stop as (reason, report).

    Only a normal exit is success. ``exited`` carries the process exit code and
    ``exited-signalled`` the terminating signal, so a killed or crashed
    inferior is never reported as a normal exit.
    """
    if event.reason == "exited-normally":
        return "exited normally", "process exited normally"
    if event.reason == "exited-signalled":
        signal = event.signal or "unknown"
        return f"terminated by signal {signal}", f"process terminated by signal {signal}"
    code = event.exit_code
    if code is None:
        return "exited with unknown code", "process exited with an unknown code"
    return f"exited with code {code}", f"process exited with code {code}"


def _parse_exit_code(value: object) -> int | None:
    if not isinstance(value, str):
        return None
    try:
        return int(value, 0)
    except ValueError:
        # GDB/MI reports remote exit codes in C-style octal (for example
        # ``0177``), which Python 3 deliberately does not accept with base 0.
        try:
            return int(value, 8) if value.startswith("0") else int(value, 10)
        except ValueError:
            return None


def allocate_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def _port_is_listening(port: int) -> bool:
    suffix = f":{port:04X}"
    for table in (Path("/proc/net/tcp"), Path("/proc/net/tcp6")):
        try:
            lines = table.read_text(encoding="ascii").splitlines()[1:]
        except OSError:
            continue
        for line in lines:
            fields = line.split()
            if len(fields) >= 4 and fields[1].endswith(suffix) and fields[3] == "0A":
                return True
    return False


class WineGdbProxy:
    def __init__(
        self,
        executable: Path,
        cwd: Path,
        environment: dict[str, str],
        port: int | None = None,
        log_path: Path | None = None,
        arguments: tuple[str, ...] = (),
        launch_command: tuple[str, ...] = ("winedbg",),
    ) -> None:
        self.executable = executable
        self.cwd = cwd
        self.environment = environment
        self.port = allocate_port() if port is None else port
        self.log_path = log_path
        self.arguments = arguments
        self.launch_command = launch_command
        self.process: subprocess.Popen[bytes] | None = None
        self._log = None

    def start(self) -> None:
        if not self.executable.is_file():
            raise DebuggerTransportError(f"missing debugger inferior {self.executable}")
        output = subprocess.DEVNULL
        if self.log_path is not None:
            self._log = self.log_path.open("wb")
            output = self._log
        self.process = subprocess.Popen(
            [
                *self.launch_command,
                "--gdb",
                "--no-start",
                "--port",
                str(self.port),
                str(self.executable),
                *self.arguments,
            ],
            cwd=self.cwd,
            env=self.environment,
            stdout=output,
            stderr=subprocess.STDOUT,
        )
        deadline = time.monotonic() + PROXY_START_TIMEOUT_SECONDS
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise DebuggerTransportError(
                    f"winedbg --gdb exited early with {self.process.returncode}"
                )
            if _port_is_listening(self.port):
                return
            time.sleep(0.1)
        self.close()
        raise DebuggerTransportError("winedbg --gdb did not open its port")

    def close(self) -> None:
        if self.process is not None and self.process.poll() is None:
            self.process.kill()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
        if self._log is not None:
            self._log.close()
            self._log = None


class GdbSession:
    """Persistent passive GDB observer for one Wine inferior."""

    def __init__(
        self,
        executable: Path,
        cwd: Path,
        environment: dict[str, str],
        artifact_dir: Path,
        arguments: tuple[str, ...] = (),
        launch_command: tuple[str, ...] = ("winedbg",),
    ) -> None:
        self.executable = executable
        self.artifact_dir = artifact_dir
        self.proxy = WineGdbProxy(
            executable,
            cwd,
            environment,
            log_path=artifact_dir / "winedbg.log",
            arguments=arguments,
            launch_command=launch_command,
        )
        self._mi: GdbMiProcess | None = None

    async def start(self) -> None:
        self.proxy.start()
        self._mi = GdbMiProcess(REPO_ROOT, self.artifact_dir / "gdb.log")
        try:
            await asyncio.wait_for(self._mi.start(self.executable), 15)
            await self._command("-gdb-set pagination off")
            await self._command("-gdb-set confirm off")
            await self._command("-gdb-set mi-async on")
            await self._command(f"-target-select remote localhost:{self.proxy.port}", timeout=45)
            if await self.wait_for_stop(15) is None:
                raise DebuggerTransportError("GDB remote target did not report its initial stop")
            await self._console("handle SIGTRAP stop print nopass")
            await self._console("handle SIGSEGV stop print pass")
        except Exception:
            await self.close()
            raise

    async def continue_inferior(self) -> None:
        await self._command("-exec-continue --all")

    async def set_breakpoint(self, address: int, condition: str | None = None) -> None:
        prefix = f"-break-insert -c {json.dumps(condition)} " if condition else "-break-insert "
        await self._command(f"{prefix}*0x{address:08x}")

    async def wait_for_stop(self, timeout: float) -> StopEvent | None:
        deadline = time.monotonic() + timeout
        while True:
            remaining = max(0.0, deadline - time.monotonic())
            record = await self._next_event(remaining)
            if record is None:
                return None
            if event := stop_event_from_record(record):
                return event

    async def capture_stop(self, label: str, event: StopEvent) -> CrashSnapshot:
        if is_terminal_stop(event):
            raise DebuggerTransportError("cannot capture an exited inferior")
        if event.thread_id is not None:
            await self._command(f"-thread-select {json.dumps(event.thread_id)}")
        path = self.artifact_dir / f"debugger-stop-{label}.txt"
        sections = [("stop", [event.raw])]
        captured: dict[str, list[str]] = {}
        for heading, command in (
            ("stopped thread", "bt 16"),
            ("registers", "info registers"),
            ("stack", "x/192wx $sp"),
            ("near pc", "x/24i $pc-24"),
            ("shared libraries", "info sharedlibrary"),
            ("all threads", "thread apply all bt 16"),
        ):
            try:
                output = await self._console(command, timeout=30)
            except DebuggerTransportError as error:
                output = [str(error)]
            captured[heading] = output
            sections.append((heading, output))
        registers = _parse_registers(captured.get("registers", []))
        fault_output: list[str] = []
        if event.signal == "SIGSEGV":
            try:
                fault_output = await self._console(
                    "p/x $_siginfo._sifields._sigfault.si_addr", timeout=10
                )
            except DebuggerTransportError as error:
                fault_output = [str(error)]
            sections.append(("fault address", fault_output))
        with path.open("w", encoding="utf-8") as report:
            for heading, output in sections:
                report.write(f"=== {heading} ===\n")
                report.writelines(line if line.endswith("\n") else line + "\n" for line in output)
        fault_match = next(
            (match for line in fault_output if (match := HEX_VALUE.search(line))), None
        )
        fault_address = int(fault_match.group("value"), 16) if fault_match is not None else None
        stack_words = _parse_stack_words(captured.get("stack", []))
        return CrashSnapshot(
            event=event,
            registers=registers,
            frame_addresses=_parse_frame_addresses(captured.get("stopped thread", [])),
            stack_words=stack_words,
            fault_address=fault_address,
            raw_path=path,
        )

    async def interrupt_and_capture(
        self, label: str
    ) -> tuple[CrashSnapshot | None, StopEvent | None]:
        event = await self.wait_for_stop(0)
        if event is None:
            try:
                await self._command("-exec-interrupt --all", timeout=5)
            except DebuggerTransportError:
                event = await self.wait_for_stop(2)
                if event is not None and is_terminal_stop(event):
                    return None, event
                raise
            event = await self.wait_for_stop(2)
        if event is None:
            try:
                await self._console("interrupt", timeout=5)
            except DebuggerTransportError:
                event = await self.wait_for_stop(2)
                if event is not None and is_terminal_stop(event):
                    return None, event
                raise
            event = await self.wait_for_stop(10)
        if event is None:
            raise DebuggerTransportError(
                "GDB accepted interrupt requests but the Wine remote target did not stop"
            )
        if is_terminal_stop(event):
            return None, event
        capture_label = label
        if event.signal not in {None, "SIGINT"}:
            capture_label = event.signal.lower()  # type: ignore[union-attr]
        return await self.capture_stop(capture_label, event), event

    async def close(self) -> None:
        if self._mi is not None:
            try:
                await self._mi.close()
            except Exception:  # noqa: BLE001, S110 - shutdown must not mask the original error
                pass
        self._mi = None
        self.proxy.close()

    async def _command(self, command: str, timeout: float = 30.0) -> list[str]:
        result = await self._require_mi().command(command, timeout)
        return list(result.output)

    async def _console(self, command: str, timeout: float = 30.0) -> list[str]:
        chunks = await self._command(
            f"-interpreter-exec console {json.dumps(command)}", timeout=timeout
        )
        return "".join(chunks).splitlines()

    async def _next_event(self, timeout: float) -> MiRecord | None:
        record = await self._require_mi().next_event(timeout)
        if record is not None and record.record_type == "debugger-exit":
            payload = record.payload if isinstance(record.payload, dict) else {}
            raise DebuggerTransportError(f"GDB exited with {payload.get('returncode', 'unknown')}")
        return record

    def _require_mi(self) -> GdbMiProcess:
        if self._mi is None:
            raise DebuggerTransportError("GDB has not started")
        return self._mi
