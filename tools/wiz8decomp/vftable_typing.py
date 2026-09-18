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

Primary and source-index ``base_vtables`` (for-clause / secondary base) with
census extents are typed. Slot ``this`` for a secondary table is the Base
subobject (or a proven Derived ComponentOffset), not the Derived implementation
body. Unmarked construction-phase tables that share a census construction family
with a marked table of the same class are typed as ``Class_vftable_ctor`` /
``Class_vftable_for_Base_ctor`` and never retarget the complete-object ``vfptr``.
Secondary ``vfptr`` fields are installed as Derived-specific subobject views
under ``/wiz8/subobjects`` when the unique incoming-ECX offset is known.
Confirmed vbtables that a lifecycle function installs alongside a marked
vftable are typed under ``/wiz8/vbtables``.
"""

from __future__ import annotations

import re
from collections import Counter
from collections.abc import Mapping, Sequence
from copy import deepcopy
from pathlib import Path
from typing import Any

from .class_binding import _sanitize_class_parts
from .config import Settings
from .paths import atomic_json, repo_relative, sha256_file
from .source_index import SourceIndex, load_source_index, source_functions

_SCHEMA = "wiz8.vftable-typing-v2"
_CATEGORY = "/wiz8/vftables"
_MAX_SLOTS = 256
_SAFE_FIELD = re.compile(r"[^0-9A-Za-z_]+")
_INCOMING_RECEIVERS = frozenset({"incoming-ecx", "incoming-ecx-plus-dynamic"})
_VFPTR_NAMES = frozenset({"vfptr", "vptr", "vftable", "__vftable"})
# Confirmed MSVC census keyed by binary content hash (or path fallback).
_CENSUS_SLOT_CACHE: dict[str, dict[int, int]] = {}
_RAW_CENSUS_CACHE: dict[str, dict[str, Any]] = {}
_ENRICHED_CENSUS_CACHE: dict[str, dict[str, Any]] = {}
_SOURCE_BACKED_SIG = frozenset({"IMPORTED", "USER_DEFINED"})


def _simple_name(qualified: str) -> str:
    return qualified.split("::")[-1]


def _subobject_view_paths(derived: str, base: str, offset: int) -> tuple[str, str, str]:
    """Return ``(category, name, path)`` for a Derived-specific Base subobject view."""

    parts, leaf = _sanitize_class_parts(derived)
    category = "/wiz8/subobjects/" + "/".join([*parts, leaf] if parts else [leaf])
    name = f"{_simple_name(base)}_at_0x{offset:x}"
    return category, name, f"{category}/{name}"


def _vftable_paths(
    qualified: str,
    base_class: str | None = None,
    *,
    phase: str | None = None,
) -> tuple[str, str, str, str]:
    """Return ``(category, structure_name, structure_path, sigs_category)``.

    Namespace parts become category segments under ``/wiz8/vftables/`` so
    ``ns::Class`` and ``other::Class`` do not collide on ``Class_vftable``.
    Secondary / for-clause tables use ``Class_vftable_for_Base``.
    Construction-phase tables append ``_ctor`` and keep a separate sigs category.
    """

    parent_parts, leaf = _sanitize_class_parts(qualified)
    if parent_parts:
        category = f"{_CATEGORY}/{'/'.join(parent_parts)}"
    else:
        category = _CATEGORY
    if base_class:
        base_leaf = _simple_name(base_class)
        structure_name = f"{leaf}_vftable_for_{base_leaf}"
        sigs_category = f"{category}/{leaf}_sigs_for_{base_leaf}"
    else:
        structure_name = f"{leaf}_vftable"
        sigs_category = f"{category}/{leaf}_sigs"
    if phase == "construction":
        structure_name = f"{structure_name}_ctor"
        sigs_category = f"{sigs_category}_ctor"
    return category, structure_name, f"{category}/{structure_name}", sigs_category


def unique_receiver_offset(writes: Sequence[Mapping[str, Any]] | None) -> int | None:
    """Single incoming-ECX receiver offset, or ``None`` when missing or conflicting."""

    offsets = {
        write["receiver_offset"]
        for write in writes or ()
        if write.get("receiver_provenance") in _INCOMING_RECEIVERS
        and isinstance(write.get("receiver_offset"), int)
    }
    if len(offsets) != 1:
        return None
    return next(iter(offsets))


def unique_object_offset(writes: Sequence[Mapping[str, Any]] | None) -> int | None:
    """Single ``[base+disp]`` object offset across writes, if unambiguous."""

    offsets = {
        write["object_offset"]
        for write in writes or ()
        if isinstance(write.get("object_offset"), int)
    }
    if len(offsets) != 1:
        return None
    return next(iter(offsets))


def lifecycle_class_name(function_source_name: str | None) -> str | None:
    """Qualified class from ``Class::Class`` / ``Class::~Class``, else ``None``."""

    if not function_source_name:
        return None
    parts = function_source_name.rsplit("::", 1)
    if len(parts) != 2:
        return None
    owner, leaf = parts
    owner_leaf = owner.split("::")[-1]
    if leaf == owner_leaf:
        return owner
    if leaf.startswith("~") and leaf[1:] == owner_leaf:
        return owner
    return None


def _is_constructor_name(function_source_name: str) -> bool:
    parts = function_source_name.rsplit("::", 1)
    return len(parts) == 2 and parts[1] == parts[0].split("::")[-1]


def _is_destructor_name(function_source_name: str) -> bool:
    parts = function_source_name.rsplit("::", 1)
    return len(parts) == 2 and parts[1].startswith("~") and parts[1][1:] == parts[0].split("::")[-1]


def _class_marked_tables(record: Any) -> dict[int, str | None]:
    """Marked vftable address → ``base_class`` (``None`` for the complete object)."""

    marked: dict[int, str | None] = {}
    address = getattr(record, "vtable_address", None)
    if address is not None:
        marked[int(address)] = None
    for base_vtable in getattr(record, "base_vtables", ()) or ():
        marked[int(base_vtable.address)] = str(base_vtable.base_class)
    return marked


def _lookup_source_class(classes: Sequence[Any], qualified: str) -> Any | None:
    exact = [record for record in classes if record.qualified_name == qualified]
    if len(exact) == 1:
        return exact[0]
    if exact:
        return None
    matches = [
        record
        for record in classes
        if _simple_name(record.qualified_name) == qualified
        or record.qualified_name == _simple_name(qualified)
    ]
    if len(matches) == 1:
        return matches[0]
    return None


def construction_vtable_attachments(
    families: Sequence[Mapping[str, Any]],
    classes: Sequence[Any],
) -> list[dict[str, Any]]:
    """Unmarked construction-phase vftables sharing a receiver with a marked table.

    Constructor families: earlier vftable transitions are construction; last is
    final. Destructor families: later transitions are construction; first is
    final. Addresses already in the source index (including support-class
    construction ``VTABLE`` markers) keep that identity. Conflicting class
    attachments are dropped.
    """

    marked_global: set[int] = set()
    for record in classes:
        marked_global.update(_class_marked_tables(record))

    chosen: dict[int, dict[str, Any]] = {}
    conflicts: set[int] = set()
    for family in families:
        name = str(family.get("function_source_name") or "")
        owner = lifecycle_class_name(name)
        if owner is None:
            continue
        record = _lookup_source_class(classes, owner)
        if record is None:
            continue
        marked = _class_marked_tables(record)
        transitions = [
            row for row in family.get("transitions") or () if row.get("kind") == "vftable"
        ]
        if len(transitions) < 2:
            continue
        if not any(int(row["table"], 0) in marked for row in transitions):
            continue
        if _is_constructor_name(name):
            construction = transitions[:-1]
            final = transitions[-1]
        elif _is_destructor_name(name):
            construction = transitions[1:]
            final = transitions[0]
        else:
            continue
        final_addr = int(final["table"], 0)
        base_class = marked.get(final_addr)
        if final_addr not in marked:
            base_class = next(
                (
                    marked[int(row["table"], 0)]
                    for row in transitions
                    if int(row["table"], 0) in marked
                ),
                None,
            )
        receiver_offset = family.get("receiver_offset")
        if not isinstance(receiver_offset, int):
            receiver_offset = None
        for row in construction:
            addr = int(row["table"], 0)
            if addr in marked_global:
                continue
            attachment = {
                "class": record.qualified_name,
                "address": addr,
                "base_class": base_class,
                "role": "construction",
                "receiver_offset": receiver_offset,
            }
            existing = chosen.get(addr)
            if existing is None:
                chosen[addr] = attachment
                continue
            if (
                existing["class"] != attachment["class"]
                or existing["base_class"] != attachment["base_class"]
            ):
                conflicts.add(addr)
    return [row for addr, row in sorted(chosen.items()) if addr not in conflicts]


def _canonical_matching_binary(repo_dir: Path, work_dir: Path) -> Path:
    """Same binary path pattern as ``commands.vtables._canonical_binary``."""

    import yaml

    config = yaml.safe_load((repo_dir / "config" / "variants.yml").read_text(encoding="utf-8"))
    target = config["canonical_matching_target"]
    return work_dir / "variants" / target["variant"] / target["module"]


def _raw_msvc_census(repo_dir: Path, work_dir: Path) -> tuple[str, dict[str, Any]]:
    """Scan (and process-cache) the canonical matching binary's MSVC tables."""

    binary = _canonical_matching_binary(repo_dir, work_dir)
    if binary.is_file():
        cache_key = sha256_file(binary)
    else:
        return str(binary), {}
    cached = _RAW_CENSUS_CACHE.get(cache_key)
    if cached is not None:
        return cache_key, cached
    from .msvc_tables import scan_msvc_tables

    report = scan_msvc_tables(binary, repo_dir=repo_dir)
    _RAW_CENSUS_CACHE[cache_key] = report
    return cache_key, report


def census_vftable_slot_counts(repo_dir: Path, work_dir: Path) -> dict[int, int]:
    """Confirmed census vftable ``slot_count`` by absolute address (process-cached)."""

    cache_key, report = _raw_msvc_census(repo_dir, work_dir)
    cached = _CENSUS_SLOT_CACHE.get(cache_key)
    if cached is not None:
        return cached
    counts = {
        int(row["address"], 0): int(row["slot_count"])
        for row in report.get("vftables", [])
        if row.get("address") is not None and row.get("slot_count") is not None
    }
    _CENSUS_SLOT_CACHE[cache_key] = counts
    return counts


def load_enriched_msvc_census(repo_dir: Path, work_dir: Path) -> dict[str, Any]:
    """Census plus receiver provenance, construction families, and source names."""

    binary = _canonical_matching_binary(repo_dir, work_dir)
    if not binary.is_file():
        return {}
    cache_key = sha256_file(binary)
    cached = _ENRICHED_CENSUS_CACHE.get(cache_key)
    if cached is not None:
        return cached
    from .msvc_table_analysis import enrich_msvc_table_report
    from .msvc_table_guard import sanitize_receiver_provenance
    from .msvc_table_source import annotate_source_identities

    _key, raw = _raw_msvc_census(repo_dir, work_dir)
    if not raw:
        _ENRICHED_CENSUS_CACHE[cache_key] = {}
        return {}
    report = deepcopy(raw)
    enrich_msvc_table_report(binary, report)
    sanitize_receiver_provenance(binary, report)
    annotate_source_identities(report, repo_dir)
    _ENRICHED_CENSUS_CACHE[cache_key] = report
    return report


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
    name = getattr(source, "name", None)
    if callable(name):
        try:
            name = name()
        except TypeError:
            name = None
    if name:
        return str(name)
    return str(source)


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


def _base_virtual_slot_definition(program: Any, base_class: str, index: int) -> Any | None:
    """FunctionDefinition for Base's primary vftable slot ``index``, if already typed."""

    _category, _name, path, _sigs = _vftable_paths(base_class)
    structure = program.getDataTypeManager().getDataType(path)
    if structure is None or not hasattr(structure, "getNumComponents"):
        return None
    if index < 0 or index >= int(structure.getNumComponents()):
        return None
    component = structure.getComponent(index)
    return _component_function_definition(component)


def _clone_slot_definition(
    _program: Any,
    definition: Any,
    *,
    qualified_class: str,
    field: str,
    base_class: str,
) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        FunctionDefinitionDataType,
    )

    _category, _name, _path, sigs_category = _vftable_paths(qualified_class, base_class=base_class)
    cloned = FunctionDefinitionDataType(definition)
    cloned.setName(f"{_sanitize_class_parts(qualified_class)[1]}_{field}")
    cloned.setCategoryPath(CategoryPath(sigs_category))
    return cloned


def _adjusted_receiver_type(
    program: Any, containing_class: str, offset: int, *, persist: bool = True
) -> Any | None:
    """PointerTypedef of ``containing_class *`` with a proven ComponentOffset.

    Used for secondary-subobject receivers. Never invents a generic ``-4``.
    Collect/plan never persists; apply adds the typedef inside a transaction.
    """

    if offset <= 0:
        return None
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        ComponentOffsetSettingsDefinition,
        DataTypeConflictHandler,
        PointerDataType,
        TypedefDataType,
    )

    pointed = _resolve_slot_type(program, containing_class)
    if pointed is None:
        return None
    if not persist:
        return None
    manager = program.getDataTypeManager()
    pointer = PointerDataType(pointed, manager)
    leaf = _simple_name(containing_class)
    name = f"{leaf}PtrOffset0x{offset:x}"
    category = pointer.getCategoryPath()
    raw = str(category.getPath()) if category is not None and hasattr(category, "getPath") else ""
    parts = [part for part in raw.split("/") if part]
    path = "/" + "/".join([*parts, name])
    existing = manager.getDataType(path)
    if existing is not None:
        return existing
    typedef = TypedefDataType(category, name, pointer)
    ComponentOffsetSettingsDefinition.DEF.setValue(typedef.getDefaultSettings(), offset)
    return manager.addDataType(typedef, DataTypeConflictHandler.REPLACE_HANDLER)


def _retarget_slot_this(
    program: Any,
    definition: Any,
    receiver_class: str,
    *,
    containing_class: str | None = None,
    offset: int | None = None,
    persist_types: bool = True,
) -> None:
    from ghidra.program.model.data import ParameterDefinitionImpl  # type: ignore[import-not-found]

    this_type = None
    if containing_class and isinstance(offset, int) and offset != 0:
        this_type = _adjusted_receiver_type(
            program, containing_class, offset, persist=persist_types
        )
    if this_type is None:
        this_type = _resolve_slot_type(program, f"{receiver_class} *")
    if this_type is None:
        raise ValueError(f"unresolved this type: {receiver_class} *")
    arguments = list(definition.getArguments()) if hasattr(definition, "getArguments") else []
    rewritten = [ParameterDefinitionImpl("this", this_type, None)]
    if arguments:
        first_name = str(arguments[0].getName() or "")
        rest = arguments[1:] if first_name in {"this", "param_0", "param0"} else arguments
        rewritten.extend(rest)
    definition.setArguments(rewritten)


def desired_slot_contract(
    program: Any,
    slot: Mapping[str, Any],
    *,
    qualified_class: str,
    field: str,
    base_class: str | None = None,
    phase: str | None = None,
    subobject_offset: int | None = None,
    persist_types: bool = True,
) -> Any:
    """FunctionDefinition for one slot. Construction and agreement share this.

    Preference: source declaration, else source-backed live function, else callee
    implementation. An unresolved *source* type is an error, not a silent demotion.
    Secondary tables use the Base virtual-slot contract; ``this`` is the Base
    subobject, or a proven Derived ComponentOffset when ``subobject_offset`` is known.
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
    _category, _structure_name, _path, sigs_category = _vftable_paths(
        qualified_class, base_class=base_class, phase=phase
    )
    declaration = slot.get("declaration")
    if base_class and phase != "construction":
        base_slot = _base_virtual_slot_definition(program, base_class, int(slot["index"]))
        if base_slot is not None:
            definition = _clone_slot_definition(
                program,
                base_slot,
                qualified_class=qualified_class,
                field=field,
                base_class=base_class,
            )
            _retarget_slot_this(
                program,
                definition,
                base_class,
                containing_class=qualified_class,
                offset=subobject_offset,
                persist_types=persist_types,
            )
            return definition
    if isinstance(declaration, Mapping):
        payload = dict(declaration)
        if base_class:
            payload["owning_class"] = base_class
            payload["has_this"] = True
        try:
            definition = _definition_from_declaration(
                program,
                definition_name=f"{_leaf}_{field}",
                sigs_category=sigs_category,
                declaration=payload,
            )
        except Exception as exc:
            raise ValueError(f"unresolved-source-type:{exc}") from exc
        if base_class:
            _retarget_slot_this(
                program,
                definition,
                base_class,
                containing_class=qualified_class,
                offset=subobject_offset,
                persist_types=persist_types,
            )
        return definition
    if function is None:
        raise ValueError("missing-callee")
    try:
        definition = FunctionDefinitionDataType(function, False)
    except Exception as exc:
        source = (
            "source-backed-signature-failed"
            if slot.get("fd_source") == "source-declaration"
            or _function_signature_source_backed(function)
            else "callee-signature-failed"
        )
        raise ValueError(f"{source}:{exc}") from exc
    if base_class:
        _retarget_slot_this(
            program,
            definition,
            base_class,
            containing_class=qualified_class,
            offset=subobject_offset,
            persist_types=persist_types,
        )
    return definition


def _slot_definitions_match_desired(
    program: Any,
    structure: Any,
    slots: Sequence[dict[str, Any]],
    qualified_class: str,
    *,
    base_class: str | None = None,
    phase: str | None = None,
    subobject_offset: int | None = None,
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
                program,
                slot,
                qualified_class=qualified_class,
                field=field,
                base_class=base_class,
                phase=phase,
                subobject_offset=subobject_offset,
                persist_types=False,
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
        if (record.vtable_address is not None or record.base_vtables)
        and (record.target is None or record.target.upper() == target.upper())
    ]
    if wanted is not None:
        classes = [
            record
            for record in classes
            if record.qualified_name in wanted or _simple_name(record.qualified_name) in wanted
        ]

    sibling_starts: set[int] = set()
    table_specs: list[dict[str, Any]] = []
    planned_addresses: set[int] = set()
    for record in classes:
        if record.vtable_address is not None:
            address = int(record.vtable_address)
            sibling_starts.add(address)
            planned_addresses.add(address)
            table_specs.append(
                {
                    "record": record,
                    "address": address,
                    "base_class": None,
                    "role": "primary",
                    "receiver_offset": None,
                }
            )
        for base_vtable in record.base_vtables:
            address = int(base_vtable.address)
            sibling_starts.add(address)
            planned_addresses.add(address)
            table_specs.append(
                {
                    "record": record,
                    "address": address,
                    "base_class": str(base_vtable.base_class),
                    "role": "base",
                    "receiver_offset": None,
                }
            )

    census_slots: dict[int, int] = {}
    census_tables: dict[int, Mapping[str, Any]] = {}
    if work_dir is not None:
        try:
            census_slots = census_vftable_slot_counts(repository, work_dir)
        except Exception:  # noqa: BLE001 — missing binary / scan failure → unbounded fallback
            census_slots = {}
        try:
            enriched = load_enriched_msvc_census(repository, work_dir)
        except Exception:  # noqa: BLE001 — enrichment is optional for marked tables
            enriched = {}
        census_tables = {
            int(row["address"], 0): row
            for row in (enriched.get("vftables") or [])
            if row.get("address") is not None
        }
        families = (enriched.get("analysis") or {}).get("construction_families") or []
        for attachment in construction_vtable_attachments(families, classes):
            address = int(attachment["address"])
            if address in planned_addresses:
                continue
            record = _lookup_source_class(classes, str(attachment["class"]))
            if record is None:
                continue
            sibling_starts.add(address)
            planned_addresses.add(address)
            table_specs.append(
                {
                    "record": record,
                    "address": address,
                    "base_class": attachment.get("base_class"),
                    "role": "construction",
                    "receiver_offset": attachment.get("receiver_offset"),
                }
            )

    # First pass: slot extents from MSVC census when confirmed; otherwise
    # unbounded read stopped at sibling table starts (not virtual_declarations).
    prepared: list[dict[str, Any]] = []
    for spec in sorted(
        table_specs,
        key=lambda item: (
            item["record"].qualified_name,
            item["address"],
            item.get("base_class") or "",
            item.get("role") or "",
        ),
    ):
        record = spec["record"]
        address = spec["address"]
        base_class = spec.get("base_class")
        role = str(spec.get("role") or ("base" if base_class else "primary"))
        phase = "construction" if role == "construction" else None
        census_row = census_tables.get(address)
        subobject_offset = unique_receiver_offset(
            census_row.get("writes") if census_row is not None else None
        )
        if subobject_offset is None and isinstance(spec.get("receiver_offset"), int):
            subobject_offset = spec["receiver_offset"]
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
            if slot.get("unresolved") or not slot.get("target"):
                continue
            field = _field_name(int(slot["index"]), slot.get("name"))
            try:
                desired_slot_contract(
                    program,
                    slot,
                    qualified_class=record.qualified_name,
                    field=field,
                    base_class=base_class,
                    phase=phase,
                    subobject_offset=subobject_offset,
                    persist_types=False,
                )
            except ValueError as exc:
                slot["contract_error"] = str(exc)
        prepared.append(
            {
                "record": record,
                "address": address,
                "base_class": base_class,
                "role": role,
                "phase": phase,
                "subobject_offset": subobject_offset,
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
        role = str(item.get("role") or ("base" if item.get("base_class") else "primary"))
        phase = item.get("phase")
        category, _vftable_name, vftable_path, _sigs = _vftable_paths(
            name, base_class=item.get("base_class"), phase=phase
        )
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
            and _slot_definitions_match_desired(
                program,
                existing,
                slots,
                name,
                base_class=item.get("base_class"),
                phase=phase,
                subobject_offset=item.get("subobject_offset"),
            )
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
                "base_class": item.get("base_class"),
                "role": role,
                "address": f"0x{address:08x}",
                "vftable": vftable_path,
                "vftable_category": category,
                "slot_count": len(slots),
                "declared_slots": declared,
                "census_slots": item.get("census_slots"),
                "subobject_offset": item.get("subobject_offset"),
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
    *,
    base_class: str | None = None,
    phase: str | None = None,
    subobject_offset: int | None = None,
) -> Any:
    """Build ``Pointer(FunctionDefinition)`` from ``desired_slot_contract``."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        PointerDataType,
        VoidDataType,
    )

    _category, _structure_name, _path, sigs_category = _vftable_paths(
        qualified_class, base_class=base_class, phase=phase
    )
    manager = program.getDataTypeManager()
    if slot.get("unresolved") or not slot.get("target"):
        slot["fd_source"] = None
        return PointerDataType(VoidDataType(), manager)
    definition = desired_slot_contract(
        program,
        slot,
        qualified_class=qualified_class,
        field=field,
        base_class=base_class,
        phase=phase,
        subobject_offset=subobject_offset,
    )
    definition.setName(f"{_sanitize_class_parts(qualified_class)[1]}_{field}")
    definition.setCategoryPath(CategoryPath(sigs_category))
    added = manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
    return PointerDataType(added, manager)


def _build_vftable_structure(
    program: Any,
    qualified_class: str,
    slots: list[dict[str, Any]],
    *,
    base_class: str | None = None,
    phase: str | None = None,
    subobject_offset: int | None = None,
) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
    )

    category, structure_name, _path, _sigs = _vftable_paths(
        qualified_class, base_class=base_class, phase=phase
    )
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
        pointer = _slot_function_pointer(
            program,
            qualified_class,
            slot,
            name,
            base_class=base_class,
            phase=phase,
            subobject_offset=subobject_offset,
        )
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


def _retarget_named_pointer_field(
    program: Any,
    class_name: str,
    pointed: Any,
    names: frozenset[str],
    *,
    offset: int,
) -> bool:
    """Retarget a named pointer field at ``offset`` on the bound class Structure."""

    from .class_binding import find_class_structure, find_ghidra_class

    ghidra_class = find_ghidra_class(program, class_name)
    if ghidra_class is None:
        return False
    structure = find_class_structure(program, ghidra_class)
    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return False
    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

    manager = program.getDataTypeManager()
    pointer = PointerDataType(pointed, manager)
    for component in structure.getDefinedComponents():
        field = component.getFieldName()
        if field not in names:
            continue
        if int(component.getOffset()) != offset:
            continue
        structure.replaceAtOffset(component.getOffset(), pointer, 4, field, component.getComment())
        return True
    return False


def _retarget_class_vfptr(program: Any, class_name: str, vftable: Any, *, offset: int = 0) -> bool:
    """Retarget ``vfptr``/``vptr`` at ``offset`` on the bound class Structure."""

    return _retarget_named_pointer_field(program, class_name, vftable, _VFPTR_NAMES, offset=offset)


def install_derived_base_view(
    program: Any,
    *,
    derived: str,
    base: str,
    offset: int,
    vftable: Any,
) -> bool:
    """Replace the Base component inside Derived with a Derived-specific subobject view.

    Canonical ``/Base`` is left untouched. The view copies Base's layout and points
    its vfptr at ``Derived_vftable_for_Base``.
    """

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        PointerDataType,
        StructureDataType,
    )

    from .class_binding import find_class_structure, find_ghidra_class

    derived_class = find_ghidra_class(program, derived)
    base_class = find_ghidra_class(program, base)
    derived_struct = find_class_structure(program, derived_class) if derived_class else None
    base_struct = find_class_structure(program, base_class) if base_class else None
    if derived_struct is None or base_struct is None:
        return False
    if not hasattr(derived_struct, "getDefinedComponents") or not hasattr(
        base_struct, "getDefinedComponents"
    ):
        return False
    component = next(
        (item for item in derived_struct.getDefinedComponents() if int(item.getOffset()) == offset),
        None,
    )
    if component is None:
        return False
    current = component.getDataType()
    current_name = (
        str(current.getName()) if current is not None and hasattr(current, "getName") else ""
    )
    base_name = str(base_struct.getName())
    current_path = (
        str(current.getPathName())
        if current is not None and hasattr(current, "getPathName")
        else ""
    )
    base_path = str(base_struct.getPathName())
    already_view = current_path.startswith("/wiz8/subobjects/")
    if current_path != base_path and current_name != base_name and not already_view:
        return False
    manager = program.getDataTypeManager()
    if already_view:
        from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

        if current is None or not hasattr(current, "getDefinedComponents"):
            return False
        pointer = PointerDataType(vftable, manager)
        for item in current.getDefinedComponents():
            if item.getFieldName() in _VFPTR_NAMES:
                current.replaceAtOffset(
                    int(item.getOffset()), pointer, 4, item.getFieldName(), item.getComment()
                )
                return True
        return True
    category, view_name, _path = _subobject_view_paths(derived, base, offset)
    manager.createCategory(CategoryPath(category))
    view = StructureDataType(CategoryPath(category), view_name, 0)
    pointer = PointerDataType(vftable, manager)
    for item in base_struct.getDefinedComponents():
        field = item.getFieldName()
        data_type = pointer if field in _VFPTR_NAMES else item.getDataType()
        view.insertAtOffset(
            int(item.getOffset()),
            data_type,
            int(item.getLength()),
            field,
            item.getComment(),
        )
    added = manager.addDataType(view, DataTypeConflictHandler.REPLACE_HANDLER)
    derived_struct.replaceAtOffset(
        offset, added, added.getLength(), component.getFieldName(), component.getComment()
    )
    return True


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
    base_class = row.get("base_class")
    role = str(row.get("role") or ("base" if base_class else "primary"))
    phase = "construction" if role == "construction" else None
    offset = row.get("subobject_offset")
    if action in {"create-and-apply", "upgrade-definitions"}:
        structure = _build_vftable_structure(
            program,
            qualified,
            slots,
            base_class=base_class,
            phase=phase,
            subobject_offset=offset if isinstance(offset, int) else None,
        )
    else:
        structure = program.getDataTypeManager().getDataType(row["vftable"])
    if structure is None:
        return {**dict(row), "error": "missing-vftable-structure"}
    try:
        _apply_data(program, int(row["address"], 0), structure)
    except ClearRangeError as exc:
        return {**dict(row), **exc.payload}
    subobject_view = False
    vfptr = False
    if role == "construction":
        pass
    elif role == "base" and isinstance(offset, int) and offset != 0 and base_class:
        subobject_view = install_derived_base_view(
            program,
            derived=qualified,
            base=str(base_class),
            offset=offset,
            vftable=structure,
        )
        if not subobject_view:
            vfptr = _retarget_class_vfptr(program, qualified, structure, offset=offset)
    elif role == "base":
        vfptr = False
    else:
        vfptr = _retarget_class_vfptr(program, qualified, structure, offset=0)
    return {
        "class": row["class"],
        "base_class": base_class,
        "role": role,
        "address": row["address"],
        "vftable": str(structure.getPathName()),
        "slot_count": structure.getNumComponents(),
        "subobject_offset": offset if isinstance(offset, int) else None,
        "vfptr_retargeted": vfptr,
        "subobject_view": subobject_view,
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
    vbtable_result: dict[str, Any] = {"applied": 0, "errors": [], "skipped": [], "vbtables": []}
    if plan.get("vbtables"):
        from .vbtable_typing import apply_vbtable_typing

        vbtable_result = apply_vbtable_typing(program, plan)
    skipped = skipped + list(vbtable_result.get("skipped") or [])
    return {
        "applied": result["applied"] + int(vbtable_result.get("applied") or 0),
        "errors": result["errors"] + list(vbtable_result.get("errors") or []),
        "skipped": skipped,
        "vftables": result["rows"],
        "vbtables": vbtable_result.get("vbtables") or [],
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
        from .vbtable_typing import collect_vbtable_typing_plan

        vb_plan = collect_vbtable_typing_plan(
            settings.repo_dir,
            program,
            work_dir=settings.work_dir,
            target=target,
            class_names=class_names,
            limit=limit,
        )
        plan["vbtables"] = vb_plan.get("vbtables") or []
        merged_counts = Counter(plan.get("counts") or {})
        for key, value in (vb_plan.get("counts") or {}).items():
            merged_counts[f"vbtable-{key}"] += int(value)
        plan["counts"] = dict(sorted(merged_counts.items()))
        plan["actionable"] = int(plan.get("actionable") or 0) + int(vb_plan.get("actionable") or 0)
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
            "vbtable_sample": (compact.get("vbtables") or [])[:15],
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
        result["vbtable_sample"] = (applied.get("vbtables") or [])[:15]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = repo_relative(error_path, settings.repo_dir)
        return result
