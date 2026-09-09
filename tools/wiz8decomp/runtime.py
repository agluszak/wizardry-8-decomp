from __future__ import annotations

import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .binary.demangle import DemanglerMissing, demangle
from .config import Settings
from .display import runtime_display


def _managed_link(source: Path, destination: Path) -> None:
    if destination.is_symlink():
        if destination.resolve() != source.resolve():
            raise RuntimeError(f"runtime symlink points at the wrong source: {destination}")
        return
    if destination.exists():
        raise RuntimeError(f"runtime staging path is not a managed symlink: {destination}")
    destination.symlink_to(source, target_is_directory=source.is_dir())


RUNTIME_OBSERVATION = re.compile(r"^WIZ8_RUNTIME_TEST (?P<fields>.+)$")
RUNTIME_CRASH = re.compile(r"^WIZ8_RUNTIME_CRASH (?P<fields>.+)$", re.MULTILINE)
RUNTIME_STACK_CANDIDATE = re.compile(
    r"^runtime-test stack-candidate: .* address=(?P<address>[0-9a-fA-F]+)", re.MULTILINE
)
MAP_SECTION = re.compile(
    r"^\s+(?P<segment>[0-9a-fA-F]{4}):(?P<offset>[0-9a-fA-F]{8})\s+"
    r"(?P<length>[0-9a-fA-F]{8})H\s+\S+\s+\S+\s*$"
)
MAP_FUNCTION = re.compile(
    r"^\s+(?P<segment>[0-9a-fA-F]{4}):(?P<offset>[0-9a-fA-F]{8})\s+(?P<symbol>\S+)\s+"
    r"(?P<address>[0-9a-fA-F]{8})\s+f(?:\s+i)?\s+(?P<object>.+?)\s*$"
)
MAP_LINE_HEADER = re.compile(r"^Line numbers for .*\((?P<source>.+)\) segment ")
MAP_LINE = re.compile(r"(?P<line>[0-9]+)\s+(?P<segment>[0-9a-fA-F]{4}):(?P<offset>[0-9a-fA-F]{8})")
RUNTIME_SCENARIOS = (
    "main-menu-startup",
    "main-menu-new-game",
    "main-menu-exit-auto-repeat",
)


@dataclass(frozen=True)
class _MapSection:
    segment: str
    start: int
    end: int


@dataclass(frozen=True)
class _MapFunction:
    address: int
    segment: str
    offset: int
    symbol: str
    owner: str


def stage_runtime_test(settings: Settings) -> dict[str, Any]:
    source = settings.work_dir / "variants" / "gog-base"
    stage = settings.repo_dir / "build" / "runtime" / "wiz8"
    executable_name = "Wiz8RuntimeTest.exe"
    executable = settings.repo_dir / "build" / "decomp" / executable_name
    for name in ("Data", "Dll", "Levels"):
        if not (source / name).is_dir():
            raise RuntimeError(f"missing retail asset directory: {source / name}")
    if not executable.is_file():
        raise RuntimeError(f"runtime executable is not built: {executable}")
    stage.mkdir(parents=True, exist_ok=True)
    (stage / "Saves").mkdir(exist_ok=True)
    for name in ("Data", "Dll", "Levels", "Patches"):
        candidate = source / name
        if candidate.exists():
            _managed_link(candidate, stage / name)
    for candidate in sorted(path for path in source.iterdir() if path.is_file()):
        if candidate.name in {"Wiz8.exe", "Wiz8Runtime.exe", "3DVideo.CFG", "Wiz8.CFG"}:
            continue
        _managed_link(candidate, stage / candidate.name)
    video_cfg = stage / "3DVideo.CFG"
    if not video_cfg.exists():
        shutil.copy2(settings.repo_dir / "config" / "runtime" / "3DVideo.CFG", video_cfg)
    game_cfg = stage / "Wiz8.CFG"
    if not game_cfg.exists():
        encoded = (settings.repo_dir / "config" / "runtime" / "Wiz8.CFG.hex").read_text()
        game_cfg.write_bytes(bytes.fromhex(encoded))
    shutil.copy2(executable, stage / executable_name)
    map_file = executable.with_suffix(".map")
    if map_file.is_file():
        shutil.copy2(map_file, stage / map_file.name)
    return {
        "stage": str(stage),
        "executable": str(stage / executable_name),
        "map": str(stage / map_file.name) if map_file.is_file() else None,
    }


def _map_functions(path: Path) -> list[_MapFunction]:
    functions: list[_MapFunction] = []
    if not path.is_file():
        return functions
    for line in path.read_text(encoding="cp1252", errors="replace").splitlines():
        if match := MAP_FUNCTION.match(line):
            functions.append(
                _MapFunction(
                    int(match.group("address"), 16),
                    match.group("segment"),
                    int(match.group("offset"), 16),
                    match.group("symbol"),
                    match.group("object"),
                )
            )
    return sorted(functions, key=lambda function: function.address)


def _map_sections(path: Path) -> tuple[list[_MapSection], dict[str, int]]:
    if not path.is_file():
        return [], {}
    lines = path.read_text(encoding="cp1252", errors="replace").splitlines()
    segment_bases: dict[str, int] = {}
    for line in lines:
        if match := MAP_FUNCTION.match(line):
            segment_bases.setdefault(
                match.group("segment"),
                int(match.group("address"), 16) - int(match.group("offset"), 16),
            )
    sections = [
        _MapSection(
            match.group("segment"),
            int(match.group("offset"), 16),
            int(match.group("offset"), 16) + int(match.group("length"), 16),
        )
        for line in lines
        if (match := MAP_SECTION.match(line))
    ]
    return sections, segment_bases


def _map_lines(path: Path, segment_bases: dict[str, int]) -> list[tuple[int, str, int, str]]:
    if not path.is_file():
        return []
    lines = path.read_text(encoding="cp1252", errors="replace").splitlines()
    entries: list[tuple[int, str, int, str]] = []
    source: str | None = None
    for line in lines:
        if header := MAP_LINE_HEADER.match(line):
            source = header.group("source").replace("Z:\\repo\\", "").replace("\\", "/")
            continue
        if source is None:
            continue
        for match in MAP_LINE.finditer(line):
            base = segment_bases.get(match.group("segment"))
            if base is not None:
                entries.append(
                    (
                        base + int(match.group("offset"), 16),
                        source,
                        int(match.group("line")),
                        match.group("segment"),
                    )
                )
    return sorted(entries)


def _symbolize_addresses(map_path: Path, addresses: list[int]) -> list[str]:
    functions = _map_functions(map_path)
    sections, segment_bases = _map_sections(map_path)
    source_lines = _map_lines(map_path, segment_bases)
    resolved: list[tuple[int, _MapFunction, int, str]] = []
    for address in addresses:
        target_sections = [
            (section, segment_bases[section.segment])
            for section in sections
            if section.segment in segment_bases
            and segment_bases[section.segment] + section.start
            <= address
            < segment_bases[section.segment] + section.end
        ]
        if not target_sections:
            continue
        section, base = target_sections[0]
        candidates = [
            function
            for function in functions
            if function.segment == section.segment
            and section.start <= function.offset < section.end
            and function.address <= address
        ]
        if not candidates:
            continue
        function = candidates[-1]
        offset = address - base
        next_public = next(
            (
                item.offset
                for item in functions
                if item.segment == function.segment
                and function.offset < item.offset
                and item.offset < section.end
            ),
            section.end,
        )
        if offset >= next_public:
            continue
        location = ""
        eligible_lines = [
            entry
            for entry in source_lines
            if entry[3] == function.segment and function.address <= entry[0] <= address
        ]
        if eligible_lines:
            _line_address, source, line, _segment = eligible_lines[-1]
            location = f" {source}:{line}"
        resolved.append((address, function, address - function.address, location))
    try:
        demangled = demangle([item[1].symbol for item in resolved])
    except (DemanglerMissing, RuntimeError):
        demangled = {}
    results: list[str] = []
    for address, function, displacement, location in resolved:
        name = demangled.get(function.symbol) or function.symbol
        results.append(f"{address:08x}: {name}+0x{displacement:x} [{function.owner}]{location}")
    return results


def _runtime_failure(
    scenario: str,
    returncode: int | None,
    stdout: str,
    stderr: str,
    stage: Path,
    executable: Path,
) -> RuntimeError:
    artifact_dir = stage / "diagnostics"
    artifact_dir.mkdir(exist_ok=True)
    artifact = artifact_dir / f"{scenario}-failure.txt"
    artifact.write_text(stdout + stderr, encoding="utf-8", errors="replace")
    combined = stdout + stderr
    crash = RUNTIME_CRASH.search(combined)
    if crash:
        summary = crash.group(0)
    else:
        diagnostic_lines = [line for line in combined.splitlines() if line.strip()]
        last_diagnostic = diagnostic_lines[-1][-500:] if diagnostic_lines else "no diagnostics"
        summary = f"status={returncode if returncode is not None else 'timeout'}: {last_diagnostic}"
    addresses = [
        int(match.group("address"), 16) for match in RUNTIME_STACK_CANDIDATE.finditer(combined)
    ]
    symbols = _symbolize_addresses(executable.with_suffix(".map"), addresses[:10])[:5]
    detail = "\n".join(symbols)
    if detail:
        detail = "\nhost stack candidates:\n" + detail
    return RuntimeError(f"{scenario} failed: {summary}{detail}\nartifacts={artifact}")


def _runtime_test_environment(settings: Settings) -> tuple[Path, dict[str, str]]:
    prefix = Path(os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime"))
    prefix.mkdir(parents=True, exist_ok=True)
    # The scenarios never assert audible output. Force the soundless-machine
    # path so Wine's stub audio drivers cannot perturb semantic observations.
    overrides = "winemenubuilder.exe=d;winealsa.drv=d;wineoss.drv=d;winepulse.drv=d;winemm.drv=d"
    environment = {
        **os.environ,
        "WINEPREFIX": str(prefix),
        "WINEDLLOVERRIDES": overrides,
    }
    environment["WINEDEBUG"] = "-all"
    return prefix, environment


def _configure_wine_window_management(
    environment: dict[str, str], *, private_display: bool
) -> None:
    """Keep Wine from waiting for a window manager on a private X server."""

    subprocess.run(
        [
            "wine",
            "reg",
            "add",
            r"HKCU\Software\Wine\X11 Driver",
            "/v",
            "Managed",
            "/d",
            "N" if private_display else "Y",
            "/f",
        ],
        env=environment,
        check=True,
        timeout=60,
    )


def _parse_runtime_observation(stdout: str) -> dict[str, str | int]:
    matches = [match for line in stdout.splitlines() if (match := RUNTIME_OBSERVATION.match(line))]
    if len(matches) != 1:
        raise RuntimeError(f"expected one runtime observation, found {len(matches)}")
    fields: dict[str, str | int] = {}
    for item in matches[0].group("fields").split():
        key, separator, value = item.partition("=")
        if not separator:
            raise RuntimeError(f"malformed runtime observation field: {item}")
        fields[key] = int(value) if value.lstrip("-").isdigit() else value
    return fields


def _timeout_output(value: str | bytes | None) -> str:
    return value.decode(errors="replace") if isinstance(value, bytes) else value or ""


def _run_runtime_scenario(
    executable: Path, stage: Path, environment: dict[str, str], scenario: str
) -> dict[str, str | int]:
    try:
        completed = subprocess.run(
            ["wine", f"./{executable.name}", "--scenario", scenario],
            cwd=stage,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=45,
        )
    except subprocess.TimeoutExpired as error:
        stdout = _timeout_output(error.stdout)
        stderr = _timeout_output(error.stderr)
        raise _runtime_failure(scenario, None, stdout, stderr, stage, executable) from error
    if completed.returncode:
        raise _runtime_failure(
            scenario, completed.returncode, completed.stdout, completed.stderr, stage, executable
        )
    try:
        observation = _parse_runtime_observation(completed.stdout)
    except RuntimeError as error:
        raise _runtime_failure(
            scenario, completed.returncode, completed.stdout, completed.stderr, stage, executable
        ) from error
    if observation.get("scenario") != scenario:
        raise _runtime_failure(
            scenario, completed.returncode, completed.stdout, completed.stderr, stage, executable
        )
    return observation


def run_runtime_suite(settings: Settings) -> dict[str, Any]:
    """Run named in-process scenarios in both orders and prove determinism."""

    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME_TEST")
    staged = stage_runtime_test(settings)
    stage = Path(staged["stage"])
    executable = Path(staged["executable"])
    prefix, environment = _runtime_test_environment(settings)
    runs: dict[str, dict[str, dict[str, str | int]]] = {}
    with runtime_display(
        environment, default="virtual", log_path=stage / "xvfb-runtime-test.log"
    ) as display:
        _configure_wine_window_management(environment, private_display=display is not None)
        try:
            for order_name, scenarios in (
                ("forward", RUNTIME_SCENARIOS),
                ("reverse", tuple(reversed(RUNTIME_SCENARIOS))),
            ):
                runs[order_name] = {
                    scenario: _run_runtime_scenario(executable, stage, environment, scenario)
                    for scenario in scenarios
                }
        finally:
            subprocess.run(
                ["wineserver", "-k"],
                cwd=stage,
                env=environment,
                check=False,
                capture_output=True,
            )
    if runs["forward"] != runs["reverse"]:
        raise RuntimeError("runtime observations depend on scenario order")
    return {
        **staged,
        "wine_prefix": str(prefix),
        "display": display or "host",
        "scenarios": runs["forward"],
        "deterministic": True,
    }
