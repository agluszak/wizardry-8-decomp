"""Tests for thunk body classification helpers."""

from __future__ import annotations

from wiz8decomp import function_attributes
from wiz8decomp.function_attributes import _is_ecx_immediate_adjust


class _FakeScalar:
    """Stand-in for Ghidra ``Scalar`` in unit tests without a JVM."""


class _Instr:
    def __init__(
        self,
        mnemonic: str,
        dest: str = "ECX",
        *,
        op_objects: list[object] | None = None,
    ) -> None:
        self._mnemonic = mnemonic
        self._dest = dest
        self._op_objects = op_objects if op_objects is not None else [_FakeScalar()]

    def getMnemonicString(self) -> str:
        return self._mnemonic

    def getDefaultOperandRepresentation(self, index: int) -> str:
        assert index == 0
        return self._dest

    def getOpObjects(self, index: int) -> list[object]:
        assert index == 1
        return self._op_objects


def test_ecx_immediate_adjust_accepts_add_sub(monkeypatch) -> None:
    monkeypatch.setattr(
        function_attributes,
        "_is_scalar",
        lambda obj: isinstance(obj, _FakeScalar),
    )
    assert _is_ecx_immediate_adjust(_Instr("ADD")) is True
    assert _is_ecx_immediate_adjust(_Instr("SUB")) is True


def test_ecx_immediate_adjust_rejects_register_src(monkeypatch) -> None:
    class _Register:
        pass

    monkeypatch.setattr(
        function_attributes,
        "_is_scalar",
        lambda obj: isinstance(obj, _FakeScalar),
    )
    assert _is_ecx_immediate_adjust(_Instr("ADD", op_objects=[_Register()])) is False


def test_ecx_immediate_adjust_rejects_non_ecx_dest(monkeypatch) -> None:
    monkeypatch.setattr(
        function_attributes,
        "_is_scalar",
        lambda obj: isinstance(obj, _FakeScalar),
    )
    assert _is_ecx_immediate_adjust(_Instr("ADD", dest="EAX")) is False


def test_ecx_immediate_adjust_rejects_generic_object(monkeypatch) -> None:
    monkeypatch.setattr(
        function_attributes,
        "_is_scalar",
        lambda obj: isinstance(obj, _FakeScalar),
    )
    assert _is_ecx_immediate_adjust(_Instr("ADD", op_objects=[object()])) is False
