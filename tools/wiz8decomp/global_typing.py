"""Apply source-backed GLOBAL marker types into the live Ghidra listing.

Uses ``parse_global_definitions`` ownership/type facts. Only addresses with a
resolved DataType whose length matches the source size (when known) are
actionable. Speculative layouts are never invented here.
"""

from __future__ import annotations

import re
from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .global_model import parse_global_definitions
from .paths import atomic_json, repo_relative

_SCHEMA = "wiz8.global-typing-v1"
_ARRAY_SUFFIX = re.compile(r"^(?P<base>.+?)(?P<arrays>(?:\[\d*\])+)$")
_POINTER_SUFFIX = re.compile(r"^(?P<base>.+?)\s*(?P<stars>\*+)\s*$")
_TEMPLATE = re.compile(r"<([^<>]+)>")
_CLASS_LIKE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*$")


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
        for prefix in ("extern ", "const ", "volatile ", "static "):
            if text.startswith(prefix):
                text = text[len(prefix) :].lstrip()
                changed = True
    return text


def _ghidra_type_name(type_name: str) -> str:
    """Map C++ template spelling to Ghidra's ``T[Args]`` Structure names."""

    text = _strip_qualifiers(type_name)
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
        "unsigned char": UnsignedCharDataType,
        "uchar": UnsignedCharDataType,
        "byte": ByteDataType,
        "UINT8": UnsignedCharDataType,
        "BOOLEAN": UnsignedCharDataType,
        "short": ShortDataType,
        "unsigned short": UnsignedShortDataType,
        "UINT16": UnsignedShortDataType,
        "wchar_t": WideCharDataType,
        "int": IntegerDataType,
        "long": LongDataType,
        "unsigned": UnsignedIntegerDataType,
        "unsigned int": UnsignedIntegerDataType,
        "unsigned long": UnsignedLongDataType,
        "UINT32": UnsignedIntegerDataType,
        "DWORD": UnsignedIntegerDataType,
        "float": FloatDataType,
        "double": DoubleDataType,
    }
    factory = builtins.get(name)
    return factory() if factory is not None else None


def _named_data_type(program: Any, name: str) -> Any | None:
    manager = program.getDataTypeManager()
    text = name.strip()
    if not text or text in {"*", "[]"}:
        return None
    simple = text.split("::")[-1].strip()
    if not simple:
        return None

    candidates: list[str] = []
    if "::" in text:
        parts = [part.strip() for part in text.split("::") if part.strip()]
        if parts:
            candidates.append("/" + "/".join(parts))
        candidates.append(f"/{text}")
    candidates.extend(
        (
            f"/{simple}",
            f"/wiz8/sgp/{simple}",
            f"/Demangler/{simple}",
        )
    )
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
    frozenset({"int", "long", "int32", "INT32", "undefined4"}),
    frozenset({"uint", "ulong", "unsigned int", "unsigned long", "UINT32", "DWORD"}),
    frozenset({"ushort", "unsigned short", "UINT16", "word"}),
    frozenset({"float", "Float4"}),
    frozenset({"double", "Float8"}),
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
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    for definition in parse_global_definitions(repository):
        if definition.get("target") != target:
            continue
        address = int(definition["address"])
        if wanted is not None and address not in wanted:
            continue
        resolved = resolve_data_type(program, str(definition.get("type") or ""))
        if resolved is None:
            action = "unresolved-type"
            resolved_name = None
            resolved_length = None
        else:
            resolved_name = str(resolved.getName())
            resolved_length = int(resolved.getLength())
            expected = definition.get("size")
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
                "source_type": definition.get("type"),
                "source_size": definition.get("size"),
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
            clear_code_units_guarded(
                program,
                address,
                end,
                expected_name=str(row.get("name") or "") or None,
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


def run_global_typing(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Report or apply source-backed GLOBAL listing types."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_global_typing_plan(
            settings.repo_dir,
            program,
            target=target,
            addresses=addresses,
            limit=limit,
        )
        out_dir = settings.build_dir / "global-typing"
        report_path = out_dir / "report.json"
        atomic_json(report_path, {**plan, "program": program_name, "apply": apply})
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": repo_relative(report_path, settings.repo_dir),
            "sample": [row for row in plan["globals"] if str(row["action"]).startswith("set-")][
                :20
            ],
        }
        if not apply:
            return result

        applied = apply_global_typing(program, plan)
        dispose_sessions()
        program.save("Source-backed GLOBAL typing", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["sample"] = applied["globals"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = repo_relative(error_path, settings.repo_dir)
        return result
