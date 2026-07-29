from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class TableSchema:
    columns: tuple[str, ...]
    identity: tuple[str, ...]
    program_column: str | None = "program"


TABLE_SCHEMAS: dict[str, TableSchema] = {
    "functions.csv": TableSchema(
        (
            "program",
            "address",
            "size",
            "current_name",
            "provisional_name",
            "owner",
            "confidence",
            "name_origin",
            "authority",
            "aliases",
            "fid_variants",
            "evidence",
            "source_path",
            "source_line",
            "relocation_masked_sha256",
        ),
        ("program", "address"),
    ),
    "claims.csv": TableSchema(
        (
            "claim_id",
            "program",
            "entity_kind",
            "entity_key",
            "predicate",
            "value",
            "origin",
            "authority",
            "confidence",
            "reference",
            "details",
        ),
        ("claim_id",),
        None,
    ),
}


def schema_for(name: str) -> TableSchema:
    try:
        return TABLE_SCHEMAS[name]
    except KeyError as error:
        raise ValueError(f"no canonical evidence schema is registered for {name}") from error
