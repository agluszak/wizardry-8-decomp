from pathlib import Path

from wiz8decomp.source_model import build_source_model


def test_every_source_owned_function_has_one_legal_ghidra_name() -> None:
    repository = Path(__file__).resolve().parents[2]
    functions = build_source_model(repository).functions

    assert functions
    assert len(functions) == len(set(functions))
    assert all(
        item.name and " " not in item.name and "`" not in item.name for item in functions.values()
    )
