from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp import config
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import cache, rebuild


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


def test_replay_hash_tracks_replay_code_and_reviewed_evidence(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    script = settings.repo_dir / "tools/wiz8decomp/ghidra/apply.py"
    evidence = settings.repo_dir / "evidence/reviewed/wiz8/functions.csv"
    script.parent.mkdir(parents=True)
    evidence.parent.mkdir(parents=True)
    script.write_text("first", encoding="utf-8")
    evidence.write_text("identity", encoding="utf-8")

    first = cache.replay_input_sha256(settings)
    bytecode = script.parent / "__pycache__/apply.pyc"
    bytecode.parent.mkdir()
    bytecode.write_bytes(b"runtime cache")
    assert cache.replay_input_sha256(settings) == first
    evidence.write_text("changed identity", encoding="utf-8")
    second = cache.replay_input_sha256(settings)

    assert first != second


def test_agent_project_is_isolated_and_content_addressed(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    # The primary key wins over CODEX_THREAD_ID, so a real one in the
    # environment would otherwise mask what this test sets.
    monkeypatch.delenv("WIZ8_GHIDRA_AGENT_ID", raising=False)
    monkeypatch.setenv("CODEX_THREAD_ID", "thread/a")
    seed = {"sha256": "1" * 64}
    first_identity = {
        "seed_sha256": seed["sha256"],
        "replay_input_sha256": "2" * 64,
        "materialization_key": "3" * 64,
    }
    first, _, _ = cache._agent_settings(settings, first_identity)
    monkeypatch.setenv("CODEX_THREAD_ID", "thread/b")
    second, _, _ = cache._agent_settings(settings, first_identity)

    assert first.project_dir != second.project_dir
    assert "thread-a" in first.project_dir.as_posix()
    assert "thread-b" in second.project_dir.as_posix()
    assert first.project_dir.name == first_identity["materialization_key"][:20]
    assert first.ghidra_runtime_dir != second.ghidra_runtime_dir
    assert len(str(first.ghidra_runtime_dir / "query.sock")) < 108


def test_matching_materialization_marker_avoids_ghidra_startup(tmp_path: Path, monkeypatch) -> None:
    settings = _settings(tmp_path)
    monkeypatch.setenv("WIZ8_GHIDRA_AGENT_ID", "unit")
    identity = {
        "seed_sha256": "1" * 64,
        "replay_input_sha256": "2" * 64,
        "materialization_key": "3" * 64,
    }
    seed = {
        "sha256": identity["seed_sha256"],
        "binary_sha256": "4" * 64,
        "archive": tmp_path / "seed.gzf",
    }
    monkeypatch.setattr(cache, "resolve_program_name", lambda _settings, selector: selector)
    monkeypatch.setattr(cache, "_seed_record", lambda *_args: seed)
    monkeypatch.setattr(cache, "_materialization_identity", lambda *_args: identity)
    effective, _, marker = cache._agent_settings(settings, identity)
    effective.project_dir.mkdir(parents=True)
    (effective.project_dir / f"{effective.project_name}.gpr").write_text("")
    marker.write_text(
        json.dumps(
            {
                "schema": cache.MATERIALIZATION_SCHEMA,
                "program": "canonical",
                **identity,
                "binary_sha256": seed["binary_sha256"],
                "validation": {"ok": True},
                "phases": [],
            }
        ),
        encoding="utf-8",
    )
    monkeypatch.setattr(
        cache,
        "start_pyghidra",
        lambda _settings: (_ for _ in ()).throw(AssertionError("Ghidra started")),
    )

    actual, report = cache.materialize_program(settings, "canonical")

    assert actual.project_dir == effective.project_dir
    assert report["status"] == "cached"
    assert report["under_one_minute"] is True


def test_reviewed_replay_does_not_recursively_materialize(
    tmp_path: Path, monkeypatch
) -> None:
    settings = _settings(tmp_path)
    calls: list[bool] = []

    def record_apply(*_args: object, materialize: bool = True, **_kwargs: object) -> dict[str, int]:
        calls.append(materialize)
        return {}

    monkeypatch.setattr(rebuild, "apply_function_map", record_apply)
    actions = dict(rebuild.reviewed_replay_actions(settings, "canonical"))

    actions["reviewed_function_catalog"]()

    assert calls == [False]


def test_a_missing_agent_identity_is_refused_rather_than_defaulted(monkeypatch) -> None:
    # A default would put two checkouts in one per-agent project root and let
    # them collide on its lock, which is the failure this identity prevents.
    monkeypatch.delenv("WIZ8_GHIDRA_AGENT_ID", raising=False)
    monkeypatch.delenv("CODEX_THREAD_ID", raising=False)
    with pytest.raises(config.GhidraAgentIdMissing, match="WIZ8_GHIDRA_AGENT_ID"):
        config.ghidra_agent_id()

    monkeypatch.setenv("WIZ8_GHIDRA_AGENT_ID", "  ")
    with pytest.raises(config.GhidraAgentIdMissing):
        config.ghidra_agent_id()
