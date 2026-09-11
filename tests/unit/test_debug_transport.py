from __future__ import annotations

import json
from pathlib import Path

from wiz8decomp.binary.linker_map import LinkerMap
from wiz8decomp.debug.debugger import find_runtime_stub
from wiz8decomp.debug.gdb_report import resolve_gdb_report
from wiz8decomp.debug.mi_protocol import parse_mi_record
from wiz8decomp.debug.session import (
    DebuggerLifecycle,
    is_terminal_stop,
    stop_event_from_record,
    terminal_stop_summary,
)


def test_sigtrap_stop_is_structured() -> None:
    record = parse_mi_record(
        '*stopped,reason="signal-received",signal-name="SIGTRAP",thread-id="1"'
    )
    event = stop_event_from_record(record)
    assert event is not None
    assert event.reason == "signal-received"
    assert event.signal_name == "SIGTRAP"
    assert event.breakpoint_number is None
    assert not is_terminal_stop(event)


def test_normal_exit_is_terminal() -> None:
    event = stop_event_from_record(parse_mi_record('*stopped,reason="exited-normally"'))
    assert event is not None
    assert is_terminal_stop(event)


def _lifecycle(*, exit_code: int | None = None, signal: str | None = None) -> DebuggerLifecycle:
    return DebuggerLifecycle(
        proxy_pid=None,
        proxy_exit_code=None,
        gdb_pid=None,
        gdb_exit_code=None,
        inferior_pid=None,
        inferior_exit_code=exit_code,
        inferior_terminal_reason=None,
        inferior_signal=signal,
        inferior_active=False,
    )


def test_terminal_stop_reports_abnormal_exit_and_signal() -> None:
    normal = stop_event_from_record(parse_mi_record('*stopped,reason="exited-normally"'))
    assert normal is not None
    assert terminal_stop_summary(normal, _lifecycle())[0] == "exited normally"

    exited = stop_event_from_record(parse_mi_record('*stopped,reason="exited",exit-code="03"'))
    assert exited is not None
    assert terminal_stop_summary(exited, _lifecycle(exit_code=3))[0] == "exited with code 3"

    killed = stop_event_from_record(
        parse_mi_record('*stopped,reason="exited-signalled",signal-name="SIGKILL"')
    )
    assert killed is not None
    assert (
        terminal_stop_summary(killed, _lifecycle(signal="SIGKILL"))[0]
        == "terminated by signal SIGKILL"
    )


def test_map_resolves_inside_function_but_not_across_symbol(tmp_path: Path) -> None:
    map_path = tmp_path / "Wiz8Runtime.map"
    map_path.write_text(
        " 0001:00000000       _first 00401000 f   first.obj\n"
        " 0001:00000010       _second 00401010 f   second.obj\n",
        encoding="ascii",
    )
    linker_map = LinkerMap.read(map_path)

    inside = linker_map.resolve(0x00401008)
    assert inside.symbol is not None
    assert inside.symbol.decorated_name == "_first"
    assert inside.confidence == "high"

    across = linker_map.resolve(0x00401012)
    assert across.symbol is not None
    assert across.symbol.decorated_name == "_second"


def test_crossing_into_next_symbol_does_not_bleed(tmp_path: Path) -> None:
    map_path = tmp_path / "Wiz8Runtime.map"
    map_path.write_text(
        " 0001:00000000       _first 00401000 f   first.obj\n"
        " 0001:00000010       _second 00401010 f   second.obj\n",
        encoding="ascii",
    )
    linker_map = LinkerMap.read(map_path)
    # 0x00401010 is exactly the next public; it must resolve to _second.
    assert linker_map.resolve(0x00401010).symbol is not None
    assert linker_map.resolve(0x00401010).symbol.decorated_name == "_second"


def test_stub_recognition_uses_map_not_gdb_names(tmp_path: Path) -> None:
    map_path = tmp_path / "Wiz8Runtime.map"
    map_path.write_text(
        " 0001:001F1234       _Wiz8UnrecoveredFunctionTrap 005F1234 f   wiz8_unrecovered.cpp.obj\n"
        " 0001:001F1270       _wiz8_runtime_stub_00506670 005F1270 f   runtime_stubs.cpp.obj\n"
        " 0001:000F9876       ?SetFact@@YAXH@Z 004F9876 f   fact_state.cpp.obj\n",
        encoding="ascii",
    )
    report_path = tmp_path / "debugger-stop-01-sigtrap.txt"
    report_path.write_text(
        "#0  0x005f1234 in ?? ()\n#1  0x005f1270 in ?? ()\n#2  0x004f9876 in ?? ()\n",
        encoding="utf-8",
    )
    manifest_path = tmp_path / "runtime_stubs.json"
    manifest_path.write_text(
        json.dumps(
            {
                "schema": "wiz8.runtime-stubs",
                "stubs": [
                    {
                        "address": "00506670",
                        "symbol": "?HandleFactChange@@YAXHE@Z",
                        "stub": "_wiz8_runtime_stub_00506670",
                        "name": "HandleFactChange",
                        "requesters": ["fact_state.cpp.obj"],
                    }
                ],
            }
        ),
        encoding="utf-8",
    )

    frames = resolve_gdb_report(report_path, map_path)
    assert len(frames) == 3
    # Every GDB name was ??, yet the MAP still identifies the stub frame.
    assert frames[1][1].symbol is not None
    assert frames[1][1].symbol.decorated_name == "_wiz8_runtime_stub_00506670"

    stub = find_runtime_stub(frames, manifest_path)
    assert stub is not None
    assert stub["address"] == "00506670"
    assert stub["symbol"] == "?HandleFactChange@@YAXHE@Z"
