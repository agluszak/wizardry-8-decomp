"""Unit tests for enrichment checkpoint outcomes and promote selection."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.enrichment_checkpoint import compute_outcomes
from wiz8decomp.enrichment_promote import (
    _live_replacement_allowed,
    _resolve_frozen_candidate,
    _verify_provenance,
    find_latest_promotable_run,
    resolve_run_dir,
)


def test_compute_outcomes_splits_safe_and_preserved() -> None:
    result = {
        "steps": [
            {"step": "class-this-typing", "result": {"apply": True, "apply_errors": 0}},
        ],
        "quality_delta": {"ok": True, "debt_total_delta": -3},
        "prototype_repair": {"applied": 2},
    }
    outcomes, safe, preserved = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes == {
        "safe_application": True,
        "preserved_recovery": True,
        "useful_improvement": True,
    }
    assert safe == []
    assert preserved == []
    assert outcomes["safe_application"] and outcomes["preserved_recovery"]


def test_useful_improvement_ignores_applied_count_alone() -> None:
    result = {
        "steps": [
            {"step": "global-typing", "result": {"apply": True, "apply_errors": 0, "applied": 12}},
        ],
        "quality_delta": {"ok": True, "debt_total_delta": 0},
        "global_typing": {"applied": 12},
    }
    outcomes, _, _ = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes["safe_application"] is True
    assert outcomes["preserved_recovery"] is True
    assert outcomes["useful_improvement"] is False


def test_useful_improvement_none_when_unmeasured_even_with_applies() -> None:
    result = {
        "steps": [],
        "prototype_repair": {"applied": 5},
    }
    outcomes, _, preserved = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes["useful_improvement"] is None
    assert outcomes["preserved_recovery"] is None
    assert "preservation unmeasured" in preserved


def test_mutated_unmeasured_candidate_is_not_promotable(tmp_path: Path) -> None:
    result = {
        "steps": [
            {"step": "class-this-typing", "result": {"apply": True, "apply_errors": 0}},
        ],
        "class_this_typing": {"applied": 3},
    }
    outcomes, _, preserved = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes["safe_application"] is True
    assert outcomes["preserved_recovery"] is None
    assert outcomes["useful_improvement"] is None
    assert preserved == ["preservation unmeasured"]
    assert outcomes["safe_application"] is True and outcomes["preserved_recovery"] is not True

    root = tmp_path / "enrichment-checkpoint" / "run-unmeasured"
    root.mkdir(parents=True)
    report = {
        "ok": False,
        "disposable": True,
        "outcomes": outcomes,
        "candidate": {"sha256": "abc"},
    }
    (root / "report.json").write_text(json.dumps(report), encoding="utf-8")
    (root / "candidate.gzf").write_bytes(b"gzf")
    (root / "candidate.sha256").write_text("deadbeef  candidate.gzf\n", encoding="utf-8")
    with pytest.raises(FileNotFoundError, match="no promotable"):
        find_latest_promotable_run(tmp_path)


def test_compute_outcomes_apply_errors_fail_safe_only() -> None:
    result = {
        "steps": [
            {"step": "global-typing", "result": {"apply": True, "apply_errors": 2}},
        ],
        "quality_delta": {"ok": True, "debt_total_delta": 0},
    }
    outcomes, safe, preserved = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes["safe_application"] is False
    assert outcomes["preserved_recovery"] is True
    assert outcomes["useful_improvement"] is False
    assert safe
    assert preserved == []


def test_compute_outcomes_quality_regression_fails_preserved() -> None:
    result = {
        "steps": [],
        "quality_delta": {"ok": False, "debt_total_delta": 5},
        "prototype_repair": {"applied": 1},
    }
    outcomes, _safe, preserved = compute_outcomes(result, mutated=True, inputs_matched=True)
    assert outcomes["safe_application"] is True
    assert outcomes["preserved_recovery"] is False
    assert outcomes["useful_improvement"] is False
    assert preserved


def test_compute_outcomes_unmeasured_useful_is_none() -> None:
    outcomes, _, _ = compute_outcomes({"steps": []}, mutated=False, inputs_matched=True)
    assert outcomes["useful_improvement"] is None
    assert outcomes["safe_application"] is True
    assert outcomes["preserved_recovery"] is True


def test_find_latest_promotable_run(tmp_path: Path) -> None:
    root = tmp_path / "enrichment-checkpoint"
    bad = root / "run-aaaa"
    good_old = root / "run-bbbb"
    good_new = root / "run-cccc"

    def payload(*, ok: bool, disposable: bool = True) -> dict:
        return {
            "ok": ok,
            "disposable": disposable,
            "ghidra_project": str(tmp_path / "proj"),
            "outcomes": {
                "safe_application": ok,
                "preserved_recovery": True,
            },
            "candidate": {"sha256": "abc"},
        }

    for path, report in (
        (bad, payload(ok=False)),
        (good_old, payload(ok=True)),
        (good_new, payload(ok=True)),
    ):
        path.mkdir(parents=True)
        (path / "report.json").write_text(json.dumps(report), encoding="utf-8")
        if report["ok"]:
            (path / "candidate.gzf").write_bytes(b"gzf")
            (path / "candidate.sha256").write_text("deadbeef  candidate.gzf\n", encoding="utf-8")

    (good_new / "report.json").write_text(json.dumps(payload(ok=True)), encoding="utf-8")
    assert find_latest_promotable_run(tmp_path) == good_new


def test_resolve_run_dir_requires_choice(tmp_path: Path) -> None:
    settings = type("S", (), {"build_dir": tmp_path, "work_dir": tmp_path})()
    with pytest.raises(ValueError, match="run directory or --from-latest"):
        resolve_run_dir(settings)  # type: ignore[arg-type]
    with pytest.raises(ValueError, match="not both"):
        resolve_run_dir(settings, run_dir=tmp_path, from_latest=True)  # type: ignore[arg-type]


def test_promote_refuses_without_candidate_gzf(tmp_path: Path) -> None:
    run = tmp_path / "run-x"
    run.mkdir()
    (run / "report.json").write_text(
        json.dumps(
            {
                "disposable": True,
                "ok": True,
                "outcomes": {"safe_application": True, "preserved_recovery": True},
            }
        ),
        encoding="utf-8",
    )
    with pytest.raises(FileNotFoundError, match="candidate.gzf"):
        _resolve_frozen_candidate(run, json.loads((run / "report.json").read_text()))


def test_promote_refuses_sha_mismatch(tmp_path: Path) -> None:
    run = tmp_path / "run-y"
    run.mkdir()
    (run / "candidate.gzf").write_bytes(b"content")
    (run / "candidate.sha256").write_text("0" * 64 + "  candidate.gzf\n", encoding="utf-8")
    report = {
        "disposable": True,
        "candidate": {"sha256": "0" * 64},
    }
    with pytest.raises(RuntimeError, match="sha256 mismatch"):
        _resolve_frozen_candidate(run, report)


def _promote_settings(tmp_path: Path):
    from wiz8decomp.config import Settings

    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": str(tmp_path / "ghidra"),
            "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
            "WIZ8_WORK_DIR": str(tmp_path / "work"),
            "WIZ8_GHIDRA_PROJECT_DIR": str(tmp_path / "project"),
        }
    )


def test_live_replacement_requires_force_when_project_dir_exists(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _promote_settings(tmp_path)
    settings.project_dir.mkdir(parents=True)
    monkeypatch.setattr(
        "wiz8decomp.ghidra.workspace.project_seed_freshness",
        lambda *_a, **_k: {"status": "not-restored", "detail": "no gpr"},
    )
    with pytest.raises(RuntimeError, match="--force"):
        _live_replacement_allowed(settings, {"sha256": "seed", "program": "wiz8"}, force=False)


def test_verify_provenance_refuses_missing_current_pdb(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _promote_settings(tmp_path)
    monkeypatch.setattr(
        "wiz8decomp.enrichment_checkpoint._collect_input_manifest",
        lambda *_a, **_k: {
            "seed_sha256": "aa",
            "binary_sha256": "bb",
            "ghidra_version": "12.1.3",
            "pyghidra_version": "3.1.0",
            "reccmp_git_rev": "abc",
            "source_tree": {"git_commit_id": "c1"},
        },
    )
    report = {
        "expected_seed_sha256": "aa",
        "seed_program": "wiz8",
        "import_source": True,
        "input_manifest": {
            "seed_sha256": "aa",
            "binary_sha256": "bb",
            "ghidra_version": "12.1.3",
            "pyghidra_version": "3.1.0",
            "reccmp_git_rev": "abc",
            "pdb_sha256": "deadbeef",
            "source_tree": {"git_commit_id": "c1"},
        },
    }
    current_seed = {"sha256": "aa", "program": "wiz8", "binary_sha256": "bb"}
    with pytest.raises(RuntimeError, match="pdb_sha256 missing on current"):
        _verify_provenance(settings, report, current_seed, force=False)


def test_reccmp_provenance_records_uv_git_pin() -> None:
    from wiz8decomp.config import repository_root
    from wiz8decomp.enrichment_checkpoint import _reccmp_provenance

    info = _reccmp_provenance(repository_root())
    assert "reccmp_pin_error" not in info, info
    assert info.get("reccmp_git_rev")
    assert len(str(info["reccmp_git_rev"])) >= 7
