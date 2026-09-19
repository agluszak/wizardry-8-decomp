"""Unit tests for decompiler-quality metric scoring and corpus sampling."""

from __future__ import annotations

from types import SimpleNamespace

from wiz8decomp.decompiler_quality import (
    METRIC_KEYS,
    _stratified_sample,
    compute_quality_delta,
    debt_total,
    require_quality_measurement,
    score_decompiled,
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


def test_pain_corpus_filters_to_mismatch_inconclusive(monkeypatch) -> None:
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

    def fake_compare(_repo, _target, addresses):
        status_by = {
            0x401000: "exact",
            0x402000: "mismatch",
            0x403000: "inconclusive",
            0x404000: "effective",
        }
        return {
            "functions": [
                {
                    "address": f"0x{address:08x}",
                    "name": markers[address].name,
                    "status": status_by[address],
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
    )
    assert corpus["match_filter"] == "mismatch|inconclusive"
    assert corpus["addresses"] == [0x402000, 0x403000]
    assert {row["status"] for row in corpus["matches"]} == {"mismatch", "inconclusive"}


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
