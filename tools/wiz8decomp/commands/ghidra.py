from __future__ import annotations

import shlex
from typing import Annotated, Any

import typer

app = typer.Typer(
    help="Query and deliberately rebuild the canonical Ghidra project.",
    no_args_is_help=True,
)
seed_app = typer.Typer(help="Refresh the validated canonical GZF seed.", no_args_is_help=True)
fid_app = typer.Typer(
    help="Build and query project-owned Function ID databases.", no_args_is_help=True
)
app.add_typer(seed_app, name="seed")
app.add_typer(fid_app, name="fid", hidden=True)


@app.command("rebuild")
def rebuild_command(program: str) -> None:
    """Fresh-import one program, replay reviewed knowledge, and validate it."""
    from .. import cli
    from ..ghidra.rebuild import rebuild_program

    cli._run_action(lambda: rebuild_program(cli._settings(), program))


@seed_app.command("refresh")
def seed_refresh_command(program: str | None = typer.Argument(None)) -> None:
    """Validate and pack the canonical project as the shared GZF seed."""
    from .. import cli
    from ..ghidra.export_programs import export_project

    cli._run_action(lambda: export_project(cli._settings(), program))


@app.command("query", context_settings={"allow_extra_args": True, "ignore_unknown_options": True})
def query_command(
    ctx: typer.Context,
    program: str,
    command: Annotated[str | None, typer.Argument()] = None,
    queries: Annotated[
        list[str] | None,
        typer.Option(
            "--query",
            "-q",
            help="Complete query to run; repeat to share one project open.",
        ),
    ] = None,
) -> None:
    """Run one read-only query batch against the existing canonical project."""
    from .. import cli
    from ..ghidra.query_daemon import query, query_many

    def action() -> dict[str, Any]:
        if queries:
            if command is not None or ctx.args:
                raise ValueError("use either COMMAND [ARGUMENTS] or repeated --query clauses")
            parsed = [shlex.split(specification) for specification in queries]
            if any(not fields for fields in parsed):
                raise ValueError("--query clauses must not be empty")
            requests = [(fields[0], fields[1:]) for fields in parsed]
            results, transport = query_many(cli._settings(), program, requests)
            return {"transport": transport, "program": program, "results": results}
        if command is None:
            raise ValueError("provide COMMAND [ARGUMENTS] or at least one --query clause")
        result, transport = query(cli._settings(), program, command, list(ctx.args))
        return {"transport": transport, "program": program, "command": command, "result": result}

    cli._run_action(action, force_json=True)


@fid_app.command("status")
def fid_status_command() -> None:
    from .. import cli
    from ..ghidra.fid import fid_status

    cli._run_action(lambda: fid_status(cli._settings()))


@fid_app.command("inventory")
def fid_inventory_command() -> None:
    from .. import cli
    from ..ghidra.fid_seeds import static_inventory

    cli._run_action(lambda: static_inventory(cli._settings()))


@fid_app.command("fetch-sources")
def fid_fetch_sources_command() -> None:
    from .. import cli
    from ..ghidra.fid_seeds import fetch_seed_sources

    cli._run_action(lambda: fetch_seed_sources(cli._settings()))


@fid_app.command("build-image")
def fid_build_image_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import cli
    from ..ghidra.fid_seeds import build_toolchain_images

    cli._run_action(lambda: build_toolchain_images(cli._settings(), toolchain))


@fid_app.command("probe-toolchain")
def fid_probe_toolchain_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import cli
    from ..ghidra.fid_seeds import probe_toolchains

    cli._run_action(lambda: probe_toolchains(cli._settings(), toolchain))


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
    from .. import cli
    from ..ghidra.fid_seeds import build_seed_objects

    cli._run_action(lambda: build_seed_objects(cli._settings(), toolchain, library))


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
    from .. import cli
    from ..ghidra.fid_seeds import extract_precompiled_objects

    cli._run_action(lambda: extract_precompiled_objects(cli._settings(), toolchain))


@fid_app.command("build")
def fid_build_command() -> None:
    from .. import cli
    from ..ghidra.fid import build_fid

    cli._run_action(lambda: build_fid(cli._settings()))


@fid_app.command("build-srs")
def fid_build_srs_command() -> None:
    from .. import cli
    from ..ghidra.fid import build_srs_fid

    cli._run_action(lambda: build_srs_fid(cli._settings()))


@fid_app.command("match")
def fid_match_command(
    program: str = typer.Option(..., "--program"),
    threshold: float | None = typer.Option(None, "--threshold"),
    database: str = typer.Option("static", "--database", help="static or srs"),
) -> None:
    from .. import cli
    from ..ghidra.fid import match_fid

    cli._run_action(lambda: match_fid(cli._settings(), program, threshold, database))
