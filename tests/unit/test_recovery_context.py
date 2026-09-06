from __future__ import annotations

import contextlib
from types import SimpleNamespace

from wiz8decomp.ghidra.unit_intervals import TranslationUnitResolver
from wiz8decomp.reports import recovery_context
from wiz8decomp.reports.recovery_context import _assertion_boundary_defects


def test_direct_assertion_ownership_wins_over_an_interval() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\ItemManager.cpp",
            "containing_function": "004f88f0",
        }
    ]
    assert TranslationUnitResolver(assertions).resolve(0x004F88F0)["source_path"] == (
        r"Local Code\ItemManager.cpp"
    )


def test_invalid_assertion_function_boundary_is_structured() -> None:
    assertions = [{"containing_function": "004a42b0", "call_site": "004a42ce"}]
    assert _assertion_boundary_defects(assertions, {0x004A42B0: None, 0x004A42CE: 0x004A42C0}) == [
        {
            "kind": "invalid-assertion-function-boundary",
            "containing_function": "0x004a42b0",
            "call_site": "0x004a42ce",
            "anchor_owner": None,
            "call_site_owner": "0x004a42c0",
        }
    ]


def test_context_batch_uses_one_session_and_never_compares(tmp_path, monkeypatch) -> None:
    evidence = tmp_path / "evidence/observations/wiz8"
    evidence.mkdir(parents=True)
    (evidence / "assertions.csv").write_text(
        "containing_function,call_site,source_path,line,expression,message\n",
        encoding="utf-8",
    )
    (tmp_path / "build").mkdir()
    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    addresses = [0x401000, 0x401020]
    monkeypatch.setattr(recovery_context, "resolve_function_selectors", lambda *_args: addresses)
    monkeypatch.setattr(recovery_context, "resolve_seed_program", lambda *_args: "wiz8")
    monkeypatch.setattr(recovery_context, "source_functions", lambda *_args: {})
    monkeypatch.setattr(recovery_context, "load_source_index", lambda *_args: {"classes": []})
    calls = 0

    def fake_query_many(_program, queries, **_kwargs):
        nonlocal calls
        calls += 1
        results = []
        for command, arguments in queries:
            if command == "function-of":
                result = {"functions": {f"0x{value:08x}": f"0x{value:08x}" for value in addresses}}
            elif command == "function":
                value = int(arguments[0], 16)
                result = {
                    "function": {
                        "entry": arguments[0],
                        "name": f"Function{value:X}",
                        "prototype": "void f()",
                        "calling_convention": "__cdecl",
                        "calls": [],
                        "caller_functions": [],
                        "data_references": [],
                        "vptr_references": [],
                        "exception_metadata": [],
                    }
                }
            elif command == "decompile":
                result = {"decompiled": "void f() {}"}
            elif command == "indirect-calls":
                result = {"calls": []}
            else:
                raise AssertionError(command)
            results.append({"command": command, "arguments": arguments, "result": result})
        return results

    monkeypatch.setattr(recovery_context, "query_many", fake_query_many)
    monkeypatch.setattr(
        recovery_context, "open_program", lambda *_args, **_kwargs: contextlib.nullcontext(object())
    )
    contexts = recovery_context.recovery_context_reports(settings, ["a", "b"])

    assert calls == 1
    assert [context["entry"] for context in contexts] == [
        "0x00401000",
        "0x00401020",
    ]
    assert all("match" not in context for context in contexts)
    assert all(
        set(context) >= {"identity", "source", "retail", "dependencies"} for context in contexts
    )
    assert all("disagree" not in context["signature_evidence"] for context in contexts)
