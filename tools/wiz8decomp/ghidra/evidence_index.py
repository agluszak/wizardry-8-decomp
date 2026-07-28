"""One typed in-memory index over the checked-in Wizardry evidence ledger."""

from __future__ import annotations

import csv
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
from typing import Any

from .reviewed_class_model import (
    ReviewedClass,
    ReviewedClassModel,
    ReviewedField,
    ReviewedVtable,
    ReviewedVtableSlot,
    load_reviewed_class_model,
)
from .reviewed_signatures import ReviewedSignature, load_reviewed_signatures


@dataclass(frozen=True)
class EvidenceIndex:
    class_model: ReviewedClassModel
    functions_by_address: dict[int, dict[str, str]]
    signatures_by_address: dict[int, ReviewedSignature]
    classes_by_name: dict[str, ReviewedClass]
    class_for_lifecycle_function: dict[int, ReviewedClass]
    vtables_by_id: dict[str, ReviewedVtable]
    vtables_by_class: dict[str, tuple[ReviewedVtable, ...]]
    slots_by_vtable: dict[str, tuple[ReviewedVtableSlot, ...]]
    fields_by_class: dict[str, tuple[ReviewedField, ...]]
    globals_by_address: dict[int, dict[str, str]]
    source_ownership: dict[int, str]
    imported_types: frozenset[str]


def _rows(path: Path) -> list[dict[str, str]]:
    if not path.is_file():
        return []
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


@lru_cache(maxsize=8)
def load_evidence_index(
    repo_dir: Path, evidence_program: str = "wiz8", binary_program: str | None = None
) -> EvidenceIndex:
    """Load each canonical source once for one command/process."""

    model = load_reviewed_class_model(repo_dir, evidence_program)
    reviewed = repo_dir / "evidence" / "reviewed" / evidence_program
    function_rows = [
        row for row in _rows(reviewed / "functions.csv") if row["program"] == evidence_program
    ]
    functions = {int(row["address"], 16): row for row in function_rows if row["address"]}
    signatures = {
        item.address: item for item in load_reviewed_signatures(repo_dir, evidence_program)
    }
    classes = {item.name: item for item in model.classes}
    lifecycle = {
        address: item
        for item in model.classes
        for address in (
            item.constructor,
            item.destructor,
            item.scalar_deleting_destructor,
        )
        if address is not None
    }
    vtables = {item.vtable_id: item for item in model.vtables}
    vtables_by_class = {
        name: tuple(sorted((item for item in model.vtables if item.class_name == name), key=lambda item: item.address))
        for name in classes
    }
    slots_by_vtable = {
        identifier: tuple(
            sorted((item for item in model.slots if item.vtable_id == identifier), key=lambda item: item.index)
        )
        for identifier in vtables
    }
    fields_by_class = {
        name: tuple(sorted((item for item in model.fields if item.class_name == name), key=lambda item: item.offset))
        for name in classes
    }
    globals_rows = _rows(repo_dir / "evidence" / "snapshots" / "globals" / "globals.csv")
    globals_by_address = {
        int(row["address"], 16): row
        for row in globals_rows
        if not binary_program or row.get("program") == binary_program
    }
    imported = {
        row["class_name"]
        for row in _rows(repo_dir / "evidence" / "observations" / "surrender" / "wiz8-sr-imports.csv")
        if row.get("class_name")
    }
    return EvidenceIndex(
        class_model=model,
        functions_by_address=functions,
        signatures_by_address=signatures,
        classes_by_name=classes,
        class_for_lifecycle_function=lifecycle,
        vtables_by_id=vtables,
        vtables_by_class=vtables_by_class,
        slots_by_vtable=slots_by_vtable,
        fields_by_class=fields_by_class,
        globals_by_address=globals_by_address,
        source_ownership={
            address: row.get("source_path", "") for address, row in functions.items()
        },
        imported_types=frozenset(imported),
    )


def reviewed_owner(index: EvidenceIndex, address: int) -> str | None:
    row: dict[str, Any] | None = index.functions_by_address.get(address)
    if row is None:
        return None
    current = row["current_name"]
    name = current if "::" in current else row["provisional_name"] or current
    return name.split("::", 1)[0] if "::" in name else None
