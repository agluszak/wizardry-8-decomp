"""Unit tests for enrichment checkpoint outcomes and promote selection."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.enrichment_checkpoint import compute_outcomes
from wiz8decomp.enrichment_promote import find_latest_promotable_run, resolve_run_dir


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
        }

    for path, report in (
        (bad, payload(ok=False)),
        (good_old, payload(ok=True)),
        (good_new, payload(ok=True)),
    ):
        path.mkdir(parents=True)
        (path / "report.json").write_text(json.dumps(report), encoding="utf-8")

    # Ensure mtime order: touch good_new last.
    (good_new / "report.json").write_text(json.dumps(payload(ok=True)), encoding="utf-8")
    assert find_latest_promotable_run(tmp_path) == good_new


def test_resolve_run_dir_requires_choice(tmp_path: Path) -> None:
    settings = type("S", (), {"work_dir": tmp_path})()
    with pytest.raises(ValueError, match="run directory or --from-latest"):
        resolve_run_dir(settings)  # type: ignore[arg-type]
    with pytest.raises(ValueError, match="not both"):
        resolve_run_dir(settings, run_dir=tmp_path, from_latest=True)  # type: ignore[arg-type]
