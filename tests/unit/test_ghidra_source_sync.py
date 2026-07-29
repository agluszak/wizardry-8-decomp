from pathlib import Path

import pytest
from wiz8decomp.ghidra.source_sync import require_source_synchronized
from wiz8decomp.source_model import build_source_model


def test_every_source_owned_function_has_one_legal_ghidra_name() -> None:
    repository = Path(__file__).resolve().parents[2]
    functions = build_source_model(repository).functions

    assert functions
    assert len(functions) == len(set(functions))
    assert all(
        item.name and " " not in item.name and "`" not in item.name for item in functions.values()
    )


def test_source_sync_gate_rejects_drift_but_not_explicitly_unsupported_types() -> None:
    require_source_synchronized(
        {"changed": 0, "missing": [], "external_missing": [], "unsupported": [{}]}
    )

    with pytest.raises(ValueError, match="source synchronization is pending"):
        require_source_synchronized(
            {"changed": 1, "missing": [], "external_missing": [], "unsupported": []}
        )

    with pytest.raises(ValueError, match="1 missing"):
        require_source_synchronized({"changed": 0, "missing": ["00401000"], "external_missing": []})
