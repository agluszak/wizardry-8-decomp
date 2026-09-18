"""Create typed vftable Structures and apply them at known vtable addresses.

For each source-index class with a ``vtable_address``, read the retail slot
targets, build per-slot ``FunctionDefinition`` types from the preferred slot
ABI contract, and install a namespace-safe Structure under ``/wiz8/vftables/``
whose fields are pointers to those definitions. Class ``vfptr``/``vptr``
fields are retargeted to pointers of that Structure when present.

Slot FunctionDefinitions prefer a source-backed declaration (source-index
``FUNCTION`` at the slot target, or a live callee whose signature source is
already ``IMPORTED``/``USER_DEFINED``) over cloning an analysis-only
implementation signature. When no declaration is available the FD falls back
to ``FunctionDefinitionDataType(function, False)``.

Deferred (not finished forever): secondary / construction / for-clause
vtables and true base-subobject slot ABI remain out of scope here. Primary
source-index ``vtable_address`` tables with census extents are the current
contract surface.
"""

from __future__ import annotations

import re
from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .class_binding import _sanitize_class_parts
from .config import Settings
from .paths import atomic_json, repo_relative, sha256_file
from .source_index import SourceIndex, load_source_index, source_functions

_SCHEMA = "wiz8.vftable-typing-v1"
_CATEGORY = "/wiz8/vftables"
_MAX_SLOTS = 256
_SAFE_FIELD = re.compile(r"[^0-9A-Za-z_]+")
# Confirmed MSVC census slot counts keyed by binary content hash (or path fallback).
_CENSUS_SLOT_CACHE: dict[str, dict[int, int]] = {}
_SOURCE_BACKED_SIG = frozenset({"IMPORTED", "USER_DEFINED"})


def _simple_name(qualified: str) -> str:
    return qualified.split("::")[-1]


def _vftable_paths(qualified: str) -> tuple[str, str, str, str]:
    """Return ``(category, structure_name, structure_path, sigs_category)``.

    Namespace parts become category segments under ``/wiz8/vftables/`` so
    ``ns::Class`` and ``other::Class`` do not collide on ``Class_vftable``.
    """

    parent_parts, leaf = _sanitize_class_parts(qualified)
    if parent_parts:
        category = f"{_CATEGORY}/{'/'.join(parent_parts)}"
    else:
        category = _CATEGORY
    structure_name = f"{leaf}_vftable"
    return category, structure_name, f"{category}/{structure_name}", f"{category}/{leaf}_sigs"


def _canonical_matching_binary(repo_dir: Path, work_dir: Path) -> Path:
    """Same binary path pattern as ``commands.vtables._canonical_binary``."""

    import yaml

    config = yaml.safe_load((repo_dir / "config" / "variants.yml").read_text(encoding="utf-8"))
    target = config["canonical_matching_target"]
    return work_dir / "variants" / target["variant"] / target["module"]


def census_vftable_slot_counts(repo_dir: Path, work_dir: Path) -> dict[int, int]:
    """Confirmed census vftable ``slot_count`` by absolute address (process-cached)."""

    binary = _canonical_matching_binary(repo_dir, work_dir)
    if binary.is_file():
        cache_key = sha256_file(binary)
    else:
        cache_key = str(binary)
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


def _signature_source_name(function: Any) -> str | None:
    if function is None or not hasattr(function, "getSignatureSource"):
        return None
    source = function.getSignatureSource()
    if source is None:
        return None
    return str(getattr(source, "name", None) or source)


def _function_signature_source_backed(function: Any) -> bool:
    name = _signature_source_name(function)
    return name in _SOURCE_BACKED_SIG if name is not None else False


def _declaration_payload(declaration: Any) -> dict[str, Any]:
    return {
        "return_type": declaration.return_type,
        "parameter_types": list(declaration.parameter_types),
        "calling_convention": declaration.calling_convention,
        "has_this": bool(declaration.has_this),
        "owning_class": declaration.owning_class,
        "is_variadic": bool(declaration.is_variadic),
    }


def annotate_slot_fd_sources(
    program: Any,
    slots: list[dict[str, Any]],
    source_by_address: Mapping[int, Any],
) -> None:
    """Stamp each slot with ``fd_source`` / optional ``declaration`` payload."""

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    for slot in slots:
        if slot.get("unresolved") or not slot.get("target"):
            slot["fd_source"] = None
            continue
        target = int(str(slot["target"]), 0)
        marker = source_by_address.get(target)
        declaration = marker.declaration if marker is not None else None
        if declaration is not None and getattr(marker, "marker_kind", None) == "FUNCTION":
            slot["fd_source"] = "source-declaration"
            slot["declaration"] = _declaration_payload(declaration)
            continue
        function = functions.getFunctionAt(space.getAddress(target))
        if _function_signature_source_backed(function):
            slot["fd_source"] = "source-declaration"
            slot.pop("declaration", None)
            continue
        slot["fd_source"] = "callee-implementation"
        slot.pop("declaration", None)


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


def desired_slot_contract(
    program: Any,
    slot: Mapping[str, Any],
    *,
    qualified_class: str,
    field: str,
) -> Any:
    """FunctionDefinition for one slot. Construction and agreement share this.

    Preference: source declaration, else source-backed live function, else callee
    implementation. An unresolved *source* type is an error, not a silent demotion.
    """

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        FunctionDefinitionDataType,
    )

    if slot.get("unresolved") or not slot.get("target"):
        raise ValueError("unresolved-slot")
    space = program.getAddressFactory().getDefaultAddressSpace()
    target = int(str(slot["target"]), 0)
    function = program.getFunctionManager().getFunctionAt(space.getAddress(target))
    _, _leaf = _sanitize_class_parts(qualified_class)
    _category, _structure_name, _path, sigs_category = _vftable_paths(qualified_class)
    declaration = slot.get("declaration")
    if isinstance(declaration, Mapping):
        try:
            definition = _definition_from_declaration(
                program,
                definition_name=f"{_leaf}_{field}",
                sigs_category=sigs_category,
                declaration=declaration,
            )
        except Exception as exc:
            raise ValueError(f"unresolved-source-type:{exc}") from exc
        return definition
    if function is None:
        raise ValueError("missing-callee")
    if slot.get("fd_source") == "source-declaration" or _function_signature_source_backed(function):
        try:
            return FunctionDefinitionDataType(function, False)
        except Exception as exc:
            raise ValueError(f"source-backed-signature-failed:{exc}") from exc
    try:
        return FunctionDefinitionDataType(function, False)
    except Exception as exc:
        raise ValueError(f"callee-signature-failed:{exc}") from exc


def _slot_definitions_match_desired(
    program: Any,
    structure: Any,
    slots: Sequence[dict[str, Any]],
    qualified_class: str,
) -> bool:
    """True when each stored FunctionDefinition matches ``desired_slot_contract``."""

    from .datatype_contracts import function_definition_contract

    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return False
    components = list(structure.getDefinedComponents())
    if len(components) != len(slots):
        return False
    for component, slot in zip(components, slots, strict=True):
        if slot.get("unresolved") or not slot.get("target"):
            return False
        if slot.get("contract_error"):
            return False
        existing = _component_function_definition(component)
        if existing is None:
            return False
        field = str(component.getFieldName() or _field_name(int(slot["index"]), slot.get("name")))
        try:
            desired = desired_slot_contract(
                program, slot, qualified_class=qualified_class, field=field
            )
        except ValueError:
            return False
        if function_definition_contract(existing) != function_definition_contract(desired):
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
    source_by_address = source_functions(repository, target)
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
        annotate_slot_fd_sources(program, slots, source_by_address)
        for slot in slots:
            if not slot.get("declaration"):
                continue
            field = _field_name(int(slot["index"]), slot.get("name"))
            try:
                desired_slot_contract(
                    program, slot, qualified_class=record.qualified_name, field=field
                )
            except ValueError as exc:
                slot["contract_error"] = str(exc)
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
        category, _vftable_name, vftable_path, _sigs = _vftable_paths(name)
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
        current_type_path = str(data.getDataType().getPathName()) if data is not None else None
        has_defs = _vftable_has_function_definitions(existing)
        existing_slots = (
            int(existing.getNumComponents())
            if existing is not None and hasattr(existing, "getNumComponents")
            else None
        )
        slots_match = existing_slots == len(slots)
        has_unresolved = any(slot.get("unresolved") for slot in slots)
        source_contract_error = any(slot.get("contract_error") for slot in slots)
        defs_match = (
            slots_match
            and has_defs
            and not has_unresolved
            and not source_contract_error
            and _slot_definitions_match_desired(program, existing, slots, name)
        )
        if covered_by is not None:
            action = "covered-by-sibling"
        elif not slots:
            action = "no-slots"
        elif item.get("extent_source") == "fallback":
            action = "fallback-extent-not-actionable"
        elif has_unresolved and item.get("extent_source") == "census":
            action = "census-slot-unresolved"
        elif source_contract_error:
            action = "unresolved-source-type"
        elif (
            existing is not None
            and has_defs
            and slots_match
            and defs_match
            and current_type_path == vftable_path
        ):
            action = "agree"
        elif existing is not None and has_defs and slots_match and defs_match:
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
                "vftable_category": category,
                "slot_count": len(slots),
                "declared_slots": declared,
                "census_slots": item.get("census_slots"),
                "extent_source": item.get("extent_source"),
                "slots": slots,
                "current_type": current_type_path,
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


def _resolve_slot_type(program: Any, spelling: str) -> Any | None:
    from .callback_typing import _resolve_type

    return _resolve_type(program, spelling)


def _definition_from_declaration(
    program: Any,
    *,
    definition_name: str,
    sigs_category: str,
    declaration: Mapping[str, Any],
) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
    )

    definition = FunctionDefinitionDataType(CategoryPath(sigs_category), definition_name)
    return_spelling = str(declaration.get("return_type") or "void")
    return_type = _resolve_slot_type(program, return_spelling)
    if return_type is None:
        raise ValueError(f"unresolved return type: {return_spelling}")
    definition.setReturnType(return_type)

    params: list[Any] = []
    convention = str(declaration.get("calling_convention") or "")
    has_this = bool(declaration.get("has_this"))
    owning = declaration.get("owning_class")
    if has_this and not convention:
        convention = "__thiscall"
    if has_this or convention == "__thiscall":
        if not owning:
            raise ValueError("unresolved this type: missing owning class")
        this_type = _resolve_slot_type(program, f"{owning} *")
        if this_type is None:
            raise ValueError(f"unresolved this type: {owning} *")
        params.append(ParameterDefinitionImpl("this", this_type, None))

    for index, spelling in enumerate(declaration.get("parameter_types") or ()):
        data_type = _resolve_slot_type(program, str(spelling))
        if data_type is None:
            raise ValueError(f"unresolved param type: {spelling}")
        params.append(ParameterDefinitionImpl(f"param_{index}", data_type, None))
    if params:
        definition.setArguments(params)
    if convention:
        definition.setCallingConvention(convention)
    if declaration.get("is_variadic") and hasattr(definition, "setVarArgs"):
        definition.setVarArgs(True)
    return definition


def _slot_function_pointer(
    program: Any,
    qualified_class: str,
    slot: dict[str, Any],
    field: str,
) -> Any:
    """Build ``Pointer(FunctionDefinition)`` from ``desired_slot_contract``."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        PointerDataType,
        VoidDataType,
    )

    _category, _structure_name, _path, sigs_category = _vftable_paths(qualified_class)
    manager = program.getDataTypeManager()
    if slot.get("unresolved") or not slot.get("target"):
        slot["fd_source"] = None
        return PointerDataType(VoidDataType(), manager)
    definition = desired_slot_contract(program, slot, qualified_class=qualified_class, field=field)
    definition.setName(f"{_sanitize_class_parts(qualified_class)[1]}_{field}")
    definition.setCategoryPath(CategoryPath(sigs_category))
    added = manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
    return PointerDataType(added, manager)


def _build_vftable_structure(
    program: Any, qualified_class: str, slots: list[dict[str, Any]]
) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
    )

    category, structure_name, _path, _sigs = _vftable_paths(qualified_class)
    manager = program.getDataTypeManager()
    manager.createCategory(CategoryPath(category))
    structure = StructureDataType(CategoryPath(category), structure_name, 0)
    used: set[str] = set()
    for slot in slots:
        name = _field_name(int(slot["index"]), slot.get("name"))
        base = name
        suffix = 1
        while name in used:
            name = f"{base}_{suffix}"
            suffix += 1
        used.add(name)
        pointer = _slot_function_pointer(program, qualified_class, slot, name)
        structure.add(pointer, 4, name, slot.get("target"))
    return manager.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def _apply_data(
    program: Any, address: int, data_type: Any, *, expected_name: str | None = None
) -> None:
    from .ghidra.listing_guards import clear_code_units_guarded

    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    start = space.getAddress(address)
    end = start.add(data_type.getLength() - 1)
    clear_code_units_guarded(program, start, end, expected_name=expected_name, address_owned=True)
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
    from .ghidra.listing_guards import ClearRangeError

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
    qualified = str(row["class"])
    if action in {"create-and-apply", "upgrade-definitions"}:
        structure = _build_vftable_structure(program, qualified, slots)
    else:
        structure = program.getDataTypeManager().getDataType(row["vftable"])
    if structure is None:
        return {**dict(row), "error": "missing-vftable-structure"}
    try:
        _apply_data(program, int(row["address"], 0), structure)
    except ClearRangeError as exc:
        return {**dict(row), **exc.payload}
    vfptr = _retarget_class_vfptr(program, qualified, structure)
    return {
        "class": row["class"],
        "address": row["address"],
        "vftable": str(structure.getPathName()),
        "slot_count": structure.getNumComponents(),
        "vfptr_retargeted": vfptr,
        "action": action,
        "function_definitions": _vftable_has_function_definitions(structure),
        "fd_sources": dict(
            Counter(slot.get("fd_source") for slot in slots if slot.get("fd_source"))
        ),
    }


def apply_vftable_typing(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Create/apply planned vftable Structures (one transaction per row).

    Rows whose extent came from the sibling-stop fallback (no census slot count)
    are skipped on apply — dry-run may still plan them with
    ``extent_source: "fallback"``. Census rows with unresolved slots are skipped
    (``census-slot-unresolved``) rather than counted as apply errors.
    """

    from .ghidra.mutations import apply_rows

    rows = [
        row
        for row in plan.get("vftables", [])
        if row.get("action") in {"create-and-apply", "apply-existing", "upgrade-definitions"}
    ]
    actionable: list[Mapping[str, Any]] = []
    skipped: list[dict[str, Any]] = []
    for row in rows:
        if row.get("extent_source") == "fallback":
            skipped.append({**dict(row), "skipped": "fallback-extent-not-actionable"})
            continue
        if row.get("extent_source") == "census" and any(
            slot.get("unresolved") for slot in (row.get("slots") or [])
        ):
            skipped.append({**dict(row), "skipped": "census-slot-unresolved"})
            continue
        if any(slot.get("contract_error") for slot in (row.get("slots") or [])):
            skipped.append({**dict(row), "skipped": "unresolved-source-type"})
            continue
        actionable.append(row)
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
                        {
                            "index": s["index"],
                            "target": s["target"],
                            "name": s.get("name"),
                            "fd_source": s.get("fd_source"),
                        }
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
