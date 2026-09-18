"""Tests for source-backed calling-convention classification."""

from __future__ import annotations

import sys
import types
from contextlib import nullcontext
from types import SimpleNamespace

from wiz8decomp.prototype_repair import apply_source_conventions, classify_pair


def test_classify_pair_agrees_on_explicit_match() -> None:
    assert classify_pair("__thiscall", "__thiscall") == "agree"
    assert classify_pair("__stdcall", "__stdcall") == "agree"


def test_classify_pair_promotes_default_cdecl() -> None:
    assert classify_pair("default", "__cdecl") == "promote-default-cdecl"
    assert classify_pair("unknown", "__cdecl") == "promote-default-cdecl"


def test_classify_pair_sets_thiscall_from_unknown() -> None:
    assert classify_pair("unknown", "__thiscall") == "set-from-source"
    assert classify_pair("default", "__fastcall") == "set-from-source"


def test_classify_pair_hard_disagree() -> None:
    assert classify_pair("__cdecl", "__thiscall") == "hard-disagree"
    assert classify_pair("__stdcall", "__cdecl") == "hard-disagree"
    assert classify_pair("__stdcall", "__thiscall") == "hard-disagree"


def test_apply_source_conventions_uses_analysis_not_imported(monkeypatch) -> None:
    sources: list[object] = []

    function = SimpleNamespace(
        setCallingConvention=lambda _cc: None,
        setSignatureSource=lambda source: sources.append(source),
    )

    class _Space:
        def getAddress(self, value: int):
            return SimpleNamespace(value=value)

    program = SimpleNamespace(
        getAddressFactory=lambda: SimpleNamespace(getDefaultAddressSpace=lambda: _Space()),
        getFunctionManager=lambda: SimpleNamespace(getFunctionAt=lambda _a: function),
    )

    symbol_mod = types.ModuleType("ghidra.program.model.symbol")
    symbol_mod.SourceType = SimpleNamespace(ANALYSIS="ANALYSIS", IMPORTED="IMPORTED")
    monkeypatch.setitem(sys.modules, "ghidra", types.ModuleType("ghidra"))
    monkeypatch.setitem(sys.modules, "ghidra.program", types.ModuleType("ghidra.program"))
    monkeypatch.setitem(
        sys.modules, "ghidra.program.model", types.ModuleType("ghidra.program.model")
    )
    monkeypatch.setitem(sys.modules, "ghidra.program.model.symbol", symbol_mod)
    monkeypatch.setattr(
        "wiz8decomp.ghidra.mutations.program_transaction",
        lambda _program, _description: nullcontext(),
    )

    plan = {
        "functions": [
            {
                "address": "0x00401000",
                "name": "foo",
                "ghidra": "default",
                "source": "__cdecl",
                "action": "promote-default-cdecl",
            },
            {
                "address": "0x00402000",
                "name": "bar",
                "ghidra": "unknown",
                "source": "__thiscall",
                "action": "set-from-source",
            },
        ]
    }
    result = apply_source_conventions(program, plan)
    assert result["applied"] == 2
    assert sources == ["ANALYSIS", "ANALYSIS"]
    assert "IMPORTED" not in sources
