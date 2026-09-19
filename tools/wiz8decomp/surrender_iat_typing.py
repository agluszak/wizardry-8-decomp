"""Project SurRender retail ABI onto Wiz8 IAT/externals.

Evidence is ``evidence/observations/surrender/wiz8-sr-imports.csv``: decorated
name, demangled signature, calling convention, and IAT address. The pass writes conventions onto thunk/external functions whose Ghidra model is
``default``/``unknown``, writes parameter/return types when every demangled type
resolves, and types the IAT cell itself (``Pointer(FunctionDefinition)`` for
callables; object/vftable pointer cells for data imports). It does not invent
types or overwrite ``USER_DEFINED`` signatures. Full resolved ABI uses
``IMPORTED`` provenance; convention-only repair stays ``ANALYSIS``.
"""

from __future__ import annotations

import csv
import hashlib
import re
from collections import Counter
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .ghidra.mutations import apply_rows, auto_parameters

_SCHEMA = "wiz8.surrender-iat-typing-v2"
_IMPORTS = Path("evidence/observations/surrender/wiz8-sr-imports.csv")
_CALLABLE = frozenset(
    {
        "free-function",
        "method",
        "constructor",
        "destructor",
        "operator",
        "vbase-destructor",
    }
)
_DATA_KINDS = frozenset({"global-object", "vftable"})
_GHIDRA_MODELS = frozenset({"__cdecl", "__stdcall", "__fastcall", "__thiscall"})
_IAT_FUNCTIONS = "/wiz8/surrender-iat/functions"
_IAT_CALLBACKS = "/wiz8/surrender-iat/callbacks"
_SOFT = frozenset({"default", "unknown"})
_PROTECTED = frozenset({"USER_DEFINED"})
_CALLING_CONVENTION = re.compile(r"__(?:thiscall|stdcall|cdecl|fastcall)")
_ACCESS = re.compile(r"^(?:public|private|protected):\s*")
_VIRTUAL_STATIC = re.compile(r"^(?:virtual|static)\s+")
_OPERATOR = re.compile(r"\boperator\s*\S+$")
_VBASE = re.compile(r"`[^`]+`$")
_QUALIFIED_NAME = re.compile(r"(?:[A-Za-z_]\w*::)*~?[A-Za-z_]\w*$")
_TRAILING_SCOPE = re.compile(r"(?:[A-Za-z_]\w*::)+$")
_FN_PTR = re.compile(
    r"^(?P<ret>.+?)\s*\(\s*(?P<cc>__(?:cdecl|stdcall|thiscall|fastcall)\s*)?\*\s*\)\s*"
    r"\((?P<args>.*)\)\s*$"
)
_IAT_NAME_SAFE = re.compile(r"[^A-Za-z0-9_]+")


@dataclass(frozen=True)
class ParsedCallable:
    convention: str
    return_type: str
    parameters: tuple[str, ...]
    varargs: bool
    has_function_pointer_param: bool


def _signature_source(function: Any) -> str:
    source = function.getSignatureSource()
    name = getattr(source, "name", None)
    if callable(name):
        try:
            name = name()
        except TypeError:
            name = None
    if name:
        return str(name)
    return str(source)


def load_surrender_imports(repository: Path) -> list[dict[str, str]]:
    path = repository / _IMPORTS
    with path.open(encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def _split_top_level(text: str, separator: str = ",") -> list[str]:
    parts: list[str] = []
    depth = 0
    start = 0
    for index, char in enumerate(text):
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
        elif char == separator and depth == 0:
            parts.append(text[start:index].strip())
            start = index + 1
    parts.append(text[start:].strip())
    return [part for part in parts if part]


def _argument_span(signature: str) -> tuple[int, int] | None:
    depth = 0
    start: int | None = None
    last: tuple[int, int] | None = None
    for index, char in enumerate(signature):
        if char == "(":
            if depth == 0:
                start = index
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0 and start is not None:
                last = (start, index)
    return last


def _return_type_from_prefix(prefix: str) -> str:
    text = _ACCESS.sub("", prefix.strip())
    while _VIRTUAL_STATIC.match(text):
        text = _VIRTUAL_STATIC.sub("", text)
    text = _CALLING_CONVENTION.sub("", text)
    text = " ".join(text.split())
    text = _OPERATOR.sub("", text).strip()
    text = _VBASE.sub("", text).strip()
    text = _QUALIFIED_NAME.sub("", text).strip()
    text = _TRAILING_SCOPE.sub("", text).strip()
    return text or "void"


def parse_demangled_callable(signature: str) -> ParsedCallable | None:
    """Parse an MSVC demangled callable; None when there is no argument list."""

    text = signature.strip()
    span = _argument_span(text)
    if span is None:
        return None
    start, end = span
    args_text = text[start + 1 : end].strip()
    prefix = text[:start].strip()
    convention_match = _CALLING_CONVENTION.search(prefix)
    convention = convention_match.group(0) if convention_match else ""
    if args_text in {"", "void"}:
        parameters: tuple[str, ...] = ()
        varargs = False
    else:
        parts = _split_top_level(args_text)
        varargs = bool(parts) and parts[-1] == "..."
        if varargs:
            parts = parts[:-1]
        parameters = tuple(parts)
    return ParsedCallable(
        convention=convention,
        return_type=_return_type_from_prefix(prefix),
        parameters=parameters,
        varargs=varargs,
        has_function_pointer_param=any("(" in part for part in parameters),
    )


def normalize_msvc_type(spelling: str) -> str:
    """Drop MSVC demangler noise so ``resolve_data_type`` can see the named type."""

    text = spelling.strip()
    for prefix in ("class ", "struct ", "enum ", "union "):
        if text.startswith(prefix):
            text = text[len(prefix) :].lstrip()
    text = text.replace("&", "*")
    text = text.replace(" const", "").replace(" volatile", "")
    text = text.replace("const ", "").replace("volatile ", "")
    return " ".join(text.split())


def _external_or_import_thunk(function: Any) -> Any | None:
    """Return the external import, or None when ``function`` is an ordinary body."""

    if function is None:
        return None
    if function.isExternal():
        return function
    if not function.isThunk():
        return None
    thunked = function.getThunkedFunction(True)
    if thunked is None:
        return None
    if thunked.isExternal():
        return thunked
    if thunked.isThunk():
        return _external_or_import_thunk(thunked)
    return None


def _function_for_iat(program: Any, address: int) -> Any | None:
    """Thunk/external that owns this IAT cell. Ordinary CALL [IAT] callers do not qualify."""

    space = program.getAddressFactory().getDefaultAddressSpace()
    addr = space.getAddress(address)
    manager = program.getFunctionManager()
    found = _external_or_import_thunk(manager.getFunctionAt(addr))
    if found is not None:
        return found
    refs = program.getReferenceManager().getReferencesTo(addr)
    while refs.hasNext():
        ref = refs.next()
        owner = manager.getFunctionContaining(ref.getFromAddress())
        found = _external_or_import_thunk(owner)
        if found is not None:
            return found
    return None


def _explicit_parameters(function: Any) -> list[Any]:
    parameters = list(function.getParameters())
    return [
        parameter
        for parameter in parameters
        if not (hasattr(parameter, "isAutoParameter") and parameter.isAutoParameter())
    ]


def _type_display(data_type: Any | None) -> str:
    if data_type is None:
        return "undefined"
    if hasattr(data_type, "getDisplayName"):
        return str(data_type.getDisplayName())
    return str(data_type)


def _is_untyped(data_type: Any | None) -> bool:
    """True for ProgramDB's ``undefined*`` placeholders, not for authored ``void *``."""

    leaf = _type_display(data_type).casefold().rsplit("/", 1)[-1].strip()
    return leaf.startswith("undefined")


def _sanitize_iat_name(text: str) -> str:
    cleaned = _IAT_NAME_SAFE.sub("_", text).strip("_")
    return cleaned[:80]


def _iat_symbol_leaf(decorated_name: str | None) -> str:
    text = str(decorated_name or "").lstrip("?")
    leaf = text.split("@", 1)[0]
    return _sanitize_iat_name(leaf) or "import"


def iat_callable_definition_name(decorated_name: str | None, address: int) -> str:
    """Stable FunctionDefinition identity for one imported callable."""

    return f"{address:08x}_{_iat_symbol_leaf(decorated_name)}"


def iat_callback_definition_name(
    return_type: str,
    parameters: tuple[str, ...],
    convention: str,
    varargs: bool,
) -> str:
    """Share a callback FunctionDefinition only when the complete contract matches."""

    payload = "|".join(
        (
            convention,
            normalize_msvc_type(return_type),
            *(normalize_msvc_type(part) for part in parameters),
            "..." if varargs else "",
        )
    )
    digest = hashlib.sha1(payload.encode("utf-8")).hexdigest()[:12]
    return f"iat_callback_{digest}"


def _resolve_function_pointer_type(program: Any, spelling: str) -> Any | None:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
    )

    text = normalize_msvc_type(spelling)
    match = _FN_PTR.match(text)
    if match is None:
        return None
    return_type = _resolve_surrender_type(program, match.group("ret").strip() or "void")
    if return_type is None:
        return None
    convention = (match.group("cc") or "__cdecl").strip()
    args_text = match.group("args").strip()
    if args_text in {"", "void"}:
        parts: list[str] = []
        varargs = False
    else:
        parts = _split_top_level(args_text)
        varargs = bool(parts) and parts[-1] == "..."
        if varargs:
            parts = parts[:-1]
    params: list[Any] = []
    for index, part in enumerate(parts):
        data_type = _resolve_surrender_type(program, part)
        if data_type is None:
            return None
        params.append(ParameterDefinitionImpl(f"param_{index}", data_type, None))
    manager = program.getDataTypeManager()
    from .ghidra.mutations import program_transaction

    with program_transaction(program, "Resolve IAT callback type"):
        manager.createCategory(CategoryPath(_IAT_CALLBACKS))
        definition_name = iat_callback_definition_name(
            match.group("ret").strip() or "void",
            tuple(parts),
            convention,
            varargs,
        )
        definition = FunctionDefinitionDataType(CategoryPath(_IAT_CALLBACKS), definition_name)
        definition.setReturnType(return_type)
        if params:
            definition.setArguments(params)
        definition.setCallingConvention(convention)
        if varargs and hasattr(definition, "setVarArgs"):
            definition.setVarArgs(True)
        added = manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
    return PointerDataType(added, manager)


def _resolve_surrender_type(program: Any, spelling: str) -> Any | None:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        PointerDataType,
        VoidDataType,
    )

    from .global_typing import resolve_data_type

    text = normalize_msvc_type(spelling)
    if not text:
        return None
    if _FN_PTR.match(text):
        return _resolve_function_pointer_type(program, text)
    stars = 0
    while text.endswith("*"):
        stars += 1
        text = text[:-1].rstrip()
    if text == "void":
        data_type: Any = VoidDataType()
    else:
        data_type = resolve_data_type(program, text)
        if data_type is None:
            return None
    manager = program.getDataTypeManager()
    for _ in range(stars):
        data_type = PointerDataType(data_type, manager)
    return data_type


def _resolved_signature(program: Any, parsed: ParsedCallable) -> tuple[Any, list[Any]] | None:
    return_type = _resolve_surrender_type(program, parsed.return_type)
    if return_type is None:
        return None
    parameters: list[Any] = []
    for spelling in parsed.parameters:
        data_type = _resolve_surrender_type(program, spelling)
        if data_type is None:
            return None
        parameters.append(data_type)
    return return_type, parameters


def _audit_ghidra(
    function: Any,
    parsed: ParsedCallable | None,
    convention: str,
    resolved: tuple[Any, list[Any]] | None = None,
) -> dict[str, str]:
    """Defects between a live callee and its demangled SurRender signature.

    Types are compared as ProgramDB-resolved shapes, not display spellings:
    ``ulong`` and ``unsigned long`` are the same type, and only genuinely
    untyped ``undefined*`` storage is a defect (an authored ``void *`` is not).
    """

    from .datatype_contracts import datatype_shape_key

    ghidra_cc = str(function.getCallingConventionName() or "unknown")
    source = _signature_source(function)
    audit: dict[str, str] = {
        "ghidra_cc": ghidra_cc,
        "signature_source": source,
    }
    if ghidra_cc != convention:
        audit["wrong_cc"] = "1"
    parameters = _explicit_parameters(function)
    thiscall = convention == "__thiscall" or (
        parsed is not None and parsed.convention == "__thiscall"
    )
    if thiscall:
        autos = [
            parameter
            for parameter in function.getParameters()
            if hasattr(parameter, "isAutoParameter") and parameter.isAutoParameter()
        ]
        if not autos:
            audit["wrong_this"] = "1"
    if any(_is_untyped(parameter.getDataType()) for parameter in parameters):
        audit["undefined_pointer_or_param"] = "1"
    return_type = function.getReturnType() if hasattr(function, "getReturnType") else None
    if parsed is not None:
        if len(parameters) != len(parsed.parameters):
            audit["wrong_parameter_count"] = "1"
        if resolved is None:
            audit["unresolved_signature_types"] = "1"
        else:
            expected_return, expected_parameters = resolved
            if len(parameters) == len(expected_parameters) and any(
                datatype_shape_key(parameter.getDataType()) != datatype_shape_key(expected)
                for parameter, expected in zip(parameters, expected_parameters, strict=True)
            ):
                audit["wrong_parameter_type"] = "1"
            # The return type stands on its own: a parameter-count defect must
            # not mask a wrong return.
            if datatype_shape_key(return_type) != datatype_shape_key(expected_return):
                audit["wrong_return"] = "1"
        actual_varargs = bool(function.hasVarArgs()) if hasattr(function, "hasVarArgs") else False
        if actual_varargs != bool(parsed.varargs):
            audit["wrong_varargs"] = "1"
    elif _is_untyped(return_type):
        audit["wrong_return"] = "1"
    if source in {"DEFAULT", "ANALYSIS"}:
        audit["default_or_analysis"] = "1"
    if not any(
        key in audit
        for key in (
            "wrong_cc",
            "undefined_pointer_or_param",
            "wrong_parameter_count",
            "wrong_parameter_type",
            "wrong_varargs",
            "wrong_return",
            "wrong_this",
            "unresolved_signature_types",
        )
    ):
        audit["exact"] = "1"
    return audit


def iat_data_pointer_levels(signature: str) -> int | None:
    """IAT cell pointer depth for a data import: object ``T`` is ``T*``, ``T*`` is ``T**``."""

    value = _imported_data_value_type(signature)
    if not value:
        return None
    return 1 + value.count("*")


def _imported_data_value_type(signature: str) -> str | None:
    text = _ACCESS.sub("", signature.strip())
    while _VIRTUAL_STATIC.match(text):
        text = _VIRTUAL_STATIC.sub("", text)
    if "`vftable'" in text:
        return None
    text = normalize_msvc_type(text)
    text = _QUALIFIED_NAME.sub("", text).strip()
    return text or None


def _vftable_iat_pointer(program: Any, item: Mapping[str, str]) -> Any | None:
    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

    from .surrender_abi import vftable_base_from_signature
    from .vftable_typing import _vftable_paths

    owner = str(item.get("class_name") or "")
    if not owner:
        return None
    signature = str(item.get("demangled_signature") or "")
    base = vftable_base_from_signature(signature)
    if not base or base == owner:
        base = None
    _category, _name, path, _sigs = _vftable_paths(owner, base_class=base)
    structure = program.getDataTypeManager().getDataType(path)
    if structure is None:
        return None
    return PointerDataType(structure, program.getDataTypeManager())


def _data_iat_pointer(program: Any, item: Mapping[str, str]) -> Any | None:
    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

    kind = str(item.get("kind") or "")
    if kind == "vftable":
        return _vftable_iat_pointer(program, item)
    value_spelling = _imported_data_value_type(str(item.get("demangled_signature") or ""))
    if not value_spelling:
        return None
    value_type = _resolve_surrender_type(program, value_spelling)
    if value_type is None:
        return None
    return PointerDataType(value_type, program.getDataTypeManager())


def _unwrap_function_definition(data_type: Any) -> Any | None:
    current = data_type
    while (
        current is not None
        and hasattr(current, "getDataType")
        and (
            "Pointer" in type(current).__name__
            or (callable(getattr(current, "isPointer", None)) and current.isPointer())
        )
    ):
        current = current.getDataType()
    if current is None:
        return None
    if "FunctionDefinition" in type(current).__name__ or (
        hasattr(current, "getArguments") and hasattr(current, "getReturnType")
    ):
        return current
    return None


def _iat_cell_function_definition(program: Any, address: int) -> Any | None:
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    data = listing.getDataAt(space.getAddress(address))
    if data is None or not hasattr(data, "getDataType"):
        return None
    return _unwrap_function_definition(data.getDataType())


def _listing_data_type(program: Any, address: int) -> Any | None:
    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    data = listing.getDefinedDataAt(space.getAddress(address))
    if data is None:
        return None
    return data.getDataType()


def _normalize_contract_convention(contract: tuple[Any, ...], expected: str) -> tuple[Any, ...]:
    """Treat Ghidra's unresolved/default convention as the expected one."""

    if len(contract) < 3:
        return contract
    current = str(contract[2] or "")
    if current in {"", "unknown", "default"}:
        return (contract[0], contract[1], expected, *contract[3:])
    return contract


def _parsed_signature_contract(program: Any, parsed: ParsedCallable) -> tuple[Any, ...] | None:
    """ProgramDB-resolved ABI contract for a parsed demangled signature."""

    from .datatype_contracts import datatype_shape_key

    resolved = _resolved_signature(program, parsed)
    if resolved is None:
        return None
    return_type, parameters = resolved
    return (
        datatype_shape_key(return_type),
        tuple(datatype_shape_key(parameter) for parameter in parameters),
        parsed.convention or "",
        parsed.varargs,
        False,
    )


def _iat_cell_matches_data_type(program: Any, address: int, data_type: Any) -> bool:
    from .datatype_contracts import datatype_shape_key

    existing = _listing_data_type(program, address)
    if existing is None or data_type is None:
        return False
    return datatype_shape_key(existing) == datatype_shape_key(data_type)


def _iat_cell_matches_callable(
    program: Any, address: int, parsed: ParsedCallable | None, function: Any
) -> bool:
    """True when the typed IAT cell already matches what this row would write.

    The expected contract mirrors the typing source: the ProgramDB-resolved
    demangled signature when it resolves, otherwise the callee's own copy.
    """

    from .datatype_contracts import function_definition_contract

    if parsed is None:
        return False
    existing = _iat_cell_function_definition(program, address)
    if existing is None:
        return False
    actual = function_definition_contract(existing)
    expected = _parsed_signature_contract(program, parsed)
    if expected is not None:
        return _normalize_contract_convention(actual, expected[2]) == expected
    if function is None:
        return False
    mirror = _function_mirror_definition(function)
    if mirror is None:
        return False
    return actual == function_definition_contract(mirror)


def _function_mirror_definition(function: Any) -> Any | None:
    """Ghidra's own copy of a live callee signature, or None when it cannot build."""

    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        FunctionDefinitionDataType,
    )

    try:
        return FunctionDefinitionDataType(function, False)
    except Exception:  # noqa: BLE001 — mirroring a live Function can throw
        return None


def _callable_iat_pointer(
    program: Any,
    function: Any,
    parsed: ParsedCallable | None,
    *,
    decorated_name: str | None = None,
    address: int | None = None,
) -> Any | None:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        FunctionDefinitionDataType,
        PointerDataType,
    )

    if parsed is not None:
        resolved = _resolved_signature(program, parsed)
        if resolved is not None:
            return_type, parameter_types = resolved
            from ghidra.program.model.data import (  # type: ignore[import-not-found]
                CategoryPath,
                DataTypeConflictHandler,
                ParameterDefinitionImpl,
            )

            manager = program.getDataTypeManager()
            manager.createCategory(CategoryPath(_IAT_FUNCTIONS))
            definition_name = iat_callable_definition_name(decorated_name, int(address or 0))
            definition = FunctionDefinitionDataType(CategoryPath(_IAT_FUNCTIONS), definition_name)
            definition.setReturnType(return_type)
            if parameter_types:
                definition.setArguments(
                    [
                        ParameterDefinitionImpl(f"param_{index}", data_type, None)
                        for index, data_type in enumerate(parameter_types)
                    ]
                )
            if parsed.convention:
                definition.setCallingConvention(parsed.convention)
            if parsed.varargs and hasattr(definition, "setVarArgs"):
                definition.setVarArgs(True)
            added = manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
            return PointerDataType(added, manager)
    if function is None:
        return None
    try:
        definition = FunctionDefinitionDataType(function, False)
    except Exception:  # noqa: BLE001 — Ghidra FunctionDefinition copy can throw
        return None
    return PointerDataType(definition, program.getDataTypeManager())


def _apply_iat_cell_type(program: Any, address: int, data_type: Any) -> bool:
    from .ghidra.listing_guards import ClearRangeError, clear_code_units_guarded

    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    start = space.getAddress(address)
    end = start.add(data_type.getLength() - 1)
    try:
        clear_code_units_guarded(program, start, end, expected_name=None, address_owned=True)
        listing.createData(start, data_type)
    except (ClearRangeError, Exception):  # noqa: BLE001 — cell typing is all-or-nothing
        return False
    return True


def collect_surrender_iat_plan(repository: Path, program: Any) -> dict[str, Any]:
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    audit_counts: Counter[str] = Counter()
    for item in load_surrender_imports(repository):
        kind = str(item.get("kind") or "")
        convention = str(item.get("calling_convention") or "")
        iat = str(item.get("iat_address") or "")
        if not iat:
            counts["skip-no-address"] += 1
            continue
        address = int(iat, 16)
        is_callable = kind in _CALLABLE and convention in _GHIDRA_MODELS
        if not is_callable:
            if kind not in _DATA_KINDS and convention in _GHIDRA_MODELS:
                counts["skip-non-callable"] += 1
                continue
            pointer = _data_iat_pointer(program, item)
            if pointer is None:
                action = "skip-unresolved-iat-type"
                counts[action] += 1
                continue
            if _iat_cell_matches_data_type(program, address, pointer):
                counts["agree"] += 1
                continue
            action = "set-iat-cell"
            counts[action] += 1
            rows.append(
                {
                    "address": f"0x{address:08x}",
                    "decorated_name": item.get("decorated_name"),
                    "kind": kind,
                    "class_name": item.get("class_name"),
                    "demangled_signature": item.get("demangled_signature"),
                    "action": action,
                    "iat_cell": "data-pointer",
                }
            )
            continue
        parsed = parse_demangled_callable(str(item.get("demangled_signature") or ""))
        function = _function_for_iat(program, address)
        resolved = None
        apply_types = False
        cell_matches = _iat_cell_matches_callable(program, address, parsed, function)
        if function is None:
            if parsed is None:
                action = "missing-function"
            elif cell_matches:
                action = "agree"
            elif _parsed_signature_contract(program, parsed) is None:
                action = "skip-unresolved-iat-type"
            else:
                action = "set-iat-cell"
        else:
            ghidra = str(function.getCallingConventionName() or "unknown")
            source = _signature_source(function)
            resolved = _resolved_signature(program, parsed) if parsed is not None else None
            audit = _audit_ghidra(function, parsed, convention, resolved)
            for key in audit:
                if key not in {"ghidra_cc", "signature_source"}:
                    audit_counts[key] += 1
            if parsed is not None and not cell_matches:
                audit_counts["iat-cell-mismatch"] += 1
            exact = "exact" in audit
            apply_types = resolved is not None and not exact
            if source in _PROTECTED and ghidra not in _SOFT:
                action = "skip-protected" if cell_matches else "set-iat-cell"
            elif exact and ghidra == convention and cell_matches:
                action = "agree"
            elif exact and ghidra == convention:
                action = "set-iat-cell"
            elif ghidra in _SOFT or apply_types:
                action = "set-from-surrender"
            elif ghidra == convention and cell_matches:
                action = "agree"
            elif ghidra == convention:
                action = "set-iat-cell"
            else:
                action = "hard-disagree"
        counts[action] += 1
        if action not in {
            "set-from-surrender",
            "set-iat-cell",
            "hard-disagree",
            "missing-function",
        }:
            continue
        row: dict[str, Any] = {
            "address": f"0x{address:08x}",
            "decorated_name": item.get("decorated_name"),
            "kind": kind,
            "convention": convention,
            "ghidra": None if function is None else str(function.getCallingConventionName()),
            "signature_source": None if function is None else _signature_source(function),
            "action": action,
            "varargs": None if parsed is None else parsed.varargs,
            "apply_types": apply_types,
            "iat_cell": "function-pointer",
        }
        if (apply_types or action == "set-iat-cell") and parsed is not None:
            row["return_type"] = parsed.return_type
            row["parameters"] = list(parsed.parameters)
        rows.append(row)
    return {
        "schema": _SCHEMA,
        "counts": dict(sorted(counts.items())),
        "audit": dict(sorted(audit_counts.items())),
        "actionable": counts["set-from-surrender"] + counts["set-iat-cell"],
        "imports": rows,
    }


def _apply_surrender_iat_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    action = row.get("action")
    address = int(str(row["address"]), 0)
    if action == "set-iat-cell" and row.get("iat_cell") == "data-pointer":
        item = {
            "kind": str(row.get("kind") or ""),
            "class_name": str(row.get("class_name") or ""),
            "demangled_signature": str(row.get("demangled_signature") or ""),
            "decorated_name": str(row.get("decorated_name") or ""),
        }
        # Reconstruct from the live CSV row fields we stored
        pointer = None
        if row.get("kind") == "vftable":
            pointer = _vftable_iat_pointer(program, {**item, **dict(row)})
        else:
            pointer = _data_iat_pointer(program, {**item, **dict(row)})
        if pointer is None:
            return {**dict(row), "error": "unresolved-iat-type"}
        iat_typed = _apply_iat_cell_type(program, address, pointer)
        if not iat_typed:
            return {**dict(row), "error": "iat-cell-not-typed"}
        return {
            "address": row["address"],
            "decorated_name": row.get("decorated_name"),
            "action": action,
            "iat_cell_typed": True,
        }
    if action not in {"set-from-surrender", "set-iat-cell"}:
        return {**dict(row), "error": f"unexpected-action:{action}"}
    from ghidra.program.model.listing import (  # type: ignore[import-not-found]
        Function,
        ParameterImpl,
    )
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    function = _function_for_iat(program, address)
    applied_types = False
    convention = str(row.get("convention") or "")
    parsed = None
    if row.get("return_type") is not None:
        parsed = ParsedCallable(
            convention=convention,
            return_type=str(row.get("return_type") or "void"),
            parameters=tuple(str(item) for item in row.get("parameters") or []),
            varargs=bool(row.get("varargs")),
            has_function_pointer_param=False,
        )
    if function is not None:
        if (
            _signature_source(function) in _PROTECTED
            and str(function.getCallingConventionName() or "unknown") not in _SOFT
        ):
            return {**dict(row), "error": "protected-signature"}
        if action == "set-from-surrender":
            if row.get("apply_types") and parsed is not None:
                resolved = _resolved_signature(program, parsed)
                if resolved is not None:
                    return_type, parameter_types = resolved
                    parameters = [
                        ParameterImpl(f"param_{index}", data_type, program)
                        for index, data_type in enumerate(parameter_types)
                    ]
                    function.setCallingConvention(convention)
                    function.setReturnType(return_type, SourceType.IMPORTED)
                    function.replaceParameters(
                        Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
                        True,
                        SourceType.IMPORTED,
                        *auto_parameters(function),
                        *parameters,
                    )
                    function.setVarArgs(bool(row.get("varargs")))
                    function.setSignatureSource(SourceType.IMPORTED)
                    applied_types = True
            if not applied_types:
                function.setCallingConvention(convention)
                function.setSignatureSource(SourceType.ANALYSIS)
            # Full resolved ABI claims IMPORTED; convention-only stays ANALYSIS.
    pointer = _callable_iat_pointer(
        program,
        function,
        parsed,
        decorated_name=str(row.get("decorated_name") or "") or None,
        address=address,
    )
    if pointer is None:
        # The plan only asks for a cell it has already shown to be resolvable;
        # a None pointer here is a real failure, not a silent no-op.
        return {**dict(row), "error": "unresolved-iat-type", "applied_types": applied_types}
    if not _apply_iat_cell_type(program, address, pointer):
        return {**dict(row), "error": "iat-cell-not-typed", "applied_types": applied_types}
    return {
        "address": row["address"],
        "decorated_name": row.get("decorated_name"),
        "convention": convention,
        "action": action,
        "applied_types": applied_types,
        "iat_cell_typed": pointer is not None,
    }


def apply_surrender_iat_typing(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    rows = [
        row
        for row in plan.get("imports", [])
        if row.get("action") in {"set-from-surrender", "set-iat-cell"}
    ]
    return apply_rows(
        program, rows, _apply_surrender_iat_row, description="Apply SurRender IAT signatures"
    )
