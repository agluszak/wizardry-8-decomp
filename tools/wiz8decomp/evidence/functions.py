from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from ..provenance import ProvenanceError, validate_provenance
from .io import parse_hex, read_table

ACCEPTED_CONFIDENCE = frozenset({"exact", "high", "strong"})


@dataclass(frozen=True)
class FunctionIdentity:
    address: int
    size: int | None
    name: str
    identity_id: str
    owner: str
    confidence: str
    evidence: str
    name_origin: tuple[str, ...]
    authority: str
    source_unit: str | None
    evidence_ids: tuple[str, ...]
    aliases: tuple[str, ...] = ()


def _load_evidence_ids(path: Path) -> dict[tuple[str, int], tuple[str, ...]]:
    evidence_path = path.parent / "function-evidence.csv"
    if not evidence_path.is_file():
        return {}
    grouped: dict[tuple[str, int], list[str]] = {}
    for row_number, row in enumerate(read_table(evidence_path).rows, start=2):
        evidence_id = row["evidence_id"].strip()
        if not evidence_id:
            raise ValueError(f"{evidence_path}:{row_number}: missing evidence_id")
        address = parse_hex(row["address"], field="address", path=evidence_path) or 0
        key = (row["program"].strip(), address)
        grouped.setdefault(key, []).append(evidence_id)
    return {key: tuple(sorted(values)) for key, values in grouped.items()}


def load_function_identities(path: Path, *, program: str | None = None) -> list[FunctionIdentity]:
    identities: list[FunctionIdentity] = []
    evidence_ids = _load_evidence_ids(path)
    for row_number, row in enumerate(read_table(path, program=program).rows, start=2):
        name = row["provisional_name"].strip()
        confidence = row["confidence"].strip()
        if not name or confidence not in ACCEPTED_CONFIDENCE:
            continue
        try:
            origins, authority = validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise ValueError(f"{path}:{row_number}: {error}") from error
        aliases = tuple(alias.strip() for alias in row["aliases"].split("|") if alias.strip())
        if name in aliases:
            raise ValueError(f"{path}:{row_number}: {name} is listed as its own alias")
        program_name = row["program"].strip()
        address = parse_hex(row["address"], field="address", path=path) or 0
        size_token = row["size"].strip()
        identities.append(
            FunctionIdentity(
                address=address,
                size=int(size_token, 0) if size_token else None,
                name=name,
                identity_id=f"functions:{program_name}:{address:08x}",
                owner=row["owner"].strip(),
                confidence=confidence,
                evidence=row["evidence"].strip(),
                name_origin=origins,
                authority=authority,
                source_unit=row["source_path"].strip() or None,
                evidence_ids=evidence_ids.get((program_name, address), ()),
                aliases=aliases,
            )
        )
    identities.sort(key=lambda identity: identity.address)
    if len({identity.address for identity in identities}) != len(identities):
        raise ValueError(f"{path}: duplicate accepted function addresses")
    return identities
