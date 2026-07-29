import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence.classes import load_reviewed_class_model
from wiz8decomp.evidence.schema import schema_for
from wiz8decomp.evidence.validate import (
    _validate_boundaries,
    _validate_function_catalogs,
    validate_source_entries,
)
from wiz8decomp.evidence_merge import EvidenceMergeConflict, stronger


def _write(path: Path, rows: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    schema = schema_for(path.name)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=schema.columns)
        writer.writeheader()
        writer.writerows(rows)


def _class_model(repo: Path, slots: list[dict[str, str]]) -> None:
    _write(
        repo / "evidence/reviewed/demo/class-provenance.csv",
        [
            {
                "program": "demo",
                "class_name": "Node",
                "confidence": "exact",
                "primary_vtable_id": "Node.primary",
            }
        ],
    )
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


def test_class_validator_rejects_dangling_vtable_id(tmp_path: Path) -> None:
    _class_model(tmp_path, [_slot("Missing.primary")])

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


def test_function_catalog_validator_rejects_invalid_provenance(tmp_path: Path) -> None:
    _write(
        tmp_path / "evidence/reviewed/demo/functions.csv",
        [
            {
                "program": "demo",
                "address": "00401000",
                "current_name": "Entry",
                "confidence": "strong",
                "name_origin": "official-demo",
                "authority": "source-backed",
            }
        ],
    )

    with pytest.raises(ValueError, match="authority 'source-backed' is not derivable"):
        _validate_function_catalogs(tmp_path)


def test_function_catalog_validator_rejects_duplicate_identity(tmp_path: Path) -> None:
    row = {
        "program": "demo",
        "address": "00401000",
        "current_name": "Entry",
        "confidence": "strong",
        "name_origin": "official-demo",
        "authority": "descriptive",
    }
    _write(tmp_path / "evidence/reviewed/demo/functions.csv", [row, row])

    with pytest.raises(ValueError, match="duplicate function identity"):
        _validate_function_catalogs(tmp_path)


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
