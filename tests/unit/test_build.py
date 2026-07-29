import json
from fcntl import LOCK_EX, LOCK_NB, flock
from pathlib import Path

import pytest
from wiz8decomp.build import (
    PRODUCT_GENERATOR,
    TARGET_ALIASES,
    ContainerBuild,
    Mount,
    _product_cache_ready,
    build_lock,
)
from wiz8decomp.config import Settings


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
    assert TARGET_ALIASES == {"match": "WIZ8", "runtime": "WIZ8_RUNTIME"}


def test_container_commands_preserve_vc6_configuration_and_target(tmp_path: Path) -> None:
    build = ContainerBuild.from_settings(_settings(tmp_path))

    configure = build.configure_command()
    compile_command = build.build_command("WIZ8", 7)

    assert "-G" in configure
    assert PRODUCT_GENERATOR == "NMake Makefiles JOM"
    assert PRODUCT_GENERATOR in configure
    assert "-DCMAKE_MAKE_PROGRAM=C:/jom/jom.exe" in configure
    assert "-DSGP_SOURCE=Z:/repo/third_party/sfi-sgp/sgp" in configure
    assert compile_command[-2:] == [
        "/c",
        "set TEMP=Z:\\out\\tmp&& set TMP=Z:\\out\\tmp&& cd /d Z:\\out&& C:\\jom\\jom.exe -j 7 WIZ8",
    ]


def test_justfile_contains_aliases_not_host_implementation() -> None:
    repository = Path(__file__).resolve().parents[2]
    justfile = (repository / "Justfile").read_text(encoding="utf-8")

    assert "docker" not in justfile
    assert "wine" not in justfile.casefold()
    assert "uv run wiz8 build" in justfile
    assert 'build target="match"' in justfile
    assert 'compare target="WIZ8"' in justfile

    dockerfile = (repository / "docker/msvc600/Dockerfile").read_text(encoding="utf-8")
    assert dockerfile.startswith("FROM debian:bookworm-20260623-slim@sha256:")


def test_product_cache_requires_the_supported_generator(tmp_path: Path) -> None:
    cache = tmp_path / "CMakeCache.txt"

    assert _product_cache_ready(tmp_path) is False
    cache.write_text("CMAKE_GENERATOR:INTERNAL=NMake Makefiles JOM\r\n", encoding="utf-8")
    assert _product_cache_ready(tmp_path) is True
    cache.write_text("CMAKE_GENERATOR:INTERNAL=Ninja\r\n", encoding="utf-8")
    assert _product_cache_ready(tmp_path) is False


def test_build_lock_reports_the_current_holder(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    lock_path = settings.repo_dir / "build/decomp/.wiz8-build.lock"
    lock_path.parent.mkdir(parents=True)
    lock_path.write_text(json.dumps({"pid": 42, "command": ["wiz8", "build"]}))

    with lock_path.open("a+") as holder:
        flock(holder.fileno(), LOCK_EX | LOCK_NB)
        with pytest.raises(RuntimeError, match='"pid": 42'), build_lock(settings):
            pass
