"""Tests for SurRender IAT convention planning."""

from __future__ import annotations

from pathlib import Path
from types import SimpleNamespace

from wiz8decomp.surrender_iat_typing import (
    _external_or_import_thunk,
    _function_for_iat,
    _imported_data_value_type,
    load_surrender_imports,
    normalize_msvc_type,
    parse_demangled_callable,
)

_REPO = Path(__file__).resolve().parents[2]


def test_surrender_import_csv_has_callable_conventions() -> None:
    rows = load_surrender_imports(_REPO)
    assert rows
    assert {"decorated_name", "calling_convention", "kind", "iat_address"} <= set(rows[0])
    assert any(
        row["kind"] == "free-function" and row["calling_convention"] == "__cdecl" for row in rows
    )
    assert any(
        row["kind"] == "method" and row["calling_convention"] == "__thiscall" for row in rows
    )


def test_parse_demangled_free_function_and_variadic() -> None:
    parsed = parse_demangled_callable(
        "void __cdecl srAssertFail(char const *, char const *, long, char const *, ...)"
    )
    assert parsed is not None
    assert parsed.convention == "__cdecl"
    assert parsed.return_type == "void"
    assert parsed.parameters == ("char const *", "char const *", "long", "char const *")
    assert parsed.varargs is True
    assert parsed.has_function_pointer_param is False


def test_parse_demangled_constructor_and_method() -> None:
    ctor = parse_demangled_callable(
        "public: __thiscall srBinIMStream::srBinIMStream(void const *, unsigned long)"
    )
    assert ctor is not None
    assert ctor.convention == "__thiscall"
    assert ctor.return_type == "void"
    assert ctor.parameters == ("void const *", "unsigned long")

    method = parse_demangled_callable(
        "protected: virtual unsigned short __thiscall srBinIStream::vget(void)"
    )
    assert method is not None
    assert method.return_type == "unsigned short"
    assert method.parameters == ()

    assign = parse_demangled_callable(
        "public: class srCamera & __thiscall srCamera::operator=(class srCamera const &)"
    )
    assert assign is not None
    assert assign.return_type == "class srCamera &"
    assert assign.parameters == ("class srCamera const &",)


def test_parse_demangled_function_pointer_param() -> None:
    parsed = parse_demangled_callable(
        "void __cdecl srAssertSetFunc(void (__cdecl *)(char const *, char const *, long, char const *))"
    )
    assert parsed is not None
    assert parsed.has_function_pointer_param is True
    assert len(parsed.parameters) == 1


def test_normalize_msvc_type_drops_class_and_const() -> None:
    assert normalize_msvc_type("char const *") == "char *"
    assert normalize_msvc_type("class srCamera const &") == "srCamera *"
    assert normalize_msvc_type("void const *") == "void *"


def test_imported_data_value_type_for_object_and_static_pointer() -> None:
    assert _imported_data_value_type("class srCore srCore") == "srCore"
    assert (
        _imported_data_value_type(
            "protected: static class srTriMeshPipeline * srTriMeshPipeline::pipe"
        )
        == "srTriMeshPipeline *"
    )


def test_function_for_iat_ignores_ordinary_caller() -> None:
    ordinary = SimpleNamespace(isExternal=lambda: False, isThunk=lambda: False)
    ref = SimpleNamespace(getFromAddress=lambda: 0x401000)

    class _Refs:
        def __init__(self) -> None:
            self._items = [ref]

        def hasNext(self) -> bool:
            return bool(self._items)

        def next(self):
            return self._items.pop(0)

    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(
            getDefaultAddressSpace=lambda: SimpleNamespace(getAddress=lambda value: value)
        ),
        getFunctionManager=lambda: SimpleNamespace(
            getFunctionAt=lambda _addr: None,
            getFunctionContaining=lambda _addr: ordinary,
        ),
        getReferenceManager=lambda: SimpleNamespace(getReferencesTo=lambda _addr: _Refs()),
    )
    assert _function_for_iat(program, 0x5EBAF4) is None
    assert _external_or_import_thunk(ordinary) is None


def test_function_for_iat_accepts_import_thunk() -> None:
    external = SimpleNamespace(isExternal=lambda: True, isThunk=lambda: False)
    thunk = SimpleNamespace(
        isExternal=lambda: False,
        isThunk=lambda: True,
        getThunkedFunction=lambda _deep=True: external,
    )
    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(
            getDefaultAddressSpace=lambda: SimpleNamespace(getAddress=lambda value: value)
        ),
        getFunctionManager=lambda: SimpleNamespace(
            getFunctionAt=lambda _addr: thunk,
            getFunctionContaining=lambda _addr: None,
        ),
        getReferenceManager=lambda: SimpleNamespace(
            getReferencesTo=lambda _addr: SimpleNamespace(hasNext=lambda: False, next=lambda: None)
        ),
    )
    assert _function_for_iat(program, 0x5EBAF4) is external


def test_signature_source_calls_java_enum_name() -> None:
    from wiz8decomp.surrender_iat_typing import _signature_source

    function = SimpleNamespace(getSignatureSource=lambda: SimpleNamespace(name=lambda: "ANALYSIS"))
    assert _signature_source(function) == "ANALYSIS"


def test_convention_only_apply_uses_analysis_not_imported(monkeypatch) -> None:
    import sys
    import types

    from wiz8decomp.surrender_iat_typing import _apply_surrender_iat_row

    sources: list[object] = []
    function = SimpleNamespace(
        setCallingConvention=lambda _cc: None,
        setSignatureSource=lambda source: sources.append(source),
        getCallingConventionName=lambda: "default",
    )
    listing = types.ModuleType("ghidra.program.model.listing")
    listing.Function = object
    listing.ParameterImpl = object
    symbol = types.ModuleType("ghidra.program.model.symbol")
    symbol.SourceType = SimpleNamespace(ANALYSIS="ANALYSIS", IMPORTED="IMPORTED")
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    monkeypatch.setitem(sys.modules, "ghidra.program.model.listing", listing)
    monkeypatch.setitem(sys.modules, "ghidra.program.model.symbol", symbol)
    monkeypatch.setattr(
        "wiz8decomp.surrender_iat_typing._function_for_iat", lambda *_a, **_k: function
    )
    monkeypatch.setattr(
        "wiz8decomp.surrender_iat_typing._callable_iat_pointer", lambda *_a, **_k: None
    )
    monkeypatch.setattr("wiz8decomp.surrender_iat_typing._signature_source", lambda _f: "DEFAULT")
    result = _apply_surrender_iat_row(
        object(),
        {
            "action": "set-from-surrender",
            "address": "0x005ebaf4",
            "convention": "__cdecl",
            "apply_types": False,
            "iat_cell": "function-pointer",
            "decorated_name": "?srExit@@YAHXZ",
        },
    )
    assert result["applied_types"] is False
    assert sources == ["ANALYSIS"]
    assert "IMPORTED" not in sources


def test_iat_cell_type_failure_is_an_apply_error(monkeypatch) -> None:
    from wiz8decomp.surrender_iat_typing import _apply_surrender_iat_row

    monkeypatch.setattr(
        "wiz8decomp.surrender_iat_typing._data_iat_pointer", lambda *_a, **_k: object()
    )
    monkeypatch.setattr(
        "wiz8decomp.surrender_iat_typing._apply_iat_cell_type", lambda *_a, **_k: False
    )
    result = _apply_surrender_iat_row(
        object(),
        {
            "action": "set-iat-cell",
            "address": "0x005eb02c",
            "kind": "data",
            "iat_cell": "data-pointer",
            "decorated_name": "?g_sr@@3PAVsrCore@@A",
        },
    )
    assert result["error"] == "iat-cell-not-typed"
