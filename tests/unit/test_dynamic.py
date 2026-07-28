"""The dynamic oracle's plan, its parsing, and what it refuses to conclude."""

from __future__ import annotations

from pathlib import Path

import pytest

from wiz8decomp.dynamic import (
    BRING_UP,
    SCREENS,
    Event,
    compare_streams,
    gdb_script,
    parse_events,
    screen_points,
    trace_plan,
)

REPOSITORY = Path(__file__).resolve().parents[2]


def test_the_plan_is_generated_from_the_ledger() -> None:
    points = trace_plan(REPOSITORY, BRING_UP)
    named = {point.name: point.address for point in points}

    assert named["WinMain"] == "00401670"
    assert named["BringUpEngine"] == "00401570"
    assert all(point.kind == "gate" for point in points)


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


def test_only_event_lines_are_events() -> None:
    events = parse_events(
        "Reading symbols from Wiz8.exe...\n"
        "EVENT gate WinMain 00401670\n"
        "[New Thread 292]\n"
        "EVENT gate BringUpEngine 00401570\n"
        "Cannot execute this command while the target is running.\n"
    )

    assert [event.name for event in events] == ["WinMain", "BringUpEngine"]
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
        _stream("WinMain", "CheckCdPresent", "BringUpEngine"),
        _stream("WinMain", "ShutdownHandler", "BringUpEngine"),
    )

    assert result["common_prefix"] == 1
    assert result["left"] == "CheckCdPresent"
    assert result["right"] == "ShutdownHandler"


def test_a_run_that_stops_early_diverges_where_it_stopped() -> None:
    result = compare_streams(
        _stream("WinMain", "CheckCdPresent"), _stream("WinMain", "CheckCdPresent", "BringUpEngine")
    )

    assert result["agrees"] is False
    assert result["common_prefix"] == 2
    assert "BringUpEngine" in result["detail"]
