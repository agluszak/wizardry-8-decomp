from pathlib import Path

import pytest
from wiz8decomp.source_model import SourceModelError, build_source_model


def test_source_model_exposes_clang_semantics_from_generated_index() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = build_source_model(repository)

    free_function = model.functions[0x004F8130]
    assert free_function.name == "ItemHasFlags"
    assert free_function.semantic_kind == "free_function"
    assert free_function.return_type == "bool"
    assert free_function.parameter_types == ("W8WorldItem *", "unsigned int")
    assert free_function.file == "src/wiz8/local_code/ItemManager.cpp"

    method = model.functions[0x004A8430]
    assert method.name == "W8GrCycle::SetSubCycle"
    assert method.semantic_kind == "instance_method"
    assert method.owning_class == "W8GrCycle"
    assert method.calling_convention == "__thiscall"

    destructor = model.functions[0x00439A00]
    assert destructor.semantic_kind == "destructor"
    assert destructor.calling_convention == "__thiscall"

    template = model.functions[0x004ADDF0]
    assert template.kind == "TEMPLATE"
    assert template.name == "W8GrowableVector<int>::Grow"

    synthetic = model.functions[0x004F6030]
    assert synthetic.kind == "SYNTHETIC"
    assert synthetic.name == "W8TextControl005ED604::`scalar deleting destructor'"
    assert synthetic.semantic_id is None

    library = model.functions[0x00401000]
    assert library.kind == "LIBRARY"
    assert library.name == "__WinMainCRTStartup"
    assert library.semantic_id is None


def test_surrender_source_model_uses_its_own_marker_target() -> None:
    repository = Path(__file__).resolve().parents[2]
    surrender = build_source_model(repository, "SURRENDER")
    wiz8 = build_source_model(repository, "WIZ8")

    assert surrender.functions[0x10015010].name == "srCore::getCopyright"
    assert surrender.functions[0x10045780].name == ("srDynamicLibrary::checkCompatibility")
    assert set(surrender.functions).isdisjoint(wiz8.functions)
    assert all(
        item.file.startswith(("src/surrender/", "include/surrender/"))
        for item in surrender.functions.values()
    )


def test_source_model_keeps_definition_as_owner_of_folded_alias(tmp_path: Path) -> None:
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v1",
  "markers": [
    {
      "address": 4878656,
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
        "is_virtual": true
      },
      "marker_name": null
    },
    {
      "address": 4878656,
      "marker_kind": "SYNTHETIC",
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

    function = build_source_model(tmp_path).functions[0x004A7140]
    assert function.name == "W8GrCycle::CanEnterCycle"
    assert function.kind == "FUNCTION"


def test_source_model_rejects_two_non_folded_owners(tmp_path: Path) -> None:
    build = tmp_path / "build"
    build.mkdir()
    (build / "source-index.json").write_text(
        """{
  "schema": "reccmp-source-index-v1",
  "markers": [
    {
      "address": 1,
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
    }
  ],
  "declarations": [],
  "classes": []
}\n""",
        encoding="utf-8",
    )

    with pytest.raises(SourceModelError, match="more than one source owner"):
        build_source_model(tmp_path)
