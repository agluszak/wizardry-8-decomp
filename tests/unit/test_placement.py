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


def test_similar_body_cross_build_does_not_enforce_placement(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    (tmp_path / "src/wiz8/local_code/Bar.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [
            UnitAnchor(
                0x401000,
                UNIT_A,
                "cross-build",
                origin_variant="demo",
                origin_function=0x401234,
                match_kind="similar-body",
                score=0.9,
            )
        ]
    )

    violations = placement_violations(
        tmp_path, layout, [_marker(0x401000, "src/wiz8/local_code/Bar.cpp")]
    )

    assert violations == []


def test_strong_attribution_without_physical_file_is_unresolved(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A, "assertion")])

    violations = placement_violations(
        tmp_path, layout, [_marker(0x401000, "src/wiz8/state_getters.cpp")]
    )

    assert violations[0]["kind"] == "unresolved-placement"
    assert violations[0]["original_unit"] == UNIT_A


def test_provisional_file_does_not_become_the_original_unit(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/engine_code").mkdir(parents=True)
    (tmp_path / "src/wiz8").mkdir(parents=True, exist_ok=True)
    (tmp_path / "src/wiz8/engine_code/Levels.cpp").write_text("", encoding="utf-8")
    (tmp_path / "src/wiz8/state_getters.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [UnitAnchor(0x42B410, r"Engine Code\Levels.cpp", "assertion")]
    )

    violations = placement_violations(
        tmp_path, layout, [_marker(0x42B410, "src/wiz8/state_getters.cpp")]
    )

    assert violations[0]["kind"] == "wrong-translation-unit"
    assert violations[0]["expected_source"] == "src/wiz8/engine_code/Levels.cpp"


def test_same_basename_in_wrong_directory_is_not_the_original_unit(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8").mkdir(parents=True, exist_ok=True)
    (tmp_path / "src/wiz8/Foo.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A, "assertion")])

    violations = placement_violations(tmp_path, layout, [_marker(0x401000, "src/wiz8/Foo.cpp")])

    assert violations[0]["kind"] == "unresolved-placement"
    assert violations[0]["current_source"] == "src/wiz8/Foo.cpp"


def test_exact_original_path_mapping_is_accepted(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout([UnitAnchor(0x401000, UNIT_A, "assertion")])

    assert (
        placement_violations(tmp_path, layout, [_marker(0x401000, "src/wiz8/local_code/Foo.cpp")])
        == []
    )


def test_advisory_cross_build_attribution_is_not_fatal(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/Foo.cpp").write_text("", encoding="utf-8")
    layout = TranslationUnitLayout(
        [
            UnitAnchor(
                0x401000,
                UNIT_A,
                "cross-build",
                origin_variant="demo",
                origin_function=0x401234,
                match_kind="similar-body",
                score=0.9,
            )
        ]
    )

    assert (
        placement_violations(tmp_path, layout, [_marker(0x401000, "src/wiz8/provisional.cpp")])
        == []
    )


def test_unresolved_fragment_does_not_prove_original_tu(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/engine_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/engine_code/world_selection.cpp").write_text(
        "// FUNCTION: WIZ8 0x00451280\nvoid GetWorld() {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        'set(WIZ8_SOURCE_UNITS\n    src/wiz8/engine_code/world_selection.cpp\n)\n',
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        '{"schema": "wiz8.source-units-v1", "compiler-emission": [],'
        ' "unresolved-fragment": ["src/wiz8/engine_code/world_selection.cpp"]}',
        encoding="utf-8",
    )
    layout = TranslationUnitLayout(
        [UnitAnchor(0x451280, r"Engine Code\world_selection.cpp", "assertion")]
    )

    violations = placement_violations(
        tmp_path, layout, [_marker(0x451280, "src/wiz8/engine_code/world_selection.cpp")]
    )

    assert violations[0]["kind"] == "unresolved-placement"


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
