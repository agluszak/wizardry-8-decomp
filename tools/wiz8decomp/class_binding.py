"""Look up a source class's Ghidra ``GhidraClass`` namespace and associated Structure."""

from __future__ import annotations

from typing import Any


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

    if namespace is None:
        return False
    ghidra_class_type: type | None
    try:
        from ghidra.program.model.listing import GhidraClass  # type: ignore[import-not-found]

        ghidra_class_type = GhidraClass
    except Exception:  # noqa: BLE001 — unit fakes / stubs before pyghidra
        ghidra_class_type = None
    if ghidra_class_type is not None and isinstance(namespace, ghidra_class_type):
        return True
    is_class = getattr(namespace, "isClass", None)
    return bool(callable(is_class) and is_class())


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


def find_class_structure(program: Any, ghidra_class: Any) -> Any | None:
    """Structure Ghidra associates with ``ghidra_class``, or None."""

    if ghidra_class is None or not is_ghidra_class(ghidra_class):
        return None

    from ghidra.program.model.listing import VariableUtilities  # type: ignore[import-not-found]

    return VariableUtilities.findExistingClassStruct(ghidra_class, program.getDataTypeManager())
