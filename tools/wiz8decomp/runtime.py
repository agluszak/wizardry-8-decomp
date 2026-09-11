from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
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
RUNTIME_CANDIDATE = re.compile(
    r"^WIZ8_RUNTIME_CANDIDATE source=(?P<source>\S+) address=(?P<address>[0-9a-fA-F]+)",
    re.MULTILINE,
)
WINE_REGISTER = re.compile(
    r"\b(?P<name>EIP|ESP|EBP|EAX|EBX|ECX|EDX|ESI|EDI)\s*:\s*(?P<value>[0-9a-fA-F]{8,16})\b"
)
WINE_EXCEPTION = re.compile(r"Unhandled exception:\s*(?P<operation>[^\n]*)", re.IGNORECASE)
WINE_FAULT = re.compile(r"Unhandled page fault", re.IGNORECASE)
WINE_FRAME = re.compile(r"^\s*(?:=>)?\d+\s+0x(?P<address>[0-9a-fA-F]+)", re.MULTILINE)
WINE_STACK_LINE = re.compile(
    r"0x(?P<address>[0-9a-fA-F]{8,16}):\s+"
    r"(?P<words>[0-9a-fA-F]{1,16}(?:\s+[0-9a-fA-F]{1,16})*)\s*$",
    re.MULTILINE,
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


@dataclass(frozen=True)
class _ResolvedAddress:
    address: int
    symbol: str
    displacement: int
    owner: str
    location: str

    def format(self) -> str:
        return (
            f"{self.address:08x}: {self.symbol}+0x{self.displacement:x} "
            f"[{self.owner}]{self.location}"
        )


@dataclass(frozen=True)
class _CrashCandidate:
    source: str
    address: int


@dataclass(frozen=True)
class _RuntimeCrash:
    record: str
    fields: dict[str, str]
    candidates: list[_CrashCandidate]


RUNTIME_CRASH_MARKER = re.compile(
    r"WIZ8_RUNTIME_CRASH|Unhandled exception|Unhandled page fault|Register dump:",
    re.IGNORECASE,
)
RUNTIME_EXECUTABLES = frozenset({"Wiz8.exe", "Wiz8Runtime.exe", "Wiz8RuntimeTest.exe"})


@dataclass(frozen=True)
class StagedGame:
    """One writable build/runtime tree assembled from an immutable variant."""

    root: Path
    executable: Path
    map: Path | None
    objects: Path | None

    def as_dict(self) -> dict[str, Any]:
        return {
            "stage": str(self.root),
            "executable": str(self.executable),
            "map": str(self.map) if self.map is not None else None,
            "objects": str(self.objects) if self.objects is not None else None,
        }


def _materialize_config(settings: Settings, stage: Path) -> None:
    video_cfg = stage / "3DVideo.CFG"
    if not video_cfg.exists():
        shutil.copy2(settings.repo_dir / "config" / "runtime" / "3DVideo.CFG", video_cfg)
    game_cfg = stage / "Wiz8.CFG"
    if not game_cfg.exists():
        encoded = (settings.repo_dir / "config" / "runtime" / "Wiz8.CFG.hex").read_text()
        game_cfg.write_bytes(bytes.fromhex(encoded))


def stage_game(
    settings: Settings,
    *,
    name: str,
    executable: Path,
    objects: Path | None = None,
    reset_saves: bool = False,
) -> StagedGame:
    """Materialize one build/runtime game tree around a chosen executable.

    The prepared variant is an immutable input: asset directories are linked,
    config files and the executable are copied, and every writable path lives
    under build/. Every runnable entry point shares this primitive.
    """

    source = settings.work_dir / "variants" / "gog-base"
    for asset in ("Data", "Dll", "Levels"):
        if not (source / asset).is_dir():
            raise RuntimeError(f"missing retail asset directory: {source / asset}")
    if not executable.is_file():
        raise RuntimeError(f"runtime executable is not built: {executable}")
    stage = settings.runtime_stage(name)
    stage.mkdir(parents=True, exist_ok=True)
    if reset_saves:
        shutil.rmtree(stage / "Saves", ignore_errors=True)
        (stage / "Saves" / "Characters").mkdir(parents=True, exist_ok=True)
        (stage / "Saves" / "NPCs").mkdir(parents=True, exist_ok=True)
    for asset in ("Data", "Dll", "Levels", "Patches"):
        candidate = source / asset
        if candidate.exists():
            _managed_link(candidate, stage / asset)
    for candidate in sorted(path for path in source.iterdir() if path.is_file()):
        if candidate.name in RUNTIME_EXECUTABLES or candidate.name == executable.name:
            continue
        if candidate.name in {"3DVideo.CFG", "Wiz8.CFG"}:
            continue
        _managed_link(candidate, stage / candidate.name)
    _materialize_config(settings, stage)
    staged_executable = stage / executable.name
    if staged_executable.is_symlink():
        staged_executable.unlink()
    shutil.copy2(executable, staged_executable)
    map_file = executable.with_suffix(".map")
    staged_map = stage / map_file.name if map_file.is_file() else None
    if staged_map is not None:
        shutil.copy2(map_file, staged_map)
    return StagedGame(stage, staged_executable, staged_map, objects)


def run_product(
    settings: Settings, arguments: list[str] | None = None, *, original: bool = False
) -> dict[str, Any]:
    """Stage and launch one game process, symbolizing a crash when it happens."""

    if shutil.which("wine") is None:
        raise RuntimeError("wine is required to run the game")
    if original:
        staged = stage_game(
            settings,
            name="original",
            executable=settings.work_dir / "variants" / "gog-base" / "Wiz8.exe",
        )
        map_path = None
    else:
        staged = stage_game(
            settings,
            name="wiz8",
            executable=settings.product_build_dir / "Wiz8Runtime.exe",
            objects=settings.recovered_objects_dir,
        )
        map_path = settings.product_build_dir / "Wiz8Runtime.map"
    completed = subprocess.run(
        ["wine", f"./{staged.executable.name}", "/WINDOW", *(arguments or [])],
        cwd=staged.root,
        check=False,
        capture_output=True,
        text=True,
        errors="replace",
    )
    output = completed.stdout + completed.stderr
    if output:
        sys.stderr.write(output)
    crash: dict[str, Any] | None = None
    if map_path is not None and RUNTIME_CRASH_MARKER.search(output):
        log_path = staged.root / "diagnostics" / "run.log"
        log_path.parent.mkdir(parents=True, exist_ok=True)
        log_path.write_text(output, encoding="utf-8", errors="replace")
        crash = analyze_runtime_crash(log_path, map_path, staged.objects)
    return {**staged.as_dict(), "status": completed.returncode, "crash": crash}


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


def _resolve_addresses(map_path: Path, addresses: list[int]) -> list[_ResolvedAddress | None]:
    functions = _map_functions(map_path)
    sections, segment_bases = _map_sections(map_path)
    source_lines = _map_lines(map_path, segment_bases)
    matches: list[tuple[_MapFunction, int, str] | None] = []
    for address in addresses:
        lookup = address
        target_sections = [
            (section, segment_bases[section.segment])
            for section in sections
            if section.segment in segment_bases
            and segment_bases[section.segment] + section.start
            <= lookup
            < segment_bases[section.segment] + section.end
        ]
        if not target_sections:
            matches.append(None)
            continue
        section, base = target_sections[0]
        candidates = [
            function
            for function in functions
            if function.segment == section.segment
            and section.start <= function.offset < section.end
            and function.address <= lookup
        ]
        if not candidates:
            matches.append(None)
            continue
        function = candidates[-1]
        offset = lookup - base
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
            matches.append(None)
            continue
        location = ""
        eligible_lines = [
            entry
            for entry in source_lines
            if entry[3] == function.segment and function.address <= entry[0] <= lookup
        ]
        if eligible_lines:
            _line_address, source, line, _segment = eligible_lines[-1]
            location = f" {source}:{line}"
        matches.append((function, lookup - function.address, location))
    names = [match[0].symbol for match in matches if match is not None]
    try:
        demangled = demangle(names)
    except (DemanglerMissing, RuntimeError):
        demangled = {}
    resolved: list[_ResolvedAddress | None] = []
    for address, match in zip(addresses, matches):
        if match is None:
            resolved.append(None)
            continue
        function, displacement, location = match
        resolved.append(
            _ResolvedAddress(
                address,
                demangled.get(function.symbol) or function.symbol,
                displacement,
                function.owner,
                location,
            )
        )
    return resolved


def _symbolize_addresses(map_path: Path, addresses: list[int]) -> list[str]:
    return [item.format() for item in _resolve_addresses(map_path, addresses) if item is not None]


def _parse_diagnostic_fields(fields: str) -> dict[str, str]:
    values: dict[str, str] = {}
    for item in fields.split():
        key, separator, value = item.partition("=")
        if not separator:
            raise RuntimeError(f"malformed runtime diagnostic field: {item}")
        values[key] = value
    return values


def _parse_runtime_crash(output: str) -> _RuntimeCrash | None:
    match = RUNTIME_CRASH.search(output)
    if match is None:
        return None
    fields = _parse_diagnostic_fields(match.group("fields"))
    candidates: list[_CrashCandidate] = []
    seen: set[int] = set()
    for candidate in RUNTIME_CANDIDATE.finditer(output):
        address = int(candidate.group("address"), 16)
        if address in seen:
            continue
        seen.add(address)
        candidates.append(_CrashCandidate(candidate.group("source"), address))
    return _RuntimeCrash(match.group(0), fields, candidates)


def _parse_wine_dump(output: str) -> _RuntimeCrash | None:
    """Recognize a Wine unhandled-exception dump without product markers.

    Kept as an offline aid for old external logs. Registers, the frame walk and
    the stack words are collected as MAP candidates; the forced-unresolved
    reconstruction (image-base fault, DOS-header execution and the consumed EDX
    return) is obsolete and deliberately absent.
    """

    registers: dict[str, int] = {}
    for match in WINE_REGISTER.finditer(output):
        registers.setdefault(match.group("name").lower(), int(match.group("value"), 16))
    eip = registers.get("eip")
    if eip is None:
        return None
    fields: dict[str, str] = {
        "code": "wine",
        "thread": "",
        "operation": "",
        "access": "",
        "eip": f"{eip:08x}",
        "esp": f"{registers.get('esp', 0):08x}",
    }
    for name in ("ebp", "eax", "ebx", "ecx", "edx", "esi", "edi"):
        fields[name] = f"{registers[name]:08x}" if name in registers else ""
    if match := WINE_EXCEPTION.search(output):
        fields["operation"] = match.group("operation").strip()[:120]
    candidates: list[_CrashCandidate] = []
    seen: set[int] = set()

    def add(source: str, address: int) -> None:
        if address >= 0x100000000:
            return
        if address in seen or len(candidates) >= 32:
            return
        seen.add(address)
        candidates.append(_CrashCandidate(source, address))

    for name in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"):
        if name in registers:
            add(f"reg:{name}", registers[name])
    for match in WINE_FRAME.finditer(output):
        add("frame", int(match.group("address"), 16))
    esp = registers.get("esp")
    for line_index, match in enumerate(WINE_STACK_LINE.finditer(output)):
        line_address = int(match.group("address"), 16)
        for word_index, word in enumerate(match.group("words").split()):
            if esp is not None:
                offset = line_address - esp + word_index * 4
            else:
                offset = line_index * 16 + word_index * 4
            add(f"stack+0x{offset:x}", int(word, 16))
    if not candidates:
        return None
    return _RuntimeCrash("", fields, candidates)


_ADDRESS_PLACEHOLDER = re.compile(r"^\??Function[0-9A-Fa-f]+(?:@|$)")


def _unresolved_rank(name: str) -> tuple[int, str]:
    """Named first-party gaps precede placeholder and compiler-internal ones."""

    if name.startswith("__"):
        return (2, name)
    if _ADDRESS_PLACEHOLDER.match(name):
        return (1, name)
    return (0, name)


def _unresolved_for_owners(
    object_root: Path | None, map_path: Path | None, owners: list[str]
) -> dict[str, list[str]]:
    if object_root is None or not object_root.is_dir():
        return {}
    from .unresolved import unresolved_report

    try:
        report = unresolved_report(object_root, map_path)
    except Exception:  # noqa: BLE001 - crash correlation must not hide the crash record
        return {}
    by_basename: dict[str, list[str]] = {}
    for unit, symbols in report["by_unit"].items():
        by_basename.setdefault(Path(unit).name, []).extend(symbols)
    return {
        owner: sorted(set(by_basename.get(owner, [])), key=_unresolved_rank)
        for owner in dict.fromkeys(owners)
        if owner in by_basename
    }


def _crash_detail(map_path: Path, object_root: Path | None, crash: _RuntimeCrash) -> str:
    resolved = _resolve_addresses(map_path, [candidate.address for candidate in crash.candidates])
    lines: list[str] = []
    for index, (candidate, item) in enumerate(list(zip(crash.candidates, resolved))[:8]):
        if item is None:
            continue
        lines.append(f"#{index} {candidate.source}: {item.format()}")
    owners = [item.owner for item in resolved if item is not None]
    for owner, symbols in _unresolved_for_owners(object_root, map_path, owners).items():
        lines.append(f"unresolved references from {owner}:")
        for symbol in symbols[:12]:
            lines.append(f"  {symbol}")
        if len(symbols) > 12:
            lines.append(f"  ... (+{len(symbols) - 12} more)")
    return "\n" + "\n".join(lines) if lines else ""


def _parse_failure_report(
    output: str, reason: str, exception: str, missing: list[str]
) -> dict[str, Any]:
    """Report an unhandled exception whose dump could not be parsed.

    Returning an empty crash list for a recognized exception hides the
    failure; keep the raw context so the next debugging step has the input
    that actually defeated the parser.
    """

    lines = output.splitlines()
    return {
        "schema": "wiz8.runtime-crash",
        "crashes": [],
        "parse_failure": {
            "reason": reason,
            "exception": exception[:200],
            "missing": missing,
            "log_tail": "\n".join(lines[-40:])[-4000:],
        },
    }


def analyze_runtime_crash(
    log_path: Path, map_path: Path | None = None, object_root: Path | None = None
) -> dict[str, Any]:
    """Symbolize a captured runnable-image crash through its link MAP."""

    output = log_path.read_text(encoding="utf-8", errors="replace")
    crash = _parse_runtime_crash(output)
    if crash is None:
        crash = _parse_wine_dump(output)
    if crash is None:
        exception = WINE_EXCEPTION.search(output)
        fault = WINE_FAULT.search(output)
        if exception is not None or fault is not None:
            detail = exception.group(0) if exception is not None else ""
            if not detail and fault is not None:
                detail = fault.group(0)
            missing: list[str] = []
            if not WINE_REGISTER.search(output):
                missing.append("EIP register dump")
            else:
                missing.append("image address in the register file, stack dump or backtrace")
            return _parse_failure_report(
                output,
                "unhandled exception present but no crash could be localized",
                detail,
                missing,
            )
        return {"schema": "wiz8.runtime-crash", "crashes": []}
    resolved = (
        _resolve_addresses(map_path, [candidate.address for candidate in crash.candidates])
        if map_path is not None
        else [None] * len(crash.candidates)
    )
    candidates: list[dict[str, Any]] = []
    for candidate, item in zip(crash.candidates, resolved):
        entry: dict[str, Any] = {
            "source": candidate.source,
            "address": f"{candidate.address:08x}",
        }
        if item is not None:
            entry["symbol"] = f"{item.symbol}+0x{item.displacement:x}"
            entry["owner"] = item.owner
            if item.location:
                entry["source_location"] = item.location.strip()
        candidates.append(entry)
    owners = [item.owner for item in resolved if item is not None]
    report: dict[str, Any] = {
        "schema": "wiz8.runtime-crash",
        "crashes": [
            {
                "code": crash.fields.get("code", ""),
                "thread": crash.fields.get("thread", ""),
                "operation": crash.fields.get("operation", ""),
                "access": crash.fields.get("access", ""),
                "eip": crash.fields.get("eip", ""),
                "esp": crash.fields.get("esp", ""),
                "registers": {
                    name: crash.fields.get(name, "")
                    for name in ("ebp", "eax", "ebx", "ecx", "edx", "esi", "edi")
                },
                "candidates": candidates,
                "unresolved_by_owner": _unresolved_for_owners(object_root, map_path, owners),
            }
        ],
    }
    return report


def _runtime_failure(
    scenario: str,
    returncode: int | None,
    stdout: str,
    stderr: str,
    stage: Path,
    executable: Path,
    object_root: Path | None = None,
) -> RuntimeError:
    artifact_dir = stage / "diagnostics"
    artifact_dir.mkdir(exist_ok=True)
    artifact = artifact_dir / f"{scenario}-failure.txt"
    artifact.write_text(stdout + stderr, encoding="utf-8", errors="replace")
    combined = stdout + stderr
    crash = _parse_runtime_crash(combined)
    if crash is not None:
        detail = _crash_detail(executable.with_suffix(".map"), object_root, crash)
        return RuntimeError(f"{scenario} failed: {crash.record}{detail}\nartifacts={artifact}")
    diagnostic_lines = [line for line in combined.splitlines() if line.strip()]
    last_diagnostic = diagnostic_lines[-1][-500:] if diagnostic_lines else "no diagnostics"
    summary = f"status={returncode if returncode is not None else 'timeout'}: {last_diagnostic}"
    return RuntimeError(f"{scenario} failed: {summary}\nartifacts={artifact}")


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
    executable: Path,
    stage: Path,
    environment: dict[str, str],
    scenario: str,
    object_root: Path | None = None,
) -> dict[str, str | int]:
    try:
        completed = subprocess.run(
            ["wine", f"./{executable.name}", "--scenario", scenario],
            cwd=stage,
            env=environment,
            check=False,
            capture_output=True,
            text=True,
            timeout=75,
        )
    except subprocess.TimeoutExpired as error:
        stdout = _timeout_output(error.stdout)
        stderr = _timeout_output(error.stderr)
        raise _runtime_failure(
            scenario, None, stdout, stderr, stage, executable, object_root
        ) from error
    if completed.returncode:
        raise _runtime_failure(
            scenario,
            completed.returncode,
            completed.stdout,
            completed.stderr,
            stage,
            executable,
            object_root,
        )
    try:
        observation = _parse_runtime_observation(completed.stdout)
    except RuntimeError as error:
        raise _runtime_failure(
            scenario,
            completed.returncode,
            completed.stdout,
            completed.stderr,
            stage,
            executable,
            object_root,
        ) from error
    if observation.get("scenario") != scenario:
        raise _runtime_failure(
            scenario,
            completed.returncode,
            completed.stdout,
            completed.stderr,
            stage,
            executable,
            object_root,
        )
    return observation


def run_runtime_suite(settings: Settings) -> dict[str, Any]:
    """Run named in-process scenarios in both orders and prove determinism."""

    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME_TEST")
    staged = stage_game(
        settings,
        name="runtime-test",
        executable=settings.product_build_dir / "Wiz8RuntimeTest.exe",
        objects=settings.recovered_objects_dir,
        reset_saves=True,
    )
    stage = staged.root
    executable = staged.executable
    object_root = staged.objects
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
                    scenario: _run_runtime_scenario(
                        executable, stage, environment, scenario, object_root
                    )
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
        **staged.as_dict(),
        "wine_prefix": str(prefix),
        "display": display or "host",
        "scenarios": runs["forward"],
        "deterministic": True,
    }
