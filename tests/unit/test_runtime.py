from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.display import runtime_display
from wiz8decomp.runtime import (
    _configure_wine_window_management,
    _crash_detail,
    _parse_runtime_crash,
    _parse_runtime_observation,
    _parse_wine_dump,
    _run_runtime_scenario,
    _symbolize_addresses,
    analyze_runtime_crash,
    stage_runtime_test,
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


def test_stage_runtime_test_uses_managed_links_and_materialized_cfg(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    result = stage_runtime_test(settings)
    stage = Path(result["stage"])

    assert (stage / "Data").is_symlink()
    assert (stage / "Wiz8.CFG").read_bytes() == b"\x00\xff"
    assert (stage / "Wiz8RuntimeTest.exe").read_bytes() == b"semantic tests"


def test_stage_runtime_test_refuses_an_unmanaged_asset_directory(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    unmanaged = settings.repo_dir / "build" / "runtime" / "wiz8" / "Data"
    unmanaged.mkdir(parents=True)

    with pytest.raises(RuntimeError, match="not a managed symlink"):
        stage_runtime_test(settings)


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

    symbols = _symbolize_addresses(map_path, [0x00401008, 0x00401012, 0x00401020, 0x00402004])

    assert symbols == [
        "00401008: _first+0x8 [first.obj] first.cpp:10",
        "00401012: _second+0x2 [second.obj]",
        "00402004: _third+0x4 [third.obj] third.cpp:30",
    ]


def test_runtime_crash_prioritizes_the_consumed_return_address(tmp_path: Path) -> None:
    map_path = tmp_path / "crash.map"
    map_path.write_text(
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00000100H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _ShowRegionHelp            00462810 f   RegionManager.cpp.obj\n"
        "Line numbers for RegionManager.cpp.obj(Z:\\repo\\RegionManager.cpp) segment .text\n"
        " 746 0001:0000007d\n",
        encoding="cp1252",
    )
    output = (
        "WIZ8_RUNTIME_CRASH code=c0000005 thread=00000124 operation=write "
        "access=0d958280 eip=00400007 esp=0067fdf0 ebp=fffffffe eax=06cac140 "
        "ebx=004dfa04 ecx=00000001 edx=00462892 esi=79b68290 edi=79b683a0\n"
        "WIZ8_RUNTIME_IMAGE_BASE_FAULT base=00400000 eip=00400007 mz=1 "
        "forced-unresolved=1 consumed=edx:00462892\n"
        "WIZ8_RUNTIME_CANDIDATE source=reg:edx address=00462892 offset=00062892\n"
        "WIZ8_RUNTIME_CANDIDATE source=stack+0x0 address=00462d2a offset=00062d2a\n"
    )

    crash = _parse_runtime_crash(output)

    assert crash is not None
    assert crash.base_fault is not None and crash.base_fault.mz
    assert crash.candidates[0].source == "return:edx"
    assert [candidate.address for candidate in crash.candidates] == [0x00462892, 0x00462D2A]
    assert _crash_detail(map_path, None, crash).splitlines()[1:4] == [
        "forced-unresolved call: target=00400000 fault=00400007",
        "PE DOS header executed as code; edx holds the consumed return address",
        (
            "#0 return:edx: 00462892: _ShowRegionHelp+0x82 [RegionManager.cpp.obj] "
            "RegionManager.cpp:746"
        ),
    ]


def _wine_crash_fixture(tmp_path: Path) -> tuple[Path, str]:
    map_path = tmp_path / "wine.map"
    map_path.write_text(
        "Wiz8Runtime\n"
        " Preferred load address is 00400000\n"
        " Start         Length     Name                   Class\n"
        " 0001:00000000 000b1f70H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:0001ecb0       _CharacterScreenFrame      0041fcb0 f   CharacterScreen.cpp.obj\n"
        "Line numbers for CharacterScreen.cpp.obj(Z:\\repo\\CharacterScreen.cpp) segment .text\n"
        " 776 0001:0001fe14\n",
        encoding="cp1252",
    )
    output = (
        "wine: Unhandled page fault on write access to 0x00000001 at address 0x00400003 "
        "(thread 0124), starting debugger...\n"
        "Unhandled exception: page fault on write access to 0x00000001 in 32-bit code "
        "(0x00400003).\n"
        "Register dump:\n"
        " CS:0023 SS:002b DS:002b ES:002b FS:0063 GS:006b\n"
        " EIP:00400003 ESP:0032fabc EBP:fffffffe EFLAGS:00210246(  R- --  I   - -P- )\n"
        " EAX:00000000 EBX:00000001 ECX:00000000 EDX:0041fe14\n"
        " ESI:00400000 EDI:00400000\n"
        "Stack dump:\n"
        "0x0032fabc:  0041fe14 00000000 0032fae0 00400abc\n"
        "Backtrace:\n"
        "=>0 0x00400003 (0x0032fabc)\n"
        "  1 0x0041fe14 (0x0032fae0)\n"
        "Modules:\n"
        "Module  Address                 Debug info      Name (104 modules)\n"
        "PE        00400000-0067a000       Export          wiz8runtime\n"
        "PE        7b000000-7b0e5000       Deferred        kernelbase\n"
    )
    return map_path, output


def test_wine_dump_is_recognized_without_product_markers(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)

    crash = _parse_wine_dump(output, map_path)

    assert crash is not None
    assert crash.base_fault is not None and crash.base_fault.mz
    assert crash.base_fault.consumed_register == "edx"
    assert crash.base_fault.consumed_address == 0x0041FE14
    assert crash.candidates[0].source == "return:edx"
    assert crash.candidates[0].address == 0x0041FE14
    assert "return:edx: 0041fe14: _CharacterScreenFrame+0x164" in _crash_detail(
        map_path, None, crash
    )


def test_wine_dump_keeps_runtime_addresses_with_a_relocated_module(
    tmp_path: Path,
) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)
    relocated = (
        output.replace("00400003", "00600003")
        .replace("0041fe14", "0061fe14")
        .replace("00400000", "00600000")
        .replace(
            "Backtrace:\n",
            "Modules:\n"
            "PE        600000-  6b1f70       Deferred        wiz8runtime\n"
            "PE        7bc00000-7be00000       Deferred        ntdll\n"
            "Backtrace:\n",
        )
    )

    crash = _parse_wine_dump(relocated, map_path)

    assert crash is not None
    assert crash.base_fault is not None
    assert crash.base_fault.base == 0x00600000
    assert crash.load_base == 0x00600000
    # The parser keeps the addresses Wine logged; the MAP lookup rebases them.
    assert crash.base_fault.consumed_address == 0x0061FE14
    assert crash.candidates[0].address == 0x0061FE14
    assert "return:edx: 0061fe14: _CharacterScreenFrame+0x164" in _crash_detail(
        map_path, None, crash
    )


def test_analyze_runtime_crash_falls_back_to_a_wine_dump(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)
    log = tmp_path / "winedbg.log"
    log.write_text(output, encoding="utf-8")

    result = analyze_runtime_crash(log, map_path)

    assert result["crashes"][0]["image_base_fault"]["consumed_return"] == {
        "register": "edx",
        "address": "0041fe14",
    }
    assert result["crashes"][0]["candidates"][0]["symbol"].startswith("_CharacterScreenFrame+0x164")


def test_wine_stack_dump_and_modules_table_drive_candidates(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)

    crash = _parse_wine_dump(output, map_path)

    assert crash is not None
    assert crash.load_base == 0x00400000
    sources = {candidate.source: candidate.address for candidate in crash.candidates}
    assert sources["stack+0xc"] == 0x00400ABC
    assert sources["return:edx"] == 0x0041FE14
    assert "page fault on write access" in crash.fields["operation"]


def test_wine_wow64_stack_rows_accept_wide_addresses(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)
    wide = output.replace(
        "0x0032fabc:  0041fe14 00000000 0032fae0 00400abc\n",
        "0x000000000032fabc:  0041fe14 00000000 0032fae0 00400abc\n",
    )

    crash = _parse_wine_dump(wide, map_path)

    assert crash is not None
    sources = {candidate.source: candidate.address for candidate in crash.candidates}
    assert sources["stack+0xc"] == 0x00400ABC


def test_wine_rebased_module_range_uses_the_load_time_base(tmp_path: Path) -> None:
    map_path = tmp_path / "rebased.map"
    map_path.write_text(
        "Wiz8Runtime\n"
        " Preferred load address is 10000000\n"
        " Start         Length     Name                   Class\n"
        " 0001:00000000 00010000H .text                   CODE\n"
        "  Address         Publics by Value              Rva+Base     Lib:Object\n"
        " 0001:00000000       _RebasedTarget             10000000 f   Rebased.cpp.obj\n",
        encoding="cp1252",
    )
    output = (
        "Unhandled exception: page fault on execute access to 0x00000000 in 32-bit code "
        "(0x00500003).\n"
        "Register dump:\n"
        " CS:0023 SS:002b DS:002b ES:002b FS:0063 GS:006b\n"
        " EIP:00500003 ESP:0032fabc EBP:fffffffe EFLAGS:00210246(  R- --  I   - -P- )\n"
        " EAX:00000000 EBX:00500001 ECX:00000000 EDX:00501234\n"
        " ESI:00500000 EDI:00500000\n"
        "Stack dump:\n"
        "0x0032fabc:  00501234 00000000 0032fae0 00000000\n"
        "Backtrace:\n"
        "=>0 0x00500003 (0x0032fabc)\n"
        "Modules:\n"
        "Module  Address                 Debug info      Name (104 modules)\n"
        "PE        00500000-0077a000       Export          wiz8runtime\n"
    )

    crash = _parse_wine_dump(output, map_path)

    assert crash is not None
    assert crash.base_fault is not None and crash.base_fault.mz
    assert crash.base_fault.base == 0x00500000
    assert crash.base_fault.consumed_address == 0x00501234
    assert crash.candidates[0].source == "return:edx"
    assert _symbolize_addresses(map_path, [0x00501234], load_base=0x00500000) == [
        "00501234: _RebasedTarget+0x1234 [Rebased.cpp.obj]"
    ]


def test_unhandled_exception_without_register_dump_reports_parse_failure(
    tmp_path: Path,
) -> None:
    log = tmp_path / "winedbg.log"
    log.write_text(
        "wine: Unhandled page fault on write access to 0x00000001 at address 0x00400003 "
        "(thread 0124), starting debugger...\n",
        encoding="utf-8",
    )

    result = analyze_runtime_crash(log, tmp_path / "missing.map")

    assert result["crashes"] == []
    failure = result["parse_failure"]
    assert "no crash could be localized" in failure["reason"]
    assert failure["missing"] == [
        "EIP register dump",
        "wiz8runtime entry in the Modules table",
    ]
    assert "Unhandled page fault" in failure["exception"]
    assert "0x00400003" in failure["log_tail"]


def test_runtime_timeout_preserves_in_process_diagnostics(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def time_out(*args, **kwargs):
        raise __import__("subprocess").TimeoutExpired(
            args[0], 45, output=b"partial stdout", stderr=b"menu reached; teardown stuck"
        )

    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", time_out)
    with pytest.raises(RuntimeError, match="menu reached; teardown stuck"):
        _run_runtime_scenario(tmp_path / "test.exe", tmp_path, {}, "main-menu-startup")


def test_runtime_display_accepts_an_existing_private_display(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    environment = {"DISPLAY": ":0"}
    monkeypatch.setenv("WIZ8_RUNTIME_DISPLAY", ":91")

    with runtime_display(environment, default="virtual", log_path=tmp_path / "xvfb.log") as display:
        assert display == ":91"
        assert environment["DISPLAY"] == ":91"

    assert environment["DISPLAY"] == ":0"


def test_runtime_display_host_mode_preserves_the_inherited_display(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    environment = {"DISPLAY": ":0"}
    monkeypatch.setenv("WIZ8_RUNTIME_DISPLAY", "host")

    with runtime_display(environment, default="virtual", log_path=tmp_path / "xvfb.log") as display:
        assert display is None
        assert environment["DISPLAY"] == ":0"


def test_virtual_runtime_display_fails_closed_without_xvfb(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setenv("WIZ8_RUNTIME_DISPLAY", "virtual")
    monkeypatch.setattr("wiz8decomp.display.shutil.which", lambda _: None)

    with (
        pytest.raises(RuntimeError, match="requires Xvfb"),
        runtime_display({}, default="virtual", log_path=tmp_path / "xvfb.log"),
    ):
        pass


@pytest.mark.parametrize(("private_display", "managed"), [(True, "N"), (False, "Y")])
def test_wine_window_management_matches_display_mode(
    private_display: bool, managed: str, monkeypatch: pytest.MonkeyPatch
) -> None:
    calls = []
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run",
        lambda *args, **kwargs: calls.append((args, kwargs)),
    )

    environment = {"WINEPREFIX": "/prefix"}
    _configure_wine_window_management(environment, private_display=private_display)

    argv = calls[0][0][0]
    assert argv[-3:] == ["/d", managed, "/f"]
    assert calls[0][1]["env"] is environment
