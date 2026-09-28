from __future__ import annotations

import typer

app = typer.Typer(
    help="Recovery-compiler workflows over the Ghidra engine.",
    no_args_is_help=True,
)


@app.command("self-test")
def self_test_command() -> None:
    """Build and recover the pinned VC6 lifecycle fixture transiently."""

    from .. import command_support as cli
    from ..ghidra.lifecycle_fixture import verify_lifecycle_fixture

    cli.emit(verify_lifecycle_fixture(cli.settings()))
