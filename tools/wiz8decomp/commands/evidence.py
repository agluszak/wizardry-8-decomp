from __future__ import annotations

import json
from pathlib import Path
from typing import Annotated

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


def eh_metadata_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..eh_metadata import sweep_eh_metadata

    cli.run_action(lambda: sweep_eh_metadata(cli.settings(), update_snapshot=update_snapshot))


def surrender_abi_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..surrender_abi import sweep_surrender_abi

    cli.run_action(lambda: sweep_surrender_abi(cli.settings(), update_snapshot=update_snapshot))


def call_sites_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..call_sites import sweep_call_sites

    cli.run_action(lambda: sweep_call_sites(cli.settings(), update_snapshot=update_snapshot))


def polymorphism_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..polymorphism import sweep_polymorphism

    cli.run_action(lambda: sweep_polymorphism(cli.settings(), update_snapshot=update_snapshot))


def globals_command(update_snapshot: bool = typer.Option(False, "--update-snapshot")) -> None:
    from .. import command_support as cli
    from ..data_globals import sweep_globals

    cli.run_action(lambda: sweep_globals(cli.settings(), update_snapshot=update_snapshot))


def function_census_command(
    update_snapshot: bool = typer.Option(False, "--update-snapshot"),
) -> None:
    from .. import command_support as cli
    from ..function_census import sweep_function_census

    cli.run_action(lambda: sweep_function_census(cli.settings(), update_snapshot=update_snapshot))


refresh_app.command("debug-artifacts")(debug_artifacts_command)
refresh_app.command("eh-metadata")(eh_metadata_command)
refresh_app.command("surrender-abi")(surrender_abi_command)
refresh_app.command("call-sites")(call_sites_command)
refresh_app.command("polymorphism")(polymorphism_command)
refresh_app.command("globals")(globals_command)
refresh_app.command("function-census")(function_census_command)


def register_root(root: typer.Typer) -> None:
    root.command("resolve-evidence-conflict", hidden=True)(resolve_conflict_command)


def resolve_conflict_command(
    paths: Annotated[list[Path], typer.Argument(help="Conflicted evidence CSVs to resolve.")],
) -> None:
    from .. import command_support as cli
    from ..evidence_merge import resolve_evidence_conflict

    cli.run_action(lambda: [resolve_evidence_conflict(path) for path in paths])


@app.command("validate")
def validate_command(program: str = typer.Option("wiz8", "--program")) -> None:
    """Validate canonical schemas, identities, references, and image observations."""
    from .. import command_support as cli
    from ..evidence.validate import require_valid_repository

    cli.run_action(lambda: require_valid_repository(cli.settings().repo_dir, program))


@app.command("upsert")
def upsert_command(
    path: Annotated[Path, typer.Argument(help="Canonical evidence CSV to update.")],
    row_json: Annotated[
        str | None,
        typer.Option("--row-json", help="One complete evidence row encoded as JSON."),
    ] = None,
    row_file: Annotated[
        Path | None,
        typer.Option("--row-file", help="Read one complete JSON row from a temporary file."),
    ] = None,
    fields: Annotated[
        list[str] | None,
        typer.Option("--field", help="Complete key=value field; repeat for every column."),
    ] = None,
) -> None:
    """Atomically insert or monotonically enrich one canonical evidence row."""
    from .. import command_support as cli
    from ..evidence.io import upsert_row

    def action() -> dict[str, object]:
        selected = sum((row_json is not None, row_file is not None, bool(fields)))
        if selected != 1:
            raise ValueError("pass exactly one of --row-json, --row-file, or repeated --field")
        if row_file is not None:
            decoded: object = json.loads(row_file.read_text(encoding="utf-8"))
        elif row_json is not None:
            decoded = json.loads(row_json)
        else:
            decoded = {}
            for item in fields or []:
                key, separator, value = item.partition("=")
                if not separator or not key:
                    raise ValueError(f"--field must be key=value: {item!r}")
                if key in decoded:
                    raise ValueError(f"--field repeats key: {key}")
                decoded[key] = value
        if not isinstance(decoded, dict) or not all(isinstance(key, str) for key in decoded):
            raise ValueError("evidence row must be one JSON object with string keys")
        return upsert_row(path, {key: str(value) for key, value in decoded.items()})

    cli.run_action(action)
