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
) -> None:
    """Optionally rebuild normal SGP objects and compare historical binaries."""
    from .. import command_support as cli
    from ..sgp_oracle import sweep_sgp_units

    cli.run_action(lambda: sweep_sgp_units(cli.settings(), unit))
