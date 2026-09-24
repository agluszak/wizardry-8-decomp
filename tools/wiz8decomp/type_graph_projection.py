"""Two-phase type-graph remapper onto bound ``class_binding`` Structures.

Never writes identity under ``/wiz8/classes``. Phase 1 builds the class
identity map and opaque shells; phase 2 reconciles fields and remaps nested
references through that map.

Plan schema (``wiz8.type-graph-projection-v1``)::

    {
      "schema": "wiz8.type-graph-projection-v1",
      "target": "WIZ8",
      "identity_map": {
        "<class>": {
          "ghidra_class": str | null,
          "bound_path": str | null,
          "evidence_path": str | null,
          "status": "agree" | "bind-existing" | "create-opaque" | "conflict"
                   | "no-layout-evidence" | "legacy-duplicate" | "missing-class",
          "asserted_size": int | null,
          "field_action": "agree" | "reconcile-fields" | "remap-nested"
                        | "conflict" | "skip" | null
        }
      },
      "counts": {...},
      "field_counts": {...},
      "actionable": int,
      "classes": [ ... actionable identity / field rows ... ]
    }
"""

from __future__ import annotations

import hashlib
import json
from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .class_binding import (
    ensure_ghidra_class,
    find_class_structure,
    find_ghidra_class,
    legacy_enriched_structure,
)
from .class_structure_projection import (
    _as_structure,
    _classes_by_name,
    _create_opaque,
    _decide_structure_action,
    _find_named_structure,
    _simple_name,
    _thiscall_owning_classes,
)
from .datatype_contracts import (
    datatype_shape_key,
    has_legacy_nested_ref,
    is_legacy_path,
    is_opaque_structure,
    is_rich_structure,
    settings_typedef_blocks_remap,
    structures_field_shape_agree,
    type_identity,
    unwrap_plain_typedefs,
    walk_datatype_refs,
)
from .source_index import SourceIndex, load_source_index

_SCHEMA = "wiz8.type-graph-projection-v1"


class TypeGraphConflict(RuntimeError):
    """Refuse a remap that would drop ABI-relevant typedef/union settings."""


def plan_semantic_hash(plan: Mapping[str, Any]) -> str:
    """Order-independent hash of identity statuses + field actions."""

    identity = plan.get("identity_map") or {}
    payload = {
        key: {
            "status": row.get("status"),
            "bound_path": row.get("bound_path"),
            "evidence_path": row.get("evidence_path"),
            "field_action": row.get("field_action"),
            "asserted_size": row.get("asserted_size"),
        }
        for key, row in sorted(identity.items())
    }
    encoded = json.dumps(payload, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(encoded.encode("utf-8")).hexdigest()


def _asserted_size_classes(index: SourceIndex) -> dict[str, int]:
    sizes: dict[str, int] = {}
    for record in index.classes.values():
        if record.asserted_size:
            sizes[str(record.qualified_name)] = int(record.asserted_size)
    return sizes


def _selected_identities(
    repository: Path,
    target: str,
    *,
    class_names: Sequence[str] | None = None,
) -> list[str]:
    index = SourceIndex.from_dict(load_source_index(repository))
    owning = set(_thiscall_owning_classes(repository, target))
    sized = set(_asserted_size_classes(index))
    selected = owning | sized
    if class_names is not None:
        wanted = {_simple_name(name) for name in class_names} | set(class_names)
        selected = {name for name in selected if name in wanted or _simple_name(name) in wanted}
    return sorted(selected, key=lambda name: (_simple_name(name), name))


def _evidence_structure(
    program: Any,
    owning_class: str,
    *,
    asserted_size: int | None,
) -> Any | None:
    """Preferred non-legacy Structure used as field evidence."""

    return _find_named_structure(program, owning_class, asserted_size=asserted_size)


def decide_field_action(
    bound: Any | None,
    evidence: Any | None,
    *,
    nested_legacy: bool | None = None,
    identity_map: Mapping[str, Mapping[str, Any]] | None = None,
) -> str | None:
    """Phase-2 action for one identity with bound + evidence Structures."""

    if bound is None or evidence is None:
        return None
    if settings_typedef_blocks_remap(bound) or settings_typedef_blocks_remap(evidence):
        return "conflict"
    if bound is evidence or type_identity(bound) == type_identity(evidence):
        legacy = has_legacy_nested_ref(bound) if nested_legacy is None else nested_legacy
        return "remap-nested" if legacy else "agree"
    if is_opaque_structure(bound) and is_rich_structure(evidence):
        if int(bound.getLength()) != int(evidence.getLength()):
            return "conflict"
        return "reconcile-fields"
    if int(bound.getLength()) != int(evidence.getLength()):
        return "conflict"
    if not structures_field_shape_agree(bound, evidence, identity_map=identity_map):
        return "conflict"
    legacy = has_legacy_nested_ref(bound) if nested_legacy is None else nested_legacy
    if not legacy and has_legacy_nested_ref(evidence):
        legacy = True
    if legacy:
        return "remap-nested"
    return "agree"


def _class_key_from_path(
    path: str,
    *,
    source_identities: set[str] | None = None,
) -> str | None:
    """Map a datatype path to a source class identity.

    ``/wiz8/classes/...`` is our own encoding. Arbitrary Ghidra/PDB/Demangler
    category paths are organization, not C++ namespaces — resolve them against
    known source identities instead of synthesizing ``foo::bar::Baz``.
    """

    if not path:
        return None
    text = str(path)
    while text.endswith("*"):
        text = text[:-1].rstrip()
    text = text.removesuffix(" *")
    if is_legacy_path(text):
        rest = text[len("/wiz8/classes/") :]
        return rest.replace("/", "::") if rest else None
    if not source_identities:
        return None
    if text.startswith("/"):
        encoded = {identity: "/" + identity.replace("::", "/") for identity in source_identities}
        for identity, encoded_path in encoded.items():
            if text == encoded_path or text == f"/{identity}":
                return identity
    leaf = text.rsplit("/", 1)[-1].removesuffix(" *")
    if leaf in source_identities:
        return leaf
    qualified = [key for key in source_identities if key.endswith("::" + leaf)]
    if len(qualified) == 1:
        return qualified[0]
    return None


def _discover_nested_structure_names(
    evidence: Any | None,
    *,
    source_identities: set[str],
    already: set[str],
) -> set[str]:
    discovered: set[str] = set()
    if evidence is None:
        return discovered
    for nested in walk_datatype_refs(evidence):
        if not hasattr(nested, "getDefinedComponents"):
            continue
        if hasattr(nested, "getArguments"):
            continue
        path = type_identity(nested)
        key = _class_key_from_path(path, source_identities=source_identities)
        if not key or key in already or key in discovered:
            continue
        discovered.add(key)
    return discovered


def build_identity_map(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    class_names: Sequence[str] | None = None,
    ensure_classes: bool = False,
) -> dict[str, dict[str, Any]]:
    """Phase 1: class identity → bound / evidence / status (no field writes)."""

    index = SourceIndex.from_dict(load_source_index(repository))
    classes = _classes_by_name(index)
    sizes = _asserted_size_classes(index)
    selected = _selected_identities(repository, target, class_names=class_names)
    known = {_simple_name(name) for name in selected} | set(selected)
    source_identities = set(classes)
    identity: dict[str, dict[str, Any]] = {}

    queue = list(selected)
    seen: set[str] = set()
    while queue:
        owning = queue.pop(0)
        if owning in seen:
            continue
        seen.add(owning)
        source_class = classes.get(owning) or classes.get(_simple_name(owning))
        asserted = (
            int(source_class.asserted_size)
            if source_class and source_class.asserted_size
            else sizes.get(owning) or sizes.get(_simple_name(owning))
        )
        ghidra_class = (
            ensure_ghidra_class(program, owning)
            if ensure_classes
            else find_ghidra_class(program, owning)
        )
        if ghidra_class is None and not ensure_classes:
            legacy = legacy_enriched_structure(program, owning)
            evidence = _evidence_structure(program, owning, asserted_size=asserted)
            identity[owning] = {
                "ghidra_class": None,
                "bound_path": None,
                "evidence_path": (str(evidence.getPathName()) if evidence is not None else None),
                "status": "missing-class",
                "asserted_size": asserted,
                "field_action": None,
                "legacy_enriched_path": (str(legacy.getPathName()) if legacy is not None else None),
            }
            continue

        bound = _as_structure(find_class_structure(program, ghidra_class))
        legacy = legacy_enriched_structure(program, owning)
        evidence = _evidence_structure(program, owning, asserted_size=asserted)
        source_size_ok = (
            evidence is None or asserted is None or int(evidence.getLength()) == asserted
        )
        size_mismatched_source = None
        if asserted is not None and evidence is None:
            candidate = _find_named_structure(program, owning, asserted_size=None)
            if candidate is not None and int(candidate.getLength()) != asserted:
                size_mismatched_source = candidate

        status = _decide_structure_action(
            bound=bound,
            legacy=legacy,
            source=evidence,
            asserted_size=asserted,
            source_size_ok=source_size_ok,
            size_mismatched_source=size_mismatched_source,
        )
        # Prefer reporting missing-class only when find failed; ensure path never
        # writes under /wiz8/classes.
        if bound is not None and is_legacy_path(str(bound.getPathName())):
            status = "conflict"
        if (
            status == "bind-existing"
            and bound is not None
            and evidence is not None
            and str(bound.getPathName()) != str(evidence.getPathName())
        ):
            status = "conflict"

        report_evidence = evidence if evidence is not None else size_mismatched_source
        identity[owning] = {
            "ghidra_class": (
                str(ghidra_class.getName(True))
                if ghidra_class is not None and hasattr(ghidra_class, "getName")
                else (str(owning) if ghidra_class is not None else None)
            ),
            "bound_path": str(bound.getPathName()) if bound is not None else None,
            "evidence_path": (
                str(report_evidence.getPathName()) if report_evidence is not None else None
            ),
            "status": status,
            "asserted_size": asserted,
            "field_action": None,
            "legacy_enriched_path": (str(legacy.getPathName()) if legacy is not None else None),
        }

        for nested_name in _discover_nested_structure_names(
            report_evidence, source_identities=source_identities, already=known
        ):
            if nested_name not in seen:
                known.add(nested_name)
                queue.append(nested_name)

    return identity


def _attach_field_actions(
    program: Any,
    identity: dict[str, dict[str, Any]],
) -> Counter[str]:
    """Phase 2: decide field reconcile / remap / conflict per identity."""

    counts: Counter[str] = Counter()
    for owning, row in identity.items():
        status = row.get("status")
        if status in {"missing-class", "no-layout-evidence", "conflict"}:
            row["field_action"] = None
            continue
        ghidra_class = find_ghidra_class(program, owning)
        bound = (
            _as_structure(find_class_structure(program, ghidra_class))
            if ghidra_class is not None
            else None
        )
        evidence = None
        evidence_path = row.get("evidence_path")
        if evidence_path:
            evidence = _as_structure(program.getDataTypeManager().getDataType(evidence_path))
        if evidence is None:
            evidence = _evidence_structure(program, owning, asserted_size=row.get("asserted_size"))
        # Agreeing / legacy-duplicate identities still need nested legacy walks.
        action = decide_field_action(bound, evidence, identity_map=identity)
        row["field_action"] = action
        if action:
            counts[action] += 1
    return counts


def collect_type_graph_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    class_names: Sequence[str] | None = None,
) -> dict[str, Any]:
    """Build dry-run identity + field plan (no GhidraClass creation)."""

    identity = build_identity_map(
        repository,
        program,
        target=target,
        class_names=class_names,
        ensure_classes=False,
    )
    field_counts = _attach_field_actions(program, identity)
    status_counts: Counter[str] = Counter()
    rows: list[dict[str, Any]] = []
    for owning, row in sorted(identity.items(), key=lambda item: item[0]):
        status_counts[str(row["status"])] += 1
        actionable_identity = row["status"] in {"bind-existing", "create-opaque"}
        actionable_fields = row.get("field_action") in {"reconcile-fields", "remap-nested"}
        if not (
            actionable_identity
            or actionable_fields
            or row["status"] == "conflict"
            or row.get("field_action") == "conflict"
        ):
            continue
        rows.append({"class": owning, **row})

    actionable = (
        status_counts["bind-existing"]
        + status_counts["create-opaque"]
        + field_counts["reconcile-fields"]
        + field_counts["remap-nested"]
    )
    plan = {
        "schema": _SCHEMA,
        "target": target,
        "identity_map": identity,
        "counts": dict(sorted(status_counts.items())),
        "field_counts": dict(sorted(field_counts.items())),
        "actionable": actionable,
        "classes": rows,
    }
    plan["plan_hash"] = plan_semantic_hash(plan)
    return plan


def _remap_datatype(program: Any, data_type: Any, identity: Mapping[str, Mapping[str, Any]]) -> Any:
    """Rewrite nested refs through the identity map (bound paths win)."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        Array,
        ArrayDataType,
        FunctionDefinition,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        Pointer,
        PointerDataType,
        Structure,
        TypeDef,
        Union,
        UnionDataType,
    )

    current = data_type
    if current is None:
        return None

    # Preserve settings-bearing typedefs. If they still hide a legacy type,
    # refuse to remap rather than silently leaving the graph dirty.
    if isinstance(current, TypeDef):
        unwrapped = unwrap_plain_typedefs(current)
        if unwrapped is current:
            if settings_typedef_blocks_remap(current):
                raise TypeGraphConflict("settings-typedef-legacy")
            return current
        remapped = _remap_datatype(program, unwrapped, identity)
        return remapped

    if isinstance(current, Pointer):
        pointee = current.getDataType()
        remapped = _remap_datatype(program, pointee, identity)
        if remapped is pointee:
            return current
        return PointerDataType(remapped, program.getDataTypeManager())

    if isinstance(current, Array):
        element = current.getDataType()
        remapped = _remap_datatype(program, element, identity)
        if remapped is element:
            return current
        return ArrayDataType(remapped, current.getNumElements(), remapped.getLength())

    if isinstance(current, FunctionDefinition):
        ret = _remap_datatype(program, current.getReturnType(), identity)
        args = []
        changed = ret is not current.getReturnType()
        for arg in current.getArguments():
            new_dt = _remap_datatype(program, arg.getDataType(), identity)
            if new_dt is not arg.getDataType():
                changed = True
            args.append(ParameterDefinitionImpl(arg.getName(), new_dt, arg.getComment()))
        if not changed:
            return current
        clone = FunctionDefinitionDataType(current.getCategoryPath(), current.getName())
        clone.setReturnType(ret)
        if args:
            clone.setArguments(args)
        if current.getCallingConvention() is not None:
            clone.setCallingConvention(current.getCallingConvention())
        if hasattr(current, "hasVarArgs") and hasattr(clone, "setVarArgs"):
            clone.setVarArgs(current.hasVarArgs())
        if hasattr(current, "hasNoReturn") and hasattr(clone, "setNoReturn"):
            clone.setNoReturn(current.hasNoReturn())
        return clone

    if isinstance(current, Union):
        components = (
            list(current.getDefinedComponents())
            if hasattr(current, "getDefinedComponents")
            else list(current.getComponents())
        )
        remapped_members: list[tuple[Any, Any]] = []
        changed = False
        for component in components:
            new_dt = _remap_datatype(program, component.getDataType(), identity)
            if new_dt is not component.getDataType():
                changed = True
            remapped_members.append((new_dt, component))
        if not changed:
            return current
        clone = UnionDataType(current.getCategoryPath(), current.getName())
        for new_dt, component in remapped_members:
            clone.add(
                new_dt,
                component.getLength(),
                component.getFieldName(),
                component.getComment() if hasattr(component, "getComment") else None,
            )
        return clone

    if isinstance(current, Structure):
        path = str(current.getPathName())
        for key, row in identity.items():
            bound_path = row.get("bound_path")
            if not bound_path or is_legacy_path(str(bound_path)):
                continue
            if path == bound_path:
                return current
            if (
                path == row.get("evidence_path")
                or _class_key_from_path(path, source_identities=set(identity)) == key
            ):
                bound = program.getDataTypeManager().getDataType(bound_path)
                if bound is not None:
                    return bound
        return current

    # Legacy path by qualified identity, never leaf-only.
    path = type_identity(current)
    if is_legacy_path(path):
        key = _class_key_from_path(path)
        row = identity.get(key) if key else None
        if row is not None:
            bound_path = row.get("bound_path")
            if bound_path and not is_legacy_path(bound_path):
                bound = program.getDataTypeManager().getDataType(bound_path)
                if bound is not None:
                    return bound
    return current


def _rebuild_fields_from_evidence(
    bound: Any,
    evidence: Any,
    program: Any,
    identity: Mapping[str, Mapping[str, Any]],
) -> None:
    """Replace bound field layout within the established extent."""

    original_length = int(bound.getLength())
    evidence_length = int(evidence.getLength())
    if original_length != evidence_length:
        raise TypeGraphConflict(f"structure-length-mismatch:{original_length}->{evidence_length}")
    desired: list[tuple[int, int, Any, str, Any]] = []
    for component in evidence.getDefinedComponents():
        remapped = _remap_datatype(program, component.getDataType(), identity)
        offset = int(component.getOffset())
        length = int(component.getLength())
        if offset < 0 or length <= 0:
            raise TypeGraphConflict(f"invalid-field:{offset}+{length}")
        if offset + length > original_length:
            raise TypeGraphConflict(f"field-exceeds-extent:{offset}+{length}>{original_length}")
        desired.append(
            (
                offset,
                length,
                remapped,
                component.getFieldName(),
                component.getComment(),
            )
        )
    desired.sort(key=lambda row: row[0])
    previous_end = 0
    for offset, length, _data_type, _name, _comment in desired:
        if offset < previous_end:
            raise TypeGraphConflict(f"field-overlap:{offset}")
        previous_end = offset + length
    components = list(bound.getDefinedComponents())
    for component in sorted(components, key=lambda item: item.getOffset(), reverse=True):
        if hasattr(bound, "clearAtOffset"):
            bound.clearAtOffset(component.getOffset())
        else:
            bound.deleteAtOffset(component.getOffset())
            if hasattr(bound, "growStructure") and int(bound.getLength()) < original_length:
                bound.growStructure(original_length - int(bound.getLength()))
    for offset, length, remapped, name, comment in desired:
        bound.replaceAtOffset(offset, remapped, length, name, comment)
    if int(bound.getLength()) != original_length:
        raise TypeGraphConflict(
            f"structure-length-changed:{original_length}->{int(bound.getLength())}"
        )
    expected = {offset: length for offset, length, _data_type, _name, _comment in desired}
    actual = {
        int(component.getOffset()): int(component.getLength())
        for component in bound.getDefinedComponents()
    }
    if actual != expected:
        raise TypeGraphConflict("structure-offsets-changed")


def _remap_structure_fields(
    bound: Any,
    program: Any,
    identity: Mapping[str, Mapping[str, Any]],
) -> int:
    """Replace nested field types that still point at legacy / remappable paths."""

    changed = 0
    for component in list(bound.getDefinedComponents()):
        current = component.getDataType()
        remapped = _remap_datatype(program, current, identity)
        if remapped is current:
            continue
        if datatype_shape_key(remapped, identity_map=identity) == datatype_shape_key(
            current, identity_map=identity
        ):
            continue
        bound.replaceAtOffset(
            component.getOffset(),
            remapped,
            component.getLength(),
            component.getFieldName(),
            component.getComment(),
        )
        changed += 1
    return changed


def apply_type_graph_projection(
    program: Any,
    plan: Mapping[str, Any],
) -> dict[str, Any]:
    """Apply identity shells + field reconcile/remap via ``apply_rows``."""

    from .ghidra.mutations import apply_rows

    identity = dict(plan.get("identity_map") or {})

    def _apply_one(_program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
        owning = str(row["class"])
        status = row.get("status")
        field_action = row.get("field_action")

        if status == "create-opaque":
            size = int(row.get("asserted_size") or 0)
            if size <= 1:
                return {**dict(row), "error": "invalid-asserted-size"}
            result = _create_opaque(_program, owning, size)
            identity[owning] = {
                **identity.get(owning, {}),
                "bound_path": str(result.getPathName()),
                "status": "agree",
                "ghidra_class": owning,
            }
            return {
                "class": owning,
                "action": "create-opaque",
                "path": str(result.getPathName()),
                "length": result.getLength(),
            }

        if status == "bind-existing":
            ghidra_class = ensure_ghidra_class(_program, owning)
            result = find_class_structure(_program, ghidra_class)
            if result is None:
                return {**dict(row), "error": "missing-bound-structure"}
            if is_legacy_path(str(result.getPathName())):
                return {**dict(row), "error": "bound-path-legacy"}
            evidence_path = row.get("evidence_path")
            if evidence_path and str(result.getPathName()) != str(evidence_path):
                return {
                    **dict(row),
                    "error": "bind-path-mismatch",
                    "bound_path": str(result.getPathName()),
                    "evidence_path": evidence_path,
                }
            identity[owning] = {
                **identity.get(owning, {}),
                "bound_path": str(result.getPathName()),
                "status": "agree",
                "ghidra_class": str(ghidra_class.getName(True)),
            }
            return {
                "class": owning,
                "action": "bind-existing",
                "path": str(result.getPathName()),
                "length": result.getLength(),
            }

        ghidra_class = ensure_ghidra_class(_program, owning)
        bound = _as_structure(find_class_structure(_program, ghidra_class))
        if bound is None:
            return {**dict(row), "error": "missing-bound-structure"}
        if is_legacy_path(str(bound.getPathName())):
            return {**dict(row), "error": "bound-path-legacy"}

        evidence = None
        evidence_path = row.get("evidence_path")
        if evidence_path:
            evidence = _as_structure(_program.getDataTypeManager().getDataType(evidence_path))
        if evidence is None:
            evidence = _evidence_structure(_program, owning, asserted_size=row.get("asserted_size"))

        if field_action == "reconcile-fields":
            if evidence is None:
                return {**dict(row), "error": "missing-evidence"}
            if int(bound.getLength()) != int(evidence.getLength()):
                return {**dict(row), "error": "size-mismatch"}
            try:
                _rebuild_fields_from_evidence(bound, evidence, _program, identity)
            except TypeGraphConflict as exc:
                return {**dict(row), "error": str(exc) or "settings-typedef-legacy"}
            return {
                "class": owning,
                "action": "reconcile-fields",
                "path": str(bound.getPathName()),
                "components": len(list(bound.getDefinedComponents())),
            }

        if field_action == "remap-nested":
            try:
                changed = _remap_structure_fields(bound, _program, identity)
            except TypeGraphConflict as exc:
                return {**dict(row), "error": str(exc) or "settings-typedef-legacy"}
            return {
                "class": owning,
                "action": "remap-nested",
                "path": str(bound.getPathName()),
                "changed_fields": changed,
            }

        return {**dict(row), "error": f"unexpected-action:{status}/{field_action}"}

    rows: list[dict[str, Any]] = []
    for owning, row in sorted((plan.get("identity_map") or {}).items()):
        status = row.get("status")
        field_action = row.get("field_action")
        if status in {"create-opaque", "bind-existing"} or field_action in {
            "reconcile-fields",
            "remap-nested",
        }:
            rows.append({"class": owning, **row})

    result = apply_rows(
        program,
        rows,
        _apply_one,
        description="Type-graph projection",
    )
    return {"applied": result["applied"], "errors": result["errors"], "classes": result["rows"]}
