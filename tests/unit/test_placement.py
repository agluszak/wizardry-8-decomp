from __future__ import annotations

from pathlib import Path

from wiz8decomp.ghidra.unit_intervals import TranslationUnitLayout, UnitAnchor
from wiz8decomp.placement import placement_violations

UNIT_A = r"Local Code\Foo.cpp"
UNIT_B = r"Local Code\Bar.cpp"


def _marker(address: int, source_file: str, kind: str = "FUNCTION") -> dict[str, object]:
    return {
        "address": address,
        "marker_kind": kind,
        "marker_name": "f",
        "source_file": source_file,
        "line": 1,
        "target": "WIZ8",
    }


def test_direct_hard_owner_in_another_file_is_a_violation(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    (tmp_path / "src/wiz8/local_code/Bar.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401100, UNIT_A, "assertion"),
        ]
    )

    violations = placement_violations(
        tmp_path, layout, [_marker(0x401080, "src/wiz8/local_code/Bar.cpp")]
    )

    assert [item["address"] for item in violations] == ["0x00401080"]
    assert violations[0]["original_unit"] == UNIT_A
    assert violations[0]["attribution"] == "bounded"


def test_gap_function_is_not_a_violation(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [
            UnitAnchor(0x401000, UNIT_A, "assertion"),
            UnitAnchor(0x401200, UNIT_B, "assertion"),
        ]
    )

    assert (
        placement_violations(tmp_path, layout, [_marker(0x401100, "src/wiz8/local_code/Foo.cpp")])
        == []
    )


def test_external_and_header_markers_are_not_violations(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [UnitAnchor(0x401000, UNIT_A, "assertion")],
        external_entries={0x401000},
    )

    assert (
        placement_violations(tmp_path, layout, [_marker(0x401000, "src/wiz8/local_code/Foo.cpp")])
        == []
    )
    assert (
        placement_violations(tmp_path, layout, [_marker(0x401010, "include/wiz8/example.h")]) == []
    )
