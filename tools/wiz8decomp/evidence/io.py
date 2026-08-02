from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path

from .schema import TableSchema, schema_for


@dataclass(frozen=True)
class EvidenceTable:
    path: Path
    schema: TableSchema
    rows: tuple[dict[str, str], ...]


def parse_hex(value: str, *, field: str, path: Path, allow_empty: bool = False) -> int | None:
    token = value.strip()
    if not token and allow_empty:
        return None
    try:
        return int(token, 16)
    except ValueError as error:
        raise ValueError(f"{path}: invalid {field} value {value!r}") from error


def identity_of(row: dict[str, str], schema: TableSchema, *, path: Path) -> tuple[str, ...]:
    identity = tuple(row[column].strip() for column in schema.identity)
    if any(not value for value in identity):
        raise ValueError(f"{path}: empty canonical identity {schema.identity}")
    return identity


def read_table(
    path: Path, *, program: str | None = None, schema_name: str | None = None
) -> EvidenceTable:
    schema = schema_for(schema_name or path.name)
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        actual = tuple(reader.fieldnames or ())
        if actual != schema.columns:
            raise ValueError(
                f"{path}: header mismatch; expected {schema.columns!r}, got {actual!r}"
            )
        rows = tuple(dict(row) for row in reader)
    selected = tuple(
        row
        for row in rows
        if program is None
        or schema.program_column is None
        or row[schema.program_column].strip() == program
    )
    seen: dict[tuple[str, ...], int] = {}
    for line, row in enumerate(selected, start=2):
        identity = identity_of(row, schema, path=path)
        if identity in seen:
            raise ValueError(
                f"{path}: duplicate identity {identity!r} at lines {seen[identity]} and {line}"
            )
        seen[identity] = line
    return EvidenceTable(path=path, schema=schema, rows=selected)
