from wiz8decomp.source_layouts import layout_failure_key


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
