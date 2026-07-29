from pathlib import Path

import pytest
from wiz8decomp.evidence.validate import _validate_function_catalogs, validate_source_entries
from wiz8decomp.evidence_merge import EvidenceMergeConflict, stronger


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


def _write_function_catalog(tmp_path: Path, rows: list[dict[str, str]]) -> None:
    import csv

    path = tmp_path / "evidence/reviewed/demo/functions.csv"
    path.parent.mkdir(parents=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=rows[0])
        writer.writeheader()
        writer.writerows(rows)


def test_function_catalog_validator_rejects_invalid_provenance(tmp_path: Path) -> None:
    _write_function_catalog(
        tmp_path,
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
    _write_function_catalog(tmp_path, [row, row])

    with pytest.raises(ValueError, match="duplicate function identity"):
        _validate_function_catalogs(tmp_path)
