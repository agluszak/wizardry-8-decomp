"""Reconcile class Structures with Ghidra's native class-namespace binding.

reccmp places ordinary class Structures at a namespace-based category path
(typically ``/ClassName`` under the global namespace) and creates a matching
``GhidraClass``. Ghidra's automatic ``this`` depends
on that association.

The projection reports bound source classes and creates opaque sized shells
in the class category when an asserted size is known.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex

from .class_binding import (
    ensure_ghidra_class,
    find_class_structure,
    find_ghidra_class,
)
from .global_model import parse_global_definitions
from .global_typing import _ghidra_type_name, _simple_name
from .source_index import load_source_index, source_functions

_SCHEMA = "wiz8.class-structure-projection-v1"


def _classes_by_name(index: SourceIndex, target: str) -> dict[str, Any]:
    by_name: dict[str, Any] = {}
    simple_names: dict[str, list[Any]] = {}
    for key, record in index.classes.items():
        if key.target != target:
            continue
        by_name[record.qualified_name] = record
        simple_names.setdefault(_simple_name(record.qualified_name), []).append(record)
    for simple, records in simple_names.items():
        if len(records) == 1:
            by_name.setdefault(simple, records[0])
    return by_name


def _thiscall_owning_classes(repository: Path, target: str) -> Counter[str]:
    counts: Counter[str] = Counter()
    for marker in source_functions(repository, target).values():
        if marker.marker_kind != "FUNCTION" or marker.declaration is None:
            continue
        if marker.declaration.calling_convention != "__thiscall":
            continue
        owning = marker.declaration.owning_class
        if owning:
            counts[str(owning)] += 1
    return counts


def _referenced_template_classes(repository: Path, target: str, index: SourceIndex) -> Counter[str]:
    """Trusted template layouts used by address-bound prototypes or globals."""

    templates = {
        record.qualified_name: "".join(_ghidra_type_name(record.qualified_name).split())
        for key, record in index.classes.items()
        if key.target == target
        and "<" in record.qualified_name
        and record.layout_trusted
        and record.size
    }
    counts: Counter[str] = Counter()
    if not templates:
        return counts
    for marker in source_functions(repository, target).values():
        declaration = marker.declaration
        if marker.marker_kind != "FUNCTION" or declaration is None:
            continue
        signature = "".join(
            _ghidra_type_name(
                " ".join((declaration.return_type or "", *declaration.parameter_types))
            ).split()
        )
        for name, spelling in templates.items():
            if spelling in signature:
                counts[name] += 1
    for definition in parse_global_definitions(repository):
        if definition.get("target") != target:
            continue
        source_type = "".join(_ghidra_type_name(str(definition.get("type") or "")).split())
        for name, spelling in templates.items():
            if spelling in source_type:
                counts[name] += 1
    return counts


def _source_size(source_class: Any | None) -> int | None:
    if source_class is None:
        return None
    if source_class.asserted_size:
        return int(source_class.asserted_size)
    if "<" in source_class.qualified_name and source_class.layout_trusted and source_class.size:
        return int(source_class.size)
    return None


def _as_structure(data_type: Any) -> Any | None:
    from .datatype_contracts import as_structure

    return as_structure(data_type)


def _structure_path_tier(path: str, simple: str, name: str) -> int:
    """Lower is better: source-identity category path, then weaker fallbacks."""

    identity = "/" + name.replace("::", "/")
    if path == identity:
        return 0
    if name != simple and path == f"/{name}":
        return 1
    if path == f"/{simple}":
        return 0 if name == simple else 2
    if path == f"/Demangler/{simple}":
        return 3
    return 4


def _identity_category_paths(name: str) -> list[str]:
    """Exact Ghidra category encodings of a source identity, then weaker lookups."""

    simple = _simple_name(name)
    paths: list[str] = ["/" + name.replace("::", "/")]
    if name != simple:
        paths.append(f"/{name}")
    if f"/{simple}" not in paths:
        paths.append(f"/{simple}")
    paths.append(f"/Demangler/{simple}")
    unique: list[str] = []
    seen: set[str] = set()
    for path in paths:
        if path not in seen:
            seen.add(path)
            unique.append(path)
    return unique


def _find_named_structure(
    program: Any,
    name: str,
    *,
    asserted_size: int | None = None,
) -> Any | None:
    """Best existing Structure for ``name``.

    Prefer the namespace-aware category path for the source identity
    (``/ns/Foo`` for ``ns::Foo``) before leaf-only or Demangler lookups.
    When ``asserted_size`` is known, reject length mismatches. Score by
    provenance tier first, then size match, then richness — never pick solely
    by max (components, size) when that ignores a better-path candidate.
    """

    from java.util import ArrayList  # type: ignore[import-not-found]

    manager = program.getDataTypeManager()
    simple = _simple_name(name)
    candidates: list[Any] = []
    seen_paths: set[str] = set()

    for path in _identity_category_paths(name):
        if "::" in name and _structure_path_tier(path, simple, name) > 1:
            continue
        structure = _as_structure(manager.getDataType(path))
        if structure is None:
            continue
        if asserted_size is not None and int(structure.getLength()) != asserted_size:
            continue
        candidates.append(structure)
        seen_paths.add(str(structure.getPathName()))

    matches = ArrayList()
    manager.findDataTypes(simple, matches)
    for data_type in matches:
        structure = _as_structure(data_type)
        if structure is None or structure.getName() != simple:
            continue
        path = str(structure.getPathName())
        if "::" in name and _structure_path_tier(path, simple, name) > 1:
            continue
        if asserted_size is not None and int(structure.getLength()) != asserted_size:
            continue
        if path in seen_paths:
            continue
        candidates.append(structure)
        seen_paths.add(path)

    best = None
    best_score: tuple[int, int, int, int] | None = None
    for structure in candidates:
        path = str(structure.getPathName())
        length = int(structure.getLength())
        size_match = 1 if asserted_size is None or length == asserted_size else 0
        components = len(list(structure.getDefinedComponents()))
        # Negate tier so lower provenance rank sorts higher.
        score = (-_structure_path_tier(path, simple, name), size_match, components, length)
        if best_score is None or score > best_score:
            best = structure
            best_score = score
    return best


def _richness(structure: Any | None) -> tuple[int, int]:
    if structure is None:
        return (0, 0)
    return (len(list(structure.getDefinedComponents())), int(structure.getLength()))


def _is_useful(structure: Any | None) -> bool:
    components, length = _richness(structure)
    return length > 1 or components > 0


def _decide_structure_action(
    *,
    bound: Any | None,
    source: Any | None,
    asserted_size: int | None,
    source_size_ok: bool,
    size_mismatched_source: Any | None,
) -> str:
    """Choose projection action by binding identity — never by component-count contests."""

    if bound is not None and (
        _is_useful(bound) or asserted_size == 1 and int(bound.getLength()) == 1
    ):
        if asserted_size is not None and int(bound.getLength()) != asserted_size:
            return "conflict"
        return "agree"
    if (
        source is not None
        and (_is_useful(source) or asserted_size == 1 and int(source.getLength()) == 1)
        and source_size_ok
    ):
        # Source already sits where findExistingClassStruct will find it once
        # the GhidraClass exists — ensure_ghidra_class is enough at apply time.
        return "agree" if bound is source else "bind-existing"
    if size_mismatched_source is not None and not _is_useful(bound):
        if asserted_size is not None and asserted_size > 0:
            return "create-opaque"
        return "size-mismatch"
    if not _is_useful(bound) and asserted_size is not None and asserted_size > 0:
        return "create-opaque"
    return "no-layout-evidence"


def collect_structure_projection_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    class_names: Sequence[str] | None = None,
) -> dict[str, Any]:
    """Plan class Structure binding / opaque shell creation for source classes."""

    source_data = load_source_index(repository)
    index = SourceIndex.from_dict(source_data)
    classes = _classes_by_name(index, target)
    owning = _thiscall_owning_classes(repository, target)
    referenced_templates = _referenced_template_classes(repository, target, index)
    # Template spellings cannot be GhidraClass namespaces. Even a marked
    # template method or an asserted size must use the datatype path below.
    for name in list(owning):
        if "<" in name:
            if name in classes and _source_size(classes[name]) is not None:
                referenced_templates[name] += owning[name]
            del owning[name]
    # Sized source records without methods still need a bound Structure for
    # globals and nested fields (for example W8WorldCursorState).
    for record in source_data.get("classes", []):
        if record.get("target") == target and record.get("asserted_size"):
            name = str(record["qualified_name"])
            if "<" in name:
                if name in classes:
                    referenced_templates.setdefault(name, 0)
            else:
                owning.setdefault(name, 0)
    if class_names is not None:
        wanted = {_simple_name(name) for name in class_names} | set(class_names)
        owning = Counter(
            {
                name: count
                for name, count in owning.items()
                if name in wanted or _simple_name(name) in wanted
            }
        )
        referenced_templates = Counter(
            {
                name: count
                for name, count in referenced_templates.items()
                if name in wanted or _simple_name(name) in wanted
            }
        )

    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    for owning_class, method_count in sorted(owning.items(), key=lambda item: (-item[1], item[0])):
        source_class = classes.get(owning_class) or classes.get(_simple_name(owning_class))
        asserted = _source_size(source_class)
        # Collect/plan is read-only: never create GhidraClass here.
        ghidra_class = find_ghidra_class(program, owning_class)
        bound = (
            _as_structure(find_class_structure(program, ghidra_class))
            if ghidra_class is not None
            else None
        )
        source = _find_named_structure(program, owning_class, asserted_size=asserted)
        bound_score = _richness(bound)
        source_size_ok = source is None or asserted is None or int(source.getLength()) == asserted
        size_mismatched_source = None
        if asserted is not None and source is None:
            candidate = _find_named_structure(program, owning_class, asserted_size=None)
            if candidate is not None and int(candidate.getLength()) != asserted:
                size_mismatched_source = candidate

        if ghidra_class is None:
            if (
                source is not None
                and _is_useful(source)
                and source_size_ok
                or asserted is not None
                and asserted > 0
            ):
                action = "create-class"
            else:
                action = "missing-class"
        else:
            action = _decide_structure_action(
                bound=bound,
                source=source,
                asserted_size=asserted,
                source_size_ok=source_size_ok,
                size_mismatched_source=size_mismatched_source,
            )
            # bind-existing must land on the planner-selected evidence Structure.
            if (
                action == "bind-existing"
                and source is not None
                and bound is not None
                and str(bound.getPathName()) != str(source.getPathName())
            ):
                action = "conflict"

        counts[action] += 1
        if action == "agree":
            continue
        report_source = source if source is not None else size_mismatched_source
        report_score = _richness(report_source)
        rows.append(
            {
                "class": owning_class,
                "methods": method_count,
                "action": action,
                "asserted_size": asserted,
                "current": {
                    "path": str(bound.getPathName()) if bound is not None else None,
                    "length": bound_score[1],
                    "components": bound_score[0],
                },
                "source": {
                    "path": str(report_source.getPathName()) if report_source is not None else None,
                    "length": report_score[1],
                    "components": report_score[0],
                },
            }
        )

    # Ghidra namespaces cannot contain template brackets. These source-backed
    # template types need a DataType, but cannot be GhidraClass-bound.
    for name, use_count in sorted(referenced_templates.items()):
        if name in owning:
            continue
        source_class = classes[name]
        size = _source_size(source_class)
        if size is None:
            continue
        data_type = _find_named_structure(program, _ghidra_type_name(name))
        if data_type is not None:
            if int(data_type.getLength()) != size:
                counts["template-type-size-mismatch"] += 1
                rows.append(
                    {
                        "class": name,
                        "methods": use_count,
                        "action": "template-type-size-mismatch",
                        "asserted_size": size,
                        "current": {
                            "path": str(data_type.getPathName()),
                            "length": int(data_type.getLength()),
                        },
                    }
                )
            else:
                counts["template-type-agree"] += 1
            continue
        counts["create-template-type"] += 1
        rows.append(
            {
                "class": name,
                "methods": use_count,
                "action": "create-template-type",
                "asserted_size": size,
            }
        )

    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": (
            counts["create-opaque"]
            + counts["bind-existing"]
            + counts["create-class"]
            + counts["create-template-type"]
        ),
        "classes": rows,
    }


def _parent_category_path(owning_class: str) -> Any:
    from ghidra.program.model.data import CategoryPath  # type: ignore[import-not-found]

    from .class_binding import _sanitize_class_parts

    parent_parts, _ = _sanitize_class_parts(owning_class)
    if not parent_parts:
        return CategoryPath("/")
    return CategoryPath("/" + "/".join(parent_parts))


def _create_opaque(program: Any, name: str, size: int) -> Any:
    """Create an opaque Structure where findExistingClassStruct will find it."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        DataTypeConflictHandler,
        StructureDataType,
    )

    ensure_ghidra_class(program, name)
    manager = program.getDataTypeManager()
    category = _parent_category_path(name)
    manager.createCategory(category)
    structure = StructureDataType(category, _simple_name(name), size)
    return manager.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)


def _create_template_type(program: Any, name: str, size: int) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
    )

    structure = StructureDataType(CategoryPath("/"), _ghidra_type_name(name), size)
    return program.getDataTypeManager().addDataType(
        structure, DataTypeConflictHandler.REPLACE_HANDLER
    )


def apply_structure_projection(
    program: Any,
    plan: Mapping[str, Any],
) -> dict[str, Any]:
    """Apply planned opaque shells / bindings (one transaction per row)."""

    from .ghidra.mutations import apply_rows

    def _apply_one(_program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
        action = row.get("action")
        owning = str(row["class"])
        if action == "create-opaque":
            size = int(row.get("asserted_size") or 0)
            if size <= 0:
                return {**dict(row), "error": "invalid-asserted-size"}
            result = _create_opaque(_program, owning, size)
        elif action == "create-template-type":
            size = int(row.get("asserted_size") or 0)
            if size <= 0:
                return {**dict(row), "error": "invalid-source-size"}
            result = _create_template_type(_program, owning, size)
        elif action == "template-type-size-mismatch":
            return {**dict(row), "error": "template-type-size-mismatch"}
        elif action == "create-class":
            size = int(row.get("asserted_size") or 0)
            source_path = (row.get("source") or {}).get("path")
            ghidra_class = ensure_ghidra_class(_program, owning)
            result = find_class_structure(_program, ghidra_class)
            if result is None and source_path:
                # Class namespace now exists; bind the evidence Structure already at
                # the preferred path (findExistingClassStruct can lag until refresh).
                candidate = _program.getDataTypeManager().getDataType(str(source_path))
                result = _as_structure(candidate)
            if result is None:
                if size > 0:
                    result = _create_opaque(_program, owning, size)
                else:
                    return {**dict(row), "error": "missing-bound-structure"}
            elif source_path and str(result.getPathName()) != str(source_path):
                return {
                    **dict(row),
                    "error": "bind-path-mismatch",
                    "bound_path": str(result.getPathName()),
                    "evidence_path": source_path,
                }
        elif action == "bind-existing":
            ensure_ghidra_class(_program, owning)
            result = find_class_structure(_program, ensure_ghidra_class(_program, owning))
            evidence_path = (row.get("source") or {}).get("path")
            if result is None and evidence_path:
                result = _as_structure(
                    _program.getDataTypeManager().getDataType(str(evidence_path))
                )
            if result is None:
                return {**dict(row), "error": "missing-bound-structure"}
            if evidence_path and str(result.getPathName()) != str(evidence_path):
                return {
                    **dict(row),
                    "error": "bind-path-mismatch",
                    "bound_path": str(result.getPathName()),
                    "evidence_path": evidence_path,
                }
        else:
            return {**dict(row), "error": f"unexpected-action:{action}"}
        return {
            "class": owning,
            "action": action,
            "path": str(result.getPathName()),
            "length": result.getLength(),
            "components": len(list(result.getDefinedComponents())),
        }

    rows = [
        row
        for row in plan.get("classes", [])
        if row.get("action")
        in {
            "create-opaque",
            "bind-existing",
            "create-class",
            "create-template-type",
            "template-type-size-mismatch",
        }
    ]
    result = apply_rows(
        program,
        rows,
        _apply_one,
        description="Bind class Structures",
    )
    return {"applied": result["applied"], "errors": result["errors"], "classes": result["rows"]}
