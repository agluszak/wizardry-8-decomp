from pathlib import Path

import pytest
from wiz8decomp.evidence.claims import validate_claims_against_documents


def _documents(
    addresses: set[str], types: list[dict[str, object]] | None = None
) -> dict[str, dict[str, object]]:
    return {
        "functions": {"functions": [{"entry": address} for address in sorted(addresses)]},
        "types": {"types": types or []},
        "vtables": {"vtables": []},
    }


def test_generated_index_validation_rejects_an_unresolved_claim(tmp_path: Path) -> None:
    directory = tmp_path / "evidence/reviewed/wiz8"
    directory.mkdir(parents=True)
    (directory / "claims.csv").write_text(
        "claim_id,program,entity_kind,entity_key,predicate,value,origin,authority,"
        "confidence,reference,details\n"
        "claim:wiz8:00401000,wiz8,function,00401000,accepted-identity,Entry,"
        "descriptive,descriptive,strong,,fixture\n",
        encoding="utf-8",
    )

    with pytest.raises(ValueError, match="does not resolve in the Ghidra function index"):
        validate_claims_against_documents(tmp_path, _documents(set()))


def test_generated_index_validation_accepts_a_resolved_claim(tmp_path: Path) -> None:
    directory = tmp_path / "evidence/reviewed/wiz8"
    directory.mkdir(parents=True)
    (directory / "claims.csv").write_text(
        "claim_id,program,entity_kind,entity_key,predicate,value,origin,authority,"
        "confidence,reference,details\n"
        "claim:wiz8:00401000,wiz8,function,00401000,accepted-identity,Entry,"
        "descriptive,descriptive,strong,,fixture\n",
        encoding="utf-8",
    )

    assert validate_claims_against_documents(tmp_path, _documents({"00401000"})) == {
        "function": 1,
    }
