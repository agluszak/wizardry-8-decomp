import json

from typer.testing import CliRunner
from wiz8decomp import cli
from wiz8decomp.cli import app
from wiz8decomp.ghidra import query_daemon


def test_cli_uses_typed_command_tree() -> None:
    result = CliRunner().invoke(app, ["--help"])
    assert result.exit_code == 0
    assert "ghidra" in result.stdout
    assert "inputs" in result.stdout
    assert "extract" in result.stdout


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
