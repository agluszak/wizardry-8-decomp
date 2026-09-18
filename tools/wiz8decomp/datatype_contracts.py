"""Shared DataType contract / field-shape comparison helpers.

Used by type-graph projection and legacy ``/wiz8/classes`` cleanup so both
agree on what “same shape” means without copy-pasted unwrap rules.
"""

from __future__ import annotations

from collections.abc import Iterable, Mapping, Sequence
from typing import Any, cast


def type_identity(data_type: Any) -> str:
    """Stable identity for DataType comparison (path preferred over bare name)."""

    if data_type is None:
        return ""
    path = getattr(data_type, "getPathName", None)
    if callable(path):
        text = str(path())
        if text:
            return text
    return str(data_type.getName()) if hasattr(data_type, "getName") else str(data_type)


def is_legacy_path(path: str | None) -> bool:
    if not path:
        return False
    text = str(path)
    while text.endswith("*"):
        text = text[:-1].rstrip()
    return text == "/wiz8/classes" or text.startswith("/wiz8/classes/")


def unwrap_plain_typedefs(data_type: Any) -> Any:
    """Unwrap alias typedefs; keep typedefs that carry settings (adjusted pointers)."""

    current = data_type
    while current is not None and "TypeDef" in type(current).__name__:
        if not hasattr(current, "getBaseDataType"):
            break
        # Preserve adjusted-pointer / offset-bearing typedefs.
        if _typedef_has_settings(current):
            break
        current = current.getBaseDataType()
    return current


def _typedef_has_settings(data_type: Any) -> bool:
    getter = getattr(data_type, "getDefaultSettings", None)
    if not callable(getter):
        return False
    try:
        settings = getter()
    except Exception:  # noqa: BLE001
        return False
    if settings is None:
        return False
    names = getattr(settings, "getNames", None)
    if callable(names):
        try:
            return bool(list(cast(Iterable[Any], names())))
        except Exception:  # noqa: BLE001
            return False
    # Fallback: any non-empty settings object counts as preserving.
    return True


def as_structure(data_type: Any) -> Any | None:
    """Return ``data_type`` when it is a Structure (unwrap plain typedefs).

    Duck-types when the Ghidra runtime is unavailable or the value is a unit fake.
    """

    current = data_type
    try:
        from ghidra.program.model.data import Structure, TypeDef  # type: ignore[import-not-found]

        while isinstance(current, TypeDef):
            current = current.getBaseDataType()
        if isinstance(current, Structure):
            return current
    except ImportError:
        pass
    current = unwrap_plain_typedefs(data_type)
    if current is None:
        return None
    if hasattr(current, "getDefinedComponents") and not hasattr(current, "getArguments"):
        return current
    return None


def _kind_name(data_type: Any) -> str:
    if data_type is None:
        return "null"
    name = type(data_type).__name__
    if "Pointer" in name or (
        callable(getattr(data_type, "isPointer", None)) and data_type.isPointer()
    ):
        return "pointer"
    if hasattr(data_type, "getDataType") and hasattr(data_type, "getNumElements"):
        return "array"
    if "Union" in name:
        return "union"
    if "FunctionDefinition" in name or (
        hasattr(data_type, "getArguments") and hasattr(data_type, "getReturnType")
    ):
        return "function"
    if hasattr(data_type, "getDefinedComponents") and not hasattr(data_type, "getArguments"):
        return "structure"
    if "TypeDef" in name:
        return "typedef"
    return "other"


def normalize_class_path(path: str, identity_map: Mapping[str, Mapping[str, Any]] | None) -> str:
    """Rewrite legacy/evidence class paths through the identity map."""

    if not path or not identity_map:
        return path
    if is_legacy_path(path):
        rest = path[len("/wiz8/classes/") :].rstrip("*").strip()
        rest = rest.removesuffix(" *")
        qualified = rest.replace("/", "::")
        row = identity_map.get(qualified)
        bound = row.get("bound_path") if row else None
        if bound and not is_legacy_path(str(bound)):
            return str(bound)
    for row in identity_map.values():
        evidence = row.get("evidence_path")
        bound = row.get("bound_path")
        if evidence and path == evidence and bound and not is_legacy_path(str(bound)):
            return str(bound)
        if bound and path == bound:
            return str(bound)
    return path


def datatype_shape_key(
    data_type: Any, *, identity_map: Mapping[str, Mapping[str, Any]] | None = None
) -> tuple[Any, ...]:
    """Shape key after controlled unwrap (offset/length/kind/path; nested shallow)."""

    current = unwrap_plain_typedefs(data_type)
    if current is None:
        return ("null", 0, "")
    kind = _kind_name(current)
    path = normalize_class_path(type_identity(current), identity_map)
    length = int(current.getLength()) if hasattr(current, "getLength") else 0
    if kind == "pointer":
        pointee = current.getDataType() if hasattr(current, "getDataType") else None
        return ("pointer", length, datatype_shape_key(pointee, identity_map=identity_map))
    if kind == "array":
        element = current.getDataType() if hasattr(current, "getDataType") else None
        count = int(current.getNumElements()) if hasattr(current, "getNumElements") else 0
        return ("array", length, count, datatype_shape_key(element, identity_map=identity_map))
    if kind == "function":
        return (
            "function",
            length,
            function_definition_contract(current, identity_map=identity_map),
        )
    if kind == "union":
        members: tuple[tuple[str, int, tuple[Any, ...]], ...] = ()
        if hasattr(current, "getDefinedComponents"):
            members = tuple(
                (
                    str(component.getFieldName() or ""),
                    int(component.getLength()),
                    datatype_shape_key(component.getDataType(), identity_map=identity_map),
                )
                for component in current.getDefinedComponents()
            )
        return ("union", length, members)
    if kind == "structure":
        return ("structure", length, path)
    if kind == "typedef":
        base = current.getBaseDataType() if hasattr(current, "getBaseDataType") else None
        return ("typedef", length, path, datatype_shape_key(base, identity_map=identity_map))
    return (kind, length, path)


def field_descriptors(
    structure: Any, *, identity_map: Mapping[str, Mapping[str, Any]] | None = None
) -> list[dict[str, Any]]:
    """Normalized defined components: offset, length, name, shape, path."""

    if structure is None or not hasattr(structure, "getDefinedComponents"):
        return []
    rows: list[dict[str, Any]] = []
    for component in structure.getDefinedComponents():
        data_type = component.getDataType()
        name = component.getFieldName()
        rows.append(
            {
                "offset": int(component.getOffset()),
                "length": int(component.getLength()),
                "name": str(name) if name is not None else "",
                "shape": datatype_shape_key(data_type, identity_map=identity_map),
                "path": normalize_class_path(type_identity(data_type), identity_map),
                "datatype": data_type,
            }
        )
    return rows


def fields_shape_agree(
    left: Sequence[Mapping[str, Any]],
    right: Sequence[Mapping[str, Any]],
) -> bool:
    """True when field lists agree on offset, length, name, and datatype shape."""

    if len(left) != len(right):
        return False
    for a, b in zip(
        sorted(left, key=lambda r: r["offset"]),
        sorted(right, key=lambda r: r["offset"]),
        strict=True,
    ):
        if int(a["offset"]) != int(b["offset"]):
            return False
        if int(a["length"]) != int(b["length"]):
            return False
        if str(a.get("name") or "") != str(b.get("name") or ""):
            return False
        if a["shape"] != b["shape"]:
            return False
    return True


def structures_field_shape_agree(
    left: Any,
    right: Any,
    *,
    identity_map: Mapping[str, Mapping[str, Any]] | None = None,
) -> bool:
    if left is None or right is None:
        return False
    if (
        hasattr(left, "getLength")
        and hasattr(right, "getLength")
        and int(left.getLength()) != int(right.getLength())
    ):
        return False
    return fields_shape_agree(
        field_descriptors(left, identity_map=identity_map),
        field_descriptors(right, identity_map=identity_map),
    )


def is_opaque_structure(structure: Any) -> bool:
    """True when the Structure has no defined components (empty shell)."""

    if structure is None:
        return True
    if not hasattr(structure, "getDefinedComponents"):
        return True
    return len(list(structure.getDefinedComponents())) == 0


def is_rich_structure(structure: Any) -> bool:
    return not is_opaque_structure(structure)


def function_definition_contract(
    definition: Any, *, identity_map: Mapping[str, Mapping[str, Any]] | None = None
) -> tuple[Any, ...]:
    """Comparable ABI contract (return, parameter types, convention, varargs, noreturn).

    Parameter names are not part of the contract.
    """

    if definition is None:
        return ()
    ret = (
        datatype_shape_key(definition.getReturnType(), identity_map=identity_map)
        if hasattr(definition, "getReturnType")
        else ()
    )
    args: list[tuple[Any, ...]] = []
    if hasattr(definition, "getArguments"):
        for arg in definition.getArguments():
            args.append(datatype_shape_key(arg.getDataType(), identity_map=identity_map))
    convention = ""
    if hasattr(definition, "getCallingConvention"):
        current = definition.getCallingConvention()
        convention = str(current) if current is not None else ""
    if convention in {"", "unknown", "default"}:
        first_name = ""
        if hasattr(definition, "getArguments"):
            raw = list(definition.getArguments() or [])
            if raw:
                first_name = str(raw[0].getName() or "")
        if first_name == "this":
            convention = "__thiscall"
    varargs = bool(definition.hasVarArgs()) if hasattr(definition, "hasVarArgs") else False
    noreturn = bool(definition.hasNoReturn()) if hasattr(definition, "hasNoReturn") else False
    return (ret, tuple(args), convention, varargs, noreturn)


def definition_contract_equals(left: Any, right: Any) -> bool:
    """True when two FunctionDefinitions share return/args/convention shape."""

    return function_definition_contract(left) == function_definition_contract(right)


def walk_datatype_refs(data_type: Any, *, seen: set[str] | None = None) -> list[Any]:
    """Depth-first nested DataType references (pointers, arrays, fields, args)."""

    if seen is None:
        seen = set()
    current = unwrap_plain_typedefs(data_type)
    if current is None:
        return []
    identity = type_identity(current)
    if identity and identity in seen:
        return []
    if identity:
        seen.add(identity)
    kind = _kind_name(current)
    found = [current]
    if kind == "typedef":
        base = current.getBaseDataType() if hasattr(current, "getBaseDataType") else None
        found.extend(walk_datatype_refs(base, seen=seen))
        return found
    if kind in {"pointer", "array"} and hasattr(current, "getDataType"):
        found.extend(walk_datatype_refs(current.getDataType(), seen=seen))
    elif kind in {"structure", "union"} and hasattr(current, "getDefinedComponents"):
        for component in current.getDefinedComponents():
            found.extend(walk_datatype_refs(component.getDataType(), seen=seen))
    elif kind == "function":
        if hasattr(current, "getReturnType"):
            found.extend(walk_datatype_refs(current.getReturnType(), seen=seen))
        if hasattr(current, "getArguments"):
            for arg in current.getArguments():
                found.extend(walk_datatype_refs(arg.getDataType(), seen=seen))
    return found


def settings_typedef_blocks_remap(data_type: Any) -> bool:
    """True when a settings-bearing typedef still hides a legacy/reference type."""

    for nested in walk_datatype_refs(data_type):
        if "TypeDef" not in type(nested).__name__:
            continue
        if not _typedef_has_settings(nested):
            continue
        base = nested.getBaseDataType() if hasattr(nested, "getBaseDataType") else None
        if has_legacy_nested_ref(base) or is_legacy_path(type_identity(base)):
            return True
    return False


def has_legacy_nested_ref(data_type: Any) -> bool:
    for nested in walk_datatype_refs(data_type):
        if is_legacy_path(type_identity(nested)):
            return True
    return False
