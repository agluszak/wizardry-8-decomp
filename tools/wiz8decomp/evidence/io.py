from __future__ import annotations

import csv
import os
import tempfile
from collections.abc import Iterable
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


_CONFIDENCE_RANK = {
    "": 0,
    "not-built": 0,
    "structurally-strong": 1,
    "strong": 2,
    "high": 3,
    "exact": 4,
}


def merge_monotonic(left: dict[str, str], right: dict[str, str]) -> dict[str, str]:
    merged: dict[str, str] = {}
    conflicts: list[str] = []
    for field, old in left.items():
        new = right[field]
        if field == "confidence":
            merged[field] = (
                new
                if _CONFIDENCE_RANK.get(new, -1) > _CONFIDENCE_RANK.get(old, -1)
                else old
            )
        elif not old:
            merged[field] = new
        elif not new or old == new:
            merged[field] = old
        else:
            conflicts.append(f"{field}: {old!r} != {new!r}")
    if conflicts:
        raise ValueError("semantic evidence conflict: " + "; ".join(conflicts))
    return merged


def _sort_key(row: dict[str, str], schema: TableSchema) -> tuple[tuple[int, int | str], ...]:
    key: list[tuple[int, int | str]] = []
    for column in schema.identity:
        value = row[column].strip()
        if column in {"address", "offset", "slot_index"}:
            try:
                key.append((0, int(value, 0 if value.lower().startswith("0x") else 16)))
                continue
            except ValueError:
                pass
        key.append((1, value.casefold()))
    return tuple(key)


def _write_atomic(path: Path, schema: TableSchema, rows: Iterable[dict[str, str]]) -> None:
    handle, temporary = tempfile.mkstemp(dir=str(path.parent), suffix=".csv")
    try:
        with os.fdopen(handle, "w", newline="", encoding="utf-8") as stream:
            writer = csv.DictWriter(stream, fieldnames=schema.columns, lineterminator="\n")
            writer.writeheader()
            writer.writerows(rows)
        os.replace(temporary, path)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def upsert_row(path: Path, row: dict[str, str]) -> dict[str, object]:
    table = read_table(path)
    missing = set(table.schema.columns) - set(row)
    extra = set(row) - set(table.schema.columns)
    if missing or extra:
        raise ValueError(f"{path}: row columns differ; missing={sorted(missing)}, extra={sorted(extra)}")
    normalized = {column: str(row[column]) for column in table.schema.columns}
    identity = identity_of(normalized, table.schema, path=path)
    by_identity = {
        identity_of(existing, table.schema, path=path): existing for existing in table.rows
    }
    action = "inserted"
    if identity in by_identity:
        by_identity[identity] = merge_monotonic(by_identity[identity], normalized)
        action = "updated"
    else:
        by_identity[identity] = normalized
    rows = sorted(by_identity.values(), key=lambda item: _sort_key(item, table.schema))
    _write_atomic(path, table.schema, rows)
    return {"path": str(path), "identity": list(identity), "action": action, "rows": len(rows)}
