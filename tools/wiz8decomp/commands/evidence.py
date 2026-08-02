from __future__ import annotations

import typer

app = typer.Typer(help="Validate and update canonical evidence.", no_args_is_help=True)
refresh_app = typer.Typer(
    help="Refresh proprietary-input evidence snapshots.", no_args_is_help=True
)
app.add_typer(refresh_app, name="refresh")


def debug_artifacts_command(
    update_snapshot: bool = typer.Option(False, "--update-snapshot"),
    archive_password: str | None = typer.Option(
        None, "--archive-password", envvar="WIZ8_DEBUG_ARCHIVE_PASSWORD"
    ),
) -> None:
    from .. import command_support as cli
    from ..debug_artifacts import sweep_debug_artifacts

    cli.run_action(
        lambda: sweep_debug_artifacts(
            cli.settings(), update_snapshot=update_snapshot, archive_password=archive_password
        )
    )


def surrender_abi_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..surrender_abi import sweep_surrender_abi

    cli.run_action(lambda: sweep_surrender_abi(cli.settings(), update_snapshot=update_snapshot))


refresh_app.command("debug-artifacts")(debug_artifacts_command)
refresh_app.command("surrender-abi")(surrender_abi_command)


@app.command("validate")
def validate_command(program: str = typer.Option("wiz8", "--program")) -> None:
    """Validate canonical schemas, identities, references, and image observations."""
    from .. import command_support as cli
    from ..evidence.validate import require_valid_repository

    cli.run_action(lambda: require_valid_repository(cli.settings().repo_dir, program))


@app.command("validate-ghidra")
def validate_ghidra_command(program: str = typer.Option("wiz8", "--program")) -> None:
    """Resolve every provenance claim against focused live Ghidra queries."""
    from .. import command_support as cli
    from ..evidence.claims import validate_claims_against_ghidra

    cli.run_action(lambda: validate_claims_against_ghidra(cli.settings(), program))
