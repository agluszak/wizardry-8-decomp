"""Wine GDB proxy lifecycle and persistent GDB/MI supervision."""

from __future__ import annotations

import asyncio
import json
import re
import socket
import subprocess
import threading
import time
from collections import deque
from collections.abc import Coroutine
from concurrent.futures import Future
from dataclasses import dataclass
from pathlib import Path
from typing import TypeVar

from ..binary.pe import image_layout
from .mi_process import DebuggerTransportError, GdbMiProcess
from .mi_protocol import MiRecord

REPO_ROOT = Path(__file__).resolve().parents[3]
PROXY_START_TIMEOUT_SECONDS = 45.0
T = TypeVar("T")

__all__ = [
    "CrashSnapshot",
    "DebuggerLifecycle",
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
    signal_name: str | None
    breakpoint_number: str | None
    thread_id: str | None
    raw: str


@dataclass(frozen=True)
class DebuggerLifecycle:
    proxy_pid: int | None
    proxy_exit_code: int | None
    gdb_pid: int | None
    gdb_exit_code: int | None
    inferior_pid: int | None
    inferior_exit_code: int | None
    inferior_terminal_reason: str | None
    inferior_signal: str | None
    inferior_active: bool


@dataclass(frozen=True)
class ImageLayout:
    base: int
    size: int
    headers_size: int


@dataclass(frozen=True)
class CrashSnapshot:
    event: StopEvent
    registers: dict[str, int]
    frame_addresses: tuple[int, ...]
    stack_words: tuple[tuple[int, int], ...]
    fault_address: int | None
    image: ImageLayout
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
    breakpoint_number = record.payload.get("bkptno")
    thread_id = record.payload.get("thread-id")
    return StopEvent(
        reason=reason if isinstance(reason, str) else "stopped",
        signal_name=signal_name if isinstance(signal_name, str) else None,
        breakpoint_number=breakpoint_number if isinstance(breakpoint_number, str) else None,
        thread_id=thread_id if isinstance(thread_id, str) else None,
        raw=record.raw,
    )


def is_terminal_stop(event: StopEvent) -> bool:
    return event.reason in {"exited", "exited-normally", "exited-signalled"}


def terminal_stop_summary(event: StopEvent, lifecycle: DebuggerLifecycle) -> tuple[str, str]:
    """Classify a terminal inferior stop as (reason, report).

    Only a normal exit is success. ``exited`` carries the process exit code and
    ``exited-signalled`` the terminating signal, so a killed or crashed
    inferior is never reported as a normal exit.
    """
    if event.reason == "exited-normally":
        return "exited normally", "process exited normally"
    if event.reason == "exited-signalled":
        signal = lifecycle.inferior_signal or event.signal_name or "unknown"
        return f"terminated by signal {signal}", f"process terminated by signal {signal}"
    code = lifecycle.inferior_exit_code
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
    ) -> None:
        self.executable = executable
        self.cwd = cwd
        self.environment = environment
        self.port = allocate_port() if port is None else port
        self.log_path = log_path
        self.arguments = arguments
        self.process: subprocess.Popen[bytes] | None = None
        self._log = None

    def start(self) -> subprocess.Popen[bytes]:
        if not self.executable.is_file():
            raise DebuggerTransportError(f"missing debugger inferior {self.executable}")
        output = subprocess.DEVNULL
        if self.log_path is not None:
            self._log = self.log_path.open("wb")
            output = self._log
        self.process = subprocess.Popen(
            [
                "winedbg",
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
                return self.process
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


class _AsyncController:
    """Run the MI asyncio transport without imposing asyncio on the test runner."""

    def __init__(self) -> None:
        self.loop = asyncio.new_event_loop()
        self._ready = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._thread.start()
        self._ready.wait()

    def call(self, coroutine: Coroutine[object, object, T], timeout: float) -> T:
        future: Future[T] = asyncio.run_coroutine_threadsafe(coroutine, self.loop)
        return future.result(timeout)

    def close(self) -> None:
        self.loop.call_soon_threadsafe(self.loop.stop)
        self._thread.join(timeout=5)
        self.loop.close()

    def _run(self) -> None:
        asyncio.set_event_loop(self.loop)
        self._ready.set()
        self.loop.run_forever()


class GdbSession:
    """Persistent passive GDB observer for one Wine inferior."""

    def __init__(
        self,
        executable: Path,
        cwd: Path,
        environment: dict[str, str],
        artifact_dir: Path,
        arguments: tuple[str, ...] = (),
    ) -> None:
        self.executable = executable
        self.artifact_dir = artifact_dir
        self.proxy = WineGdbProxy(
            executable,
            cwd,
            environment,
            log_path=artifact_dir / "winedbg.log",
            arguments=arguments,
        )
        self._controller: _AsyncController | None = None
        self._mi: GdbMiProcess | None = None
        self._stops: deque[StopEvent] = deque()
        self._inferior_pid: int | None = None
        self._inferior_exit_code: int | None = None
        self._inferior_terminal_reason: str | None = None
        self._inferior_signal: str | None = None
        self._inferior_active = False
        self.stop_count = 0

    @property
    def process(self) -> subprocess.Popen[bytes]:
        if self.proxy.process is None:
            raise DebuggerTransportError("debugger proxy has not started")
        return self.proxy.process

    def start(self, auto_continue: bool = True) -> subprocess.Popen[bytes]:
        self.proxy.start()
        self._controller = _AsyncController()
        self._controller.start()
        self._mi = GdbMiProcess(REPO_ROOT, self.artifact_dir / "gdb.log")
        try:
            self._call(self._mi.start(self.executable), 15)
            self._command("-gdb-set pagination off")
            self._command("-gdb-set confirm off")
            self._command("-gdb-set mi-async on")
            self._command(f"-target-select remote localhost:{self.proxy.port}", timeout=45)
            self._console("handle SIGTRAP stop print nopass")
            self._console("handle SIGSEGV stop print pass")
            self._drain_events()
            self._stops.clear()
            if auto_continue:
                self.continue_inferior()
        except Exception:
            self.close()
            raise
        return self.process

    def lifecycle(self) -> DebuggerLifecycle:
        self._drain_events()
        proxy_process = self.proxy.process
        mi = self._mi
        return DebuggerLifecycle(
            proxy_pid=proxy_process.pid if proxy_process is not None else None,
            proxy_exit_code=proxy_process.poll() if proxy_process is not None else None,
            gdb_pid=mi.pid if mi is not None else None,
            gdb_exit_code=mi.returncode if mi is not None else None,
            inferior_pid=self._inferior_pid,
            inferior_exit_code=self._inferior_exit_code,
            inferior_terminal_reason=self._inferior_terminal_reason,
            inferior_signal=self._inferior_signal,
            inferior_active=self._inferior_active,
        )

    def continue_inferior(self) -> None:
        self._command("-exec-continue --all")

    def set_breakpoint(self, address: int, condition: str | None = None) -> str:
        prefix = f"-break-insert -c {json.dumps(condition)} " if condition else "-break-insert "
        result = self._call(self._require_mi().command(f"{prefix}*0x{address:08x}"), 32)
        payload = result.result.payload
        breakpoint = payload.get("bkpt") if isinstance(payload, dict) else None
        number = breakpoint.get("number") if isinstance(breakpoint, dict) else None
        if not isinstance(number, str):
            raise DebuggerTransportError(f"GDB returned no breakpoint number for 0x{address:08x}")
        return number

    def wait_for_stop(self, timeout: float) -> StopEvent | None:
        event = self.poll_stop()
        return event if event is not None else self._wait_for_stop(timeout)

    def poll_stop(self) -> StopEvent | None:
        self._drain_events()
        return self._stops.popleft() if self._stops else None

    def capture_stop(self, label: str, event: StopEvent) -> CrashSnapshot:
        self.stop_count += 1
        path = self.artifact_dir / f"debugger-stop-{self.stop_count:02d}-{label}.txt"
        sections = [("stop", [event.raw])]
        captured: dict[str, list[str]] = {}
        for heading, command in (
            ("all threads", "thread apply all bt 16"),
            ("registers", "info registers"),
            ("stack", "x/96wx $sp"),
            ("near pc", "x/24i $pc-24"),
        ):
            try:
                output = self._console(command, timeout=30)
            except DebuggerTransportError as error:
                output = [str(error)]
            captured[heading] = output
            sections.append((heading, output))
        registers = _parse_registers(captured.get("registers", []))
        fault_output: list[str] = []
        if event.signal_name == "SIGSEGV":
            try:
                fault_output = self._console(
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
        image = ImageLayout(*image_layout(self.executable))
        stack_words = _parse_stack_words(captured.get("stack", []))
        return CrashSnapshot(
            event=event,
            registers=registers,
            frame_addresses=_parse_frame_addresses(captured.get("all threads", [])),
            stack_words=stack_words,
            fault_address=fault_address,
            image=image,
            raw_path=path,
        )

    def interrupt_and_capture(self, label: str) -> tuple[CrashSnapshot | None, StopEvent | None]:
        event = self.poll_stop()
        if event is None:
            self._command("-exec-interrupt --all", timeout=5)
            event = self._wait_for_stop(2)
        if event is None:
            self._console("interrupt", timeout=5)
            event = self._wait_for_stop(10)
        if event is None:
            raise DebuggerTransportError(
                "GDB accepted interrupt requests but the Wine remote target did not stop"
            )
        capture_label = label
        if event.signal_name not in {None, "SIGINT"}:
            capture_label = event.signal_name.lower()  # type: ignore[union-attr]
        return self.capture_stop(capture_label, event), event

    def terminate_inferior(self) -> None:
        if not self._inferior_active:
            return
        try:
            self._console("kill", timeout=10)
        except DebuggerTransportError:
            pass
        self._drain_events()

    def close(self) -> None:
        if self._controller is not None and self._mi is not None:
            try:
                self._call(self._mi.close(), 12)
            except Exception:  # noqa: BLE001, S110 - shutdown must not mask the original error
                pass
        if self._controller is not None:
            self._controller.close()
        self._mi = None
        self._controller = None
        self.proxy.close()

    def _command(self, command: str, timeout: float = 30.0) -> list[str]:
        mi = self._require_mi()
        result = self._call(mi.command(command, timeout), timeout + 2)
        return list(result.output)

    def _console(self, command: str, timeout: float = 30.0) -> list[str]:
        chunks = self._command(f"-interpreter-exec console {json.dumps(command)}", timeout=timeout)
        return "".join(chunks).splitlines()

    def _drain_events(self) -> None:
        records = self._call(self._require_mi().drain_events(), 2)
        for record in records:
            self._record_event(record)

    def _wait_for_stop(self, timeout: float) -> StopEvent | None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            record = self._call(
                self._require_mi().next_event(deadline - time.monotonic()),
                deadline - time.monotonic() + 1,
            )
            if record is None:
                return None
            self._record_event(record)
            if self._stops:
                return self._stops.popleft()
        return None

    def _record_event(self, record: MiRecord) -> None:
        event = stop_event_from_record(record)
        if event is not None:
            if is_terminal_stop(event):
                self._inferior_active = False
                self._inferior_terminal_reason = event.reason
                self._inferior_signal = event.signal_name
                payload = record.payload if isinstance(record.payload, dict) else {}
                self._inferior_exit_code = _parse_exit_code(payload.get("exit-code"))
            self._stops.append(event)
        elif record.record_type == "notify" and isinstance(record.payload, dict):
            if record.message == "thread-group-started":
                self._inferior_pid = _parse_exit_code(record.payload.get("pid"))
                self._inferior_active = True
            elif record.message == "thread-group-exited":
                self._inferior_active = False
                self._inferior_terminal_reason = "thread-group-exited"
                self._inferior_exit_code = _parse_exit_code(record.payload.get("exit-code"))
        elif record.record_type == "debugger-exit":
            payload = record.payload if isinstance(record.payload, dict) else {}
            raise DebuggerTransportError(f"GDB exited with {payload.get('returncode', 'unknown')}")

    def _call(self, coroutine: Coroutine[object, object, T], timeout: float) -> T:
        if self._controller is None:
            coroutine.close()
            raise DebuggerTransportError("GDB controller has not started")
        try:
            return self._controller.call(coroutine, timeout)
        except DebuggerTransportError:
            raise
        except Exception as error:
            raise DebuggerTransportError(str(error)) from error

    def _require_mi(self) -> GdbMiProcess:
        if self._mi is None:
            raise DebuggerTransportError("GDB has not started")
        return self._mi
