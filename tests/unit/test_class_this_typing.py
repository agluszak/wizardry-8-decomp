"""Tests for class-binding helpers used by automatic ``this`` typing.

Broader class-binding coverage lives in ``test_class_binding.py``.
"""

from __future__ import annotations

from types import SimpleNamespace
from typing import Any

from wiz8decomp.class_this_typing import (
    _storage_matches,
    apply_this_typing_row,
)


def test_storage_matches_compares_parameter_slots() -> None:
    before = {
        "return_storage": "EAX:4",
        "parameters": [{"ordinal": 0, "name": "this", "storage": "ECX:4"}],
    }
    after = {
        "return_storage": "EAX:4",
        "parameters": [{"ordinal": 0, "name": "this", "storage": "ECX:4"}],
    }
    assert _storage_matches(before, after)
    after["parameters"][0]["storage"] = "Stack[0x4]:4"
    assert not _storage_matches(before, after)


class _FakeParam:
    def __init__(self) -> None:
        self._storage = "ECX:4 (auto)"
        self._name = "this"
        self._dt = "W8Monster *"

    def getOrdinal(self) -> int:
        return 0

    def getName(self) -> str:
        return self._name

    def getVariableStorage(self):
        return self._storage

    def getDataType(self):
        return self._dt


class _FakeReturn:
    def getVariableStorage(self):
        return "EAX:4"


class _FakeFunction:
    def __init__(self, *, custom: bool = False, convention: str = "__thiscall") -> None:
        self._custom = custom
        self._convention = convention
        self._params = [_FakeParam()]
        self._cleared_custom = False
        self._set_convention: str | None = None
        self._parent = SimpleNamespace(getName=lambda _q=True: "W8Monster", equals=lambda _o: True)

    def hasCustomVariableStorage(self) -> bool:
        return self._custom

    def setCustomVariableStorage(self, value: bool) -> None:
        self._cleared_custom = value is False
        self._custom = value

    def getCallingConventionName(self) -> str:
        return self._convention

    def setCallingConvention(self, name: str) -> None:
        self._set_convention = name
        self._convention = name

    def getParameters(self):
        return list(self._params)

    def getReturn(self):
        return _FakeReturn()

    def getParentNamespace(self):
        return self._parent

    def setParentNamespace(self, ns: Any) -> None:
        self._parent = ns


def test_ordinary_path_skips_custom_storage_without_clearing(monkeypatch) -> None:
    fn = _FakeFunction(custom=True, convention="__thiscall")
    space = SimpleNamespace(getAddress=lambda _a: object())
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: space),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: fn),
        getDataTypeManager=lambda: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.ensure_ghidra_class",
        lambda *_a, **_k: (_ for _ in ()).throw(AssertionError("must not ensure")),
    )
    row = {
        "address": "0x00401000",
        "name": "W8Monster::method",
        "owning_class": "W8Monster",
        "action": "bind-class-this",
    }
    result = apply_this_typing_row(program, row, allow_custom_storage=False)
    assert result.get("error") == "skip-custom-storage"
    assert fn._cleared_custom is False
    assert fn.hasCustomVariableStorage() is True


def test_convention_hard_disagree_skips_without_override(monkeypatch) -> None:
    fn = _FakeFunction(custom=False, convention="__fastcall")
    space = SimpleNamespace(getAddress=lambda _a: object())
    structure = SimpleNamespace(getPathName=lambda: "/W8Monster")
    ghidra_class = SimpleNamespace(getName=lambda _q=True: "W8Monster")
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: space),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: fn),
        getDataTypeManager=lambda: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.ensure_ghidra_class",
        lambda *_a, **_k: ghidra_class,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.find_class_structure",
        lambda *_a, **_k: structure,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.ensure_function_class_namespace",
        lambda *_a, **_k: False,
    )
    row = {
        "address": "0x00401000",
        "name": "W8Monster::method",
        "owning_class": "W8Monster",
        "action": "bind-class-this",
    }
    result = apply_this_typing_row(program, row, allow_custom_storage=False)
    assert result.get("error") == "convention-hard-disagree"
    assert fn._set_convention is None
    assert fn.getCallingConventionName() == "__fastcall"


def test_stdcall_does_not_promote_to_thiscall(monkeypatch) -> None:
    fn = _FakeFunction(custom=False, convention="__stdcall")
    space = SimpleNamespace(getAddress=lambda _a: object())
    structure = SimpleNamespace(getPathName=lambda: "/W8Monster")
    ghidra_class = SimpleNamespace(getName=lambda _q=True: "W8Monster")
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: space),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: fn),
        getDataTypeManager=lambda: object(),
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.ensure_ghidra_class",
        lambda *_a, **_k: ghidra_class,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.find_class_structure",
        lambda *_a, **_k: structure,
    )
    monkeypatch.setattr(
        "wiz8decomp.class_this_typing.ensure_function_class_namespace",
        lambda *_a, **_k: False,
    )
    row = {
        "address": "0x00401000",
        "name": "W8Monster::method",
        "owning_class": "W8Monster",
        "action": "bind-class-this",
    }
    result = apply_this_typing_row(program, row, allow_custom_storage=False)
    assert result.get("error") == "convention-hard-disagree"
    assert fn._set_convention is None
    assert fn.getCallingConventionName() == "__stdcall"
