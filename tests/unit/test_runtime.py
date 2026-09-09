from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.display import runtime_display
from wiz8decomp.runtime import (
    _configure_wine_window_management,
    _parse_gdb_diagnostics,
    _parse_runtime_observation,
    _run_runtime_scenario,
    _symbolize_addresses,
    stage_runtime,
)


def _settings(tmp_path: Path) -> Settings:
    repo = tmp_path / "repo"
    work = tmp_path / "work"
    for name in ("Data", "Dll", "Levels"):
        (work / "variants" / "gog-base" / name).mkdir(parents=True, exist_ok=True)
    (repo / "build" / "decomp").mkdir(parents=True)
    (repo / "build" / "decomp" / "Wiz8Runtime.exe").write_bytes(b"runtime")
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


def test_stage_runtime_uses_managed_links_and_materialized_cfg(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    result = stage_runtime(settings)
    stage = Path(result["stage"])

    assert (stage / "Data").is_symlink()
    assert (stage / "Wiz8.CFG").read_bytes() == b"\x00\xff"
    assert (stage / "Wiz8Runtime.exe").read_bytes() == b"runtime"


def test_stage_runtime_refuses_an_unmanaged_asset_directory(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    unmanaged = settings.repo_dir / "build" / "runtime" / "wiz8" / "Data"
    unmanaged.mkdir(parents=True)

    with pytest.raises(RuntimeError, match="not a managed symlink"):
        stage_runtime(settings)


def test_stage_runtime_selects_the_semantic_test_product(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    executable = settings.repo_dir / "build/decomp/Wiz8RuntimeTest.exe"
    executable.write_bytes(b"semantic tests")

    result = stage_runtime(settings, executable.name)

    assert Path(result["executable"]).read_bytes() == b"semantic tests"


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


def test_gdb_sigsegv_stop_extracts_numbered_backtrace_addresses() -> None:
    diagnosis = _parse_gdb_diagnostics(
        "Thread 1 received signal SIGSEGV, Segmentation fault.\n"
        "#0  0x00401234 in first ()\n"
        "#1  0x7bc45678 in second ()\n",
        0,
    )

    assert diagnosis == {
        "classification": "debug_crashed",
        "stop_reason": "signal SIGSEGV",
        "addresses": [0x00401234, 0x7BC45678],
        "crash_signature": None,
    }


def test_gdb_normal_completion_is_a_debug_pass() -> None:
    diagnosis = _parse_gdb_diagnostics("[Inferior 1 (process 42) exited normally]\n", 0)

    assert diagnosis == {
        "classification": "debug_passed",
        "stop_reason": "normal exit",
        "addresses": [],
    }


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


def test_runtime_timeout_preserves_in_process_diagnostics(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    def time_out(*args, **kwargs):
        raise __import__("subprocess").TimeoutExpired(
            args[0], 45, output=b"partial stdout", stderr=b"menu reached; teardown stuck"
        )

    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", time_out)
    monkeypatch.setattr(
        "wiz8decomp.runtime.diagnose_runtime_failure",
        lambda *_args: {
            "classification": "debug_timeout",
            "stop_reason": "GDB timed out",
            "host_symbol_candidates": [],
            "artifacts": str(tmp_path / "gdb.txt"),
        },
    )

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
