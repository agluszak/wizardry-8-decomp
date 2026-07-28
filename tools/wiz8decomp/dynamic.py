"""A scenario-bounded dynamic oracle: watch the original run, from evidence.

Everything else in this repository reasons about the image at rest. That is
where most of the evidence is, but some questions only the running program
answers: which gates actually run and in what order, which screen handler the
dispatcher reaches, whether a recompiled body is reached at all. This runs the
original under Wine with a debugger attached and turns the addresses the
reviewed model already knows into an event stream.

Three properties keep it honest.

**The breakpoints come from the ledger, not from taste.** A trace plan is
generated from the reviewed boundary map and the frame-dispatch table, so what
is watched is exactly what the project has already established, and a plan can
be regenerated after the evidence changes rather than drifting away from it.

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
import json
import os
import re
import shutil
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

EVENT = re.compile(r"^EVENT\s+(?P<kind>\S+)\s+(?P<name>\S+)\s+(?P<address>[0-9a-f]{8})\s*$")
READY = "TRACE_READY"

# Scenario names are part of every claim this module makes, so they are fixed
# here rather than invented per run.
BRING_UP = "bring-up"
SCREENS = "screens"


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


def bring_up_points(repo: Path) -> list[TracePoint]:
    """The reviewed bodies WinMain runs before the first frame.

    The gates are named in the reviewed boundary map and the recovered WinMain
    calls them in a fixed order, which is exactly what a trace can check.
    """

    wanted = {
        "GetRuntimeSettings",
        "ProcessCommandLine",
        "CheckCdPresent",
        "BringUpEngine",
        "SetModuleSubdirectory",
        "QueryAvailableMemory",
        "VerifyDataSubdirs",
        "InitializeInputManager",
        "ShutdownHandler",
        "WinMain",
    }
    points = []
    with (repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        for row in csv.DictReader(stream):
            symbol = row["symbol"].strip()
            if symbol in wanted:
                points.append(TracePoint(address=row["address"], name=symbol, kind="gate"))
    return sorted(points, key=lambda point: point.address)


def screen_points(repo: Path) -> list[TracePoint]:
    """The dispatcher's handlers, named by the state index that reaches them.

    A handler firing is a screen transition: the dispatcher is indexed by the
    application's state, so the handler identifies the state. Folded stubs are
    left out - the linker merged several trivial handlers into one address, and
    a hit there cannot say which state it belonged to.
    """

    slots: dict[str, list[int]] = {}
    kinds: dict[str, str] = {}
    with (repo / "evidence" / "observations" / "wiz8" / "frame-dispatch-table.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        for row in csv.DictReader(stream):
            slots.setdefault(row["handler_address"], []).append(int(row["slot"]))
            kinds[row["handler_address"]] = row["kind"]
    return sorted(
        (
            TracePoint(
                address=address,
                name="screen" + "".join(f"_{slot}" for slot in sorted(states)),
                kind="screen",
            )
            for address, states in slots.items()
            if kinds[address] == "handler"
        ),
        key=lambda point: point.address,
    )


def trace_plan(repo: Path, scenario: str) -> list[TracePoint]:
    if scenario == BRING_UP:
        return bring_up_points(repo)
    if scenario == SCREENS:
        return bring_up_points(repo) + screen_points(repo)
    raise ValueError(f"unknown scenario: {scenario}; expected {BRING_UP} or {SCREENS}")


def gdb_script(points: list[TracePoint], port: int) -> str:
    """A batch script that prints one line per hit and never stops the run."""

    lines = [
        "set confirm off",
        "set pagination off",
        "set height 0",
        "set width 0",
        f"target remote localhost:{port}",
    ]
    for point in points:
        lines += [
            f"break *0x{point.address}",
            "commands",
            "silent",
            f'printf "EVENT {point.kind} {point.name} {point.address}\\n"',
            "continue",
            "end",
        ]
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


def run_trace(
    repo: Path,
    sandbox: Sandbox,
    scenario: str,
    seconds: int = 120,
    port: int = 54340,
) -> dict[str, Any]:
    """Run one scenario under the debugger and return its event stream."""

    for tool in ("winedbg", "gdb", "ss"):
        if shutil.which(tool) is None:
            raise ValueError(f"{tool} is not on PATH; the dynamic oracle needs it")
    if not (sandbox.game_dir / "Wiz8.exe").is_file():
        raise ValueError(f"no Wiz8.exe in {sandbox.game_dir}")

    points = trace_plan(repo, scenario)
    script = sandbox.game_dir.parent / f"trace-{scenario}.gdb"
    script.write_text(gdb_script(points, port), encoding="utf-8")

    proxy = subprocess.Popen(
        [
            "winedbg",
            "--gdb",
            "--no-start",
            "--port",
            str(port),
            sandbox.windows_path("Wiz8.exe"),
        ],
        cwd=sandbox.game_dir,
        env=sandbox.environment(),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
    )
    try:
        if not _listening(port, time.monotonic() + 60):
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
        output = (expired.stdout or b"").decode("utf-8", "replace")
        if expired.stderr:
            output += expired.stderr.decode("utf-8", "replace")
    finally:
        proxy.kill()
        proxy.wait(timeout=10)
        subprocess.run(["pkill", "-x", "Wiz8.exe"], check=False)

    events = parse_events(output)
    return {
        "scenario": scenario,
        "watched": len(points),
        "reached": len({event.name for event in events}),
        "events": [
            {"order": event.order, "kind": event.kind, "name": event.name, "address": event.address}
            for event in events
        ],
        "started": READY in output,
    }


def write_report(result: dict[str, Any], destination: Path) -> dict[str, Any]:
    destination.mkdir(parents=True, exist_ok=True)
    path = destination / f"{result['scenario']}.json"
    path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    summary = {key: value for key, value in result.items() if key != "events"}
    summary["events"] = len(result["events"])
    summary["report"] = str(path)
    return summary
