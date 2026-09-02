from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp import reccmp_workflows, selectors
from wiz8decomp.ghidra.unit_intervals import TranslationUnitResolver
from wiz8decomp.reports import recovery_context
from wiz8decomp.reports.recovery_context import _assertion_boundary_defects, render_context


def test_direct_assertion_ownership_wins_over_an_interval() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\ItemManager.cpp",
            "containing_function": "004f88f0",
        }
    ]
    result = TranslationUnitResolver(assertions).resolve(0x004F88F0)

    assert result == {
        "source_path": r"Local Code\ItemManager.cpp",
        "attribution": "direct",
        "alternatives": [],
    }


def test_multiple_direct_units_are_reported_as_inlined_instead_of_guessed() -> None:
    assertions = [
        {
            "source_path": r"C:\Projects\Wizardry 8\Engine Code\A.cpp",
            "containing_function": "00401000",
        },
        {
            "source_path": r"C:\Projects\Wizardry 8\Local Code\B.cpp",
            "containing_function": "00401000",
        },
    ]
    result = TranslationUnitResolver(assertions).resolve(0x00401000)

    assert result["attribution"] == "inlined-or-conflicting"
    assert result["source_path"] == ""
    assert result["alternatives"] == [r"Engine Code\A.cpp", r"Local Code\B.cpp"]


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


def test_context_renders_primary_evidence_without_opening_artifacts() -> None:
    context = {
        "entry": 0x401000,
        "translation_unit": {"source_path": "src/wiz8/unit.cpp", "attribution": "source"},
        "reviewed": {"function": {"name": "Thing::Run"}, "class_names": ["Thing"]},
        "assertions": [],
        "eh": {"unwind": []},
        "globals": [],
        "polymorphism": {"vptr_writes": [], "tables": {}},
        "semantic": {"facts": {}, "field_accesses": {"accesses": []}, "indirect_calls": []},
        "calls": [{"site": "00401002", "target": "00402000", "name": "Callee"}],
        "match": {
            "functions": [
                {
                    "status": "mismatch",
                    "raw_matching": 0.75,
                    "difference": {"kind": "call_target"},
                }
            ]
        },
        "ghidra": {
            "function": {
                "name": "Thing::Run",
                "prototype": "void Thing::Run()",
                "size": 12,
                "callers": ["00400000"],
                "referenced_strings": ["hello"],
            },
            "decompiled": "void Thing::Run() {}",
            "listing": "",
        },
        "outputs": ["build/reports/recovery-context/00401000.json"],
    }

    output = render_context(context)
    assert "## Calls" in output
    assert "First divergence: call_target" in output
    assert "void Thing::Run() {}" in output
    assert output.rstrip().endswith("00401000.json")


def test_multiple_contexts_use_one_ghidra_session(tmp_path, monkeypatch) -> None:
    evidence = tmp_path / "evidence/observations/wiz8"
    evidence.mkdir(parents=True)
    (evidence / "assertions.csv").write_text(
        "containing_function,call_site,source_path,line,expression,message\n",
        encoding="utf-8",
    )
    (tmp_path / "build").mkdir()
    settings = SimpleNamespace(repo_dir=tmp_path, build_dir=tmp_path / "build")
    addresses = [0x401000, 0x401020]
    monkeypatch.setattr(selectors, "resolve_function_selectors", lambda *_args: addresses)
    monkeypatch.setattr(recovery_context, "resolve_seed_program", lambda *_args: "wiz8")
    monkeypatch.setattr(
        recovery_context,
        "build_source_model",
        lambda *_args: SimpleNamespace(functions={}),
    )
    monkeypatch.setattr(recovery_context, "load_source_index", lambda *_args: {"classes": []})
    monkeypatch.setattr(recovery_context, "load_claims", lambda *_args: ())
    monkeypatch.setattr(
        reccmp_workflows,
        "compare_selected",
        lambda _repo, _target, values: {
            "functions": [{"address": f"0x{value:08x}", "status": "exact"} for value in values]
        },
    )
    calls = 0

    def fake_query_many(_settings, _selector, queries, **_kwargs):
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
                        "size": 1,
                        "callers": [],
                        "referenced_strings": [],
                    }
                }
            elif command == "decompile":
                result = {"decompiled": "void f() {}"}
            elif command == "function-facts":
                result = {
                    "calls": [],
                    "data_references": [],
                    "vptr_references": [],
                    "exception_metadata": [],
                }
            elif command == "indirect-calls":
                result = {"calls": []}
            else:
                raise AssertionError(command)
            results.append({"command": command, "arguments": arguments, "result": result})
        return results, "pyghidra"

    monkeypatch.setattr(recovery_context, "query_many", fake_query_many)

    contexts = recovery_context.recovery_context_reports(settings, ["a", "b"])

    assert len(contexts) == 2
    assert calls == 1
