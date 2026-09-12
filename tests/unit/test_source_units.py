from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.source_units import (
    SourceUnitError,
    mapped_repository_source_file,
    source_unit_violations,
    validate_source_units,
)


def _cmake(units: list[str]) -> str:
    listed = "\n".join(f"    {item}" for item in units)
    return f"set(WIZ8_SOURCE_UNITS\n{listed}\n)\n"


def test_directory_filename_mapping_beats_basename(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/engine_code").mkdir(parents=True)
    (tmp_path / "src/wiz8").mkdir(parents=True, exist_ok=True)
    (tmp_path / "src/wiz8/engine_code/Levels.cpp").write_text("", encoding="utf-8")
    (tmp_path / "src/wiz8/Levels.cpp").write_text("", encoding="utf-8")

    assert (
        mapped_repository_source_file(tmp_path, r"Engine Code\Levels.cpp")
        == "src/wiz8/engine_code/Levels.cpp"
    )


def test_original_path_map_is_directory_qualified(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8/local_code").mkdir(parents=True)
    (tmp_path / "src/wiz8/local_code/FormationAndFacing.cpp").write_text(
        "// FUNCTION: WIZ8 0x005549e0\nvoid f() {}\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        _cmake(["src/wiz8/local_code/FormationAndFacing.cpp"]), encoding="utf-8"
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        json.dumps(
            {
                "schema": "wiz8.source-units-v1",
                "compiler-emission": [],
                "unresolved-fragment": [],
                "original-path-map": {
                    r"Local Code\Formation & Facing.cpp": (
                        "src/wiz8/local_code/FormationAndFacing.cpp"
                    )
                },
            }
        ),
        encoding="utf-8",
    )
    (tmp_path / "evidence/observations/wiz8").mkdir(parents=True)
    (tmp_path / "evidence/observations/wiz8/source-tree.csv").write_text(
        "relative_path,subsystem,canonical_absolute_path,demo_absolute_path,variants\n"
        "Local Code\\Formation & Facing.cpp,Local Code,"
        "C:\\Projects\\Wizardry 8\\Local Code\\Formation & Facing.cpp,,gog-base\n",
        encoding="utf-8",
    )

    assert (
        mapped_repository_source_file(tmp_path, r"Local Code\Formation & Facing.cpp")
        == "src/wiz8/local_code/FormationAndFacing.cpp"
    )
    assert validate_source_units(tmp_path)["original-tu"] == 1


def test_empty_non_emission_file_is_a_violation(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8").mkdir(parents=True)
    (tmp_path / "src/wiz8/empty.cpp").write_text('#include "wiz8/empty.h"\n', encoding="utf-8")
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        _cmake(["src/wiz8/empty.cpp"]), encoding="utf-8"
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        json.dumps(
            {
                "schema": "wiz8.source-units-v1",
                "compiler-emission": [],
                "unresolved-fragment": ["src/wiz8/empty.cpp"],
            }
        ),
        encoding="utf-8",
    )

    violations = source_unit_violations(tmp_path)
    assert any(item["kind"] == "empty-translation-unit" for item in violations)


def test_catch_all_cannot_be_original_tu(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8").mkdir(parents=True)
    (tmp_path / "src/wiz8/state_getters.cpp").write_text(
        "// FUNCTION: WIZ8 0x0042b580\nint GetLoadedLevelID(void) { return 0; }\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        _cmake(["src/wiz8/state_getters.cpp"]), encoding="utf-8"
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        json.dumps({"schema": "wiz8.source-units-v1", "compiler-emission": []}),
        encoding="utf-8",
    )
    (tmp_path / "evidence/observations/wiz8").mkdir(parents=True)
    (tmp_path / "evidence/observations/wiz8/source-tree.csv").write_text(
        "relative_path,subsystem,canonical_absolute_path,demo_absolute_path,variants\n"
        "state_getters.cpp,root,C:\\Projects\\Wizardry 8\\state_getters.cpp,,gog-base\n",
        encoding="utf-8",
    )

    violations = source_unit_violations(tmp_path)
    assert any(item["kind"] == "catch-all-original-tu" for item in violations)


def test_catch_all_requires_explicit_unresolved_classification(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8").mkdir(parents=True)
    (tmp_path / "src/wiz8/state_getters.cpp").write_text(
        "// FUNCTION: WIZ8 0x0042b580\nint GetLoadedLevelID(void) { return 0; }\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        _cmake(["src/wiz8/state_getters.cpp"]), encoding="utf-8"
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        json.dumps({"schema": "wiz8.source-units-v1", "compiler-emission": []}),
        encoding="utf-8",
    )

    violations = source_unit_violations(tmp_path)
    assert any(item["kind"] == "catch-all-unclassified" for item in violations)


def test_compiler_emission_need_not_map_to_an_original_path(tmp_path: Path) -> None:
    (tmp_path / "src/wiz8").mkdir(parents=True)
    (tmp_path / "src/wiz8/vector.cpp").write_text(
        "// TEMPLATE: WIZ8 0x004addf0\n// W8GrowableVector<int>::Grow\n",
        encoding="utf-8",
    )
    (tmp_path / "src/wiz8/sources.cmake").write_text(
        _cmake(["src/wiz8/vector.cpp"]), encoding="utf-8"
    )
    (tmp_path / "src/wiz8/source_units.json").write_text(
        json.dumps(
            {
                "schema": "wiz8.source-units-v1",
                "compiler-emission": ["src/wiz8/vector.cpp"],
                "unresolved-fragment": [],
            }
        ),
        encoding="utf-8",
    )
    (tmp_path / "evidence/observations/wiz8").mkdir(parents=True)
    (tmp_path / "evidence/observations/wiz8/source-tree.csv").write_text(
        "relative_path,subsystem,canonical_absolute_path,demo_absolute_path,variants\n",
        encoding="utf-8",
    )

    assert validate_source_units(tmp_path)["compiler-emission"] == 1
    with pytest.raises(SourceUnitError):
        (tmp_path / "src/wiz8/vector.cpp").write_text("", encoding="utf-8")
        (tmp_path / "src/wiz8/sources.cmake").write_text(
            _cmake(["src/wiz8/missing.cpp"]), encoding="utf-8"
        )
        validate_source_units(tmp_path)
