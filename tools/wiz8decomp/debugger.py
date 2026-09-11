"""Deterministic GDB driver for the runnable Wizardry product.

The old debugging path fed ``cont``/``quit`` to interactive WineDbg. That has
no reliable stop policy: the queued commands execute at whatever stop comes
first, and a trap can be consumed before anything is captured. This driver
launches Wine's GDB proxy explicitly, waits for its port, connects system GDB
with a deterministic stop policy, captures the normal stack, then symbolizes
Wizardry addresses through the runtime linker MAP and maps a generated trap
back to its retail identity through the stub manifest.
"""

from __future__ import annotations

import json
import os
import re
import signal
import socket
import subprocess
import time
from pathlib import Path
from typing import Any

from .config import Settings
from .display import runtime_display

RUNTIME_DIR = Path("build/decomp")
RUNTIME_MAP = RUNTIME_DIR / "Wiz8Runtime.map"
STUB_MANIFEST = RUNTIME_DIR / "generated/runtime-stubs/runtime_stubs.json"
GDB_TIMEOUT_SECONDS = 180
PROXY_TIMEOUT_SECONDS = 30
FRAME = re.compile(
    r"^#(?P<index>\d+)\s+(?:0x(?P<address>[0-9a-fA-F]+)\s+in\s+)?(?P<name>.*?)\s*(?:\(.*\))?$"
)
THREAD_HEADER = re.compile(r"^Thread (?P<id>\d+)\s+\(Thread [^)]*\):\s*$", re.MULTILINE)
STOPPED_THREAD = re.compile(
    r"^Thread (?P<id>\d+)\b.*?(?:hit Breakpoint|received signal)", re.MULTILINE
)
GDB_SCRIPT = """set pagination off
set confirm off
set width 0
set height 0
target remote localhost:{port}
handle SIGTRAP stop print nopass
handle SIGSEGV stop print pass
{extra}continue
echo \\n=== BACKTRACE ===\\n
thread apply all bt full
echo \\n=== REGISTERS ===\\n
info registers
echo \\n=== SHARED LIBRARIES ===\\n
info sharedlibrary
echo \\n=== CODE ===\\n
x/32i $pc-32
echo \\n=== STACK ===\\n
x/128wx $sp
echo \\n=== END ===\\n
quit
"""


def _free_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
        probe.bind(("127.0.0.1", 0))
        return int(probe.getsockname()[1])


def _port_listening(port: int) -> bool:
    """Read /proc directly so probing never consumes the proxy's connection."""

    path = Path("/proc/net/tcp")
    if not path.is_file():
        return False
    needle = f"{port:04X}"
    for line in path.read_text().splitlines()[1:]:
        fields = line.split()
        if len(fields) < 4:
            continue
        local = fields[1].split(":")
        if len(local) == 2 and local[1].upper() == needle and fields[3] == "0A":
            return True
    return False


def _wait_for_proxy(process: subprocess.Popen[str], port: int) -> None:
    started = time.monotonic()
    while time.monotonic() - started < PROXY_TIMEOUT_SECONDS:
        if _port_listening(port):
            return
        if process.poll() is not None:
            stdout, _ = process.communicate()
            raise RuntimeError(f"winedbg exited before its GDB port opened:\n{stdout}")
        time.sleep(0.1)
    raise RuntimeError(f"winedbg did not open its GDB port {port} within {PROXY_TIMEOUT_SECONDS}s")


def _threads(output: str) -> list[tuple[str, list[tuple[int, str]]]]:
    """Frames grouped by the `thread apply all bt full` headers."""

    threads: list[tuple[str, list[tuple[int, str]]]] = []
    current: tuple[str, list[tuple[int, str]]] | None = None
    for line in output.splitlines():
        header = THREAD_HEADER.match(line)
        if header is not None:
            current = (header.group("id"), [])
            threads.append(current)
            continue
        match = FRAME.match(line)
        if match is None or match.group("address") is None:
            continue
        if current is None:
            current = ("1", [])
            threads.append(current)
        current[1].append((int(match.group("address"), 16), match.group("name")))
    return threads


def _stopped_thread(output: str, threads: list[tuple[str, list[tuple[int, str]]]]) -> str:
    match = STOPPED_THREAD.search(output)
    if match is not None:
        for thread_id, _frames in threads:
            if thread_id == match.group("id"):
                return thread_id
    return threads[0][0] if threads else ""


def _stub_identity(manifest_path: Path, frames: list[tuple[int, str]]) -> dict[str, Any] | None:
    if not manifest_path.is_file():
        return None
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    by_stub = {
        str(entry.get("stub", "")).lstrip("_"): entry
        for entry in manifest.get("stubs", [])
        if entry.get("stub")
    }
    for _address, name in frames:
        clean = name.split("(")[0].strip().lstrip("_")
        entry = by_stub.get(clean)
        if entry is not None:
            return entry
    return None


def _symbolize(map_path: Path, frames: list[tuple[int, str]]) -> list[str]:
    from .runtime import _resolve_addresses

    resolved = _resolve_addresses(map_path, [address for address, _ in frames])
    lines: list[str] = []
    for (address, name), item in zip(frames, resolved):
        gdb_name = name.split("(")[0].strip() or "??"
        if item is None:
            lines.append(f"#{len(lines)} 0x{address:08x} {gdb_name}")
            continue
        location = f" {item.location}" if item.location else ""
        lines.append(
            f"#{len(lines)} 0x{address:08x} {item.symbol}+0x{item.displacement:x} "
            f"[{item.owner}]{location}"
        )
    return lines


def _report(
    stubs: dict[str, Any] | None,
    thread_id: str,
    symbolized: list[str],
) -> str:
    lines: list[str] = []
    if stubs is not None:
        lines.append("UNRECOVERED FUNCTION")
        lines.append(f"  retail: {stubs.get('address') or 'unmapped'}")
        lines.append(f"  symbol: {stubs.get('symbol')}")
        lines.append(f"  stub:   {stubs.get('stub')}")
        lines.append("")
    lines.append(f"THREAD {thread_id}")
    lines.extend(symbolized)
    if not symbolized:
        lines.append("  no frames captured")
    return "\n".join(lines) + "\n"


def _text(value: str | bytes | None) -> str:
    if isinstance(value, bytes):
        return value.decode(errors="replace")
    return value or ""


def run_debugger(
    settings: Settings,
    arguments: list[str] | None = None,
    *,
    extra_gdb: str | None = None,
    timeout: int = GDB_TIMEOUT_SECONDS,
) -> dict[str, Any]:
    """Run one deterministic GDB session and return its symbolized report."""

    from .runtime import stage_game

    staged = stage_game(
        settings,
        name="debug",
        executable=settings.product_build_dir / "Wiz8Runtime.exe",
        objects=settings.recovered_objects_dir,
    )
    game_dir = staged.root
    run_dir = game_dir
    managed = staged.executable
    if extra_gdb is None:
        extra_gdb = os.environ.get("WIZ8_DEBUG_GDB", "")
    map_path = settings.repo_dir / RUNTIME_MAP
    manifest_path = settings.repo_dir / STUB_MANIFEST
    gdb = "gdb"
    port = _free_port()
    gdb_script = settings.repo_dir / "build/debug/gdb-commands.txt"
    gdb_script.parent.mkdir(parents=True, exist_ok=True)
    extra = "" if not extra_gdb else extra_gdb.rstrip("\n") + "\n"
    gdb_script.write_text(GDB_SCRIPT.format(port=port, extra=extra), encoding="utf-8")
    command = [
        "winedbg",
        "--gdb",
        "--no-start",
        "--port",
        str(port),
        f"./{managed.name}",
        "/WINDOW",
        *(arguments or []),
    ]
    environment = dict(os.environ)
    subprocess.run(["wineserver", "-k"], env=environment, check=False, capture_output=True)
    time.sleep(1)
    with runtime_display(
        environment, default="host", log_path=settings.repo_dir / "build/debug/display.log"
    ) as display:
        proxy = subprocess.Popen(
            command,
            cwd=run_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            start_new_session=True,
            env=environment,
        )
        try:
            _wait_for_proxy(proxy, port)
            completed = subprocess.run(
                [gdb, "-q", "-batch", "-x", str(gdb_script)],
                cwd=run_dir,
                capture_output=True,
                text=True,
                timeout=timeout,
                check=False,
                env=environment,
            )
            output = completed.stdout + completed.stderr
        except subprocess.TimeoutExpired as error:
            output = _text(error.stdout) + _text(error.stderr)
            output += "\n[debugger session timed out]\n"
        finally:
            subprocess.run(
                ["wineserver", "-k"],
                cwd=game_dir,
                env=environment,
                check=False,
                capture_output=True,
            )
            if proxy.poll() is None:
                try:
                    os.killpg(proxy.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    proxy.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    try:
                        os.killpg(proxy.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
    frames_by_thread = _threads(output)
    thread_id = _stopped_thread(output, frames_by_thread)
    frames = next((frames for ident, frames in frames_by_thread if ident == thread_id), [])
    stopped = bool(re.search(r"hit Breakpoint \d+", output)) or "received signal" in output
    reason = ""
    signal_match = re.search(r"received signal (SIG\w+)", output)
    breakpoint_match = re.search(r"hit Breakpoint (\d+)", output)
    if signal_match:
        reason = signal_match.group(1)
    elif breakpoint_match:
        reason = f"breakpoint {breakpoint_match.group(1)}"
    elif re.search(r"exited normally", output):
        reason = "exited normally"
    elif re.search(r"exited with code", output):
        reason = "exited"
    symbolized = _symbolize(map_path, frames) if map_path.is_file() else []
    stubs = _stub_identity(manifest_path, frames)
    report = _report(stubs, thread_id, symbolized)
    session_log = settings.repo_dir / "build/debug/session.txt"
    session_log.parent.mkdir(parents=True, exist_ok=True)
    session_log.write_text(report + "\n--- raw gdb ---\n" + output, encoding="utf-8")
    return {
        "report": report,
        "stopped": stopped,
        "reason": reason,
        "frames": [f"0x{address:08x}" for address, _ in frames],
        "unrecovered": stubs,
        "display": display or "host",
        "log": str(session_log),
        "raw_output": output,
    }
