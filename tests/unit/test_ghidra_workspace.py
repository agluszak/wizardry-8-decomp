from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import workspace


def _settings(tmp_path: Path, **overrides: object) -> Settings:
    values = {
        "GHIDRA_INSTALL_DIR": str(tmp_path / "ghidra-install"),
        "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
        "WIZ8_WORK_DIR": str(tmp_path / "work"),
        **overrides,
    }
    return Settings.model_validate(values)


def test_project_dir_defaults_inside_the_checkout(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    assert settings.project_dir == settings.repo_dir / "ghidra-project"


def test_project_dir_override_relocates_the_project(tmp_path: Path) -> None:
    settings = _settings(tmp_path, WIZ8_GHIDRA_PROJECT_DIR=str(tmp_path / "elsewhere"))
    assert settings.project_dir == (tmp_path / "elsewhere").resolve()


def test_owner_check_accepts_this_checkout_and_unclaimed_projects(tmp_path: Path) -> None:
    settings = SimpleNamespace(project_dir=tmp_path / "project", repo_dir=tmp_path / "repo")
    workspace.check_project_owner(settings)

    settings.project_dir.mkdir()
    workspace._write_project_owner(settings)
    workspace.check_project_owner(settings)


def test_owner_check_refuses_another_checkouts_project(tmp_path: Path) -> None:
    project_dir = tmp_path / "project"
    other = SimpleNamespace(project_dir=project_dir, repo_dir=tmp_path / "other-repo")
    project_dir.mkdir()
    workspace._write_project_owner(other)

    mine = SimpleNamespace(project_dir=project_dir, repo_dir=tmp_path / "my-repo")
    with pytest.raises(RuntimeError, match="different checkout"):
        workspace.check_project_owner(mine)


def test_existing_program_does_not_validate_unused_seed_archive(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = SimpleNamespace(project_dir=tmp_path / "project", repo_dir=tmp_path / "repo")
    seed = {
        "program": "wiz8-program",
        "archive": tmp_path / "missing.gzf",
        "binary_sha256": "binary-hash",
    }
    monkeypatch.setattr(workspace, "seed_record", lambda *_args, **_kwargs: seed)
    monkeypatch.setattr(workspace, "_program_hash", lambda *_args: "binary-hash")
    monkeypatch.setattr(
        workspace,
        "_validate_seed_archive",
        lambda *_args: pytest.fail("unused archive was validated"),
    )

    result = workspace.restore_seed(settings, object())

    assert result["status"] == "already-restored"


def test_missing_program_validates_seed_before_import(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = SimpleNamespace(project_dir=tmp_path / "project", repo_dir=tmp_path / "repo")
    seed = {
        "program": "wiz8-program",
        "archive": tmp_path / "seed.gzf",
        "binary_sha256": "binary-hash",
    }
    monkeypatch.setattr(workspace, "seed_record", lambda *_args, **_kwargs: seed)
    monkeypatch.setattr(workspace, "_program_hash", lambda *_args: None)
    monkeypatch.setattr(
        workspace,
        "_validate_seed_archive",
        lambda actual: (_ for _ in ()).throw(RuntimeError(f"validated {actual['archive']}")),
    )

    with pytest.raises(RuntimeError, match="validated .*seed.gzf"):
        workspace.restore_seed(settings, object())
