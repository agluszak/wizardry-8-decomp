from __future__ import annotations

import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .binary.demangle import DemanglerMissing, demangle
from .config import Settings
from .display import runtime_display
from .dynamic import _allocate_port, _listening, _terminate_process_group


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
GDB_FRAME = re.compile(r"^#\d+\s+0x(?P<address>[0-9a-fA-F]+)\b", re.MULTILINE)
GDB_SIGNAL = re.compile(
    r"^(?:Thread .* )?(?:Program received|received|Program terminated with) signal "
    r"(?P<signal>[A-Z][A-Z0-9]+)\b",
    re.MULTILINE,
)
GDB_NORMAL_EXIT = re.compile(r"(?:exited normally|exited with code 0\b)", re.IGNORECASE)
GDB_FAILED_EXIT = re.compile(r"exited with code (?!0\b)(?P<code>[^\]\s]+)", re.IGNORECASE)
GDB_TRANSPORT_FAILURE = re.compile(
    r"(?:Connection (?:refused|timed out)|Remote communication error|"
    r"Remote connection closed|The program is not being run)",
    re.IGNORECASE,
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


def stage_runtime(settings: Settings, executable_name: str = "Wiz8Runtime.exe") -> dict[str, Any]:
    source = settings.work_dir / "variants" / "gog-base"
    stage = settings.repo_dir / "build" / "runtime" / "wiz8"
    executable = settings.repo_dir / "build" / "decomp" / executable_name
    for name in ("Data", "Dll", "Levels"):
        if not (source / name).is_dir():
            raise RuntimeError(f"missing retail asset directory: {source / name}")
    if not executable.is_file():
        raise RuntimeError(f"runtime executable is not built: {executable}")
    stage.mkdir(parents=True, exist_ok=True)
    (stage / "Saves").mkdir(exist_ok=True)
    links: list[str] = []
    for name in ("Data", "Dll", "Levels", "Patches"):
        candidate = source / name
        if candidate.exists():
            _managed_link(candidate, stage / name)
            links.append(name)
    for candidate in sorted(path for path in source.iterdir() if path.is_file()):
        if candidate.name in {"Wiz8.exe", "Wiz8Runtime.exe", "3DVideo.CFG"}:
            continue
        _managed_link(candidate, stage / candidate.name)
        links.append(candidate.name)
    video_cfg = stage / "3DVideo.CFG"
    if not video_cfg.exists():
        shutil.copy2(settings.repo_dir / "config" / "runtime" / "3DVideo.CFG", video_cfg)
    game_cfg = stage / "Wiz8.CFG"
    if not game_cfg.exists():
        encoded = (settings.repo_dir / "config" / "runtime" / "Wiz8.CFG.hex").read_text()
        game_cfg.write_bytes(bytes.fromhex(encoded))
    shutil.copy2(executable, stage / executable_name)
    # Stage the VC6 program database next to the executable so Wine's
    # debugger resolves our symbols instead of reporting Deferred modules.
    program_database = executable.with_suffix(".pdb")
    if program_database.is_file():
        shutil.copy2(program_database, stage / program_database.name)
    map_file = executable.with_suffix(".map")
    if map_file.is_file():
        shutil.copy2(map_file, stage / map_file.name)
    data = executable.read_bytes()
    timestamp: str | None = None
    if len(data) >= 0x40:
        pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if pe_offset + 12 <= len(data):
            timestamp = f"{struct.unpack_from('<I', data, pe_offset + 8)[0]:08x}"
    identity = {
        "executable": executable_name,
        "sha256": hashlib.sha256(data).hexdigest(),
        "pe_timestamp": timestamp,
        "map": map_file.name if map_file.is_file() else None,
    }
    (stage / f"{Path(executable_name).stem}.symbols.json").write_text(
        json.dumps(identity, indent=2) + "\n", encoding="utf-8"
    )
    return {
        "stage": str(stage),
        "links": links,
        "executable": str(stage / executable_name),
        "map": str(stage / map_file.name) if map_file.is_file() else None,
        "sha256": identity["sha256"],
        "pe_timestamp": timestamp,
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


def _runtime_crash_signature(output: str) -> tuple[str, str, str] | None:
    match = RUNTIME_CRASH.search(output)
    if match is None:
        return None
    fields = dict(item.split("=", 1) for item in match.group("fields").split() if "=" in item)
    code = fields.get("code")
    operation = fields.get("operation")
    access = fields.get("access")
    return (code, operation, access) if code and operation and access else None


def _parse_gdb_diagnostics(output: str, returncode: int) -> dict[str, Any]:
    """Classify only stop and exit states explicitly reported by GDB."""

    frames = [int(match.group("address"), 16) for match in GDB_FRAME.finditer(output)]
    signal_match = GDB_SIGNAL.search(output)
    if signal_match:
        location = re.search(
            r"^0x(?P<address>[0-9a-fA-F]+) in (?P<name>.+)$",
            output[signal_match.end() :],
            re.MULTILINE,
        )
        stop_reason = f"signal {signal_match.group('signal')}"
        if location:
            stop_reason += f" at 0x{location.group('address')} in {location.group('name')}"
        return {
            "classification": "debug_crashed",
            "stop_reason": stop_reason,
            "addresses": frames,
            "crash_signature": _runtime_crash_signature(output),
        }
    if GDB_NORMAL_EXIT.search(output):
        return {"classification": "debug_passed", "stop_reason": "normal exit", "addresses": []}
    if failed_exit := GDB_FAILED_EXIT.search(output):
        return {
            "classification": "debug_failed",
            "stop_reason": f"exit code {failed_exit.group('code')}",
            "addresses": [],
        }
    if GDB_TRANSPORT_FAILURE.search(output) or returncode != 0:
        return {
            "classification": "debugger_transport_failure",
            "stop_reason": "GDB transport did not produce a target stop or exit",
            "addresses": frames,
        }
    return {
        "classification": "debug_failed",
        "stop_reason": "no explicit GDB target stop or exit",
        "addresses": frames,
    }


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


def _wine_environment(
    settings: Settings, *, silent_audio: bool = False, quiet: bool = False
) -> tuple[Path, dict[str, str]]:
    prefix = Path(os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime"))
    prefix.mkdir(parents=True, exist_ok=True)
    # Let a crashing process return to this orchestrator instead of blocking in
    # Wine's automatic debugger. The explicit winedbg rerun removes this one
    # override below.
    overrides = "winedbg.exe=d;winemenubuilder.exe=d"
    if silent_audio:
        # Miles crashes inside Wine's stub audio drivers instead of reporting
        # no usable driver. The scenarios never assert audible output, so the
        # suite runs the soundless-machine path: every Miles open fails and
        # the game stays silent.
        overrides += ";winealsa.drv=d;wineoss.drv=d;winepulse.drv=d;winemm.drv=d"
    environment = {
        **os.environ,
        "WINEPREFIX": str(prefix),
        "WINEDLLOVERRIDES": overrides,
    }
    if quiet:
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
        failure = _runtime_failure(scenario, None, stdout, stderr, stage, executable)
        diagnosis = diagnose_runtime_failure(
            stage,
            executable,
            ["--scenario", scenario],
            environment,
            scenario,
        )
        raise RuntimeError(f"{failure}\n{_format_diagnosis(diagnosis)}") from error
    if completed.returncode:
        failure = _runtime_failure(
            scenario, completed.returncode, completed.stdout, completed.stderr, stage, executable
        )
        diagnosis = diagnose_runtime_failure(
            stage,
            executable,
            ["--scenario", scenario],
            environment,
            scenario,
        )
        raise RuntimeError(f"{failure}\n{_format_diagnosis(diagnosis)}")
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
    staged = stage_runtime(settings, "Wiz8RuntimeTest.exe")
    stage = Path(staged["stage"])
    executable = Path(staged["executable"])
    prefix, environment = _wine_environment(settings, silent_audio=True, quiet=True)
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


def diagnose_runtime_failure(
    stage: Path,
    executable: Path,
    arguments: list[str],
    environment: dict[str, str],
    label: str,
) -> dict[str, Any]:
    """Rerun the failed staged executable under winedbg and GDB."""

    environment = {
        **environment,
        "WINEDLLOVERRIDES": environment.get("WINEDLLOVERRIDES", "").replace("winedbg.exe=d;", ""),
    }
    artifact_dir = stage / "diagnostics"
    artifact_dir.mkdir(exist_ok=True)
    artifact = artifact_dir / f"{label}-gdb.txt"
    proxy_artifact = artifact_dir / f"{label}-gdb-proxy.txt"
    proxy: subprocess.Popen[str] | None = None
    proxy_stream = None
    try:
        try:
            port = _allocate_port()
            proxy_stream = proxy_artifact.open("w", encoding="utf-8")
            proxy = subprocess.Popen(
                [
                    "winedbg",
                    "--gdb",
                    "--no-start",
                    "--port",
                    str(port),
                    f"./{executable.name}",
                    *arguments,
                ],
                cwd=stage,
                env=environment,
                stdout=proxy_stream,
                stderr=subprocess.STDOUT,
                text=True,
                start_new_session=True,
            )
            if not _listening(port, time.monotonic() + 60):
                return {
                    "classification": "debugger_transport_failure",
                    "stop_reason": "winedbg --gdb never opened its port",
                    "artifacts": str(proxy_artifact),
                }
            commands = [
                "set pagination off",
                "handle SIGTRAP stop print nopass",
                f"target remote localhost:{port}",
                "continue",
                "thread apply all bt full",
                "info registers",
                "info sharedlibrary",
                "x/16i $pc-16",
            ]
            argv = ["gdb", "--batch"]
            for command in commands:
                argv.extend(("-ex", command))
            completed = subprocess.run(
                argv,
                cwd=stage,
                env=environment,
                capture_output=True,
                text=True,
                timeout=60,
                check=False,
            )
            output = completed.stdout + completed.stderr
            artifact.write_text(output, encoding="utf-8", errors="replace")
            diagnosis = _parse_gdb_diagnostics(output, completed.returncode)
            symbols = _symbolize_addresses(
                executable.with_suffix(".map"), diagnosis.pop("addresses")
            )[:5]
            diagnosis.pop("crash_signature", None)
            return {
                **diagnosis,
                "host_symbol_candidates": symbols,
                "artifacts": str(artifact),
            }
        except subprocess.TimeoutExpired as error:
            output = _timeout_output(error.stdout) + _timeout_output(error.stderr)
            artifact.write_text(output, encoding="utf-8", errors="replace")
            return {
                "classification": "debug_timeout",
                "stop_reason": "GDB timed out",
                "host_symbol_candidates": [],
                "artifacts": str(artifact),
            }
    finally:
        if proxy is not None:
            _terminate_process_group(proxy)
        if proxy_stream is not None:
            proxy_stream.close()


def _format_diagnosis(diagnosis: dict[str, Any]) -> str:
    symbols = "\n".join(diagnosis.get("host_symbol_candidates", []))
    detail = f"\nstack candidates:\n{symbols}" if symbols else ""
    return (
        f"debugger classification={diagnosis['classification']} "
        f"stop={diagnosis.get('stop_reason', '')}{detail}\n"
        f"debugger artifact={diagnosis['artifacts']}"
    )


def run_game(settings: Settings) -> dict[str, Any]:
    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME")
    staged = stage_runtime(settings)
    stage = Path(staged["stage"])
    prefix, environment = _wine_environment(settings)
    with runtime_display(
        environment, default="host", log_path=stage / "xvfb-runtime.log"
    ) as display:
        _configure_wine_window_management(environment, private_display=display is not None)
        try:
            completed = subprocess.run(
                ["wine", "./Wiz8Runtime.exe"], cwd=stage, env=environment, check=False
            )
            if completed.returncode:
                diagnosis = diagnose_runtime_failure(
                    stage, Path(staged["executable"]), [], environment, "run"
                )
                raise RuntimeError(
                    f"Wiz8Runtime.exe exited with status {completed.returncode}\n"
                    f"{_format_diagnosis(diagnosis)}"
                )
        finally:
            subprocess.run(
                ["wineserver", "-k"],
                cwd=stage,
                env=environment,
                check=False,
                capture_output=True,
            )
    return {
        **staged,
        "wine_prefix": str(prefix),
        "display": display or "host",
        "exit_status": 0,
    }
