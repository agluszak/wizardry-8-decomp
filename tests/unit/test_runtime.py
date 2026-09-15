from contextlib import nullcontext
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.display import runtime_display
from wiz8decomp.runtime import (
    RUNTIME_SCENARIO_STATE_FILES,
    _crash_detail,
    _parse_runtime_crash,
    _parse_runtime_observation,
    _parse_wine_dump,
    _reset_runtime_scenario_state,
    _run_runtime_scenario,
    _symbolize_addresses,
    analyze_runtime_crash,
    configure_wine_window_management,
    run_product,
    run_runtime_suite,
    stage_game,
)


def _settings(tmp_path: Path) -> Settings:
    repo = tmp_path / "repo"
    work = tmp_path / "work"
    for name in ("Data", "Dll", "Levels"):
        (work / "variants" / "gog-base" / name).mkdir(parents=True, exist_ok=True)
    (repo / "build" / "decomp").mkdir(parents=True)
    (repo / "build" / "decomp" / "Wiz8RuntimeTest.exe").write_bytes(b"semantic tests")
    (repo / "config" / "runtime").mkdir(parents=True)
    (repo / "config" / "runtime" / "3DVideo.CFG").write_text("video")
    (repo / "config" / "runtime" / "Wiz8.CFG.hex").write_text("00ff")
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": work,
            "repo_dir": repo,
        }
    )


def test_stage_game_uses_managed_links_and_materialized_cfg(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    result = stage_game(
        settings,
        name="runtime-test",
        executable=settings.product_build_dir / "Wiz8RuntimeTest.exe",
        reset_saves=True,
    )
    stage = result.root

    assert (stage / "Data").is_symlink()
    assert (stage / "Wiz8.CFG").read_bytes() == b"\x00\xff"
    assert (stage / "Wiz8RuntimeTest.exe").read_bytes() == b"semantic tests"
    assert (stage / "Saves" / "Characters").is_dir()


def test_reset_runtime_scenario_state_removes_only_scenario_outputs(tmp_path: Path) -> None:
    stage = tmp_path / "runtime-test"
    for relative_path in RUNTIME_SCENARIO_STATE_FILES:
        path = stage / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"scenario state")
    retained = stage / "Saves" / "Characters" / "fixture.CHR"
    retained.write_bytes(b"fixture")

    _reset_runtime_scenario_state(stage)

    assert all(
        not (stage / relative_path).exists() for relative_path in RUNTIME_SCENARIO_STATE_FILES
    )
    assert retained.read_bytes() == b"fixture"


def test_stage_game_refuses_an_unmanaged_asset_directory(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    unmanaged = settings.runtime_stage("runtime-test") / "Data"
    unmanaged.mkdir(parents=True)

    with pytest.raises(RuntimeError, match="not a managed symlink"):
        stage_game(
            settings,
            name="runtime-test",
            executable=settings.product_build_dir / "Wiz8RuntimeTest.exe",
        )


def test_interactive_run_restores_managed_wine_window(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    (settings.product_build_dir / "Wiz8Runtime.exe").write_bytes(b"runtime")
    calls = []

    monkeypatch.setattr("wiz8decomp.runtime.shutil.which", lambda name: f"/usr/bin/{name}")
    monkeypatch.setattr(
        "wiz8decomp.runtime.runtime_display", lambda *args, **kwargs: nullcontext(None)
    )
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run",
        lambda *args, **kwargs: (
            calls.append((args, kwargs)) or SimpleNamespace(returncode=0, stdout="", stderr="")
        ),
    )

    run_product(settings)

    assert calls[0][0][0][-3:] == ["/d", "Y", "/f"]
    assert calls[1][0][0][-5:] == ["/v", "Desktop", "/d", "Wizardry", "/f"]
    assert calls[2][0][0][-5:] == ["/v", "Wizardry", "/d", "640x480", "/f"]
    assert calls[3][0][0][-2:] == ["./Wiz8Runtime.exe", "/WINDOW"]
    assert calls[3][1]["env"]["WINEPREFIX"] == str(settings.work_dir / "wine" / "wiz8-runtime")


def test_runtime_observation_is_normalized_to_typed_fields() -> None:
    observation = _parse_runtime_observation(
        "noise\nWIZ8_RUNTIME_TEST scenario=main-menu-exit-auto-repeat menu_seen=1 "
        "menu_state=0 exit_observed=1 teardown=1 timed_out=0\n"
    )

    assert observation == {
        "scenario": "main-menu-exit-auto-repeat",
        "menu_seen": 1,
        "menu_state": 0,
        "exit_observed": 1,
        "teardown": 1,
        "timed_out": 0,
    }


def test_runtime_observation_requires_one_owned_record() -> None:
    with pytest.raises(RuntimeError, match="expected one runtime observation"):
        _parse_runtime_observation("wine diagnostics only")


@pytest.mark.parametrize("check_order", [False, True])
def test_runtime_suite_selection_and_server_lifetime(
    tmp_path: Path, monkeypatch, check_order
) -> None:
    settings = _settings(tmp_path)
    scenarios = ("main-menu-startup", "split-stack")
    visited = []
    shutdowns = []
    monkeypatch.setattr("wiz8decomp.runtime.shutil.which", lambda name: f"/usr/bin/{name}")
    monkeypatch.setattr(
        "wiz8decomp.runtime.runtime_display", lambda *args, **kwargs: nullcontext(None)
    )
    monkeypatch.setattr(
        "wiz8decomp.runtime.configure_wine_window_management", lambda *args, **kwargs: None
    )
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run", lambda command, **kwargs: shutdowns.append(command)
    )

    def run(executable, stage, environment, scenario, object_root, map_path):
        visited.append(scenario)
        return {"scenario": scenario, "teardown": 1}

    monkeypatch.setattr("wiz8decomp.runtime._run_runtime_scenario", run)
    result = run_runtime_suite(settings, scenarios=scenarios, check_order=check_order)
    assert visited == list(scenarios) + (list(reversed(scenarios)) if check_order else [])
    assert shutdowns == [["wineserver", "-k"]]
    assert result["deterministic"] is (True if check_order else None)


def test_staging_keeps_the_map_for_each_executable_snapshot(
    tmp_path: Path, synthetic_pe: Path
) -> None:
    settings = _settings(tmp_path)
    executable = settings.product_build_dir / "Wiz8Runtime.exe"
    executable.write_bytes(synthetic_pe.read_bytes())
    map_file = executable.with_suffix(".map")
    first_map = " Timestamp is 12345678\n first build\n"
    map_file.write_text(first_map)
    first = stage_game(settings, name="wiz8", executable=executable)
    assert first.map is not None
    assert first.map.read_text() == first_map

    second_map = " Timestamp is 12345678\n second build\n"
    map_file.write_text(second_map)
    second = stage_game(settings, name="wiz8", executable=executable)
    assert second.map is not None and second.map != first.map
    assert second.map.read_text() == second_map
    assert first.map.read_text() == first_map


def test_staging_refuses_mismatched_map_before_replacing_executable(
    tmp_path: Path, synthetic_pe: Path
) -> None:
    settings = _settings(tmp_path)
    executable = settings.product_build_dir / "Wiz8Runtime.exe"
    executable.write_bytes(synthetic_pe.read_bytes())
    map_file = executable.with_suffix(".map")
    map_file.write_text(" Timestamp is 12345678\n")
    staged = stage_game(settings, name="wiz8", executable=executable)
    previous = staged.executable.read_bytes()
    map_file.write_text(" Timestamp is 87654321\n")
    with pytest.raises(RuntimeError, match="timestamp mismatch"):
        stage_game(settings, name="wiz8", executable=executable)
    assert staged.executable.read_bytes() == previous


def test_staging_without_map_never_reuses_a_previous_map(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    stage = settings.runtime_stage("runtime-test")
    stage.mkdir(parents=True)
    (stage / "Wiz8RuntimeTest.map").write_text("stale symbols")
    staged = stage_game(
        settings, name="runtime-test", executable=settings.product_build_dir / "Wiz8RuntimeTest.exe"
    )
    assert staged.map is None


def test_interactive_crash_uses_staged_map_after_build_map_changes(
    tmp_path: Path, synthetic_pe: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    executable = settings.product_build_dir / "Wiz8Runtime.exe"
    executable.write_bytes(synthetic_pe.read_bytes())
    map_file = executable.with_suffix(".map")
    original_map = " Timestamp is 12345678\n first build\n"
    map_file.write_text(original_map)
    monkeypatch.setattr("wiz8decomp.runtime.shutil.which", lambda _: "/usr/bin/wine")
    monkeypatch.setattr("wiz8decomp.runtime.runtime_display", lambda *a, **kw: nullcontext(None))
    monkeypatch.setattr(
        "wiz8decomp.runtime.configure_wine_window_management", lambda *a, **kw: None
    )

    def launch(*args, **kwargs):
        map_file.write_text("relinked while the game was running")
        return SimpleNamespace(returncode=1, stdout="WIZ8_RUNTIME_CRASH", stderr="")

    def analyze(log, selected_map, objects):
        assert selected_map != map_file
        assert selected_map.read_text() == original_map
        return {"matched": True}

    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", launch)
    monkeypatch.setattr("wiz8decomp.runtime.analyze_runtime_crash", analyze)
    assert run_product(settings)["crash"] == {"matched": True}


def test_map_symbolization_refuses_cross_function_lines_and_section_end(tmp_path: Path) -> None:
    map_path = tmp_path / "synthetic.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00000020H .text                   CODE\n"
        " 0002:00000000 00000010H .text$x                 CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _first                     00401000 f   first.obj\n"
        " 0001:00000010       _second                    00401010 f   second.obj\n"
        " 0002:00000000       _third                     00402000 f   third.obj\n"
        "Line numbers for first.obj(Z:\\repo\\first.cpp) segment .text\n"
        " 10 0001:00000008\n"
        "Line numbers for third.obj(Z:\\repo\\third.cpp) segment .text$x\n"
        " 30 0002:00000004\n",
        encoding="cp1252",
    )
    assert _symbolize_addresses(map_path, [0x00401008]) == ["first+0x8 (first.cpp:10)"]
    assert _symbolize_addresses(map_path, [0x00401010]) == ["second+0x0"]
    assert _symbolize_addresses(map_path, [0x00401020]) == []
    assert _symbolize_addresses(map_path, [0x00402004]) == ["third+0x4 (third.cpp:30)"]


def test_wine_dump_fallback_collects_registers_frames_and_stack_candidates(tmp_path: Path) -> None:
    log_path = tmp_path / "winedbg.log"
    log_path.write_text(
        """
Unhandled exception: page fault on execute access to 0x00401234 in 32-bit code
Register dump:
 Eip:00401234 Esp:0019f100 Ebp:0019f120 Eax:00402000 Ebx:00000000
Stack dump:
0x0019f100: 00403000 00000000 00404000 00000000
Backtrace:
=>0 0x00401234
  1 0x00405000
"""
    )
    parsed = _parse_wine_dump(log_path.read_text())
    assert parsed is not None
    assert parsed.fields["eip"] == "00401234"
    assert [candidate.address for candidate in parsed.candidates] == [
        0x00402000,
        0x0019F120,
        0x00401234,
        0x00405000,
        0x00403000,
        0x00404000,
    ]


def test_native_runtime_crash_parser_keeps_all_candidates() -> None:
    parsed = _parse_runtime_crash(
        "WIZ8_RUNTIME_CRASH code=c0000005 thread=1 operation=read access=12345678 "
        "eip=00401000 esp=0019f100 ebp=0019f120 eax=00402000 ebx=0 ecx=0 edx=0 esi=0 edi=0\n"
        "WIZ8_RUNTIME_CANDIDATE source=reg:eax address=00402000\n"
        "WIZ8_RUNTIME_CANDIDATE source=stack address=00403000\n"
    )
    assert parsed is not None
    assert parsed.fields["code"] == "c0000005"
    assert [(candidate.source, candidate.address) for candidate in parsed.candidates] == [
        ("reg:eax", 0x00402000),
        ("stack", 0x00403000),
    ]


def test_crash_detail_formats_fields_and_symbols(tmp_path: Path) -> None:
    map_path = tmp_path / "synthetic.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00000100H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _fault                     00401000 f   first.obj\n"
    )
    parsed = _parse_runtime_crash(
        "WIZ8_RUNTIME_CRASH code=c0000005 thread=1 operation=read access=12345678 "
        "eip=00401000 esp=0019f100 ebp=0019f120 eax=0 ebx=0 ecx=0 edx=0 esi=0 edi=0\n"
    )
    assert parsed is not None
    detail = _crash_detail(parsed, map_path, None)
    assert detail["operation"] == "read"
    assert detail["fault"] == "fault+0x0"


def test_analyze_runtime_crash_uses_native_marker_before_wine_dump(tmp_path: Path) -> None:
    log = tmp_path / "runtime.log"
    map_path = tmp_path / "synthetic.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00000100H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _fault                     00401000 f   first.obj\n"
    )
    log.write_text(
        "WIZ8_RUNTIME_CRASH code=c0000005 thread=1 operation=read access=12345678 "
        "eip=00401000 esp=0019f100 ebp=0019f120 eax=0 ebx=0 ecx=0 edx=0 esi=0 edi=0\n"
        "Unhandled exception: should not win\n"
    )
    report = analyze_runtime_crash(log, map_path)
    assert report is not None
    assert report["source"] == "runtime"
    assert report["fault"] == "fault+0x0"


def test_analyze_runtime_crash_falls_back_to_wine_dump(tmp_path: Path) -> None:
    log = tmp_path / "runtime.log"
    map_path = tmp_path / "synthetic.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00000100H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _fault                     00401000 f   first.obj\n"
    )
    log.write_text(
        "Unhandled exception: page fault on execute access\n"
        "Register dump:\n Eip:00401000 Esp:0019f100 Ebp:0019f120 Eax:0 Ebx:0 Ecx:0 Edx:0 Esi:0 Edi:0\n"
    )
    report = analyze_runtime_crash(log, map_path)
    assert report is not None
    assert report["source"] == "wine"
    assert report["fault"] == "fault+0x0"
