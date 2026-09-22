"""The dynamic oracle's plan, its parsing, and what it refuses to conclude."""

from __future__ import annotations

from inspect import getsource
from pathlib import Path

import pytest
from wiz8decomp.binary.linker_map import LinkerMap, MapSymbol
from wiz8decomp.dynamic import (
    BRING_UP,
    LOAD,
    SCREENS,
    Event,
    _allocate_port,
    compare_streams,
    gdb_script,
    load_points,
    parse_events,
    rebase_plan,
    run_trace,
    screen_points,
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


def test_rebase_resolves_a_point_by_its_canonical_name() -> None:
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


def test_an_ambiguous_canonical_name_stays_dropped() -> None:
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


def test_each_trace_allocates_a_port_and_uses_scoped_cleanup() -> None:
    assert 0 < _allocate_port() < 65536
    source = getsource(run_trace)
    assert "start_new_session=True" in source
    assert '["wineserver", "-k"]' in source
    assert "pkill" not in source


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
