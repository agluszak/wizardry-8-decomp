from __future__ import annotations

import json
from pathlib import Path

import pytest
from wiz8decomp.dynamic import freeze_reference
from wiz8decomp.paths import sha256_file


def _reports(tmp_path: Path):
    runs = {}
    for label in ("retail-a", "retail-b", "recomp"):
        provenance = {
            key: "a" * 64
            for key in (
                "executable_sha256",
                "provider_sha256",
                "trace_plan_sha256",
                "reviewed_evidence_sha256",
                "link_map_sha256",
            )
        }
        provenance.update(
            repository_revision="revision",
            fixture={"name": "fixture", "sha256": "b" * 64},
            wine={"version": "wine"},
            gdb={"version": "gdb"},
        )
        capture = {
            "scenario": f"load-{label}",
            "events": [{"name": "LoadGame"}],
            "started": True,
            "state": {},
            "provenance": provenance,
        }
        path = tmp_path / f"load-{label}.json"
        path.write_text(json.dumps(capture))
        runs[label] = {**capture, "events": 1, "capture_sha256": sha256_file(path)}
    requirements = dict.fromkeys(
        (
            "all_started",
            "no_capture_failures",
            "all_reached_terminal",
            "retail_repeatable",
            "no_unwatched_points",
            "streams_agree",
            "state_repeatable",
            "state_agrees",
        ),
        True,
    )
    report = {"scenario": "load", "affirmative": True, "requirements": requirements, "runs": runs}
    path = tmp_path / "differential.json"
    path.write_text(json.dumps(report))
    return path, report


def test_reference_retains_full_captures_and_refuses_overwrite(tmp_path):
    report, _ = _reports(tmp_path)
    output = tmp_path / "reference.json"
    result = freeze_reference(report, tmp_path, output)
    assert result["sha256"] == sha256_file(output)
    reference = json.loads(output.read_text())
    assert reference["captures"]["recomp"]["events"] == [{"name": "LoadGame"}]
    with pytest.raises(ValueError, match="already exists"):
        freeze_reference(report, tmp_path, output)


@pytest.mark.parametrize(
    "mutation,error",
    [
        ("failure", "incomplete"),
        ("hash", "capture hash"),
        ("provider", "provider_sha256"),
        ("fixture", "fixture provenance"),
    ],
)
def test_freeze_rejects_incomplete_or_changed_evidence(tmp_path, mutation, error):
    path, report = _reports(tmp_path)
    if mutation == "failure":
        report["requirements"]["retail_repeatable"] = False
    else:
        capture = tmp_path / "load-recomp.json"
        raw = json.loads(capture.read_text())
        if mutation == "hash":
            raw["events"][0]["name"] = "OtherFunction"
        else:
            del raw["provenance"]["provider_sha256" if mutation == "provider" else "fixture"]
            report["runs"]["recomp"]["provenance"] = raw["provenance"]
        capture.write_text(json.dumps(raw))
        if mutation != "hash":
            report["runs"]["recomp"]["capture_sha256"] = sha256_file(capture)
    path.write_text(json.dumps(report))
    with pytest.raises(ValueError, match=error):
        freeze_reference(path, tmp_path, tmp_path / "reference.json")
