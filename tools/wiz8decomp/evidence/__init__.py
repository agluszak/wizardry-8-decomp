"""Typed, checked access to canonical reviewed evidence."""

from .io import EvidenceTable, read_table, upsert_row
from .validate import validate_repository

__all__ = [
    "EvidenceTable",
    "read_table",
    "upsert_row",
    "validate_repository",
]
