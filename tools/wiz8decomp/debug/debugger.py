"""Live debugger orchestration and first-party crash interpretation."""

from __future__ import annotations

import json
import os
import subprocess
from pathlib import Path
from typing import Any

from ..binary.linker_map import SymbolResolution
from ..config import Settings
from ..display import runtime_display
from ..paths import atomic_json, sha256_file
from ..runtime import format_crash_candidates, runtime_test_environment
from ..subprocesses import tool_version
from .mi_process import DebuggerTransportError
from .session import CrashSnapshot, GdbSession, is_terminal_stop, terminal_stop_summary

DEBUG_ARTIFACT_PATTERNS = ("debugger-stop-*.txt",)
DEBUG_ARTIFACT_FILES = (
    "display.log",
    "gdb.log",
    "winedbg.log",
)


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


def _in_image(snapshot: CrashSnapshot, address: int) -> bool:
    return snapshot.image.base <= address < snapshot.image.base + snapshot.image.size


def _snapshot_candidates(snapshot: CrashSnapshot) -> list[tuple[str, int]]:
    """Collect only main-image candidates; foreign GDB frames never reach the MAP."""

    candidates: list[tuple[str, int]] = []
    seen: set[int] = set()

    def add(source: str, address: int) -> None:
        if address not in seen and _in_image(snapshot, address):
            seen.add(address)
            candidates.append((source, address))

    pc = snapshot.registers.get("eip")
    headers_end = snapshot.image.base + snapshot.image.headers_size
    pc_in_headers = pc is not None and snapshot.image.base <= pc < headers_end
    if pc is not None and not pc_in_headers:
        add("pc", pc)
    # x86 MZ starts with dec ebp; pop edx. After those bytes EDX may hold
    # the return address consumed from the stack, regardless of the original EBP.
    if (
        pc_in_headers
        and pc is not None
        and pc >= snapshot.image.base + 2
        and (address := snapshot.registers.get("edx")) is not None
    ):
        add("probable consumed return (edx)", address)
    for address in snapshot.frame_addresses:
        add("frame", address)
    for name in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"):
        if (address := snapshot.registers.get(name)) is not None:
            add(f"reg:{name}", address)
    esp = snapshot.registers.get("esp")
    for location, value in snapshot.stack_words:
        offset = location - esp if esp is not None else location
        add(f"stack+0x{offset:x}", value)
        if len(candidates) >= 32:
            break
    return candidates


def _prepare_artifact_dir(artifact_dir: Path) -> None:
    """Remove only debugger-owned projections from the previous session."""

    artifact_dir.mkdir(parents=True, exist_ok=True)
    for name in DEBUG_ARTIFACT_FILES:
        (artifact_dir / name).unlink(missing_ok=True)
    for pattern in DEBUG_ARTIFACT_PATTERNS:
        for path in artifact_dir.glob(pattern):
            path.unlink()


def _stop_debug_wineserver(environment: dict[str, str]) -> None:
    """Stop only the server belonging to the debugger's dedicated prefix."""

    subprocess.run(
        ["wineserver", "-k"],
        env=environment,
        check=False,
        capture_output=True,
        timeout=30,
    )


def _start_session(
    session: GdbSession,
    breakpoints: list[tuple[int, str | None]] | None,
) -> None:
    session.start(auto_continue=False)
    for address, condition in breakpoints or []:
        session.set_breakpoint(address, condition)
    session.continue_inferior()


def _debug_result(
    session: GdbSession,
    *,
    timeout: int,
    map_path: Path,
    manifest_path: Path,
    object_root: Path,
    display: str | None,
    provenance: Path,
) -> dict[str, Any]:
    event = session.wait_for_stop(timeout)
    timed_out = event is None
    if event is None:
        snapshot, event = session.interrupt_and_capture("timeout")
        if snapshot is None or event is None:
            raise DebuggerTransportError("runtime timed out and no debugger snapshot was captured")
    elif is_terminal_stop(event):
        reason, report = terminal_stop_summary(event, session.lifecycle())
        return {
            "report": report + "\n",
            "stopped": False,
            "reason": reason,
            "candidates": [],
            "unrecovered": None,
            "display": display or "host",
            "log": str(session.artifact_dir / "winedbg.log"),
            "session": str(provenance),
        }
    else:
        label = event.signal_name.lower() if event.signal_name else event.reason
        snapshot = session.capture_stop(label, event)

    report, resolutions = format_crash_snapshot(snapshot, map_path, object_root)
    stub = find_runtime_stub(resolutions, manifest_path)
    reason = "debugger timeout" if timed_out else event.signal_name or event.reason
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
        "stopped": True,
        "reason": reason,
        "candidates": [f"0x{item.address:08x}" for item in resolutions],
        "unrecovered": stub,
        "display": display or "host",
        "log": str(snapshot.raw_path),
        "session": str(provenance),
    }


def _file_identity(path: Path) -> dict[str, str | int]:
    return {"path": str(path), "size": path.stat().st_size, "sha256": sha256_file(path)}


def _write_provenance(executable: Path, map_path: Path, prefix: Path, artifact_dir: Path) -> Path:
    path = artifact_dir / "session.json"
    atomic_json(
        path,
        {
            "schema": "wiz8.debug-session",
            "executable": _file_identity(executable),
            "map": _file_identity(map_path) if map_path.is_file() else None,
            "wine_prefix": str(prefix),
            "wine": tool_version("wine"),
            "gdb": tool_version("gdb"),
        },
    )
    return path


def format_crash_snapshot(
    snapshot: CrashSnapshot, map_path: Path, object_root: Path | None = None
) -> tuple[str, list[SymbolResolution]]:
    """Add debugger context to the shared candidate-based crash report."""
    candidates = _snapshot_candidates(snapshot)
    detail, resolutions = (
        format_crash_candidates(map_path, object_root, candidates)
        if map_path.is_file()
        else ("", [])
    )
    signal = snapshot.event.signal_name or snapshot.event.reason
    lines = [
        signal if snapshot.fault_address is None else f"{signal} at 0x{snapshot.fault_address:08x}"
    ]
    pc = snapshot.registers.get("eip")
    if pc is not None:
        context = (
            "  PE image headers"
            if snapshot.image.base <= pc < snapshot.image.base + snapshot.image.headers_size
            else ""
        )
        lines.append(f"PC  0x{pc:08x}{context}")
    if detail:
        lines.extend(["", detail])
    lines.extend(["", f"raw debugger state: {snapshot.raw_path}"])
    return "\n".join(lines) + "\n", resolutions


def _debug_environment(
    settings: Settings, *, scenario: bool = False
) -> tuple[Path, dict[str, str]]:
    prefix = Path(
        os.environ.get("WIZ8_DEBUG_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-debug")
    )
    if scenario:
        return runtime_test_environment(settings, prefix=prefix)
    prefix.mkdir(parents=True, exist_ok=True)
    environment = {**os.environ, "WINEPREFIX": str(prefix)}
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
    """Run one persistent GDB/MI session and return its concise report."""

    from ..runtime import configure_wine_window_management, stage_game

    product_name = "Wiz8RuntimeTest.exe" if scenario is not None else "Wiz8Runtime.exe"
    staged = stage_game(
        settings,
        name=f"debug-{scenario}" if scenario is not None else "debug",
        executable=settings.product_build_dir / product_name,
        objects=settings.recovered_objects_dir,
        reset_saves=scenario is not None,
    )
    executable = staged.executable
    map_path = executable.with_suffix(".map")
    artifact_dir = settings.repo_dir / "build/debug"
    _prepare_artifact_dir(artifact_dir)
    manifest_path = settings.product_build_dir / "generated/runtime-stubs/runtime_stubs.json"
    prefix, environment = _debug_environment(settings, scenario=scenario is not None)
    _stop_debug_wineserver(environment)
    provenance = _write_provenance(executable, map_path, prefix, artifact_dir)

    try:
        with runtime_display(
            environment,
            default="virtual" if scenario is not None else "host",
            log_path=artifact_dir / "display.log",
        ) as display:
            if scenario is not None:
                configure_wine_window_management(environment, private_display=display is not None)
            session = GdbSession(
                executable,
                staged.root,
                environment,
                artifact_dir,
                arguments=(
                    ("--scenario", scenario)
                    if scenario is not None
                    else ("/WINDOW", *(arguments or []))
                ),
            )
            try:
                _start_session(session, breakpoints)
                return _debug_result(
                    session,
                    timeout=timeout,
                    map_path=map_path,
                    manifest_path=manifest_path,
                    object_root=settings.recovered_objects_dir,
                    display=display,
                    provenance=provenance,
                )
            finally:
                session.close()
    finally:
        _stop_debug_wineserver(environment)
