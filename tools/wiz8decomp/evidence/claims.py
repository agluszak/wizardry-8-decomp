from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path
from typing import Any

from ..provenance import ProvenanceError, validate_provenance
from .io import parse_hex, read_table

ENTITY_KINDS = frozenset({"function"})
CONFIDENCE = frozenset({"", "candidate", "structurally-strong", "strong", "high", "exact"})


def load_claims(repository: Path, program: str = "wiz8") -> tuple[dict[str, str], ...]:
    path = repository / "evidence/reviewed" / program / "claims.csv"
    return tuple(row for row in read_table(path).rows if row["program"].strip() == program)


def validate_claim_rows(
    repository: Path, function_addresses: set[int], program: str = "wiz8"
) -> int:
    path = repository / "evidence/reviewed" / program / "claims.csv"
    claims = load_claims(repository, program)
    for line, claim in enumerate(claims, start=2):
        kind = claim["entity_kind"].strip()
        if kind not in ENTITY_KINDS:
            raise ValueError(f"{path}:{line}: unsupported entity kind {kind!r}")
        if not claim["predicate"].strip() or not claim["origin"].strip():
            raise ValueError(f"{path}:{line}: claims require predicate and origin")
        confidence = claim["confidence"].strip()
        if confidence not in CONFIDENCE:
            raise ValueError(f"{path}:{line}: unsupported confidence {confidence!r}")
        authority = claim["authority"].strip()
        if authority:
            try:
                validate_provenance(claim["origin"], authority)
            except ProvenanceError as error:
                raise ValueError(f"{path}:{line}: {error}") from error
        if kind == "function":
            address = parse_hex(claim["entity_key"], field="entity_key", path=path) or 0
            if address not in function_addresses:
                raise ValueError(
                    f"{path}:{line}: function claim does not resolve to the entity ledger: "
                    f"{address:08x}"
                )
    return len(claims)


def validate_claims_against_documents(
    repository: Path, documents: Mapping[str, Mapping[str, Any]], program: str = "wiz8"
) -> dict[str, int]:
    functions = {
        str(row["entry"]).lower().removeprefix("0x").zfill(8)
        for row in documents["functions"]["functions"]
    }
    entities = {"function": functions}
    counts = {kind: 0 for kind in sorted(ENTITY_KINDS)}
    for claim in load_claims(repository, program):
        kind = claim["entity_kind"].strip()
        key = claim["entity_key"].strip()
        if kind not in entities:
            raise ValueError(f"claim {claim['claim_id']} uses unsupported entity kind: {kind}")
        normalized = key.lower().removeprefix("0x").zfill(8)
        if normalized not in entities[kind]:
            raise ValueError(
                f"claim {claim['claim_id']} does not resolve in the Ghidra {kind} index: {key}"
            )
        counts[kind] += 1
    return counts


def validate_claims_against_index(repository: Path, program: str = "wiz8") -> dict[str, int]:
    import json

    directory = repository / "build/ghidra-index"
    documents = {
        name: json.loads((directory / f"{name}.json").read_text(encoding="utf-8"))
        for name in ("functions", "types", "vtables")
    }
    return validate_claims_against_documents(repository, documents, program)
