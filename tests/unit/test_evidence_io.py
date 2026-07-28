import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence.io import read_table, upsert_row
from wiz8decomp.evidence.schema import schema_for


def _write(path: Path, rows: list[dict[str, str]]) -> None:
    schema = schema_for(path.name)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=schema.columns)
        writer.writeheader()
        writer.writerows(rows)


def _boundary(address: str, **overrides: str) -> dict[str, str]:
    row = {
        "address": address,
        "size": "0x10",
        "symbol": "Function" + address,
        "owner": "wiz8",
        "confidence": "strong",
        "relocation_masked_sha256": "",
        "evidence": "reviewed",
    }
    row.update(overrides)
    return row


def test_checked_table_rejects_header_drift(tmp_path: Path) -> None:
    path = tmp_path / "wiz8-gameplay-boundaries.csv"
    path.write_text("address,symbol\n00401000,Entry\n", encoding="utf-8")

    with pytest.raises(ValueError, match="header mismatch"):
        read_table(path)


def test_checked_table_rejects_duplicate_identity(tmp_path: Path) -> None:
    path = tmp_path / "wiz8-gameplay-boundaries.csv"
    _write(path, [_boundary("00401000"), _boundary("00401000")])

    with pytest.raises(ValueError, match="duplicate identity"):
        read_table(path)


def test_upsert_is_atomic_sorted_and_monotonic(tmp_path: Path) -> None:
    path = tmp_path / "wiz8-gameplay-boundaries.csv"
    _write(path, [_boundary("00402000")])

    inserted = upsert_row(path, _boundary("00401000"))
    updated = upsert_row(
        path,
        _boundary("00401000", confidence="exact", relocation_masked_sha256="a" * 64),
    )

    rows = read_table(path).rows
    assert inserted["action"] == "inserted"
    assert updated["action"] == "updated"
    assert [row["address"] for row in rows] == ["00401000", "00402000"]
    assert rows[0]["confidence"] == "exact"
    assert rows[0]["relocation_masked_sha256"] == "a" * 64


def test_upsert_refuses_semantic_conflict_without_writing(tmp_path: Path) -> None:
    path = tmp_path / "wiz8-gameplay-boundaries.csv"
    _write(path, [_boundary("00401000", symbol="Entry")])
    before = path.read_bytes()

    with pytest.raises(ValueError, match="semantic evidence conflict"):
        upsert_row(path, _boundary("00401000", symbol="Other"))

    assert path.read_bytes() == before
