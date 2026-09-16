from __future__ import annotations

from wiz8decomp.commands.vtables import _mark_match_ambiguity


def test_unique_shape_group_is_marked_unique() -> None:
    result = {
        "matches": [
            {
                "fingerprint": "1|L0",
                "tables": [
                    {"report": 0, "address": "0x1000"},
                    {"report": 1, "address": "0x2000"},
                ],
            }
        ]
    }

    _mark_match_ambiguity(result)

    assert result["matches"][0]["match_kind"] == "unique-shape"
    assert result["matches"][0]["per_report_counts"] == {0: 1, 1: 1}
    assert result["summary"] == {"unique_shape_groups": 1, "ambiguous_shape_groups": 0}


def test_many_to_many_shape_group_is_marked_ambiguous() -> None:
    result = {
        "matches": [
            {
                "fingerprint": "1|L0",
                "tables": [
                    {"report": 0, "address": "0x1000"},
                    {"report": 0, "address": "0x1040"},
                    {"report": 1, "address": "0x2000"},
                ],
            }
        ]
    }

    _mark_match_ambiguity(result)

    assert result["matches"][0]["match_kind"] == "ambiguous-shape"
    assert result["matches"][0]["per_report_counts"] == {0: 2, 1: 1}
    assert result["summary"] == {"unique_shape_groups": 0, "ambiguous_shape_groups": 1}
