from __future__ import annotations

import pytest
from wiz8decomp.config import load_settings
from wiz8decomp.inputs.scan import load_local_inputs


def test_configured_real_inputs_exist_or_skip() -> None:
    settings = load_settings(require=False)
    if settings is None or not settings.input_dir.is_dir():
        pytest.skip("Wizardry inputs are not configured on this machine")
    mapping = load_local_inputs(settings)
    if not mapping:
        pytest.skip("no real inputs configured")
    assert {"gog-media", "demo", "patch-1261", "patch-128"}.issubset(set(mapping.values()))
