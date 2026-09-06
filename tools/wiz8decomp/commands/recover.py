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

    cli.run_action(lambda: verify_lifecycle_fixture(cli.settings()))


@app.command("function")
def function_command(
    selectors: Annotated[
        list[str],
        typer.Argument(help="Function addresses, ranges, or exact reviewed Ghidra names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Generate persistent source-aware C++ candidates for a function batch."""
    from .. import command_support as cli
    from ..recover import recover_candidates

    cli.run_action(
        lambda: recover_candidates(cli.settings(), list(selectors), program_selector=program)
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
    selectors: Annotated[
        list[str],
        typer.Argument(help="Function addresses, ranges, or exact source-owned names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Trace structured recovery facts for one function."""
    from .. import command_support as cli
    from ..ghidra.recovery import explain_functions

    def action():
        return explain_functions(cli.settings(), selectors, program_selector=program)

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
