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


def _project_settings(tmp_path: Path) -> SimpleNamespace:
    return SimpleNamespace(
        project_dir=tmp_path / "project", repo_dir=tmp_path / "repo", project_name="wizardry8"
    )


def _seed(sha256: str = "seed-hash") -> dict[str, str | Path]:
    return {
        "program": "wiz8-program",
        "archive": Path("seed.gzf"),
        "binary_sha256": "binary-hash",
        "sha256": sha256,
    }


def _create_project(settings: SimpleNamespace) -> None:
    settings.project_dir.mkdir(parents=True, exist_ok=True)
    (settings.project_dir / f"{settings.project_name}.gpr").touch()


def test_project_dir_defaults_inside_the_checkout(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    assert settings.project_dir == settings.repo_dir / "ghidra-project"


def test_project_dir_override_relocates_the_project(tmp_path: Path) -> None:
    settings = _settings(tmp_path, WIZ8_GHIDRA_PROJECT_DIR=str(tmp_path / "elsewhere"))
    assert settings.project_dir == (tmp_path / "elsewhere").resolve()


def test_owner_check_accepts_this_checkout_and_unclaimed_projects(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    workspace.check_project_owner(settings)

    settings.project_dir.mkdir()
    workspace._write_project_owner(settings)
    workspace.check_project_owner(settings)


def test_owner_check_refuses_another_checkouts_project(tmp_path: Path) -> None:
    project_dir = tmp_path / "project"
    other = SimpleNamespace(
        project_dir=project_dir, repo_dir=tmp_path / "other-repo", project_name="wizardry8"
    )
    project_dir.mkdir()
    workspace._write_project_owner(other)

    mine = SimpleNamespace(
        project_dir=project_dir, repo_dir=tmp_path / "my-repo", project_name="wizardry8"
    )
    with pytest.raises(RuntimeError, match="different checkout"):
        workspace.check_project_owner(mine)


def test_seed_freshness_accepts_project_not_restored_yet(tmp_path: Path) -> None:
    result = workspace.project_seed_freshness(_project_settings(tmp_path), _seed())

    assert result["ok"] is True
    assert result["status"] == "not-restored"


def test_seed_freshness_rejects_legacy_project_without_seed_provenance(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    _create_project(settings)
    workspace._write_project_owner(settings)

    result = workspace.project_seed_freshness(settings, _seed())

    assert result["ok"] is False
    assert result["status"] == "unknown"


def test_seed_freshness_detects_newer_reviewed_checkpoint(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    _create_project(settings)
    workspace.record_project_seed(settings, _seed("old-seed"))

    result = workspace.project_seed_freshness(settings, _seed("new-seed"))

    assert result["ok"] is False
    assert result["status"] == "stale"
    assert result["recorded_seed_sha256"] == "old-seed"
    assert result["expected_seed_sha256"] == "new-seed"


def test_recorded_current_seed_is_accepted(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    _create_project(settings)
    seed = _seed()
    workspace.record_project_seed(settings, seed)

    result = workspace.project_seed_freshness(settings, seed)

    assert result["ok"] is True
    assert result["status"] == "current"


def test_existing_program_does_not_validate_unused_seed_archive(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _project_settings(tmp_path)
    seed = {
        "program": "wiz8-program",
        "archive": tmp_path / "missing.gzf",
        "binary_sha256": "binary-hash",
        "sha256": "seed-hash",
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


def test_existing_program_refuses_known_stale_reviewed_seed(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _project_settings(tmp_path)
    _create_project(settings)
    workspace.record_project_seed(settings, _seed("old-seed"))
    current = {
        "program": "wiz8-program",
        "archive": tmp_path / "current.gzf",
        "binary_sha256": "binary-hash",
        "sha256": "new-seed",
    }
    monkeypatch.setattr(workspace, "seed_record", lambda *_args, **_kwargs: current)
    monkeypatch.setattr(workspace, "_program_hash", lambda *_args: "binary-hash")

    with pytest.raises(RuntimeError, match="run `uv run wiz8 doctor`"):
        workspace.restore_seed(settings, object())


def test_missing_program_validates_seed_before_import(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _project_settings(tmp_path)
    seed = {
        "program": "wiz8-program",
        "archive": tmp_path / "seed.gzf",
        "binary_sha256": "binary-hash",
        "sha256": "seed-hash",
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


def test_source_projection_freshness_never_when_unrecorded(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    settings.repo_dir.mkdir()
    (settings.repo_dir / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    result = workspace.source_projection_freshness(settings, "wiz8-program")
    assert result["ok"] is True
    assert result["status"] == "never"
    assert "reviewed-seed origin" in result["detail"]


def test_source_projection_freshness_stale_after_index_change(tmp_path: Path) -> None:
    settings = _project_settings(tmp_path)
    settings.project_dir.mkdir(parents=True)
    settings.repo_dir.mkdir()
    (settings.repo_dir / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    source-root: src/wiz8\n"
        "    hash:\n      sha256: abc\n",
        encoding="utf-8",
    )
    build = settings.repo_dir / "build"
    build.mkdir()
    index = build / "source-index.json"
    index.write_text(
        '{"schema": "reccmp-source-index-v2", "markers": []}',
        encoding="utf-8",
    )
    workspace._write_project_owner(settings)
    workspace.record_source_projection(
        settings,
        "wiz8-program",
        {"source_index_sha256": "old", "applied_ns": 1, "target": "WIZ8"},
    )
    result = workspace.source_projection_freshness(settings, "wiz8-program")
    assert result["ok"] is True
    assert result["status"] == "stale"
    assert "wiz8 ghidra sync" in result["detail"]
