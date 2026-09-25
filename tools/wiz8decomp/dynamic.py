"""A scenario-bounded dynamic oracle: watch the original run, from evidence.

Everything else in this repository reasons about the image at rest. That is
where most of the evidence is, but some questions only the running program
answers: which gates actually run and in what order, which screen handler the
dispatcher reaches, whether a recompiled body is reached at all. This runs the
original under Wine with a debugger attached and turns the addresses the
reviewed model already knows into an event stream.

Three properties keep it honest.

**The breakpoints come from canonical owners.** A trace plan is generated from
compiler-bound source markers and the original frame-dispatch observation, so
it can be regenerated after either model changes rather than drifting away.

**A claim is bounded by the scenario that produced it.** An event stream says
what happened in *this* run to *this* point - it never says a function is
unreachable, only that this scenario did not reach it. Every recorded stream
carries the scenario that produced it.

**Comparison is by name, across builds.** Two builds put the same function at
different addresses, so streams are compared on the reviewed name each
breakpoint carries. The first divergence is the answer; the counts after it are
noise, because one extra event shifts everything that follows.

The runtime side is deliberately thin: `winedbg --gdb` proxies the Windows
process to an ordinary gdb, which prints one line per hit and continues. There
is no in-process agent, nothing is injected into the image, and the game runs
from a copy so the immutable input trees are never written to.
"""

from __future__ import annotations

import csv
import hashlib
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .paths import json_hash, sha256_file
from .subprocesses import tool_version

EVENT = re.compile(r"^EVENT\s+(?P<kind>\S+)\s+(?P<name>\S+)\s+(?P<address>[0-9a-f]{8})\s*$")
READY = "TRACE_READY"
STATE = re.compile(r"^STATE\s+(?P<name>\S+)\s+(?P<value>\S+)\s*$")

# Scenario names are part of every claim this module makes, so they are fixed
# here rather than invented per run.
BRING_UP = "bring-up"
SCREENS = "screens"
LOAD = "load"
SMOKE = "smoke"

# A scenario's launch contract is part of the claim too: /LOAD makes the
# product load the newest quicksave during startup instead of reaching the
# menu, which gives the differential a deterministic entry with no input
# injection at all.
SCENARIO_ARGUMENTS: dict[str, tuple[str, ...]] = {
    BRING_UP: (),
    SCREENS: (),
    LOAD: ("/LOAD",),
    SMOKE: (),
}

# The first event of the steady state a scenario's claim ends at: the
# semantic window runs through this event's first occurrence, and the
# periodic frame loop after it is capture noise. For /LOAD that is the first
# main-game tick - the save is loaded and the world is live. For smoke the
# process's own exit path: SGPExit is the last product-side lifecycle event.
SCENARIO_TERMINAL: dict[str, str] = {LOAD: "ProcessMainGameAutoSave", SMOKE: "SGPExit"}

# Scalar words the checkpoint fingerprint reads when the terminal event first
# fires, as (label, reviewed global, byte offset, indirect). A fingerprint
# compares state, never addresses: pointer-bearing fields would legitimately
# differ across builds, so every entry resolves one 32-bit word either inside
# the global itself or through the pointer it holds. Offsets follow the
# source-modeled layouts - W8ScreenStateRuntime and GDCamera - and the same
# offsets read both images because the layout is authored, not relocated.
# For /LOAD the settled screen-state records and the world camera's placement
# are the durable effects the load produced.
_STATE_PROBES: dict[str, tuple[tuple[str, str, int, bool], ...]] = {
    LOAD: (
        ("screen.id", "g_current_screen_state", 0x00, False),
        ("screen.mode", "g_current_screen_state", 0x04, False),
        ("screen.parameter", "g_current_screen_state", 0x08, False),
        ("screen.parameter_2", "g_current_screen_state", 0x0C, False),
        ("pending.id", "g_pending_screen_state", 0x00, False),
        ("pending.mode", "g_pending_screen_state", 0x04, False),
        ("camera.yaw", "g_gd_camera", 0x04, True),
        ("camera.pitch", "g_gd_camera", 0x08, True),
        ("camera.x", "g_gd_camera", 0x8C, True),
        ("camera.y", "g_gd_camera", 0x90, True),
        ("camera.z", "g_gd_camera", 0x94, True),
    ),
}


@dataclass(frozen=True)
class TracePoint:
    """One address to watch, and what the ledger calls it."""

    address: str
    name: str
    kind: str


@dataclass(frozen=True)
class Event:
    """One breakpoint hit, in the order the run produced it."""

    order: int
    kind: str
    name: str
    address: str


@dataclass(frozen=True)
class StateProbe:
    """One scalar word the fingerprint reads at the terminal checkpoint."""

    name: str  # STATE label and comparison key
    global_name: str  # reviewed GLOBAL-marked object the base resolves through
    address: str  # resolved absolute base address for the traced image
    offset: int  # byte offset of the word inside the object
    indirect: bool  # the base holds a pointer; read through it


def bring_up_points(repo: Path) -> list[TracePoint]:
    """Source-owned startup functions, derived from physical TU ownership."""

    from .source_index import source_functions

    points = []
    for function in source_functions(repo).values():
        path = Path(function.source_file)
        if not (path.stem.startswith("startup_") or path.name in {"game_init.cpp", "winmain.cpp"}):
            continue
        points.append(
            TracePoint(address=f"{function.address:08x}", name=function.name, kind="gate")
        )
    return sorted(points, key=lambda point: point.address)


def screen_points(repo: Path) -> list[TracePoint]:
    """The dispatcher's handlers, named by the state index that reaches them.

    A handler firing is a screen transition: the dispatcher is indexed by the
    application's state, so the handler identifies the state. Folded stubs are
    left out - the linker merged several trivial handlers into one address, and
    a hit there cannot say which state it belonged to.
    """

    slots: dict[str, list[tuple[int, str]]] = {}
    kinds: dict[str, str] = {}
    with (repo / "evidence" / "observations" / "wiz8" / "frame-dispatch-table.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        for row in csv.DictReader(stream):
            slots.setdefault(row["handler_address"], []).append((int(row["state"]), row["role"]))
            kinds[row["handler_address"]] = row["kind"]
    return sorted(
        (
            TracePoint(
                address=address,
                name="screen" + "".join(f"_{state}_{role}" for state, role in sorted(states)),
                kind="screen",
            )
            for address, states in slots.items()
            if kinds[address] == "handler"
        ),
        key=lambda point: point.address,
    )


def load_points(repo: Path) -> list[TracePoint]:
    """Recovered save/loading functions: the gates a /LOAD run must reach.

    Derived from the same physical TU ownership as the startup points, so the
    plan regenerates when the loading chain's recovery moves."""

    from .source_index import source_functions

    points = []
    for function in source_functions(repo).values():
        if Path(function.source_file).name != "LoadSaveGame.cpp":
            continue
        points.append(
            TracePoint(address=f"{function.address:08x}", name=function.name, kind="load")
        )
    return sorted(points, key=lambda point: point.address)


def smoke_points(repo: Path) -> list[TracePoint]:
    """The product's own entry/exit lifecycle: WinMain, the menu, the quit
    path, SGPExit. The runtime-test executable also calls WinMain, but exits
    through an explicit SGPExit plus TerminateProcess; the smoke scenario
    exists because the runnable product's real entry and real exit are not
    the same evidence."""

    from .source_index import source_functions

    lifecycle = {"WinMain", "SGPExit"}
    points = [
        TracePoint(address=f"{function.address:08x}", name=function.name, kind="lifecycle")
        for function in source_functions(repo).values()
        if function.name in lifecycle
    ]
    points += [
        point
        for point in screen_points(repo)
        if point.name
        in {
            "screen_0_enter",
            "screen_0_frame",
            "screen_1_enter",
            "screen_12_enter",
        }
    ]
    return sorted(points, key=lambda point: point.address)


def trace_plan(repo: Path, scenario: str) -> list[TracePoint]:
    if scenario == BRING_UP:
        return bring_up_points(repo)
    if scenario == SCREENS:
        return bring_up_points(repo) + screen_points(repo)
    if scenario == LOAD:
        return bring_up_points(repo) + screen_points(repo) + load_points(repo)
    if scenario == SMOKE:
        return smoke_points(repo)
    raise ValueError(
        f"unknown scenario: {scenario}; expected {BRING_UP}, {SCREENS}, {LOAD} or {SMOKE}"
    )


def verified_link_map(image: Path, link_map_path: Path) -> Any:
    """The rebuilt image's own linker map - and proof it belongs to that image.

    A MAP from another build would rebase every breakpoint to wrong
    addresses and silently trace nothing useful, so the link timestamps must
    agree before the plan is translated."""

    from reccmp.formats import detect_image
    from reccmp.formats.pe import PEImage

    from .binary.linker_map import LinkerMap

    link_map = LinkerMap.read(link_map_path)
    detected = detect_image(image)
    if (
        not isinstance(detected, PEImage)
        or link_map.timestamp is None
        or link_map.timestamp != detected.header.time_date_stamp
    ):
        raise ValueError(f"executable/MAP link timestamp mismatch: {image} vs {link_map_path}")
    return link_map


def _bare_symbol_name(demangled: str) -> str:
    """The qualified name a demangled symbol spells: no access, return type,
    calling convention or parameter list. The name itself may contain spaces
    (template arguments), so the prefix ends at the convention keyword, not
    at a space."""

    text = demangled
    if text.startswith(("public:", "private:", "protected:")):
        text = text.split(":", 1)[1].strip()
    convention = re.search(r"__cdecl\s+|__thiscall\s+|__stdcall\s+|__fastcall\s+", text)
    if convention is not None:
        text = text[convention.end() :]
    return text.split("(", 1)[0].strip()


def _map_functions_by_name(link_map: Any) -> dict[str, list[Any]]:
    """The map's function symbols indexed by their canonical (undecorated)
    name - the identity the stream comparison actually uses."""

    from .binary.linker_map import demangle_names

    decorated = [symbol.decorated_name for symbol in link_map.symbols if symbol.is_function]
    demangled = demangle_names(decorated)
    by_name: dict[str, list[Any]] = {}
    for symbol in link_map.symbols:
        if not symbol.is_function:
            continue
        bare = _bare_symbol_name(demangled.get(symbol.decorated_name) or symbol.decorated_name)
        by_name.setdefault(bare, []).append(symbol)
    return by_name


def rebase_plan(
    repo: Path, points: list[TracePoint], link_map: Any
) -> tuple[list[TracePoint], list[str]]:
    """Translate a retail plan into a rebuilt image's addresses, by name.

    Two builds put the same function at different addresses; the comparison
    is by name, so each build's plan resolves the reviewed identity through
    its own linker map. Points the rebuilt image does not carry (unrecovered
    functions) cannot be watched and are reported, not silently dropped."""

    from .source_index import source_functions

    functions = source_functions(repo)
    semantic_ids = {
        address: function.declaration.semantic_id
        for address, function in functions.items()
        if function.declaration is not None
    }
    rebased = []
    dropped = []
    by_name: dict[str, list[Any]] | None = None
    for point in points:
        address = int(point.address, 16)
        semantic_id = semantic_ids.get(address)
        symbol = link_map.find_decorated(semantic_id) if semantic_id is not None else None
        if symbol is None:
            # The linker may keep another unit's instantiation or spell the
            # declaration differently: retry on the canonical name, which is
            # the identity the comparison uses anyway. Ambiguous candidates
            # stay dropped rather than binding to an arbitrary emission.
            if by_name is None:
                by_name = _map_functions_by_name(link_map)
            function = functions.get(address)
            candidates = by_name.get(function.name, []) if function is not None else []
            if len(candidates) == 1:
                symbol = candidates[0]
        if symbol is None:
            dropped.append(point.name)
            continue
        rebased.append(
            TracePoint(address=f"{symbol.address:08x}", name=point.name, kind=point.kind)
        )
    return rebased, dropped


def _global_addresses(repo: Path, names: set[str]) -> dict[str, int]:
    """Reviewed GLOBAL marker addresses for the retail image, by object name."""

    from .global_model import parse_global_definitions

    return {
        definition["name"]: definition["address"]
        for definition in parse_global_definitions(repo)
        if definition["name"] in names and definition["target"] == "WIZ8"
    }


def _global_semantic_ids(repo: Path, names: set[str]) -> dict[str, str]:
    """Decorated names of the reviewed globals, for rebuilt-image rebasing."""

    from .source_index import SourceIndex, load_source_index

    index = SourceIndex.from_dict(load_source_index(repo))
    return {
        variable.qualified_name: variable.semantic_id
        for key, variable in index.variables.items()
        if variable.qualified_name in names
        and key.target == "WIZ8"
        and variable.definition_kind == "definition"
    }


def state_probes(repo: Path, scenario: str) -> tuple[list[StateProbe], list[str]]:
    """The scenario's fingerprint reads resolved to this image's globals.

    A missing GLOBAL marker is reported, not dropped: a fingerprint that
    reads nothing proves nothing about the state it claims to compare."""

    specs = _STATE_PROBES.get(scenario, ())
    names = {global_name for _, global_name, _, _ in specs}
    addresses = _global_addresses(repo, names)
    probes = []
    unwatched = []
    for name, global_name, offset, indirect in specs:
        address = addresses.get(global_name)
        if address is None:
            unwatched.append(global_name)
            continue
        probes.append(StateProbe(name, global_name, f"{address:08x}", offset, indirect))
    return probes, unwatched


def rebase_probes(
    repo: Path, probes: list[StateProbe], link_map: Any
) -> tuple[list[StateProbe], list[str]]:
    """Translate fingerprint bases into the rebuilt image, by decorated name.

    Globals move between builds like functions do, so each build's probes
    resolve through that build's linker map; a global the image does not
    carry is reported rather than read at a wrong address."""

    semantic_ids = _global_semantic_ids(repo, {probe.global_name for probe in probes})
    rebased = []
    dropped = []
    for probe in probes:
        semantic_id = semantic_ids.get(probe.global_name)
        symbol = link_map.find_decorated(semantic_id) if semantic_id is not None else None
        if symbol is None:
            dropped.append(probe.global_name)
            continue
        rebased.append(
            StateProbe(
                name=probe.name,
                global_name=probe.global_name,
                address=f"{symbol.address:08x}",
                offset=probe.offset,
                indirect=probe.indirect,
            )
        )
    return rebased, dropped


def _state_lines(probes: list[StateProbe]) -> list[str]:
    """Breakpoint commands that print the checkpoint fingerprint.

    Indirect probes share one null check on their base pointer: an unset
    object reads as `missing` rather than faulting the inferior's trace."""

    lines = []
    groups: dict[tuple[str, bool], list[StateProbe]] = {}
    for probe in probes:
        groups.setdefault((probe.address, probe.indirect), []).append(probe)
    for (address, indirect), group in groups.items():
        if not indirect:
            for probe in group:
                lines.append(
                    f'printf "STATE {probe.name} 0x%08x\\n", '
                    f"*(unsigned int*)(0x{address} + {probe.offset})"
                )
            continue
        lines.append(f"if *(unsigned int*)0x{address} != 0")
        for probe in group:
            lines.append(
                f'printf "STATE {probe.name} 0x%08x\\n", '
                f"*(unsigned int*)(*(unsigned int*)0x{address} + {probe.offset})"
            )
        lines.append("else")
        for probe in group:
            lines.append(f'printf "STATE {probe.name} missing\\n"')
        lines.append("end")
    return lines


@dataclass(frozen=True)
class BreakpointAction:
    """Commands injected into a named breakpoint's command block.

    `every` unset runs the lines once, at the first hit: a state checkpoint,
    or the gesture that drives the quit path. An integer runs them every
    Nth hit - the intro screen needs Escape pressed periodically while its
    videos play, the same cadence the runtime harness re-arms on."""

    lines: tuple[str, ...]
    every: int | None = None


def gdb_script(
    points: list[TracePoint],
    port: int,
    actions: dict[str, BreakpointAction] | None = None,
) -> str:
    """A batch script that prints one line per hit and never stops the run.

    Each action block guards on its own convenience variable so a point
    that fires every frame does not repeat a once-only action, and a
    periodic action fires on every Nth hit rather than all of them."""

    lines = [
        "set confirm off",
        "set pagination off",
        "set height 0",
        "set width 0",
        f"target remote localhost:{port}",
    ]
    for name in actions or {}:
        lines.append(f"set $action_{re.sub(r'[^A-Za-z0-9_]', '_', name)} = 0")
    for point in points:
        lines += [
            f"break *0x{point.address}",
            "commands",
            "silent",
            f'printf "EVENT {point.kind} {point.name} {point.address}\\n"',
        ]
        injected = (actions or {}).get(point.name)
        if injected:
            guard = f"$action_{re.sub(r'[^A-Za-z0-9_]', '_', point.name)}"
            if injected.every is None:
                lines += [f"if {guard} == 0", f"set {guard} = 1", *injected.lines, "end"]
            else:
                lines += [
                    f"set {guard} = {guard} + 1",
                    f"if {guard} % {injected.every} == 0",
                    *injected.lines,
                    "end",
                ]
        lines += ["continue", "end"]
    lines += [f'printf "{READY}\\n"', "continue"]
    return "\n".join(lines) + "\n"


def parse_events(output: str) -> list[Event]:
    """The event stream, in order, ignoring everything gdb says around it."""

    events = []
    for line in output.splitlines():
        match = EVENT.match(line.strip())
        if match is not None:
            events.append(
                Event(
                    order=len(events),
                    kind=match.group("kind"),
                    name=match.group("name"),
                    address=match.group("address"),
                )
            )
    return events


def parse_state(output: str) -> dict[str, str]:
    """The checkpoint fingerprint a run emitted, first write per field."""

    state: dict[str, str] = {}
    for line in output.splitlines():
        match = STATE.match(line.strip())
        if match is not None and match.group("name") not in state:
            state[match.group("name")] = match.group("value")
    return state


def compare_states(left: dict[str, str], right: dict[str, str]) -> dict[str, Any]:
    """Field-level fingerprint agreement. A field absent on either side is a
    divergence, not a skipped check: the checkpoint promised to read it."""

    names = sorted(set(left) | set(right))
    diffs = [
        {"name": name, "left": left.get(name, "<absent>"), "right": right.get(name, "<absent>")}
        for name in names
        if left.get(name) != right.get(name)
    ]
    return {
        "agrees": not diffs,
        "fields": len(names),
        "diffs": diffs,
        "detail": f"{len(names) - len(diffs)}/{len(names)} fields identical"
        if names
        else "no fingerprint fields",
    }


def compare_streams(left: list[Event], right: list[Event]) -> dict[str, Any]:
    """Where two runs first disagree, by name rather than by address.

    Only the first divergence is a finding. Everything after it is a
    consequence: one extra or missing event shifts the whole tail, and
    reporting that tail as further differences would multiply one fact.
    """

    limit = min(len(left), len(right))
    for index in range(limit):
        if left[index].name != right[index].name:
            return {
                "agrees": False,
                "common_prefix": index,
                "left": left[index].name,
                "right": right[index].name,
                "detail": f"event {index} is {left[index].name} in one run and "
                f"{right[index].name} in the other",
            }
    if len(left) != len(right):
        longer = left if len(left) > len(right) else right
        return {
            "agrees": False,
            "common_prefix": limit,
            "left": len(left),
            "right": len(right),
            "detail": (
                f"the runs agree for {limit} events and then one continues with "
                f"{longer[limit].name}"
            ),
        }
    return {"agrees": True, "common_prefix": limit, "detail": f"{limit} events, identical"}


@dataclass(frozen=True)
class Sandbox:
    """Where the traced copy of the game lives. Never an input tree.

    The variant trees are hardlinked into the work directory, so a game that
    writes its configuration back would modify the canonical input through the
    shared inode. The sandbox is a real copy for that reason alone.
    """

    game_dir: Path
    prefix: Path
    display: str

    @classmethod
    def from_environment(cls) -> Sandbox:
        root = os.environ.get("WIZ8_DYNAMIC_DIR")
        if not root:
            raise ValueError(
                "WIZ8_DYNAMIC_DIR is unset; it must name a directory holding a "
                "`game/` copy of a variant and a `prefix/` Wine prefix"
            )
        base = Path(root)
        return cls(
            game_dir=base / "game",
            prefix=base / "prefix",
            display=os.environ.get("WIZ8_DYNAMIC_DISPLAY", ":99"),
        )

    def environment(self) -> dict[str, str]:
        return {
            **os.environ,
            "WINEPREFIX": str(self.prefix),
            "DISPLAY": self.display,
            "WINEDEBUG": "-all",
        }

    def windows_path(self, name: str) -> str:
        return "Z:" + str(self.game_dir / name).replace("/", "\\")

    def install_save(self, source: Path) -> Path:
        """Stage a fixture as the only quicksave the loader can select.

        /LOAD picks the newest ``Saves\\Quick*.SAV``; leaving other quicksaves
        in place, or preserving the fixture's name/mtime, could make it load
        something else. Clear the slots and stage exactly one known file."""

        saves = self.game_dir / "Saves"
        saves.mkdir(exist_ok=True)
        for stale in saves.glob("Quick*.SAV"):
            stale.unlink()
        destination = saves / "Quick 1.SAV"
        shutil.copyfile(source, destination)
        return destination


def _listening(port: int, deadline: float) -> bool:
    """Wait for the proxy's port without connecting to it.

    `winedbg --gdb` accepts exactly one connection, so a probe that connects
    consumes the one gdb needs - which presents as gdb timing out against a
    port that is demonstrably open.
    """

    while time.monotonic() < deadline:
        result = subprocess.run(
            ["ss", "-ltnH", f"sport = :{port}"], capture_output=True, text=True, check=False
        )
        if "LISTEN" in result.stdout:
            return True
        time.sleep(0.5)
    return False


def _allocate_port() -> int:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as listener:
        listener.bind(("127.0.0.1", 0))
        return int(listener.getsockname()[1])


def _terminate_process_group(process: subprocess.Popen[Any]) -> None:
    """Terminate only the group created for this trace."""

    if process.poll() is not None:
        return
    try:
        os.killpg(process.pid, signal.SIGTERM)
        process.wait(timeout=10)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait(timeout=10)


def _repository_revision(repo: Path) -> str:
    completed = subprocess.run(
        ["jj", "log", "-r", "@", "--no-graph", "-T", "commit_id"],
        cwd=repo,
        capture_output=True,
        text=True,
        check=False,
    )
    return completed.stdout.strip() if completed.returncode == 0 else "unavailable"


def _reviewed_evidence_hash(repo: Path) -> str:
    digest = hashlib.sha256()
    roots = [repo / "evidence" / "reviewed", repo / "evidence" / "observations" / "wiz8"]
    for path in sorted(
        (path for root in roots for path in root.rglob("*") if path.is_file()),
        key=lambda item: item.relative_to(repo).as_posix(),
    ):
        digest.update(path.relative_to(repo).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(sha256_file(path).encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def _text(value: str | bytes | None) -> str:
    if value is None:
        return ""
    return value.decode("utf-8", "replace") if isinstance(value, bytes) else value


def run_trace(
    repo: Path,
    sandbox: Sandbox,
    scenario: str,
    seconds: int = 120,
    port: int | None = None,
    *,
    executable: str = "Wiz8.exe",
    link_map: Path | None = None,
    arguments: list[str] | None = None,
    save: Path | None = None,
) -> dict[str, Any]:
    """Run one scenario under the debugger and return its event stream."""

    for tool in ("winedbg", "wineserver", "gdb", "ss"):
        if shutil.which(tool) is None:
            raise ValueError(f"{tool} is not on PATH; the dynamic oracle needs it")
    if not (sandbox.game_dir / executable).is_file():
        raise ValueError(f"no {executable} in {sandbox.game_dir}")
    if scenario == LOAD and save is None:
        raise ValueError("the load scenario requires --save: stage one known fixture")

    points = trace_plan(repo, scenario)
    unwatched: list[str] = []
    if link_map is not None:
        # A rebuilt image needs its own addresses; the plan keeps the
        # reviewed retail names and resolves each through this build's map,
        # which must provably belong to the traced image.
        resolved_map = verified_link_map(sandbox.game_dir / executable, link_map)
        points, unwatched = rebase_plan(repo, points, resolved_map)
    launch_arguments = (
        list(arguments) if arguments is not None else list(SCENARIO_ARGUMENTS[scenario])
    )
    fixture: dict[str, Any] | None = None
    if save is not None:
        staged_save = sandbox.install_save(save)
        fixture = {
            "name": save.name,
            "sha256": sha256_file(staged_save),
        }
    selected_port = port if port is not None else _allocate_port()
    plan_hash = json_hash(
        [{"address": point.address, "name": point.name, "kind": point.kind} for point in points]
    )
    probes, probe_unwatched = state_probes(repo, scenario)
    unwatched += probe_unwatched
    if link_map is not None:
        probes, probe_rebase_unwatched = rebase_probes(repo, probes, resolved_map)
        unwatched += probe_rebase_unwatched
    terminal = SCENARIO_TERMINAL.get(scenario)
    actions = (
        {terminal: BreakpointAction(lines=tuple(_state_lines(probes)))}
        if terminal is not None and probes
        else {}
    )

    script = sandbox.game_dir.parent / f"trace-{scenario}-{selected_port}.gdb"
    script.write_text(gdb_script(points, selected_port, actions=actions), encoding="utf-8")

    proxy = subprocess.Popen(
        [
            "winedbg",
            "--gdb",
            "--no-start",
            "--port",
            str(selected_port),
            sandbox.windows_path(executable),
            *launch_arguments,
        ],
        cwd=sandbox.game_dir,
        env=sandbox.environment(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    try:
        if not _listening(selected_port, time.monotonic() + 60):
            raise ValueError("winedbg --gdb never opened its port")
        completed = subprocess.run(
            ["gdb", "-q", "-batch", "-x", str(script)],
            cwd=sandbox.game_dir,
            env=sandbox.environment(),
            capture_output=True,
            text=True,
            timeout=seconds,
            check=False,
        )
        output = completed.stdout + completed.stderr
    except subprocess.TimeoutExpired as expired:
        output = _text(expired.stdout) + _text(expired.stderr)
    finally:
        _terminate_process_group(proxy)
        subprocess.run(
            ["wineserver", "-k"],
            cwd=sandbox.game_dir,
            env=sandbox.environment(),
            check=False,
        )
        script.unlink(missing_ok=True)

    events = parse_events(output)
    image = sandbox.game_dir / executable
    provenance: dict[str, Any] = {
        "executable": executable,
        "executable_sha256": sha256_file(image),
        "arguments": launch_arguments,
        "unwatched": unwatched,
        "link_map_sha256": sha256_file(link_map) if link_map is not None else None,
        "variant_identity": os.environ.get("WIZ8_DYNAMIC_VARIANT", f"sha256:{sha256_file(image)}"),
        "trace_plan_sha256": plan_hash,
        "reviewed_evidence_sha256": _reviewed_evidence_hash(repo),
        "repository_revision": _repository_revision(repo),
        "wine": tool_version("wine", ("--version",)),
        "gdb": tool_version("gdb", ("--version",)),
        "timeout_seconds": seconds,
        "proxy_port": selected_port,
    }
    # Which SurRender provider the run actually loaded is part of the claim:
    # a rebuilt exe under a stock provider says nothing about the provider.
    provider = sandbox.game_dir / "sr.dll"
    if provider.is_file():
        provenance["provider_sha256"] = sha256_file(provider)
    if fixture is not None:
        provenance["fixture"] = fixture
    return {
        "scenario": scenario,
        "watched": len(points),
        "reached": len({event.name for event in events}),
        "events": [
            {"order": event.order, "kind": event.kind, "name": event.name, "address": event.address}
            for event in events
        ],
        "state": parse_state(output),
        "started": READY in output,
        "provenance": provenance,
    }


def run_smoke(
    repo: Path,
    sandbox: Sandbox,
    seconds: int = 120,
    port: int | None = None,
    *,
    executable: str = "Wiz8Runtime.exe",
    link_map: Path | None = None,
) -> dict[str, Any]:
    """One lifecycle run of the runnable product: entry, menu, quit, exit.

    The quit is a real player gesture, not an injected flag: when the trace
    sees the menu's enter handler the script queues PageDown+Return (the
    exit-screen binding), and when it sees the exit screen's enter handler
    it queues the confirming Return. A gesture that never lands leaves the
    run at the timeout - `exited` stays false rather than guessing."""

    for tool in ("winedbg", "wineserver", "gdb", "ss", "xdotool"):
        if shutil.which(tool) is None:
            raise ValueError(f"{tool} is not on PATH; the smoke test needs it")
    if not (sandbox.game_dir / executable).is_file():
        raise ValueError(f"no {executable} in {sandbox.game_dir}")

    points = trace_plan(repo, SMOKE)
    unwatched: list[str] = []
    if link_map is not None:
        resolved_map = verified_link_map(sandbox.game_dir / executable, link_map)
        points, unwatched = rebase_plan(repo, points, resolved_map)
    watched_names = {point.name for point in points}
    actions = {
        name: action
        for name, action in (
            # The intro screen loops videos until Escape dismisses them;
            # the frame handler's every-Nth-hit cadence re-arms the key
            # press roughly once a second, like the runtime harness does.
            (
                "screen_0_frame",
                BreakpointAction(lines=("shell xdotool key Escape",), every=30),
            ),
            ("screen_1_enter", BreakpointAction(lines=("shell xdotool key Next Return",))),
            ("screen_12_enter", BreakpointAction(lines=("shell xdotool key Return",))),
        )
        if name in watched_names
    }
    selected_port = port if port is not None else _allocate_port()
    plan_hash = json_hash(
        [{"address": point.address, "name": point.name, "kind": point.kind} for point in points]
    )
    script = sandbox.game_dir.parent / f"smoke-{selected_port}.gdb"
    script.write_text(gdb_script(points, selected_port, actions=actions), encoding="utf-8")

    proxy = subprocess.Popen(
        [
            "winedbg",
            "--gdb",
            "--no-start",
            "--port",
            str(selected_port),
            sandbox.windows_path(executable),
        ],
        cwd=sandbox.game_dir,
        env=sandbox.environment(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    timed_out = False
    try:
        if not _listening(selected_port, time.monotonic() + 60):
            raise ValueError("winedbg --gdb never opened its port")
        try:
            completed = subprocess.run(
                ["gdb", "-q", "-batch", "-x", str(script)],
                cwd=sandbox.game_dir,
                env=sandbox.environment(),
                capture_output=True,
                text=True,
                timeout=seconds,
                check=False,
            )
            output = completed.stdout + completed.stderr
            finished = completed.returncode is not None
        except subprocess.TimeoutExpired as expired:
            timed_out = True
            output = _text(expired.stdout) + _text(expired.stderr)
            finished = False
    finally:
        _terminate_process_group(proxy)
        subprocess.run(
            ["wineserver", "-k"],
            cwd=sandbox.game_dir,
            env=sandbox.environment(),
            check=False,
        )
        script.unlink(missing_ok=True)

    events = parse_events(output)
    image = sandbox.game_dir / executable
    provenance: dict[str, Any] = {
        "executable": executable,
        "executable_sha256": sha256_file(image),
        "unwatched": unwatched,
        "link_map_sha256": sha256_file(link_map) if link_map is not None else None,
        "variant_identity": os.environ.get("WIZ8_DYNAMIC_VARIANT", f"sha256:{sha256_file(image)}"),
        "trace_plan_sha256": plan_hash,
        "reviewed_evidence_sha256": _reviewed_evidence_hash(repo),
        "repository_revision": _repository_revision(repo),
        "wine": tool_version("wine", ("--version",)),
        "gdb": tool_version("gdb", ("--version",)),
        "timeout_seconds": seconds,
        "proxy_port": selected_port,
    }
    provider = sandbox.game_dir / "sr.dll"
    if provider.is_file():
        provenance["provider_sha256"] = sha256_file(provider)
    reached = {event.name for event in events}
    requirements = {
        "started": READY in output,
        "winmain_entered": "WinMain" in reached,
        "menu_entered": "screen_1_enter" in reached,
        "exit_screen_entered": "screen_12_enter" in reached,
        "sgp_exit_reached": "SGPExit" in reached,
        # SGPExit is the last product-side step; the inferior exiting under
        # the debugger - gdb finishing inside the timeout - is the product's
        # real process exit, not the test harness's TerminateProcess.
        "process_exited": finished and not timed_out,
        "no_unwatched_points": not unwatched,
    }
    return {
        "scenario": SMOKE,
        "affirmative": all(requirements.values()),
        "requirements": requirements,
        "timed_out": timed_out,
        "events": [
            {"order": event.order, "kind": event.kind, "name": event.name, "address": event.address}
            for event in events
        ],
        "started": requirements["started"],
        "provenance": provenance,
    }


def write_report(result: dict[str, Any], destination: Path) -> dict[str, Any]:
    destination.mkdir(parents=True, exist_ok=True)
    path = destination / f"{result['scenario']}.json"
    path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    summary = {key: value for key, value in result.items() if key != "events"}
    summary["events"] = len(result["events"])
    summary["report"] = str(path)
    return summary
