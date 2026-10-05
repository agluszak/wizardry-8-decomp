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


def _row(
    address: int,
    outcome: str,
    *,
    code: bool = False,
    data: bool = False,
    score: float | None = None,
) -> dict:
    return {
        "orig": hex(address),
        "recomp": hex(0x10000 + address),
        "outcome": outcome,
        "selected_pass": "ordinary",
        "passes": {
            "ordinary": {
                "outcome": outcome,
                "body_diff": ["-old", "+new"] if code else [],
                "signature_diff": [],
                "change_kind": None,
                "similarity": score
                if code
                else (1.0 if outcome in {"no-differences", "differences"} else None),
                "data": [{"kind": "object-contents"}] if data else [],
                "failures": [],
                "warnings": [],
                "unidentified_references": 1,
            }
        },
    }


def _summary(*rows: dict) -> dict:
    return {
        "target": "WIZ8",
        "inputs": {
            "orig": {"sha256": "fixture-original"},
            "normalization_key": "fixture-policy",
            "ghidra_version": "12.1.4",
            "decompiler_sha256": "native-fixture",
            "decompiler_timeout": 60,
            "threaded": True,
            "max_ram_percent": 60,
            "wizardry_revision": "head",
        },
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
    metrics = comparison_metrics(summary)
    assert metrics["average_similarity"] == 1.0
    assert metrics["median_similarity"] == 1.0
    assert metrics["similarity_scored"] == 1
    assert metrics["similarity_unscored"] == 1
    assert metrics["similarity_coverage"] == 0.5
    assert metrics["clean"] == 1
    assert metrics["code_differences"] == 1


def test_selected_inline_score_does_not_use_ordinary_score() -> None:
    row = _row(1, "differences", code=True, score=0.1)
    row["selected_pass"] = "inline"
    row["inline_callees"] = ["0x9"]
    row["passes"]["inline"] = {**row["passes"]["ordinary"], "similarity": 0.9}
    metrics = comparison_metrics(_summary(row))
    assert metrics["average_similarity"] == 0.9


@pytest.mark.parametrize("summary", [_summary(), _summary(_row(1, "differences", code=True))])
def test_similarity_without_scored_functions_is_unknown(summary: dict) -> None:
    metrics = comparison_metrics(summary)
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
            "head": comparison_metrics(summary),
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
    assert "Requested | Analyzed | Non-emitted" in rendered
    assert "Unpaired | Analysis failed | Missing" in rendered
    assert "Similarity scores" in rendered
    assert "| 1/2 | 100.00% |" in rendered
    assert "missing pass scores are excluded" in rendered


def test_comparison_metrics_use_selected_pass_scores() -> None:
    summary = _summary(
        _row(1, "no-differences"),
        _row(2, "differences", code=True, score=0.8),
        _row(3, "differences", data=True),
        _row(4, "unpaired"),
    )
    metrics = comparison_metrics(summary)

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
    clean = _row(1, "no-differences")
    different = _row(2, "differences", code=True, score=0.9)
    failed = _row(3, "analysis-failed")
    for row in (clean, different, failed):
        row["selected_pass"] = "inline"
        row["inline_callees"] = ["0x9"]
        row["passes"]["inline"] = row["passes"]["ordinary"]
        row["passes"]["ordinary"] = _row(99, "differences", code=True, score=0.2)["passes"][
            "ordinary"
        ]
    ordinary = _row(4, "differences", code=True, score=0.8)
    metrics = comparison_metrics(_summary(clean, different, failed, ordinary))

    assert metrics["inline_retries"] == 3
    assert metrics["inline_normalized_clean"] == 1
    assert metrics["inline_still_different"] == 1
    assert metrics["inline_retry_failures"] == 1


def test_pr_report_contains_comparison_and_data_deltas(tmp_path: Path) -> None:
    head_summary = _summary(
        _row(1, "no-differences"),
        _row(2, "differences", code=True, score=0.9),
        _row(3, "differences", data=True),
        _row(4, "unpaired"),
    )
    base_summary = _summary(
        _row(1, "differences", code=True, score=0.8),
        _row(2, "differences", code=True, score=0.6),
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


@pytest.mark.parametrize("paired", [True, False])
def test_internal_non_emission_cannot_hide_disappearing_procedure(tmp_path: Path, paired: bool):
    base = _summary(
        {
            **_row(1, "differences" if paired else "internal-non-emission"),
            "recomp": "0x10001" if paired else None,
        }
    )
    head = _summary({**_row(1, "internal-non-emission"), "recomp": None})
    paths = {}
    for name, value in {
        "head_summary": head,
        "base_summary": base,
        "head_ghidriff": _ghidriff(),
        "base_ghidriff": _ghidriff(),
    }.items():
        paths[name] = tmp_path / f"{name}.json"
        paths[name].write_text(json.dumps(value))
    report = pr_comparison_report("WIZ8", **{f"{key}_path": value for key, value in paths.items()})
    assert report["ok"] is not paired
    assert len(report["emission_regressions"]) == int(paired)
    assert report["comparison"]["head"]["non_emitted"] == 1
    assert report["comparison"]["delta"]["non_emitted"] == int(paired)


@pytest.mark.parametrize(
    "head,added,removed",
    [
        (["existing-a", "existing-b"], [], []),
        (["existing-a", "existing-b", "new"], ["new"], []),
        (["existing-a", "replacement"], ["replacement"], ["existing-b"]),
        (["existing-a"], [], ["existing-b"]),
    ],
)
def test_export_debt_is_compared_by_identity(tmp_path: Path, head, added, removed):
    paths = []
    for name, symbols in [("head", head), ("base", ["existing-a", "existing-b"])]:
        path = tmp_path / f"{name}.json"
        path.write_text(json.dumps({"compiler_exports_absent_from_retail": symbols}))
        paths.append(path)
    report = pr_comparison_report(
        "SURRENDER", head_exports_path=paths[0], base_exports_path=paths[1]
    )
    assert report["exports"]["added"] == added
    assert report["exports"]["removed"] == removed
    assert report["ok"] is (not added)


def test_surender_export_baseline_is_required(tmp_path: Path):
    with pytest.raises(ValueError, match="both head and merge-base export reports"):
        pr_comparison_report("SURRENDER", head_summary_path=tmp_path / "head.json")


def test_classified_comparison_coverage_includes_all_debt():
    summary = _summary(
        *[
            _row(i, outcome)
            for i, outcome in enumerate(
                [
                    "no-differences",
                    "internal-non-emission",
                    "template-non-emission",
                    "header-emission",
                    "unpaired",
                    "analysis-failed",
                    "missing",
                ]
            )
        ]
    )
    metrics = comparison_metrics(summary)
    assert [
        metrics[key]
        for key in [
            "requested",
            "analyzed",
            "non_emitted",
            "unpaired",
            "analysis_failed",
            "missing",
        ]
    ] == [7, 1, 3, 1, 1, 1]


@pytest.mark.parametrize("added", [False, True])
def test_pr_report_cli_fails_on_new_export_debt(tmp_path: Path, added: bool):
    from typer.testing import CliRunner
    from wiz8decomp.commands.reports import app

    paths = {}
    for side, symbols in [
        ("head", ["existing", "new"] if added else ["existing"]),
        ("base", ["existing"]),
    ]:
        paths[side] = tmp_path / f"{side}.json"
        paths[side].write_text(json.dumps({"compiler_exports_absent_from_retail": symbols}))
    result = CliRunner().invoke(
        app,
        [
            "pr-comparison",
            "--target",
            "SURRENDER",
            "--head-exports",
            str(paths["head"]),
            "--base-exports",
            str(paths["base"]),
        ],
    )
    assert result.exit_code == int(added)
    report = json.loads(result.stdout)
    assert report["ok"] is (not added)
    assert report["exports"]["added"] == (["new"] if added else [])


def test_declaration_findings_do_not_reduce_body_quality() -> None:
    declaration = _row(1, "no-differences")
    declaration["passes"]["ordinary"]["signature_diff"] = ["-uint", "+int"]
    signedness = _row(2, "differences", code=True, score=0.9)
    signedness["passes"]["ordinary"]["change_kind"] = "scalar-signedness"
    metrics = comparison_metrics(_summary(declaration, signedness))
    assert metrics["clean"] == 1
    assert metrics["code_differences"] == 1
    assert metrics["signature_differences"] == 1
    assert metrics["scalar_signedness_differences"] == 1
    assert metrics["average_similarity"] == 0.95


@pytest.mark.parametrize("key", ["normalization_key", "decompiler_sha256"])
def test_pr_delta_rejects_different_comparison_policies(tmp_path: Path, key: str):
    head = _summary(_row(1, "no-differences"))
    base = _summary(_row(1, "no-differences"))
    base["inputs"][key] = "older-policy"
    paths = {}
    for name, value in {
        "head_summary": head,
        "base_summary": base,
        "head_ghidriff": _ghidriff(),
        "base_ghidriff": _ghidriff(),
    }.items():
        path = tmp_path / f"{name}.json"
        path.write_text(json.dumps(value))
        paths[f"{name}_path"] = path
    with pytest.raises(ValueError, match=f"Comparison policies differ: {key}"):
        pr_comparison_report("WIZ8", **paths)
