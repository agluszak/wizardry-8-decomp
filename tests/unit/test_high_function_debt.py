"""Tests for HighFunction residual scoring."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.high_function_debt import METRIC_KEYS, score_high_function


class _Op:
    def __init__(self, mnemonic: str) -> None:
        self._mnemonic = mnemonic

    def getMnemonic(self) -> str:
        return self._mnemonic


class _Ops:
    def __init__(self, ops: list[_Op]) -> None:
        self._ops = list(ops)

    def hasNext(self) -> bool:
        return bool(self._ops)

    def next(self) -> _Op:
        return self._ops.pop(0)


def test_score_high_function_counts_undefined_this_and_indirect_calls() -> None:
    this = SimpleNamespace(
        getName=lambda: "this",
        getDataType=lambda: SimpleNamespace(getDisplayName=lambda: "undefined4"),
    )
    other = SimpleNamespace(
        getName=lambda: "count", getDataType=lambda: SimpleNamespace(getDisplayName=lambda: "int")
    )
    prototype = SimpleNamespace(
        getModelName=lambda: "unknown",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "void *"),
        getNumParams=lambda: 2,
        getParam=lambda index: this if index == 0 else other,
    )

    class _Callind(_Op):
        def getInput(self, _index: int):
            return SimpleNamespace(
                getHigh=lambda: SimpleNamespace(
                    getDataType=lambda: SimpleNamespace(getDisplayName=lambda: "undefined4")
                )
            )

    class _Locals:
        def __init__(self) -> None:
            self._items = [
                SimpleNamespace(getName=lambda: "unaff_ESI"),
                SimpleNamespace(getName=lambda: "count"),
            ]

        def hasNext(self) -> bool:
            return bool(self._items)

        def next(self):
            return self._items.pop(0)

    high = SimpleNamespace(
        getFunctionPrototype=lambda: prototype,
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: _Locals()),
        getPcodeOps=lambda: _Ops([_Op("CAST"), _Callind("CALLIND"), _Op("PTRADD"), _Op("CALL")]),
    )
    counts = score_high_function(high)
    assert counts["undefined_this"] == 1
    assert counts["undefined_params"] == 1
    assert counts["untyped_return"] == 1
    assert counts["default_convention"] == 1
    assert counts["cast_ops"] == 1
    assert counts["callind_ops"] == 1
    assert counts["untyped_callind"] == 1
    assert counts["suspicious_ptr_ops"] == 1
    assert counts["unaff_vars"] == 1
    assert set(counts) == set(METRIC_KEYS)


def test_score_high_function_treats_typed_ptrsub_as_healthy() -> None:
    class _Struct:
        def getDefinedComponents(self):
            return [SimpleNamespace(getOffset=lambda: 4)]

        def getDisplayName(self):
            return "W8Foo"

    class _PointerType:
        def getDisplayName(self):
            return "W8Foo *"

        def getDataType(self):
            return _Struct()

    class _PtrSub(_Op):
        def getInput(self, index: int):
            if index == 0:
                return SimpleNamespace(
                    getHigh=lambda: SimpleNamespace(getDataType=lambda: _PointerType()),
                    getDataType=lambda: _PointerType(),
                )
            return SimpleNamespace(
                isConstant=lambda: True, getOffset=lambda: 4, getHigh=lambda: None
            )

    prototype = SimpleNamespace(
        getModelName=lambda: "__thiscall",
        getReturnType=lambda: SimpleNamespace(getDisplayName=lambda: "void"),
        getNumParams=lambda: 0,
        getParam=lambda _index: None,
    )
    high = SimpleNamespace(
        getFunctionPrototype=lambda: prototype,
        getLocalSymbolMap=lambda: SimpleNamespace(getSymbols=lambda: _Ops([])),
        getPcodeOps=lambda: _Ops([_PtrSub("PTRSUB")]),
    )
    counts = score_high_function(high)
    assert counts["suspicious_ptr_ops"] == 0


def test_debt_total_weights_undefined_and_untyped_callind() -> None:
    from wiz8decomp.high_function_debt import _HIGH_DEBT, debt_total

    counts = {key: 1 for key in METRIC_KEYS}
    expected = sum(4 if key in _HIGH_DEBT else 1 for key in METRIC_KEYS)
    assert debt_total(counts) == expected
    assert debt_total(counts) != sum(counts.values())
