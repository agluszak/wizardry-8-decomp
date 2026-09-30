"""PR comparison statistics against the merge-base build."""

from __future__ import annotations

import json
import statistics
from collections import Counter
from pathlib import Path
from typing import Any

_ANALYZED = frozenset({"differences", "no-differences"})


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _similarities(summary: dict[str, Any], ghidriff: dict[str, Any]) -> list[float] | None:
    """Read Ghidriff facts by exact pairs; missing facts leave aggregates unknown."""
    ratios: dict[tuple[int, int], float] = {}
    for function in ghidriff["functions"]["modified"]:
        pair = (int(function["old"]["address"], 16), int(function["new"]["address"], 16))
        if pair in ratios:
            raise ValueError("Ghidriff report has duplicate function pairs")
        ratios[pair] = float(function["ratio"])

    similarities = []
    for row in summary.get("functions", ()):
        if row.get("outcome") not in _ANALYZED:
            continue
        if not row.get("code_diff"):
            similarities.append(1.0)
            continue
        pair = (int(row["orig"], 16), int(row["recomp"], 16))
        if pair not in ratios:
            return None
        similarities.append(ratios[pair])
    return similarities


def comparison_metrics(summary: dict[str, Any], ghidriff: dict[str, Any]) -> dict[str, Any]:
    functions = list(summary.get("functions", ()))
    outcomes = Counter(str(row.get("outcome") or "") for row in functions)
    analyzed = outcomes["differences"] + outcomes["no-differences"]
    similarities = _similarities(summary, ghidriff)
    clean = outcomes["no-differences"]
    return {
        "requested": int(summary.get("requested") or len(functions)),
        "analyzed": analyzed,
        "average_similarity": statistics.fmean(similarities) if similarities else None,
        "median_similarity": statistics.median(similarities) if similarities else None,
        "clean": clean,
        "clean_rate": clean / analyzed if analyzed else None,
        "differences": outcomes["differences"],
        "code_differences": sum(
            bool(row.get("code_diff")) for row in functions if row.get("outcome") in _ANALYZED
        ),
        "data_differences": sum(
            bool(row.get("data")) for row in functions if row.get("outcome") in _ANALYZED
        ),
        "unpaired": outcomes["unpaired"],
        "analysis_failed": outcomes["analysis-failed"],
        "unidentified_references": sum(
            int(row.get("unidentified_references") or 0) for row in functions
        ),
    }


def datacmp_metrics(report: dict[str, Any]) -> dict[str, Any]:
    count = int(report.get("count") or 0)
    issues = list(report.get("issues") or ())
    issue_count = int(report.get("issue_count") or len(issues))
    matched = max(0, count - issue_count)
    return {
        "count": count,
        "matched": matched,
        "issue_count": issue_count,
        "match_rate": matched / count if count else None,
        "field_differences": sum(int(issue.get("difference_count") or 0) for issue in issues),
        "raw_only_issues": sum(bool(issue.get("raw_only")) for issue in issues),
    }


def _project_metrics(status: dict[str, Any], target: str) -> dict[str, Any]:
    try:
        row = status["targets"][target]
    except KeyError as error:
        raise ValueError(f"status report has no target {target}") from error
    if row.get("state") != "comparison":
        raise ValueError(f"target {target} is not in comparison state")
    pairing = row["pairing"]
    return {
        "source_functions": int(row.get("source", {}).get("functions") or 0),
        "original_functions": row.get("original_functions"),
        "source_coverage": row.get("source_coverage"),
        "paired": int(pairing.get("paired") or 0),
        "unpaired": int(pairing.get("unpaired") or 0),
        "unpaired_line_refs": int(pairing.get("unpaired_line_refs") or 0),
    }


def _difference(head: Any, base: Any) -> Any:
    if head is None or base is None:
        return None
    return head - base


def _metric_delta(head: dict[str, Any], base: dict[str, Any]) -> dict[str, Any]:
    return {
        key: _difference(head.get(key), base.get(key))
        for key in head.keys() & base.keys()
        if isinstance(head.get(key), (int, float))
        and not isinstance(head.get(key), bool)
        and isinstance(base.get(key), (int, float))
        and not isinstance(base.get(key), bool)
    }


def _transitions(head: dict[str, Any], base: dict[str, Any]) -> dict[str, int]:
    current = {
        int(row["orig"], 16): str(row.get("outcome") or "") for row in head.get("functions", ())
    }
    previous = {
        int(row["orig"], 16): str(row.get("outcome") or "") for row in base.get("functions", ())
    }
    common = current.keys() & previous.keys()
    return {
        "resolved": sum(
            previous[address] == "differences" and current[address] == "no-differences"
            for address in common
        ),
        "newly_different": sum(
            previous[address] == "no-differences" and current[address] == "differences"
            for address in common
        ),
        "newly_paired": sum(
            previous[address] == "unpaired" and current[address] in _ANALYZED for address in common
        ),
        "newly_unpaired": sum(
            previous[address] in _ANALYZED and current[address] == "unpaired" for address in common
        ),
        "new_analysis_failures": sum(
            current[address] == "analysis-failed" and previous[address] != "analysis-failed"
            for address in common
        ),
        "head_only": len(current.keys() - previous.keys()),
        "base_only": len(previous.keys() - current.keys()),
    }


def pr_comparison_report(
    target: str,
    head_status_path: Path,
    base_status_path: Path,
    *,
    head_summary_path: Path | None = None,
    base_summary_path: Path | None = None,
    head_ghidriff_path: Path | None = None,
    base_ghidriff_path: Path | None = None,
    head_datacmp_path: Path | None = None,
    base_datacmp_path: Path | None = None,
) -> dict[str, Any]:
    head_project = _project_metrics(_read_json(head_status_path), target)
    base_project = _project_metrics(_read_json(base_status_path), target)
    report: dict[str, Any] = {
        "schema": "wiz8.pr-comparison-v1",
        "target": target,
        "project": {
            "head": head_project,
            "base": base_project,
            "delta": _metric_delta(head_project, base_project),
        },
        "comparison": None,
        "datacmp": None,
    }

    datacmp_paths = (head_datacmp_path, base_datacmp_path)
    if any(path is not None for path in datacmp_paths):
        if any(path is None for path in datacmp_paths):
            raise ValueError("datacmp delta requires both head and base reports")
        assert head_datacmp_path is not None
        assert base_datacmp_path is not None
        head_data = datacmp_metrics(_read_json(head_datacmp_path))
        base_data = datacmp_metrics(_read_json(base_datacmp_path))
        report["datacmp"] = {
            "head": head_data,
            "base": base_data,
            "delta": _metric_delta(head_data, base_data),
        }

    paths = (
        head_summary_path,
        base_summary_path,
        head_ghidriff_path,
        base_ghidriff_path,
    )
    if not any(path is not None for path in paths):
        return report
    if any(path is None for path in paths):
        raise ValueError("comparison delta requires both summaries and both Ghidriff reports")

    assert head_summary_path is not None
    assert base_summary_path is not None
    assert head_ghidriff_path is not None
    assert base_ghidriff_path is not None
    head_summary = _read_json(head_summary_path)
    base_summary = _read_json(base_summary_path)
    if head_summary.get("target") != target or base_summary.get("target") != target:
        raise ValueError("comparison summary target does not match requested target")

    head_metrics = comparison_metrics(head_summary, _read_json(head_ghidriff_path))
    base_metrics = comparison_metrics(base_summary, _read_json(base_ghidriff_path))
    report["comparison"] = {
        "head": head_metrics,
        "base": base_metrics,
        "delta": _metric_delta(head_metrics, base_metrics),
        "transitions": _transitions(head_summary, base_summary),
    }
    return report
