"""Guards for listing mutations that clear code units."""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

_BYTE_FILL_NAMES = frozenset(
    {
        "byte",
        "uchar",
        "char",
        "schar",
        "bool",
        "boolean",
        "sbyte",
        "int1",
        "uint1",
    }
)


class ClearRangeError(Exception):
    """Refuse ``clearCodeUnits`` when the range is not safe to clear."""

    def __init__(self, payload: Mapping[str, Any]):
        super().__init__(str(payload.get("error") or "clear-range-unsafe"))
        self.payload = dict(payload)


def _type_name(data: Any) -> str | None:
    if data is None:
        return None
    data_type = data.getDataType() if hasattr(data, "getDataType") else None
    if data_type is None:
        return None
    if hasattr(data_type, "getName"):
        return str(data_type.getName())
    return str(data_type)


def _is_undefined_listing_data(data: Any) -> bool:
    """True for missing data or a lone undefined/default byte unit."""

    if data is None:
        return True
    name = _type_name(data)
    if name is not None and name.startswith("undefined"):
        return True
    data_type = data.getDataType() if hasattr(data, "getDataType") else None
    if data_type is None:
        return True
    type_name = type(data_type).__name__
    return "Undefined" in type_name


def _is_clearable_fill_data(data: Any) -> bool:
    """Undefined or single-byte primitive fill that apply may replace."""

    if _is_undefined_listing_data(data):
        return True
    length = int(data.getLength()) if hasattr(data, "getLength") else 0
    if length != 1:
        return False
    name = _type_name(data)
    if name is None:
        return True
    return name.lower() in _BYTE_FILL_NAMES


def _address_offset(address: Any) -> int:
    if hasattr(address, "getOffset"):
        return int(address.getOffset())
    return int(address)


def _range_length(start: Any, end: Any) -> int:
    if hasattr(end, "subtract") and hasattr(start, "getOffset"):
        delta = end.subtract(start)
        if isinstance(delta, int) or hasattr(delta, "__int__"):
            try:
                return int(delta) + 1
            except (TypeError, ValueError):
                pass
    return _address_offset(end) - _address_offset(start) + 1


def _primary_name(program: Any, address: Any) -> str | None:
    primary = program.getSymbolTable().getPrimarySymbol(address)
    if primary is None:
        return None
    return str(primary.getName())


def _is_owned_or_debris_symbol(name: str, owner: str | None) -> bool:
    """DAT_ defaults and Structure component labels under ``owner`` are not foreign."""

    if name.startswith("DAT_"):
        return True
    if owner is None:
        return False
    if name == owner:
        return True
    return name.startswith(f"{owner}[") or name.startswith(f"{owner}.")


def assert_clear_range_safe(
    program: Any,
    start: Any,
    end: Any,
    *,
    expected_name: str | None = None,
) -> None:
    """Verify ``[start, end]`` is safe to ``clearCodeUnits``.

    Safe when:

    - defined data at ``start`` already covers the range (interior components OK), or
    - expanding that unit over clearable fill (undefined / single-byte primitives), or
    - the span is empty/undefined/fill with no instructions and no foreign primary
      symbols starting inside ``(start, end]``.

    When ``expected_name`` is provided, the primary symbol at ``start`` must match.
    """

    listing = program.getListing()
    functions = program.getFunctionManager()
    length = _range_length(start, end)
    if length <= 0:
        raise ClearRangeError(
            {
                "error": "clear-range-empty",
                "start": str(start),
                "end": str(end),
            }
        )

    start_primary = _primary_name(program, start)
    if expected_name is not None:
        if start_primary != expected_name:
            raise ClearRangeError(
                {
                    "error": "clear-range-name-mismatch",
                    "expected_name": expected_name,
                    "actual_name": start_primary,
                    "start": str(start),
                    "end": str(end),
                }
            )

    for offset in range(length):
        addr = start.add(offset) if hasattr(start, "add") else start + offset
        containing = functions.getFunctionContaining(addr)
        if containing is not None:
            raise ClearRangeError(
                {
                    "error": "clear-range-function-overlap",
                    "start": str(start),
                    "end": str(end),
                    "overlap_at": str(addr),
                    "function": (
                        containing.getName(True)
                        if hasattr(containing, "getName")
                        else str(containing)
                    ),
                }
            )

    owner = expected_name if expected_name is not None else start_primary
    foreign_symbols: list[dict[str, Any]] = []
    for offset in range(1, length):
        addr = start.add(offset) if hasattr(start, "add") else start + offset
        name = _primary_name(program, addr)
        if name is None or _is_owned_or_debris_symbol(name, owner):
            continue
        if start_primary is not None and _is_owned_or_debris_symbol(name, start_primary):
            continue
        foreign_symbols.append({"at": str(addr), "name": name})
    if foreign_symbols:
        raise ClearRangeError(
            {
                "error": "clear-range-foreign-symbol",
                "start": str(start),
                "end": str(end),
                "foreign": foreign_symbols,
            }
        )

    data_at_start = listing.getDataAt(start)
    if data_at_start is not None and not _is_clearable_fill_data(data_at_start):
        covered = int(data_at_start.getLength())
        if covered >= length:
            # Interior offsets of the same defined Data (e.g. Structure components)
            # are not foreign — they are what clearCodeUnits is meant to replace.
            return
        # Expanding an existing typed unit: the tail beyond ``covered`` must be
        # clearable fill (not another meaningful defined type / instruction).
        for offset in range(covered, length):
            addr = start.add(offset) if hasattr(start, "add") else start + offset
            data = listing.getDataAt(addr)
            if data is not None and not _is_clearable_fill_data(data):
                raise ClearRangeError(
                    {
                        "error": "clear-range-incomplete-cover",
                        "start": str(start),
                        "end": str(end),
                        "existing_length": covered,
                        "required_length": length,
                        "blocked_at": str(addr),
                        "blocked_kind": "data",
                    }
                )
            if listing.getInstructionAt(addr) is not None:
                raise ClearRangeError(
                    {
                        "error": "clear-range-incomplete-cover",
                        "start": str(start),
                        "end": str(end),
                        "existing_length": covered,
                        "required_length": length,
                        "blocked_at": str(addr),
                        "blocked_kind": "instruction",
                    }
                )
        return

    # Start is empty or clearable fill: refuse instructions anywhere in the span.
    # Other defined data under the same ownership is replaceable debris.
    if listing.getInstructionAt(start) is not None and data_at_start is None:
        raise ClearRangeError(
            {
                "error": "clear-range-instruction-at-start",
                "start": str(start),
                "end": str(end),
            }
        )

    foreign_instructions: list[dict[str, Any]] = []
    for offset in range(length):
        addr = start.add(offset) if hasattr(start, "add") else start + offset
        if listing.getInstructionAt(addr) is not None and listing.getDataAt(addr) is None:
            foreign_instructions.append({"kind": "instruction", "at": str(addr)})
    if foreign_instructions:
        raise ClearRangeError(
            {
                "error": "clear-range-foreign-defined",
                "start": str(start),
                "end": str(end),
                "foreign": foreign_instructions,
            }
        )


def clear_code_units_guarded(
    program: Any,
    start: Any,
    end: Any,
    *,
    expected_name: str | None = None,
    clear_context: bool = False,
) -> None:
    """``clearCodeUnits`` after ``assert_clear_range_safe``."""

    assert_clear_range_safe(program, start, end, expected_name=expected_name)
    program.getListing().clearCodeUnits(start, end, clear_context)
