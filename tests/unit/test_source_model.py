from pathlib import Path

import pytest
from wiz8decomp.source_model import build_source_model, validate_source_names_against_index


def test_source_model_derives_identity_and_prototype_from_source() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = build_source_model(repository)

    free_function = model.functions[0x004F8130]
    assert free_function.name == "ItemHasFlags"
    assert free_function.prototype == "bool ItemHasFlags(W8WorldItem* item, unsigned int mask)"
    assert free_function.file == "src/wiz8/local_code/ItemManager.cpp"

    method = model.functions[0x004A8430]
    assert method.name == "W8GrCycle::SetSubCycle"
    assert method.prototype == "void W8GrCycle::SetSubCycle(unsigned char subcycle)"
    assert method.ghidra_prototype == "void source_function(unsigned char subcycle)"
    assert method.calling_convention == "__thiscall"

    stdcall = model.functions[0x004011E0]
    assert stdcall.ghidra_prototype == (
        "long source_function(void* window, int message, unsigned int wparam, long lparam)"
    )
    assert stdcall.calling_convention == "__stdcall"

    const_pointer = model.functions[0x00427A60]
    assert const_pointer.ghidra_prototype == "char* source_function(void)"
    assert const_pointer.calling_convention == "__cdecl"

    destructor = model.functions[0x00439A00]
    assert destructor.ghidra_prototype == "void source_function()"
    assert destructor.calling_convention == "__thiscall"

    template = model.functions[0x004ADDF0]
    assert template.kind == "TEMPLATE"
    assert template.name == "W8GrowableVector<int>::Grow"
    assert template.prototype.startswith("int W8GrowableVector<T>::Grow(")

    synthetic = model.functions[0x004F6030]
    assert synthetic.kind == "SYNTHETIC"
    assert synthetic.name == "W8TextControl005ED604::`scalar deleting destructor'"
    assert synthetic.prototype == ""

    library = model.functions[0x00401000]
    assert library.kind == "LIBRARY"
    assert library.name == "__WinMainCRTStartup"
    assert library.prototype == ""
    assert library.ghidra_prototype == ""

    external = model.externals["srAssertFail"]
    assert external.calling_convention == "__cdecl"
    assert external.ghidra_prototype == (
        "void source_function( char* expression, char* source_path, long line, char* message)"
    )
    assert "..." not in external.ghidra_prototype


def test_source_index_validation_rejects_a_stale_ghidra_name(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/Example.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        "// FUNCTION: WIZ8 0x00401000\nvoid RecoveredName(void) {}\n",
        encoding="utf-8",
    )
    (tmp_path / "include/wiz8").mkdir(parents=True)
    document = {
        "functions": [
            {
                "entry": "00401000",
                "name": "FUN_00401000",
                "qualified_name": "FUN_00401000",
                "namespace": "Global",
            }
        ]
    }

    with pytest.raises(ValueError, match="synchronization is pending"):
        validate_source_names_against_index(tmp_path, document)


def test_surrender_source_model_uses_its_own_roots_and_marker_target() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = build_source_model(repository, "SURRENDER")

    assert set(model.functions) == {0x10015010, 0x10015030}
    assert model.functions[0x10015010].name == "srCore::getCopyright"
    assert model.functions[0x10015030].name == "srCore::getVersion"
    assert all(item.file.startswith("src/surrender/") for item in model.functions.values())
