from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("class-family")
def class_family_command(
    vtable: Annotated[str, typer.Argument(help="A vtable address in the family, e.g. 005ed5bc.")],
) -> None:
    """Show every vptr write around one class, so a family reads at a glance."""
    from .. import cli
    from ..reports.class_family import class_family_report

    cli._run_action(lambda: class_family_report(cli._settings(), vtable)["rendered"])


@app.command("class-candidates")
def class_candidates_command() -> None:
    """Generate reviewable class candidates from the polymorphism snapshots."""
    from .. import cli
    from ..reports.class_candidates import class_candidates_report

    cli._run_action(lambda: class_candidates_report(cli._settings()))


@app.command("status")
def status_command() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .. import cli
    from ..reports.status import status_report

    cli._run_action(lambda: status_report(cli._settings()))


@app.command("context")
def context_command(
    address: Annotated[str, typer.Argument(help="Address inside the function to inspect")],
    program: str = typer.Option("wiz8", "--program"),
    deep: bool = typer.Option(False, "--deep", help="Include listing, P-code and rooted flow."),
    root: str = typer.Option("this", "--root", help="Deep-analysis parameter root."),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .. import cli
    from ..reports.recovery_context import recovery_context_report

    cli._run_action(
        lambda: recovery_context_report(
            cli._settings(), address, program, deep=deep, root=root
        )
    )
