import json
import re
from pathlib import Path

from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.extract import variants

REPOSITORY = Path(__file__).resolve().parents[2]


def test_cli_uses_typed_command_tree() -> None:
    result = CliRunner().invoke(app, ["--help"])
    assert result.exit_code == 0
    assert "ghidra" in result.stdout
    assert "corpus" in result.stdout
    assert "inputs" not in result.stdout
    assert "pipeline" not in result.stdout
    assert "function-census" not in result.stdout
    assert "sgp" not in result.stdout
    assert "doctor-command" not in result.stdout
    assert "doctor" in result.stdout
    assert "evidence" in result.stdout
    assert "analyze" in result.stdout
    for command in ("lint", "triage", "vtable", "datacmp", "addr", "runtime-test"):
        assert command in result.stdout

    for obsolete_root in ("function-census", "inventory", "trace", "reconstructed-transfer"):
        rejected = CliRunner().invoke(app, [obsolete_root, "--help"])
        assert rejected.exit_code != 0

    evidence = CliRunner().invoke(app, ["evidence", "refresh", "--help"])
    assert evidence.exit_code == 0
    assert "function-census" in evidence.stdout
    upsert = CliRunner().invoke(
        app,
        ["evidence", "upsert", "--help"],
        terminal_width=120,
    )
    assert upsert.exit_code == 0
    upsert_help = re.sub(r"\x1b\[[0-9;]*m", "", upsert.stdout)
    assert "--row-file" in upsert_help
    assert "--field" in upsert_help

    analyze = CliRunner().invoke(app, ["analyze", "--help"])
    assert analyze.exit_code == 0
    assert "inventory" in analyze.stdout


def test_corpus_extract_accepts_multiple_roles(monkeypatch) -> None:
    settings = object()
    seen: list[tuple[object, str]] = []
    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(
        variants,
        "extract_role",
        lambda actual, role: seen.append((actual, role)) or {"role": role},
    )

    result = CliRunner().invoke(app, ["corpus", "extract", "demo", "patch-128"])

    assert result.exit_code == 0
    assert seen == [(settings, "demo"), (settings, "patch-128")]
    assert json.loads(result.stdout) == [{"role": "demo"}, {"role": "patch-128"}]


def test_corpus_extract_all_uses_the_canonical_sequence(monkeypatch) -> None:
    settings = object()
    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(
        variants,
        "extract_all",
        lambda actual: {"all": actual is settings},
    )

    result = CliRunner().invoke(app, ["corpus", "extract", "--all"])

    assert result.exit_code == 0
    assert json.loads(result.stdout) == {"all": True}
