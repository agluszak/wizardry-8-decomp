from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(
    help="Scan, extract, and materialize the local corpus.",
    no_args_is_help=True,
)


@app.command("scan")
def scan_command() -> None:
    """Scan recursively by signatures and bind only explicitly configured roles."""
    from .. import command_support as cli
    from ..inputs.scan import scan_inputs

    cli.emit(scan_inputs(cli.settings()).model_dump(mode="json", by_alias=True))


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
        cli.emit(extract_all(cli.settings()))
    else:
        cli.emit([extract_role(cli.settings(), role) for role in requested])


@app.command("materialize")
def materialize_command() -> None:
    from .. import command_support as cli
    from ..extract.variants import materialize_variants

    cli.emit(materialize_variants(cli.settings()))


