from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(
    help="Restore, inspect, and export the canonical Ghidra project.",
    no_args_is_help=True,
)
seed_app = typer.Typer(help="Refresh the validated canonical GZF seed.", no_args_is_help=True)
fid_app = typer.Typer(
    help="Build and query project-owned Function ID databases.", no_args_is_help=True
)
app.add_typer(seed_app, name="seed")
app.add_typer(fid_app, name="fid", hidden=True)


@app.command("restore")
def restore_command(program: str = "wiz8") -> None:
    """Restore the tracked reviewed checkpoint if the local project is absent."""
    from .. import command_support as cli
    from ..ghidra.workspace import restore_seed

    cli.run_action(lambda: restore_seed(cli.settings(), program))


@app.command("import")
def import_command(
    program: str | None = typer.Argument(None),
    replace: bool = typer.Option(False, "--replace"),
) -> None:
    """Import and analyze a materialized binary from the configured corpus."""
    from .. import command_support as cli
    from ..ghidra.import_programs import import_programs

    cli.run_action(
        lambda: import_programs(cli.settings(), requested_program=program, replace_existing=replace)
    )


@app.command("index")
def index_command(program: str = "wiz8") -> None:
    """Export disposable normalized functions, types, and vtables."""
    from .. import command_support as cli
    from ..ghidra.index import export_index

    cli.run_action(lambda: export_index(cli.settings(), program))


@app.command("sync-source")
def sync_source_command(
    program: str = "wiz8",
    apply: bool = typer.Option(False, "--apply", help="Write and save source-owned names."),
) -> None:
    """Check or apply address-marked source names to Ghidra."""
    from .. import command_support as cli
    from ..ghidra.source_sync import sync_source_names

    cli.run_action(lambda: sync_source_names(cli.settings(), program, apply=apply))


@seed_app.command("refresh")
def seed_refresh_command(program: str | None = typer.Argument(None)) -> None:
    """Pack an intentionally reviewed canonical project checkpoint."""
    from .. import command_support as cli
    from ..ghidra.export_programs import export_project

    cli.run_action(lambda: export_project(cli.settings(), program))


@fid_app.command("status")
def fid_status_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import fid_status

    cli.run_action(lambda: fid_status(cli.settings()))


@fid_app.command("inventory")
def fid_inventory_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import static_inventory

    cli.run_action(lambda: static_inventory(cli.settings()))


@fid_app.command("fetch-sources")
def fid_fetch_sources_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import fetch_seed_sources

    cli.run_action(lambda: fetch_seed_sources(cli.settings()))


@fid_app.command("build-image")
def fid_build_image_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import build_toolchain_images

    cli.run_action(lambda: build_toolchain_images(cli.settings(), toolchain))


@fid_app.command("probe-toolchain")
def fid_probe_toolchain_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import probe_toolchains

    cli.run_action(lambda: probe_toolchains(cli.settings(), toolchain))


@fid_app.command("build-seeds")
def fid_build_seeds_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
    library: Annotated[
        list[str] | None,
        typer.Option("--library", help="Static-library ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import build_seed_objects

    cli.run_action(lambda: build_seed_objects(cli.settings(), toolchain, library))


@fid_app.command("extract-libraries")
def fid_extract_libraries_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option(
            "--toolchain",
            help="Pinned precompiled-library snapshot ID; repeat to select several.",
        ),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import extract_precompiled_objects

    cli.run_action(lambda: extract_precompiled_objects(cli.settings(), toolchain))


@fid_app.command("build")
def fid_build_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import build_fid

    cli.run_action(lambda: build_fid(cli.settings()))


@fid_app.command("build-srs")
def fid_build_srs_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import build_srs_fid

    cli.run_action(lambda: build_srs_fid(cli.settings()))


@fid_app.command("match")
def fid_match_command(
    program: str = typer.Option(..., "--program"),
    threshold: float | None = typer.Option(None, "--threshold"),
    database: str = typer.Option("static", "--database", help="static or srs"),
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid import match_fid

    cli.run_action(lambda: match_fid(cli.settings(), program, threshold, database))
