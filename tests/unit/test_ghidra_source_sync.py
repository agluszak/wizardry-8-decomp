import pytest
from wiz8decomp.ghidra.source_sync import require_source_synchronized


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
