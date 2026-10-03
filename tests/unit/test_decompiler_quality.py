"""Unit tests for decompiler-quality metric scoring and corpus sampling."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.decompiler_quality import (
    HIGH_FUNCTION_METRIC_KEYS,
    METRIC_KEYS,
    _stratified_sample,
    compute_quality_delta,
    debt_total,
    require_quality_measurement,
    score_decompiled,
    score_high_function,
)


def test_score_decompiled_counts_known_debt_tokens() -> None:
    text = """
    undefined4 local;
    DAT_005a1234 = FUN_00401000(param_1);
    CONCAT22(uVar1, uVar2);
    SUB41(in_EAX, 0);
    (**(code **)(this + 0x14))(this);
    value = *(int *)(param_2 + 0x38);
    ptr = (undefined4 *)local;
    """
    counts = score_decompiled(text)
    assert counts["undefined"] >= 2
    assert counts["dat_"] == 1
    assert counts["fun_"] == 1
    assert counts["concat"] == 1
    assert counts["sub_extract"] == 1
    assert counts["code_ptr"] == 1
    assert counts["param_n"] >= 2
    assert counts["anonymous_indirect_call"] == 1
    assert counts["untyped_ptr_arith"] >= 1
    assert counts["c_style_cast"] >= 1
    assert debt_total(counts) == sum(counts[key] for key in METRIC_KEYS)


def test_score_decompiled_empty_is_zero() -> None:
    assert debt_total(score_decompiled(None)) == 0
    assert debt_total(score_decompiled("")) == 0


def test_stratified_sample_is_stable_and_spread() -> None:
    markers = {0x401000 + i: SimpleNamespace(source_file=f"src/a/{i % 5}.cpp") for i in range(50)}
    first = _stratified_sample(markers, limit=10, seed=7)
    second = _stratified_sample(markers, limit=10, seed=7)
    assert first == second
    assert len(first) == 10
    files = {markers[address].source_file for address in first}
    assert len(files) >= 4


def test_compute_quality_delta_totals_and_functions() -> None:
    zero = {key: 0 for key in METRIC_KEYS}
    before = {
        "summary": {"totals": {**zero, "undefined": 5, "dat_": 1}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 5, "dat_": 1},
            }
        ],
    }
    after = {
        "summary": {"totals": {**zero, "undefined": 2, "dat_": 1}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 2, "dat_": 1},
            }
        ],
    }
    delta = compute_quality_delta(before, after)
    assert delta["ok"] is True
    assert delta["failure_delta"] == 0
    assert delta["decompiler_regressions"] == []
    assert delta["totals_delta"]["undefined"] == -3
    assert delta["totals_delta"]["dat_"] == 0
    assert delta["debt_total_delta"] == -3
    assert delta["functions"][0]["metrics_delta"]["undefined"] == -3


def test_compute_quality_delta_ok_to_failure_is_hard_regression() -> None:
    zero = {key: 0 for key in METRIC_KEYS}
    before = {
        "summary": {"totals": {**zero, "undefined": 18}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 18},
            }
        ],
    }
    after = {
        "summary": {"totals": {**zero}, "failures": 1},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "decompiler-failure",
                "metrics": {**zero},
            }
        ],
    }
    delta = compute_quality_delta(before, after)
    assert delta["ok"] is False
    assert delta["failure_delta"] == 1
    assert len(delta["decompiler_regressions"]) == 1
    assert delta["decompiler_regressions"][0]["address"] == "0x00401000"
    row = delta["functions"][0]
    assert row["debt_delta"] is None
    assert row["decompiler_regression"] is True


def test_compute_quality_delta_positive_debt_fails_gate() -> None:
    zero = {key: 0 for key in METRIC_KEYS}
    before = {
        "summary": {"totals": {**zero, "undefined": 2}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 2},
            }
        ],
    }
    after = {
        "summary": {"totals": {**zero, "undefined": 5}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 5},
            }
        ],
    }
    delta = compute_quality_delta(before, after)
    assert delta["ok"] is False
    assert delta["failure_delta"] == 0
    assert delta["decompiler_regressions"] == []
    assert delta["debt_total_delta"] == 3
    assert delta["max_debt_total_delta"] == 0


def test_compute_quality_delta_debt_allowance_permits_reviewed_increase() -> None:
    zero = {key: 0 for key in METRIC_KEYS}
    before = {
        "summary": {"totals": {**zero, "undefined": 2}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 2},
            }
        ],
    }
    after = {
        "summary": {"totals": {**zero, "undefined": 5}, "failures": 0},
        "functions": [
            {
                "address": "0x00401000",
                "name": "Foo",
                "status": "ok",
                "metrics": {**zero, "undefined": 5},
            }
        ],
    }
    delta = compute_quality_delta(before, after, max_debt_total_delta=3)
    assert delta["ok"] is True
    assert delta["debt_total_delta"] == 3
    assert delta["max_debt_total_delta"] == 3


def test_pain_corpus_keeps_functions_with_differences(monkeypatch) -> None:
    from pathlib import Path

    from wiz8decomp import decompiler_quality as dq

    markers = {
        0x401000: SimpleNamespace(source_file="a.cpp", marker_kind="FUNCTION", name="A"),
        0x402000: SimpleNamespace(source_file="b.cpp", marker_kind="FUNCTION", name="B"),
        0x403000: SimpleNamespace(source_file="c.cpp", marker_kind="FUNCTION", name="C"),
        0x404000: SimpleNamespace(source_file="d.cpp", marker_kind="FUNCTION", name="D"),
    }
    monkeypatch.setattr(dq, "_function_markers", lambda *_a, **_k: markers)
    monkeypatch.setattr(
        dq,
        "_stratified_sample",
        lambda _markers, *, limit, seed: sorted(markers)[:limit],
    )

    def fake_compare(_repo, _target, addresses, _ghidra):
        outcome_by = {
            0x401000: "no-differences",
            0x402000: "differences",
            0x403000: "differences",
            0x404000: "analysis-failed",
        }
        return {
            "functions": [
                {
                    "orig": f"0x{address:08x}",
                    "name": markers[address].name,
                    "outcome": outcome_by[address],
                }
                for address in addresses
            ]
        }

    monkeypatch.setattr("wiz8decomp.comparison.compare_selected", fake_compare)

    corpus = dq.select_corpus(
        Path("/tmp"),
        target="WIZ8",
        limit=10,
        seed=1,
        corpus_kind="pain",
        ghidra_install_dir=Path("/opt/ghidra"),
    )
    assert corpus["match_filter"] == "differences"
    assert corpus["addresses"] == [0x402000, 0x403000]
    assert {row["outcome"] for row in corpus["matches"]} == {"differences"}


def test_require_quality_measurement_rejects_missing_report(tmp_path) -> None:
    import pytest

    settings = SimpleNamespace(build_dir=tmp_path)
    with pytest.raises(RuntimeError, match="decompiler-quality"):
        require_quality_measurement(settings, object(), "wiz8")


def test_require_quality_measurement_requires_matching_fingerprint(tmp_path, monkeypatch) -> None:
    import json

    import pytest

    report_dir = tmp_path / "decompiler-quality"
    report_dir.mkdir()
    (report_dir / "report.json").write_text(
        json.dumps(
            {
                "program": "wiz8",
                "summary": {"ok": 3, "failures": 0},
                "program_state": {
                    "binary_sha256": "aaa",
                    "function_count": 10,
                    "datatype_count": 20,
                },
            }
        ),
        encoding="utf-8",
    )
    settings = SimpleNamespace(build_dir=tmp_path)
    monkeypatch.setattr(
        "wiz8decomp.decompiler_quality.program_analysis_fingerprint",
        lambda _program: {
            "binary_sha256": "bbb",
            "function_count": 10,
            "datatype_count": 20,
        },
    )
    with pytest.raises(RuntimeError, match="current ProgramDB"):
        require_quality_measurement(settings, object(), "wiz8")


def test_run_decompiler_quality_records_canonical_program_name(tmp_path, monkeypatch) -> None:
    import json

    import wiz8decomp.decompiler_quality as dq

    canonical_name = "wiz8--gog-base--wiz8--18a74ff61c65"
    settings = SimpleNamespace(
        repo_dir=tmp_path, build_dir=tmp_path / "build", ghidra_install_dir=tmp_path
    )
    evaluated_programs = []
    monkeypatch.setattr(
        "wiz8decomp.ghidra.project.resolve_program_name",
        lambda _settings, selector: canonical_name if selector == "wiz8" else selector,
    )
    monkeypatch.setattr(
        dq,
        "select_corpus",
        lambda *_args, **_kwargs: {
            "addresses": [0x401000],
            "seed": 1,
            "limit": 1,
            "candidates": 1,
            "match_filter": "exact|effective",
            "corpus_kind": "oracle",
            "corpus_source": "test",
        },
    )

    def evaluate(_settings, _addresses, *, program_name, profile):
        evaluated_programs.append(program_name)
        return {
            "program_state": {},
            "functions": [],
            "summary": {
                "requested": 1,
                "ok": 1,
                "failures": 0,
                "totals": {},
                "mean_debt": 0.0,
                "max_debt": 0,
            },
        }

    monkeypatch.setattr(dq, "evaluate_corpus", evaluate)
    report_dir = tmp_path / "decompiler-quality"

    dq.run_decompiler_quality(settings, program_name="wiz8", out_dir=report_dir)

    report = json.loads((report_dir / "report.json").read_text(encoding="utf-8"))
    assert evaluated_programs == [canonical_name]
    assert report["program"] == canonical_name


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
    assert set(counts) == set(HIGH_FUNCTION_METRIC_KEYS)


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


def test_evaluate_corpus_shares_decompilation_and_keeps_high_metrics_informational(
    tmp_path, monkeypatch
) -> None:
    from contextlib import nullcontext

    import wiz8decomp.decompiler_quality as dq
    from wiz8decomp.ghidra import env, inspect

    high = SimpleNamespace(
        getFunctionPrototype=lambda: SimpleNamespace(
            getModelName=lambda: "unknown",
            getReturnType=lambda: "undefined4",
            getNumParams=lambda: 0,
        ),
        getPcodeOps=lambda: _Ops([]),
    )
    calls = []
    closed = []

    class Session:
        def __init__(self, program, *, profile):
            pass

        def decompile(self, function):
            calls.append(function)
            return SimpleNamespace(
                decompileCompleted=lambda: function != 3,
                getDecompiledFunction=lambda: SimpleNamespace(getC=lambda: "void f() {}"),
                getHighFunction=lambda: high if function == 1 else None,
                getErrorMessage=lambda: "timeout",
            )

        def close(self):
            closed.append(True)

    monkeypatch.setattr(env, "open_program", lambda *_args: nullcontext(object()))
    monkeypatch.setattr(inspect, "DecompileSession", Session)
    monkeypatch.setattr(dq, "program_analysis_fingerprint", lambda _program: {"test": True})
    monkeypatch.setattr(dq, "_resolve_function", lambda _program, addr: addr if addr < 4 else None)
    report = dq.evaluate_corpus(SimpleNamespace(repo_dir=tmp_path), [1, 2, 3, 4], markers={})
    assert calls == [1, 2, 3]
    assert closed == [True]
    assert report["summary"]["ok"] == 2
    assert report["summary"]["failures"] == 2
    assert report["summary"]["high_function_measured"] == 1
    assert report["summary"]["high_function_totals"]["untyped_return"] == 1
    assert report["summary"]["mean_debt"] == 0
    assert report["functions"][1]["high_function_metrics"] is None
    assert report["functions"][2]["status"] == "decompiler-failure"
    assert report["functions"][3]["status"] == "missing-function"
