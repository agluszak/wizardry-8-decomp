"""Type Cosmic Forge ``.cfdat`` destination tables in the live listing.

``evidence/reviewed/wiz8/formats/cfdat-overrides.csv`` records English
destination addresses, canonical sizes, and element layout for race /
profession / skill override blobs. When those addresses are still undefined
(or weaker than a sized array), install the CSV-specified arrays and
CF-derived names.
"""

from __future__ import annotations

import csv
import re
from collections import Counter
from collections.abc import Mapping
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json

_SCHEMA = "wiz8.cosmic-forge-globals-v1"
_OVERRIDES = Path("evidence/reviewed/wiz8/formats/cfdat-overrides.csv")

_ELEMENT_SIZES = {
    "byte": 1,
    "uchar": 1,
    "float": 4,
    "int": 4,
}

# Datatype strength for Cosmic Forge upgrades:
# undefined/null=0, generic byte/uchar array=1, typed CF array match=3.
# Named reviewed structures also rank >= 3 so partially-typed rows skip them.
_RANK_UNDEFINED = 0
_RANK_GENERIC = 1
_RANK_TYPED = 3

_ARRAY_NAME = re.compile(
    r"^(?P<base>[A-Za-z_][\w:]*)\s*\[\s*(?P<count>\d+)\s*\]$",
)


def _load_overrides(repository: Path) -> list[dict[str, Any]]:
    path = repository / _OVERRIDES
    rows: list[dict[str, Any]] = []
    with path.open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            status = (row.get("status") or "").strip()
            if status not in {"typed", "partially-typed"}:
                continue
            destination = (row.get("english_destination") or "").strip()
            size_text = (row.get("canonical_size") or "").strip()
            if not destination or not size_text:
                continue
            try:
                address = int(destination, 0)
                size = int(size_text, 0)
            except ValueError:
                continue
            if size <= 0:
                continue
            element_type = (row.get("element_type") or "").strip().casefold() or None
            count_text = (row.get("element_count") or "").strip()
            element_count: int | None = None
            if count_text:
                try:
                    element_count = int(count_text, 0)
                except ValueError:
                    continue
            rows.append(
                {
                    "filename": row.get("filename"),
                    "address": address,
                    "size": size,
                    "status": status,
                    "evidence": row.get("evidence"),
                    "element_type": element_type,
                    "element_count": element_count,
                }
            )
    return rows


def _symbol_name(filename: str | None) -> str:
    stem = (filename or "cfdat").rsplit(".", 1)[0]
    cleaned = "".join(ch if ch.isalnum() else "_" for ch in stem)
    return f"g_cf_{cleaned}"


def _element_data_type(element_type: str) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        FloatDataType,
        IntegerDataType,
        UnsignedCharDataType,
    )

    key = element_type.casefold()
    if key in {"byte", "uchar"}:
        return UnsignedCharDataType()
    if key == "float":
        return FloatDataType()
    if key == "int":
        return IntegerDataType()
    raise ValueError(f"unsupported cfdat element_type: {element_type}")


def _array_from_override(override: dict[str, Any]) -> Any:
    """Build ``ArrayDataType`` from CSV element_type + element_count."""

    from ghidra.program.model.data import ArrayDataType  # type: ignore[import-not-found]

    element_type = override.get("element_type")
    element_count = override.get("element_count")
    size = int(override["size"])
    if not element_type or element_count is None:
        raise ValueError(f"missing element_type/element_count for {override.get('filename')}")
    count = int(element_count)
    if count <= 0:
        raise ValueError(f"invalid element_count for {override.get('filename')}: {count}")
    expected_size = _ELEMENT_SIZES.get(str(element_type).casefold())
    if expected_size is not None and count * expected_size != size:
        raise ValueError(
            f"element layout mismatch for {override.get('filename')}: "
            f"{count}*{expected_size} != canonical_size {size}"
        )
    element = _element_data_type(str(element_type))
    return ArrayDataType(element, count, element.getLength())


def _parse_array_name(type_name: str) -> tuple[str, int] | None:
    match = _ARRAY_NAME.match(type_name.strip())
    if match is None:
        return None
    return match.group("base"), int(match.group("count"))


def _element_names_equivalent(left: str, right: str) -> bool:
    a = left.casefold()
    b = right.casefold()
    if a == b:
        return True
    byteish = {"byte", "uchar", "undefined1", "unsigned char"}
    return a in byteish and b in byteish


def _is_structure_data_type(data_type: Any | None) -> bool:
    """True when ``data_type`` unwraps to a Structure or Union."""

    if data_type is None:
        return False
    current = data_type
    while "TypeDef" in type(current).__name__ and hasattr(current, "getBaseDataType"):
        current = current.getBaseDataType()
        if current is None:
            return False
    structure_types: tuple[type, ...] | None = None
    try:
        from ghidra.program.model.data import (  # type: ignore[import-not-found]
            Structure,
            Union,
        )

        structure_types = (Structure, Union)
    except Exception:  # noqa: BLE001 — unit tests without Ghidra
        structure_types = None
    if structure_types is not None and isinstance(current, structure_types):
        return True
    # Name fallback for fakes and when the live type is not a Structure/Union.
    name = type(current).__name__
    return name in {"Structure", "StructureDataType", "Union", "UnionDataType"}


def _is_named_structure(
    type_name: str | None,
    *,
    data_type: Any | None = None,
) -> bool:
    """True when the listing type is a named Structure/Union (not a scalar/array).

    Prefer the live DataType when available. Name-only fallback treats only
    non-scalar, non-pointer, non-array spellings as structure-like.
    """

    if data_type is not None:
        return _is_structure_data_type(data_type)
    if type_name is None or type_name.startswith("undefined"):
        return False
    if _parse_array_name(type_name) is not None:
        return False
    stripped = type_name.strip()
    if not stripped or stripped.endswith("*"):
        return False
    leaf = stripped.split()[0] if stripped else ""
    scalars = {
        "bool",
        "byte",
        "uchar",
        "char",
        "short",
        "ushort",
        "int",
        "uint",
        "long",
        "ulong",
        "float",
        "double",
        "void",
        "wchar_t",
        "undefined1",
        "undefined2",
        "undefined4",
        "undefined8",
        "word",
        "dword",
        "qword",
        "pointer",
        "string",
        "TerminatedCString",
    }
    return leaf.casefold() not in scalars


def _datatype_rank(
    type_name: str | None,
    *,
    element_type: str | None,
    element_count: int | None,
    length: int | None,
    size: int,
    data_type: Any | None = None,
) -> int:
    """Rank current listing type for Cosmic Forge upgrade decisions."""

    if type_name is None or type_name.startswith("undefined"):
        return _RANK_UNDEFINED
    if _is_named_structure(type_name, data_type=data_type):
        return _RANK_TYPED
    parsed = _parse_array_name(type_name)
    if parsed is None:
        return _RANK_UNDEFINED
    base, count = parsed
    if (
        element_type is not None
        and element_count is not None
        and _element_names_equivalent(base, str(element_type))
        and count == int(element_count)
        and length == size
    ):
        return _RANK_TYPED
    if base.casefold() in {"byte", "uchar", "undefined1", "unsigned char"}:
        return _RANK_GENERIC
    # Non-generic array that does not match CF evidence (e.g. wrong int count).
    return _RANK_GENERIC + 1


def _matches_cf_layout(
    type_name: str | None,
    *,
    length: int | None,
    size: int,
    element_type: str | None,
    element_count: int | None,
) -> bool:
    if type_name is None or element_type is None or element_count is None or length != size:
        return False
    parsed = _parse_array_name(type_name)
    if parsed is None:
        return False
    base, count = parsed
    return _element_names_equivalent(base, str(element_type)) and count == int(element_count)


def _wrong_element_or_count(
    type_name: str | None,
    *,
    element_type: str | None,
    element_count: int | None,
) -> bool:
    if type_name is None or element_type is None or element_count is None:
        return False
    parsed = _parse_array_name(type_name)
    if parsed is None:
        return False
    base, count = parsed
    return not _element_names_equivalent(base, str(element_type)) or count != int(element_count)


def _cf_type_decision(
    *,
    status: str,
    current_type: str | None,
    current_length: int | None,
    size: int,
    element_type: str | None,
    element_count: int | None,
    data_type: Any | None = None,
) -> tuple[bool, str | None]:
    """Return ``(needs_type, skip_action)``.

    ``skip_action`` is ``preserve-reviewed`` when a richer reviewed structure
    must not be overwritten by a weaker Cosmic Forge layout.
    """

    if status == "partially-typed" and _is_named_structure(current_type, data_type=data_type):
        return False, "preserve-reviewed"

    if _matches_cf_layout(
        current_type,
        length=current_length,
        size=size,
        element_type=element_type,
        element_count=element_count,
    ):
        return False, None

    rank = _datatype_rank(
        current_type,
        element_type=element_type,
        element_count=element_count,
        length=current_length,
        size=size,
        data_type=data_type,
    )
    wrong_shape = _wrong_element_or_count(
        current_type, element_type=element_type, element_count=element_count
    )

    if status == "typed":
        if (
            _is_named_structure(current_type, data_type=data_type)
            and current_length == size
            and rank >= _RANK_TYPED
        ):
            return False, "preserve-reviewed"
        if current_type is None or current_type.startswith("undefined"):
            return True, None
        if current_length != size or wrong_shape:
            return True, None
        return False, None

    # partially-typed: only upgrade weaker undefined/generic layouts.
    if rank >= _RANK_TYPED:
        return False, "preserve-reviewed"
    if (
        current_type is None
        or current_type.startswith("undefined")
        or wrong_shape
        or (current_length != size)
    ):
        return True, None
    return False, None


def collect_cosmic_forge_plan(repository: Path, program: Any) -> dict[str, Any]:
    """Plan typed arrays for accepted Cosmic Forge destination blobs."""

    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    symbols = program.getSymbolTable()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()

    for override in _load_overrides(repository):
        address = int(override["address"])
        size = int(override["size"])
        entry = space.getAddress(address)
        data = listing.getDataAt(entry)
        symbol = symbols.getPrimarySymbol(entry)
        current_dt = data.getDataType() if data is not None else None
        current_type = str(current_dt.getName()) if current_dt is not None else None
        current_length = int(data.getLength()) if data is not None else None
        name = _symbol_name(override.get("filename"))
        needs_type, skip_action = _cf_type_decision(
            status=str(override.get("status") or ""),
            current_type=current_type,
            current_length=current_length,
            size=size,
            element_type=override.get("element_type"),
            element_count=override.get("element_count"),
            data_type=current_dt,
        )
        needs_name = symbol is None or str(symbol.getName()).startswith("DAT_")
        if skip_action == "preserve-reviewed":
            action = "preserve-reviewed"
        elif needs_type and needs_name:
            action = "set-type-and-name"
        elif needs_type:
            action = "set-type"
        elif needs_name:
            action = "set-name"
        else:
            action = "agree"
        counts[action] += 1
        if action == "agree":
            continue
        rows.append(
            {
                "filename": override.get("filename"),
                "address": f"0x{address:08x}",
                "size": size,
                "name": name,
                "element_type": override.get("element_type"),
                "element_count": override.get("element_count"),
                "ghidra_type": current_type,
                "ghidra_length": current_length,
                "ghidra_symbol": str(symbol.getName()) if symbol is not None else None,
                "action": action,
                "status": override.get("status"),
            }
        )

    return {
        "schema": _SCHEMA,
        "counts": dict(sorted(counts.items())),
        "actionable": sum(counts[key] for key in counts if key.startswith("set-")),
        "globals": rows,
    }


def _apply_cosmic_forge_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    from .global_typing import set_primary_label

    listing = program.getListing()
    space = program.getAddressFactory().getDefaultAddressSpace()
    action = str(row.get("action") or "")
    address = space.getAddress(int(row["address"], 0))
    if action in {"set-type", "set-type-and-name"}:
        data_type = _array_from_override(dict(row))
        end = address.add(data_type.getLength() - 1)
        listing.clearCodeUnits(address, end, False)
        listing.createData(address, data_type)
    if action in {"set-name", "set-type-and-name"}:
        set_primary_label(program, address, str(row["name"]), SourceType.IMPORTED)
    return {
        "address": row["address"],
        "name": row.get("name"),
        "filename": row.get("filename"),
        "action": action,
    }


def apply_cosmic_forge_globals(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Apply planned Cosmic Forge destination typings (one transaction per row)."""

    from .ghidra.mutations import apply_rows

    rows = [
        row for row in plan.get("globals", []) if str(row.get("action") or "").startswith("set-")
    ]
    result = apply_rows(
        program,
        rows,
        _apply_cosmic_forge_row,
        description="Type Cosmic Forge cfdat destinations",
    )
    return {
        "applied": result["applied"],
        "errors": result["errors"],
        "globals": result["rows"],
    }


def run_cosmic_forge_globals(
    settings: Settings,
    *,
    program_name: str = "wiz8",
    apply: bool = False,
) -> dict[str, Any]:
    """Report or apply Cosmic Forge ``.cfdat`` destination typings."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_cosmic_forge_plan(settings.repo_dir, program)
        out_dir = settings.build_dir / "cosmic-forge-globals"
        report_path = out_dir / "report.json"
        atomic_json(report_path, {**plan, "program": program_name, "apply": apply})
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": str(report_path.relative_to(settings.repo_dir)),
            "sample": plan["globals"][:20],
        }
        if not apply:
            return result
        applied = apply_cosmic_forge_globals(program, plan)
        dispose_sessions()
        program.save("Type Cosmic Forge cfdat destinations", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["sample"] = applied["globals"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = str(error_path.relative_to(settings.repo_dir))
        return result
