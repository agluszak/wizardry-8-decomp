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

    template = model.functions[0x004ADDF0]
    assert template.kind == "TEMPLATE"
    assert template.name == "W8GrowableVector<int>::Grow"
    assert template.prototype.startswith("int W8GrowableVector<T>::Grow(")

    library = model.functions[0x00401000]
    assert library.kind == "LIBRARY"
    assert library.name == "__WinMainCRTStartup"
    assert library.prototype == ""


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
