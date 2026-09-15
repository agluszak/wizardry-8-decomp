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


@pytest.mark.parametrize("scenario", [None, "main-game-start"])
@pytest.mark.parametrize(
    "reason,exit_code",
    [
        ("exited normally", 0),
        ("exited with code 3", 3),
        ("SIGSEGV", 1),
        ("debugger timeout", 1),
        ("breakpoint-hit", 0),
    ],
)
def test_debug_builds_selected_product_before_launch(
    monkeypatch, scenario, reason, exit_code
) -> None:
    from wiz8decomp import build
    from wiz8decomp.debug import debugger

    events = []
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace())
    monkeypatch.setattr(build, "build_target", lambda _, target: events.append(target))

    def launch(_, arguments, **options):
        events.append("launch")
        assert arguments == []
        assert options == {
            "scenario": scenario,
            "timeout": 7,
            "breakpoints": [(0x401000, "$eax == 1")],
        }
        return {
            "report": "stopped\n",
            "reason": reason,
            "exit_code": exit_code,
            "log": "raw.txt",
            "session": "session.json",
        }

    monkeypatch.setattr(debugger, "run_debugger", launch)
    arguments = ["debug", "--timeout", "7", "--break", "0x401000:$eax == 1"]
    if scenario is not None:
        arguments.extend(["--scenario", scenario])
    result = CliRunner().invoke(app, arguments)
    assert result.exit_code == exit_code, result.output
    assert events == ["runtime-test" if scenario else "runtime", "launch"]
    assert "session: session.json" in result.output


def test_compare_changed_uses_existing_index_without_building(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, comparison, source_index

    settings = SimpleNamespace(repo_dir=tmp_path)
    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    source = tmp_path / "new.cpp"
    source.write_text("// FUNCTION: WIZ8 0x00401000\nvoid added() {}\n")
    (tmp_path / "build").mkdir()
    index = tmp_path / "build/source-index.json"
    stale = {"schema": "reccmp-source-index-v2", "markers": []}
    index.write_text(json.dumps(stale))
    events = []

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
    monkeypatch.setattr(comparison, "changed_source_files", lambda *_args: [source])
    monkeypatch.setattr(
        source_index, "write_source_index", lambda *_args: pytest.fail("must not refresh index")
    )
    monkeypatch.setattr(
        build, "build_target", lambda *_args: pytest.fail("must not build products")
    )
    monkeypatch.setattr(comparison, "compare_selected", compare)

    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code == 0, result.output
    assert events == ["compare"]
    payload = json.loads(result.stdout)
    assert payload["functions"][0]["address"] == "0x00401000"
    assert payload["selection"]["changed_files"] == ["new.cpp"]
    assert payload["selection"]["dependent_files"] == []


def test_compare_changed_does_not_fall_back_to_whole_image(tmp_path, monkeypatch) -> None:
    from wiz8decomp import comparison

    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace(repo_dir=tmp_path))
    monkeypatch.setattr(comparison, "changed_source_files", lambda *_args: [])
    result = CliRunner().invoke(app, ["compare", "--changed"])
    assert result.exit_code != 0
    assert isinstance(result.exception, ValueError)
    assert "no functions selected" in str(result.exception)


def test_numeric_compare_is_read_only_and_passes_exact_addresses(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, comparison, source_index

    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    seen = []
    monkeypatch.setattr(command_support, "settings", lambda: SimpleNamespace(repo_dir=tmp_path))
    monkeypatch.setattr(
        source_index, "write_source_index", lambda *_args: pytest.fail("must not write index")
    )
    monkeypatch.setattr(build, "build_target", lambda *_args: pytest.fail("must not build"))
    monkeypatch.setattr(
        comparison,
        "compare_selected",
        lambda _repo, _target, addresses, **_kwargs: seen.append(addresses) or {"ok": True},
    )

    result = CliRunner().invoke(app, ["compare", "0x4538d0"])

    assert result.exit_code == 0, result.output
    assert seen == [[0x4538D0]]


def test_compare_build_explicitly_refreshes_and_builds(tmp_path, monkeypatch) -> None:
    from wiz8decomp import build, comparison, source_index

    (tmp_path / "reccmp-project.yml").write_text(
        "targets:\n  WIZ8:\n    filename: Wiz8.exe\n    hash:\n      sha256: abc\n"
    )
    settings = SimpleNamespace(repo_dir=tmp_path)
    events = []
    monkeypatch.setattr(command_support, "settings", lambda: settings)
    monkeypatch.setattr(source_index, "write_source_index", lambda actual: events.append("index"))
    monkeypatch.setattr(
        build, "build_target", lambda actual, target: events.append(("build", target))
    )
    monkeypatch.setattr(
        comparison, "compare_selected", lambda *_args, **_kwargs: events.append("compare") or {}
    )

    result = CliRunner().invoke(app, ["compare", "--build", "0x4538d0"])

    assert result.exit_code == 0, result.output
    assert events == ["index", ("build", "WIZ8"), "compare"]


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

    run = CliRunner().invoke(app, ["run", "--help"], terminal_width=120)
    assert run.exit_code == 0
    assert "--original" in re.sub(r"\x1b\[[0-9;]*m", "", run.stdout)


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
