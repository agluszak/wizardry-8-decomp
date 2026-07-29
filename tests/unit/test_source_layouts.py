from types import SimpleNamespace

import pytest
from wiz8decomp.reconstructed_pdb import CompiledField, CompiledLayout
from wiz8decomp.source_layouts import compare_source_layouts, require_source_layouts


def test_source_layout_audit_checks_size_offset_width_and_pointer_depth() -> None:
    reviewed = SimpleNamespace(
        classes=[SimpleNamespace(name="GDProp", size=0x58, base_classes="")],
        fields=[
            SimpleNamespace(
                class_name="GDProp",
                name="instance_24",
                offset=0x24,
                size=4,
                data_type="pointer",
                pointee="srNode",
            )
        ],
    )
    compiled = {
        "GDProp": CompiledLayout(
            "GDProp",
            0x58,
            (CompiledField("m_instance_24", 0x24, 4, 1, "srNode *"),),
            (),
        )
    }

    report = compare_source_layouts(reviewed, compiled)

    assert report["ok"] is True
    assert report["checks"] == {"classes": 1, "fields": 1, "bases": 0}


def test_source_layout_audit_treats_reviewed_size_as_a_minimum() -> None:
    reviewed = SimpleNamespace(
        classes=[SimpleNamespace(name="Node", size=4, base_classes="")],
        fields=[],
    )
    compiled = {"Node": CompiledLayout("Node", 8, (), ())}

    assert compare_source_layouts(reviewed, compiled)["ok"] is True


def test_source_layout_gate_rejects_reported_drift() -> None:
    with pytest.raises(ValueError, match="differs at 1 checks"):
        require_source_layouts(
            {
                "ok": False,
                "failure_count": 1,
                "report": "build/reports/source-layouts/report.json",
            }
        )
