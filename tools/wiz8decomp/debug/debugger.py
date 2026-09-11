"""Thin live-debugger orchestration over the persistent GDB/MI transport."""

from __future__ import annotations

import json
import os
from pathlib import Path
from typing import Any

from ..config import Settings
from ..display import runtime_display
from .mi_process import DebuggerTransportError
from .session import GdbSession, is_terminal_stop
from .symbols import SymbolResolution, resolve_gdb_report


def _load_manifest(manifest_path: Path) -> dict[str, dict[str, Any]]:
    if not manifest_path.is_file():
        return {}
    document = json.loads(manifest_path.read_text(encoding="utf-8"))
    by_stub: dict[str, dict[str, Any]] = {}
    for entry in document.get("stubs", []):
        stub = str(entry.get("stub", ""))
        if not stub:
            continue
        by_stub[stub] = entry
        by_stub[stub.lstrip("_")] = entry
    return by_stub


def find_runtime_stub(
    frames: list[tuple[int, SymbolResolution]],
    manifest_path: Path,
) -> dict[str, Any] | None:
    """Identify a generated trap through the MAP, never through GDB names."""
    by_stub = _load_manifest(manifest_path)
    if not by_stub:
        return None
    for _frame, resolution in frames:
        symbol = resolution.symbol
        if symbol is None:
            continue
        entry = by_stub.get(symbol.decorated_name)
        if entry is not None:
            return entry
    return None


def _demangled_names(symbols: list[str]) -> dict[str, str]:
    unique = sorted({name for name in symbols if name.startswith("?")})
    if not unique:
        return {}
    from ..binary.demangle import DemanglerMissing, demangle

    try:
        return demangle(unique)
    except (DemanglerMissing, RuntimeError):
        return {}


def _format_frames(frames: list[tuple[int, SymbolResolution]]) -> list[str]:
    names = [
        resolution.symbol.decorated_name
        for _, resolution in frames
        if resolution.symbol is not None
    ]
    demangled = _demangled_names(names)
    lines: list[str] = []
    for frame, resolution in frames:
        address = resolution.address
        symbol = resolution.symbol
        if symbol is None:
            lines.append(f"#{frame} 0x{address:08x} <unresolved:{resolution.confidence}>")
            continue
        name = demangled.get(symbol.decorated_name, symbol.decorated_name)
        lines.append(
            f"#{frame} 0x{address:08x} {name}+0x{address - symbol.address:x} [{symbol.object_name}]"
        )
    return lines


def run_debugger(
    settings: Settings,
    arguments: list[str] | None = None,
    *,
    timeout: int = 180,
) -> dict[str, Any]:
    """Run one persistent GDB/MI session and return its concise report."""
    from ..runtime import stage_game

    staged = stage_game(
        settings,
        name="debug",
        executable=settings.product_build_dir / "Wiz8Runtime.exe",
        objects=settings.recovered_objects_dir,
    )
    game_dir = staged.root
    executable = staged.executable
    artifact_dir = settings.repo_dir / "build/debug"
    artifact_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = settings.product_build_dir / "generated/runtime-stubs/runtime_stubs.json"
    environment = dict(os.environ)

    with runtime_display(
        environment, default="host", log_path=artifact_dir / "display.log"
    ) as display:
        session = GdbSession(
            executable,
            game_dir,
            environment,
            artifact_dir,
            arguments=("/WINDOW", *(arguments or [])),
        )
        try:
            session.start()
            event = session.wait_for_stop(timeout)
            if event is None:
                raise DebuggerTransportError("runtime did not stop or exit")
            if is_terminal_stop(event):
                report = "process exited normally\n"
                return {
                    "report": report,
                    "stopped": False,
                    "reason": "exited normally",
                    "frames": [],
                    "unrecovered": None,
                    "display": display or "host",
                    "log": str(artifact_dir / "winedbg.log"),
                    "symbolized": None,
                }
            label = event.signal_name.lower() if event.signal_name else event.reason
            raw = session.capture_stop(label, event)
            map_path = executable.with_suffix(".map")
            frames = resolve_gdb_report(raw, map_path) if map_path.is_file() else []
            stub = find_runtime_stub(frames, manifest_path) if frames else None
            symbolized_path = raw.with_name(raw.name.replace("debugger-stop-", "symbolized-stack-"))
            symbolized = str(symbolized_path) if symbolized_path.is_file() else None
            frame_lines = _format_frames(frames)
            addresses = [f"0x{resolution.address:08x}" for _, resolution in frames]

            if stub is not None:
                lines = [
                    "UNRECOVERED FUNCTION",
                    f"  retail: {stub.get('address') or 'unmapped'}",
                    f"  symbol: {stub.get('symbol')}",
                    f"  stub:   {stub.get('stub')}",
                    "",
                    *frame_lines,
                    f"raw debugger state: {raw.relative_to(settings.repo_dir)}",
                ]
                reason = f"unrecovered {stub.get('address') or 'unmapped'}"
            elif event.signal_name:
                lines = [
                    event.signal_name,
                    "",
                    *frame_lines,
                    f"raw debugger state: {raw.relative_to(settings.repo_dir)}",
                ]
                reason = event.signal_name
            else:
                lines = [
                    event.reason,
                    "",
                    *frame_lines,
                    f"raw debugger state: {raw.relative_to(settings.repo_dir)}",
                ]
                reason = event.reason
            report = "\n".join(lines) + "\n"
            return {
                "report": report,
                "stopped": True,
                "reason": reason,
                "frames": addresses,
                "unrecovered": stub,
                "display": display or "host",
                "log": str(raw),
                "symbolized": symbolized,
            }
        finally:
            session.close()
