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
            "program", "address", "size", "current_name", "provisional_name", "owner",
            "confidence", "name_origin", "authority", "aliases", "fid_variants", "evidence",
            "source_path", "source_line", "relocation_masked_sha256",
        ),
        ("program", "address"),
    ),
    "function-evidence.csv": TableSchema(
        ("evidence_id", "program", "address", "origin", "kind", "reference", "details"),
        ("evidence_id",),
        None,
    ),
    "classes.csv": TableSchema(
        (
            "program", "class_name", "confidence", "primary_vtable_id", "constructor",
            "destructor", "scalar_deleting_destructor", "minimum_size", "base_classes",
            "base_name_origin", "source_path", "evidence", "layout_proof",
        ),
        ("program", "class_name"),
    ),
    "fields.csv": TableSchema(
        (
            "program", "class_name", "offset", "size", "field_name", "data_type", "pointee",
            "confidence", "evidence_id", "description",
        ),
        ("program", "class_name", "offset"),
    ),
    "vtables.csv": TableSchema(
        (
            "program", "vtable_id", "class_name", "address", "subobject_offset", "kind",
            "slot_count", "confidence", "evidence_id",
        ),
        ("program", "vtable_id"),
    ),
    "vtable-slots.csv": TableSchema(
        ("program", "vtable_id", "slot_index", "target", "slot_name", "confidence", "evidence_id"),
        ("program", "vtable_id", "slot_index"),
    ),
    "signatures.csv": TableSchema(
        (
            "program", "address", "calling_convention", "return_type", "parameters_json",
            "variadic", "this_type", "confidence", "evidence_id", "previous_auto_signature",
            "evidence",
        ),
        ("program", "address"),
    ),
    "wiz8-gameplay-boundaries.csv": TableSchema(
        ("address", "size", "symbol", "owner", "confidence", "relocation_masked_sha256", "evidence"),
        ("address",),
        None,
    ),
}


def schema_for(name: str) -> TableSchema:
    try:
        return TABLE_SCHEMAS[name]
    except KeyError as error:
        raise ValueError(f"no canonical evidence schema is registered for {name}") from error
