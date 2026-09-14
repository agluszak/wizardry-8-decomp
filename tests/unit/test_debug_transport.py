from __future__ import annotations

import asyncio
import json
from contextlib import nullcontext
from dataclasses import replace
from pathlib import Path
from types import SimpleNamespace
from typing import Any
from unittest.mock import ANY, AsyncMock, Mock

import pytest
from wiz8decomp.binary.linker_map import LinkerMap
from wiz8decomp.debug.debugger import find_runtime_stub, format_crash_snapshot
from wiz8decomp.debug.mi_protocol import parse_mi_record
from wiz8decomp.debug.session import (
    CrashSnapshot,
    GdbSession,
    _parse_frame_addresses,
    _parse_registers,
    _parse_stack_words,
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
    assert event.signal == "SIGTRAP"
    assert event.exit_code is None
    assert not is_terminal_stop(event)


def test_normal_exit_is_terminal() -> None:
    event = stop_event_from_record(parse_mi_record('*stopped,reason="exited-normally"'))
    assert event is not None
    assert is_terminal_stop(event)


def test_terminal_stop_reports_abnormal_exit_and_signal() -> None:
    normal = stop_event_from_record(parse_mi_record('*stopped,reason="exited-normally"'))
    assert normal is not None
    assert terminal_stop_summary(normal)[0] == "exited normally"

    exited = stop_event_from_record(parse_mi_record('*stopped,reason="exited",exit-code="03"'))
    assert exited is not None
    assert terminal_stop_summary(exited)[0] == "exited with code 3"

    killed = stop_event_from_record(
        parse_mi_record('*stopped,reason="exited-signalled",signal-name="SIGKILL"')
    )
    assert killed is not None
    assert terminal_stop_summary(killed)[0] == "terminated by signal SIGKILL"


def test_session_start_awaits_the_remote_initial_stop(tmp_path: Path, monkeypatch) -> None:
    initial_stop = parse_mi_record('*stopped,reason="signal-received",signal-name="SIGTRAP"')

    class FakeMiProcess:
        def __init__(self, *args) -> None:
            self.commands: list[str] = []
            self.events = [initial_stop]

        async def start(self, executable: Path) -> None:
            pass

        async def command(self, command: str, timeout: float = 30.0) -> SimpleNamespace:
            self.commands.append(command)
            return SimpleNamespace(output=())

        async def next_event(self, timeout: float):
            return self.events.pop(0) if self.events else None

        async def close(self) -> None:
            pass

    monkeypatch.setattr("wiz8decomp.debug.session.GdbMiProcess", FakeMiProcess)
    session = GdbSession(tmp_path / "runtime.exe", tmp_path, {}, tmp_path)
    monkeypatch.setattr(session.proxy, "start", Mock())
    monkeypatch.setattr(session.proxy, "close", Mock())

    asyncio.run(session.start())

    assert session._mi is not None
    mi: Any = session._mi
    assert "-exec-continue --all" not in mi.commands
    asyncio.run(session.close())


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
    resolution = linker_map.resolve(0x00401010)
    assert resolution.symbol is not None
    assert resolution.symbol.decorated_name == "_second"


def test_stub_recognition_uses_map_not_gdb_names(tmp_path: Path, monkeypatch) -> None:
    map_path = tmp_path / "Wiz8Runtime.map"
    map_path.write_text(
        " 0001:001F1234       _Wiz8UnrecoveredFunctionTrap 005F1234 f   wiz8_unrecovered.cpp.obj\n"
        " 0001:001F1270       _wiz8_runtime_stub_00506670 005F1270 f   runtime_stubs.cpp.obj\n"
        " 0001:000F9876       ?SetFact@@YAXH@Z 004F9876 f   fact_state.cpp.obj\n",
        encoding="ascii",
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

    event = stop_event_from_record(
        parse_mi_record('*stopped,reason="signal-received",signal-name="SIGTRAP"')
    )
    assert event is not None
    snapshot = CrashSnapshot(
        event=event,
        registers={"eip": 0x005F1234},
        frame_addresses=(0x005F1270,),
        stack_words=(),
        fault_address=None,
        raw_path=tmp_path / "raw.txt",
    )
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.image_layout", lambda _: (0x400000, 0x200000, 0x1000)
    )
    report, resolutions = format_crash_snapshot(snapshot, tmp_path / "runtime.exe", map_path)
    assert "SIGTRAP" in report
    stub = find_runtime_stub(resolutions, manifest_path)
    assert stub is not None
    assert stub["address"] == "00506670"
    assert stub["symbol"] == "?HandleFactChange@@YAXHE@Z"


def test_sigsegv_stack_address_does_not_classify_runtime_stub(tmp_path: Path, monkeypatch) -> None:
    from wiz8decomp.debug.debugger import _debug_result

    map_path = tmp_path / "runtime.map"
    map_path.write_text(
        " 0001:001F1270       _wiz8_runtime_stub_00506670 005F1270 f   runtime_stubs.cpp.obj\n",
        encoding="ascii",
    )
    manifest_path = tmp_path / "runtime_stubs.json"
    manifest_path.write_text(
        json.dumps(
            {
                "stubs": [
                    {
                        "address": "00506670",
                        "symbol": "?HandleFactChange@@YAXHE@Z",
                        "stub": "_wiz8_runtime_stub_00506670",
                    }
                ]
            }
        ),
        encoding="utf-8",
    )
    event = stop_event_from_record(
        parse_mi_record('*stopped,reason="signal-received",signal-name="SIGSEGV"')
    )
    assert event is not None
    snapshot = CrashSnapshot(
        event=event,
        registers={"eip": 0x005F1270},
        frame_addresses=(),
        stack_words=(),
        fault_address=None,
        raw_path=tmp_path / "debugger-stop-sigsegv.txt",
    )
    executable = tmp_path / "runtime.exe"
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.image_layout", lambda _: (0x400000, 0x200000, 0x1000)
    )
    session: Any = SimpleNamespace(
        executable=executable,
        artifact_dir=tmp_path,
        wait_for_stop=AsyncMock(return_value=event),
        capture_stop=AsyncMock(return_value=snapshot),
    )

    result = asyncio.run(
        _debug_result(
            session,
            timeout=1,
            map_path=map_path,
            manifest_path=manifest_path,
            provenance=tmp_path / "session.json",
        )
    )

    assert result["reason"] == "SIGSEGV"
    assert "UNRECOVERED FUNCTION" not in result["report"]


def test_pe_header_crash_recovers_raw_stack_candidates_without_foreign_frames(
    tmp_path: Path, monkeypatch
) -> None:
    map_path = tmp_path / "Wiz8Runtime.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 000b1f70H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00062810       _ShowRegionHelp            00462810 f   RegionManager.cpp.obj\n"
        " 0001:00062d00       _AfterRegionHelp           00462d00 f   RegionManager.cpp.obj\n"
        " 0001:00079800       _LoadGameState             00479800 f   LoadSaveGame.cpp.obj\n"
        "Line numbers for RegionManager.cpp.obj(Z:\\repo\\RegionManager.cpp) segment .text\n"
        " 746 0001:0006287d\n",
        encoding="cp1252",
    )
    raw_path = tmp_path / "debugger-stop-01-sigsegv.txt"
    raw_path.write_text(
        "Thread 7 (audio_client_main):\n"
        "#0  0x7bd642fc in wine_unix_call () from ntdll.dll\n"
        "Thread 6 (wine_rpcrt4_server):\n"
        "#0  0x7b6c2566 in ?? () from kernelbase.dll\n"
        "#1  0x21101655 in ?? ()\n"
        "Thread 1:\n"
        "#0  0x00400007 in ?? ()\n"
        "Backtrace stopped: Cannot access memory at address 0x2\n",
        encoding="utf-8",
    )
    register_lines = [
        "eax            0x6cac998           113953176",
        "edx            0x462892            4597906",
        "esp            0x67fdf0            0x67fdf0",
        "ebp            0xfffffffe          0xfffffffe",
        "eip            0x400007            0x400007",
    ]
    stack_lines = [
        "0x67fdf0:\t0x06cac998\t0x79b683a0\t0x79b68290\t0x00000000",
        "0x67fe00:\t0x00000000\t0x00000000\t0x00462d2a\t0x00000167",
        "0x67fe10:\t0x00479834\t0x7bd644f3\t0x21101655\t0x00000000",
    ]
    event = stop_event_from_record(
        parse_mi_record(
            '*stopped,reason="signal-received",signal-name="SIGSEGV",frame={addr="0x00400007"}'
        )
    )
    assert event is not None
    snapshot = CrashSnapshot(
        event=event,
        registers=_parse_registers(register_lines),
        frame_addresses=_parse_frame_addresses(raw_path.read_text().splitlines()),
        stack_words=_parse_stack_words(stack_lines),
        fault_address=0x0D959330,
        raw_path=raw_path,
    )

    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.image_layout", lambda _: (0x400000, 0xB2000, 0x1000)
    )
    report, resolutions = format_crash_snapshot(snapshot, tmp_path / "runtime.exe", map_path)

    assert "SIGSEGV at 0x0d959330" in report
    assert "PC  0x00400007  PE image headers" in report
    assert "reg:edx" in report
    assert "00462892: _ShowRegionHelp+0x82" in report
    assert "stack+0x18" in report and "00462d2a: _AfterRegionHelp+0x2a" in report
    assert "00479834: _LoadGameState+0x34" in report
    assert "0x7bd" not in report
    assert "0x211" not in report
    assert all(0x00400000 <= item.address < 0x004B2000 for item in resolutions)


def test_ordinary_pc_and_modal_assertion_frames_resolve(tmp_path: Path, monkeypatch) -> None:
    map_path = tmp_path / "runtime.map"
    map_path.write_text(
        " 0001:00000000       _srAssertFail 00401000 f   assert.obj\n"
        " 0001:00000100       _Caller 00401100 f   caller.obj\n"
    )
    event = stop_event_from_record(
        parse_mi_record('*stopped,reason="signal-received",signal-name="SIGSEGV"')
    )
    assert event is not None
    snapshot = CrashSnapshot(
        event=event,
        registers={"eip": 0x401008},
        frame_addresses=(),
        stack_words=(),
        fault_address=None,
        raw_path=tmp_path / "raw.txt",
    )
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.image_layout", lambda _: (0x400000, 0xB2000, 0x1000)
    )
    executable = tmp_path / "runtime.exe"
    report, _ = format_crash_snapshot(snapshot, executable, map_path)
    assert "pc: 00401008: _srAssertFail+0x8" in report
    modal = replace(
        snapshot,
        registers={"eip": 0x7BD642FC},
        frame_addresses=_parse_frame_addresses(
            [
                "#0  0x7bd642fc in MessageBoxA ()",
                "#1  0x21101655 in ?? ()",
                "#2  0x00401008 in ?? ()",
                "#3  0x00401108 in ?? ()",
            ]
        ),
    )
    _, resolutions = format_crash_snapshot(modal, executable, map_path)
    assert [item.address for item in resolutions] == [0x401008, 0x401108]


def test_capture_is_bounded_and_parses_frames(tmp_path: Path, monkeypatch) -> None:
    session = GdbSession(tmp_path / "runtime.exe", tmp_path, {}, tmp_path)
    outputs = {
        "thread apply all bt 16": ["#0  0x7bd642fc in ?? ()", "#1  0x00462892 in ?? ()"],
        "info registers": ["eip 0x400007", "edx 0x462892"],
        "x/96wx $sp": ["0x67fe00: 0x00479834"],
        "x/24i $pc-24": ["Cannot access memory"],
        "p/x $_siginfo._sifields._sigfault.si_addr": ["$1 = 0x0d959330"],
    }
    console = AsyncMock(side_effect=lambda command, **_: outputs[command])
    monkeypatch.setattr(session, "_console", console)
    event = stop_event_from_record(
        parse_mi_record('*stopped,reason="signal-received",signal-name="SIGSEGV"')
    )
    assert event is not None
    snapshot = asyncio.run(session.capture_stop("sigsegv", event))
    assert [call.args[0] for call in console.call_args_list] == list(outputs)
    assert snapshot.frame_addresses == (0x7BD642FC, 0x462892)
    assert snapshot.stack_words == ((0x67FE00, 0x479834),)
    assert snapshot.fault_address == 0xD959330
    assert "Cannot access memory" in snapshot.raw_path.read_text()


def test_console_reassembles_mi_chunks_before_parsing(tmp_path: Path, monkeypatch) -> None:
    session = GdbSession(tmp_path / "runtime.exe", tmp_path, {}, tmp_path)
    command = AsyncMock(
        return_value=[
            "#0  0x79a9c8b4 in NtUserPeekMessage@20 ()\n#1  0x79b6834f in PeekMessageW@20 ()\n",
            "#2  PeekMessageA@20 ()\n#3  0x004d",
            "9a09 in ?? ()\n#4  0x458b08ec in ?? ()\n",
        ]
    )
    monkeypatch.setattr(session, "_command", command)
    frames = _parse_frame_addresses(asyncio.run(session._console("thread apply all bt 16")))
    assert frames == (0x79A9C8B4, 0x79B6834F, 0x004D9A09, 0x458B08EC)


@pytest.mark.parametrize(
    ("scenario", "product", "arguments"),
    [
        (None, "Wiz8Runtime.exe", ["/WINDOW"]),
        ("main-game-start", "Wiz8RuntimeTest.exe", ["--scenario", "main-game-start"]),
    ],
)
def test_launcher_uses_one_proxy_path(
    tmp_path: Path, monkeypatch, scenario, product, arguments
) -> None:
    from wiz8decomp.debug.debugger import run_debugger

    executable = tmp_path / product
    executable.touch()
    settings: Any = SimpleNamespace(
        repo_dir=tmp_path,
        work_dir=tmp_path,
        product_build_dir=tmp_path,
        recovered_objects_dir=tmp_path,
    )
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.stage_game",
        lambda *a, **kw: SimpleNamespace(
            executable=kw["executable"],
            root=tmp_path,
        ),
    )
    configure_window = Mock()
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.configure_wine_window_management", configure_window
    )
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger.runtime_display", lambda *a, **kw: nullcontext(None)
    )
    monkeypatch.setattr("wiz8decomp.debug.debugger._stop_debug_wineserver", Mock())
    monkeypatch.setattr(
        "wiz8decomp.debug.debugger._write_provenance", lambda *a: tmp_path / "session.json"
    )
    monkeypatch.setattr("wiz8decomp.debug.debugger._debug_result", AsyncMock(return_value={}))

    async def start(session: GdbSession) -> None:
        session.proxy.start()

    monkeypatch.setattr(GdbSession, "start", start)
    monkeypatch.setattr(GdbSession, "continue_inferior", AsyncMock())
    monkeypatch.setattr(GdbSession, "close", AsyncMock())
    monkeypatch.setattr("wiz8decomp.debug.session._port_is_listening", lambda _: True)
    process = Mock()
    process.poll.return_value = None
    popen = Mock(return_value=process)
    monkeypatch.setattr("wiz8decomp.debug.session.subprocess.Popen", popen)

    run_debugger(settings, scenario=scenario)

    configure_window.assert_called_once_with(ANY, private_display=False)
    command = popen.call_args.args[0]
    assert command[:4] == ["winedbg", "--gdb", "--no-start", "--port"]
    assert command[5:] == [str(executable), *arguments]
