"""Focused tests for the mismatch-cluster observations."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from wiz8decomp.reports.mismatch_clusters import (
    _census_row,
    _retail_trace_short,
    _signature_inferred,
    _unresolved_call_shape,
    broad_shape,
    control_flow_excess,
    identity_store_deltas,
    mismatch_clusters,
    same_unit_helper_calls,
    text_fingerprints,
)


def _callee(orig_source: str, recomp_source: str) -> dict:
    return {
        "private_comparison": {
            "orig": {"signature_source": orig_source},
            "recomp": {"signature_source": recomp_source},
        }
    }


def test_signature_inferred_names_the_ghidra_side_that_inferred() -> None:
    assert _signature_inferred(_callee("ANALYSIS", "ANALYSIS")) == "orig:ANALYSIS,recomp:ANALYSIS"
    assert _signature_inferred(_callee("USER_DEFINED", "ANALYSIS")) == "recomp:ANALYSIS"
    assert _signature_inferred(_callee("ANALYSIS", "USER_DEFINED")) == "orig:ANALYSIS"
    assert _signature_inferred(_callee("USER_DEFINED", "DEFAULT")) == "recomp:DEFAULT"


def test_signature_inferred_is_absent_when_neither_side_inferred() -> None:
    assert _signature_inferred(_callee("USER_DEFINED", "USER_DEFINED")) is None
    assert _signature_inferred(_callee("IMPORTED", "USER_DEFINED")) is None


def test_control_flow_excess_reports_a_decision_the_rebuild_does_not_have() -> None:
    old = ["if (*(int *)(param_1 + 0xc4) == 0) {", "    *param_2 = *param_3;", "    return;"]
    new = ["*(int *)(param_1 + 0xc4) = iVar0;"]

    assert control_flow_excess(old, new) == (("if", 1, 0), ("return", 1, 0))


def test_control_flow_excess_ignores_a_rebuild_that_has_more() -> None:
    old = ["iVar0 = 1;"]
    new = ["if (iVar0 != 0) {", "    return;"]

    assert control_flow_excess(old, new) == ()


def test_control_flow_excess_ignores_matching_shapes_and_generated_names() -> None:
    old = ["if (iVar0 != 0) {", "    return;"]
    new = ["if (iVar1 != 0) {", "    return;"]

    assert control_flow_excess(old, new) == ()


def test_control_flow_excess_counts_each_keyword_separately() -> None:
    old = ["while (iVar0 != 0) {", "    while (iVar1 != 0) {", "        goto LAB;"]
    new = ["while (iVar0 != 0) {"]

    assert control_flow_excess(old, new) == (("while", 2, 1), ("goto", 1, 0))


def _caller_row(*, orig_calls, recomp_calls, orig_size, recomp_size) -> dict:
    return {
        "address": "0x401000",
        "orig": {"calls": orig_calls, "body_ranges": [["00401000", f"{0x401000 + orig_size:08x}"]]},
        "recomp": {
            "calls": recomp_calls,
            "body_ranges": [["00501000", f"{0x501000 + recomp_size:08x}"]],
        },
    }


def test_retail_trace_short_flags_a_truncated_retail_side() -> None:
    traced = {0x401000: {"orig": 2, "recomp": 36}}

    assert _retail_trace_short(0x401000, traced) is True
    assert _retail_trace_short(0x402000, traced) is False


def test_retail_trace_short_accepts_roughly_matched_coverage() -> None:
    traced = {0x401000: {"orig": 9, "recomp": 10}, 0x402000: {"orig": 4, "recomp": 4}}

    assert _retail_trace_short(0x401000, traced) is False
    assert _retail_trace_short(0x402000, traced) is False


def test_census_row_is_absent_when_either_side_is_unresolved() -> None:
    calls = {
        0x401000: {"orig": {"calls": []}, "recomp": {"calls": []}},
        0x402000: {"orig": {"calls": None}, "recomp": {"calls": []}},
        0x403000: {"orig": {"calls": []}},
    }

    assert _census_row(calls, 0x401000) is not None
    assert _census_row(calls, 0x402000) is None
    assert _census_row(calls, 0x403000) is None
    assert _census_row(calls, 0x404000) is None


def test_same_unit_helper_call_is_a_carried_body_when_the_retail_names_its_callees() -> None:
    row = _caller_row(
        orig_calls=[{"identity": "pair:0x500000", "name": "SetValue"}],
        recomp_calls=[{"identity": "pair:0x400000", "name": "Decrement"}],
        orig_size=0x40,
        recomp_size=0x10,
    )
    targets = {
        ("orig", "pair:0x400000"): {
            "identity": "pair:0x400000",
            "calls": [{"identity": "pair:0x500000", "name": "SetValue"}],
            "body_ranges": [["00400000", "00400020"]],
        }
    }
    units = {0x400000: "a.cpp", 0x401000: "a.cpp"}

    assert same_unit_helper_calls(row, targets, units) == [
        ("retail-carried-body", "pair:0x400000", "Decrement")
    ]


def test_same_unit_helper_call_reports_a_helper_the_retail_never_reaches() -> None:
    row = _caller_row(
        orig_calls=[{"identity": "pair:0x500100", "name": "Other"}],
        recomp_calls=[{"identity": "pair:0x400000", "name": "Decrement"}],
        orig_size=0x10,
        recomp_size=0x10,
    )
    targets = {
        ("orig", "pair:0x400000"): {
            "identity": "pair:0x400000",
            "calls": [{"identity": "pair:0x500000", "name": "SetValue"}],
            "body_ranges": [["00400000", "00400020"]],
        }
    }
    units = {0x400000: "a.cpp", 0x401000: "a.cpp"}

    assert same_unit_helper_calls(row, targets, units) == [
        ("helper-identity", "pair:0x400000", "Decrement")
    ]


def test_same_unit_helper_call_reads_a_call_free_helper_by_the_size_it_grew() -> None:
    row = _caller_row(
        orig_calls=[],
        recomp_calls=[{"identity": "pair:0x400000", "name": "GetCharacter"}],
        orig_size=0x40,
        recomp_size=0x10,
    )
    targets = {
        ("orig", "pair:0x400000"): {
            "identity": "pair:0x400000",
            "calls": [],
            "body_ranges": [["00400000", "0040001c"]],
        }
    }
    units = {0x400000: "a.cpp", 0x401000: "a.cpp"}

    assert same_unit_helper_calls(row, targets, units) == [
        ("retail-carried-body", "pair:0x400000", "GetCharacter")
    ]


def test_same_unit_helper_call_ignores_helpers_from_another_unit() -> None:
    row = _caller_row(
        orig_calls=[],
        recomp_calls=[{"identity": "pair:0x400000", "name": "Elsewhere"}],
        orig_size=0x40,
        recomp_size=0x10,
    )
    targets = {
        ("orig", "pair:0x400000"): {
            "identity": "pair:0x400000",
            "calls": [],
            "body_ranges": [["00400000", "0040001c"]],
        }
    }
    units = {0x400000: "b.cpp", 0x401000: "a.cpp"}

    assert same_unit_helper_calls(row, targets, units) == []


def test_same_unit_helper_call_ignores_a_call_both_sides_make() -> None:
    row = _caller_row(
        orig_calls=[{"identity": "pair:0x400000", "name": "Decrement"}],
        recomp_calls=[{"identity": "pair:0x400000", "name": "Decrement"}],
        orig_size=0x10,
        recomp_size=0x10,
    )

    assert same_unit_helper_calls(row, {}, {0x401000: "a.cpp"}) == []


def test_identity_store_deltas_reports_a_table_only_one_side_installs() -> None:
    old = ["param_1[0x10] = &PTR_W8Vector___scalar_deleting_destructor__005ee744;"]
    new = ["param_1[0x10] = 0;"]

    assert identity_store_deltas(old, new) == {
        ("retail", "PTR_W8Vector___scalar_deleting_destructor__005ee744", "missing")
    }


def test_identity_store_deltas_ignores_shared_tables() -> None:
    line = "*param_1 = &W8Navigator___vftable_;"

    assert identity_store_deltas([line], [line]) == set()


def test_identity_store_deltas_separates_the_two_directions() -> None:
    old = ["*puVar0 = &W8TriggerEvent___vftable_;", "*local_14 = &stScript___vftable_;"]
    new = ["*puVar0 = &W8TextControl___vftable_;"]

    assert identity_store_deltas(old, new) == {
        ("retail", "W8TriggerEvent___vftable_", "renamed"),
        ("rebuild", "W8TextControl___vftable_", "renamed"),
        ("retail", "stScript___vftable_", "missing"),
    }


def test_identity_store_deltas_separates_a_rename_from_a_missing_table() -> None:
    """Each image names its own tables, so equal store sites are renames."""
    old = ["*param_1 = &PTR_W8Navigator___scalar_deleting_destructor__005ec2d0;"]
    new = ["*param_1 = &W8Navigator___vftable_;"]

    assert identity_store_deltas(old, new) == {
        ("retail", "PTR_W8Navigator___scalar_deleting_destructor__005ec2d0", "renamed"),
        ("rebuild", "W8Navigator___vftable_", "renamed"),
    }


def test_identity_store_deltas_ignores_non_identity_right_hand_sides() -> None:
    old = ["iVar0 = &local_buffer;", "puVar0 = g_scratch_buffer;"]

    assert identity_store_deltas(old, ["iVar0 = 1;", "puVar0 = g_scratch_buffer;"]) == set()


def test_broad_shape_keeps_generated_and_literal_distinctions() -> None:
    assert broad_shape(["iVar0 = 1;"], ["iVar1 = 1;"]) == "generated names only"
    assert broad_shape(["iVar0 = 1;"], ["iVar0 = 2;"]) == "literal or address only"
    assert broad_shape(["iVar0 = (undefined4)uVar1;"], ["iVar0 = (uint)uVar1;"]) == (
        "types or casts only"
    )
    assert broad_shape(["F();"], ["G();"]) == "calls differ"


def test_text_fingerprints_pair_a_single_token_change_only() -> None:
    old = ["if (value == 0) {"]
    new = ["if (value != 0) {"]

    assert text_fingerprints(old, new) == {("predicate", "==", "!=")}


def _minimal_report(tmp_path: Path, *, field_uses: bool, tag: str = "report") -> Path:
    """A report directory holding only what a bare compare --changed produces.

    The field and signature censuses are separate commands, so a fresh run has
    neither. The report has to classify what is there rather than fail.
    """
    repository = tmp_path / "repo"
    (repository / "build").mkdir(parents=True, exist_ok=True)
    (repository / "build/source-index.json").write_text(
        json.dumps({"target": "WIZ8", "declarations": [], "markers": []})
    )
    directory = tmp_path / tag
    directory.mkdir(exist_ok=True)
    manifest = {
        "objects": [
            {
                "orig": "0x401000",
                "recomp": "0x501000",
                "name": "Caller",
                "recomp_symbol": "?Caller@@YAXXZ",
                "type": "function",
            }
        ],
        "unpaired": [],
    }
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    inputs = {"manifest_sha256": digest}
    (directory / "manifest.json").write_text(json.dumps(manifest))
    (directory / "summary.json").write_text(
        json.dumps(
            {
                "target": "WIZ8",
                "inputs": inputs,
                "counts": {
                    "differences": 1,
                    "no-differences": 0,
                    "unpaired": 0,
                    "analysis-failed": 0,
                },
                "functions": [
                    {
                        "orig": "0x401000",
                        "recomp": "0x501000",
                        "name": "Caller",
                        "outcome": "differences",
                        "data": [],
                        "failures": [],
                        "unidentified_references": 0,
                        "code_diff": [
                            "--- a\n+++ b\n@@ -1,2 +1,2 @@\n-if (value == 0) {\n+if (value != 0) {\n"
                        ],
                    }
                ],
            }
        )
    )
    (directory / "direct-calls.json").write_text(
        json.dumps(
            {
                "manifest_sha256": digest,
                "scope": {"targets": 1, "functions": 1},
                "functions": [{"address": "0x401000", "targets": []}],
                "targets": [],
            }
        )
    )
    if field_uses:
        (directory / "field-uses.json").write_text(
            json.dumps(
                {
                    "comparison_inputs": inputs,
                    "private_access_observations": [],
                    "access_fingerprints": [],
                }
            )
        )
    return directory


def test_unresolved_call_shape_says_which_side_has_the_extra_call() -> None:
    """`unresolved-target` is one bucket for every unpaired callee.

    The shape is read from reccmp's own opcodes so the population stays a
    triage order; these are the counts that bucket was hiding.
    """
    both = {"deltas": [{"retail": ["a"], "rebuild": ["b"]}]}
    retail_only = {"deltas": [{"retail": ["a", "b"], "rebuild": []}]}
    rebuild_only = {"deltas": [{"retail": [], "rebuild": ["a", "b"]}]}

    assert _unresolved_call_shape(both) == "unresolved-calls-both-sides"
    assert _unresolved_call_shape(retail_only) == "unresolved-retail-only-calls"
    assert _unresolved_call_shape(rebuild_only) == "unresolved-rebuild-only-calls"
    assert _unresolved_call_shape({"deltas": []}) == "unresolved-no-call-delta"
    assert _unresolved_call_shape({}) == "unresolved-no-call-delta"


def test_unresolved_call_shape_sums_across_several_opcodes() -> None:
    delta = {
        "deltas": [
            {"retail": ["a"], "rebuild": []},
            {"retail": ["b"], "rebuild": ["c"]},
        ]
    }

    assert _unresolved_call_shape(delta) == "unresolved-calls-both-sides"


def test_mismatch_clusters_runs_without_the_optional_censuses(tmp_path: Path) -> None:
    """A bare comparison directory has no field-uses.json or signature-census.

    Both feed optional signals, so their absence must leave the report running
    rather than raise: the trace-coverage lookup is consulted for every
    differing function and was unbound without a census.
    """
    repository = tmp_path / "repo"
    directory = _minimal_report(tmp_path, field_uses=False)

    result = mismatch_clusters(repository, directory)
    report = json.loads((directory / "mismatch-clusters.json").read_text())

    assert result["metrics"]["differences"] == 1
    assert [row["signals"] for row in report["functions"]] == [["predicate"]]


def test_mismatch_clusters_agrees_with_and_without_the_field_census(
    tmp_path: Path,
) -> None:
    """An empty field census must not change the classification.

    The census is optional, so a report built from one and a report built
    without it have to agree; otherwise the presence of a census would silently
    decide a function's signals.
    """
    repository = tmp_path / "repo"
    mismatch_clusters(repository, _minimal_report(tmp_path, field_uses=False, tag="without"))
    mismatch_clusters(repository, _minimal_report(tmp_path, field_uses=True, tag="with"))

    without = json.loads((tmp_path / "without/mismatch-clusters.json").read_text())
    with_census = json.loads((tmp_path / "with/mismatch-clusters.json").read_text())

    assert without["functions"] == with_census["functions"]
    assert without["clusters"] == with_census["clusters"]
