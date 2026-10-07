from __future__ import annotations

import typer

app = typer.Typer(help="Refresh canonical evidence.", no_args_is_help=True)
refresh_app = typer.Typer(
    help="Refresh proprietary-input evidence snapshots.", no_args_is_help=True
)
app.add_typer(refresh_app, name="refresh")


def surrender_abi_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..surrender_abi import sweep_surrender_abi

    cli.emit(sweep_surrender_abi(cli.settings(), update_snapshot=update_snapshot))


refresh_app.command("surrender-abi")(surrender_abi_command)


