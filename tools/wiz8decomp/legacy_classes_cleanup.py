"""Gated cleanup of leftover ``/wiz8/classes`` enriched Structures.

Dry-run by default. Apply only with an explicit ``--apply`` (standalone) or
``--cleanup-legacy-classes`` under enrichment apply. Never rewrites vendor GZFs
or runs ``seed refresh``.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Iterable, Mapping, Sequence
from typing import Any, cast

from .class_binding import (
    find_class_structure,
    find_ghidra_class,
    resolve_class_binding,
)
from .class_structure_projection import _simple_name
from .config import Settings
from .datatype_contracts import (
    as_structure,
    is_legacy_path,
    structures_field_shape_agree,
    type_identity,
)
from .paths import atomic_json, repo_relative

_SCHEMA = "wiz8.legacy-classes-cleanup-v1"
_LEGACY_CATEGORY = "/wiz8/classes"


def _legacy_category_datatypes(program: Any) -> list[Any]:
    """Prefer category walk; fall back to scanning all DataTypes."""

    manager = program.getDataTypeManager()
    found: list[Any] = []
    seen: set[str] = set()
    get_cat = getattr(manager, "getCategory", None)
    if callable(get_cat):
        try:
            from ghidra.program.model.data import CategoryPath  # type: ignore[import-not-found]

            cat = manager.getCategory(CategoryPath(_LEGACY_CATEGORY))
        except Exception:  # noqa: BLE001
            cat = None
        if cat is not None:
            for data_type in cat.getDataTypes():
                path = str(data_type.getPathName())
                if path not in seen:
                    seen.add(path)
                    found.append(data_type)
            for child in cat.getCategories():
                for data_type in child.getDataTypes():
                    path = str(data_type.getPathName())
                    if path not in seen:
                        seen.add(path)
                        found.append(data_type)
    if found:
        return found
    get_all = getattr(manager, "getAllDataTypes", None)
    if not callable(get_all):
        return found
    for data_type in cast(Iterable[Any], get_all()):
        path = str(data_type.getPathName())
        if is_legacy_path(path) and path not in seen:
            seen.add(path)
            found.append(data_type)
    return found


def _is_pointer_wrapper_name(name: str) -> bool:
    return name.endswith((" *", "*"))


def _leaf_class_name(path: str) -> str:
    leaf = path.rsplit("/", 1)[-1]
    if leaf.endswith(" *"):
        leaf = leaf[:-2]
    elif leaf.endswith("*"):
        leaf = leaf[:-1].rstrip()
    return leaf


def _bound_for_legacy(
    program: Any,
    leaf: str,
    identity_map_or_bindings: Mapping[str, Mapping[str, Any]] | None,
) -> tuple[Any | None, str | None, str]:
    """Resolve non-legacy bound Structure for a legacy leaf name."""

    if identity_map_or_bindings:
        for key, row in identity_map_or_bindings.items():
            if key == leaf or _simple_name(key) == leaf:
                bound_path = row.get("bound_path") or row.get("structure_path")
                if bound_path and is_legacy_path(str(bound_path)):
                    return None, str(bound_path), "bound-still-legacy"
                if bound_path and not is_legacy_path(str(bound_path)):
                    bound = program.getDataTypeManager().getDataType(str(bound_path))
                    return bound, str(bound_path), "identity-map"
                status = str(row.get("status") or "")
                if status in {"missing-class", "no-bound", "legacy-enriched-path"}:
                    return None, None, status or "no-bound"
                return None, None, "no-bound"

    binding = resolve_class_binding(program, leaf)
    if binding.get("status") == "missing-class":
        return None, None, "missing-class"
    path = binding.get("structure_path")
    if path and is_legacy_path(str(path)):
        return None, str(path), "bound-still-legacy"
    if path:
        bound = program.getDataTypeManager().getDataType(str(path))
        return bound, str(path), "binding"
    ghidra_class = find_ghidra_class(program, leaf)
    if ghidra_class is None:
        return None, None, "no-bound"
    bound = find_class_structure(program, ghidra_class)
    if bound is None:
        return None, None, "no-bound"
    bound_path = str(bound.getPathName())
    if is_legacy_path(bound_path):
        return None, bound_path, "bound-still-legacy"
    return bound, bound_path, "binding"


def classify_legacy_datatype(
    program: Any,
    data_type: Any,
    *,
    identity_map_or_bindings: Mapping[str, Mapping[str, Any]] | None = None,
) -> dict[str, Any]:
    """Classify one ``/wiz8/classes`` DataType for cleanup."""

    path = str(data_type.getPathName())
    name = str(data_type.getName()) if hasattr(data_type, "getName") else _leaf_class_name(path)
    leaf = _leaf_class_name(path)
    structure = as_structure(data_type)

    if structure is None and not _is_pointer_wrapper_name(name):
        return {
            "path": path,
            "name": name,
            "action": "not-class",
            "bound_path": None,
            "reason": "not-structure",
        }

    bound, bound_path, how = _bound_for_legacy(program, leaf, identity_map_or_bindings)
    if bound_path is None or bound is None:
        action = "no-bound"
        if how == "bound-still-legacy":
            action = "conflict"
        return {
            "path": path,
            "name": name,
            "action": action,
            "bound_path": bound_path,
            "reason": how,
        }

    if is_legacy_path(bound_path):
        return {
            "path": path,
            "name": name,
            "action": "conflict",
            "bound_path": bound_path,
            "reason": "bound-still-legacy",
        }

    if structure is None:
        # Pointer wrapper named like ``X *`` — safe once bound exists.
        return {
            "path": path,
            "name": name,
            "action": "replace-then-delete",
            "bound_path": bound_path,
            "reason": "pointer-wrapper",
        }

    if int(structure.getLength()) != int(bound.getLength()):
        return {
            "path": path,
            "name": name,
            "action": "conflict",
            "bound_path": bound_path,
            "reason": "size-mismatch",
            "legacy_length": int(structure.getLength()),
            "bound_length": int(bound.getLength()),
        }

    same_shape = structures_field_shape_agree(structure, bound)
    exact_duplicate = same_shape or (
        type_identity(structure) != type_identity(bound)
        and int(structure.getLength()) == int(bound.getLength())
        and len(list(structure.getDefinedComponents())) == len(list(bound.getDefinedComponents()))
        and structures_field_shape_agree(structure, bound)
    )
    if not same_shape:
        return {
            "path": path,
            "name": name,
            "action": "conflict",
            "bound_path": bound_path,
            "reason": "field-shape-mismatch",
        }

    containing = 0
    get_containing = getattr(program.getDataTypeManager(), "getDataTypesContaining", None)
    if callable(get_containing):
        try:
            containing = len(list(cast(Iterable[Any], get_containing(data_type))))
        except Exception:  # noqa: BLE001
            containing = -1

    action = "safe-delete" if containing == 0 else "replace-then-delete"
    return {
        "path": path,
        "name": name,
        "action": action,
        "bound_path": bound_path,
        "reason": "exact-duplicate" if exact_duplicate else "shape-agree",
        "containing": containing,
    }


def collect_legacy_classes_cleanup_plan(
    program: Any,
    *,
    identity_map_or_bindings: Mapping[str, Mapping[str, Any]] | None = None,
) -> dict[str, Any]:
    """Inventory every DataType under ``/wiz8/classes`` and classify deletion safety."""

    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    datatypes = _legacy_category_datatypes(program)
    if not datatypes:
        counts["category-empty"] = 1
        return {
            "schema": _SCHEMA,
            "counts": dict(sorted(counts.items())),
            "actionable": 0,
            "types": [],
        }

    for data_type in sorted(datatypes, key=lambda dt: str(dt.getPathName())):
        row = classify_legacy_datatype(
            program, data_type, identity_map_or_bindings=identity_map_or_bindings
        )
        counts[str(row["action"])] += 1
        rows.append(row)

    actionable = counts["safe-delete"] + counts["replace-then-delete"]
    return {
        "schema": _SCHEMA,
        "counts": dict(sorted(counts.items())),
        "actionable": actionable,
        "types": rows,
    }


def apply_legacy_classes_cleanup(
    program: Any,
    plan: Mapping[str, Any],
) -> dict[str, Any]:
    """Replace containing refs then remove safe legacy types (one transaction each)."""

    from ghidra.util.task import TaskMonitor  # type: ignore[import-not-found]

    from .ghidra.mutations import apply_rows

    def _apply_one(_program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
        action = row.get("action")
        if action not in {"safe-delete", "replace-then-delete"}:
            return {**dict(row), "error": f"refused:{action}"}
        bound_path = row.get("bound_path")
        if not bound_path or is_legacy_path(str(bound_path)):
            return {**dict(row), "error": "bound-path-legacy-or-missing"}
        manager = _program.getDataTypeManager()
        legacy = manager.getDataType(str(row["path"]))
        bound = manager.getDataType(str(bound_path))
        if legacy is None:
            return {**dict(row), "error": "legacy-missing"}
        if bound is None:
            return {**dict(row), "error": "bound-missing"}
        if as_structure(legacy) is not None and as_structure(bound) is not None:
            if int(legacy.getLength()) != int(bound.getLength()):
                return {**dict(row), "error": "size-mismatch"}
            if not structures_field_shape_agree(legacy, bound):
                return {**dict(row), "error": "field-shape-mismatch"}
        if action == "replace-then-delete":
            manager.replaceDataType(legacy, bound, True)
        removed = manager.remove(legacy, TaskMonitor.DUMMY)
        if not removed:
            return {**dict(row), "error": "remove-failed"}
        return {
            "path": row["path"],
            "action": action,
            "bound_path": bound_path,
            "removed": True,
        }

    rows = [
        row
        for row in plan.get("types", [])
        if row.get("action") in {"safe-delete", "replace-then-delete"}
    ]
    result = apply_rows(
        program,
        rows,
        _apply_one,
        description="Legacy /wiz8/classes cleanup",
    )

    # Best-effort empty category removal after successful deletes.
    try:
        from ghidra.program.model.data import CategoryPath  # type: ignore[import-not-found]

        manager = program.getDataTypeManager()
        cat = manager.getCategory(CategoryPath(_LEGACY_CATEGORY))
        if cat is not None and len(list(cat.getDataTypes())) == 0:
            parent = cat.getParent()
            if parent is not None:
                parent.removeCategory(cat.getName(), TaskMonitor.DUMMY)
    except Exception:  # noqa: BLE001, S110
        pass

    return {"applied": result["applied"], "errors": result["errors"], "types": result["rows"]}


def run_legacy_classes_cleanup(
    settings: Settings,
    *,
    program_name: str = "wiz8",
    apply: bool = False,
    identity_map_or_bindings: Mapping[str, Mapping[str, Any]] | None = None,
    class_names: Sequence[str] | None = None,
) -> dict[str, Any]:
    """Report or apply legacy ``/wiz8/classes`` cleanup."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    del class_names  # reserved for future filtering; inventory is category-wide

    with open_program(settings, program_name) as program:
        bindings = identity_map_or_bindings
        if bindings is None:
            # Prefer a fresh type-graph identity map when available.
            try:
                from .type_graph_projection import build_identity_map

                bindings = build_identity_map(settings.repo_dir, program)
            except Exception:  # noqa: BLE001
                bindings = None
        plan = collect_legacy_classes_cleanup_plan(program, identity_map_or_bindings=bindings)
        out_dir = settings.build_dir / "legacy-classes-cleanup"
        report_path = out_dir / "report.json"
        atomic_json(report_path, {**plan, "program": program_name, "apply": apply})
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": repo_relative(report_path, settings.repo_dir),
            "sample": plan["types"][:20],
        }
        if not apply:
            return result

        applied = apply_legacy_classes_cleanup(program, plan)
        dispose_sessions()
        program.save("Remove legacy /wiz8/classes Structures", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["sample"] = applied["types"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = repo_relative(error_path, settings.repo_dir)
        return result
