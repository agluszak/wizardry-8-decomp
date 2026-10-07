"""Baseline failures retain head evidence and restore the shared checkout."""

import json
import os
import shutil
import subprocess
from importlib.util import module_from_spec, spec_from_file_location
from pathlib import Path
from types import SimpleNamespace

import pytest

_SCRIPT = Path(__file__).resolve().parents[2] / ".github/scripts/compare-reccmp-base.py"
_SPEC = spec_from_file_location("ci_baseline", _SCRIPT)
assert _SPEC is not None and _SPEC.loader is not None
_MODULE = module_from_spec(_SPEC)
_SPEC.loader.exec_module(_MODULE)


def test_baseline_reports_use_frozen_selection_and_retain_failed_evidence(tmp_path, monkeypatch):
    from wiz8decomp import comparison, config, source_index

    result = {
        "ok": False,
        "report": {"classified_summary": "completed-run/summary.json"},
    }
    stages = []

    def compare(repository, target, addresses, ghidra, **_kwargs):
        assert (repository, target, addresses, ghidra) == (tmp_path, "WIZ8", [0x1000], "ghidra")
        return result

    monkeypatch.setattr(
        config, "load_settings", lambda **_kwargs: SimpleNamespace(ghidra_install_dir="ghidra")
    )
    monkeypatch.setattr(source_index, "write_source_index", lambda _settings: None)
    monkeypatch.setattr(comparison, "compare_selected", compare)
    with pytest.raises(ValueError, match="unpaired or incomplete"):
        _MODULE._reports(tmp_path, "WIZ8", "wiz8", tmp_path, [0x1000], stages.append)
    assert stages == ["source-index", "comparison"]
    assert json.loads((tmp_path / "wiz8-base-summary.json").read_text()) == result


@pytest.mark.parametrize("failure", ["build", "comparison", "restore", None])
def test_baseline_retains_evidence_and_reports_failure_stage(tmp_path, monkeypatch, failure):
    tools = tmp_path / ".venv/bin"
    tools.mkdir(parents=True)
    executable = tools / "reccmp-project"
    executable.write_text("#!/bin/sh\nexit 0\n")
    executable.chmod(0o755)
    monkeypatch.setenv("PATH", "/usr/bin:/bin")
    assert shutil.which("reccmp-project") is None
    native_run = subprocess.run
    head_report = tmp_path / "wiz8-head-summary.json"
    evidence = json.dumps({"target": "WIZ8", "functions": [{"orig": "0x1000"}]})
    head_report.write_text(evidence)
    commands = []

    def execute(args, **_kwargs):
        commands.append(tuple(args))
        if "prepare" in args:
            # A baseline CLI's child must find tools without uv or shell activation.
            native_run(["reccmp-project"], check=True)
        if (failure == "build" and "build" in args) or (
            failure == "restore" and args == ("git", "checkout", "--detach", "head")
        ):
            raise subprocess.CalledProcessError(1, args)

    def reports(_repo, _target, _prefix, _directory, addresses, mark):
        assert shutil.which("reccmp-project") == str(executable)
        assert addresses == [0x1000]
        mark("comparison")
        if failure == "comparison":
            raise ValueError("incomplete analysis")
        return {"inputs": {}, "ok": True}

    monkeypatch.setattr(_MODULE.subprocess, "run", execute)
    monkeypatch.setattr(_MODULE, "_reports", reports)
    code = _MODULE.compare_base(tmp_path, "WIZ8", "match", "base", "head", "wiz8", tmp_path)
    status = json.loads((tmp_path / "wiz8-base-status.json").read_text())
    assert os.environ["PATH"] == "/usr/bin:/bin"
    assert head_report.read_text() == evidence
    assert commands[-1] == ("git", "checkout", "--detach", "head")
    assert sum(args[:2] == ("git", "checkout") for args in commands) == 2
    assert not any("merge-base" in args or "uv" in args for args in commands)
    assert code == int(failure is not None)
    assert status["source_revision"] == "base"
    assert status["comparison_revision"] == "head"
    assert status["status"] == ("failed" if failure is not None else "completed")
    if failure in ("build", "comparison"):
        assert status["stage"] == failure
        assert status["error"]
    if failure == "restore":
        assert status["restore_error"]
    if failure is None:
        report = json.loads((tmp_path / "wiz8-base-summary.json").read_text())
        assert report["inputs"] == {"source_revision": "base", "wizardry_revision": "head"}
