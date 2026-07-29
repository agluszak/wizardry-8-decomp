import json
from fcntl import LOCK_EX, LOCK_NB, flock
from pathlib import Path

import pytest
import wiz8decomp.build as build_module
from wiz8decomp import reccmp_workflows, runtime, source_index, source_layouts
from wiz8decomp.build import (
    PRODUCT_GENERATOR,
    ContainerBuild,
    Mount,
    _enable_jom_parallelism,
    _product_cache_ready,
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
    monkeypatch.setattr(source_layouts, "require_source_layouts", lambda result: result)
    monkeypatch.setattr(
        runtime,
        "run_runtime_suite",
        lambda _settings: {"deterministic": True},
    )

    result = build_module.verify(settings)

    assert built == ["WIZ8", "SURRENDER", "WIZ8_RUNTIME_TEST"]
    assert result["runtime_tests"] == {"deterministic": True}
