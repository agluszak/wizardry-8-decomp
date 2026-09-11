"""Map a captured GDB report's frame addresses through the linker MAP."""

from __future__ import annotations

from pathlib import Path

from ..binary.linker_map import FRAME_RE, LinkerMap, SymbolResolution, demangle_names


def _resolve_frames(report_path: Path, linker_map: LinkerMap) -> list[tuple[int, SymbolResolution]]:
    frames: list[tuple[int, SymbolResolution]] = []
    for line in report_path.read_text(encoding="utf-8").splitlines():
        match = FRAME_RE.match(line)
        if not match:
            continue
        address = int(match.group(2), 16)
        frames.append((int(match.group(1)), linker_map.resolve(address)))
    return frames


def resolve_gdb_report(
    report_path: Path,
    map_path: Path,
) -> list[tuple[int, SymbolResolution]]:
    return _resolve_frames(report_path, LinkerMap.read(map_path))


def symbolize_gdb_report(report_path: Path, map_path: Path) -> Path | None:
    if not map_path.is_file():
        return None
    linker_map = LinkerMap.read(map_path)
    frames = _resolve_frames(report_path, linker_map)
    names = list(
        dict.fromkeys(
            resolution.symbol.decorated_name
            for _, resolution in frames
            if resolution.symbol is not None
        )
    )
    demangled = demangle_names(names)
    output_path = report_path.with_name(
        report_path.name.replace("debugger-stop-", "symbolized-stack-")
    )
    with output_path.open("w", encoding="utf-8") as output:
        for frame, resolution in frames:
            address = resolution.address
            symbol = resolution.symbol
            if symbol is None:
                output.write(
                    f"#{frame} 0x{address:08x} <unresolved:{resolution.confidence}> "
                    f"{resolution.reason}\n"
                )
                continue
            name = demangled.get(symbol.decorated_name, symbol.decorated_name)
            output.write(
                f"#{frame} 0x{address:08x} {name}+0x{address - symbol.address:x} "
                f"[{symbol.object_name}]\n"
            )
    return output_path
