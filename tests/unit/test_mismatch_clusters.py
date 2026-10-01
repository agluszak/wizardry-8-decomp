"""Focused tests for the mismatch-cluster observations."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from wiz8decomp.reports.mismatch_clusters import (
    _census_row,
    identity_store_deltas,
    mismatch_clusters,
    text_fingerprints,
)


def _caller_row(*, orig_calls, recomp_calls, orig_size, recomp_size) -> dict:
    return {
        "address": "0x401000",
        "orig": {"calls": orig_calls, "body_ranges": [["00401000", f"{0x401000 + orig_size:08x}"]]},
        "recomp": {
            "calls": recomp_calls,
            "body_ranges": [["00501000", f"{0x501000 + recomp_size:08x}"]],
        },
    }


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
    assert [row["signals"] for row in report["functions"]] == [[]]
    assert [row["token_deltas"] for row in report["functions"]] == [[["predicate", "==", "!="]]]


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


def test_a_field_width_observation_is_never_replaced_by_another_signal(tmp_path: Path) -> None:
    """A width disagreement stays a width signal whatever else the function shows.

    The function also calls a same-unit helper the retail does not; nothing
    connects that call to this member access, so it must not erase the field
    observation or relabel it.
    """
    repository = tmp_path / "repo"
    directory = _minimal_report(tmp_path, field_uses=True)
    census = json.loads((directory / "direct-calls.json").read_text())
    census["functions"][0]["orig"] = {"calls": []}
    census["functions"][0]["recomp"] = {
        "calls": [{"identity": "pair:0x402000", "paired": True, "target": "0x502000"}]
    }
    (directory / "direct-calls.json").write_text(json.dumps(census))
    fields = json.loads((directory / "field-uses.json").read_text())
    fields["access_fingerprints"] = [
        {
            "owner": "Owner",
            "offset": "0x10",
            "operation": "read",
            "retail_observed_widths": [1],
            "rebuild_observed_widths": [4],
            "functions": ["0x401000"],
        }
    ]
    (directory / "field-uses.json").write_text(json.dumps(fields))

    mismatch_clusters(repository, directory)
    report = json.loads((directory / "mismatch-clusters.json").read_text())

    assert "field-width" in report["functions"][0]["signals"]
