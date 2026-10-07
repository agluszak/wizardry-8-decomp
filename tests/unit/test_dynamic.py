"""The dynamic oracle's plan, its parsing, and what it refuses to conclude."""

from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.binary.linker_map import LinkerMap, MapSymbol
from wiz8decomp.debug.session import allocate_port
from wiz8decomp.dynamic import (
    BRING_UP,
    LOAD,
    SCREENS,
    SMOKE,
    BreakpointAction,
    Event,
    StateProbe,
    TracePoint,
    _state_lines,
    compare_states,
    compare_streams,
    gdb_script,
    load_points,
    parse_events,
    parse_state,
    rebase_plan,
    rebase_probes,
    screen_points,
    state_probes,
    trace_plan,
)

REPOSITORY = Path(__file__).resolve().parents[2]


def test_a_folded_stub_is_not_a_screen_because_it_cannot_name_its_state() -> None:
    # The linker merged seventeen trivial handlers into one address; a hit
    # there cannot say which state reached it, so it is not watched.
    points = screen_points(REPOSITORY)

    assert points
    assert all(point.address != "005b1740" for point in points)
    assert all(point.name.startswith("screen_") for point in points)


def test_the_screens_scenario_includes_the_gates_that_reach_them() -> None:
    gates = trace_plan(REPOSITORY, BRING_UP)
    screens = trace_plan(REPOSITORY, SCREENS)

    assert len(screens) > len(gates)
    assert set(gates) <= set(screens)


def test_the_load_scenario_watches_the_recovered_loading_chain() -> None:
    points = trace_plan(REPOSITORY, LOAD)
    names = [point.name for point in points]

    # Cold-start gates and the dispatcher's screens frame the load: a claim
    # about the scenario covers the whole path into the world.
    assert set(trace_plan(REPOSITORY, BRING_UP)) <= set(points)
    assert set(screen_points(REPOSITORY)) <= set(points)
    assert "FindStartupQuickSave" in names
    assert "LoadGame" in names
    assert all(
        point.kind == "load"
        for point in points
        if point.name in {"FindStartupQuickSave", "LoadGame"}
    )
    assert load_points(REPOSITORY)


def test_rebase_translates_names_through_the_builds_map() -> None:
    # The rebuilt image puts the same reviewed functions at different
    # addresses; the plan keeps the names and takes each build's addresses
    # from that build's linker map.
    load = next(point for point in trace_plan(REPOSITORY, LOAD) if point.name == "LoadGame")
    symbols = [
        MapSymbol(
            segment=1,
            offset=0x1200,
            address=0x411200,
            decorated_name="?LoadGame@@YAEPBD@Z",
            object_name="LoadSaveGame.obj",
            is_function=True,
        )
    ]
    link_map = LinkerMap(symbols=symbols, sections=[], source_lines=[])

    rebased, dropped = rebase_plan(REPOSITORY, [load], link_map)

    assert dropped == []
    assert [point.name for point in rebased] == ["LoadGame"]
    assert rebased[0].address == "00411200"


def test_rebase_reports_points_the_rebuilt_image_lacks() -> None:
    # A point whose reviewed identity is not in the rebuilt image cannot be
    # watched; it is reported rather than silently dropped.
    load = next(point for point in trace_plan(REPOSITORY, LOAD) if point.name == "LoadGame")
    link_map = LinkerMap(symbols=[], sections=[], source_lines=[])

    rebased, dropped = rebase_plan(REPOSITORY, [load], link_map)

    assert rebased == []
    assert dropped == ["LoadGame"]


def test_rebase_resolves_a_point_by_its_canonical_name(monkeypatch) -> None:
    monkeypatch.setattr("wiz8decomp.source_index.source_functions", lambda *_: {})
    # Marker-emitted functions have no declaration to decorate, and the
    # linker may keep another unit's instantiation: the comparison is by
    # name, so a unique map symbol with the same canonical name resolves the
    # point.
    grow = next(
        point
        for point in trace_plan(REPOSITORY, LOAD)
        if point.name == "W8GrowableVector<unsigned char>::Grow"
    )
    symbols = [
        MapSymbol(
            segment=1,
            offset=0xD420,
            address=0x4AD420,
            decorated_name="?Grow@?$W8GrowableVector@E@@QAEHH@Z",
            object_name="DialogFactoryDialogs.obj",
            is_function=True,
        )
    ]
    link_map = LinkerMap(symbols=symbols, sections=[], source_lines=[])

    rebased, dropped = rebase_plan(REPOSITORY, [grow], link_map)

    assert dropped == []
    assert [point.name for point in rebased] == ["W8GrowableVector<unsigned char>::Grow"]
    assert rebased[0].address == "004ad420"


def test_an_ambiguous_canonical_name_stays_dropped(monkeypatch) -> None:
    monkeypatch.setattr("wiz8decomp.source_index.source_functions", lambda *_: {})
    # Two emissions with the same name could bind the wrong one; inconclusive
    # is honest where an arbitrary pick would silently watch the wrong code.
    grow = next(
        point
        for point in trace_plan(REPOSITORY, LOAD)
        if point.name == "W8GrowableVector<unsigned char>::Grow"
    )
    duplicates = [
        MapSymbol(
            segment=1,
            offset=offset,
            address=0x4AD420 + index,
            decorated_name="?Grow@?$W8GrowableVector@E@@QAEHH@Z",
            object_name="DialogFactoryDialogs.obj",
            is_function=True,
        )
        for index, offset in enumerate((0xD420, 0xD430))
    ]
    link_map = LinkerMap(symbols=duplicates, sections=[], source_lines=[])

    rebased, dropped = rebase_plan(REPOSITORY, [grow], link_map)

    assert rebased == []
    assert dropped == ["W8GrowableVector<unsigned char>::Grow"]


@pytest.mark.parametrize("candidate_count", [1, 2])
def test_rebase_uses_a_decorated_selector_without_changing_display_metadata(
    monkeypatch: pytest.MonkeyPatch, candidate_count: int
) -> None:
    from wiz8decomp.emissions import Emission

    emission = Emission(
        "WIZ8",
        0x1000,
        "",
        "Vec<T>::Grow",
        "template",
        ("src/wiz8/local_code/LoadSaveGame.cpp",),
        "?Grow@?$Vec@H@@QAEHH@Z",
        True,
    )
    monkeypatch.setattr("wiz8decomp.source_index.source_functions", lambda *_: {})
    monkeypatch.setattr("wiz8decomp.dynamic.emission_inventory", lambda *_: [emission])
    points = load_points(REPOSITORY)
    assert points[0].name == "Vec<int>::Grow"
    symbols = [
        MapSymbol(
            segment=1,
            offset=0xD420 + index,
            address=0x4AD420 + index,
            decorated_name="?Grow@?$Vec@H@@QAEHH@Z",
            object_name="Vector.obj",
            is_function=True,
        )
        for index in range(candidate_count)
    ]
    rebased, dropped = rebase_plan(
        REPOSITORY, points, LinkerMap(symbols=symbols, sections=[], source_lines=[])
    )
    assert emission.name == "Vec<T>::Grow"
    if candidate_count == 1:
        assert rebased[0].address == "004ad420"
        assert dropped == []
    else:
        assert rebased == []
        assert dropped == ["Vec<int>::Grow"]


def test_an_unknown_scenario_is_refused() -> None:
    with pytest.raises(ValueError, match="unknown scenario"):
        trace_plan(REPOSITORY, "whatever")


def test_the_script_never_leaves_the_program_stopped() -> None:
    points = trace_plan(REPOSITORY, BRING_UP)
    script = gdb_script(points, 4242)

    assert "target remote localhost:4242" in script
    # One `continue` inside every breakpoint's commands, plus the final one
    # that starts the run: a scenario that stops is a scenario that never ends.
    assert script.count("continue") == len(points) + 1
    assert script.count("break *0x") == len(points)


def test_each_trace_allocates_a_port() -> None:
    assert 0 < allocate_port() < 65536


def test_only_event_lines_are_events() -> None:
    events = parse_events(
        "Reading symbols from Wiz8.exe...\n"
        "EVENT gate WinMain 00401670\n"
        "[New Thread 292]\n"
        "EVENT gate InitializeSubsystem 00401570\n"
        "Cannot execute this command while the target is running.\n"
    )

    assert [event.name for event in events] == ["WinMain", "InitializeSubsystem"]
    assert [event.order for event in events] == [0, 1]


def _stream(*names: str) -> list[Event]:
    return [
        Event(order=index, kind="gate", name=name, address=f"{index:08x}")
        for index, name in enumerate(names)
    ]


def test_two_runs_are_compared_by_name_because_builds_move_addresses() -> None:
    left = _stream("WinMain", "CheckCdPresent")
    right = [
        Event(order=0, kind="gate", name="WinMain", address="00411670"),
        Event(order=1, kind="gate", name="CheckCdPresent", address="0043b830"),
    ]

    assert compare_streams(left, right)["agrees"] is True


def test_only_the_first_divergence_is_reported() -> None:
    # Everything after a divergence is its consequence: one extra event shifts
    # the whole tail, and reporting that tail would multiply one fact.
    result = compare_streams(
        _stream("WinMain", "CheckCdPresent", "InitializeSubsystem"),
        _stream("WinMain", "ShutdownHandler", "InitializeSubsystem"),
    )

    assert result["common_prefix"] == 1
    assert result["left"] == "CheckCdPresent"
    assert result["right"] == "ShutdownHandler"


def test_a_run_that_stops_early_diverges_where_it_stopped() -> None:
    result = compare_streams(
        _stream("WinMain", "CheckCdPresent"),
        _stream("WinMain", "CheckCdPresent", "InitializeSubsystem"),
    )

    assert result["agrees"] is False
    assert result["common_prefix"] == 2
    assert "InitializeSubsystem" in result["detail"]


def test_the_smoke_scenario_watches_the_products_own_lifecycle() -> None:
    # Entry, the menu, the quit path and the exit: the lifecycle the runtime
    # test executable's explicit SGPExit plus TerminateProcess cannot prove.
    names = [point.name for point in trace_plan(REPOSITORY, SMOKE)]

    assert "WinMain" in names
    assert "screen_1_enter" in names
    assert "screen_12_enter" in names
    assert "SGPExit" in names


def test_the_load_checkpoint_resolves_its_probes_from_reviewed_globals() -> None:
    probes, unwatched = state_probes(REPOSITORY, LOAD)

    assert unwatched == []
    names = [probe.name for probe in probes]
    assert {"screen.id", "pending.id", "camera.x"} <= set(names)
    # The camera placement is reached through the global's pointer, not at a
    # fixed address: its object is lazily allocated, so the base is indirect.
    assert {probe.global_name for probe in probes if probe.indirect} == {"g_gd_camera"}


def test_rebase_probes_translate_bases_through_the_builds_map() -> None:
    probes, _ = state_probes(REPOSITORY, LOAD)
    camera = next(probe for probe in probes if probe.name == "camera.x")
    link_map = LinkerMap(
        symbols=[
            MapSymbol(
                segment=3,
                offset=0x92048,
                address=0x685048,
                decorated_name="?g_gd_camera@@3PAVGDCamera@@A",
                object_name="GDCamera.cpp.obj",
                is_function=False,
            )
        ],
        sections=[],
        source_lines=[],
    )

    rebased, dropped = rebase_probes(REPOSITORY, [camera], link_map)

    assert dropped == []
    assert rebased[0].address == "00685048"
    assert rebased[0].offset == camera.offset


def test_rebase_probes_reports_a_global_the_image_lacks() -> None:
    probes, _ = state_probes(REPOSITORY, LOAD)
    camera = next(probe for probe in probes if probe.name == "camera.x")
    link_map = LinkerMap(symbols=[], sections=[], source_lines=[])

    rebased, dropped = rebase_probes(REPOSITORY, [camera], link_map)

    assert rebased == []
    assert dropped == ["g_gd_camera"]


def test_an_action_runs_once_at_its_breakpoint() -> None:
    script = gdb_script(
        [TracePoint(address="00401000", name="screen_1_enter", kind="screen")],
        4242,
        actions={"screen_1_enter": BreakpointAction(lines=("shell xdotool key Next Return",))},
    )

    assert "set $action_screen_1_enter = 0" in script
    assert "if $action_screen_1_enter == 0" in script
    assert "shell xdotool key Next Return" in script
    # The gesture runs at the hit it belongs to, after the event is recorded.
    assert script.index("EVENT screen screen_1_enter") < script.index("shell xdotool")


def test_a_periodic_action_repeats_every_nth_hit() -> None:
    script = gdb_script(
        [TracePoint(address="00401000", name="screen_0_frame", kind="screen")],
        4242,
        actions={"screen_0_frame": BreakpointAction(lines=("shell xdotool key Escape",), every=30)},
    )

    assert "set $action_screen_0_frame = $action_screen_0_frame + 1" in script
    assert "if $action_screen_0_frame % 30 == 0" in script
    assert "shell xdotool key Escape" in script


def test_the_fingerprint_reads_through_a_null_checked_pointer() -> None:
    lines = _state_lines(
        [
            StateProbe("screen.id", "g_current_screen_state", "0068ec78", 0x00, False),
            StateProbe("camera.x", "g_gd_camera", "0065a0f8", 0x8C, True),
        ]
    )

    assert 'printf "STATE screen.id 0x%08x\\n", *(unsigned int*)(0x0068ec78 + 0)' in lines
    assert "if *(unsigned int*)0x0065a0f8 != 0" in lines
    assert (
        'printf "STATE camera.x 0x%08x\\n", *(unsigned int*)(*(unsigned int*)0x0065a0f8 + 140)'
    ) in lines
    assert 'printf "STATE camera.x missing\\n"' in lines


def test_the_first_fingerprint_write_wins() -> None:
    state = parse_state(
        "STATE screen.id 0x00000007\n"
        "gdb chatter\n"
        "STATE screen.id 0x00000009\n"
        "STATE camera.x missing\n"
    )

    assert state == {"screen.id": "0x00000007", "camera.x": "missing"}


def test_state_comparison_reports_field_level_divergence() -> None:
    result = compare_states(
        {"screen.id": "0x7", "camera.x": "0x42"},
        {"screen.id": "0x7", "camera.x": "0x41", "pending.id": "0x0"},
    )

    assert result["agrees"] is False
    assert [diff["name"] for diff in result["diffs"]] == ["camera.x", "pending.id"]
    assert result["diffs"][1]["left"] == "<absent>"


def test_identical_fingerprints_agree() -> None:
    assert compare_states({"a": "0x1"}, {"a": "0x1"})["agrees"] is True


def test_smoke_script_observes_exit_after_continue() -> None:
    script = gdb_script([], 4242, observe_exit=True)
    assert script.endswith('continue\nif $_exitcode == 0\nprintf "PROCESS_EXIT 0\\n"\nend\n')
    assert "PROCESS_EXIT" not in gdb_script([], 4242)


@pytest.mark.parametrize(
    ("returncode", "exit_line", "timeout", "expected"),
    [
        (0, "PROCESS_EXIT 0", False, True),
        (0, "", False, False),
        (1, "PROCESS_EXIT 0", False, False),
        (0, "PROCESS_EXIT 1", False, False),
        (0, "prefix PROCESS_EXIT 0", False, False),
        (0, "PROCESS_EXIT 0", True, False),
    ],
)
def test_smoke_requires_observed_exit(
    tmp_path: Path, monkeypatch, returncode: int, exit_line: str, timeout: bool, expected: bool
) -> None:
    import subprocess
    from unittest.mock import Mock

    from wiz8decomp import dynamic

    image = tmp_path / "Wiz8Runtime.exe"
    image.touch()
    names = ["WinMain", "screen_1_enter", "screen_12_enter", "SGPExit"]
    points = [
        TracePoint(address=f"{index:08x}", name=name, kind="gate")
        for index, name in enumerate(names)
    ]
    output = (
        "TRACE_READY\n"
        + "".join(f"EVENT {point.kind} {point.name} {point.address}\n" for point in points)
        + exit_line
        + "\n"
    )
    monkeypatch.setattr(dynamic.shutil, "which", lambda tool: tool)
    monkeypatch.setattr(dynamic, "trace_plan", lambda repo, scenario: points)
    monkeypatch.setattr(dynamic, "tool_version", lambda *args: "test")
    monkeypatch.setattr(dynamic, "_reviewed_evidence_hash", lambda repo: "test")
    monkeypatch.setattr(dynamic, "_repository_revision", lambda repo: "test")
    proxy = Mock()
    monkeypatch.setattr(dynamic, "WineGdbProxy", Mock(return_value=proxy))

    def run(command, **kwargs):
        if command[0] == "gdb":
            if timeout:
                raise subprocess.TimeoutExpired(command, 1, output=output)
            return subprocess.CompletedProcess(command, returncode, output, "")
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(dynamic.subprocess, "run", run)
    monkeypatch.setattr(
        dynamic,
        "_trace_environment",
        lambda *_: {
            "WINEPREFIX": str(tmp_path / "prefix"),
            "WIZ8_UMU_WINESERVER": "/prepared/wineserver",
            "WIZ8_UMU_RUN": "/prepared/umu-run",
            "PROTONPATH": "/prepared/GE",
        },
    )
    monkeypatch.setattr(dynamic, "require_umu_runner", lambda _: "/prepared/umu-run")
    sandbox = dynamic.Sandbox(tmp_path, tmp_path / "prefix", ":99")
    result = dynamic.run_smoke(tmp_path, sandbox, port=4242)
    assert result["provenance"]["arguments"] == ["/WINDOW"]
    assert dynamic.WineGdbProxy.call_args.kwargs["arguments"] == ("/WINDOW",)
    assert result["requirements"]["process_exited"] is expected
    assert result["affirmative"] is expected
    assert result["timed_out"] is timeout
    proxy.close.assert_called_once()
    assert not (tmp_path.parent / "smoke-4242.gdb").exists()


@pytest.mark.parametrize("failure", ["start", "close", "wineserver", "timeout"])
def test_capture_cleans_script_and_scopes_shutdown(
    tmp_path: Path, monkeypatch, failure: str
) -> None:
    import subprocess
    from unittest.mock import Mock

    from wiz8decomp import dynamic

    script = tmp_path / "trace.gdb"
    script.touch()
    proxy = Mock()
    if failure in {"start", "close"}:
        getattr(proxy, failure).side_effect = RuntimeError(failure)
    monkeypatch.setattr(dynamic, "WineGdbProxy", Mock(return_value=proxy))
    shutdown = []

    def run(command, **kwargs):
        if command[0] == "gdb":
            if failure == "timeout":
                raise subprocess.TimeoutExpired(
                    command, 1, output=b"EVENT partial\n", stderr=b"error\n"
                )
            return subprocess.CompletedProcess(command, 0, "", "")
        shutdown.append((command, kwargs["env"]["WINEPREFIX"]))
        if failure == "wineserver":
            raise OSError("wineserver")
        return subprocess.CompletedProcess(command, 0)

    monkeypatch.setattr(dynamic.subprocess, "run", run)
    sandbox = dynamic.Sandbox(tmp_path, tmp_path / "prefix", ":99")
    environment = {"WINEPREFIX": str(sandbox.prefix), "WIZ8_UMU_WINESERVER": "/prepared/wineserver"}
    monkeypatch.setattr(dynamic, "require_umu_runner", lambda _: "/prepared/umu-run")
    if failure == "timeout":
        assert dynamic._capture(sandbox, "game.exe", script, 4242, 1, environment=environment) == (
            "EVENT partial\nerror\n",
            None,
        )
    else:
        with pytest.raises((RuntimeError, OSError), match=failure):
            dynamic._capture(sandbox, "game.exe", script, 4242, 1, environment=environment)
    assert shutdown == [(["/prepared/wineserver", "-k"], str(sandbox.prefix))]
    assert dynamic.WineGdbProxy.call_args.kwargs["launch_command"] == (
        "/prepared/umu-run",
        "winedbg.exe",
    )
    proxy.close.assert_called_once()
    assert not script.exists()


@pytest.mark.parametrize("returncode", [None, 0, 1])
def test_trace_retains_capture_completion_state(tmp_path, monkeypatch, returncode):
    from wiz8decomp import dynamic

    (tmp_path / "Wiz8.exe").touch()
    monkeypatch.setattr(dynamic.shutil, "which", lambda _: "/bin/tool")
    monkeypatch.setattr(dynamic, "trace_plan", lambda *_: [])
    monkeypatch.setattr(dynamic, "_capture", lambda *_, **__: ("TRACE_READY\n", returncode))
    monkeypatch.setattr(dynamic, "_provenance", lambda *_: {})
    sandbox = dynamic.Sandbox(tmp_path, tmp_path / "prefix", ":99")
    monkeypatch.setattr(
        dynamic,
        "_trace_environment",
        lambda *_: {
            "WIZ8_UMU_WINESERVER": "/prepared/wineserver",
            "WIZ8_UMU_RUN": "/prepared/umu-run",
            "PROTONPATH": "/prepared/GE",
        },
    )
    result = dynamic.run_trace(tmp_path, sandbox, "screens", port=4242)
    assert result["started"] is True
    assert result["capture_returncode"] == returncode
    assert result["timed_out"] is (returncode is None)
