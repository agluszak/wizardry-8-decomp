"""Bind source class identity to Ghidra's native class namespace + Structure.

Ghidra types automatic ``this`` via ``VariableUtilities.findOrCreateClassStruct``
from the function's ``GhidraClass`` parent namespace. The preferred Structure uses
a namespace-based category path (reccmp places ordinary classes at ``/ClassName``
under the global namespace).

A category path is organization, not provenance. Do not invent a second mutable
runtime graph under ``/wiz8/classes`` merely to hold "enriched" copies — that
breaks ``VtableResolver.classNamespace()`` (``/wiz8/classes/W8Monster`` →
``wiz8::classes::W8Monster``) and forces custom-storage workarounds for auto
``this``.

Acceptance: ordinary dynamically stored methods, global class pointers, and the
Java exporter all resolve the same Structure without enabling custom storage.
"""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

# Legacy projection category from the competing-universe experiment. Still
# readable for migration reports; never the write target for new bindings.
_LEGACY_ENRICHED_CATEGORY = "/wiz8/classes/"


def is_legacy_enriched_path(path: str | None) -> bool:
    """True when ``path`` is under the former ``/wiz8/classes`` category."""

    if not path:
        return False
    text = str(path)
    while text.endswith("*"):
        text = text[:-1].rstrip()
    return text == "/wiz8/classes" or text.startswith(_LEGACY_ENRICHED_CATEGORY)


def _simple_name(qualified: str) -> str:
    return qualified.split("::")[-1]


def _sanitize_class_parts(owning_class: str) -> tuple[tuple[str, ...], str]:
    """Split ``ns::Class`` into namespace parts and leaf class name.

    Mirrors reccmp's ``sanitize_name`` enough for ordinary (non-template) classes.
    Template-bearing names belong to the compiler-backed importer, not this helper.
    """

    text = owning_class.strip()
    if not text:
        raise ValueError("empty owning class")
    # Avoid splitting inside template arguments if still spelled with ``::``.
    if "<" in text or "[" in text:
        leaf = _simple_name(text.replace("<", "[").replace(">", "]"))
        return (), leaf
    parts = [part for part in text.split("::") if part]
    if not parts:
        raise ValueError(f"empty owning class: {owning_class!r}")
    return tuple(parts[:-1]), parts[-1]


def is_ghidra_class(namespace: Any) -> bool:
    """True when ``namespace`` is a Ghidra ``GhidraClass`` (12.1.3+ listing API)."""

    from ghidra.program.model.listing import GhidraClass  # type: ignore[import-not-found]

    return isinstance(namespace, GhidraClass)


def find_ghidra_class(program: Any, owning_class: str) -> Any | None:
    """Look up an existing ``GhidraClass`` for ``owning_class`` without creating."""

    parent_parts, class_name = _sanitize_class_parts(owning_class)
    symbols = program.getSymbolTable()
    namespace = program.getGlobalNamespace()
    for part in parent_parts:
        child = symbols.getNamespace(part, namespace)
        if child is None:
            return None
        namespace = child
    existing = symbols.getNamespace(class_name, namespace)
    if existing is None or not is_ghidra_class(existing):
        return None
    return existing


def ensure_ghidra_class(program: Any, owning_class: str) -> Any:
    """Return the ``GhidraClass`` for ``owning_class``, creating namespaces/class as needed.

    Requires an open Ghidra transaction. Prefer :func:`find_ghidra_class` for reports.
    """

    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    parent_parts, class_name = _sanitize_class_parts(owning_class)
    symbols = program.getSymbolTable()
    namespace = program.getGlobalNamespace()
    for part in parent_parts:
        child = symbols.getNamespace(part, namespace)
        if child is None:
            child = symbols.createNameSpace(part, namespace, SourceType.IMPORTED)
        namespace = child
    existing = symbols.getNamespace(class_name, namespace)
    if existing is not None:
        if not is_ghidra_class(existing):
            raise RuntimeError(
                f"namespace collision: {owning_class!r} exists as a plain namespace, "
                "not a GhidraClass"
            )
        return existing
    created = symbols.createClass(namespace, class_name, SourceType.IMPORTED)
    if not is_ghidra_class(created):
        raise TypeError(f"createClass did not return a GhidraClass for {owning_class!r}")
    return created


def find_class_structure(program: Any, ghidra_class: Any) -> Any | None:
    """Structure Ghidra associates with ``ghidra_class``, or None."""

    from ghidra.program.model.listing import VariableUtilities  # type: ignore[import-not-found]

    if not is_ghidra_class(ghidra_class):
        return None
    return VariableUtilities.findExistingClassStruct(ghidra_class, program.getDataTypeManager())


def find_or_create_class_structure(program: Any, ghidra_class: Any) -> Any | None:
    """Existing class Structure, or Ghidra's placeholder (may be uncommitted)."""

    from ghidra.program.model.listing import VariableUtilities  # type: ignore[import-not-found]

    if not is_ghidra_class(ghidra_class):
        return None
    return VariableUtilities.findOrCreateClassStruct(ghidra_class, program.getDataTypeManager())


def legacy_enriched_structure(program: Any, owning_class: str) -> Any | None:
    """Structure previously projected under ``/wiz8/classes``, if any."""

    manager = program.getDataTypeManager()
    for name in (owning_class, _simple_name(owning_class)):
        data_type = manager.getDataType(f"{_LEGACY_ENRICHED_CATEGORY}{name}")
        if data_type is not None:
            return data_type
    return None


def resolve_class_binding(program: Any, owning_class: str) -> dict[str, Any]:
    """Report the live class ↔ Structure binding for one source class identity.

    Read-only: does not create namespaces or classes. Apply paths that need a
    missing ``GhidraClass`` must call :func:`ensure_ghidra_class` in a transaction.
    """

    legacy = legacy_enriched_structure(program, owning_class)
    ghidra_class = find_ghidra_class(program, owning_class)
    if ghidra_class is None:
        return {
            "owning_class": owning_class,
            "ghidra_class": None,
            "structure_path": None,
            "structure_length": None,
            "legacy_enriched_path": (str(legacy.getPathName()) if legacy is not None else None),
            "status": "missing-class",
        }
    structure = find_class_structure(program, ghidra_class)
    path = str(structure.getPathName()) if structure is not None else None
    status = "bound"
    if structure is None:
        status = "missing-structure"
    elif path is not None and path.startswith(_LEGACY_ENRICHED_CATEGORY):
        # Should not happen via findExistingClassStruct for a real GhidraClass, but
        # report if category mapping somehow prefers the legacy copy.
        status = "legacy-enriched-path"
    return {
        "owning_class": owning_class,
        "ghidra_class": str(ghidra_class.getName(True)),
        "structure_path": path,
        "structure_length": int(structure.getLength()) if structure is not None else None,
        "legacy_enriched_path": (str(legacy.getPathName()) if legacy is not None else None),
        "status": status,
    }


def ensure_function_class_namespace(function: Any, ghidra_class: Any) -> bool:
    """Move ``function`` under ``ghidra_class`` when the parent differs.

    Returns True when the parent namespace changed.
    """

    current = function.getParentNamespace()
    if current is not None and current.equals(ghidra_class):
        return False
    function.setParentNamespace(ghidra_class)
    return True


def auto_this_structure(function: Any) -> Any | None:
    """Pointee Structure of the automatic ``this`` parameter, if any."""

    from ghidra.program.model.listing import (  # type: ignore[import-not-found]
        AutoParameterType,
        VariableUtilities,
    )

    for parameter in function.getParameters():
        storage = parameter.getVariableStorage()
        if storage is None or not storage.isAutoStorage():
            continue
        if storage.getAutoParameterType() != AutoParameterType.THIS:
            continue
        data_type = VariableUtilities.getAutoDataType(function, function.getReturnType(), storage)
        if data_type is None:
            return None
        current = data_type
        while "TypeDef" in type(current).__name__ and hasattr(current, "getBaseDataType"):
            current = current.getBaseDataType()
        if hasattr(current, "isPointer") and current.isPointer():
            pointee = current.getDataType()
            return pointee
        if "Pointer" in type(current).__name__ and hasattr(current, "getDataType"):
            return current.getDataType()
        return None
    return None


def binding_agrees(binding: Mapping[str, Any], function: Any) -> bool:
    """True when auto ``this`` already points at the bound Structure."""

    expected = binding.get("structure_path")
    if not expected or binding.get("status") != "bound":
        return False
    pointee = auto_this_structure(function)
    if pointee is None:
        return False
    path_getter = getattr(pointee, "getPathName", None)
    if not callable(path_getter):
        return False
    return str(path_getter()) == str(expected)
