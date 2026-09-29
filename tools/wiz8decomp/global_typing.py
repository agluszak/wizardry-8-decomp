"""Apply source-backed GLOBAL marker types into the live Ghidra listing.

Uses ``parse_global_definitions`` ownership/type facts. Only addresses with a
resolved DataType whose length matches the source size (when known) are
actionable. Speculative layouts are never invented here.
"""

from __future__ import annotations

import re
from collections import Counter
from collections.abc import Mapping, Sequence
from hashlib import sha256
from pathlib import Path
from typing import Any

from .global_model import parse_global_definitions
from .source_index import try_load_source_index

_SCHEMA = "wiz8.global-typing-v1"
_ARRAY_SUFFIX = re.compile(r"^(?P<base>.+?)(?P<arrays>(?:\[\s*(?:0[xX][0-9a-fA-F]+|\d*)\s*\])+)$")
_SOURCE_ARRAY_SUFFIX = re.compile(r"^(?P<base>.+?)(?P<arrays>(?:\[[^\]]*\])+)$")
_INDEX_ARRAY_SUFFIX = re.compile(r"^(?P<base>.+?)(?P<arrays>(?:\[\d+\])+)$")
_POINTER_SUFFIX = re.compile(r"^(?P<base>.+?)\s*(?P<stars>\*+)\s*$")
_TEMPLATE = re.compile(r"<([^<>]+)>")
_CLASS_LIKE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$")
_FUNCTION_POINTER = re.compile(r"^(?P<result>.+?)\s*\(\s*\*\s*\)\s*\((?P<arguments>.*)\)$")
_ARRAY_POINTER = re.compile(r"^(?P<base>.+?)\s*\(\s*\*\s*\)\s*(?P<array>\[\d+\])$")
# Released SGP headers typedef these record tags; the imported debug types
# retain the tag names rather than the typedef names.
_SGP_RECORD_TYPEDEFS = {"MOUSE_REGION": "_MOUSE_REGION", "GUI_BUTTON": "_GUI_BUTTON"}
_SOURCE_POINTER_TYPEDEFS = {
    "H3DPOBJECT": "h3DPOBJECT",
    "HDIGDRIVER": "_DIG_DRIVER",
    "HVOBJECT": "TAG_HVOBJECT",
    "HVSURFACE": "SGPVSurface",
    "LPDIRECTDRAW": "IDirectDraw",
    "LPDIRECTDRAW2": "IDirectDraw2",
    "LPDIRECTDRAWSURFACE": "IDirectDrawSurface",
    "LPDIRECTDRAWSURFACE2": "IDirectDrawSurface2",
}
_EXTERNAL_TYPE_PATHS = {
    "CHAR": "/winnt.h/CHAR",
    "FILE": "/mbstring.h/FILE",
    "HANDLE": "/winnt.h/HANDLE",
    "HHOOK": "/WinDef.h/HHOOK",
    "HINSTANCE": "/WinDef.h/HINSTANCE",
    "HWND": "/WinDef.h/HWND",
    "POINT": "/WinDef.h/POINT",
    "WNDPROC": "/winuser.h/WNDPROC",
    # The WIZ8/SGP build selects the ANSI Win32 typedef.
    "WIN32_FIND_DATA": "/wiz8/sgp/WIN32_FIND_DATAA",
}


def _qualified_parts(name: str) -> list[str]:
    """Split C++ namespaces while retaining nested template arguments intact."""

    parts: list[str] = []
    depth = 0
    start = 0
    index = 0
    while index < len(name):
        char = name[index]
        if char in "<[(":
            depth += 1
        elif char in ">])":
            depth -= 1
        elif name[index : index + 2] == "::" and depth == 0:
            parts.append(name[start:index].strip())
            index += 2
            start = index
            continue
        index += 1
    parts.append(name[start:].strip())
    return [part for part in parts if part]


def _simple_name(qualified: str) -> str:
    parts = _qualified_parts(qualified)
    return parts[-1] if parts else ""


def _split_type_arguments(text: str) -> list[str]:
    arguments = []
    start = 0
    depth = 0
    for index, char in enumerate(text):
        if char in "(<[":
            depth += 1
        elif char in ")>]":
            depth -= 1
        elif char == "," and depth == 0:
            arguments.append(text[start:index].strip())
            start = index + 1
    arguments.append(text[start:].strip())
    return arguments


def _is_class_like_type_name(type_name: str) -> bool:
    """True for simple ``Class`` / ``ns::Class`` spellings without pointer/array/template noise."""

    text = type_name.strip()
    if not text or any(ch in text for ch in "*[](<>"):
        return False
    return _CLASS_LIKE.fullmatch(text) is not None


def _strip_qualifiers(type_name: str) -> str:
    text = type_name.strip()
    changed = True
    while changed:
        changed = False
        for prefix in (
            "extern ",
            "const ",
            "volatile ",
            "static ",
            "struct ",
            "class ",
            "enum ",
            "union ",
        ):
            if text.startswith(prefix):
                text = text[len(prefix) :].lstrip()
                changed = True
        for suffix in ("const", "volatile"):
            match = re.search(rf"\b{suffix}\s*$", text)
            if match:
                text = text[: match.start()].rstrip()
                changed = True
    return text


def _literal_character_array(
    definition: Mapping[str, Any], lines: Sequence[str]
) -> tuple[str, int] | None:
    """Size a simple ASCII string array from its current source declaration."""

    spelling = str(definition.get("type") or "")
    if _strip_qualifiers(spelling) not in {"char[]", "wchar_t[]"}:
        return None
    line_index = int(definition["line"]) - 1
    if line_index < 0:
        return None
    try:
        line = lines[line_index]
    except IndexError:
        return None
    if line.rstrip().endswith("=") and line_index + 1 < len(lines):
        line += " " + lines[line_index + 1].strip()
    kind = "wchar_t" if "wchar_t" in spelling else "char"
    prefix = "L" if kind == "wchar_t" else ""
    name = re.escape(str(definition["name"]))
    declaration = rf"^\s*(?:(?:static|const)\s+)*{kind}\s+{name}\s*\[\s*\]\s*=\s*"
    suffix = r"\s*;\s*(?://.*)?$"
    pattern = declaration + rf'{prefix}"(?P<body>(?:\\.|[^"\\])*)"' + suffix
    match = re.fullmatch(pattern, line)
    if match is None:
        brace = re.fullmatch(declaration + r"\{(?P<body>[^{}]*)\}" + suffix, line)
        if brace is None:
            return None
        body = brace.group("body")
        number = r"(?:0[xX][0-9a-fA-F]+|\d+)"
        if re.fullmatch(rf"\s*{number}(?:\s*,\s*{number})*\s*,?\s*", body) is None:
            return None
        values = re.findall(number, body)
        limit = 0xFFFF if kind == "wchar_t" else 0xFF
        if any(
            int(value, 16 if value.lower().startswith("0x") else 10) > limit for value in values
        ):
            return None
        count = len(values)
        return spelling[:-2] + f"[{count}]", count * (2 if kind == "wchar_t" else 1)
    body = match.group("body")
    if not body.isascii():
        return None
    count = 0
    index = 0
    while index < len(body):
        if body[index] != "\\":
            count += 1
            index += 1
            continue
        escaped = body[index + 1]
        if escaped in "abfnrtv\\\"'?":
            index += 2
        elif escaped in "01234567":
            end = index + 2
            while end < min(index + 4, len(body)) and body[end] in "01234567":
                end += 1
            if int(body[index + 1 : end], 8) > 0x7F:
                return None
            index = end
        else:
            return None
        count += 1
    count += 1  # terminating NUL
    return spelling[:-2] + f"[{count}]", count * (2 if kind == "wchar_t" else 1)


def _literal_sized_character_arrays(
    repository: Path, definitions: Sequence[Mapping[str, Any]]
) -> dict[int, tuple[str, int]]:
    sized: dict[int, tuple[str, int]] = {}
    sources: dict[str, list[str]] = {}
    for definition in definitions:
        if _strip_qualifiers(str(definition.get("type") or "")) not in {"char[]", "wchar_t[]"}:
            continue
        source_file = str(definition["source_file"])
        if source_file not in sources:
            path = repository / source_file
            sources[source_file] = (
                path.read_text(encoding="utf-8", errors="replace").splitlines()
                if path.is_file()
                else []
            )
        result = _literal_character_array(definition, sources[source_file])
        if result is not None:
            sized[int(definition["address"])] = result
    return sized


def _compiler_sized_arrays(
    repository: Path, definitions: Sequence[Mapping[str, Any]]
) -> dict[int, tuple[str, int | None]]:
    """Fill non-literal array bounds from an exact, fresh compiler declaration."""

    index = try_load_source_index(repository)
    if index is None:
        return {}
    variables: dict[tuple[str, str, int, str], list[str]] = {}
    for variable in index.get("variables", []):
        key = (
            str(variable.get("target")),
            str(variable.get("source_file")),
            int(variable.get("line") or 0),
            _simple_name(str(variable.get("qualified_name", ""))),
        )
        variables.setdefault(key, []).append(str(variable.get("type") or ""))
    digests = index.get("source_digests") or {}
    dependencies = index.get("unit_dependencies") or {}
    current_digests: dict[str, str | None] = {}
    sized: dict[int, tuple[str, int | None]] = {}
    for definition in definitions:
        spelling = str(definition.get("type") or "")
        source_array = _SOURCE_ARRAY_SUFFIX.fullmatch(spelling)
        if source_array is None:
            continue
        source_bounds = re.findall(r"\[([^\]]*)\]", source_array.group("arrays"))
        if all(re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|\d+)", bound) for bound in source_bounds):
            continue
        source_file = str(definition["source_file"])
        # Macro bounds can change in an included header without changing the
        # declaration file. Check every indexed dependency with a digest.
        indexed_paths = dependencies.get(source_file, [source_file])
        if source_file not in indexed_paths:
            indexed_paths = [source_file, *indexed_paths]
        for indexed_path in indexed_paths:
            if indexed_path not in current_digests:
                path = repository / indexed_path
                current_digests[indexed_path] = (
                    sha256(path.read_bytes()).hexdigest() if path.is_file() else None
                )
        if any(
            current_digests[indexed_path] != digests.get(indexed_path)
            for indexed_path in indexed_paths
        ):
            continue
        key = (
            str(definition["target"]),
            source_file,
            int(definition["line"]),
            str(definition["name"]),
        )
        observed = variables.get(key, [])
        if len(observed) != 1:
            continue
        indexed_array = _INDEX_ARRAY_SUFFIX.fullmatch(observed[0])
        if indexed_array is None:
            continue
        bounds = [int(bound) for bound in re.findall(r"\[(\d+)\]", indexed_array.group("arrays"))]
        if len(bounds) != len(source_bounds) or any(bound <= 0 for bound in bounds):
            continue
        base = _strip_qualifiers(source_array.group("base"))
        source_size = definition.get("size")
        if source_size is None and base in {"char", "wchar_t"}:
            source_size = 2 if base == "wchar_t" else 1
            for bound in bounds:
                source_size *= bound
        sized[int(definition["address"])] = (
            source_array.group("base") + "".join(f"[{bound}]" for bound in bounds),
            source_size,
        )
    return sized


def _ghidra_type_name(type_name: str) -> str:
    """Map C++ template spelling to Ghidra's ``T[Args]`` Structure names."""

    text = _strip_qualifiers(type_name)
    text = re.sub(r"\b(?:class|struct|union|enum)\s+", "", text)
    while True:
        updated = _TEMPLATE.sub(r"[\1]", text)
        if updated == text:
            return text
        text = updated


def _builtin_data_type(program: Any, name: str) -> Any | None:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        BooleanDataType,
        ByteDataType,
        CharDataType,
        DoubleDataType,
        FloatDataType,
        IntegerDataType,
        LongDataType,
        ShortDataType,
        UnsignedCharDataType,
        UnsignedIntegerDataType,
        UnsignedLongDataType,
        UnsignedShortDataType,
        WideCharDataType,
    )

    builtins = {
        "bool": BooleanDataType,
        "char": CharDataType,
        "signed char": CharDataType,
        "INT8": CharDataType,
        "CHAR8": CharDataType,
        "unsigned char": UnsignedCharDataType,
        "uchar": UnsignedCharDataType,
        "byte": ByteDataType,
        "UINT8": UnsignedCharDataType,
        "BOOLEAN": UnsignedCharDataType,
        "short": ShortDataType,
        "unsigned short": UnsignedShortDataType,
        "INT16": ShortDataType,
        "UINT16": UnsignedShortDataType,
        "wchar_t": WideCharDataType,
        "int": IntegerDataType,
        "INT32": IntegerDataType,
        "long": LongDataType,
        "unsigned": UnsignedIntegerDataType,
        "unsigned int": UnsignedIntegerDataType,
        "unsigned long": UnsignedLongDataType,
        "size_t": UnsignedIntegerDataType,
        "UINT32": UnsignedIntegerDataType,
        "DWORD": UnsignedIntegerDataType,
        "HPROVIDER": UnsignedIntegerDataType,
        "TIMER": UnsignedIntegerDataType,
        "float": FloatDataType,
        "FLOAT": FloatDataType,
        "double": DoubleDataType,
        "DOUBLE": DoubleDataType,
    }
    factory = builtins.get(name)
    return factory() if factory is not None else None


def _named_data_type(program: Any, name: str) -> Any | None:
    manager = program.getDataTypeManager()
    text = name.strip()
    if not text or text in {"*", "[]"}:
        return None
    parts = _qualified_parts(text)
    simple = parts[-1] if parts else ""
    if not simple:
        return None

    candidates: list[str] = []
    if len(parts) > 1:
        candidates.append("/" + "/".join(parts))
        candidates.append(f"/{text}")
    candidates.extend(
        (
            f"/{simple}",
            f"/wiz8/sgp/{simple}",
            f"/Demangler/{simple}",
        )
    )
    if text in _SGP_RECORD_TYPEDEFS:
        candidates.append(f"/{_SGP_RECORD_TYPEDEFS[text]}")
    if text in _EXTERNAL_TYPE_PATHS:
        candidates.append(_EXTERNAL_TYPE_PATHS[text])
    # PDB imports encode a pointer template argument as ``T_#``. Resolve that
    # spelling only after the exact source spelling, and only for one simple
    # argument, so nested or ambiguous template identities remain unresolved.
    template, bracket, argument = simple.partition("[")
    if bracket and argument.endswith("]"):
        element = argument[:-1].strip()
        if (
            element.endswith("*")
            and "[" not in element
            and "]" not in element
            and "," not in element
        ):
            pointee = element[:-1].strip()
            if pointee and " " not in pointee and "::" not in pointee:
                candidates.append(f"/{template}[{pointee}_#]")
    seen: set[str] = set()
    for path in candidates:
        if path in seen or "//" in path or path.endswith("/"):
            continue
        seen.add(path)
        try:
            data_type = manager.getDataType(path)
        except Exception as exc:
            if "Paths must have non-empty elements" not in str(exc):
                raise
            continue
        if data_type is not None:
            from .class_binding import is_legacy_enriched_path

            resolved_path = (
                str(data_type.getPathName()) if hasattr(data_type, "getPathName") else path
            )
            if is_legacy_enriched_path(resolved_path):
                continue
            return data_type

    # Class Structures resolve through GhidraClass after the exact datatype path.
    if _is_class_like_type_name(text):
        from .class_binding import (
            find_class_structure,
            find_ghidra_class,
            is_legacy_enriched_path,
            resolve_class_binding,
        )

        binding = resolve_class_binding(program, text)
        path = binding.get("structure_path")
        if binding.get("status") == "bound" and path and not is_legacy_enriched_path(str(path)):
            data_type = manager.getDataType(str(path))
            if data_type is not None:
                return data_type
        ghidra_class = find_ghidra_class(program, text)
        if ghidra_class is not None:
            structure = find_class_structure(program, ghidra_class)
            if structure is not None:
                structure_path = str(structure.getPathName())
                if not is_legacy_enriched_path(structure_path):
                    return structure

    builtin = _builtin_data_type(program, simple)
    if builtin is not None:
        return builtin
    return None


def _data_type_path(data_type: Any | None) -> str | None:
    if data_type is None:
        return None
    getter = getattr(data_type, "getPathName", None)
    if callable(getter):
        return str(getter())
    return None


def _pointer_depth(data_type: Any | None) -> int:
    """Count leading Pointer wrappers (typedefs unwrapped at each step)."""

    depth = 0
    current = data_type
    while current is not None:
        while "TypeDef" in type(current).__name__ and hasattr(current, "getBaseDataType"):
            current = current.getBaseDataType()
            if current is None:
                return depth
        is_pointer = getattr(current, "isPointer", None)
        if callable(is_pointer):
            if not is_pointer():
                break
        elif "Pointer" not in type(current).__name__:
            break
        if not hasattr(current, "getDataType"):
            break
        pointee = current.getDataType()
        if pointee is None:
            break
        depth += 1
        current = pointee
    return depth


def _pointee_path(data_type: Any | None) -> str | None:
    current = data_type
    while current is not None:
        while "TypeDef" in type(current).__name__ and hasattr(current, "getBaseDataType"):
            current = current.getBaseDataType()
            if current is None:
                return None
        is_pointer = getattr(current, "isPointer", None)
        if callable(is_pointer):
            if not is_pointer():
                return _data_type_path(current)
        elif "Pointer" not in type(current).__name__:
            return _data_type_path(current)
        if not hasattr(current, "getDataType"):
            return _data_type_path(current)
        pointee = current.getDataType()
        if pointee is None:
            return None
        current = pointee
    return None


def _path_leaf(path: str | None) -> str | None:
    if not path:
        return None
    text = path.rstrip()
    while text.endswith("*"):
        text = text[:-1].rstrip()
    return text.rsplit("/", 1)[-1].strip() or None


def _is_legacy_enriched_path(path: str | None) -> bool:
    """True for the former competing-universe ``/wiz8/classes/…`` category."""

    if not path:
        return False
    leaf_path = path
    while leaf_path.endswith("*"):
        leaf_path = leaf_path[:-1].rstrip()
    return leaf_path.startswith("/wiz8/classes/")


def _is_canonical_class_path(path: str | None) -> bool:
    """True for preferred bound/root class Structures (not legacy ``/wiz8/classes``).

    ``/wiz8/classes/X`` must never be treated as more canonical than ``/X``.
    """

    if not path:
        return False
    leaf_path = path
    while leaf_path.endswith("*"):
        leaf_path = leaf_path[:-1].rstrip()
    return leaf_path.startswith("/") and not _is_legacy_enriched_path(leaf_path)


def resolve_data_type(program: Any, type_name: str) -> Any | None:
    """Resolve a source type spelling to a Ghidra DataType, or None."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        ArrayDataType,
        PointerDataType,
    )

    text = _ghidra_type_name(type_name)
    if not text:
        return None

    array_pointer = _ARRAY_POINTER.fullmatch(text)
    if array_pointer is not None:
        element_array = resolve_data_type(
            program, array_pointer.group("base") + array_pointer.group("array")
        )
        if element_array is None:
            return None
        return PointerDataType(element_array, program.getDataTypeManager())

    callback = _FUNCTION_POINTER.fullmatch(text)
    if callback is not None:
        from ghidra.program.model.data import (  # type: ignore[import-not-found]
            CategoryPath,
            FunctionDefinitionDataType,
            ParameterDefinitionImpl,
            VoidDataType,
        )

        result_spelling = callback.group("result")
        result = (
            VoidDataType()
            if result_spelling == "void"
            else resolve_data_type(program, result_spelling)
        )
        if result is None:
            return None
        parameters = []
        argument_spellings = _split_type_arguments(callback.group("arguments"))
        if argument_spellings != ["void"] and argument_spellings != [""]:
            for index, spelling in enumerate(argument_spellings):
                argument = resolve_data_type(program, spelling)
                if argument is None:
                    return None
                parameters.append(ParameterDefinitionImpl(f"arg_{index}", argument, None))
        name = "callback_" + sha256(text.encode()).hexdigest()[:16]
        definition = FunctionDefinitionDataType(CategoryPath("/wiz8/source-callbacks"), name)
        definition.setReturnType(result)
        if parameters:
            definition.setArguments(parameters)
        return PointerDataType(definition, program.getDataTypeManager())

    if text.endswith("&"):
        referent = resolve_data_type(program, text[:-1].rstrip())
        if referent is None:
            return None
        return PointerDataType(referent, program.getDataTypeManager())

    pointer = _POINTER_SUFFIX.match(text)
    if pointer is not None:
        base = resolve_data_type(program, pointer.group("base"))
        if base is None:
            return None
        # Count every trailing ``*`` (``char**`` → two Pointer wrappers).
        star_count = len(pointer.group("stars"))
        data_type = base
        manager = program.getDataTypeManager()
        for _ in range(star_count):
            data_type = PointerDataType(data_type, manager)
        return data_type

    array = _ARRAY_SUFFIX.match(text)
    if array is not None:
        base = resolve_data_type(program, array.group("base"))
        if base is None:
            return None
        extents: list[int] = []
        for match in re.finditer(r"\[([^\]]*)\]", array.group("arrays")):
            raw = match.group(1).strip()
            if not raw:
                return None
            try:
                extents.append(int(raw, 0))
            except ValueError:
                return None
        data_type = base
        for count in reversed(extents):
            if count <= 0:
                return None
            data_type = ArrayDataType(data_type, count, data_type.getLength())
        return data_type

    if text in _SOURCE_POINTER_TYPEDEFS:
        pointee = _named_data_type(program, _SOURCE_POINTER_TYPEDEFS[text])
        if pointee is not None:
            return PointerDataType(pointee, program.getDataTypeManager())

    return _named_data_type(program, text)


def _current_data(program: Any, address: int) -> dict[str, Any]:
    space = program.getAddressFactory().getDefaultAddressSpace()
    entry = space.getAddress(address)
    data = program.getListing().getDataAt(entry)
    symbol = program.getSymbolTable().getPrimarySymbol(entry)
    data_type = data.getDataType() if data is not None else None
    return {
        "address": entry,
        "data": data,
        "type": str(data_type.getName()) if data_type is not None else None,
        "type_path": _data_type_path(data_type),
        "pointer_depth": _pointer_depth(data_type) if data_type is not None else None,
        "length": int(data.getLength()) if data is not None else None,
        "symbol": str(symbol.getName()) if symbol is not None else None,
    }


_EQUIVALENT_TYPES = (
    # SGP BOOLEAN is a one-byte integer flag, not C++ bool.
    frozenset({"uchar", "byte", "BOOLEAN", "unsigned char", "undefined1"}),
    frozenset({"char", "schar", "signed char", "INT8"}),
    frozenset({"short", "INT16"}),
    frozenset({"int", "long", "int32", "INT32", "undefined4"}),
    frozenset({"uint", "ulong", "unsigned int", "unsigned long", "UINT32", "DWORD"}),
    frozenset({"ushort", "unsigned short", "UINT16", "word"}),
    frozenset({"float", "Float4", "FLOAT"}),
    frozenset({"double", "Float8", "DOUBLE"}),
)


def _types_equivalent(left: str, right: str) -> bool:
    if left == right:
        return True
    left_array = _ARRAY_SUFFIX.match(left)
    right_array = _ARRAY_SUFFIX.match(right)
    if bool(left_array) != bool(right_array):
        return False
    if left_array and right_array:
        if left_array.group("arrays") != right_array.group("arrays"):
            return False
        left = left_array.group("base").strip()
        right = right_array.group("base").strip()
    for group in _EQUIVALENT_TYPES:
        if left in group and right in group:
            return True
    return False


def _needs_type_update(
    current_type: str | None,
    resolved_name: str,
    *,
    current_path: str | None = None,
    resolved_path: str | None = None,
    current_depth: int | None = None,
    resolved_depth: int | None = None,
) -> bool:
    """True when listing type should be replaced by the resolved DataType.

    When paths are available, prefer identity of ``getPathName()`` (and pointer
    depth) over bare ``getName()``. A legacy ``/wiz8/classes/X`` listing must
    still update when the resolved type is the bound/root ``/X`` Structure;
    the reverse must not.
    """

    if current_type is None:
        return True

    if (
        current_path is not None
        and resolved_path is not None
        and current_depth is not None
        and resolved_depth is not None
    ):
        if current_depth != resolved_depth:
            return True
        if current_path == resolved_path:
            return False
        if (
            _is_canonical_class_path(resolved_path)
            and _is_legacy_enriched_path(current_path)
            and _path_leaf(current_path) == _path_leaf(resolved_path)
        ):
            return True
        # Same path identity already handled; different non-canonical paths fall
        # through to name equivalence for builtins / aliases.

    if current_type.startswith("undefined") or current_type in {"byte", "string"}:
        # undefined1 is covered as byte-equivalent above when resolved is BOOLEAN.
        if current_type == "undefined1" and _types_equivalent("undefined1", resolved_name):
            return False
        if current_type.startswith("undefined"):
            return True
    if _types_equivalent(current_type, resolved_name):
        return False
    if resolved_name.endswith("*") and current_type == "void *":
        return True
    if (
        current_path is not None
        and resolved_path is not None
        and current_path != resolved_path
        and not _types_equivalent(current_type, resolved_name)
    ):
        return True
    return current_type != resolved_name


def collect_global_typing_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Plan listing type/name repairs for source GLOBAL markers."""

    wanted = set(addresses) if addresses is not None else None
    definitions = parse_global_definitions(repository)
    sized_arrays = _compiler_sized_arrays(repository, definitions)
    sized_arrays.update(_literal_sized_character_arrays(repository, definitions))
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    for definition in definitions:
        if definition.get("target") != target:
            continue
        address = int(definition["address"])
        if wanted is not None and address not in wanted:
            continue
        source_type, source_size = sized_arrays.get(
            address,
            (
                str(definition.get("projected_type") or definition.get("type") or ""),
                definition.get("size"),
            ),
        )
        resolved = resolve_data_type(program, source_type)
        if resolved is None:
            action = "unresolved-type"
            resolved_name = None
            resolved_length = None
        else:
            resolved_name = str(resolved.getName())
            resolved_length = int(resolved.getLength())
            expected = source_size
            if expected is not None and int(expected) != resolved_length:
                action = "size-mismatch"
            else:
                current = _current_data(program, address)
                type_update = _needs_type_update(
                    current["type"],
                    resolved_name,
                    current_path=current.get("type_path"),
                    resolved_path=_data_type_path(resolved),
                    current_depth=current.get("pointer_depth"),
                    resolved_depth=_pointer_depth(resolved),
                )
                name = str(definition.get("name") or "")
                rename = bool(
                    name
                    and (
                        current["symbol"] is None
                        or str(current["symbol"]).startswith(("DAT_", "unnamed_"))
                    )
                    and current["symbol"] != name
                )
                if type_update and rename:
                    action = "set-type-and-name"
                elif type_update:
                    action = "set-type"
                elif rename:
                    action = "set-name"
                else:
                    action = "agree"

        counts[action] += 1
        if action == "agree":
            continue
        current = _current_data(program, address)
        rows.append(
            {
                "address": f"0x{address:08x}",
                "name": definition.get("name"),
                "source_type": source_type,
                "source_size": source_size,
                "source_file": definition.get("source_file"),
                "ghidra_type": current["type"],
                "ghidra_type_path": current.get("type_path"),
                "ghidra_symbol": current["symbol"],
                "resolved_type": resolved_name,
                "resolved_type_path": _data_type_path(resolved) if resolved is not None else None,
                "resolved_length": resolved_length,
                "action": action,
            }
        )
        if limit is not None and len(rows) >= limit and action.startswith("set-"):
            # Keep counting, but cap actionable sample size only for emission? No - limit total rows.
            pass

    if limit is not None:
        actionable = [row for row in rows if str(row["action"]).startswith("set-")]
        other = [row for row in rows if not str(row["action"]).startswith("set-")]
        rows = actionable[:limit] + other

    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": sum(counts[key] for key in counts if key.startswith("set-")),
        "globals": rows,
    }


def set_primary_label(program: Any, address: Any, name: str, source_type: Any) -> None:
    """Create ``name`` at ``address`` and make it the primary symbol."""

    from ghidra.app.cmd.label import SetLabelPrimaryCmd  # type: ignore[import-not-found]
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    symbols = program.getSymbolTable()
    primary = symbols.getPrimarySymbol(address)
    if primary is not None and primary.getName() == name:
        return
    # Prefer renaming a default/DAT primary over create+SetLabelPrimaryCmd, which
    # often fails with "Set primary not permitted" on already-labeled data.
    if primary is not None and (
        str(primary.getName()).startswith("DAT_") or primary.getSource() == SourceType.DEFAULT
    ):
        primary.setName(name, source_type)
        return
    created = symbols.createLabel(address, name, source_type)
    if created is None:
        for symbol in symbols.getSymbols(address):
            if symbol.getName() == name:
                if symbol.isPrimary():
                    return
                if symbol.setPrimary():
                    return
                break
        raise RuntimeError(f"createLabel failed for {name}")
    if created.isPrimary():
        return
    if created.setPrimary():
        return
    cmd = SetLabelPrimaryCmd(address, name, None)
    if not cmd.applyTo(program):
        raise RuntimeError(cmd.getStatusMsg() or "SetLabelPrimaryCmd failed")


def _apply_global_typing_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    action = str(row.get("action") or "")
    address = space.getAddress(int(row["address"], 0))
    if action in {"set-type", "set-type-and-name"}:
        from .ghidra.listing_guards import ClearRangeError, clear_code_units_guarded

        resolved = resolve_data_type(program, str(row.get("source_type") or ""))
        if resolved is None:
            return {**dict(row), "error": "unresolved-type"}
        end = address.add(resolved.getLength() - 1)
        try:
            # Every row comes from an explicit GLOBAL address marker. Retain
            # unrelated interior symbols, but permit an older primary name.
            clear_code_units_guarded(
                program,
                address,
                end,
                expected_name=str(row.get("name") or "") or None,
                address_owned=True,
                allow_contained_strings=bool(
                    re.fullmatch(
                        r"(?:static )?char\[\d+\]\[\d+\]", str(row.get("source_type") or "")
                    )
                ),
            )
        except ClearRangeError as exc:
            return {**dict(row), **exc.payload}
        listing.createData(address, resolved)
    if action in {"set-name", "set-type-and-name"}:
        name = str(row.get("name") or "")
        if name:
            set_primary_label(program, address, name, SourceType.IMPORTED)
    return {
        "address": row["address"],
        "name": row.get("name"),
        "action": action,
        "type": row.get("resolved_type") or row.get("source_type"),
    }


def apply_global_typing(program: Any, plan: Mapping[str, Any]) -> dict[str, Any]:
    """Apply planned GLOBAL type/name repairs (one transaction per row)."""

    from .ghidra.mutations import apply_rows

    rows = [
        row for row in plan.get("globals", []) if str(row.get("action") or "").startswith("set-")
    ]
    result = apply_rows(
        program,
        rows,
        _apply_global_typing_row,
        description="Source-backed GLOBAL typing",
    )
    errors: list[dict[str, Any]] = []
    skipped: list[dict[str, Any]] = []
    for row in result["errors"]:
        if row.get("error") == "clear-range-foreign-symbol":
            skipped.append({**dict(row), "skipped": "clear-range-foreign-symbol"})
        else:
            errors.append(row)
    return {
        "applied": result["applied"],
        "errors": errors,
        "skipped": skipped,
        "globals": result["rows"],
    }
