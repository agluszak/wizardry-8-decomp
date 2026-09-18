"""Type confirmed MSVC vbtables and retarget class ``vbptr`` fields.

A vbtable is a run of signed displacements. Entry 0 is the displacement from the
vbptr back to the owning subobject (not generally ``-4`` versus the complete
object). Later entries locate virtual bases from that vbptr.

Structures live under ``/wiz8/vbtables``. A table is attached to a source class
only when a ctor/dtor of that class also installs a source-index vftable.
Construction-phase vbtables are listed but never retarget the class ``vbptr``.
Existing ``VBasePtr`` / ``o_*`` PointerTypedefs at ``vbptr + displacement`` may
receive a ComponentOffset; this pass never invents those typedefs.
"""

from __future__ import annotations

import re
from collections import Counter, defaultdict
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .class_binding import _sanitize_class_parts
from .source_index import SourceIndex, load_source_index
from .vftable_typing import (
    _class_marked_tables,
    _is_constructor_name,
    _is_destructor_name,
    _lookup_source_class,
    _simple_name,
    lifecycle_class_name,
    load_enriched_msvc_census,
    unique_object_offset,
    unique_receiver_offset,
)

_SCHEMA = "wiz8.vbtable-typing-v1"
_CATEGORY = "/wiz8/vbtables"
_INCOMING_RECEIVERS = frozenset({"incoming-ecx", "incoming-ecx-plus-dynamic"})
_VBPTR_NAMES = frozenset({"vbptr", "vbtable", "__vbtable"})
_VBASEPTR_NAME = re.compile(r"(?:^o_|VBasePtr)", re.IGNORECASE)


def _vbtable_paths(qualified: str, base_class: str | None = None) -> tuple[str, str, str]:
    """Return ``(category, structure_name, structure_path)``."""

    parent_parts, leaf = _sanitize_class_parts(qualified)
    if parent_parts:
        category = f"{_CATEGORY}/{'/'.join(parent_parts)}"
    else:
        category = _CATEGORY
    if base_class:
        structure_name = f"{leaf}_vbtable_for_{_simple_name(base_class)}"
    else:
        structure_name = f"{leaf}_vbtable"
    return category, structure_name, f"{category}/{structure_name}"


def _vbtable_entries(row: Mapping[str, Any]) -> list[int]:
    entries = []
    for item in row.get("entries") or ():
        if isinstance(item, Mapping) and isinstance(item.get("displacement"), int):
            entries.append(int(item["displacement"]))
        elif isinstance(item, int):
            entries.append(item)
    return entries


def _construction_vbtable_addresses(families: Sequence[Mapping[str, Any]]) -> set[int]:
    construction: set[int] = set()
    for family in families:
        name = str(family.get("function_source_name") or "")
        transitions = [
            row for row in family.get("transitions") or () if row.get("kind") == "vbtable"
        ]
        if len(transitions) < 2:
            continue
        if _is_constructor_name(name):
            rows = transitions[:-1]
        elif _is_destructor_name(name):
            rows = transitions[1:]
        else:
            continue
        construction.update(int(row["table"], 0) for row in rows)
    return construction


def vbtable_class_attachments(
    *,
    vbtables: Sequence[Mapping[str, Any]],
    vftables: Sequence[Mapping[str, Any]],
    families: Sequence[Mapping[str, Any]],
    classes: Sequence[Any],
) -> list[dict[str, Any]]:
    """Attach confirmed vbtables to classes whose lifecycle also writes a marked vftable."""

    events_by_function: dict[str, list[tuple[str, Mapping[str, Any], Mapping[str, Any]]]] = (
        defaultdict(list)
    )
    for kind, tables in (("vftable", vftables), ("vbtable", vbtables)):
        for table in tables:
            for write in table.get("writes") or ():
                if write.get("receiver_provenance") not in _INCOMING_RECEIVERS:
                    continue
                function = write.get("function")
                if not function:
                    continue
                events_by_function[str(function)].append((kind, table, write))

    construction = _construction_vbtable_addresses(families)
    chosen: dict[int, dict[str, Any]] = {}
    conflicts: set[int] = set()
    for events in events_by_function.values():
        name = next(
            (
                str(write.get("function_source_name"))
                for _kind, _table, write in events
                if write.get("function_source_name")
            ),
            "",
        )
        owner = lifecycle_class_name(name)
        if owner is None:
            continue
        record = _lookup_source_class(classes, owner)
        if record is None:
            continue
        marked = _class_marked_tables(record)
        if not any(
            kind == "vftable" and int(table["address"], 0) in marked
            for kind, table, _write in events
        ):
            continue
        for kind, table, _write in events:
            if kind != "vbtable":
                continue
            address = int(table["address"], 0)
            writes = table.get("writes") or ()
            role = "construction" if address in construction else "vbtable"
            vbptr_offset = unique_object_offset(writes)
            if vbptr_offset is None:
                vbptr_offset = unique_receiver_offset(writes)
            attachment = {
                "class": record.qualified_name,
                "address": address,
                "base_class": None,
                "role": role,
                "vbptr_offset": vbptr_offset,
                "entries": _vbtable_entries(table),
            }
            existing = chosen.get(address)
            if existing is None:
                chosen[address] = attachment
                continue
            if existing["class"] != attachment["class"]:
                conflicts.add(address)
            elif existing["role"] != "construction" and role == "construction":
                existing["role"] = "construction"
    return [row for addr, row in sorted(chosen.items()) if addr not in conflicts]


def collect_vbtable_typing_plan(
    repository: Path,
    program: Any,
    *,
    work_dir: Path | None = None,
    target: str = "WIZ8",
    class_names: Sequence[str] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Plan vbtable Structures for census tables attached to source classes."""

    index = SourceIndex.from_dict(load_source_index(repository))
    wanted = None
    if class_names is not None:
        wanted = {_simple_name(name) for name in class_names} | set(class_names)
    classes = [
        record
        for record in index.classes
        if (record.vtable_address is not None or record.base_vtables)
        and (record.target is None or record.target.upper() == target.upper())
    ]
    if wanted is not None:
        classes = [
            record
            for record in classes
            if record.qualified_name in wanted or _simple_name(record.qualified_name) in wanted
        ]

    if work_dir is None:
        return {"schema": _SCHEMA, "target": target, "counts": {}, "actionable": 0, "vbtables": []}
    try:
        census = load_enriched_msvc_census(repository, work_dir)
    except Exception:  # noqa: BLE001 — missing binary / scan failure
        census = {}
    attachments = vbtable_class_attachments(
        vbtables=census.get("vbtables") or [],
        vftables=census.get("vftables") or [],
        families=(census.get("analysis") or {}).get("construction_families") or [],
        classes=classes,
    )

    manager = program.getDataTypeManager()
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    for attachment in attachments:
        entries = list(attachment.get("entries") or [])
        if len(entries) < 2:
            counts["no-entries"] += 1
            continue
        category, _structure_name, path = _vbtable_paths(
            str(attachment["class"]), base_class=attachment.get("base_class")
        )
        if attachment.get("role") == "construction":
            path = f"{path}_ctor"
        address = int(attachment["address"])
        existing = manager.getDataType(path)
        data = listing.getDataAt(space.getAddress(address))
        current_type_path = str(data.getDataType().getPathName()) if data is not None else None
        existing_count = (
            int(existing.getNumComponents())
            if existing is not None and hasattr(existing, "getNumComponents")
            else None
        )
        if existing is not None and existing_count == len(entries) and current_type_path == path:
            action = "agree"
        elif existing is not None and existing_count == len(entries):
            action = "apply-existing"
        elif existing is not None:
            action = "upgrade-definitions"
        else:
            action = "create-and-apply"
        counts[action] += 1
        if action == "agree":
            continue
        rows.append(
            {
                "class": attachment["class"],
                "base_class": attachment.get("base_class"),
                "role": attachment.get("role") or "vbtable",
                "address": f"0x{address:08x}",
                "vbtable": path,
                "vbtable_category": category,
                "vbptr_offset": attachment.get("vbptr_offset"),
                "entries": [
                    {"index": index, "displacement": displacement}
                    for index, displacement in enumerate(entries)
                ],
                "current_type": current_type_path,
                "action": action,
            }
        )
        if limit is not None and len(rows) >= limit:
            break
    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": sum(
            counts[key]
            for key in ("create-and-apply", "apply-existing", "upgrade-definitions")
            if key in counts
        ),
        "vbtables": rows,
    }


def _looks_like_vbaseptr(component: Any) -> bool:
    field = str(component.getFieldName() or "")
    data_type = component.getDataType() if hasattr(component, "getDataType") else None
    type_name = (
        str(data_type.getName()) if data_type is not None and hasattr(data_type, "getName") else ""
    )
    if not (_VBASEPTR_NAME.search(field) or _VBASEPTR_NAME.search(type_name)):
        return False
    return hasattr(data_type, "getComponentOffset") and hasattr(data_type, "setComponentOffset")


def apply_existing_vbaseptr_offsets(
    structure: Any, *, vbptr_offset: int, entries: Sequence[int]
) -> int:
    """Set ComponentOffset on existing VBasePtr typedefs at proven virtual-base offsets.

    Never creates PointerTypedefs. The complete-object offset of each later vbtable
    entry is ``vbptr_offset + displacement``.
    """

    if structure is None or not hasattr(structure, "getDefinedComponents") or len(entries) < 2:
        return 0
    targets = {vbptr_offset + int(displacement): int(displacement) for displacement in entries[1:]}
    applied = 0
    for component in structure.getDefinedComponents():
        field_offset = int(component.getOffset())
        if field_offset not in targets or not _looks_like_vbaseptr(component):
            continue
        data_type = component.getDataType()
        current = data_type.getComponentOffset()
        if current == field_offset:
            continue
        data_type.setComponentOffset(field_offset)
        applied += 1
    return applied


def _build_vbtable_structure(
    program: Any,
    qualified_class: str,
    entries: Sequence[int],
    *,
    base_class: str | None = None,
    construction: bool = False,
) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        IntegerDataType,
        StructureDataType,
    )

    category, structure_name, _path = _vbtable_paths(qualified_class, base_class=base_class)
    if construction:
        structure_name = f"{structure_name}_ctor"
    manager = program.getDataTypeManager()
    manager.createCategory(CategoryPath(category))
    structure = StructureDataType(CategoryPath(category), structure_name, 0)
    integer = IntegerDataType()
    for index, displacement in enumerate(entries):
        structure.add(integer, 4, f"disp_{index:02d}", str(displacement))
    return manager.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def _apply_vbtable_typing_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    from .ghidra.listing_guards import ClearRangeError, clear_code_units_guarded
    from .vftable_typing import _retarget_named_pointer_field

    action = row.get("action")
    if action not in {"create-and-apply", "apply-existing", "upgrade-definitions"}:
        return {**dict(row), "error": f"unexpected-action:{action}"}
    qualified = str(row["class"])
    base_class = row.get("base_class")
    role = str(row.get("role") or "vbtable")
    entries = [int(item["displacement"]) for item in row.get("entries") or ()]
    if action in {"create-and-apply", "upgrade-definitions"}:
        structure = _build_vbtable_structure(
            program,
            qualified,
            entries,
            base_class=base_class,
            construction=role == "construction",
        )
    else:
        structure = program.getDataTypeManager().getDataType(row["vbtable"])
    if structure is None:
        return {**dict(row), "error": "missing-vbtable-structure"}
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    start = space.getAddress(int(row["address"], 0))
    end = start.add(structure.getLength() - 1)
    try:
        clear_code_units_guarded(program, start, end, expected_name=None, address_owned=True)
        listing.createData(start, structure)
    except ClearRangeError as exc:
        return {**dict(row), **exc.payload}
    vbptr_offset = row.get("vbptr_offset")
    vbptr = False
    vbaseptr_offsets = 0
    if role != "construction" and isinstance(vbptr_offset, int):
        vbptr = _retarget_named_pointer_field(
            program, qualified, structure, _VBPTR_NAMES, offset=vbptr_offset
        )
        from .class_binding import find_class_structure, find_ghidra_class

        ghidra_class = find_ghidra_class(program, qualified)
        bound = find_class_structure(program, ghidra_class) if ghidra_class is not None else None
        vbaseptr_offsets = apply_existing_vbaseptr_offsets(
            bound, vbptr_offset=vbptr_offset, entries=entries
        )
    return {
        "class": row["class"],
        "base_class": base_class,
        "role": role,
        "address": row["address"],
        "vbtable": str(structure.getPathName()),
        "entry_count": structure.getNumComponents(),
        "vbptr_offset": vbptr_offset if isinstance(vbptr_offset, int) else None,
        "vbptr_retargeted": vbptr,
        "vbaseptr_component_offsets": vbaseptr_offsets,
        "action": action,
    }


def apply_vbtable_typing(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Create/apply planned vbtable Structures (one transaction per row)."""

    from .ghidra.mutations import apply_rows

    rows = [
        row
        for row in plan.get("vbtables", [])
        if row.get("action") in {"create-and-apply", "apply-existing", "upgrade-definitions"}
    ]
    result = apply_rows(
        program,
        rows,
        _apply_vbtable_typing_row,
        description="Type known vbtables",
    )
    return {
        "applied": result["applied"],
        "errors": result["errors"],
        "skipped": [],
        "vbtables": result["rows"],
    }
