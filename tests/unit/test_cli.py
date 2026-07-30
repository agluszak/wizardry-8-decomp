import json
import re
from pathlib import Path

from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.extract import variants

REPOSITORY = Path(__file__).resolve().parents[2]


def test_cli_groups_subcommands_instead_of_exposing_them_at_the_root() -> None:
    """Grouped work is reachable only through its group.

    Pinning the absent names of every retired command makes this test fail on
    any CLI reshuffle, so it asserts the current shape instead: each group is
    present at the root, and a grouped command is not.
    """

    result = CliRunner().invoke(app, ["--help"])
    assert result.exit_code == 0
    for group in ("corpus", "ghidra", "report", "toolchain", "evidence", "analyze"):
        assert group in result.stdout
        assert CliRunner().invoke(app, [group, "--help"]).exit_code == 0

    assert "inventory" not in result.stdout
    assert CliRunner().invoke(app, ["inventory", "--help"]).exit_code != 0

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

    verify = CliRunner().invoke(app, ["verify", "--help"], terminal_width=120)
    assert verify.exit_code == 0
    assert "--against" in re.sub(r"\x1b\[[0-9;]*m", "", verify.stdout)


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
