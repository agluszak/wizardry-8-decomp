import pytest
from wiz8decomp.source_layouts import (
    compare_source_layout_reports,
    layout_failure_key,
    load_source_layout_baseline,
    require_source_layout_delta,
    require_source_layouts,
    write_source_layout_baseline,
)


def test_source_layout_gate_rejects_reported_drift() -> None:
    with pytest.raises(ValueError, match="differs at 1 checks"):
        require_source_layouts(
            {
                "ok": False,
                "failure_count": 1,
                "report": "build/reports/source-layouts/report.json",
            }
        )


def test_layout_failure_key_normalizes_legacy_pointer_fields() -> None:
    assert layout_failure_key(
        {
            "kind": "field",
            "class": "Node",
            "field": "parent",
            "expected_pointer_depth": 1,
            "actual_types": ["Node", "Node *"],
        }
    ) == ("field", "Node", "parent", "1", '["Node","Node *"]')


def test_source_layout_delta_lists_progress_and_separates_review_state() -> None:
    baseline = {
        "failures": [
            {"kind": "size", "class": "Old", "expected": 4, "actual": 8},
            {"kind": "missing-ghidra-class", "class": "KnownGap"},
        ]
    }
    current = {
        "failures": [
            {"kind": "missing-ghidra-class", "class": "KnownGap"},
            {"kind": "base", "class": "New", "expected": "Base", "actual": []},
            {"kind": "missing-ghidra-class", "class": "NewReviewGap"},
        ]
    }

    delta = compare_source_layout_reports(
        current,
        baseline,
        baseline_name="config/verification/source-layout-baseline.csv",
    )

    assert delta["ok"] is False
    assert delta["introduced_count"] == 2
    assert [item["class"] for item in delta["new_contradictions"]] == ["New"]
    assert [item["class"] for item in delta["new_review_state"]] == ["NewReviewGap"]
    assert [item["class"] for item in delta["fixed"]] == ["Old"]


def test_source_layout_delta_detects_failure_spelling_churn() -> None:
    delta = compare_source_layout_reports(
        {"failures": [{"kind": "size", "class": "Renamed", "expected": 4, "actual": 8}]},
        {"failures": [{"kind": "size", "class": "Original", "expected": 4, "actual": 8}]},
        baseline_name="config/verification/source-layout-baseline.csv",
    )

    assert delta["ok"] is False
    assert delta["spelling_changes"] == [
        {
            "baseline": {
                "kind": "size",
                "class": "Original",
                "field": "",
                "expected": "4",
                "actual": "8",
            },
            "current": {
                "kind": "size",
                "class": "Renamed",
                "field": "",
                "expected": "4",
                "actual": "8",
            },
        }
    ]


def test_source_layout_delta_gate_rejects_only_new_failures() -> None:
    with pytest.raises(ValueError, match="introduced 1 failures"):
        require_source_layout_delta(
            {
                "ok": False,
                "introduced_count": 1,
                "baseline": "config/verification/source-layout-baseline.csv",
                "report": "build/reports/source-layouts/delta.json",
            }
        )

    report = {"ok": True, "introduced_count": 0}
    assert require_source_layout_delta(report) is report


def test_source_layout_baseline_can_only_ratchet_down(tmp_path) -> None:
    path = tmp_path / "source-layout-baseline.csv"
    size = {"kind": "size", "class": "Node", "expected": 4, "actual": 8}
    missing = {"kind": "missing-ghidra-class", "class": "Other"}

    initialized = write_source_layout_baseline(path, {"failures": [size, missing]})
    assert initialized["failure_count"] == 2
    assert load_source_layout_baseline(path)["failure_count"] == 2

    ratcheted = write_source_layout_baseline(path, {"failures": [size]})
    assert ratcheted["failure_count"] == 1

    with pytest.raises(ValueError, match="refusing to add 1 failures"):
        write_source_layout_baseline(path, {"failures": [size, missing]})
