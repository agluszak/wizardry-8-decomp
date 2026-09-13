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
