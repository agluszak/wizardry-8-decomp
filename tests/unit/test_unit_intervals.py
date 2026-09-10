from __future__ import annotations

import pytest
from wiz8decomp.ghidra.unit_intervals import (
    CROSS_BUILD,
    TranslationUnitContradiction,
    TranslationUnitLayout,
    UnitAnchor,
    assertion_anchors,
    derive_intervals,
)

UNIT_A = r"Local Code\Foo.cpp"
UNIT_B = r"Local Code\Bar.cpp"


def _anchor(function: int, unit: str, evidence: str = "assertion", **kwargs) -> UnitAnchor:
    return UnitAnchor(function=function, source_path=unit, evidence=evidence, **kwargs)


def test_direct_anchor_reports_its_source_line_and_evidence() -> None:
    layout = TranslationUnitLayout([_anchor(0x401000, UNIT_A, line=454)])

    owner = layout.owner(0x401000)

    assert owner["source_path"] == UNIT_A
    assert owner["attribution"] == "direct"
    assert owner["line"] == 454
    assert owner["evidence"][0]["evidence"] == "assertion"


def test_bounded_function_inside_a_single_unit_hull() -> None:
    layout = TranslationUnitLayout([_anchor(0x401000, UNIT_A), _anchor(0x401100, UNIT_A)])

    owner = layout.owner(0x401080)

    assert owner["source_path"] == UNIT_A
    assert owner["attribution"] == "bounded"
    assert owner["interval_lower"] == "00401000"
    assert owner["interval_upper"] == "00401100"


def test_gap_between_units_keeps_both_neighbours_visible() -> None:
    layout = TranslationUnitLayout([_anchor(0x401000, UNIT_A), _anchor(0x401200, UNIT_B)])

    owner = layout.owner(0x401100)

    assert owner["attribution"] == "gap"
    assert owner["source_path"] == ""
    assert owner["previous_hard_unit"]["source_path"] == UNIT_A
    assert owner["next_hard_unit"]["source_path"] == UNIT_B


def test_two_units_in_one_function_are_inlined_or_conflicting() -> None:
    layout = TranslationUnitLayout([_anchor(0x401000, UNIT_A), _anchor(0x401000, UNIT_B)])

    owner = layout.owner(0x401000)

    assert owner["attribution"] == "inlined-or-conflicting"
    assert owner["alternatives"] == sorted([UNIT_A, UNIT_B])


def test_overlapping_hulls_are_a_model_contradiction() -> None:
    with pytest.raises(TranslationUnitContradiction, match="contiguous"):
        TranslationUnitLayout(
            [_anchor(0x401000, UNIT_A), _anchor(0x401200, UNIT_A), _anchor(0x401100, UNIT_B)]
        )


def test_cross_build_anchor_keeps_its_origin() -> None:
    layout = TranslationUnitLayout(
        [
            _anchor(
                0x573420,
                UNIT_A,
                evidence=CROSS_BUILD,
                origin_variant="demo",
                origin_function=0x401234,
            )
        ]
    )

    owner = layout.owner(0x573420)

    assert owner["attribution"] == "cross-build"
    assert owner["evidence"][0]["origin_variant"] == "demo"
    assert owner["evidence"][0]["origin_function"] == "00401234"


def test_similar_body_cross_build_is_advisory_not_a_hard_hull() -> None:
    layout = TranslationUnitLayout(
        [
            _anchor(0x401000, UNIT_A, evidence=CROSS_BUILD, match_kind="similar-body"),
            _anchor(0x401200, UNIT_A, evidence=CROSS_BUILD, match_kind="similar-body"),
        ]
    )

    assert layout.intervals == []
    assert layout.owner(0x401000)["attribution"] == "cross-build-similar"
    assert layout.owner(0x401100)["attribution"] == "gap"


def test_similar_body_anchor_cannot_extend_a_unique_hull() -> None:
    layout = TranslationUnitLayout(
        [
            _anchor(0x401000, UNIT_A, evidence=CROSS_BUILD, match_kind="unique-body"),
            _anchor(0x401100, UNIT_A, evidence=CROSS_BUILD, match_kind="unique-body"),
            _anchor(0x401300, UNIT_A, evidence=CROSS_BUILD, match_kind="similar-body"),
        ]
    )

    assert [(interval.lower, interval.upper) for interval in layout.intervals] == [
        (0x401000, 0x401100)
    ]
    assert layout.owner(0x401200)["attribution"] == "gap"
    assert layout.owner(0x401300)["attribution"] == "cross-build-similar"


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

    units, headers = assertion_anchors(rows)
    layout = TranslationUnitLayout(units, header_anchors=headers)

    assert [anchor.source_path for anchor in units] == [r"Engine Code\Octree.cpp"]
    assert [anchor.header_path for anchor in headers] == [r"..\Engine Code\Include\AnimRep.hpp"]
    assert layout.header_owner(0x401000)["attribution"] == "header-origin"


def test_assertion_projection_still_drives_interval_report() -> None:
    rows = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\ItemManager.cpp",
            "containing_function": "004f88f0",
            "line": "1",
        }
    ]
    intervals = derive_intervals(rows)
    assert [interval.source_path for interval in intervals] == [r"Local Code\ItemManager.cpp"]
