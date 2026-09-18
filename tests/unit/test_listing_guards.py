"""Tests for listing clearCodeUnits range guards."""

from __future__ import annotations

from types import SimpleNamespace

import pytest
from wiz8decomp.ghidra.listing_guards import ClearRangeError, assert_clear_range_safe


class _Addr:
    def __init__(self, offset: int):
        self._offset = offset

    def getOffset(self) -> int:
        return self._offset

    def add(self, delta: int) -> _Addr:
        return _Addr(self._offset + delta)

    def subtract(self, other: _Addr) -> int:
        return self._offset - other._offset

    def __str__(self) -> str:
        return f"0x{self._offset:08x}"


def _program(
    *,
    data_at: dict[int, object] | None = None,
    instructions: set[int] | None = None,
    functions: dict[int, object] | None = None,
    primary_name: str | None = None,
    primary_names: dict[int, str] | None = None,
) -> SimpleNamespace:
    data_at = data_at or {}
    instructions = instructions or set()
    functions = functions or {}
    primary_names = dict(primary_names or {})
    if primary_name is not None:
        primary_names.setdefault(0x1000, primary_name)

    class _Listing:
        def getDataAt(self, addr: _Addr):
            return data_at.get(addr.getOffset())

        def getInstructionAt(self, addr: _Addr):
            return object() if addr.getOffset() in instructions else None

    class _Functions:
        def getFunctionContaining(self, addr: _Addr):
            offset = addr.getOffset()
            for start, fn in functions.items():
                body = getattr(fn, "body", None)
                if body is not None and body[0] <= offset <= body[1]:
                    return fn
                if start == offset:
                    return fn
            return None

    class _Symbols:
        def getPrimarySymbol(self, addr: _Addr):
            name = primary_names.get(addr.getOffset())
            if name is None:
                return None
            return SimpleNamespace(getName=lambda n=name: n)

    return SimpleNamespace(
        getListing=lambda: _Listing(),
        getFunctionManager=lambda: _Functions(),
        getSymbolTable=lambda: _Symbols(),
    )


def test_clear_range_allows_all_undefined() -> None:
    program = _program()
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x1003))


def test_clear_range_allows_covering_data_at_start() -> None:
    data = SimpleNamespace(getLength=lambda: 16, getDataType=lambda: "Foo")
    program = _program(data_at={0x1000: data})
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_allows_covering_data_even_with_interior_component_lookalikes() -> None:
    """Structure components look like data-at-offset; they are not foreign."""

    cover = SimpleNamespace(getLength=lambda: 16, getDataType=lambda: "Foo")
    component = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "int")
    program = _program(data_at={0x1000: cover, 0x1008: component})
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_allows_unowned_defined_debris_when_start_undefined() -> None:
    """Interior typed bytes without their own primary are replaceable debris."""

    foreign = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "Bar")
    program = _program(data_at={0x1008: foreign})
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_refuses_foreign_primary_symbol() -> None:
    foreign = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "Bar")
    program = _program(
        data_at={0x1008: foreign},
        primary_names={0x1000: "g_known", 0x1008: "other_global"},
    )
    with pytest.raises(ClearRangeError) as exc:
        assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F), expected_name="g_known")
    assert exc.value.payload["error"] == "clear-range-foreign-symbol"


def test_clear_range_allows_dat_and_component_labels_under_owner() -> None:
    program = _program(
        primary_names={
            0x1000: "g_known",
            0x1004: "DAT_001004",
            0x1008: "g_known[0].field",
            0x100C: "g_known.nested",
        }
    )
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F), expected_name="g_known")


def test_clear_range_allows_expansion_over_byte_fill() -> None:
    typed = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "dword")
    uchar = SimpleNamespace(
        getLength=lambda: 1,
        getDataType=lambda: SimpleNamespace(getName=lambda: "uchar"),
    )
    data_at = {0x1000: typed}
    data_at.update({offset: uchar for offset in range(0x1004, 0x1010)})
    program = _program(data_at=data_at)
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_allows_expansion_over_undefined_tail() -> None:
    data = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "byte")
    program = _program(data_at={0x1000: data})
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_allows_undefined_byte_chain() -> None:
    """Ghidra often materializes undefined1 units; they must not block apply."""

    undef = SimpleNamespace(
        getLength=lambda: 1,
        getDataType=lambda: SimpleNamespace(getName=lambda: "undefined1"),
    )
    program = _program(data_at={offset: undef for offset in range(0x1000, 0x1010)})
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_allows_typed_start_expanding_over_undefined_chain() -> None:
    typed = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "dword")
    undef = SimpleNamespace(
        getLength=lambda: 1,
        getDataType=lambda: SimpleNamespace(getName=lambda: "undefined"),
    )
    data_at = {0x1000: typed}
    data_at.update({offset: undef for offset in range(0x1004, 0x1010)})
    program = _program(data_at=data_at)
    assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))


def test_clear_range_refuses_incomplete_cover_when_tail_blocked() -> None:
    data = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "Foo")
    blocker = SimpleNamespace(getLength=lambda: 4, getDataType=lambda: "Bar")
    program = _program(data_at={0x1000: data, 0x1008: blocker})
    with pytest.raises(ClearRangeError) as exc:
        assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x100F))
    assert exc.value.payload["error"] == "clear-range-incomplete-cover"


def test_clear_range_refuses_function_overlap() -> None:
    fn = SimpleNamespace(getName=lambda _q=True: "Victim", body=(0x1000, 0x1010))
    program = _program(functions={0x1000: fn})
    with pytest.raises(ClearRangeError) as exc:
        assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x1003))
    assert exc.value.payload["error"] == "clear-range-function-overlap"


def test_clear_range_requires_expected_name_when_provided() -> None:
    program = _program(primary_name="DAT_001000")
    with pytest.raises(ClearRangeError) as exc:
        assert_clear_range_safe(program, _Addr(0x1000), _Addr(0x1003), expected_name="g_known")
    assert exc.value.payload["error"] == "clear-range-name-mismatch"

    program_ok = _program(primary_name="g_known")
    assert_clear_range_safe(program_ok, _Addr(0x1000), _Addr(0x1003), expected_name="g_known")
