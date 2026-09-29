"""Tests for GLOBAL typing string helpers."""

from __future__ import annotations

import json
import sys
import types
from hashlib import sha256
from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.global_model import parse_global_definitions
from wiz8decomp.global_typing import (
    _POINTER_SUFFIX,
    _compiler_sized_arrays,
    _ghidra_type_name,
    _literal_sized_character_arrays,
    _named_data_type,
    _needs_type_update,
    _pointer_depth,
    _qualified_parts,
    _strip_qualifiers,
    _types_equivalent,
    resolve_data_type,
)


def test_compiler_sized_arrays_require_matching_source(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/globals.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        '// GLOBAL: WIZ8 0x00650000\nchar g_narrow[] = "ab";\n'
        '// GLOBAL: WIZ8 0x00650010\nwchar_t g_wide[] = L"xy";\n'
        "// GLOBAL: WIZ8 0x00650020\nint g_symbolic[COUNT] = {0};\n",
        encoding="utf-8",
    )
    header = tmp_path / "include/count.h"
    header.parent.mkdir(parents=True)
    header.write_text("#define COUNT 7\n", encoding="utf-8")
    index_path = tmp_path / "build/source-index.json"
    index_path.parent.mkdir()
    index_path.write_text(
        json.dumps(
            {
                "variables": [
                    {
                        "target": "WIZ8",
                        "source_file": "src/wiz8/globals.cpp",
                        "line": 2,
                        "qualified_name": "g_narrow",
                        "type": "char[3]",
                    },
                    {
                        "target": "WIZ8",
                        "source_file": "src/wiz8/globals.cpp",
                        "line": 4,
                        "qualified_name": "g_wide",
                        "type": "unsigned short[3]",
                    },
                    {
                        "target": "WIZ8",
                        "source_file": "src/wiz8/globals.cpp",
                        "line": 6,
                        "qualified_name": "g_symbolic",
                        "type": "int[7]",
                    },
                ],
                "source_digests": {
                    "src/wiz8/globals.cpp": sha256(source.read_bytes()).hexdigest(),
                    "include/count.h": sha256(header.read_bytes()).hexdigest(),
                },
                "unit_dependencies": {
                    "src/wiz8/globals.cpp": ["src/wiz8/globals.cpp", "include/count.h"]
                },
            }
        ),
        encoding="utf-8",
    )
    definitions = parse_global_definitions(tmp_path)

    assert _literal_sized_character_arrays(tmp_path, definitions) == {
        0x650000: ("char[3]", 3),
        0x650010: ("wchar_t[3]", 6),
    }
    assert _compiler_sized_arrays(tmp_path, definitions) == {
        0x650000: ("char[3]", 3),
        0x650010: ("wchar_t[3]", 6),
        0x650020: ("int[7]", None),
    }
    indexed = json.loads(index_path.read_text(encoding="utf-8"))
    del indexed["source_digests"]["include/count.h"]
    index_path.write_text(json.dumps(indexed), encoding="utf-8")
    assert _compiler_sized_arrays(tmp_path, definitions) == {}
    indexed["source_digests"]["include/count.h"] = sha256(header.read_bytes()).hexdigest()
    index_path.write_text(json.dumps(indexed), encoding="utf-8")
    header.write_text("#define COUNT 8\n", encoding="utf-8")
    assert _compiler_sized_arrays(tmp_path, definitions) == {}
    assert _literal_sized_character_arrays(tmp_path, definitions) == {
        0x650000: ("char[3]", 3),
        0x650010: ("wchar_t[3]", 6),
    }
    header.write_text("#define COUNT 7\n", encoding="utf-8")
    source.write_text(source.read_text() + "\n", encoding="utf-8")
    assert _compiler_sized_arrays(tmp_path, definitions) == {}
    assert _literal_sized_character_arrays(tmp_path, definitions) == {
        0x650000: ("char[3]", 3),
        0x650010: ("wchar_t[3]", 6),
    }


def test_literal_array_sizing_counts_escapes_and_rejects_hex(tmp_path: Path) -> None:
    source = tmp_path / "src/wiz8/strings.cpp"
    source.parent.mkdir(parents=True)
    source.write_text(
        '// GLOBAL: WIZ8 0x00650000\nchar g_path[] = "a\\\\b";\n'
        '// GLOBAL: WIZ8 0x00650010\nchar g_hex[] = "\\x41";\n'
        "// GLOBAL: WIZ8 0x00650020\nwchar_t g_mark[] = {0xfff4, 0};\n"
        '// GLOBAL: WIZ8 0x00650030\nchar g_next_line[] =\n    "abc";\n',
        encoding="utf-8",
    )
    assert _literal_sized_character_arrays(tmp_path, parse_global_definitions(tmp_path)) == {
        0x650000: ("char[4]", 4),
        0x650020: ("wchar_t[2]", 4),
        0x650030: ("char[4]", 4),
    }


def test_strip_and_template_mapping() -> None:
    assert _strip_qualifiers("const float") == "float"
    assert _strip_qualifiers("static const float") == "float"
    assert _strip_qualifiers("struct W8Character *") == "W8Character *"
    assert _strip_qualifiers("class W8ItemInstance *") == "W8ItemInstance *"
    assert _ghidra_type_name("srVector3T<float>") == "srVector3T[float]"
    assert _ghidra_type_name("const W8Foo *") == "W8Foo *"
    assert _ghidra_type_name("struct W8Character *") == "W8Character *"
    assert (
        _ghidra_type_name("W8GrowableVector<class W8Character *>")
        == "W8GrowableVector[W8Character *]"
    )
    assert _qualified_parts("srGERD::Renderer::Parameters") == [
        "srGERD",
        "Renderer",
        "Parameters",
    ]
    assert _qualified_parts("W8GrowableVector[srClipPlane::ClientType*]") == [
        "W8GrowableVector[srClipPlane::ClientType*]"
    ]


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
    assert "/ns/W8Foo" in hits
    assert "/W8Foo" in hits
    assert hits.index("/ns/W8Foo") < hits.index("/ns::W8Foo")
    assert hits.index("/ns::W8Foo") < hits.index("/W8Foo")
    assert "/wiz8/classes/W8Foo" not in hits


def test_named_data_type_resolves_nested_namespace_category() -> None:
    class Manager:
        def getDataType(self, path: str):
            if path == "/srCamera/Rect":
                return SimpleNamespace(path=path, getPathName=lambda: path)
            return None

    program = SimpleNamespace(
        getDataTypeManager=lambda: Manager(),
        getSymbolTable=lambda: SimpleNamespace(getNamespace=lambda *_a: None),
        getGlobalNamespace=lambda: object(),
    )
    resolved = _named_data_type(program, "srCamera::Rect")
    assert resolved is not None
    assert resolved.path == "/srCamera/Rect"


def test_named_data_type_resolves_imported_pointer_template() -> None:
    imported = "/W8Vector[MGSKeyBinding_#]"

    class Manager:
        def getDataType(self, path: str):
            if path == imported:
                return SimpleNamespace(path=path, getPathName=lambda: path)
            return None

    program = SimpleNamespace(
        getDataTypeManager=lambda: Manager(),
        getSymbolTable=lambda: SimpleNamespace(getNamespace=lambda *_a: None),
        getGlobalNamespace=lambda: object(),
    )
    resolved = _named_data_type(program, "W8Vector[MGSKeyBinding*]")
    assert resolved is not None
    assert resolved.path == imported


def test_source_typedefs_resolve_to_existing_record_and_windows_types(monkeypatch) -> None:
    class Pointer:
        def __init__(self, pointee, _manager):
            self.pointee = pointee

        def getPathName(self) -> str:
            return self.pointee.getPathName() + " *"

    data_mod = types.ModuleType("ghidra.program.model.data")
    data_mod.ArrayDataType = object
    data_mod.PointerDataType = Pointer
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    monkeypatch.setitem(sys.modules, "ghidra.program.model.data", data_mod)

    class Manager:
        def getDataType(self, path: str):
            if path in {"/TAG_HVOBJECT", "/_MOUSE_REGION", "/WinDef.h/HWND"}:
                return SimpleNamespace(getPathName=lambda: path)
            return None

    program = SimpleNamespace(getDataTypeManager=lambda: Manager())
    assert resolve_data_type(program, "HVOBJECT").getPathName() == "/TAG_HVOBJECT *"
    assert resolve_data_type(program, "MOUSE_REGION").getPathName() == "/_MOUSE_REGION"
    assert resolve_data_type(program, "HWND").getPathName() == "/WinDef.h/HWND"


def test_needs_type_update() -> None:
    assert _needs_type_update(None, "int")
    assert _needs_type_update("undefined4", "int")
    assert _needs_type_update("void *", "W8Monster *")
    assert not _needs_type_update("int", "int")
    assert not _needs_type_update("uchar", "BOOLEAN")
    assert not _needs_type_update("uint", "UINT32")
    assert not _needs_type_update("char", "INT8")
    assert not _needs_type_update("short", "INT16")
    assert not _needs_type_update("int", "INT32")
    assert not _needs_type_update("float", "FLOAT")
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
    for spelling in ("char**", "wchar_t**", "char * *", "const char *const *", "char *volatile *"):
        # wchar_t needs builtin — only test char** here for FakeBase.
        if "wchar" in spelling:
            continue
        resolved = gt.resolve_data_type(program, spelling)
        assert resolved is not None
        assert _pointer_depth(resolved) == 2, spelling
    assert _pointer_depth(gt.resolve_data_type(program, "const char &")) == 1


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


def test_resolve_hex_array_extent_but_not_symbolic_extent(monkeypatch) -> None:
    from wiz8decomp import global_typing as gt

    class FakeBase:
        def getLength(self) -> int:
            return 1

    class FakeArray:
        def __init__(self, _base, count: int, element_length: int) -> None:
            self.count = count
            self.element_length = element_length

    monkeypatch.setattr(
        gt, "_named_data_type", lambda _program, name: FakeBase() if name == "char" else None
    )
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    data_mod = types.ModuleType("ghidra.program.model.data")
    data_mod.ArrayDataType = FakeArray
    data_mod.PointerDataType = object
    monkeypatch.setitem(sys.modules, "ghidra.program.model.data", data_mod)
    program = SimpleNamespace(getDataTypeManager=lambda: None)

    resolved = gt.resolve_data_type(program, "char[0x10]")
    assert (resolved.count, resolved.element_length) == (16, 1)
    spaced = gt.resolve_data_type(program, "char[ 16 ]")
    assert (spaced.count, spaced.element_length) == (16, 1)
    assert gt.resolve_data_type(program, "char[W8_COUNT]") is None
