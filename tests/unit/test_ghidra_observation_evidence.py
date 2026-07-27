from __future__ import annotations

from wiz8decomp.ghidra.observation_evidence import (
    load_observation_bundle,
    strict_scalar_observation,
)

_CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


def test_bundle_uses_exact_program_identity_and_corrected_eh_schema() -> None:
    bundle = load_observation_bundle(_CANONICAL)

    assert bundle["eh_functions"]
    assert bundle["vtables"]
    assert bundle["globals"]
    assert all("eh_setup_start" in row for row in bundle["eh_functions"])
    assert any(row["eh_setup_start"] for row in bundle["eh_functions"])
    assert all("function_start" not in row for row in bundle["eh_functions"])


def test_strict_scalar_rejects_address_taken_and_ambiguous_widths() -> None:
    base = {"kind": "data", "access_kinds": "read write", "widths": "4"}

    assert strict_scalar_observation(base)
    assert not strict_scalar_observation({**base, "access_kinds": "address-taken read write"})
    assert not strict_scalar_observation({**base, "widths": "1 4"})
    assert not strict_scalar_observation({**base, "access_kinds": "read"})
