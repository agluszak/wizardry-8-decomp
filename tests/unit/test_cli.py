from typer.testing import CliRunner
from wiz8decomp.cli import app


def test_cli_uses_typed_command_tree() -> None:
    result = CliRunner().invoke(app, ["--help"])
    assert result.exit_code == 0
    assert "ghidra" in result.stdout
    assert "inputs" in result.stdout
    assert "extract" in result.stdout
