import subprocess
import time
from contextlib import nullcontext
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.display import runtime_display
from wiz8decomp.runtime import (
    _compare_repetitions,
    _crash_detail,
    _parse_runtime_crash,
    _parse_runtime_observation,
    _parse_wine_dump,
    _run_runtime_scenario,
    _runtime_failure,
    _runtime_history,
    _runtime_phase_summary,
    _semantic_observation,
    _symbolize_addresses,
    analyze_runtime_crash,
    configure_wine_window_management,
    run_product,
    run_runtime_suite,
    runtime_test_environment,
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
    assert result.executable_written is True

    restaged = stage_game(
        settings,
        name="runtime-test",
        executable=settings.product_build_dir / "Wiz8RuntimeTest.exe",
    )
    assert restaged.executable_written is False
    assert restaged.executable == result.executable
    assert (stage / "Wiz8.CFG").read_bytes() == b"\x00\xff"


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
    prefix = settings.work_dir / "wine" / "wiz8-runtime"
    prefix.mkdir(parents=True)
    (prefix / "system.reg").write_text("")
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
    monkeypatch.delenv("WIZ8_WINE_VIRTUAL_DESKTOP", raising=False)

    run_product(settings)

    assert calls[0][0][0][-3:] == ["/d", "Y", "/f"]
    # Host display maps Wiz8 as an ordinary managed window: the Wine Explorer
    # Desktop value is removed rather than pointing at a 640x480 shell.
    assert calls[1][0][0][-4:] == [r"HKCU\Software\Wine\Explorer", "/v", "Desktop", "/f"]
    assert calls[2][0][0][-2:] == ["./Wiz8Runtime.exe", "/WINDOW"]
    assert calls[2][1]["env"]["WINEPREFIX"] == str(settings.work_dir / "wine" / "wiz8-runtime")


def test_runtime_test_environment_honours_explicit_renderer(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    monkeypatch.delenv("GALLIUM_DRIVER", raising=False)

    _, environment = runtime_test_environment(settings)
    assert "GALLIUM_DRIVER" not in environment

    _, environment = runtime_test_environment(settings, renderer="softpipe")
    assert environment["GALLIUM_DRIVER"] == "softpipe"


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


def test_runtime_history_collects_completed_actions_with_observations() -> None:
    output = (
        "WIZ8_RUNTIME_STEP scenario=save-load-move step=game-saved state=pass "
        "elapsed_ms=100 screen=9 pending=-1 pos=1,2,3 combat=0 motion=2\n"
        "WIZ8_RUNTIME_STEP scenario=other-case step=engine-ready state=pass elapsed_ms=50\n"
        "WIZ8_RUNTIME_STEP scenario=save-load-move step=quickload state=pass "
        "elapsed_ms=200 screen=9 pending=-1 combat=0 motion=0\n"
    )

    assert _runtime_history(output, "save-load-move") == [
        {
            "action": "game-saved",
            "elapsed_ms": 100,
            "screen": 9,
            "pending": -1,
            "combat": 0,
            "motion": 2,
            "pos": [1, 2, 3],
        },
        {
            "action": "quickload",
            "elapsed_ms": 200,
            "screen": 9,
            "pending": -1,
            "combat": 0,
            "motion": 0,
        },
    ]


def test_semantic_observation_drops_volatile_fields() -> None:
    observation = {
        "scenario": "save-load-move",
        "case_passed": 1,
        "elapsed_ms": 1200,
        "history": [{"action": "game-saved"}],
    }

    assert _semantic_observation(observation) == {"scenario": "save-load-move", "case_passed": 1}


def test_repetition_comparison_reports_diverging_observations() -> None:
    first = {"scenario": "s", "menu_seen": 1, "playlist_tracks": 4}
    repeated = {"scenario": "s", "menu_seen": 1, "playlist_tracks": 3}

    problems = _compare_repetitions("s", [first, repeated])

    assert problems == ["s: repetition 2 disagrees with the first run: playlist_tracks: 4 != 3"]


def test_repetition_comparison_keeps_a_flake_a_failure() -> None:
    passed = {"scenario": "s", "menu_seen": 1}
    failed = {"scenario": "s", "failure": "condition-not-met"}

    problems = _compare_repetitions("s", [passed, failed, passed])

    assert problems == ["s: flaky across 3 repetitions: 1 failed"]


def test_repetition_comparison_accepts_identical_runs() -> None:
    passed = {"scenario": "s", "menu_seen": 1, "history": [{"action": "a", "elapsed_ms": 9}]}

    assert (
        _compare_repetitions(
            "s", [passed, dict(passed, history=[{"action": "a", "elapsed_ms": 20}])]
        )
        == []
    )


def test_runtime_failure_reports_native_reason_instead_of_timeout(tmp_path: Path) -> None:
    failure = _runtime_failure(
        "hostile-encounter",
        2,
        "",
        "WIZ8_RUNTIME_STEP scenario=hostile-encounter step=main-game-entered state=pass "
        "elapsed_ms=13900\n"
        "WIZ8_RUNTIME_FAILURE scenario=hostile-encounter step=monster-engagement "
        "reason=monster-never-engaged line=2288\n",
        tmp_path,
        Path("Wiz8RuntimeTest.exe"),
    )

    assert "step=monster-engagement reason=monster-never-engaged line=2288" in str(failure)
    assert "phases: main-game-entered=13900ms" in str(failure)
    assert "timeout" not in str(failure)


@pytest.mark.parametrize(
    "cleanup_error",
    [
        subprocess.TimeoutExpired("wineserver", 5),
        OSError("wineserver missing"),
    ],
)
def test_scenario_cleanup_failure_preserves_primary_failure(
    tmp_path: Path, monkeypatch, cleanup_error
) -> None:
    import subprocess as real_subprocess

    fake_wine = tmp_path / "fake-wine.sh"
    fake_wine.write_text(
        "#!/bin/sh\n"
        "echo 'WIZ8_RUNTIME_FAILURE scenario=probe step=step-a reason=crashed line=9' >&2\n"
        "exit 3\n"
    )
    fake_wine.chmod(0o755)
    real_popen = real_subprocess.Popen
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.Popen",
        lambda command, **kwargs: real_popen([str(fake_wine), *command[1:]], **kwargs),
    )

    def failing_cleanup(*args, **kwargs):
        raise cleanup_error

    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", failing_cleanup)

    with pytest.raises(RuntimeError) as error:
        _run_runtime_scenario(
            tmp_path / "Wiz8RuntimeTest.exe",
            tmp_path,
            {},
            "probe",
            30,
        )

    assert "step=step-a reason=crashed line=9" in str(error.value)
    artifact = tmp_path / "diagnostics" / "probe-failure.txt"
    assert "wineserver -k failed" in artifact.read_text()


def test_runtime_phase_summary_ignores_other_scenarios_and_unusable_steps() -> None:
    output = (
        "WIZ8_RUNTIME_STEP scenario=main-game-start step=main-menu-reached state=pass "
        "elapsed_ms=9000\n"
        "WIZ8_RUNTIME_STEP scenario=other step=ignored state=pass elapsed_ms=1\n"
        "WIZ8_RUNTIME_STEP scenario=main-game-start step=no-time state=pass\n"
        "WIZ8_RUNTIME_STEP scenario=main-game-start step=main-game-entered state=pass "
        "elapsed_ms=14000"
    )

    assert (
        _runtime_phase_summary(output, "main-game-start")
        == "main-menu-reached=9000ms -> main-game-entered=14000ms"
    )


def _registry():
    from wiz8decomp.runtime import _parse_runtime_scenarios

    return _parse_runtime_scenarios(
        "name\tphase\ttier\tkind\ttimeout_ms\tfixture\tpath\tbatch\n"
        "main-menu-startup\tmain-menu\tpr\tintegration\t15000\tmain-menu\tnatural\tno\n"
        "split-stack\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tnatural\tyes\n"
    )


@pytest.mark.parametrize(
    "row",
    [
        "../escape\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tnatural\tno",
        "probe\tunknown\tpr\tsemantic\t15000\tengine-ready\tnatural\tno",
        "probe\tengine-ready\tpr\tsemantic\t0\tengine-ready\tnatural\tno",
        "probe\tengine-ready\tpr\tsemantic\t-1\tengine-ready\tnatural\tno",
        "probe\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tunknown\tno",
        "probe\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tnatural\tmaybe",
        (
            "probe\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tnatural\tno\n"
            "probe\tengine-ready\tpr\tsemantic\t15000\tengine-ready\tnatural\tno"
        ),
    ],
)
def test_registry_rejects_unsafe_or_ambiguous_metadata(row):
    from wiz8decomp.runtime import _parse_runtime_scenarios

    with pytest.raises(RuntimeError):
        _parse_runtime_scenarios(
            "name\tphase\ttier\tkind\ttimeout_ms\tfixture\tpath\tbatch\n" + row
        )


@pytest.mark.parametrize("all_cases_reported", [False, True])
def test_batch_error_only_when_the_process_dies(
    tmp_path: Path, monkeypatch, all_cases_reported
) -> None:
    """A reported case failure inside a batch exits the process nonzero; that
    is an ordinary case result, not a dead batch. Only a process that died
    before reporting every case produces the batch error."""
    from wiz8decomp.runtime import _run_runtime_batch, _RuntimeProcessResult

    registry = _registry()
    scenarios = ("main-menu-startup", "split-stack")

    def drive(*args, **kwargs):
        stdout = "WIZ8_RUNTIME_TEST scenario=main-menu-startup case_passed=0\n"
        if all_cases_reported:
            stdout += "WIZ8_RUNTIME_TEST scenario=split-stack case_passed=1\n"
            stdout += "WIZ8_RUNTIME_SESSION cases=2 driver=2 teardown=1\n"
        return _RuntimeProcessResult(
            stdout=stdout,
            stderr="WIZ8_RUNTIME_FAILURE scenario=main-menu-startup step=x reason=y line=1\n",
            returncode=1,
            timed_out=False,
            failed_early=False,
            last_step="x",
            last_step_scenario="split-stack",
            elapsed=1.0,
        )

    monkeypatch.setattr("wiz8decomp.runtime._drive_runtime_process", drive)
    observations, error = _run_runtime_batch(
        tmp_path / "Wiz8RuntimeTest.exe",
        tmp_path,
        {},
        scenarios,
        registry,
    )

    assert observations["main-menu-startup"]["case_passed"] == 0
    if all_cases_reported:
        assert len(observations) == 2
        assert error is None
    else:
        assert len(observations) == 1
        assert "batch process died after 1/2 cases" in error
        assert "in-flight=split-stack" in error


def test_batch_teardown_failure_fails_the_suite(tmp_path: Path, monkeypatch) -> None:
    """Every case reports case_passed=1 but final SGPExit teardown failed:
    the session record, not the exit code, is the verdict."""
    from wiz8decomp.runtime import _run_runtime_batch, _RuntimeProcessResult

    registry = _registry()

    def drive(*args, **kwargs):
        return _RuntimeProcessResult(
            stdout=(
                "WIZ8_RUNTIME_TEST scenario=main-menu-startup case_passed=1\n"
                "WIZ8_RUNTIME_TEST scenario=split-stack case_passed=1\n"
                "WIZ8_RUNTIME_SESSION cases=2 driver=0 teardown=0\n"
            ),
            stderr="",
            returncode=1,
            timed_out=False,
            failed_early=False,
            last_step="winmain-returned",
            last_step_scenario="split-stack",
            elapsed=1.0,
        )

    monkeypatch.setattr("wiz8decomp.runtime._drive_runtime_process", drive)
    observations, error = _run_runtime_batch(
        tmp_path / "Wiz8RuntimeTest.exe",
        tmp_path,
        {},
        ("main-menu-startup", "split-stack"),
        registry,
    )

    assert all(observation["case_passed"] == 1 for observation in observations.values())
    assert error == "batch teardown failed (session teardown=0)"


def test_batch_without_a_session_record_fails_closed(tmp_path: Path, monkeypatch) -> None:
    """All cases reported success but no session record exists: the verdict
    cannot be established, so it is not silently a pass."""
    from wiz8decomp.runtime import _run_runtime_batch, _RuntimeProcessResult

    registry = _registry()

    def drive(*args, **kwargs):
        return _RuntimeProcessResult(
            stdout=(
                "WIZ8_RUNTIME_TEST scenario=main-menu-startup case_passed=1\n"
                "WIZ8_RUNTIME_TEST scenario=split-stack case_passed=1\n"
            ),
            stderr="",
            returncode=0,
            timed_out=False,
            failed_early=False,
            last_step="winmain-returned",
            last_step_scenario="split-stack",
            elapsed=1.0,
        )

    monkeypatch.setattr("wiz8decomp.runtime._drive_runtime_process", drive)
    _, error = _run_runtime_batch(
        tmp_path / "Wiz8RuntimeTest.exe",
        tmp_path,
        {},
        ("main-menu-startup", "split-stack"),
        registry,
    )

    assert error == "batch ended without a session record"


@pytest.mark.parametrize("check_order", [False, True])
@pytest.mark.parametrize("repeat", [1, 3])
def test_runtime_suite_selection_and_server_lifetime(
    tmp_path: Path, monkeypatch, check_order, repeat
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

    monkeypatch.setattr("wiz8decomp.runtime._read_runtime_scenarios", lambda *args: _registry())

    def run(executable, stage, environment, scenario, timeout_seconds, object_root, map_path):
        assert timeout_seconds == 15
        visited.append((scenario, stage))
        (stage / "scenario-output.tmp").write_text(scenario)
        return {"scenario": scenario, "teardown": 1}

    monkeypatch.setattr("wiz8decomp.runtime._run_runtime_scenario", run)
    result = run_runtime_suite(
        settings, scenarios=scenarios, check_order=check_order, repeat=repeat
    )
    expected = (list(scenarios) + (list(reversed(scenarios)) if check_order else [])) * repeat
    assert [scenario for scenario, _ in visited] == expected
    assert len({stage for _, stage in visited}) == len(visited)
    assert all(
        (stage / "scenario-output.tmp").read_text() == scenario for scenario, stage in visited
    )
    assert shutdowns == [["wineserver", "-k"]]
    assert result["deterministic"] is (True if check_order else None)
    assert result["scenario_stages"] == {
        f"{stage.parent.name}/{scenario}": str(stage) for scenario, stage in visited
    }


@pytest.mark.parametrize("check_order", [False, True])
def test_runtime_suite_preserves_failures_and_continues(
    tmp_path: Path, monkeypatch, check_order
) -> None:
    settings = _settings(tmp_path)
    scenarios = ("main-menu-startup", "split-stack")
    visited = []
    monkeypatch.setattr("wiz8decomp.runtime.shutil.which", lambda name: f"/usr/bin/{name}")
    monkeypatch.setattr(
        "wiz8decomp.runtime.runtime_display", lambda *args, **kwargs: nullcontext(None)
    )
    monkeypatch.setattr(
        "wiz8decomp.runtime.configure_wine_window_management", lambda *args, **kwargs: None
    )
    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", lambda *args, **kwargs: None)

    monkeypatch.setattr("wiz8decomp.runtime._read_runtime_scenarios", lambda *args: _registry())

    def run(executable, stage, environment, scenario, timeout_seconds, object_root, map_path):
        visited.append(scenario)
        if scenario == "main-menu-startup":
            raise RuntimeError(f"startup invariant failed; artifacts={stage}")
        return {"scenario": scenario, "teardown": 1}

    monkeypatch.setattr("wiz8decomp.runtime._run_runtime_scenario", run)
    with pytest.raises(
        RuntimeError, match="forward/main-menu-startup: startup invariant failed"
    ) as error:
        run_runtime_suite(settings, scenarios=scenarios, check_order=check_order)
    assert "depend on scenario order" not in str(error.value)
    assert visited == list(scenarios) + (list(reversed(scenarios)) if check_order else [])


def test_runtime_suite_workers_get_private_prefixes(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    scenarios = ("main-menu-startup", "split-stack")
    prefixes = []
    monkeypatch.setattr("wiz8decomp.runtime.shutil.which", lambda name: f"/usr/bin/{name}")
    monkeypatch.setattr(
        "wiz8decomp.runtime.runtime_display", lambda *args, **kwargs: nullcontext(None)
    )
    monkeypatch.setattr(
        "wiz8decomp.runtime.configure_wine_window_management", lambda *args, **kwargs: None
    )
    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", lambda *args, **kwargs: None)
    monkeypatch.setattr("wiz8decomp.runtime._read_runtime_scenarios", lambda *args: _registry())

    def run(executable, stage, environment, scenario, timeout_seconds, object_root, map_path):
        prefixes.append(environment["WINEPREFIX"])
        return {"scenario": scenario, "teardown": 1}

    monkeypatch.setattr("wiz8decomp.runtime._run_runtime_scenario", run)
    result = run_runtime_suite(settings, scenarios=scenarios, workers=2)

    assert len(set(prefixes)) == 2
    assert result["workers"] == 2
    assert len(result["wine_prefixes"]) == 2
    assert result["input_digest"]


def test_runtime_suite_workers_require_private_virtual_displays(
    tmp_path: Path, monkeypatch
) -> None:
    settings = _settings(tmp_path)
    monkeypatch.setenv("WIZ8_RUNTIME_DISPLAY", "host")
    with pytest.raises(RuntimeError, match="private virtual display"):
        run_runtime_suite(settings, scenarios=("split-stack",), workers=2)


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


def test_staging_skips_rewriting_an_identical_executable(
    tmp_path: Path, synthetic_pe: Path
) -> None:
    settings = _settings(tmp_path)
    executable = settings.product_build_dir / "Wiz8Runtime.exe"
    executable.write_bytes(synthetic_pe.read_bytes())
    map_file = executable.with_suffix(".map")
    map_file.write_text(" Timestamp is 12345678\n")
    first = stage_game(settings, name="wiz8", executable=executable)
    stamped = first.executable.stat().st_mtime_ns
    second = stage_game(settings, name="wiz8", executable=executable)
    assert second.executable == first.executable
    assert second.executable.stat().st_mtime_ns == stamped


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

    symbols = _symbolize_addresses(map_path, [0x00401008, 0x00401012, 0x00401020, 0x00402004])

    assert symbols == [
        "00401008: _first+0x8 [first.obj] first.cpp:10",
        "00401012: _second+0x2 [second.obj]",
        "00402004: _third+0x4 [third.obj] third.cpp:30",
    ]


def test_runtime_crash_symbolizes_reported_candidates(tmp_path: Path) -> None:
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
        "WIZ8_RUNTIME_CANDIDATE source=reg:edx address=00462892 offset=00062892\n"
        "WIZ8_RUNTIME_CANDIDATE source=stack+0x0 address=00462d2a offset=00062d2a\n"
    )

    crash = _parse_runtime_crash(output)

    assert crash is not None
    assert [candidate.address for candidate in crash.candidates] == [0x00462892, 0x00462D2A]
    assert _crash_detail(map_path, None, crash).splitlines()[1:] == [
        "#0 reg:edx: 00462892: _ShowRegionHelp+0x82 [RegionManager.cpp.obj] RegionManager.cpp:746",
    ]


def _wine_crash_fixture(tmp_path: Path) -> tuple[Path, str]:
    map_path = tmp_path / "wine.map"
    map_path.write_text(
        "Wiz8Runtime\n"
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
    )
    return map_path, output


def test_wine_dump_candidates_are_symbolized_without_product_markers(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)

    crash = _parse_wine_dump(output)

    assert crash is not None
    sources = {candidate.source: candidate.address for candidate in crash.candidates}
    assert sources["reg:edx"] == 0x0041FE14
    assert sources["frame"] == 0x00400003
    assert "reg:edx: 0041fe14: _CharacterScreenFrame+0x164" in _crash_detail(map_path, None, crash)


def test_analyze_runtime_crash_falls_back_to_a_wine_dump(tmp_path: Path) -> None:
    map_path, output = _wine_crash_fixture(tmp_path)
    log = tmp_path / "winedbg.log"
    log.write_text(output, encoding="utf-8")

    result = analyze_runtime_crash(log, map_path)

    crash = result["crashes"][0]
    assert "image_base_fault" not in crash
    assert any(
        candidate.get("symbol", "").startswith("_CharacterScreenFrame+0x164")
        for candidate in crash["candidates"]
    )


def test_wine_stack_dump_drives_wide_candidates(tmp_path: Path) -> None:
    _map_path, output = _wine_crash_fixture(tmp_path)

    crash = _parse_wine_dump(output)

    assert crash is not None
    sources = {candidate.source: candidate.address for candidate in crash.candidates}
    assert sources["stack+0xc"] == 0x00400ABC
    assert sources["reg:edx"] == 0x0041FE14
    assert "page fault on write access" in crash.fields["operation"]


def test_wine_wow64_stack_rows_accept_wide_addresses() -> None:
    output = (
        "Unhandled exception: page fault on read access to 0x00000000 in 32-bit code "
        "(0x00400003).\n"
        "Register dump:\n"
        " CS:0023 SS:002b DS:002b ES:002b FS:0063 GS:006b\n"
        " EIP:00400003 ESP:0032fabc EBP:fffffffe EFLAGS:00210246(  R- --  I   - -P- )\n"
        " EAx:00000000 EBX:00000001 ECX:00000000 EDX:0041fe14\n"
        " ESI:00400000 EDI:00400000\n"
        "Stack dump:\n"
        "0x000000000032fabc:  0041fe14 00000000 0032fae0 00400abc\n"
    )

    crash = _parse_wine_dump(output)

    assert crash is not None
    sources = {candidate.source: candidate.address for candidate in crash.candidates}
    assert sources["stack+0xc"] == 0x00400ABC


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
    assert failure["missing"] == ["EIP register dump"]
    assert "Unhandled page fault" in failure["exception"]
    assert "0x00400003" in failure["log_tail"]


def test_runtime_timeout_preserves_in_process_diagnostics(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    wine = tmp_path / "wine"
    wine.write_text(
        "#!/usr/bin/env python3\n"
        "import sys, time\n"
        "print('partial stdout', flush=True)\n"
        "print('menu reached; teardown stuck', file=sys.stderr, flush=True)\n"
        "time.sleep(60)\n"
    )
    wine.chmod(0o755)
    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", lambda *args, **kwargs: None)
    with pytest.raises(RuntimeError, match="last_step=process-start"):
        _run_runtime_scenario(
            tmp_path / "test.exe",
            tmp_path,
            {"PATH": f"{tmp_path}:/usr/bin:/bin"},
            "main-menu-startup",
            timeout_seconds=1,
        )
    diagnostic = tmp_path / "diagnostics" / "main-menu-startup-failure.txt"
    assert "menu reached; teardown stuck" in diagnostic.read_text()
    assert "partial stdout" in diagnostic.read_text()


@pytest.mark.parametrize("crash", [False, True])
@pytest.mark.parametrize("hang", [False, True])
def test_runtime_terminal_failure_has_short_grace_and_preserves_report(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, crash: bool, hang: bool
) -> None:
    report = (
        "WIZ8_RUNTIME_CRASH code=c0000005 eip=00401000\n"
        "WIZ8_RUNTIME_CANDIDATE source=stack+0 address=00402000 offset=00002000\n"
        "WIZ8_RUNTIME_CRASH_END\n"
        if crash
        else "WIZ8_RUNTIME_FAILURE scenario=probe step=fixture reason=broken line=12\n"
    )
    wine = tmp_path / "wine"
    wine.write_text(
        "#!/usr/bin/env python3\nimport sys, time\n"
        f"report = {report!r}\n"
        # Exercise partial pipe reads without losing a candidate before CRASH_END.
        "for line in report.splitlines():\n"
        "    print(line, file=sys.stderr, flush=True)\n"
        "    time.sleep(0.03)\n" + ("time.sleep(60)\n" if hang else "")
    )
    wine.chmod(0o755)
    stopped = []
    monkeypatch.setattr("wiz8decomp.runtime.RUNTIME_FAILURE_GRACE_SECONDS", 0.15)
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run", lambda command, **kwargs: stopped.append(command)
    )
    started = time.monotonic()
    with pytest.raises(RuntimeError, match="probe failed"):
        _run_runtime_scenario(
            tmp_path / "test.exe",
            tmp_path,
            {"PATH": f"{tmp_path}:/usr/bin:/bin"},
            "probe",
            timeout_seconds=5,
        )
    assert time.monotonic() - started < 2
    assert report in (tmp_path / "diagnostics/probe-failure.txt").read_text()
    assert stopped == [["wineserver", "-k"]]


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
    private_display: bool, managed: str, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    calls = []
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run",
        lambda *args, **kwargs: calls.append((args, kwargs)),
    )
    monkeypatch.delenv("WIZ8_WINE_VIRTUAL_DESKTOP", raising=False)

    prefix = tmp_path / "prefix"
    prefix.mkdir()
    (prefix / "system.reg").write_text("")
    environment = {"WINEPREFIX": str(prefix)}
    configure_wine_window_management(environment, private_display=private_display)

    argv = calls[0][0][0]
    assert argv[-3:] == ["/d", managed, "/f"]
    assert calls[0][1]["env"] is environment
    # The Wine virtual desktop is off by default on both display modes.
    assert calls[1][0][0][-4:] == [
        r"HKCU\Software\Wine\Explorer",
        "/v",
        "Desktop",
        "/f",
    ]
    assert len(calls) == 2


@pytest.mark.parametrize("private_display", [True, False])
def test_wine_window_management_virtual_desktop_is_an_explicit_opt_in(
    private_display: bool, monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    calls = []
    monkeypatch.setattr(
        "wiz8decomp.runtime.subprocess.run",
        lambda *args, **kwargs: calls.append((args, kwargs)),
    )
    monkeypatch.setenv("WIZ8_WINE_VIRTUAL_DESKTOP", "1")

    prefix = tmp_path / "prefix"
    prefix.mkdir()
    (prefix / "system.reg").write_text("")
    environment = {"WINEPREFIX": str(prefix)}
    configure_wine_window_management(environment, private_display=private_display)

    assert calls[1][0][0][-5:] == ["/v", "Desktop", "/d", "Wizardry", "/f"]
    assert calls[2][0][0][-5:] == ["/v", "Wizardry", "/d", "640x480", "/f"]


def test_configure_wine_initializes_a_fresh_prefix_without_audio_overrides(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    """Fresh prefixes are populated without WINEDLLOVERRIDES: wine's first-run
    init crashes when the overrides disable dsound."""
    calls = []
    prefix = tmp_path / "fresh-prefix"

    def run(command, **kwargs):
        if command[1:3] == ["reg", "query"]:
            prefix.mkdir(exist_ok=True)
            (prefix / "system.reg").write_text("")
        calls.append((command, kwargs))

    monkeypatch.setattr("wiz8decomp.runtime.subprocess.run", run)
    monkeypatch.delenv("WIZ8_WINE_VIRTUAL_DESKTOP", raising=False)

    environment = {"WINEPREFIX": str(prefix), "WINEDLLOVERRIDES": "dsound=d"}
    configure_wine_window_management(environment, private_display=True)

    assert calls[0][0][1:3] == ["reg", "query"]
    assert "WINEDLLOVERRIDES" not in calls[0][1]["env"]
    assert calls[1][1]["env"] is environment
