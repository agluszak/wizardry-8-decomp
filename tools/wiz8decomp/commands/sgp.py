from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(help="Compile and compare pinned SGP source-oracle units.", no_args_is_help=True)


@app.command("sweep")
def sweep_command(
    unit: Annotated[
        list[str] | None,
        typer.Option("--unit", help="Configured SGP unit ID; repeat to select several."),
    ] = None,
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot; requires a complete sweep.",
    ),
) -> None:
    """Compile the declarative flag matrix and compare every reviewed build."""
    from .. import cli
    from ..sgp_oracle import sweep_sgp_units

    cli._run_action(lambda: sweep_sgp_units(cli._settings(), unit, update_snapshot=update_snapshot))
