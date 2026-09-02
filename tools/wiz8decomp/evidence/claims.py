from __future__ import annotations

from pathlib import Path

from ..config import Settings
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


def validate_claims_against_ghidra(settings: Settings, program: str = "wiz8") -> dict[str, int]:
    """Resolve only the claimed function entries against the live reviewed program."""

    from ..ghidra.query import validate_function_entries

    claims = load_claims(settings.repo_dir, program)
    path = settings.repo_dir / "evidence/reviewed" / program / "claims.csv"
    addresses = {
        parse_hex(claim["entity_key"], field="entity_key", path=path) or 0
        for claim in claims
        if claim["entity_kind"].strip() == "function"
    }
    audit = validate_function_entries(settings, addresses)
    missing = {int(value, 16) for value in audit["missing"]}
    if missing:
        unresolved = [
            claim["claim_id"]
            for claim in claims
            if claim["entity_kind"].strip() == "function"
            and (parse_hex(claim["entity_key"], field="entity_key", path=path) or 0) in missing
        ]
        raise ValueError("claims do not resolve to live Ghidra functions: " + ", ".join(unresolved))
    return {"function": sum(claim["entity_kind"].strip() == "function" for claim in claims)}
