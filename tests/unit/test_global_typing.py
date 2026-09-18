"""Tests for GLOBAL typing string helpers."""

from __future__ import annotations

import sys
import types
from types import SimpleNamespace

from wiz8decomp.global_typing import (
    _POINTER_SUFFIX,
    _ghidra_type_name,
    _named_data_type,
    _needs_type_update,
    _pointer_depth,
    _strip_qualifiers,
    _types_equivalent,
)


def test_strip_and_template_mapping() -> None:
    assert _strip_qualifiers("const float") == "float"
    assert _strip_qualifiers("static const float") == "float"
    assert _ghidra_type_name("srVector3T<float>") == "srVector3T[float]"
    assert _ghidra_type_name("const W8Foo *") == "W8Foo *"


def test_named_data_type_prefers_root_class_structure() -> None:
    hits: list[str] = []

    class Manager:
        def getDataType(self, path: str):
            hits.append(path)
            if path == "/W8Monster":
                return SimpleNamespace(name="W8Monster", path=path, getPathName=lambda: path)
            if path == "/wiz8/classes/W8Monster":
                return SimpleNamespace(name="W8Monster", path=path, getPathName=lambda: path)
            return None

    class Symbols:
        def getNamespace(self, _name: str, _parent: object):
            return None

    program = SimpleNamespace(
        getDataTypeManager=lambda: Manager(),
        getSymbolTable=lambda: Symbols(),
        getGlobalNamespace=lambda: object(),
    )
    resolved = _named_data_type(program, "W8Monster")
    assert resolved is not None
    assert resolved.path == "/W8Monster"
    assert "/W8Monster" in hits


def test_named_data_type_search_order(monkeypatch) -> None:
    hits: list[str] = []

    class Manager:
        def getDataType(self, path: str):
            hits.append(path)

    class Symbols:
        def getNamespace(self, _name: str, _parent: object):
            return None

    monkeypatch.setattr(
        "wiz8decomp.global_typing._builtin_data_type",
        lambda _program, _name: None,
    )
    program = SimpleNamespace(
        getDataTypeManager=lambda: Manager(),
        getSymbolTable=lambda: Symbols(),
        getGlobalNamespace=lambda: object(),
    )
    assert _named_data_type(program, "ns::W8Foo") is None
    assert "/ns::W8Foo" in hits
    assert "/W8Foo" in hits
    assert hits.index("/ns::W8Foo") < hits.index("/W8Foo")
    assert "/wiz8/classes/W8Foo" not in hits


def test_needs_type_update() -> None:
    assert _needs_type_update(None, "int")
    assert _needs_type_update("undefined4", "int")
    assert _needs_type_update("void *", "W8Monster *")
    assert not _needs_type_update("int", "int")
    assert not _needs_type_update("uchar", "BOOLEAN")
    assert not _needs_type_update("uint", "UINT32")
    assert not _needs_type_update("uchar[256]", "BOOLEAN[256]")
    # C++ bool is not interchangeable with SGP BOOLEAN / uchar.
    assert _needs_type_update("bool", "BOOLEAN")
    assert _needs_type_update("uchar", "bool")


def test_needs_type_update_prefers_canonical_class_path() -> None:
    # Bound/root listing must not be overwritten by a leftover /wiz8/classes copy.
    assert not _needs_type_update(
        "W8Monster",
        "W8Monster",
        current_path="/W8Monster",
        resolved_path="/wiz8/classes/W8Monster",
        current_depth=0,
        resolved_depth=0,
    )
    # Legacy listing must update to the bound/root Structure.
    assert _needs_type_update(
        "W8Monster",
        "W8Monster",
        current_path="/wiz8/classes/W8Monster",
        resolved_path="/W8Monster",
        current_depth=0,
        resolved_depth=0,
    )
    assert not _needs_type_update(
        "W8Monster *",
        "W8Monster *",
        current_path="/wiz8/classes/W8Monster *",
        resolved_path="/wiz8/classes/W8Monster *",
        current_depth=1,
        resolved_depth=1,
    )


def test_bool_is_not_equivalent_to_uchar() -> None:
    assert not _types_equivalent("bool", "uchar")
    assert not _types_equivalent("bool", "BOOLEAN")
    assert _needs_type_update("uchar", "bool")


def test_pointer_suffix_counts_each_trailing_star() -> None:
    for spelling, base, stars in (
        ("wchar_t**", "wchar_t", 2),
        ("char**", "char", 2),
        ("W8ItemTableRecord**", "W8ItemTableRecord", 2),
        ("W8Foo *", "W8Foo", 1),
        ("int*", "int", 1),
    ):
        match = _POINTER_SUFFIX.match(spelling)
        assert match is not None, spelling
        assert match.group("base") == base
        assert len(match.group("stars")) == stars


def test_resolve_data_type_wraps_each_trailing_star(monkeypatch) -> None:
    from wiz8decomp import global_typing as gt

    class FakePointer:
        def __init__(self, pointee, _manager=None):
            self._pointee = pointee

        def getDataType(self):
            return self._pointee

        def isPointer(self) -> bool:
            return True

        def getName(self) -> str:
            return f"{self._pointee.getName()} *"

        def getPathName(self) -> str:
            return f"{self._pointee.getPathName()} *"

    class FakeBase:
        def __init__(self, name: str, path: str):
            self._name = name
            self._path = path

        def getName(self) -> str:
            return self._name

        def getPathName(self) -> str:
            return self._path

        def isPointer(self) -> bool:
            return False

    base = FakeBase("char", "/char")
    monkeypatch.setattr(
        gt, "_named_data_type", lambda _program, name: base if name == "char" else None
    )
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    data_mod = types.ModuleType("ghidra.program.model.data")
    data_mod.ArrayDataType = object
    data_mod.PointerDataType = FakePointer
    monkeypatch.setitem(sys.modules, "ghidra.program.model.data", data_mod)
    program = SimpleNamespace(getDataTypeManager=lambda: None)
    for spelling in ("char**", "wchar_t**", "char * *"):
        # wchar_t needs builtin — only test char** here for FakeBase.
        if "wchar" in spelling:
            continue
        resolved = gt.resolve_data_type(program, spelling)
        assert resolved is not None
        assert _pointer_depth(resolved) == 2, spelling


def test_resolve_template_spelling_does_not_become_array(monkeypatch) -> None:
    from wiz8decomp import global_typing as gt

    class FakeBase:
        def getName(self) -> str:
            return "srVector3T[float]"

        def getPathName(self) -> str:
            return "/srVector3T[float]"

        def getLength(self) -> int:
            return 12

    monkeypatch.setattr(
        gt,
        "_named_data_type",
        lambda _program, name: FakeBase() if name == "srVector3T[float]" else None,
    )
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    data_mod = types.ModuleType("ghidra.program.model.data")
    data_mod.ArrayDataType = object
    data_mod.PointerDataType = object
    monkeypatch.setitem(sys.modules, "ghidra.program.model.data", data_mod)
    program = SimpleNamespace(getDataTypeManager=lambda: None)
    resolved = gt.resolve_data_type(program, "srVector3T<float>")
    assert resolved is not None
    assert resolved.getName() == "srVector3T[float]"
