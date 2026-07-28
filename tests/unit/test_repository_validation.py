from __future__ import annotations

import pytest
from wiz8decomp.evidence.io import merge_monotonic
from wiz8decomp.evidence.validate import (
    validate_exact_digests,
    validate_field_rows,
    validate_provenance_rows,
    validate_source_entries,
    validate_unique,
    validate_vtable_rows,
)


def test_duplicate_function_address_is_rejected() -> None:
    rows = [{"program": "wiz8", "address": "00401000"}] * 2
    with pytest.raises(ValueError, match="duplicate identity"):
        validate_unique(rows, ("program", "address"), label="functions")


def test_dangling_vtable_id_is_rejected() -> None:
    with pytest.raises(ValueError, match="dangling vtable ID"):
        validate_vtable_rows([], [{"vtable_id": "missing", "slot_index": "0"}], label="slots")


def test_overlapping_fields_are_rejected() -> None:
    rows = [
        {"class_name": "Owner", "offset": "0x0", "size": "0x4"},
        {"class_name": "Owner", "offset": "0x2", "size": "0x4"},
    ]
    with pytest.raises(ValueError, match="overlapping field"):
        validate_field_rows({"Owner": 8}, rows, label="fields")


def test_invalid_provenance_is_rejected() -> None:
    rows = [{"name_origin": "guess", "authority": "reviewed"}]
    with pytest.raises(ValueError):
        validate_provenance_rows(rows, label="functions")


def test_exact_boundary_without_digest_is_rejected() -> None:
    rows = [{"address": "00401000", "confidence": "exact", "relocation_masked_sha256": ""}]
    with pytest.raises(ValueError, match="no valid digest"):
        validate_exact_digests(rows, label="boundaries")


def test_conflicting_evidence_merge_is_rejected() -> None:
    with pytest.raises(ValueError, match="semantic evidence conflict"):
        merge_monotonic(
            {"program": "wiz8", "address": "00401000", "name": "One"},
            {"program": "wiz8", "address": "00401000", "name": "Two"},
        )


def test_duplicate_source_entry_is_rejected() -> None:
    with pytest.raises(ValueError, match="duplicate source entry"):
        validate_source_entries(["src/wiz8/A.cpp", "src/wiz8/A.cpp"], label="sources")
