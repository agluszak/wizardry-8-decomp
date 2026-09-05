from __future__ import annotations

from typing import Annotated

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

    cli.run_action(lambda: cli.summary(verify_lifecycle_fixture(cli.settings())))


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
    json_output: bool = typer.Option(False, "--json", help="Emit complete structured details."),
) -> None:
    """Recover one function into its owning translation unit."""
    from .. import command_support as cli
    from ..recover import recover_function

    cli.run_action(
        lambda: cli.summary(
            recover_function(
                cli.settings(), address, apply=apply, target=target, program_selector=program
            )
        ),
        force_json=json_output,
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
        lambda: cli.summary(
            sweep(
                cli.settings(),
                source_file=file,
                class_name=class_name,
                target=target,
                program_selector=program,
            )
        )
    )


@app.command("explain")
def explain_command(
    selectors: Annotated[
        list[str],
        typer.Argument(help="Function addresses, ranges, or exact source-owned names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
    json_output: bool = typer.Option(False, "--json", help="Emit complete structured details."),
) -> None:
    """Trace structured recovery facts for one function."""
    from .. import command_support as cli
    from ..ghidra.recovery import explain_functions

    def action():
        result = explain_functions(cli.settings(), selectors, program_selector=program)
        return cli.human(result["text"], result)

    cli.run_action(action, force_json=json_output)


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
        lambda: cli.summary(
            regress(cli.settings(), list(addresses), target=target, program_selector=program)
        )
    )
