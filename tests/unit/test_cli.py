import json
import re
from pathlib import Path
from types import SimpleNamespace

import pytest
from typer.testing import CliRunner
from wiz8decomp import command_support
from wiz8decomp.cli import app
from wiz8decomp.extract import variants

REPOSITORY = Path(__file__).resolve().parents[2]


def test_compare_refreshes_changed_file_selection_before_build(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, reccmp_workflows, source_index

    settings = SimpleNamespace(repo_dir=tmp_path)
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    source = tmp_path / "new.cpp"
    source.write_text("// FUNCTION: WIZ8 0x00401000\nvoid added() {}\n")
    (tmp_path / "build").mkdir()
    index = tmp_path / "build/source-index.json"
    stale = {"schema": "reccmp-source-index-v1", "markers": []}
    index.write_text(json.dumps(stale))
    events = []

    def refresh(actual):
        assert actual is settings
        events.append("index")
        index.write_text(
            json.dumps(
                {
                    **stale,
                    "markers": [
                        {
                            "marker_kind": "FUNCTION",
                            "address": 0x401000,
                            "source_file": "new.cpp",
                            "target": "WIZ8",
                        }
                    ],
                }
            )
        )

    def compare(_repo, _target, selected, **_kwargs):
        assert selected == [0x401000]
        events.append("compare")
        return {"functions": [{"address": "0x00401000", "name": "added", "status": "exact"}]}

    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(reccmp_workflows, "changed_source_files", lambda *_args: [source])
    monkeypatch.setattr(source_index, "write_source_index", refresh)
    monkeypatch.setattr(build, "build_target", lambda *_args: events.append("build"))
    monkeypatch.setattr(reccmp_workflows, "compare_selected", compare)

    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code == 0, result.output
    assert events == ["index", "build", "compare"]
    payload = json.loads(result.stdout)
    assert payload["functions"][0]["address"] == "0x00401000"
    assert "unchanged callers" in payload["selection"]["warning"]


def test_compare_changed_does_not_fall_back_to_whole_image(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, reccmp_workflows

    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace(repo_dir=tmp_path))
    monkeypatch.setattr(reccmp_workflows, "changed_source_files", lambda *_args: [])
    monkeypatch.setattr(build, "compare", lambda *_args, **_kwargs: pytest.fail("whole image"))
    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code != 0
    assert "no changed C++ files" in result.output


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
    assert "debug-artifacts" in evidence.stdout
    assert "surrender-abi" in evidence.stdout
    assert "function-census" not in evidence.stdout
    assert CliRunner().invoke(app, ["evidence", "upsert", "--help"]).exit_code != 0

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
