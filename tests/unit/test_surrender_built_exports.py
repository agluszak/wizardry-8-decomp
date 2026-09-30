"""Focused tests for the built sr.dll export-table comparison."""

from __future__ import annotations

from wiz8decomp.surrender_exports import built_export_disagreements


def _evidence(**ordinals: int) -> dict[str, dict[str, str]]:
    return {name: {"ordinal": str(ordinal)} for name, ordinal in ordinals.items()}


def test_matching_table_has_no_disagreement() -> None:
    assert built_export_disagreements({"a": 1, "b": 2}, _evidence(a=1, b=2)) == []


def test_an_extra_class_member_export_is_named_with_the_renumbering_it_causes() -> None:
    """An operator new on a dllexport class exports without touching sr.def."""
    built = {"??2Quantizer": 1, "a": 2, "b": 3}

    assert built_export_disagreements(built, _evidence(a=1, b=2)) == [
        "not a retail export: @1 ??2Quantizer",
        "@2 a: retail ordinal 1",
        "@3 b: retail ordinal 2",
    ]


def test_a_missing_retail_export_is_reported() -> None:
    assert built_export_disagreements({"a": 1}, _evidence(a=1, b=2)) == [
        "retail export not built: @2 b"
    ]
