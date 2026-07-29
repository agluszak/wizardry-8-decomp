from __future__ import annotations

from pathlib import Path

from wiz8decomp.config import Settings
from wiz8decomp.ghidra import cache


def _settings(tmp_path: Path) -> Settings:
    for name in ("ghidra-install", "inputs", "work", "repo"):
        (tmp_path / name).mkdir()
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra-install",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
            "repo_dir": tmp_path / "repo",
        }
    )


def test_settings_name_one_canonical_project(tmp_path: Path) -> None:
    settings = _settings(tmp_path)

    assert settings.project_dir == settings.work_dir / "ghidra"
    assert settings.project_name == "wizardry8"


def test_existing_program_is_reused_without_replay(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    monkeypatch.setattr(cache, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(cache, "_program_exists", lambda _settings, _program: True)

    actual, report = cache.materialize_program(settings, "canonical")

    assert actual is settings
    assert report["status"] == "existing"
    assert report["project_dir"] == str(settings.project_dir)


def test_mutations_use_the_same_project(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    restored: list[tuple[Settings, str | None]] = []
    started: list[Settings] = []
    monkeypatch.setattr(
        cache,
        "materialize_program",
        lambda actual, selector: (
            restored.append((actual, selector)) or (actual, {"status": "existing"})
        ),
    )
    monkeypatch.setattr(cache, "start_pyghidra", lambda actual: started.append(actual))

    actual = cache.open_for_mutation(settings, "canonical")

    assert actual is settings
    assert restored == [(settings, "canonical")]
    assert started == [settings]


def test_legacy_materialization_plumbing_is_gone() -> None:
    source = Path(cache.__file__).read_text(encoding="utf-8")

    for obsolete in (
        "materialization_key",
        "ghidra-agents",
        "query.sock",
        "WIZ8_GHIDRA_KEEP_MATERIALIZATIONS",
        "ghidra_agent_id",
    ):
        assert obsolete not in source
