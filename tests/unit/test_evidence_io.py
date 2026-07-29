import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence.io import read_table, upsert_row
from wiz8decomp.evidence.schema import schema_for


def _claim(claim_id: str, **overrides: str) -> dict[str, str]:
    row = {
        "claim_id": claim_id,
        "program": "wiz8",
        "entity_kind": "function",
        "entity_key": "00401000",
        "predicate": "abi-note",
        "value": "observed",
        "origin": "original-binary",
        "authority": "reviewed",
        "confidence": "strong",
        "reference": "ghidra",
        "details": "external observation",
    }
    row.update(overrides)
    return row


def _write(path: Path, rows: list[dict[str, str]]) -> None:
    schema = schema_for(path.name)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=schema.columns)
        writer.writeheader()
        writer.writerows(rows)


def test_checked_table_rejects_header_drift(tmp_path: Path) -> None:
    path = tmp_path / "claims.csv"
    path.write_text("claim_id,value\na,one\n", encoding="utf-8")
    with pytest.raises(ValueError, match="header mismatch"):
        read_table(path)


def test_checked_table_rejects_duplicate_identity(tmp_path: Path) -> None:
    path = tmp_path / "claims.csv"
    _write(path, [_claim("a"), _claim("a")])
    with pytest.raises(ValueError, match="duplicate identity"):
        read_table(path)


def test_upsert_is_atomic_and_refuses_semantic_conflict(tmp_path: Path) -> None:
    path = tmp_path / "claims.csv"
    _write(path, [_claim("b")])
    assert upsert_row(path, _claim("a"))["action"] == "inserted"
    before = path.read_bytes()
    with pytest.raises(ValueError, match="semantic evidence conflict"):
        upsert_row(path, _claim("a", value="different"))
    assert path.read_bytes() == before
