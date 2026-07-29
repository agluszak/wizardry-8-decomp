from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("class-family")
def class_family_command(
    vtable: Annotated[str, typer.Argument(help="A vtable address in the family, e.g. 005ed5bc.")],
) -> None:
    """Show every vptr write around one class, so a family reads at a glance."""
    from .. import command_support as cli
    from ..reports.class_family import class_family_report

    cli.run_action(lambda: class_family_report(cli.settings(), vtable)["rendered"])


@app.command("status")
def status_command() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .. import command_support as cli
    from ..reports.status import status_report

    cli.run_action(lambda: status_report(cli.settings()))


@app.command("context")
def context_command(
    address: Annotated[str, typer.Argument(help="Address inside the function to inspect")],
    program: str = typer.Option("wiz8", "--program"),
    deep: bool = typer.Option(False, "--deep", help="Include listing, P-code and rooted flow."),
    root: str = typer.Option("this", "--root", help="Deep-analysis parameter root."),
    discover: bool = typer.Option(
        False,
        "--discover",
        help="Create a missing function transactionally for this report, then roll it back.",
    ),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .. import command_support as cli
    from ..reports.recovery_context import recovery_context_report

    cli.run_action(
        lambda: recovery_context_report(
            cli.settings(), address, program, deep=deep, root=root, discover=discover
        )
    )
