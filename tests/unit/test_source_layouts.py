import pytest
from wiz8decomp.source_layouts import (
    layout_failure_key,
    load_source_layout_baseline,
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
