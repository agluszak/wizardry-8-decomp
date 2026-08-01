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


@app.command("sweep")
def sweep_command(
    file: Annotated[
        str | None,
        typer.Option("--file", help="Sweep only this translation unit's functions."),
    ] = None,
    class_name: Annotated[
        str | None,
        typer.Option("--class", help="Sweep only this class's functions."),
    ] = None,
    target: str = typer.Option("WIZ8", "--target"),
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Classify zero-edit regeneration for every recovered function."""
    from .. import command_support as cli
    from ..recover import sweep

    cli.run_action(
        lambda: sweep(
            cli.settings(),
            source_file=file,
            class_name=class_name,
            target=target,
            program_selector=program,
        )
    )


@app.command("explain")
def explain_command(
    address: Annotated[
        str | None,
        typer.Argument(help="Function entry address to trace, e.g. 0x004a5f20."),
    ] = None,
    class_name: Annotated[
        str | None,
        typer.Option("--class", help="Report a class's complete lifecycle ABI family."),
    ] = None,
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Trace a function or report a class's complete lifecycle ABI family."""
    import sys

    from .. import command_support as cli
    from ..ghidra.export_cpp import explain_function

    def action() -> None:
        result = explain_function(
            cli.settings(), address, program_selector=program, class_name=class_name
        )
        sys.stdout.write(result["text"])
        sys.stdout.flush()

    cli.run_action(action)


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
