from __future__ import annotations

from typing import Annotated

import typer

from ..pipeline import PipelineStage

app = typer.Typer(
    help="Scan, extract, materialize, verify, and clean the local corpus.",
    no_args_is_help=True,
)


@app.command("scan")
def scan_command() -> None:
    """Scan recursively by signatures and bind only explicitly configured roles."""
    from .. import command_support as cli
    from ..inputs.scan import scan_inputs

    cli.run_action(lambda: scan_inputs(cli.settings()).model_dump(mode="json", by_alias=True))


@app.command("extract")
def extract_command(
    roles: Annotated[
        list[str] | None, typer.Argument(help="Configured input roles to extract.")
    ] = None,
    all_roles: Annotated[
        bool, typer.Option("--all", help="Extract every configured input role.")
    ] = False,
) -> None:
    """Extract one or more configured roles without modifying the inputs."""
    from .. import command_support as cli
    from ..extract.variants import extract_all, extract_role

    requested = roles or []
    if all_roles and requested:
        raise typer.BadParameter("use ROLE... or --all, not both")
    if not all_roles and not requested:
        raise typer.BadParameter("provide at least one ROLE or --all")
    if all_roles:
        cli.run_action(lambda: extract_all(cli.settings()))
    else:
        cli.run_action(lambda: [extract_role(cli.settings(), role) for role in requested])


@app.command("materialize")
def materialize_command() -> None:
    from .. import command_support as cli
    from ..extract.variants import materialize_variants

    cli.run_action(lambda: materialize_variants(cli.settings()))


@app.command("verify")
def verify_command() -> None:
    """Rehash generated trees and verify every complete stage recipe."""
    from .. import command_support as cli
    from ..pipeline import verify_pipeline

    result = verify_pipeline(cli.settings())
    cli.emit(cli.summary(result, label="corpus verification"))
    if not result["ok"]:
        raise typer.Exit(1)


@app.command("clean")
def clean_command(
    stage: Annotated[PipelineStage, typer.Option("--stage", help="Generated stage to remove.")],
) -> None:
    """Remove one explicit generated stage and its downstream derived evidence."""
    from .. import command_support as cli
    from ..pipeline import clean_pipeline

    cli.run_action(lambda: clean_pipeline(cli.settings(), stage))
