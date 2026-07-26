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


def test_every_mutating_apply_command_opens_its_own_project() -> None:
    """A mutation that skips materialization reaches the shared project.

    It then contends for a lock another agent may hold, or takes one that
    blocks them. Checked structurally rather than by running Ghidra, so a new
    apply command that forgets it fails here instead of in the field.
    """

    ghidra = Path(__file__).resolve().parents[2] / "tools" / "wiz8decomp" / "ghidra"
    commands = sorted(ghidra.glob("apply_*.py"))

    assert len(commands) >= 7
    for module in commands:
        source = module.read_text(encoding="utf-8")
        if "def apply_" not in source:
            continue
        assert "open_for_mutation" in source, (
            f"{module.name} mutates Ghidra without materializing a per-agent project"
        )
        assert "start_pyghidra(settings)" not in source, (
            f"{module.name} starts Ghidra on the incoming settings rather than the "
            "materialized ones"
        )
        assert "materialize: bool = True" in source, (
            f"{module.name} cannot be told to skip materialization, so the reviewed "
            "replay - which is made of these commands - would recurse into itself"
        )


def test_the_replay_never_materializes_recursively() -> None:
    # reviewed_replay_actions runs inside materialize_program while it holds the
    # lock, so every command it invokes must skip materializing.
    rebuild = (
        Path(__file__).resolve().parents[2]
        / "tools"
        / "wiz8decomp"
        / "ghidra"
        / "rebuild.py"
    ).read_text(encoding="utf-8")
    calls = [line for line in rebuild.splitlines() if "apply_" in line and "(" in line]
    applied = [line for line in calls if "lambda" in line or "settings," in line]

    assert applied
    assert rebuild.count("materialize=False") >= 6


def _materialization(root: Path, name: str, age: float) -> Path:
    path = root / "projects" / name
    path.mkdir(parents=True)
    (path / "wizardry8.gpr").write_text("", encoding="utf-8")
    import os

    os.utime(path, (age, age))
    return path


def test_pruning_keeps_the_newest_and_drops_the_rest(tmp_path: Path) -> None:
    root = tmp_path / "agent"
    oldest = _materialization(root, "aaaa", 1000)
    middle = _materialization(root, "bbbb", 2000)
    newest = _materialization(root, "cccc", 3000)

    evicted = cache.prune_materializations(root, keep=2)

    assert evicted == ["aaaa"]
    assert not oldest.exists()
    assert middle.exists()
    assert newest.exists()


def test_the_current_project_survives_even_when_it_is_the_oldest(tmp_path: Path) -> None:
    # It is about to be used. Evicting it would delete the very project the
    # caller materialized.
    root = tmp_path / "agent"
    current = _materialization(root, "aaaa", 1000)
    _materialization(root, "bbbb", 2000)
    _materialization(root, "cccc", 3000)

    evicted = cache.prune_materializations(root, keep=1, current=current)

    assert current.exists()
    assert sorted(evicted) == ["bbbb"]


def test_pruning_an_agent_with_nothing_materialized_is_not_an_error(tmp_path: Path) -> None:
    assert cache.prune_materializations(tmp_path / "absent", keep=3) == []


def test_the_keep_count_is_configurable_and_never_zero(monkeypatch) -> None:
    monkeypatch.setenv("WIZ8_GHIDRA_KEEP_MATERIALIZATIONS", "5")
    assert cache.materialization_keep_count() == 5

    # Keeping none would delete the project the caller is about to open.
    monkeypatch.setenv("WIZ8_GHIDRA_KEEP_MATERIALIZATIONS", "0")
    assert cache.materialization_keep_count() == 1

    monkeypatch.setenv("WIZ8_GHIDRA_KEEP_MATERIALIZATIONS", "not a number")
    assert cache.materialization_keep_count() == 3


def test_replay_plumbing_does_not_invalidate_the_materialization(tmp_path: Path) -> None:
    """Editing a transport or a cache must not force every agent to re-import.

    The key exists to notice changes that alter what the replay writes into a
    program. Hashing the daemon or this very module into it means routine
    tooling work rebuilds a 51MB project for nothing.
    """

    settings = _settings(tmp_path)
    ghidra = settings.repo_dir / "tools" / "wiz8decomp" / "ghidra"
    ghidra.mkdir(parents=True)
    (ghidra / "rebuild.py").write_text("replay order", encoding="utf-8")
    (ghidra / "apply_zlib_model.py").write_text("writes types", encoding="utf-8")
    (ghidra / "query_daemon.py").write_text("transport", encoding="utf-8")
    (ghidra / "cache.py").write_text("this module", encoding="utf-8")

    names = {path.name for path in cache._replay_input_paths(settings)}

    assert "rebuild.py" in names
    assert "apply_zlib_model.py" in names
    assert "query_daemon.py" not in names
    assert "cache.py" not in names


def test_an_unclassified_module_still_feeds_the_key(tmp_path: Path) -> None:
    # The deny-list errs toward a needless rebuild rather than a stale program:
    # a module nobody has classified is assumed to matter.
    settings = _settings(tmp_path)
    ghidra = settings.repo_dir / "tools" / "wiz8decomp" / "ghidra"
    ghidra.mkdir(parents=True)
    (ghidra / "apply_something_new.py").write_text("unknown", encoding="utf-8")

    assert "apply_something_new.py" in {p.name for p in cache._replay_input_paths(settings)}
