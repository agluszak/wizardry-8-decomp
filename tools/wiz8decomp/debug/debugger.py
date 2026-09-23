"""Live debugger orchestration and first-party crash interpretation."""

from __future__ import annotations

import asyncio
import json
import os
import shutil
import subprocess
from fcntl import LOCK_EX, LOCK_NB, flock
from pathlib import Path
from typing import Any

from ..binary.linker_map import SymbolResolution
from ..binary.pe import image_layout
from ..config import Settings
from ..display import runtime_display
from ..paths import atomic_json, sha256_file
from ..runtime import (
    apply_product_video_config,
    configure_runtime_runner,
    configure_wine_window_management,
    format_crash_candidates,
    require_umu_runner,
    runtime_runner,
    runtime_test_environment,
    stage_game,
)
from .mi_process import DebuggerTransportError
from .session import CrashSnapshot, GdbSession, is_terminal_stop, terminal_stop_summary


def _load_manifest(manifest_path: Path) -> dict[str, dict[str, Any]]:
    if not manifest_path.is_file():
        return {}
    document = json.loads(manifest_path.read_text(encoding="utf-8"))
    by_stub: dict[str, dict[str, Any]] = {}
    for entry in document.get("stubs", []):
        stub = str(entry.get("stub", ""))
        if stub:
            by_stub[stub] = entry
            by_stub[stub.lstrip("_")] = entry
    return by_stub


def find_runtime_stub(
    resolutions: list[SymbolResolution], manifest_path: Path
) -> dict[str, Any] | None:
    """Identify a generated trap through the MAP, never through GDB names."""

    by_stub = _load_manifest(manifest_path)
    for resolution in resolutions:
        symbol = resolution.symbol
        if symbol is not None and (entry := by_stub.get(symbol.decorated_name)) is not None:
            return entry
    return None


def _snapshot_candidates(snapshot: CrashSnapshot) -> list[tuple[str, int]]:
    """Collect plausible code addresses for section-aware MAP resolution."""

    candidates: list[tuple[str, int]] = []
    seen: set[int] = set()

    def add(source: str, address: int) -> None:
        if address not in seen:
            seen.add(address)
            candidates.append((source, address))

    pc = snapshot.registers.get("eip")
    if pc is not None:
        add("pc", pc)
    for address in snapshot.frame_addresses:
        add("frame", address)
    for name in ("edx", "ecx", "eax", "ebx", "esi", "edi", "ebp"):
        if (address := snapshot.registers.get(name)) is not None:
            add(f"reg:{name}", address)
    esp = snapshot.registers.get("esp")
    for location, value in snapshot.stack_words:
        offset = location - esp if esp is not None else location
        add(f"stack+0x{offset:x}", value)
    return candidates


def _prepare_artifact_dir(artifact_dir: Path) -> None:
    shutil.rmtree(artifact_dir, ignore_errors=True)
    artifact_dir.mkdir(parents=True, exist_ok=True)


def _stop_debug_wineserver(environment: dict[str, str], runner: str) -> None:
    """Stop only the server belonging to the debugger's dedicated prefix."""

    wineserver = "wineserver" if runner == "wine" else os.environ.get("WIZ8_UMU_WINESERVER")
    if not wineserver or shutil.which(wineserver) is None:
        raise RuntimeError(f"wineserver is not available for {runner}")
    subprocess.run(
        [wineserver, "-k"],
        env=environment,
        check=False,
        capture_output=True,
        timeout=30,
    )


async def _debug_result(
    session: GdbSession,
    *,
    timeout: int,
    map_path: Path | None,
    manifest_path: Path,
    provenance: Path,
) -> dict[str, Any]:
    event = await session.wait_for_stop(timeout)
    timed_out = event is None
    snapshot = None
    if event is None:
        snapshot, event = await session.interrupt_and_capture("timeout")
        if event is None:
            raise DebuggerTransportError("runtime timed out and no debugger snapshot was captured")
    if is_terminal_stop(event):
        reason, report = terminal_stop_summary(event)
        return {
            "report": report + "\n",
            "reason": reason,
            "exit_code": (
                0
                if event.reason == "exited-normally"
                else event.exit_code
                if event.reason == "exited" and event.exit_code is not None
                else 1
            ),
            "log": str(session.artifact_dir / "winedbg.log"),
            "session": str(provenance),
        }
    if snapshot is None:
        label = event.signal.lower() if event.signal else event.reason
        snapshot = await session.capture_stop(label, event)

    report, resolutions = format_crash_snapshot(snapshot, session.executable, map_path)
    causal_addresses = set(snapshot.frame_addresses)
    if (pc := snapshot.registers.get("eip")) is not None:
        causal_addresses.add(pc)
    stub = (
        find_runtime_stub(
            [item for item in resolutions if item.address in causal_addresses], manifest_path
        )
        if event.signal == "SIGTRAP"
        else None
    )
    reason = "debugger timeout" if timed_out else event.signal or event.reason
    if stub is not None:
        report = (
            "UNRECOVERED FUNCTION\n"
            f"  retail: {stub.get('address') or 'unmapped'}\n"
            f"  symbol: {stub.get('symbol')}\n"
            f"  stub:   {stub.get('stub')}\n\n" + report
        )
        reason = f"unrecovered {stub.get('address') or 'unmapped'}"
    return {
        "report": report,
        "reason": reason,
        "exit_code": 0
        if event.reason == "breakpoint-hit" and not timed_out and stub is None
        else 1,
        "log": str(snapshot.raw_path),
        "session": str(provenance),
    }


async def _run_debug_session(
    session: GdbSession,
    breakpoints: list[tuple[int, str | None]] | None,
    *,
    timeout: int,
    map_path: Path | None,
    manifest_path: Path,
    provenance: Path,
) -> dict[str, Any]:
    try:
        await session.start()
        for address, condition in breakpoints or []:
            await session.set_breakpoint(address, condition)
        await session.continue_inferior()
        return await _debug_result(
            session,
            timeout=timeout,
            map_path=map_path,
            manifest_path=manifest_path,
            provenance=provenance,
        )
    finally:
        await session.close()


def _file_identity(path: Path) -> dict[str, str]:
    return {"path": str(path), "sha256": sha256_file(path)}


def _write_provenance(
    executable: Path,
    map_path: Path | None,
    prefix: Path,
    artifact_dir: Path,
    invocation: dict[str, Any],
) -> Path:
    path = artifact_dir / "session.json"
    atomic_json(
        path,
        {
            "executable": _file_identity(executable),
            "invocation": invocation,
            "map": _file_identity(map_path) if map_path is not None else None,
            "wine_prefix": str(prefix),
        },
    )
    return path


def format_crash_snapshot(
    snapshot: CrashSnapshot, executable: Path, map_path: Path | None
) -> tuple[str, list[SymbolResolution]]:
    """Add debugger context to the shared candidate-based crash report."""
    candidates = _snapshot_candidates(snapshot)
    detail, resolutions = (
        format_crash_candidates(map_path, None, candidates)
        if map_path is not None and map_path.is_file()
        else ("", [])
    )
    signal = snapshot.event.signal or snapshot.event.reason
    lines = [
        signal if snapshot.fault_address is None else f"{signal} at 0x{snapshot.fault_address:08x}"
    ]
    pc = snapshot.registers.get("eip")
    if pc is not None:
        image_base, _, headers_size = image_layout(executable)
        context = "  PE image headers" if image_base <= pc < image_base + headers_size else ""
        lines.append(f"PC  0x{pc:08x}{context}")
    if detail:
        lines.extend(["", detail])
    lines.extend(["", f"raw debugger state: {snapshot.raw_path}"])
    return "\n".join(lines) + "\n", resolutions


def _debug_environment(
    settings: Settings, *, scenario: bool = False, runner: str = "wine"
) -> tuple[Path, dict[str, str]]:
    prefix = Path(
        os.environ.get(
            "WIZ8_DEBUG_WINE_PREFIX",
            settings.work_dir / "wine" / ("wiz8-ge-debug" if runner == "umu" else "wiz8-debug"),
        )
    )
    if scenario:
        return runtime_test_environment(settings, prefix=prefix, sound=runner == "umu")
    prefix.mkdir(parents=True, exist_ok=True)
    environment = {**os.environ, "WINEPREFIX": str(prefix)}
    configure_runtime_runner(environment, runner)
    environment.setdefault("WINEDLLOVERRIDES", "winemenubuilder.exe=d")
    return prefix, environment


def run_debugger(
    settings: Settings,
    arguments: list[str] | None = None,
    *,
    timeout: int = 180,
    breakpoints: list[tuple[int, str | None]] | None = None,
    scenario: str | None = None,
) -> dict[str, Any]:
    """Exclusively own the checkout's debugger prefix and artifacts."""
    lock_path = settings.repo_dir / "build/.wiz8-debug.lock"
    lock_path.parent.mkdir(parents=True, exist_ok=True)
    with lock_path.open("a+", encoding="utf-8") as lock:
        try:
            flock(lock.fileno(), LOCK_EX | LOCK_NB)
        except BlockingIOError as error:
            lock.seek(0)
            raise DebuggerTransportError(
                f"debugger session already running: {lock.read().strip()}"
            ) from error
        lock.seek(0)
        lock.truncate()
        json.dump({"pid": os.getpid(), "scenario": scenario}, lock)
        lock.flush()
        return _run_debugger_locked(
            settings, arguments, timeout=timeout, breakpoints=breakpoints, scenario=scenario
        )


def _run_debugger_locked(
    settings: Settings,
    arguments: list[str] | None = None,
    *,
    timeout: int = 180,
    breakpoints: list[tuple[int, str | None]] | None = None,
    scenario: str | None = None,
) -> dict[str, Any]:
    """Run one persistent GDB/MI session and return its concise report."""

    product_name = "Wiz8RuntimeTest.exe" if scenario is not None else "Wiz8Runtime.exe"
    staged = stage_game(
        settings,
        name=f"debug-{scenario}" if scenario is not None else "debug",
        executable=settings.product_build_dir / product_name,
        objects=settings.recovered_objects_dir,
        reset_saves=scenario is not None,
    )
    runner = runtime_runner()
    if scenario is None or runner == "umu":
        apply_product_video_config(settings, staged.root)
    runner_environment = dict(os.environ)
    configure_runtime_runner(runner_environment, runner)
    umu_run = require_umu_runner(runner_environment) if runner == "umu" else ""
    executable = staged.executable
    map_path = staged.map
    artifact_dir = settings.repo_dir / "build/debug"
    manifest_path = settings.product_build_dir / "generated/runtime-stubs/runtime_stubs.json"
    prefix, environment = _debug_environment(settings, scenario=scenario is not None, runner=runner)
    _stop_debug_wineserver(environment, runner)
    _prepare_artifact_dir(artifact_dir)
    launch_arguments = (
        ("--scenario", scenario) if scenario is not None else ("/WINDOW", *(arguments or []))
    )
    provenance = _write_provenance(
        executable,
        map_path,
        prefix,
        artifact_dir,
        {
            "scenario": scenario,
            "arguments": list(launch_arguments),
            "breakpoints": breakpoints or [],
            "timeout": timeout,
            "runner": runner,
        },
    )

    try:
        with runtime_display(
            environment,
            default="virtual" if scenario is not None else "host",
            log_path=artifact_dir / "display.log",
        ) as display:
            if runner == "wine":
                configure_wine_window_management(environment, private_display=display is not None)
            session = GdbSession(
                executable,
                staged.root,
                environment,
                artifact_dir,
                arguments=launch_arguments,
                launch_command=(umu_run, "winedbg.exe") if runner == "umu" else ("winedbg",),
            )
            return asyncio.run(
                _run_debug_session(
                    session,
                    breakpoints,
                    timeout=timeout,
                    map_path=map_path,
                    manifest_path=manifest_path,
                    provenance=provenance,
                )
            )
    finally:
        _stop_debug_wineserver(environment, runner)
