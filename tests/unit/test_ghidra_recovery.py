"""Unit tests for the read-only headless recovery host."""

from __future__ import annotations

import json
from pathlib import Path
from types import SimpleNamespace

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.ghidra import recovery as recovery_host
from wiz8decomp.ghidra.recovery import parse_selection, run_headless_script


def test_parse_selection_accepts_addresses_and_ranges() -> None:
    assert parse_selection("0x004a5e50") == (0x4A5E50, None)
    assert parse_selection("4882000") == (4882000, None)
    assert parse_selection("0x004a5e50:0x004a6610") == (0x4A5E50, 0x4A6610)
    assert parse_selection(" 0x10:0x10 ") == (0x10, 0x10)


@pytest.mark.parametrize("text", ["", "0x20:0x10", "nonsense", "0x10:", "-0x10"])
def test_parse_selection_rejects_malformed_input(text: str) -> None:
    with pytest.raises(ValueError):
        parse_selection(text)


def _settings(tmp_path: Path) -> Settings:
    install = tmp_path / "ghidra-install"
    (install / "support").mkdir(parents=True)
    (install / "support/analyzeHeadless").write_text("", encoding="utf-8")
    repo = tmp_path / "repo"
    script_dir = repo / "tools/ghidra-scripts"
    script_dir.mkdir(parents=True)
    (script_dir / "Wiz8Recover.java").write_text("class Wiz8Recover {}\n", encoding="utf-8")
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": str(install),
            "WIZ8_INPUT_DIR": str(tmp_path / "inputs"),
            "WIZ8_WORK_DIR": str(tmp_path / "work"),
            "repo_dir": repo,
        }
    )


def test_run_headless_script_uses_read_only_headless_source_bundle(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    invocations: list[list[str]] = []

    def fake_run(argv, *, cwd, log_path=None, env=None, check=True):
        command = [str(part) for part in argv]
        invocations.append(command)
        output = Path(command[command.index("--output") + 1])
        output.write_text('{"functions": []}\n', encoding="utf-8")

    monkeypatch.setattr(recovery_host.subprocesses, "run", fake_run)
    output = run_headless_script(
        settings,
        "Wiz8Recover.java",
        ["--source-index", "index.json", "0x401000"],
        program_name="wiz8",
    )
    assert json.loads(output.read_text(encoding="utf-8")) == {"functions": []}
    command = invocations[0]
    assert "-readOnly" in command
    assert "-noanalysis" in command
    assert command[command.index("-scriptPath") + 1].endswith("tools/ghidra-scripts")
    assert command[command.index("-postScript") + 1] == "Wiz8Recover.java"
    assert "javac" not in command


def test_recover_functions_rejects_empty_selection() -> None:
    with pytest.raises(ValueError, match="at least one"):
        recovery_host.recover_functions(object(), [])


def test_explain_accepts_ranges(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    settings = SimpleNamespace(repo_dir=tmp_path)
    monkeypatch.setattr(
        recovery_host,
        "_recover",
        lambda _settings, selections, **_kwargs: {
            "program": "wiz8",
            "selections": selections,
            "exports": [
                {
                    "entry": "0x00401000",
                    "name": "Function401000",
                    "recovery": {
                        "emission_kind": "function",
                        "source_kind": "unrecovered",
                        "passes": [],
                        "defects": [],
                    },
                }
            ],
        },
    )

    result = recovery_host.explain_function(settings, "0x401000:0x401020")

    assert result["selections"] == ["0x00401000:0x00401020"]
    assert result["functions"][0]["entry"] == "0x00401000"
