"""Tests for Cosmic Forge cfdat override loading and datatype strength."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.cosmic_forge_globals import (
    _ELEMENT_SIZES,
    _cf_type_decision,
    _datatype_rank,
    _is_named_structure,
    _load_overrides,
)


def test_load_overrides_includes_element_type() -> None:
    repo = Path(__file__).resolve().parents[2]
    rows = _load_overrides(repo)
    by_name = {row["filename"]: row for row in rows}

    mag = by_name["classesmagschools.cfdat"]
    assert mag["element_type"] == "byte"
    assert mag["element_count"] == 15
    assert mag["element_count"] * _ELEMENT_SIZES["byte"] == mag["size"]

    hps = by_name["classeshps.cfdat"]
    assert hps["element_type"] == "float"
    assert hps["element_count"] == 15

    skills = by_name["skillsattrs.cfdat"]
    assert skills["element_type"] == "int"
    assert skills["element_count"] == 0x290 // 4

    # compiler-lowering rows are skipped entirely
    assert "classesexpgroup.cfdat" not in by_name


def test_element_layout_matches_canonical_size() -> None:
    repo = Path(__file__).resolve().parents[2]
    for row in _load_overrides(repo):
        element_type = row["element_type"]
        element_count = row["element_count"]
        assert element_type is not None
        assert element_count is not None
        assert element_count * _ELEMENT_SIZES[element_type] == row["size"]


def test_typed_upgrades_same_length_wrong_element() -> None:
    needs_type, skip = _cf_type_decision(
        status="typed",
        current_type="uchar[60]",
        current_length=60,
        size=60,
        element_type="int",
        element_count=15,
    )
    assert needs_type is True
    assert skip is None


def test_typed_agrees_when_cf_layout_already_present() -> None:
    needs_type, skip = _cf_type_decision(
        status="typed",
        current_type="int[15]",
        current_length=60,
        size=60,
        element_type="int",
        element_count=15,
    )
    assert needs_type is False
    assert skip is None


def test_partially_typed_preserves_named_structure_even_on_length_mismatch() -> None:
    needs_type, skip = _cf_type_decision(
        status="partially-typed",
        current_type="ReviewedBlob",
        current_length=48,
        size=60,
        element_type="int",
        element_count=15,
    )
    assert needs_type is False
    assert skip == "preserve-reviewed"


def test_partially_typed_upgrades_generic_weaker_than_cf() -> None:
    needs_type, skip = _cf_type_decision(
        status="partially-typed",
        current_type="uchar[60]",
        current_length=60,
        size=60,
        element_type="int",
        element_count=15,
    )
    assert needs_type is True
    assert skip is None
    assert (
        _datatype_rank(
            "uchar[60]",
            element_type="int",
            element_count=15,
            length=60,
            size=60,
        )
        < 3
    )


def test_is_named_structure_prefers_datatype_over_spelling() -> None:
    class _ArrayBlob:
        pass

    class _StructDT:
        pass

    # Spelling alone looks structure-like, but the DataType is not.
    assert not _is_named_structure("LooksLikeStruct", data_type=_ArrayBlob())
    # Spelling looks like a scalar name, but the DataType name marks a Structure.
    _StructDT.__name__ = "StructureDataType"
    assert _is_named_structure("int", data_type=_StructDT())
    assert not _is_named_structure("int")
    assert _is_named_structure("ReviewedBlob")
    assert not _is_named_structure("uchar[60]")
    assert not _is_named_structure("void *")


def test_cf_preserves_named_element_array() -> None:
    needs_type, skip = _cf_type_decision(
        status="typed",
        current_type="W8AttributeMinimums[11]",
        current_length=308,
        size=308,
        element_type="int",
        element_count=77,
    )
    assert needs_type is False
    assert skip == "preserve-reviewed"
