from __future__ import annotations

import json
import runpy
from pathlib import Path

import pytest
from wiz8decomp.reports.comparison_delta import (
    comparison_metrics,
    datacmp_metrics,
    pr_comparison_report,
)


def _row(address: int, outcome: str, *, code: bool = False, data: bool = False) -> dict:
    return {
        "orig": hex(address),
        "recomp": hex(0x10000 + address),
        "outcome": outcome,
        "code_diff": ["-old", "+new"] if code else [],
        "data": [{"kind": "object-contents"}] if data else [],
        "unidentified_references": 1,
    }


def _summary(*rows: dict) -> dict:
    return {
        "target": "WIZ8",
        "requested": len(rows),
        "functions": list(rows),
    }


def _ghidriff(*pairs: tuple[int, float]) -> dict:
    return {
        "functions": {
            "modified": [
                {
                    "old": {"address": hex(address)},
                    "new": {"address": hex(0x10000 + address)},
                    "ratio": ratio,
                }
                for address, ratio in pairs
            ]
        }
    }


def test_missing_ratio_does_not_discard_scored_similarity() -> None:
    summary = _summary(_row(1, "no-differences"), _row(2, "differences", code=True))
    metrics = comparison_metrics(summary, _ghidriff((3, 0.9)))
    assert metrics["average_similarity"] == 1.0
    assert metrics["median_similarity"] == 1.0
    assert metrics["similarity_scored"] == 1
    assert metrics["similarity_unscored"] == 1
    assert metrics["similarity_coverage"] == 0.5
    assert metrics["clean"] == 1
    assert metrics["code_differences"] == 1


def test_duplicate_ratio_pairs_are_rejected() -> None:
    with pytest.raises(ValueError, match="duplicate function pairs"):
        comparison_metrics(
            _summary(_row(1, "differences", code=True)), _ghidriff((1, 0.8), (1, 0.9))
        )


@pytest.mark.parametrize("summary", [_summary(), _summary(_row(1, "differences", code=True))])
def test_similarity_without_scored_functions_is_unknown(summary: dict) -> None:
    metrics = comparison_metrics(summary, _ghidriff())
    assert metrics["average_similarity"] is None
    assert metrics["median_similarity"] is None
    assert metrics["similarity_scored"] == 0
    assert metrics["similarity_unscored"] == metrics["analyzed"]
    assert metrics["similarity_coverage"] == (0.0 if metrics["analyzed"] else None)


def test_comment_discloses_partial_similarity_coverage(monkeypatch, capsys) -> None:
    summary = _summary(_row(1, "no-differences"), _row(2, "differences", code=True))
    report = {
        "project": {"head": {"source_functions": 2, "paired": 2}, "delta": {}},
        "comparison": {
            "head": comparison_metrics(summary, _ghidriff()),
            "delta": {},
            "transitions": {"resolved": 0, "newly_different": 0},
        },
    }
    monkeypatch.setenv("WIZ8_STATUS", json.dumps(report))
    monkeypatch.delenv("SURRENDER_STATUS", raising=False)
    runpy.run_path(
        str(Path(__file__).resolve().parents[2] / ".github/scripts/render-reccmp-comment.py")
    )
    rendered = capsys.readouterr().out
    assert "Similarity scores" in rendered
    assert "| 1/2 | 100.00% |" in rendered
    assert "missing ratios are excluded" in rendered


def test_comparison_metrics_use_ghidriff_ratio_and_exact_matches() -> None:
    summary = _summary(
        _row(1, "no-differences"),
        _row(2, "differences", code=True),
        _row(3, "differences", data=True),
        _row(4, "unpaired"),
    )
    metrics = comparison_metrics(summary, _ghidriff((2, 0.8)))

    assert metrics["analyzed"] == 3
    assert metrics["average_similarity"] == pytest.approx((1.0 + 0.8 + 1.0) / 3)
    assert metrics["median_similarity"] == 1.0
    assert metrics["similarity_scored"] == 3
    assert metrics["similarity_unscored"] == 0
    assert metrics["similarity_coverage"] == 1.0
    assert metrics["clean_rate"] == pytest.approx(1 / 3)
    assert metrics["code_differences"] == 1
    assert metrics["data_differences"] == 1
    assert metrics["unpaired"] == 1


def test_comparison_metrics_count_inline_retries_by_retry_outcome() -> None:
    clean = {**_row(1, "no-differences"), "inline_callees": ["0x9"], "inline_normalized_diff": []}
    different = {
        **_row(2, "differences", code=True),
        "inline_callees": ["0x9"],
        "inline_normalized_diff": ["-old", "+new"],
    }
    failed = {
        **_row(3, "analysis-failed"),
        "inline_callees": ["0x9"],
        "inline_normalized_diff": None,
    }
    ordinary = {**_row(4, "differences", code=True), "inline_callees": []}
    metrics = comparison_metrics(
        _summary(clean, different, failed, ordinary), _ghidriff((2, 0.9), (4, 0.8))
    )

    assert metrics["inline_retries"] == 3
    assert metrics["inline_normalized_clean"] == 1
    assert metrics["inline_still_different"] == 1
    assert metrics["inline_retry_failures"] == 1


def test_pr_report_contains_comparison_and_data_deltas(tmp_path: Path) -> None:
    head_summary = _summary(
        _row(1, "no-differences"),
        _row(2, "differences", code=True),
        _row(3, "differences", data=True),
        _row(4, "unpaired"),
    )
    base_summary = _summary(
        _row(1, "differences", code=True),
        _row(2, "differences", code=True),
        _row(3, "no-differences"),
        _row(4, "unpaired"),
    )

    files = {
        "head_summary": head_summary,
        "base_summary": base_summary,
    }
    paths = {}
    for name, value in files.items():
        path = tmp_path / f"{name}.json"
        path.write_text(json.dumps(value), encoding="utf-8")
        paths[name] = path
    head_data = tmp_path / "head-datacmp.json"
    base_data = tmp_path / "base-datacmp.json"
    head_data.write_text(
        json.dumps(
            {
                "count": 20,
                "issue_count": 1,
                "issues": [{"difference_count": 2, "raw_only": False}],
            }
        ),
        encoding="utf-8",
    )
    base_data.write_text(
        json.dumps(
            {
                "count": 18,
                "issue_count": 2,
                "issues": [
                    {"difference_count": 3, "raw_only": False},
                    {"difference_count": 1, "raw_only": True},
                ],
            }
        ),
        encoding="utf-8",
    )
    head_md = tmp_path / "head.json"
    base_md = tmp_path / "base.json"
    head_md.write_text(json.dumps(_ghidriff((2, 0.9))), encoding="utf-8")
    base_md.write_text(json.dumps(_ghidriff((1, 0.8), (2, 0.6))), encoding="utf-8")

    report = pr_comparison_report(
        "WIZ8",
        head_summary_path=paths["head_summary"],
        base_summary_path=paths["base_summary"],
        head_ghidriff_path=head_md,
        base_ghidriff_path=base_md,
        head_datacmp_path=head_data,
        base_datacmp_path=base_data,
    )

    comparison = report["comparison"]
    assert comparison is not None
    assert comparison["head"]["average_similarity"] == pytest.approx((1 + 0.9 + 1) / 3)
    assert comparison["base"]["average_similarity"] == pytest.approx((0.8 + 0.6 + 1) / 3)
    assert comparison["delta"]["code_differences"] == -1
    assert comparison["delta"]["data_differences"] == 1
    assert comparison["transitions"]["resolved"] == 1
    assert comparison["transitions"]["newly_different"] == 1
    datacmp = report["datacmp"]
    assert datacmp is not None
    assert datacmp["head"]["match_rate"] == pytest.approx(19 / 20)
    assert datacmp["delta"]["issue_count"] == -1
    assert datacmp["delta"]["field_differences"] == -2


def test_datacmp_metrics_report_matches_and_differences() -> None:
    metrics = datacmp_metrics(
        {
            "count": 10,
            "issue_count": 2,
            "issues": [
                {"difference_count": 3, "raw_only": False},
                {"difference_count": 1, "raw_only": True},
            ],
        }
    )

    assert metrics == {
        "count": 10,
        "matched": 8,
        "issue_count": 2,
        "match_rate": 0.8,
        "field_differences": 4,
        "raw_only_issues": 1,
    }
