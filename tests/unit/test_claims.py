import csv
from pathlib import Path

import pytest
from wiz8decomp.evidence.claims import (
    load_claims,
    validate_claim_rows,
    validate_claims_against_documents,
)


def _documents(
    addresses: set[str], types: list[dict[str, object]] | None = None
) -> dict[str, dict[str, object]]:
    return {
        "functions": {"functions": [{"entry": address} for address in sorted(addresses)]},
        "types": {"types": types or []},
        "vtables": {"vtables": []},
    }


def test_reviewed_claims_resolve_to_the_function_entity_ledger() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/function-provenance.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        addresses = {int(row["address"], 16) for row in csv.DictReader(stream)}

    assert validate_claim_rows(repository, addresses) == len(load_claims(repository))


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
        "type": 0,
        "vtable": 0,
    }


def test_generated_index_validation_checks_claimed_field_identity(tmp_path: Path) -> None:
    directory = tmp_path / "evidence/reviewed/wiz8"
    directory.mkdir(parents=True)
    (directory / "claims.csv").write_text(
        "claim_id,program,entity_kind,entity_key,predicate,value,origin,authority,"
        "confidence,reference,details\n"
        "field:wiz8:Node:4,wiz8,type,/wiz8/classes/Node,field-identity,"
        '"{""field"":""next"",""length"":4,""offset"":4}",'
        "descriptive,descriptive,exact,fixture,fixture\n",
        encoding="utf-8",
    )
    types = [
        {
            "path": "/wiz8/classes/Node",
            "components": [{"offset": 0, "length": 4, "field": "next"}],
        }
    ]

    with pytest.raises(ValueError, match="does not resolve to a matching Ghidra field"):
        validate_claims_against_documents(tmp_path, _documents(set(), types))
