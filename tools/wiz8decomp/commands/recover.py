from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(
    help="Recovery-compiler workflows over the Ghidra exporter.",
    no_args_is_help=True,
)


@app.command("function")
def function_command(
    address: Annotated[
        str,
        typer.Argument(help="Entry address of the function to recover, e.g. 0x004a6970."),
    ],
    apply: bool = typer.Option(
        False,
        "--apply",
        help="Write the best non-regressing candidate into the tree "
        "(the default previews and restores).",
    ),
    target: str = typer.Option("WIZ8", "--target"),
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Recover one function into its owning translation unit."""
    from .. import command_support as cli
    from ..recover import recover_function

    cli.run_action(
        lambda: recover_function(
            cli.settings(), address, apply=apply, target=target, program_selector=program
        )
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
