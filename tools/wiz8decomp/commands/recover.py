from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(
    help="Recovery-compiler workflows over the Ghidra exporter.",
    no_args_is_help=True,
)


@app.command("regress")
def regress_command(
    addresses: Annotated[
        list[str],
        typer.Argument(help="Function addresses whose recovered bodies to regenerate."),
    ],
    target: str = typer.Option("WIZ8", "--target"),
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Export, splice, build, and compare each function; restore afterwards."""
    from .. import command_support as cli
    from ..recover import regress

    cli.run_action(
        lambda: regress(cli.settings(), list(addresses), target=target, program_selector=program)
    )
