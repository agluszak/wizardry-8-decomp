from __future__ import annotations

import hashlib
import os
import queue
import re
import selectors
import shutil
import subprocess
import sys
import threading
import time
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from reccmp.formats import detect_image
from reccmp.formats.pe import PEImage

from .binary.linker_map import LinkerMap, SymbolResolution
from .config import Settings
from .display import runtime_display
from .paths import write_if_changed


def _managed_link(source: Path, destination: Path) -> None:
    if destination.is_symlink():
        if destination.resolve() != source.resolve():
            raise RuntimeError(f"runtime symlink points at the wrong source: {destination}")
        return
    if destination.exists():
        raise RuntimeError(f"runtime staging path is not a managed symlink: {destination}")
    destination.symlink_to(source, target_is_directory=source.is_dir())


RUNTIME_OBSERVATION = re.compile(r"^WIZ8_RUNTIME_TEST (?P<fields>.+)$")
RUNTIME_SESSION = re.compile(r"^WIZ8_RUNTIME_SESSION (?P<fields>.+)$", re.MULTILINE)
RUNTIME_FAILURE = re.compile(r"^WIZ8_RUNTIME_FAILURE (?P<fields>.+)$", re.MULTILINE)
RUNTIME_STEP = re.compile(r"^WIZ8_RUNTIME_STEP (?P<fields>.+)$", re.MULTILINE)
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
RUNTIME_FAILURE_GRACE_SECONDS = 2.0


@dataclass(frozen=True)
class RuntimeScenario:
    name: str
    phase: str
    tier: str
    kind: str
    timeout_ms: int
    fixture: str
    path: str
    batch: bool


def _parse_runtime_scenarios(output: str) -> dict[str, RuntimeScenario]:
    lines = output.splitlines()
    if not lines or lines[0] != "name\tphase\ttier\tkind\ttimeout_ms\tfixture\tpath\tbatch":
        raise RuntimeError(
            "runtime executable did not report a scenario registry; rebuild runtime-test"
        )
    scenarios: dict[str, RuntimeScenario] = {}
    for line in lines[1:]:
        fields = line.split("\t")
        if len(fields) != 8:
            raise RuntimeError(f"malformed runtime scenario: {line}")
        name, phase, tier, kind, timeout, fixture, path, batch = fields
        if (
            re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", name) is None
            or name in scenarios
            or phase not in {"engine-ready", "main-menu", "main-game"}
            or tier not in {"pr", "main", "nightly"}
            or kind not in {"acceptance", "integration", "semantic"}
            or not timeout.isascii()
            or not timeout.isdigit()
            or int(timeout) <= 0
            or re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", fixture) is None
            or path not in {"natural", "shortcut"}
            or batch not in {"yes", "no"}
        ):
            raise RuntimeError(f"invalid runtime scenario metadata: {line}")
        scenarios[name] = RuntimeScenario(
            name, phase, tier, kind, int(timeout), fixture, path, batch == "yes"
        )
    if not scenarios:
        raise RuntimeError("runtime scenario registry is empty")
    return scenarios


def _read_runtime_scenarios(
    executable: Path, stage: Path, environment: dict[str, str]
) -> dict[str, RuntimeScenario]:
    result = subprocess.run(
        ["wine", f"./{executable.name}", "--list-scenarios"],
        cwd=stage,
        env=environment,
        capture_output=True,
        text=True,
        timeout=15,
        check=True,
    )
    return _parse_runtime_scenarios(result.stdout)


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
    executable_written: bool = True
    map_written: bool = False

    def as_dict(self) -> dict[str, Any]:
        return {
            "stage": str(self.root),
            "executable": str(self.executable),
            "map": str(self.map) if self.map is not None else None,
            "objects": str(self.objects) if self.objects is not None else None,
            "executable_written": self.executable_written,
            "map_written": self.map_written,
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
    input_pinned: bool = False,
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
    from .build import build_lock

    staged_executable = stage / executable.name
    staged_map = None
    map_written = False

    def publish_executable() -> None:
        nonlocal staged_map, map_written, executable_written
        executable_bytes = executable.read_bytes()
        map_file = executable.with_suffix(".map")
        if map_file.is_file():
            map_bytes = map_file.read_bytes()
            timestamp = LinkerMap.read(map_file).timestamp
            image = detect_image(executable)
            if (
                not isinstance(image, PEImage)
                or timestamp is None
                or timestamp != image.header.time_date_stamp
            ):
                raise RuntimeError(f"executable/MAP link timestamp mismatch; rebuild {executable}")
            identity = hashlib.sha256(executable_bytes + map_bytes).hexdigest()
            staged_map = stage / "diagnostics" / f"{executable.stem}-{identity}.map"
            map_written = write_if_changed(staged_map, map_bytes)
        executable_written = write_if_changed(staged_executable, executable_bytes)

    executable_written = False
    # The linker writes both files while holding this same lock. Publish the
    # complete executable only after its MAP snapshot is safely in place. A
    # pinned input is immutable already, so staging it needs no lock and can
    # proceed concurrently on several workers.
    if input_pinned:
        publish_executable()
    else:
        with build_lock(settings):
            publish_executable()
    return StagedGame(
        stage,
        staged_executable,
        staged_map,
        objects,
        executable_written=executable_written,
        map_written=map_written,
    )


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
        map_path = staged.map
    prefix = Path(os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime"))
    prefix.mkdir(parents=True, exist_ok=True)
    environment = {**os.environ, "WINEPREFIX": str(prefix)}
    environment.setdefault("WINEDLLOVERRIDES", "winemenubuilder.exe=d")
    with runtime_display(
        environment, default="host", log_path=staged.root / "xvfb-run.log"
    ) as display:
        configure_wine_window_management(environment, private_display=display is not None)
        completed = subprocess.run(
            ["wine", f"./{staged.executable.name}", "/WINDOW", *(arguments or [])],
            cwd=staged.root,
            env=environment,
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


def _resolve_addresses(map_path: Path, addresses: list[int]) -> list[SymbolResolution | None]:
    return LinkerMap.read(map_path).resolve_many(addresses)


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


def format_crash_candidates(
    map_path: Path, object_root: Path | None, candidates: list[tuple[str, int]]
) -> tuple[str, list[SymbolResolution]]:
    """Symbolize candidates and correlate unresolved references at their object owners."""
    resolved = _resolve_addresses(map_path, [address for _, address in candidates])
    lines: list[str] = []
    resolved_pairs = [
        (candidate, item) for candidate, item in zip(candidates, resolved) if item is not None
    ]
    for index, (candidate, item) in enumerate(resolved_pairs[:8]):
        lines.append(f"#{index} {candidate[0]}: {item.format()}")
    owners = [item.owner for item in resolved if item is not None]
    for owner, symbols in _unresolved_for_owners(object_root, map_path, owners).items():
        lines.append(f"unresolved references from {owner}:")
        for symbol in symbols[:12]:
            lines.append(f"  {symbol}")
        if len(symbols) > 12:
            lines.append(f"  ... (+{len(symbols) - 12} more)")
    return "\n".join(lines), [item for item in resolved if item is not None]


def _crash_detail(map_path: Path, object_root: Path | None, crash: _RuntimeCrash) -> str:
    detail, _ = format_crash_candidates(
        map_path, object_root, [(item.source, item.address) for item in crash.candidates]
    )
    return "\n" + detail if detail else ""


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
        if item is not None and item.symbol is not None:
            entry["symbol"] = f"{item.name or item.symbol.decorated_name}+0x{item.displacement:x}"
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
    map_path: Path | None = None,
) -> RuntimeError:
    artifact_dir = stage / "diagnostics"
    artifact_dir.mkdir(exist_ok=True)
    artifact = artifact_dir / f"{scenario}-failure.txt"
    artifact.write_text(stdout + stderr, encoding="utf-8", errors="replace")
    combined = stdout + stderr
    crash = _parse_runtime_crash(combined)
    if crash is not None:
        detail = _crash_detail(map_path, object_root, crash) if map_path is not None else ""
        return RuntimeError(f"{scenario} failed: {crash.record}{detail}\nartifacts={artifact}")
    failure = RUNTIME_FAILURE.search(combined)
    if failure is not None:
        fields = _parse_diagnostic_fields(failure.group("fields"))
        step = fields.get("step", "unknown")
        reason = fields.get("reason", "unknown")
        line = fields.get("line", "unknown")
        phases = _runtime_phase_summary(combined, scenario)
        phase_detail = f"\nphases: {phases}" if phases else ""
        return RuntimeError(
            f"{scenario} failed: step={step} reason={reason} line={line}"
            f"{phase_detail}\nartifacts={artifact}"
        )
    diagnostic_lines = [line for line in combined.splitlines() if line.strip()]
    last_diagnostic = diagnostic_lines[-1][-500:] if diagnostic_lines else "no diagnostics"
    summary = f"status={returncode if returncode is not None else 'timeout'}: {last_diagnostic}"
    return RuntimeError(f"{scenario} failed: {summary}\nartifacts={artifact}")


def _runtime_phase_summary(output: str, scenario: str) -> str:
    phases: list[str] = []
    for match in RUNTIME_STEP.finditer(output):
        fields = _parse_diagnostic_fields(match.group("fields"))
        if fields.get("scenario") != scenario or "step" not in fields or "elapsed_ms" not in fields:
            continue
        phases.append(f"{fields['step']}={fields['elapsed_ms']}ms")
    return " -> ".join(phases)


def _runtime_history(output: str, scenario: str) -> list[dict[str, Any]]:
    """The case's completed actions, each with the observation it recorded.

    The executable prints one WIZ8_RUNTIME_STEP line per completed action
    carrying the last copied gameplay snapshot when one exists; the history
    keeps identity, timing and that observation, not every field of every
    frame."""
    history: list[dict[str, Any]] = []
    for match in RUNTIME_STEP.finditer(output):
        fields = _parse_diagnostic_fields(match.group("fields"))
        if fields.get("scenario") != scenario or "step" not in fields:
            continue
        entry: dict[str, Any] = {"action": fields["step"]}
        if "elapsed_ms" in fields:
            entry["elapsed_ms"] = int(fields["elapsed_ms"])
        for key in ("screen", "pending", "combat", "motion"):
            if key in fields:
                entry[key] = int(fields[key])
        if "pos" in fields:
            parts = fields["pos"].split(",")
            if len(parts) == 3 and all(part.lstrip("-").isdigit() for part in parts):
                entry["pos"] = [int(part) for part in parts]
        history.append(entry)
    return history


# Keys in a case's observation record that carry no semantic claim: the
# failure text, the action narrative, and any elapsed/timing field. Everything
# else is compared verbatim across order checks and repetitions; a field only
# earns a tolerance below with an explicit justification for why two correct
# runs may legitimately report different values.
_OBSERVATION_VOLATILE_KEYS = frozenset({"failure", "history"})
_OBSERVATION_TOLERANCES: dict[str, int] = {}


def _semantic_observation(observation: Mapping[str, Any]) -> dict[str, Any]:
    return {
        key: value
        for key, value in observation.items()
        if key not in _OBSERVATION_VOLATILE_KEYS and not key.endswith("_ms")
    }


def _observation_differences(baseline: Mapping[str, Any], repeated: Mapping[str, Any]) -> list[str]:
    differences = []
    for key in sorted(set(baseline) | set(repeated)):
        if key not in baseline:
            differences.append(f"{key} only observed in the repetition")
            continue
        if key not in repeated:
            differences.append(f"{key} missing in the repetition")
            continue
        expected, observed = baseline[key], repeated[key]
        if expected == observed:
            continue
        if (
            isinstance(expected, int)
            and isinstance(observed, int)
            and abs(expected - observed) <= _OBSERVATION_TOLERANCES.get(key, 0)
        ):
            continue
        differences.append(f"{key}: {expected!r} != {observed!r}")
    return differences


def _compare_repetitions(scenario: str, outcomes: list[dict[str, Any]]) -> list[str]:
    """What repeated forward runs of one case disagree on.

    A case that flips between pass and fail across repetitions is flaky: the
    earlier failure is already in the suite's failure list, and this report
    adds the repetition detail instead of letting the later pass turn the
    first failure green."""
    problems = []
    passed = [observation for observation in outcomes if "failure" not in observation]
    if 0 < len(passed) < len(outcomes):
        problems.append(
            f"{scenario}: flaky across {len(outcomes)} repetitions: "
            f"{len(outcomes) - len(passed)} failed"
        )
    if len(passed) >= 2:
        baseline = _semantic_observation(passed[0])
        for index, observation in enumerate(passed[1:], start=2):
            differences = _observation_differences(baseline, _semantic_observation(observation))
            if differences:
                problems.append(
                    f"{scenario}: repetition {index} disagrees with the first run: "
                    + "; ".join(differences)
                )
    return problems


def runtime_test_environment(
    settings: Settings, *, prefix: Path | None = None, renderer: str | None = None
) -> tuple[Path, dict[str, str]]:
    if prefix is None:
        prefix = Path(
            os.environ.get("WIZ8_WINE_PREFIX", settings.work_dir / "wine" / "wiz8-runtime")
        )
    prefix.mkdir(parents=True, exist_ok=True)
    # The scenarios never assert audible output. Force the soundless-machine
    # path so Wine's stub audio drivers cannot perturb semantic observations.
    overrides = (
        "winemenubuilder.exe=d;winealsa.drv=d;wineoss.drv=d;winepulse.drv=d;winemm.drv=d;"
        "mmdevapi=d;dsound=d"
    )
    environment = {
        **os.environ,
        "WINEPREFIX": str(prefix),
        "WINEDLLOVERRIDES": overrides,
    }
    if renderer is not None:
        environment["GALLIUM_DRIVER"] = renderer
    environment["WINEDEBUG"] = "-all"
    return prefix, environment


def _initialize_wine_prefix(prefix: Path, environment: dict[str, str]) -> None:
    """Populate a fresh prefix once without the WINEDLLOVERRIDES audio set.

    wine's first-run prefix initialization crashes when WINEDLLOVERRIDES
    disables dsound, so the overrides must not be present until the prefix
    exists; scenario runs keep the full override set afterwards.
    """

    if (prefix / "system.reg").exists():
        return
    clean = {key: value for key, value in environment.items() if key != "WINEDLLOVERRIDES"}
    subprocess.run(
        ["wine", "reg", "query", r"HKCU\Software\Wine"],
        env=clean,
        check=False,
        capture_output=True,
        timeout=120,
    )
    if not (prefix / "system.reg").exists():
        raise RuntimeError(f"wine failed to initialize prefix {prefix}")


def configure_wine_window_management(
    environment: dict[str, str],
    *,
    private_display: bool,
    virtual_desktop: bool | None = None,
) -> None:
    """Match Wine's window ownership and desktop geometry to the selected display.

    Display selection and Wine's virtual-desktop policy are independent:
    ``private_display`` only controls the ``Managed`` driver flag. The Wine
    ``Explorer`` virtual desktop is an explicit opt-in via
    ``WIZ8_WINE_VIRTUAL_DESKTOP`` so a host display maps Wiz8 as an ordinary
    managed window instead of a 640x480 desktop shell.
    """

    if virtual_desktop is None:
        virtual_desktop = os.environ.get("WIZ8_WINE_VIRTUAL_DESKTOP", "").strip().lower() in {
            "1",
            "true",
            "yes",
            "on",
        }

    _initialize_wine_prefix(Path(environment["WINEPREFIX"]), environment)
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
    if not virtual_desktop:
        subprocess.run(
            [
                "wine",
                "reg",
                "delete",
                r"HKCU\Software\Wine\Explorer",
                "/v",
                "Desktop",
                "/f",
            ],
            env=environment,
            check=False,
            timeout=60,
        )
        return
    subprocess.run(
        [
            "wine",
            "reg",
            "add",
            r"HKCU\Software\Wine\Explorer",
            "/v",
            "Desktop",
            "/d",
            "Wizardry",
            "/f",
        ],
        env=environment,
        check=True,
        timeout=60,
    )
    subprocess.run(
        [
            "wine",
            "reg",
            "add",
            r"HKCU\Software\Wine\Explorer\Desktops",
            "/v",
            "Wizardry",
            "/d",
            "640x480",
            "/f",
        ],
        env=environment,
        check=True,
        timeout=60,
    )


def _parse_runtime_observation_fields(fields: str) -> dict[str, str | int]:
    parsed: dict[str, str | int] = {}
    for item in fields.split():
        key, separator, value = item.partition("=")
        if not separator:
            raise RuntimeError(f"malformed runtime observation field: {item}")
        parsed[key] = int(value) if value.lstrip("-").isdigit() else value
    return parsed


def _parse_runtime_observation(stdout: str) -> dict[str, Any]:
    matches = [match for line in stdout.splitlines() if (match := RUNTIME_OBSERVATION.match(line))]
    if len(matches) != 1:
        raise RuntimeError(f"expected one runtime observation, found {len(matches)}")
    return _parse_runtime_observation_fields(matches[0].group("fields"))


def _runtime_session(stdout: str) -> dict[str, str] | None:
    match = RUNTIME_SESSION.search(stdout)
    return _parse_diagnostic_fields(match.group("fields")) if match is not None else None


def _parse_runtime_observations(stdout: str) -> dict[str, dict[str, Any]]:
    """Batch runs emit one WIZ8_RUNTIME_TEST line per completed case."""
    observations: dict[str, dict[str, Any]] = {}
    for line in stdout.splitlines():
        if match := RUNTIME_OBSERVATION.match(line):
            fields = _parse_runtime_observation_fields(match.group("fields"))
            scenario = fields.get("scenario")
            if isinstance(scenario, str):
                observations[scenario] = fields
    return observations


@dataclass
class _RuntimeProcessResult:
    stdout: str
    stderr: str
    returncode: int | None
    timed_out: bool
    failed_early: bool
    last_step: str
    last_step_scenario: str | None
    elapsed: float


def _drive_runtime_process(
    executable: Path,
    stage: Path,
    environment: dict[str, str],
    argv_tail: list[str],
    timeout_seconds: float,
    *,
    batch: bool = False,
) -> _RuntimeProcessResult:
    """Run one runtime-test process. In batch mode per-case WIZ8_RUNTIME_FAILURE
    lines are ordinary case results, not a dying process, so only the crash
    marker arms the early-exit grace."""
    started = time.monotonic()
    output: dict[str, bytearray] = {"stdout": bytearray(), "stderr": bytearray()}
    timed_out = False
    failure_deadline: float | None = None
    pending_stderr = b""
    last_step = "process-start"
    last_step_scenario: str | None = None
    with subprocess.Popen(
        ["wine", f"./{executable.name}", *argv_tail],
        cwd=stage,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ) as process:
        assert process.stdout is not None and process.stderr is not None
        with selectors.DefaultSelector() as selector:
            selector.register(process.stdout, selectors.EVENT_READ, "stdout")
            selector.register(process.stderr, selectors.EVENT_READ, "stderr")
            try:
                while selector.get_map():
                    deadline = started + timeout_seconds
                    if failure_deadline is not None:
                        deadline = min(deadline, failure_deadline)
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        timed_out = failure_deadline is None
                        break
                    for key, _ in selector.select(min(remaining, 0.25)):
                        chunk = os.read(key.fd, 65536)
                        if not chunk:
                            selector.unregister(key.fileobj)
                            continue
                        output[key.data].extend(chunk)
                        if key.data == "stderr":
                            sys.stderr.write(chunk.decode(errors="replace"))
                            sys.stderr.flush()
                            pending_stderr += chunk
                            while b"\n" in pending_stderr:
                                line, pending_stderr = pending_stderr.split(b"\n", 1)
                                if failure_deadline is None and (
                                    line.rstrip(b"\r") == b"WIZ8_RUNTIME_CRASH_END"
                                    or (not batch and line.startswith(b"WIZ8_RUNTIME_FAILURE "))
                                ):
                                    failure_deadline = (
                                        time.monotonic() + RUNTIME_FAILURE_GRACE_SECONDS
                                    )
                                if line.startswith(b"WIZ8_RUNTIME_STEP "):
                                    fields = dict(
                                        item.split("=", 1)
                                        for item in line.decode(errors="replace").split()[1:]
                                        if "=" in item
                                    )
                                    last_step = fields.get("step", last_step)
                                    last_step_scenario = fields.get("scenario")
                if not timed_out:
                    try:
                        deadline = started + timeout_seconds
                        if failure_deadline is not None:
                            deadline = min(deadline, failure_deadline)
                        process.wait(timeout=max(0.001, deadline - time.monotonic()))
                    except subprocess.TimeoutExpired:
                        timed_out = failure_deadline is None
            finally:
                if timed_out or failure_deadline is not None:
                    # A Wine debugger can outlive the executable and retain its pipes/window.
                    # wineserver -k is scoped by WINEPREFIX, so this only retires the
                    # worker's own prefix; the next isolated stage cannot find the
                    # failed scenario's window.
                    # Cleanup is strictly best-effort: its failure must never
                    # replace the scenario's own crash/failure diagnostics.
                    try:
                        subprocess.run(
                            ["wineserver", "-k"],
                            env=environment,
                            check=False,
                            capture_output=True,
                            timeout=5,
                        )
                    except (subprocess.TimeoutExpired, OSError) as cleanup_error:
                        output["stderr"].extend(
                            f"\nruntime-test cleanup: wineserver -k failed: {cleanup_error}\n".encode()
                        )
                if process.poll() is None:
                    process.kill()
                process.wait()
    stdout = output["stdout"].decode(errors="replace")
    stderr = output["stderr"].decode(errors="replace")
    if timed_out:
        stderr += f"\nruntime-test deadline: last_step={last_step}\n"
    return _RuntimeProcessResult(
        stdout=stdout,
        stderr=stderr,
        returncode=process.returncode,
        timed_out=timed_out,
        failed_early=failure_deadline is not None,
        last_step=last_step,
        last_step_scenario=last_step_scenario,
        elapsed=time.monotonic() - started,
    )


def _run_runtime_scenario(
    executable: Path,
    stage: Path,
    environment: dict[str, str],
    scenario: str,
    timeout_seconds: float,
    object_root: Path | None = None,
    map_path: Path | None = None,
) -> dict[str, Any]:
    result = _drive_runtime_process(
        executable,
        stage,
        environment,
        ["--scenario", scenario],
        timeout_seconds,
    )
    if result.timed_out or result.failed_early or result.returncode:
        raise _runtime_failure(
            scenario,
            None if result.timed_out else result.returncode,
            result.stdout,
            result.stderr,
            stage,
            executable,
            object_root,
            map_path,
        )
    try:
        observation = _parse_runtime_observation(result.stdout)
        observation["history"] = _runtime_history(result.stderr, scenario)
    except RuntimeError as error:
        raise _runtime_failure(
            scenario,
            result.returncode,
            result.stdout,
            result.stderr,
            stage,
            executable,
            object_root,
            map_path,
        ) from error
    if observation.get("scenario") != scenario:
        raise _runtime_failure(
            scenario,
            result.returncode,
            result.stdout,
            result.stderr,
            stage,
            executable,
            object_root,
            map_path,
        )
    phases = _runtime_phase_summary(result.stderr, scenario)
    if phases:
        print(f"PHASES {scenario} {phases}", file=sys.stderr, flush=True)
    print(f"PASS {scenario} {result.elapsed:.1f}s", file=sys.stderr, flush=True)
    return observation


def _run_runtime_batch(
    executable: Path,
    stage: Path,
    environment: dict[str, str],
    scenarios: tuple[str, ...],
    registry: dict[str, RuntimeScenario],
    object_root: Path | None = None,
    map_path: Path | None = None,
) -> tuple[dict[str, dict[str, Any]], str | None]:
    """Run one same-process batch. Returns per-case observations plus a batch
    error string when the process died before reporting every case (crash,
    abort, or deadline). Never raises: a poisoned batch must not mask which
    cases actually ran."""
    timeout_seconds = sum(registry[name].timeout_ms for name in scenarios) / 1000 + 60
    result = _drive_runtime_process(
        executable,
        stage,
        environment,
        ["--scenarios", ",".join(scenarios)],
        timeout_seconds,
        batch=True,
    )
    observations = _parse_runtime_observations(result.stdout)
    for name, observation in observations.items():
        observation["history"] = _runtime_history(result.stderr, name)
    missing = [name for name in scenarios if name not in observations]
    error: str | None = None
    if result.timed_out or result.failed_early or missing:
        detail = (
            result.stderr.strip().splitlines()[-1]
            if result.stderr.strip()
            else f"exit={result.returncode}"
        )
        in_flight = result.last_step_scenario or (missing[0] if missing else "unknown")
        error = (
            f"batch process died after {len(observations)}/{len(scenarios)} cases "
            f"(in-flight={in_flight}, exit={result.returncode}, "
            f"timed_out={result.timed_out}): {detail[:400]}"
        )
    else:
        # Every case reported, so a nonzero exit is only acceptable when a
        # reported case itself failed. The session record is the authority
        # on final teardown: without it a bad SGPExit would exit 1 while
        # every case says case_passed=1.
        session = _runtime_session(result.stdout)
        failed = [
            name
            for name, observation in observations.items()
            if observation.get("case_passed") != 1
        ]
        if session is None:
            error = "batch ended without a session record"
        elif session.get("teardown") != "1":
            error = "batch teardown failed (session teardown=0)"
        elif session.get("driver") != "0" and not failed:
            error = "batch driver reported failure with no failed case"
        elif result.returncode not in (0, None) and not failed:
            error = f"batch exited {result.returncode} with every case reporting success"
    for name in observations:
        phases = _runtime_phase_summary(result.stderr, name)
        if phases:
            print(f"PHASES {name} {phases}", file=sys.stderr, flush=True)
    return observations, error


def _pin_suite_executable(settings: Settings, executable: Path, stage: Path) -> tuple[Path, str]:
    """Snapshot the suite executable and its MAP once per invocation.

    Every staged case copies from this pinned pair instead of the live build
    output, so a concurrent relink cannot make one reported suite exercise two
    different binaries. stage_game re-verifies the same immutable pair per
    case, retaining the executable/MAP timestamp check.
    """

    from .build import build_lock

    if not executable.is_file():
        raise RuntimeError(f"runtime executable is not built: {executable}")
    map_file = executable.with_suffix(".map")
    with build_lock(settings):
        executable_bytes = executable.read_bytes()
        map_bytes = map_file.read_bytes() if map_file.is_file() else None
        if map_bytes is not None:
            timestamp = LinkerMap.read(map_file).timestamp
            image = detect_image(executable)
            if (
                not isinstance(image, PEImage)
                or timestamp is None
                or timestamp != image.header.time_date_stamp
            ):
                raise RuntimeError(f"executable/MAP link timestamp mismatch; rebuild {executable}")
        digest = hashlib.sha256(executable_bytes + (map_bytes or b"")).hexdigest()
    pinned = stage / "pinned"
    pinned.mkdir(parents=True, exist_ok=True)
    write_if_changed(pinned / executable.name, executable_bytes)
    if map_bytes is not None:
        write_if_changed(pinned / map_file.name, map_bytes)
    return pinned / executable.name, digest


def run_runtime_suite(
    settings: Settings,
    *,
    scenarios: tuple[str, ...] | None = None,
    tier: str = "pr",
    check_order: bool = False,
    repeat: int = 1,
    renderer: str | None = None,
    batch: bool = False,
    workers: int = 1,
) -> dict[str, Any]:
    """Run selected scenarios, optionally checking reverse-order determinism.

    With batch=True, consecutive batch-eligible cases sharing a fixture run in
    one game process. If a batch process dies mid-group, the unreported cases
    re-run in fresh processes so a poisoned session cannot mask or manufacture
    per-case failures.

    With workers>1, independent jobs run on that many isolated workers: each
    owns a writable stage per case, its own WINEPREFIX (wineserver -k is
    prefix-scoped, so one worker's failure cannot kill another's server), and
    its own virtual display (the game uses real OS input and window focus, so
    sharing a display is unsafe). The suite executable and MAP are pinned once
    per invocation so a concurrent relink cannot mix binaries within a run."""

    suite_started = time.monotonic()
    tiers = ("pr", "main", "nightly")
    if tier not in tiers:
        raise ValueError(f"invalid runtime tier: {tier}")
    if repeat < 1:
        raise ValueError("runtime repeat count must be positive")
    if workers < 1:
        raise ValueError("runtime worker count must be positive")
    if workers > 1 and os.environ.get("WIZ8_RUNTIME_DISPLAY", "virtual") != "virtual":
        raise RuntimeError(
            "workers>1 requires a private virtual display per worker: "
            "unset WIZ8_RUNTIME_DISPLAY or set it to 'virtual'"
        )

    if shutil.which("wine") is None or shutil.which("wineserver") is None:
        raise RuntimeError("wine and wineserver are required to run WIZ8_RUNTIME_TEST")
    stage = settings.runtime_stage("runtime-test")
    stage.mkdir(parents=True, exist_ok=True)
    executable = settings.product_build_dir / "Wiz8RuntimeTest.exe"
    object_root = settings.recovered_objects_dir
    pinned_executable, input_digest = _pin_suite_executable(settings, executable, stage)

    base_prefix = os.environ.get(
        "WIZ8_WINE_PREFIX", str(settings.work_dir / "wine" / "wiz8-runtime")
    )
    prefixes: list[Path] = []
    environments: list[dict[str, str]] = []
    for index in range(workers):
        prefix = Path(base_prefix if workers == 1 else f"{base_prefix}-w{index}")
        prefixes.append(prefix)
        environments.append(runtime_test_environment(settings, prefix=prefix, renderer=renderer)[1])

    runs: dict[str, dict[str, dict[str, Any]]] = {}
    scenario_stages: dict[str, str] = {}
    failures: list[str] = []
    timings: dict[str, dict[str, dict[str, Any]]] = {}

    # Read the scenario registry once on worker 0's prefix; the display context
    # is short-lived — each worker opens its own before running cases.
    with runtime_display(
        environments[0], default="virtual", log_path=stage / "xvfb-registry.log"
    ) as display:
        configure_wine_window_management(environments[0], private_display=display is not None)
        registry_stage = stage_game(
            settings,
            name="runtime-test/registry",
            executable=pinned_executable,
            objects=object_root,
            reset_saves=True,
            input_pinned=True,
        )
        registry = _read_runtime_scenarios(
            registry_stage.executable, registry_stage.root, environments[0]
        )
    if scenarios is None:
        scenarios = tuple(
            name for name, spec in registry.items() if tiers.index(spec.tier) <= tiers.index(tier)
        )
    if not scenarios or set(scenarios) - registry.keys():
        raise ValueError(f"invalid runtime scenario selection: {scenarios}")

    orders = []
    comparisons = []
    for repetition in range(1, repeat + 1):
        suffix = f"-{repetition}" if repeat > 1 else ""
        forward, reverse = f"forward{suffix}", f"reverse{suffix}"
        orders.append((forward, scenarios))
        if check_order:
            orders.append((reverse, tuple(reversed(scenarios))))
            comparisons.append((forward, reverse))

    # A job is one process launch: a singleton case, or a batch group of
    # consecutive batch-eligible cases sharing one fixture.
    jobs: list[tuple[str, tuple[str, ...]]] = []
    for order_name, ordered_scenarios in orders:
        runs[order_name] = {}
        timings[order_name] = {}
        groups: list[tuple[str, ...]] = []
        for name in ordered_scenarios:
            spec = registry[name]
            if (
                batch
                and spec.batch
                and groups
                and registry[groups[-1][-1]].batch
                and registry[groups[-1][-1]].fixture == spec.fixture
            ):
                groups[-1] = (*groups[-1], name)
            else:
                groups.append((name,))
        for group in groups:
            jobs.append((order_name, group))

    def run_group(
        order_name: str, group: tuple[str, ...], environment: dict[str, str]
    ) -> dict[str, Any]:
        job: dict[str, Any] = {"runs": {}, "stages": {}, "timings": {}, "failures": []}

        def run_single(scenario: str, stage_key: str, stage_name: str) -> None:
            scenario_stage = stage / stage_key
            shutil.rmtree(scenario_stage, ignore_errors=True)
            stage_started = time.monotonic()
            staged = stage_game(
                settings,
                name=f"runtime-test/{stage_key}",
                executable=pinned_executable,
                objects=object_root,
                reset_saves=True,
                input_pinned=True,
            )
            stage_seconds = time.monotonic() - stage_started
            job["stages"][scenario] = str(staged.root)
            run_started = time.monotonic()
            try:
                job["runs"][scenario] = _run_runtime_scenario(
                    staged.executable,
                    staged.root,
                    environment,
                    scenario,
                    registry[scenario].timeout_ms / 1000,
                    object_root,
                    staged.map,
                )
            except RuntimeError as error:
                failure = str(error)
                job["failures"].append(f"{stage_name}: {failure}")
                job["runs"][scenario] = {
                    "scenario": scenario,
                    "failure": failure,
                }
                print(
                    f"FAIL {scenario} ({stage_name}): {failure}",
                    file=sys.stderr,
                    flush=True,
                )
            job["timings"][scenario] = {
                "stage_seconds": stage_seconds,
                "process_seconds": time.monotonic() - run_started,
            }

        if len(group) == 1:
            scenario = group[0]
            print(f"RUN {scenario} ({order_name})", file=sys.stderr, flush=True)
            run_single(scenario, f"{order_name}/{scenario}", f"{order_name}/{scenario}")
            return job

        batch_stage_key = f"{order_name}/batch-{group[0]}-{group[-1]}"
        batch_stage = stage / batch_stage_key
        shutil.rmtree(batch_stage, ignore_errors=True)
        stage_started = time.monotonic()
        staged = stage_game(
            settings,
            name=f"runtime-test/{batch_stage_key}",
            executable=pinned_executable,
            objects=object_root,
            reset_saves=True,
            input_pinned=True,
        )
        stage_seconds = time.monotonic() - stage_started
        print(
            f"RUN batch [{', '.join(group)}] ({order_name})",
            file=sys.stderr,
            flush=True,
        )
        run_started = time.monotonic()
        observations, batch_error = _run_runtime_batch(
            staged.executable,
            staged.root,
            environment,
            group,
            registry,
            object_root,
            staged.map,
        )
        process_seconds = time.monotonic() - run_started
        for scenario in group:
            job["stages"][scenario] = str(staged.root)
            job["timings"][scenario] = {
                "stage_seconds": stage_seconds,
                "process_seconds": process_seconds,
                "shared_process": batch_stage_key,
            }
        if batch_error is not None:
            job["failures"].append(f"{order_name}/batch[{','.join(group)}]: {batch_error}")
        for scenario in group:
            observation = observations.get(scenario)
            if observation is not None:
                job["runs"][scenario] = observation
                if observation.get("case_passed") == 1:
                    print(
                        f"PASS {scenario} ({order_name}, batch)",
                        file=sys.stderr,
                        flush=True,
                    )
                else:
                    failure = f"{scenario} failed in same-process batch"
                    job["failures"].append(f"{order_name}/{scenario}: {failure}")
                    print(
                        f"FAIL {scenario} ({order_name}): {failure}",
                        file=sys.stderr,
                        flush=True,
                    )
                continue
            # Unreported: crashed, aborted, or never reached. The case did not
            # fail — the batch did — so re-run it in a fresh process and
            # report its own result.
            print(
                f"RE-RUN {scenario} ({order_name}): no result in batch",
                file=sys.stderr,
                flush=True,
            )
            run_single(scenario, f"{order_name}/{scenario}", f"{order_name}/{scenario}")
        return job

    pending: queue.Queue[int | None] = queue.Queue()
    for job_index in range(len(jobs)):
        pending.put(job_index)
    for _ in range(workers):
        pending.put(None)
    results: list[dict[str, Any] | None] = [None] * len(jobs)
    worker_errors: list[tuple[int, BaseException]] = []

    def worker(index: int, environment: dict[str, str]) -> None:
        try:
            with runtime_display(
                environment,
                default="virtual",
                log_path=stage / f"xvfb-worker-{index}.log",
            ) as display:
                configure_wine_window_management(environment, private_display=display is not None)
                while True:
                    job_index = pending.get()
                    try:
                        if job_index is None:
                            return
                        order_name, group = jobs[job_index]
                        results[job_index] = run_group(order_name, group, environment)
                    finally:
                        pending.task_done()
        except Exception as error:  # noqa: BLE001 — surfaced as a suite error after join
            worker_errors.append((index, error))
        finally:
            try:
                subprocess.run(
                    ["wineserver", "-k"],
                    cwd=stage,
                    env=environment,
                    check=False,
                    capture_output=True,
                )
            except OSError as cleanup_error:
                print(
                    f"runtime-test cleanup: wineserver -k failed: {cleanup_error}",
                    file=sys.stderr,
                    flush=True,
                )

    if workers == 1:
        worker(0, environments[0])
    else:
        threads = [
            threading.Thread(
                target=worker,
                args=(index, environments[index]),
                name=f"runtime-worker-{index}",
            )
            for index in range(workers)
        ]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
    if worker_errors:
        index, error = worker_errors[0]
        raise RuntimeError(f"runtime worker {index} failed: {error}")

    for job_index, (order_name, group) in enumerate(jobs):
        result = results[job_index]
        if result is None:
            failures.append(f"{order_name}/[{','.join(group)}]: job never ran (worker aborted)")
            continue
        runs[order_name].update(result["runs"])
        timings[order_name].update(result["timings"])
        for scenario, staged_path in result["stages"].items():
            scenario_stages[f"{order_name}/{scenario}"] = staged_path
        failures.extend(result["failures"])

    if repeat > 1:
        forward_orders = [f"forward-{index}" for index in range(1, repeat + 1)]
        for scenario in scenarios:
            outcomes = [
                runs[order][scenario] for order in forward_orders if scenario in runs[order]
            ]
            if len(outcomes) > 1:
                failures.extend(_compare_repetitions(scenario, outcomes))

    for forward, reverse in comparisons:
        if any(
            _semantic_observation(observation) != _semantic_observation(runs[reverse][scenario])
            for scenario, observation in runs[forward].items()
            if "failure" not in observation and "failure" not in runs[reverse][scenario]
        ):
            failures.append(f"runtime observations depend on scenario order: {forward}/{reverse}")

    staging_seconds = 0.0
    execution_seconds = 0.0
    shared_processes: set[str] = set()
    for order_timings in timings.values():
        for entry in order_timings.values():
            shared = entry.get("shared_process")
            if shared is not None:
                if shared in shared_processes:
                    continue
                shared_processes.add(shared)
            staging_seconds += entry["stage_seconds"]
            execution_seconds += entry["process_seconds"]

    elapsed = time.monotonic() - suite_started
    print(
        f"RUNTIME_SUITE scenarios={sum(len(run) for run in runs.values())} "
        f"failures={len(failures)} elapsed_s={elapsed:.1f} "
        f"stage_s={staging_seconds:.1f} run_s={execution_seconds:.1f} "
        f"workers={workers} "
        f"renderer={environments[0].get('GALLIUM_DRIVER', 'default')}",
        file=sys.stderr,
        flush=True,
    )
    if failures:
        raise RuntimeError("runtime suite failures:\n\n" + "\n\n".join(failures))
    return {
        "stage": str(stage),
        "executable": str(executable),
        "input_digest": input_digest,
        "objects": str(object_root),
        "scenario_stages": scenario_stages,
        "wine_prefix": str(prefixes[0]),
        "wine_prefixes": [str(prefix) for prefix in prefixes],
        "workers": workers,
        "runs": runs,
        "timings": {
            "staging_seconds": staging_seconds,
            "execution_seconds": execution_seconds,
            "scenarios": timings,
        },
        "elapsed_seconds": elapsed,
        "deterministic": True if check_order else None,
    }
