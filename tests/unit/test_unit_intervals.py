from __future__ import annotations

import pytest
from wiz8decomp.unit_intervals import (
    TranslationUnitContradiction,
    TranslationUnitLayout,
    UnitAnchor,
    assertion_anchors,
)

UNIT_A = r"Local Code\Foo.cpp"
UNIT_B = r"Local Code\Bar.cpp"


def test_direct_anchor_reports_its_source_line() -> None:
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A, line=454)])

    owner = layout.owner(0x401000)

    assert owner["source_path"] == UNIT_A
    assert owner["attribution"] == "direct"
    assert owner["evidence"] == [{"evidence": "assertion", "function": "00401000", "line": 454}]


def test_bounded_function_inside_a_single_unit_hull() -> None:
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A), UnitAnchor(0x401100, UNIT_A)])

    owner = layout.owner(0x401080)

    assert owner["source_path"] == UNIT_A
    assert owner["attribution"] == "bounded"


def test_gap_between_units_has_no_owner() -> None:
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A), UnitAnchor(0x401200, UNIT_B)])

    owner = layout.owner(0x401100)

    assert owner["attribution"] == "gap"
    assert owner["source_path"] == ""


def test_two_units_in_one_function_are_inlined_or_conflicting() -> None:
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A), UnitAnchor(0x401000, UNIT_B)])

    owner = layout.owner(0x401000)

    assert owner["attribution"] == "inlined-or-conflicting"
    assert owner["alternatives"] == sorted([UNIT_A, UNIT_B])


def test_overlapping_hulls_are_a_model_contradiction() -> None:
    with pytest.raises(TranslationUnitContradiction, match="contiguous"):
        TranslationUnitLayout(
            [
                UnitAnchor(0x401000, UNIT_A),
                UnitAnchor(0x401200, UNIT_A),
                UnitAnchor(0x401100, UNIT_B),
            ]
        )


def test_header_spelling_is_not_a_unit_anchor() -> None:
    rows = [
        {
            "source_path": r"..\Engine Code\Include\AnimRep.hpp",
            "containing_function": "00401000",
            "line": "12",
        },
        {
            "source_path": r"C:\Projects\Wizardry 8\Engine Code\Octree.cpp",
            "containing_function": "00401000",
            "line": "454",
        },
    ]

    units = assertion_anchors(rows)

    assert units == [UnitAnchor(0x401000, r"Engine Code\Octree.cpp", 454)]
