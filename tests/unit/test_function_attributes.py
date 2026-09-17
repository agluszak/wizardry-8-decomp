"""Tests for thunk body classification and noreturn apply gating."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp import function_attributes
from wiz8decomp.function_attributes import (
    _is_ecx_immediate_adjust,
    apply_function_attributes,
)


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


def test_apply_skips_report_noreturn_leaf_candidates() -> None:
    class _Fn:
        def __init__(self) -> None:
            self.noreturn = False

        def setNoReturn(self, value: bool) -> None:
            self.noreturn = value

        def setVarArgs(self, value: bool) -> None:
            raise AssertionError("unexpected varargs")

    fn = _Fn()

    class _Space:
        def getAddress(self, _value: int):
            return object()

    class _Functions:
        def getFunctionAt(self, _addr):
            return fn

    program = type(
        "P",
        (),
        {
            "getAddressFactory": lambda self: type(
                "A", (), {"getDefaultAddressSpace": lambda s: _Space()}
            )(),
            "getFunctionManager": lambda self: _Functions(),
        },
    )()
    plan = {
        "functions": [
            {
                "address": "0x00401000",
                "name": "srAssertFail",
                "action": "report-noreturn",
            }
        ]
    }
    result = apply_function_attributes(program, plan, apply_attributes=True)
    assert result["applied"] == 0
    assert fn.noreturn is False


def test_apply_still_sets_source_backed_noreturn() -> None:
    class _Fn:
        def __init__(self) -> None:
            self.noreturn = False

        def setNoReturn(self, value: bool) -> None:
            self.noreturn = value

        def setVarArgs(self, _value: bool) -> None:
            return None

    fn = _Fn()

    class _Space:
        def getAddress(self, _value: int):
            return object()

    class _Functions:
        def getFunctionAt(self, _addr):
            return fn

    program = type(
        "P",
        (),
        {
            "getAddressFactory": lambda self: type(
                "A", (), {"getDefaultAddressSpace": lambda s: _Space()}
            )(),
            "getFunctionManager": lambda self: _Functions(),
        },
    )()
    plan = {
        "functions": [
            {
                "address": "0x00401000",
                "name": "FatalError",
                "action": "set-noreturn",
            }
        ]
    }
    result = apply_function_attributes(program, plan, apply_attributes=True)
    assert result["applied"] == 1
    assert fn.noreturn is True


def test_second_discovery_loop_respects_address_filter(monkeypatch) -> None:
    class _Entry:
        def getOffset(self) -> int:
            return 0x00402000

    class _Fn:
        def isExternal(self) -> bool:
            return False

        def getName(self) -> str:
            return "FatalError"

        def getEntryPoint(self):
            return _Entry()

        def hasNoReturn(self) -> bool:
            return False

        def isThunk(self) -> bool:
            return False

        def getBody(self):
            return SimpleNamespace(getNumAddresses=lambda: 100)

    class _Functions:
        def getFunctionAt(self, _addr):
            return None

        def getFunctions(self, _forward: bool):
            return [_Fn()]

    monkeypatch.setattr(
        "wiz8decomp.function_attributes.source_functions",
        lambda *_a, **_k: {},
    )
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(
            getDefaultAddressSpace=lambda: SimpleNamespace(getAddress=lambda _v: object())
        ),
        getFunctionManager=lambda: _Functions(),
    )
    # Address filter excludes 0x00402000 → second loop must not report noreturn.
    plan = function_attributes.collect_function_attribute_plan(
        object(),  # type: ignore[arg-type]
        program,
        addresses=[0x00401000],
    )
    assert all(row.get("action") != "report-noreturn" for row in plan["functions"])
