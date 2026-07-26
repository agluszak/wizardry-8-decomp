from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path

ACCEPTED_CONFIDENCE = frozenset({"exact", "high", "strong"})


@dataclass(frozen=True)
class ReviewedClass:
    name: str
    size: int | None
    primary_vtable_id: str | None


@dataclass(frozen=True)
class ReviewedField:
    class_name: str
    offset: int
    size: int
    name: str
    data_type: str
    description: str
    evidence_id: str


@dataclass(frozen=True)
class ReviewedVtable:
    vtable_id: str
    class_name: str
    address: int
    subobject_offset: int | None
    kind: str
    slot_count: int | None
    evidence_id: str


@dataclass(frozen=True)
class ReviewedVtableSlot:
    vtable_id: str
    index: int
    target: int
    name: str
    evidence_id: str


@dataclass(frozen=True)
class ReviewedClassModel:
    classes: tuple[ReviewedClass, ...]
    fields: tuple[ReviewedField, ...]
    vtables: tuple[ReviewedVtable, ...]
    slots: tuple[ReviewedVtableSlot, ...]


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _hex(value: str, *, field: str, path: Path) -> int | None:
    value = value.strip()
    if not value:
        return None
    try:
        return int(value, 16)
    except ValueError as error:
        raise ValueError(f"{path}: invalid {field} value {value!r}") from error


def _accepted(row: dict[str, str], program: str) -> bool:
    return (
        row.get("program", "").strip() == program
        and row.get("confidence", "").strip() in ACCEPTED_CONFIDENCE
    )


def load_reviewed_class_model(repo_dir: Path, program: str) -> ReviewedClassModel:
    """Load and cross-validate the canonical reviewed class relationships."""

    directory = repo_dir / "evidence" / "reviewed" / program
    classes_path = directory / "classes.csv"
    fields_path = directory / "fields.csv"
    vtables_path = directory / "vtables.csv"
    slots_path = directory / "vtable-slots.csv"

    classes = tuple(
        ReviewedClass(
            name=row["class_name"].strip(),
            size=_hex(row["minimum_size"], field="minimum_size", path=classes_path),
            primary_vtable_id=row["primary_vtable_id"].strip() or None,
        )
        for row in _rows(classes_path)
        if _accepted(row, program)
    )
    if not classes:
        raise ValueError(f"{classes_path}: no accepted classes for {program}")
    classes_by_name = {item.name: item for item in classes}
    if len(classes_by_name) != len(classes):
        raise ValueError(f"{classes_path}: duplicate accepted class names")

    fields = tuple(
        ReviewedField(
            class_name=row["class_name"].strip(),
            offset=_hex(row["offset"], field="offset", path=fields_path) or 0,
            size=_hex(row["size"], field="size", path=fields_path) or 0,
            name=row["field_name"].strip(),
            data_type=row["data_type"].strip(),
            description=row["description"].strip(),
            evidence_id=row["evidence_id"].strip(),
        )
        for row in _rows(fields_path)
        if _accepted(row, program)
    )
    field_keys: set[tuple[str, int]] = set()
    last_end: dict[str, int] = {}
    for field in sorted(fields, key=lambda item: (item.class_name, item.offset)):
        owner = classes_by_name.get(field.class_name)
        if owner is None:
            raise ValueError(f"{fields_path}: unknown class {field.class_name}")
        if not field.name or field.size <= 0 or field.data_type not in {
            "bytes",
            "int32",
            "pointer",
        }:
            raise ValueError(f"{fields_path}: invalid field {field.class_name}+0x{field.offset:x}")
        key = (field.class_name, field.offset)
        if key in field_keys or field.offset < last_end.get(field.class_name, 0):
            raise ValueError(f"{fields_path}: overlapping field {field.class_name}+0x{field.offset:x}")
        if owner.size is None or field.offset + field.size > owner.size:
            raise ValueError(f"{fields_path}: field exceeds {field.class_name} size")
        if field.data_type == "pointer" and field.size != 4:
            raise ValueError(f"{fields_path}: reviewed x86 pointer field must be four bytes")
        if field.data_type == "int32" and field.size != 4:
            raise ValueError(f"{fields_path}: reviewed int32 field must be four bytes")
        field_keys.add(key)
        last_end[field.class_name] = field.offset + field.size

    vtables = tuple(
        ReviewedVtable(
            vtable_id=row["vtable_id"].strip(),
            class_name=row["class_name"].strip(),
            address=_hex(row["address"], field="address", path=vtables_path) or 0,
            subobject_offset=_hex(
                row["subobject_offset"], field="subobject_offset", path=vtables_path
            ),
            kind=row["kind"].strip(),
            slot_count=(int(row["slot_count"]) if row["slot_count"].strip() else None),
            evidence_id=row["evidence_id"].strip(),
        )
        for row in _rows(vtables_path)
        if _accepted(row, program)
    )
    vtables_by_id = {item.vtable_id: item for item in vtables}
    if len(vtables_by_id) != len(vtables):
        raise ValueError(f"{vtables_path}: duplicate accepted vtable IDs")
    if len({item.address for item in vtables}) != len(vtables):
        raise ValueError(f"{vtables_path}: duplicate accepted vtable addresses")
    for vtable in vtables:
        owner = classes_by_name.get(vtable.class_name)
        if owner is None or vtable.kind not in {"primary", "secondary"}:
            raise ValueError(f"{vtables_path}: invalid vtable {vtable.vtable_id}")
        if vtable.kind == "primary" and owner.primary_vtable_id != vtable.vtable_id:
            raise ValueError(f"{vtables_path}: {vtable.vtable_id} is not its class primary vtable")
        if vtable.kind == "secondary" and vtable.subobject_offset is None:
            # A known address without a recovered subobject offset remains useful,
            # but it must not be instantiated as an object member yet.
            continue
        if vtable.slot_count is not None and vtable.slot_count <= 0:
            raise ValueError(f"{vtables_path}: invalid slot count for {vtable.vtable_id}")

    slots = tuple(
        ReviewedVtableSlot(
            vtable_id=row["vtable_id"].strip(),
            index=int(row["slot_index"]),
            target=_hex(row["target"], field="target", path=slots_path) or 0,
            name=row["slot_name"].strip(),
            evidence_id=row["evidence_id"].strip(),
        )
        for row in _rows(slots_path)
        if _accepted(row, program)
    )
    slots_by_vtable: dict[str, list[ReviewedVtableSlot]] = {}
    for slot in slots:
        if slot.vtable_id not in vtables_by_id:
            raise ValueError(f"{slots_path}: unknown vtable {slot.vtable_id}")
        slots_by_vtable.setdefault(slot.vtable_id, []).append(slot)
    for vtable_id, grouped in slots_by_vtable.items():
        grouped.sort(key=lambda item: item.index)
        expected = vtables_by_id[vtable_id].slot_count
        if [item.index for item in grouped] != list(range(len(grouped))):
            raise ValueError(f"{slots_path}: non-contiguous slots for {vtable_id}")
        if expected is None or len(grouped) != expected:
            raise ValueError(f"{slots_path}: slot count disagrees for {vtable_id}")

    return ReviewedClassModel(classes, fields, vtables, slots)
