import json
from pathlib import Path

import pytest
from wiz8decomp.reports.comparison import comparison_report


def report(tmp_path: Path, name: str, rows: list[dict]) -> Path:
    path = tmp_path / name
    path.write_text(json.dumps({"target": "SURRENDER", "inputs": {}, "functions": rows}))
    return path


def row(address: int, outcome: str, source: str = "/repo/src/surrender/a.cpp") -> dict:
    return {
        "orig": hex(address),
        "name": "f",
        "basis": "annotation",
        "outcome": outcome,
        "source": {"path": source, "line": 1},
        "code_diff": ["- a\n", "+ b\n"] * 100,
        "data": [],
        "failures": [],
        "unidentified_references": 0,
    }


def test_report_filters_before_returning_bounded_rows_and_diffs(tmp_path):
    path = report(
        tmp_path,
        "summary.json",
        [row(1, "differences"), row(2, "no-differences"), row(3, "differences")],
    )
    result = comparison_report(
        tmp_path,
        "SURRENDER",
        report=path,
        files=[Path("src/surrender/a.cpp")],
        outcome="differences",
        limit=1,
        diff_lines=3,
    )
    assert result["matched"] == 2
    assert result["omitted"] == 1
    [entry] = result["functions"]
    assert len(entry["code_diff"]) == 3
    assert entry["diff_truncated"] is True
    result = comparison_report(tmp_path, "SURRENDER", report=path, addresses=[2])
    assert [entry["orig"] for entry in result["functions"]] == ["0x2"]


def test_report_delta_keeps_absent_selection_distinct_from_resolution(tmp_path):
    old = report(tmp_path, "old.json", [row(1, "differences"), row(2, "differences")])
    current = report(tmp_path, "current.json", [row(1, "no-differences")])
    result = comparison_report(tmp_path, "SURRENDER", report=current, against=old)
    assert result["changes"] == {"resolved": 1, "only-previous": 1}
    assert [entry["change"] for entry in result["functions"]] == ["resolved", "only-previous"]


def test_missing_or_wrong_target_report_is_an_error(tmp_path):
    with pytest.raises(FileNotFoundError):
        comparison_report(tmp_path, "SURRENDER")
    path = report(tmp_path, "summary.json", [])
    with pytest.raises(ValueError, match="target"):
        comparison_report(tmp_path, "WIZ8", report=path)
    with pytest.raises(ValueError):
        comparison_report(tmp_path, "SURRENDER", report=path, outcome="misspelled")
    with pytest.raises(ValueError, match="inside"):
        comparison_report(tmp_path, "SURRENDER", report=path, files=[Path("/outside/a.cpp")])
