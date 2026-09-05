from pathlib import Path

import pytest
from reccmp.source import SourceIndexError
from wiz8decomp.source_index import source_functions


def test_source_functions_expose_clang_semantics_from_generated_index() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = source_functions(repository)

    free_function = model[0x004F8130]
    assert free_function.name == "ItemHasFlags"
    assert free_function.declaration.semantic_kind == "free_function"
    assert free_function.declaration.return_type == "bool"
    assert free_function.declaration.parameter_types == ("W8WorldItem *", "unsigned int")
    assert free_function.source_file == "src/wiz8/local_code/ItemManager.cpp"

    method = model[0x004A8430]
    assert method.name == "W8GrCycle::SetSubCycle"
    assert method.declaration.semantic_kind == "instance_method"
    assert method.declaration.owning_class == "W8GrCycle"
    assert method.declaration.calling_convention == "__thiscall"

    destructor = model[0x00439A00]
    assert destructor.declaration.semantic_kind == "destructor"
    assert destructor.declaration.calling_convention == "__thiscall"

    template = model[0x004ADDF0]
    assert template.marker_kind == "TEMPLATE"
    assert template.name == "W8GrowableVector<int>::Grow"

    synthetic = model[0x004F6030]
    assert synthetic.marker_kind == "SYNTHETIC"
    assert synthetic.name == "W8TextControl005ED604::`scalar deleting destructor'"
    assert synthetic.declaration is None

    library = model[0x00401000]
    assert library.marker_kind == "LIBRARY"
    assert library.name == "__WinMainCRTStartup"
    assert library.declaration is None


def test_surrender_source_functions_use_their_own_marker_target() -> None:
    repository = Path(__file__).resolve().parents[2]
    surrender = source_functions(repository, "SURRENDER")
    wiz8 = source_functions(repository, "WIZ8")

    assert surrender[0x10015010].name == "srCore::getCopyright"
    assert surrender[0x10045780].name == ("srDynamicLibrary::checkCompatibility")
    assert set(surrender).isdisjoint(wiz8)
    assert all(
        item.source_file.startswith(("src/surrender/", "include/surrender/"))
        for item in surrender.values()
    )


def test_source_functions_keep_definition_as_owner_of_folded_alias(tmp_path: Path) -> None:
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v1",
  "markers": [
    {
      "address": 4878656,
      "target": "WIZ8",
      "marker_kind": "FUNCTION",
      "source_file": "include/wiz8/engine_code/GrCycle.h",
      "line": 149,
      "declaration": {
        "semantic_id": "?CanEnterCycle@W8GrCycle@@UAEEC@Z",
        "qualified_name": "W8GrCycle::CanEnterCycle",
        "semantic_kind": "instance_method",
        "calling_convention": "__thiscall",
        "return_type": "unsigned char",
        "parameter_types": ["signed char"],
        "owning_class": "W8GrCycle",
        "has_this": true,
        "source_file": "include/wiz8/engine_code/GrCycle.h",
        "line": 149,
        "end_line": 150,
        "is_definition": true,
        "is_virtual": true
      },
      "marker_name": null
    },
    {
      "address": 4878656,
      "marker_kind": "SYNTHETIC",
      "target": "WIZ8",
      "source_file": "include/wiz8/engine_code/GrCycle.h",
      "line": 196,
      "declaration": null,
      "marker_name": "W8Navigator::secondary_vslot3",
      "folded": true
    }
  ],
  "declarations": [],
  "classes": []
}\n""",
        encoding="utf-8",
    )

    function = source_functions(tmp_path)[0x004A7140]
    assert function.name == "W8GrCycle::CanEnterCycle"
    assert function.marker_kind == "FUNCTION"


def test_source_functions_reject_two_non_folded_owners(tmp_path: Path) -> None:
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v1",
  "markers": [
    {
      "address": 1,
      "target": "WIZ8",
      "marker_kind": "SYNTHETIC",
      "source_file": "src/wiz8/a.cpp",
      "line": 1,
      "declaration": null,
      "marker_name": "First"
    },
    {
      "address": 1,
      "marker_kind": "SYNTHETIC",
      "source_file": "src/wiz8/b.cpp",
      "line": 1,
      "declaration": null,
      "marker_name": "Second"
      ,"target": "WIZ8"
    }
  ],
  "declarations": [],
  "classes": []
}\n""",
        encoding="utf-8",
    )

    with pytest.raises(SourceIndexError, match="more than one source owner"):
        source_functions(tmp_path)
