import json
from fcntl import LOCK_EX, LOCK_NB, flock
from pathlib import Path

import pytest
import wiz8decomp.build as build_module
from wiz8decomp import reccmp_workflows, runtime, source_index, source_layouts, unresolved
from wiz8decomp.build import (
    PRODUCT_GENERATOR,
    ContainerBuild,
    Mount,
    _enable_jom_parallelism,
    _product_cache_ready,
    _reccmp_configs_ready,
    build_lock,
)
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import index as ghidra_index
from wiz8decomp.ghidra import reccmp_import


def _settings(tmp_path: Path) -> Settings:
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
            "repo_dir": tmp_path / "repo",
        }
    )


def test_container_build_owns_the_product_mounts(tmp_path: Path) -> None:
    build = ContainerBuild.from_settings(_settings(tmp_path))

    assert build.image == "wizardry8-msvc600:sp5"
    assert build.docker_prefix()[1:5] == ["run", "--rm", "--init", "--network"]
    assert build.mounts[0] == Mount(tmp_path / "repo", "/repo")
    assert build.mounts[-1] == Mount(tmp_path / "repo/build/decomp", "/out", False)


def test_container_commands_preserve_vc6_configuration_and_target(tmp_path: Path) -> None:
    build = ContainerBuild.from_settings(_settings(tmp_path))

    configure = build.configure_command()
    compile_command = build.build_command("WIZ8", 7)

    assert "-G" in configure
    assert PRODUCT_GENERATOR in configure
    assert not any(argument.startswith("-DCMAKE_MAKE_PROGRAM=") for argument in configure)
    assert "-DSGP_SOURCE=Z:/repo/third_party/sfi-sgp/sgp" in configure
    assert compile_command[-2:] == [
        "/c",
        "set TEMP=Z:\\out\\tmp&& set TMP=Z:\\out\\tmp&& cd /d Z:\\out&& C:\\jom\\jom.exe -j 7 WIZ8",
    ]


def test_product_cache_requires_the_supported_generator(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"

    assert _product_cache_ready(tmp_path) is False
    cache.write_text("CMAKE_GENERATOR:INTERNAL=NMake Makefiles\r\n", encoding="utf-8")
    assert _product_cache_ready(tmp_path) is True
    cache.write_text("CMAKE_GENERATOR:INTERNAL=Ninja\r\n", encoding="utf-8")
    assert _product_cache_ready(tmp_path) is False


def test_reccmp_configs_require_every_project_target(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    repository = settings.repo_dir
    build_dir = repository / "build/decomp"
    build_dir.mkdir(parents=True)
    (repository / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8: {}\n  SURRENDER: {}\n", encoding="utf-8"
    )
    (repository / "reccmp-user.yml").write_text(
        "targets:\n  WIZ8:\n    path: original.exe\n", encoding="utf-8"
    )
    (build_dir / "reccmp-build.yml").write_text(
        "targets:\n  WIZ8:\n    path: Wiz8.exe\n    pdb: Wiz8.pdb\n",
        encoding="utf-8",
    )

    assert _reccmp_configs_ready(settings) is False

    (repository / "reccmp-user.yml").write_text(
        "targets:\n  WIZ8:\n    path: original.exe\n  SURRENDER:\n    path: sr.dll\n",
        encoding="utf-8",
    )
    (build_dir / "reccmp-build.yml").write_text(
        "targets:\n"
        "  WIZ8:\n    path: Wiz8.exe\n    pdb: Wiz8.pdb\n"
        "  SURRENDER:\n    path: sr.dll\n    pdb: sr.pdb\n",
        encoding="utf-8",
    )

    assert _reccmp_configs_ready(settings) is True


def test_parallel_adaptation_removes_only_generated_serial_guards(tmp_path: Path) -> None:
    top = tmp_path / "Makefile"
    nested = tmp_path / "CMakeFiles/Makefile2"
    nested.parent.mkdir()
    top.write_bytes(b"default_target: all\r\n.NOTPARALLEL:\r\n")
    nested.write_bytes(b"all: target\n.NOTPARALLEL:\n")

    assert _enable_jom_parallelism(tmp_path) == [str(top), str(nested)]
    assert b".NOTPARALLEL:" not in top.read_bytes()
    assert b".NOTPARALLEL:" not in nested.read_bytes()
    assert _enable_jom_parallelism(tmp_path) == []


def test_build_lock_reports_the_current_holder(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    lock_path = settings.repo_dir / "build/decomp/.wiz8-build.lock"
    lock_path.parent.mkdir(parents=True)
    lock_path.write_text(json.dumps({"pid": 42, "command": ["wiz8", "build"]}))

    with lock_path.open("a+") as holder:
        flock(holder.fileno(), LOCK_EX | LOCK_NB)
        with pytest.raises(RuntimeError, match='"pid": 42'), build_lock(settings):
            pass


def test_full_diagnostics_uses_an_isolated_build_and_explicit_target(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    commands: list[list[str]] = []

    monkeypatch.setattr(build_module, "resolve_executable", lambda _name: "/usr/bin/docker")

    def fake_run(command: list[str], **_kwargs) -> build_module.CommandResult:
        commands.append(command)
        return build_module.CommandResult(
            argv=command,
            executable=command[0],
            cwd=str(settings.repo_dir),
            exit_status=0,
            stdout="",
            stderr="",
            timestamp_utc="2026-07-30T00:00:00+00:00",
        )

    monkeypatch.setattr(build_module, "run", fake_run)

    result = build_module.lint(settings, full_diagnostics=True)

    assert result["mode"] == "full-diagnostics"
    assert "-DWIZ8_FULL_DIAGNOSTICS=ON" in commands[0]
    assert "WIZ8_CLANG_DIAGNOSTICS" in commands[1]
    assert (settings.repo_dir / build_module.DIAGNOSTICS_BUILD_DIR).is_dir()


def test_strict_warning_surface_enables_every_recovery_diagnostic() -> None:
    cmake = (Path(__file__).resolve().parents[2] / "CMakeLists.txt").read_text()

    assert "set(WIZ8_STRICT_WARNING_UNITS" in cmake
    assert "src/wiz8/engine_code/BitArray.cpp" in cmake
    for warning in (
        "-Werror=sometimes-uninitialized",
        "-Werror=switch",
        "-Werror=array-bounds",
        "-Werror=sign-compare",
        "-Werror=missing-field-initializers",
    ):
        assert warning in cmake


def test_verify_builds_and_runs_the_runtime_semantic_suite(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    built: list[str] = []

    monkeypatch.setattr(build_module, "lint", lambda _settings: {"lint": "ok"})
    monkeypatch.setattr(
        build_module,
        "build_target",
        lambda _settings, target: built.append(target),
    )
    monkeypatch.setattr(build_module, "build_analysis_target", lambda *_args: None)
    monkeypatch.setattr(
        build_module,
        "run",
        lambda command, **_kwargs: build_module.CommandResult(
            argv=command,
            executable=command[0],
            cwd=str(settings.repo_dir),
            exit_status=0,
            stdout="",
            stderr="",
            timestamp_utc="2026-07-29T00:00:00+00:00",
        ),
    )
    monkeypatch.setattr(build_module, "compare", lambda *_args, **_kwargs: {"match": "ok"})

    monkeypatch.setattr(source_index, "write_source_index", lambda *_args: {"index": "ok"})
    monkeypatch.setattr(
        reccmp_import,
        "import_reccmp_source",
        lambda *_args: {"import": "ok"},
    )
    monkeypatch.setattr(ghidra_index, "export_index", lambda *_args: {"index": "ok"})
    monkeypatch.setattr(
        reccmp_workflows,
        "compare_vtables",
        lambda *_args: {"ok": True},
    )
    monkeypatch.setattr(source_layouts, "verify_source_layouts", lambda *_args: {"valid": True})
    monkeypatch.setattr(
        source_layouts,
        "verify_source_layout_delta",
        lambda _settings, _current, against: {"valid": True, "against": against},
    )
    monkeypatch.setattr(source_layouts, "require_source_layout_delta", lambda result: result)
    monkeypatch.setattr(
        unresolved,
        "unresolved_report",
        lambda *_args: {"by_symbol": {}, "ranked_units": [], "near_link_complete_units": []},
    )
    monkeypatch.setattr(
        unresolved,
        "verify_unresolved_delta",
        lambda _settings, _current: {"valid": True},
    )
    monkeypatch.setattr(unresolved, "require_unresolved_delta", lambda result: result)
    monkeypatch.setattr(
        runtime,
        "run_runtime_suite",
        lambda _settings: {"deterministic": True},
    )

    result = build_module.verify(settings)

    assert built == ["WIZ8", "SURRENDER", "WIZ8_RUNTIME_TEST"]
    assert result["runtime_tests"] == {"deterministic": True}
    assert result["source_layouts"]["against"] is None
