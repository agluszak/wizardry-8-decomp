from __future__ import annotations

import csv
import string
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from ..provenance import ProvenanceError, validate_provenance
from .boundaries import load_boundary_rows
from .classes import load_reviewed_class_model
from .functions import load_function_identities
from .signatures import load_reviewed_signatures

BOUNDARY_CONFIDENCE = frozenset({"exact", "structurally-strong", "provisional"})


def validate_unique(
    rows: Iterable[dict[str, str]], columns: tuple[str, ...], *, label: str
) -> None:
    seen: set[tuple[str, ...]] = set()
    for row in rows:
        key = tuple(row[column].strip() for column in columns)
        if key in seen:
            raise ValueError(f"{label}: duplicate identity {key!r}")
        seen.add(key)


def validate_exact_digests(rows: Iterable[dict[str, str]], *, label: str) -> None:
    for row in rows:
        confidence = row["confidence"].strip()
        if confidence not in BOUNDARY_CONFIDENCE:
            raise ValueError(f"{label}: invalid confidence {confidence!r}")
        digest = row["relocation_masked_sha256"].strip()
        if confidence == "exact" and (
            len(digest) != 64 or any(character not in string.hexdigits for character in digest)
        ):
            raise ValueError(f"{label}: exact row {row['address']} has no valid digest")
        if confidence != "exact" and digest:
            raise ValueError(f"{label}: non-exact row {row['address']} carries an exact digest")


def validate_field_rows(
    class_sizes: dict[str, int], rows: Iterable[dict[str, str]], *, label: str
) -> None:
    end_by_class: dict[str, int] = {}
    for row in sorted(rows, key=lambda item: (item["class_name"], int(item["offset"], 0))):
        class_name = row["class_name"]
        if class_name not in class_sizes:
            raise ValueError(f"{label}: dangling class {class_name}")
        offset = int(row["offset"], 0)
        size = int(row["size"], 0)
        if size <= 0 or offset < end_by_class.get(class_name, 0):
            raise ValueError(f"{label}: overlapping field {class_name}+0x{offset:x}")
        if offset + size > class_sizes[class_name]:
            raise ValueError(f"{label}: field exceeds {class_name} size")
        end_by_class[class_name] = offset + size


def validate_vtable_rows(
    vtables: Iterable[dict[str, str]], slots: Iterable[dict[str, str]], *, label: str
) -> None:
    counts = {row["vtable_id"]: int(row["slot_count"]) for row in vtables}
    by_vtable: dict[str, list[int]] = {}
    for slot in slots:
        vtable_id = slot["vtable_id"]
        if vtable_id not in counts:
            raise ValueError(f"{label}: dangling vtable ID {vtable_id}")
        by_vtable.setdefault(vtable_id, []).append(int(slot["slot_index"]))
    for vtable_id, indices in by_vtable.items():
        if sorted(indices) != list(range(counts[vtable_id])):
            raise ValueError(f"{label}: non-contiguous slots for {vtable_id}")


def validate_provenance_rows(rows: Iterable[dict[str, str]], *, label: str) -> None:
    for row in rows:
        try:
            validate_provenance(row["name_origin"], row["authority"])
        except ProvenanceError as error:
            raise ValueError(f"{label}: {error}") from error


def validate_source_entries(entries: Iterable[str], *, label: str) -> None:
    seen: set[str] = set()
    for entry in entries:
        normalized = entry.strip().replace("\\", "/")
        if not normalized:
            raise ValueError(f"{label}: empty source entry")
        if normalized in seen:
            raise ValueError(f"{label}: duplicate source entry {normalized}")
        seen.add(normalized)


def _raw_rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames:
            raise ValueError(f"{path}: missing header")
        rows = list(reader)
    for line, row in enumerate(rows, start=2):
        if row.get(None) is not None or len(row) != len(reader.fieldnames):
            raise ValueError(f"{path}:{line}: row does not match its header")
    return rows


def _validate_census_agreement(repo: Path) -> dict[str, int]:
    reviewed_dir = repo / "evidence/reviewed/wiz8"
    reviewed_vtables = _raw_rows(reviewed_dir / "vtables.csv")
    reviewed_slots = _raw_rows(reviewed_dir / "vtable-slots.csv")
    census_vtables = [
        row
        for row in _raw_rows(repo / "evidence/snapshots/polymorphism/vtables.csv")
        if "--gog-base--" in row["program"]
    ]
    census_slots = [
        row
        for row in _raw_rows(repo / "evidence/snapshots/polymorphism/slots.csv")
        if "--gog-base--" in row["program"]
    ]
    census_by_address = {row["address"]: row for row in census_vtables}
    address_by_id = {row["vtable_id"]: row["address"] for row in reviewed_vtables}
    for vtable in reviewed_vtables:
        observed = census_by_address.get(vtable["address"])
        if observed is None or observed["kind"] != "vftable":
            raise ValueError(
                f"{reviewed_dir / 'vtables.csv'}: {vtable['vtable_id']} is not an observed vftable"
            )
    observed_slots = {
        (row["vtable"], int(row["slot_index"])): row["target"] for row in census_slots
    }
    checked = 0
    for slot in reviewed_slots:
        expected = observed_slots.get((address_by_id[slot["vtable_id"]], int(slot["slot_index"])))
        if expected is None:
            continue
        checked += 1
        if slot["target"] != expected:
            raise ValueError(
                f"{reviewed_dir / 'vtable-slots.csv'}: {slot['vtable_id']} "
                f"slot {slot['slot_index']} disagrees with observation"
            )
    return {"reviewed_vtables": len(reviewed_vtables), "observed_slots_checked": checked}


def validate_repository(repo: Path) -> dict[str, Any]:
    repo = repo.resolve()
    tracked_csvs = sorted([*(repo / "evidence").rglob("*.csv"), *(repo / "config").rglob("*.csv")])
    for path in tracked_csvs:
        _raw_rows(path)

    reviewed = repo / "evidence/reviewed/wiz8"
    functions_path = reviewed / "functions.csv"
    functions = load_function_identities(functions_path, program="wiz8")
    validate_provenance_rows(_raw_rows(functions_path), label=str(functions_path))
    model = load_reviewed_class_model(repo, "wiz8")
    signatures = load_reviewed_signatures(repo, "wiz8")
    boundary_path = repo / "config/reccmp/wiz8-gameplay-boundaries.csv"
    boundaries = load_boundary_rows(boundary_path)
    validate_exact_digests(boundaries, label=str(boundary_path))
    census = _validate_census_agreement(repo)

    return {
        "ok": True,
        "tracked_csvs": len(tracked_csvs),
        "functions": len(functions),
        "classes": len(model.classes),
        "fields": len(model.fields),
        "vtables": len(model.vtables),
        "vtable_slots": len(model.slots),
        "signatures": len(signatures),
        "boundaries": len(boundaries),
        **census,
    }
