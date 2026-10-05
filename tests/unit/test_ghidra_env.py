from __future__ import annotations

import sys
from pathlib import Path

import pytest
from wiz8decomp.ghidra.env import _remove_cwd_from_sys_path


def test_remove_cwd_from_sys_path_removes_every_alias(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    cwd = tmp_path / "checkout"
    unrelated = tmp_path / "dependency"
    cwd.mkdir()
    unrelated.mkdir()
    monkeypatch.chdir(cwd)
    monkeypatch.setattr(
        sys,
        "path",
        ["", ".", str(cwd), str(cwd / ".." / cwd.name), str(unrelated)],
    )

    _remove_cwd_from_sys_path()

    assert sys.path == [str(unrelated)]


def test_remove_cwd_from_sys_path_removes_local_java_package_parent(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    cwd = tmp_path / "checkout"
    script_directory = cwd / "build"
    unrelated = tmp_path / "dependency"
    (script_directory / "ghidra").mkdir(parents=True)
    unrelated.mkdir()
    monkeypatch.chdir(cwd)
    monkeypatch.setattr(sys, "path", [str(script_directory), str(unrelated)])

    _remove_cwd_from_sys_path()

    assert sys.path == [str(unrelated)]


@pytest.mark.parametrize("revision", [None, "upstream", "required"])
def test_environment_requires_pinned_fork_revision(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, revision: str | None
) -> None:
    from types import SimpleNamespace

    from wiz8decomp.config import (
        REQUIRED_GHIDRA_RELEASE,
        REQUIRED_GHIDRA_REVISION,
        REQUIRED_GHIDRA_VERSION,
        REQUIRED_PYGHIDRA_VERSION,
        Settings,
    )
    from wiz8decomp.ghidra.env import validate_environment

    install = tmp_path / "install"
    (install / "Ghidra").mkdir(parents=True)
    properties = (
        f"application.version={REQUIRED_GHIDRA_VERSION}\n"
        f"application.release.name={REQUIRED_GHIDRA_RELEASE}\n"
    )
    if revision is not None:
        properties += (
            "application.fork.revision="
            + (REQUIRED_GHIDRA_REVISION if revision == "required" else revision)
            + "\n"
        )
    (install / "Ghidra/application.properties").write_text(properties)
    settings = Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": install,
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
        }
    )
    monkeypatch.setitem(
        sys.modules, "pyghidra", SimpleNamespace(__version__=REQUIRED_PYGHIDRA_VERSION)
    )
    monkeypatch.delenv("GHIDRA_INSTALL_DIR", raising=False)
    if revision == "required":
        assert validate_environment(settings)["ghidra_revision"] == REQUIRED_GHIDRA_REVISION
    else:
        with pytest.raises(RuntimeError, match="Ghidra fork revision"):
            validate_environment(settings)
