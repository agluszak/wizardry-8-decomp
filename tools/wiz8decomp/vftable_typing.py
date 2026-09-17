"""Create typed vftable Structures and apply them at known vtable addresses.

For each source-index class with a ``vtable_address``, read the retail slot
targets, build per-slot ``FunctionDefinition`` types from the callee signature
(including an explicit ``this`` for ``__thiscall``), and install a
``/wiz8/vftables/ClassName_vftable`` Structure whose fields are pointers to
those definitions. Class ``vfptr``/``vptr`` fields are retargeted to pointers
of that Structure when present.
"""

from __future__ import annotations

import re
from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json, repo_relative
from .source_index import SourceIndex, load_source_index

_SCHEMA = "wiz8.vftable-typing-v1"
_CATEGORY = "/wiz8/vftables"
_MAX_SLOTS = 256
_SAFE_FIELD = re.compile(r"[^0-9A-Za-z_]+")
# Confirmed MSVC census slot counts keyed by resolved binary path.
_CENSUS_SLOT_CACHE: dict[str, dict[int, int]] = {}


def _simple_name(qualified: str) -> str:
    return qualified.split("::")[-1]


def _canonical_matching_binary(repo_dir: Path, work_dir: Path) -> Path:
    """Same binary path pattern as ``commands.vtables._canonical_binary``."""

    import yaml

    config = yaml.safe_load((repo_dir / "config" / "variants.yml").read_text(encoding="utf-8"))
    target = config["canonical_matching_target"]
    return work_dir / "variants" / target["variant"] / target["module"]


def census_vftable_slot_counts(repo_dir: Path, work_dir: Path) -> dict[int, int]:
    """Confirmed census vftable ``slot_count`` by absolute address (process-cached)."""

    binary = _canonical_matching_binary(repo_dir, work_dir)
    cache_key = str(binary.resolve()) if binary.exists() else str(binary)
    cached = _CENSUS_SLOT_CACHE.get(cache_key)
    if cached is not None:
        return cached
    from .msvc_tables import scan_msvc_tables

    report = scan_msvc_tables(binary, repo_dir=repo_dir)
    counts = {
        int(row["address"], 0): int(row["slot_count"])
        for row in report.get("vftables", [])
        if row.get("address") is not None and row.get("slot_count") is not None
    }
    _CENSUS_SLOT_CACHE[cache_key] = counts
    return counts


def _field_name(index: int, function_name: str | None) -> str:
    if not function_name:
        return f"slot_{index:02d}"
    leaf = function_name.split("::")[-1]
    leaf = leaf.strip("'").replace("~", "dtor_")
    leaf = _SAFE_FIELD.sub("_", leaf).strip("_")
    if not leaf:
        return f"slot_{index:02d}"
    if leaf[0].isdigit():
        leaf = f"m_{leaf}"
    return f"{leaf}_{index:02d}"


def _read_slots(
    program: Any,
    address: int,
    *,
    max_slots: int | None = None,
    stop_before: set[int] | None = None,
) -> list[dict[str, Any]]:
    """Read vftable slot targets from retail memory.

    When ``max_slots`` is set (census extent), always emit exactly that many
    entries. Missing Function / unreadable memory → ``unresolved: true`` with
    null target/name as appropriate. Never silently shrink a census extent.
    """

    memory = program.getMemory()
    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    census_mode = max_slots is not None
    limit = max_slots if max_slots is not None else _MAX_SLOTS
    slots: list[dict[str, Any]] = []
    for index in range(limit):
        slot_addr = address + index * 4
        # Unbounded reads must not swallow a neighboring class's vtable start.
        if index > 0 and stop_before is not None and slot_addr in stop_before:
            break
        entry = space.getAddress(slot_addr)
        if not memory.contains(entry):
            if census_mode:
                slots.append(
                    {
                        "index": index,
                        "target": None,
                        "name": None,
                        "prototype": None,
                        "unresolved": True,
                    }
                )
                continue
            break
        try:
            target = memory.getInt(entry) & 0xFFFFFFFF
        except Exception as exc:
            if "MemoryAccessException" not in type(exc).__name__ and "MemoryAccess" not in str(
                type(exc)
            ):
                raise
            if census_mode:
                slots.append(
                    {
                        "index": index,
                        "target": None,
                        "name": None,
                        "prototype": None,
                        "unresolved": True,
                    }
                )
                continue
            break
        function = functions.getFunctionAt(space.getAddress(target))
        if function is None:
            if census_mode:
                slots.append(
                    {
                        "index": index,
                        "target": f"0x{target:08x}",
                        "name": None,
                        "prototype": None,
                        "unresolved": True,
                    }
                )
                continue
            break
        slots.append(
            {
                "index": index,
                "target": f"0x{target:08x}",
                "name": function.getName(True),
                "prototype": function.getPrototypeString(False, False),
            }
        )
    return slots


def _points_to_function_definition(data_type: Any) -> bool:
    if data_type is None or not hasattr(data_type, "getDataType"):
        return False
    pointed = data_type.getDataType()
    if pointed is None:
        return False
    name = type(pointed).__name__
    return "FunctionDefinition" in name or hasattr(pointed, "getArguments")


def _vftable_has_function_definitions(structure: Any) -> bool:
    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return False
    components = list(structure.getDefinedComponents())
    if not components:
        return False
    return all(_points_to_function_definition(component.getDataType()) for component in components)


def _component_function_definition(component: Any) -> Any | None:
    data_type = component.getDataType() if component is not None else None
    if data_type is None or not hasattr(data_type, "getDataType"):
        return None
    pointed = data_type.getDataType()
    if pointed is None:
        return None
    while pointed is not None and "TypeDef" in type(pointed).__name__:
        if not hasattr(pointed, "getBaseDataType"):
            break
        pointed = pointed.getBaseDataType()
    if pointed is None:
        return None
    if "FunctionDefinition" in type(pointed).__name__ or hasattr(pointed, "getArguments"):
        return pointed
    return None


def _slot_definitions_match_callees(
    program: Any, structure: Any, slots: Sequence[dict[str, Any]]
) -> bool:
    """True when each stored FunctionDefinition matches the live callee contract."""

    from .callback_typing import definition_matches_function

    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return False
    components = list(structure.getDefinedComponents())
    if len(components) != len(slots):
        return False
    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    for component, slot in zip(components, slots, strict=True):
        if slot.get("unresolved") or not slot.get("target"):
            return False
        definition = _component_function_definition(component)
        if definition is None:
            return False
        target = int(str(slot["target"]), 0)
        function = functions.getFunctionAt(space.getAddress(target))
        if function is None:
            return False
        if not definition_matches_function(definition, function):
            return False
    return True


def collect_vftable_typing_plan(
    repository: Path,
    program: Any,
    *,
    work_dir: Path | None = None,
    target: str = "WIZ8",
    class_names: Sequence[str] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Plan vftable Structure creation for source classes with vtable addresses."""

    index = SourceIndex.from_dict(load_source_index(repository))
    wanted = None
    if class_names is not None:
        wanted = {_simple_name(name) for name in class_names} | set(class_names)

    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    manager = program.getDataTypeManager()
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()

    classes = [
        record
        for record in index.classes
        if record.vtable_address is not None
        and (record.target is None or record.target.upper() == target.upper())
    ]
    if wanted is not None:
        classes = [
            record
            for record in classes
            if record.qualified_name in wanted or _simple_name(record.qualified_name) in wanted
        ]

    sibling_starts = {
        int(address) for record in classes if (address := record.vtable_address) is not None
    }

    census_slots: dict[int, int] = {}
    if work_dir is not None:
        try:
            census_slots = census_vftable_slot_counts(repository, work_dir)
        except Exception:  # noqa: BLE001 — missing binary / scan failure → unbounded fallback
            census_slots = {}

    # First pass: slot extents from MSVC census when confirmed; otherwise
    # unbounded read stopped at sibling table starts (not virtual_declarations).
    prepared: list[dict[str, Any]] = []
    for record in sorted(classes, key=lambda item: item.qualified_name):
        vtable_address = record.vtable_address
        assert vtable_address is not None
        address = int(vtable_address)
        declared = len(record.virtual_declarations) if record.virtual_declarations else None
        census_count = census_slots.get(address)
        if census_count is not None:
            slots = _read_slots(program, address, max_slots=census_count, stop_before=None)
            extent_source = "census"
        else:
            slots = _read_slots(
                program,
                address,
                max_slots=None,
                stop_before=sibling_starts,
            )
            extent_source = "fallback"
        prepared.append(
            {
                "record": record,
                "address": address,
                "declared": declared,
                "census_slots": census_count,
                "extent_source": extent_source,
                "slots": slots,
                "end": address + 4 * len(slots),
            }
        )

    for item in prepared:
        record = item["record"]
        name = record.qualified_name
        simple = _simple_name(name)
        vftable_name = f"{simple}_vftable"
        vftable_path = f"{_CATEGORY}/{vftable_name}"
        address = item["address"]
        slots = item["slots"]
        declared = item["declared"]
        covered_by = None
        for other in prepared:
            if other["address"] == address:
                continue
            if other["address"] < address < other["end"]:
                covered_by = other["record"].qualified_name
                break
        existing = manager.getDataType(vftable_path)
        data = listing.getDataAt(space.getAddress(address))
        current_type = str(data.getDataType().getName()) if data is not None else None
        has_defs = _vftable_has_function_definitions(existing)
        existing_slots = (
            int(existing.getNumComponents())
            if existing is not None and hasattr(existing, "getNumComponents")
            else None
        )
        slots_match = existing_slots == len(slots)
        has_unresolved = any(slot.get("unresolved") for slot in slots)
        defs_match_callees = (
            slots_match
            and has_defs
            and not has_unresolved
            and _slot_definitions_match_callees(program, existing, slots)
        )
        if covered_by is not None:
            action = "covered-by-sibling"
        elif not slots:
            action = "no-slots"
        elif (
            existing is not None
            and has_defs
            and slots_match
            and defs_match_callees
            and current_type == vftable_name
        ):
            action = "agree"
        elif existing is not None and has_defs and slots_match and defs_match_callees:
            action = "apply-existing"
        elif existing is not None:
            action = "upgrade-definitions"
        else:
            action = "create-and-apply"
        counts[action] += 1
        if action in {"agree", "covered-by-sibling"}:
            continue
        rows.append(
            {
                "class": name,
                "address": f"0x{address:08x}",
                "vftable": vftable_path,
                "slot_count": len(slots),
                "declared_slots": declared,
                "census_slots": item.get("census_slots"),
                "extent_source": item.get("extent_source"),
                "slots": slots,
                "current_type": current_type,
                "has_function_definitions": has_defs,
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
        "vftables": rows,
    }


def _slot_function_pointer(program: Any, simple: str, slot: dict[str, Any], field: str) -> Any:
    """Build ``Pointer(FunctionDefinition)`` from the slot target's signature."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        PointerDataType,
        VoidDataType,
    )

    manager = program.getDataTypeManager()
    if slot.get("unresolved") or not slot.get("target"):
        return PointerDataType(VoidDataType(), manager)
    space = program.getAddressFactory().getDefaultAddressSpace()
    target = int(slot["target"], 0)
    function = program.getFunctionManager().getFunctionAt(space.getAddress(target))
    if function is None:
        return PointerDataType(VoidDataType(), manager)

    # Formal signature includes an explicit ``this`` for __thiscall methods —
    # required for FunctionDefinition-typed function pointers (Ghidra #5484).
    try:
        definition = FunctionDefinitionDataType(function, False)
    except Exception as exc:
        raise RuntimeError(
            "FunctionDefinitionDataType(function, False) failed for "
            f"{function.getName(True)} at {slot.get('target')}: {exc}"
        ) from exc
    definition.setName(f"{simple}_{field}")
    definition.setCategoryPath(CategoryPath(f"{_CATEGORY}/{simple}_sigs"))
    added = manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
    return PointerDataType(added, manager)


def _build_vftable_structure(program: Any, simple: str, slots: list[dict[str, Any]]) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
    )

    manager = program.getDataTypeManager()
    manager.createCategory(CategoryPath(_CATEGORY))
    structure = StructureDataType(CategoryPath(_CATEGORY), f"{simple}_vftable", 0)
    used: set[str] = set()
    for slot in slots:
        name = _field_name(int(slot["index"]), slot.get("name"))
        base = name
        suffix = 1
        while name in used:
            name = f"{base}_{suffix}"
            suffix += 1
        used.add(name)
        pointer = _slot_function_pointer(program, simple, slot, name)
        structure.add(pointer, 4, name, slot.get("target"))
    return manager.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def _apply_data(program: Any, address: int, data_type: Any) -> None:
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    start = space.getAddress(address)
    end = start.add(data_type.getLength() - 1)
    listing.clearCodeUnits(start, end, False)
    listing.createData(start, data_type)


def _retarget_class_vfptr(program: Any, class_name: str, vftable: Any) -> bool:
    """Retarget ``vfptr``/``vptr`` on the bound class Structure, if present."""

    from .class_binding import find_class_structure, find_ghidra_class

    ghidra_class = find_ghidra_class(program, class_name)
    if ghidra_class is None:
        return False
    structure = find_class_structure(program, ghidra_class)
    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return False
    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

    manager = program.getDataTypeManager()
    pointer = PointerDataType(vftable, manager)
    for component in structure.getDefinedComponents():
        field = component.getFieldName()
        if field in {"vfptr", "vptr", "vftable", "__vftable"}:
            structure.replaceAtOffset(
                component.getOffset(), pointer, 4, field, component.getComment()
            )
            return True
    # Many VC6 layouts store the first base subobject at offset 0; leave those alone.
    return False


def _apply_vftable_typing_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    action = row.get("action")
    if action not in {"create-and-apply", "apply-existing", "upgrade-definitions"}:
        return {**dict(row), "error": f"unexpected-action:{action}"}
    if row.get("extent_source") == "fallback":
        return {
            **dict(row),
            "error": "fallback-extent-not-actionable",
        }
    slots = list(row.get("slots") or [])
    if row.get("extent_source") == "census" and any(slot.get("unresolved") for slot in slots):
        return {**dict(row), "error": "census-slot-unresolved"}
    simple = _simple_name(str(row["class"]))
    if action in {"create-and-apply", "upgrade-definitions"}:
        structure = _build_vftable_structure(program, simple, slots)
    else:
        structure = program.getDataTypeManager().getDataType(row["vftable"])
    if structure is None:
        return {**dict(row), "error": "missing-vftable-structure"}
    _apply_data(program, int(row["address"], 0), structure)
    vfptr = _retarget_class_vfptr(program, str(row["class"]), structure)
    return {
        "class": row["class"],
        "address": row["address"],
        "vftable": str(structure.getPathName()),
        "slot_count": structure.getNumComponents(),
        "vfptr_retargeted": vfptr,
        "action": action,
        "function_definitions": _vftable_has_function_definitions(structure),
    }


def apply_vftable_typing(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Create/apply planned vftable Structures (one transaction per row).

    Rows whose extent came from the sibling-stop fallback (no census slot count)
    are skipped on apply — dry-run may still plan them with
    ``extent_source: "fallback"``.
    """

    from .ghidra.mutations import apply_rows

    rows = [
        row
        for row in plan.get("vftables", [])
        if row.get("action") in {"create-and-apply", "apply-existing", "upgrade-definitions"}
    ]
    actionable = [row for row in rows if row.get("extent_source") != "fallback"]
    skipped = [
        {**dict(row), "skipped": "fallback-extent-not-actionable"}
        for row in rows
        if row.get("extent_source") == "fallback"
    ]
    result = apply_rows(
        program,
        actionable,
        _apply_vftable_typing_row,
        description="Type known vftables",
    )
    return {
        "applied": result["applied"],
        "errors": result["errors"],
        "skipped": skipped,
        "vftables": result["rows"],
    }


def run_vftable_typing(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    class_names: Sequence[str] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Report or apply typed vftable Structures."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_vftable_typing_plan(
            settings.repo_dir,
            program,
            work_dir=settings.work_dir,
            target=target,
            class_names=class_names,
            limit=limit,
        )
        # Bound the on-disk report: drop per-slot prototypes in the saved artifact sample.
        compact = {
            **plan,
            "vftables": [
                {k: v for k, v in row.items() if k != "slots"}
                | {
                    "slots": [
                        {"index": s["index"], "target": s["target"], "name": s.get("name")}
                        for s in row.get("slots", [])
                    ]
                }
                for row in plan["vftables"]
            ],
            "program": program_name,
            "apply": apply,
        }
        out_dir = settings.build_dir / "vftable-typing"
        report_path = out_dir / "report.json"
        atomic_json(report_path, compact)
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": repo_relative(report_path, settings.repo_dir),
            "sample": compact["vftables"][:15],
        }
        if not apply:
            return result

        applied = apply_vftable_typing(program, plan)
        dispose_sessions()
        program.save("Type known vftables", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["skipped_fallback"] = len(applied.get("skipped") or [])
        result["sample"] = applied["vftables"][:15]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = repo_relative(error_path, settings.repo_dir)
        return result
