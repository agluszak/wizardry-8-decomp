"""Typed, checked access to canonical reviewed evidence."""

from .io import EvidenceTable, read_table
from .validate import validate_repository

__all__ = [
    "EvidenceTable",
    "read_table",
    "validate_repository",
]
