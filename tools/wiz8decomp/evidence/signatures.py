from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from .io import parse_hex, read_table

ACCEPTED_CONFIDENCE = frozenset({"exact", "high", "strong"})


@dataclass(frozen=True)
class ReviewedSignature:
    address: int
    calling_convention: str
    return_type: str
    parameters: tuple[tuple[str, str], ...]
    variadic: bool
    this_type: str | None
    confidence: str
    evidence_id: str
    previous_auto_signature: str


def load_reviewed_signatures(repo_dir: Path, program: str) -> tuple[ReviewedSignature, ...]:
    path = repo_dir / "evidence" / "reviewed" / program / "signatures.csv"
    signatures: list[ReviewedSignature] = []
    for row_number, row in enumerate(read_table(path, program=program).rows, start=2):
        confidence = row["confidence"].strip()
        if confidence not in ACCEPTED_CONFIDENCE:
            continue
        try:
            decoded = json.loads(row["parameters_json"])
            parameters = tuple((str(name), str(data_type)) for name, data_type in decoded)
        except (TypeError, ValueError) as error:
            raise ValueError(f"{path}:{row_number}: invalid parameters_json") from error
        signature = ReviewedSignature(
            address=parse_hex(row["address"], field="address", path=path) or 0,
            calling_convention=row["calling_convention"].strip(),
            return_type=row["return_type"].strip(),
            parameters=parameters,
            variadic=row["variadic"].strip().lower() == "true",
            this_type=row["this_type"].strip() or None,
            confidence=confidence,
            evidence_id=row["evidence_id"].strip(),
            previous_auto_signature=row["previous_auto_signature"].strip(),
        )
        if not signature.calling_convention or not signature.return_type:
            raise ValueError(f"{path}:{row_number}: incomplete signature")
        if not signature.evidence_id.startswith(f"signatures:{program}:"):
            raise ValueError(f"{path}:{row_number}: unstable evidence_id")
        if signature.variadic and signature.calling_convention != "__cdecl":
            raise ValueError(f"{path}:{row_number}: variadic function must use __cdecl")
        signatures.append(signature)
    signatures.sort(key=lambda item: item.address)
    if len({item.address for item in signatures}) != len(signatures):
        raise ValueError(f"{path}: duplicate accepted signature addresses")
    if len({item.evidence_id for item in signatures}) != len(signatures):
        raise ValueError(f"{path}: duplicate signature evidence IDs")
    return tuple(signatures)
