import pytest
from wiz8decomp.source_layouts import compare_source_layouts, require_source_layouts


def test_source_layout_audit_checks_exact_size_fields_and_bases() -> None:
    source = [
        {
            "qualified_name": "Widget",
            "asserted_size": 8,
            "bases": ["Base"],
            "fields": [{"name": "m_instance", "type": "Node *"}],
        }
    ]
    ghidra = {
        "/Widget": {
            "length": 8,
            "components": [
                {"field": "base", "offset": 0, "length": 4, "type": "Base"},
                {"field": "m_instance", "offset": 4, "length": 4, "type": "Node *"},
            ],
        },
        "/wiz8/classes/Widget": {
            "components": [{"field": "m_instance", "offset": 4, "length": 4, "type": "Node *"}]
        },
    }

    report = compare_source_layouts(source, ghidra)

    assert report["ok"] is True
    assert report["checks"] == {"classes": 1, "source_fields": 1, "fields": 1, "bases": 1}


def test_source_layout_audit_rejects_missing_pdb_class_and_size_drift() -> None:
    source = [
        {"qualified_name": "Missing", "asserted_size": 4, "bases": []},
        {"qualified_name": "Node", "asserted_size": 4, "bases": []},
    ]
    report = compare_source_layouts(
        source,
        {
            "/Node": {"length": 8, "components": []},
            "/wiz8/classes/Node": {"components": []},
        },
    )

    assert {item["kind"] for item in report["failures"]} == {"missing-pdb-class", "size"}


def test_source_layout_gate_rejects_reported_drift() -> None:
    with pytest.raises(ValueError, match="differs at 1 checks"):
        require_source_layouts(
            {
                "ok": False,
                "failure_count": 1,
                "report": "build/reports/source-layouts/report.json",
            }
        )
