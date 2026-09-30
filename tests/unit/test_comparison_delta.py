from __future__ import annotations

import json
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


def test_missing_ratio_does_not_invent_clean_similarity() -> None:
    summary = _summary(_row(1, "no-differences"), _row(2, "differences", code=True))
    metrics = comparison_metrics(summary, _ghidriff((3, 0.9)))
    assert metrics["average_similarity"] is None
    assert metrics["median_similarity"] is None
    assert metrics["clean"] == 1
    assert metrics["code_differences"] == 1


def test_duplicate_ratio_pairs_are_rejected() -> None:
    with pytest.raises(ValueError, match="duplicate function pairs"):
        comparison_metrics(
            _summary(_row(1, "differences", code=True)), _ghidriff((1, 0.8), (1, 0.9))
        )


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
    assert metrics["clean_rate"] == pytest.approx(1 / 3)
    assert metrics["code_differences"] == 1
    assert metrics["data_differences"] == 1
    assert metrics["unpaired"] == 1


def test_pr_report_contains_project_and_comparison_deltas(tmp_path: Path) -> None:
    head_status = {
        "targets": {
            "WIZ8": {
                "state": "comparison",
                "source": {"functions": 100},
                "original_functions": 200,
                "source_coverage": 0.5,
                "pairing": {"paired": 95, "unpaired": 5, "unpaired_line_refs": 2},
            }
        }
    }
    base_status = {
        "targets": {
            "WIZ8": {
                "state": "comparison",
                "source": {"functions": 90},
                "original_functions": 200,
                "source_coverage": 0.45,
                "pairing": {"paired": 86, "unpaired": 4, "unpaired_line_refs": 2},
            }
        }
    }
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
        "head_status": head_status,
        "base_status": base_status,
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
        paths["head_status"],
        paths["base_status"],
        head_summary_path=paths["head_summary"],
        base_summary_path=paths["base_summary"],
        head_ghidriff_path=head_md,
        base_ghidriff_path=base_md,
        head_datacmp_path=head_data,
        base_datacmp_path=base_data,
    )

    assert report["project"]["delta"]["source_functions"] == 10
    assert report["project"]["delta"]["source_coverage"] == pytest.approx(0.05)
    assert report["project"]["delta"]["paired"] == 9
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
