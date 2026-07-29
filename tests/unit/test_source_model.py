from pathlib import Path

from wiz8decomp.source_model import build_source_model


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
    model = build_source_model(repository, "SURRENDER")

    assert set(model.functions) == {0x10015010, 0x10015030}
    assert model.functions[0x10015010].name == "srCore::getCopyright"
    assert model.functions[0x10015030].name == "srCore::getVersion"
    assert all(item.file.startswith("src/surrender/") for item in model.functions.values())
