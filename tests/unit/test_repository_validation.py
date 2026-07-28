import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence.classes import load_reviewed_class_model
from wiz8decomp.evidence.schema import schema_for
from wiz8decomp.evidence.validate import (
    _validate_boundaries,
    _validate_functions,
    require_valid_repository,
    validate_source_entries,
)
from wiz8decomp.evidence_merge import EvidenceMergeConflict, stronger

REPOSITORY = Path(__file__).resolve().parents[2]


def _write(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    schema = schema_for(path.name)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=schema.columns)
        writer.writeheader()
        writer.writerows(rows)


def _function(address: str, **overrides: str) -> dict[str, str]:
    row = {column: "" for column in schema_for("functions.csv").columns}
    row.update(
        program="wiz8",
        address=address,
        size="0x10",
        provisional_name="Function" + address,
        owner="wiz8",
        confidence="strong",
        name_origin="descriptive",
        authority="descriptive",
        evidence="fixture",
    )
    row.update(overrides)
    return row


def _class_model(repo: Path, fields: list[dict[str, str]], slots: list[dict[str, str]]) -> None:
    _write(
        repo / "evidence/reviewed/demo/classes.csv",
        [
            {
                "program": "demo",
                "class_name": "Node",
                "confidence": "exact",
                "primary_vtable_id": "Node.primary",
                "minimum_size": "0x8",
            }
        ],
    )
    _write(repo / "evidence/reviewed/demo/fields.csv", fields)
    _write(
        repo / "evidence/reviewed/demo/vtables.csv",
        [
            {
                "program": "demo",
                "vtable_id": "Node.primary",
                "class_name": "Node",
                "address": "00500000",
                "subobject_offset": "0x0",
                "kind": "primary",
                "slot_count": "1",
                "confidence": "exact",
                "evidence_id": "classes:demo:Node",
            }
        ],
    )
    _write(repo / "evidence/reviewed/demo/vtable-slots.csv", slots)


def _field(offset: str) -> dict[str, str]:
    return {
        "program": "demo",
        "class_name": "Node",
        "offset": offset,
        "size": "0x4",
        "field_name": "field_" + offset,
        "data_type": "uint32",
        "pointee": "",
        "confidence": "exact",
        "evidence_id": "classes:demo:Node",
        "description": "fixture",
    }


def _slot(vtable_id: str = "Node.primary") -> dict[str, str]:
    return {
        "program": "demo",
        "vtable_id": vtable_id,
        "slot_index": "0",
        "target": "00401000",
        "slot_name": "Destroy",
        "confidence": "exact",
        "evidence_id": "classes:demo:Node",
    }


def test_repository_validator_accepts_the_current_canonical_ledger() -> None:
    assert require_valid_repository(REPOSITORY)["ok"] is True


def test_function_validator_rejects_duplicate_address(tmp_path: Path) -> None:
    path = tmp_path / "evidence/reviewed/wiz8/functions.csv"
    _write(path, [_function("00401000"), _function("00401000")])

    with pytest.raises(ValueError, match="duplicate identity"):
        _validate_functions(tmp_path, "wiz8")


def test_function_validator_rejects_invalid_provenance(tmp_path: Path) -> None:
    path = tmp_path / "evidence/reviewed/wiz8/functions.csv"
    _write(
        path, [_function("00401000", name_origin="fan-patch-signature", authority="source-backed")]
    )

    with pytest.raises(ValueError, match="not derivable"):
        _validate_functions(tmp_path, "wiz8")


def test_class_validator_rejects_overlapping_fields(tmp_path: Path) -> None:
    _class_model(tmp_path, [_field("0x0"), _field("0x2")], [_slot()])

    with pytest.raises(ValueError, match="overlapping field"):
        load_reviewed_class_model(tmp_path, "demo")


def test_class_validator_rejects_dangling_vtable_id(tmp_path: Path) -> None:
    _class_model(tmp_path, [_field("0x0")], [_slot("Missing.primary")])

    with pytest.raises(ValueError, match="unknown vtable"):
        load_reviewed_class_model(tmp_path, "demo")


def test_boundary_validator_requires_digest_for_exact_row(tmp_path: Path) -> None:
    path = tmp_path / "config/reccmp/wiz8-gameplay-boundaries.csv"
    _write(
        path,
        [
            {
                "address": "00401000",
                "size": "0x10",
                "symbol": "Entry",
                "owner": "wiz8",
                "confidence": "exact",
                "relocation_masked_sha256": "",
                "evidence": "fixture",
            }
        ],
    )

    with pytest.raises(ValueError, match="requires a SHA-256 digest"):
        _validate_boundaries(tmp_path)


def test_conflicting_semantic_merge_is_rejected() -> None:
    with pytest.raises(EvidenceMergeConflict, match="semantic evidence conflict"):
        stronger(
            {"address": "00401000", "symbol": "Entry"}, {"address": "00401000", "symbol": "Other"}
        )


def test_source_validator_rejects_duplicate_entry(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/Unit.cpp"
    source.parent.mkdir(parents=True)
    source.write_text("", encoding="utf-8")

    with pytest.raises(ValueError, match="appears in both"):
        validate_source_entries(
            {"original": ["src/wiz8/Unit.cpp"], "unattributed": ["src/wiz8/Unit.cpp"]},
            tmp_path,
        )
