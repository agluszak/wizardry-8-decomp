"""One resolver for reviewed, imported and reconstructed ABI type specs."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any, Literal

_ARRAY = re.compile(r"^(?P<base>.+)\[(?P<count>[1-9][0-9]*)\]$")


@dataclass(frozen=True)
class TypeSpec:
    kind: Literal["primitive", "pointer", "array", "named", "function"]
    name: str
    target: TypeSpec | None = None
    count: int | None = None


def parse_type_spec(spelling: str) -> TypeSpec:
    value = spelling.strip()
    for prefix in ("class ", "struct ", "enum "):
        if value.startswith(prefix):
            value = value[len(prefix) :].strip()
    value = re.sub(r"\bconst\b", "", value).strip()
    match = _ARRAY.fullmatch(value)
    if match:
        return TypeSpec(
            "array",
            value,
            parse_type_spec(match.group("base")),
            int(match.group("count")),
        )
    if value.endswith(("*", "&")):
        return TypeSpec("pointer", value, parse_type_spec(value[:-1]))
    if value.startswith("function:"):
        return TypeSpec("function", value.removeprefix("function:"))
    primitives = {
        "void",
        "bool",
        "byte",
        "char",
        "double",
        "float",
        "int",
        "int16",
        "int32",
        "long",
        "short",
        "signed char",
        "uint8",
        "uint16",
        "uint32",
        "unsigned char",
        "unsigned int",
        "unsigned long",
        "unsigned short",
        "wchar_t",
    }
    return TypeSpec("primitive" if value in primitives else "named", value)


def type_category_paths(evidence_program: str) -> tuple[str, ...]:
    return (
        f"/{evidence_program}/classes",
        f"/{evidence_program}/formats/slf",
        f"/{evidence_program}/sgp",
        f"/{evidence_program}/zlib_1_0_4",
        f"/{evidence_program}/srext_unzip",
        f"/{evidence_program}/vtables",
        "/surrender/classes",
    )


def resolve_type_spec(
    dtm: Any,
    spelling: str | TypeSpec,
    evidence_program: str,
    *,
    imported_types: frozenset[str] = frozenset(),
    allow_opaque_imported: bool = False,
) -> Any:
    """Resolve a TypeSpec through exact DataTypePaths, optionally creating an opaque import."""

    from ghidra.program.model.data import (
        ArrayDataType,
        BooleanDataType,
        ByteDataType,
        CategoryPath,
        CharDataType,
        DataTypeConflictHandler,
        DataTypePath,
        DoubleDataType,
        FloatDataType,
        IntegerDataType,
        PointerDataType,
        ShortDataType,
        SignedCharDataType,
        StructureDataType,
        UnsignedCharDataType,
        UnsignedIntegerDataType,
        UnsignedShortDataType,
        VoidDataType,
        WideCharDataType,
    )

    spec = spelling if isinstance(spelling, TypeSpec) else parse_type_spec(spelling)
    primitives = {
        "void": VoidDataType.dataType,
        "bool": BooleanDataType.dataType,
        "byte": ByteDataType.dataType,
        "char": CharDataType.dataType,
        "double": DoubleDataType.dataType,
        "float": FloatDataType.dataType,
        "int": IntegerDataType.dataType,
        "int16": ShortDataType.dataType,
        "int32": IntegerDataType.dataType,
        "long": IntegerDataType.dataType,
        "short": ShortDataType.dataType,
        # Preserve reviewed byte signedness: this can select a different VC6
        # conditional branch encoding in a ported body.
        "signed char": SignedCharDataType.dataType,
        "uint8": UnsignedCharDataType.dataType,
        "uint16": UnsignedShortDataType.dataType,
        "uint32": UnsignedIntegerDataType.dataType,
        "unsigned int": UnsignedIntegerDataType.dataType,
        "unsigned char": UnsignedCharDataType.dataType,
        "unsigned long": UnsignedIntegerDataType.dataType,
        "unsigned short": UnsignedShortDataType.dataType,
        "wchar_t": WideCharDataType.dataType,
    }
    if spec.kind == "primitive":
        return primitives[spec.name]
    if spec.kind == "pointer":
        return PointerDataType(
            resolve_type_spec(
                dtm,
                spec.target,
                evidence_program,
                imported_types=imported_types,
                allow_opaque_imported=allow_opaque_imported,
            ),
            dtm,
        )
    if spec.kind == "array":
        element = resolve_type_spec(
            dtm,
            spec.target,
            evidence_program,
            imported_types=imported_types,
            allow_opaque_imported=allow_opaque_imported,
        )
        return ArrayDataType(element, int(spec.count), element.getLength())
    for category in type_category_paths(evidence_program):
        resolved = dtm.getDataType(DataTypePath(CategoryPath(category), spec.name))
        if resolved is not None:
            return resolved
    if allow_opaque_imported and spec.name in imported_types:
        segments = spec.name.split("::")
        category = CategoryPath("/".join(["/surrender/classes", *segments[:-1]]))
        return dtm.addDataType(
            StructureDataType(category, segments[-1], 0),
            DataTypeConflictHandler.KEEP_HANDLER,
        )
    raise ValueError(f"unsupported reviewed type spec: {spec.name}")
