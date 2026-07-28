import json
from pathlib import Path

from typer.testing import CliRunner
from wiz8decomp import cli
from wiz8decomp.cli import app
from wiz8decomp.extract import variants
from wiz8decomp.ghidra import query_daemon

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


def test_corpus_extract_accepts_multiple_roles(monkeypatch) -> None:
    settings = object()
    seen: list[tuple[object, str]] = []
    monkeypatch.setattr(cli, "_settings", lambda: settings)
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
    monkeypatch.setattr(cli, "_settings", lambda: settings)
    monkeypatch.setattr(
        variants,
        "extract_all",
        lambda actual: {"all": actual is settings},
    )

    result = CliRunner().invoke(app, ["corpus", "extract", "--all"])

    assert result.exit_code == 0
    assert json.loads(result.stdout) == {"all": True}


def test_public_ghidra_surface_hides_replay_and_process_lifecycle() -> None:
    result = CliRunner().invoke(app, ["ghidra", "--help"])

    assert result.exit_code == 0
    for command in ("apply-functions", "validate-replay", "daemon", "cache", "import"):
        rejected = CliRunner().invoke(app, ["ghidra", command, "--help"])
        assert rejected.exit_code != 0
    for command in ("query", "rebuild", "seed", "overlay"):
        assert command in result.stdout


def test_ghidra_query_emits_unwrapped_json_at_narrow_terminal_width(monkeypatch) -> None:
    decompiled = "int example(void) {\n    return 123456789;\n}\n"
    monkeypatch.setattr(cli, "_settings", lambda: object())
    monkeypatch.setattr(
        query_daemon,
        "query",
        lambda _settings, program, command, arguments: (
            {"decompiled": decompiled, "arguments": arguments},
            "daemon",
        ),
    )

    result = CliRunner().invoke(
        app,
        ["ghidra", "query", "canonical", "decompile", "0x401000"],
        terminal_width=20,
    )

    assert result.exit_code == 0
    payload = json.loads(result.stdout)
    assert payload["transport"] == "daemon"
    assert payload["program"] == "canonical"
    assert payload["result"]["decompiled"] == decompiled
    assert payload["result"]["arguments"] == ["0x401000"]


def test_ghidra_query_sends_repeated_query_clauses_as_one_ordered_batch(monkeypatch) -> None:
    monkeypatch.setattr(cli, "_settings", lambda: object())
    seen: list[tuple[str, list[tuple[str, list[str]]]]] = []

    def query_many(_settings, program, queries):
        seen.append((program, queries))
        return [
            {"command": command, "arguments": arguments, "result": {"index": index}}
            for index, (command, arguments) in enumerate(queries)
        ], "daemon"

    monkeypatch.setattr(query_daemon, "query_many", query_many)

    result = CliRunner().invoke(
        app,
        [
            "ghidra",
            "query",
            "canonical",
            "-q",
            "function 0x401000",
            "-q",
            'search "Monster Info"',
        ],
    )

    assert result.exit_code == 0
    payload = json.loads(result.stdout)
    assert seen == [
        (
            "canonical",
            [("function", ["0x401000"]), ("search", ["Monster Info"])],
        )
    ]
    assert [item["result"]["index"] for item in payload["results"]] == [0, 1]


def test_just_ghidra_preserves_each_quoted_batch_clause() -> None:
    justfile = (REPOSITORY / "Justfile").read_text(encoding="utf-8")

    assert '[positional-arguments]\nghidra *args:\n    uv run wiz8 ghidra "$@"' in justfile
