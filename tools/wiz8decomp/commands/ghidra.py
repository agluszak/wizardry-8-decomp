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
    from ..ghidra.env import open_project
    from ..ghidra.workspace import restore_seed

    def action():
        settings = cli.settings()
        settings.project_dir.mkdir(parents=True, exist_ok=True)
        with open_project(settings, create=True) as project:
            return restore_seed(settings, project, program)

    cli.emit(action())


@app.command("sync")
def sync_command(
    program: str = typer.Option("wiz8", "--program"),
    import_source: bool = typer.Option(
        False,
        "--import-source",
        help="Also run reccmp PDB import after established source facts are applied.",
    ),
) -> None:
    """Project established source/evidence facts into the live ProgramDB."""
    from .. import command_support as cli
    from ..ghidra.sync import synchronize

    payload = synchronize(cli.settings(), program_selector=program, import_source=import_source)
    cli.emit(payload)
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


@app.command("decompile")
def decompile_command(
    selectors: Annotated[
        list[str],
        typer.Argument(help="Function addresses, ranges, or exact Ghidra names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
    as_json: bool = typer.Option(False, "--json", help="Emit the structured result as JSON."),
) -> None:
    """Decompile selected functions from native ProgramDB without compiling source."""
    from .. import command_support as cli
    from ..ghidra.inspect import decompile_functions, format_decompile_text

    payload = decompile_functions(cli.settings(), list(selectors), program_selector=program)
    cli.emit(payload, as_json=as_json, text=format_decompile_text(payload))
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


@app.command("asm")
def asm_command(
    selectors: Annotated[
        list[str],
        typer.Argument(help="Function addresses, ranges, or exact Ghidra names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
    as_json: bool = typer.Option(False, "--json", help="Emit the structured result as JSON."),
) -> None:
    """Write annotated assembly for selected functions or bounded address windows."""
    from .. import command_support as cli
    from ..ghidra.inspect import assemble_functions, format_asm_text

    payload = assemble_functions(cli.settings(), list(selectors), program_selector=program)
    cli.emit(payload, as_json=as_json, text=format_asm_text(payload))
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


@app.command("sym")
def sym_command(
    selectors: Annotated[
        list[str],
        typer.Argument(help="Addresses to resolve as functions, data, fields, or imports."),
    ],
    program: str = typer.Option("wiz8", "--program"),
    interpret: str | None = typer.Option(
        None, "--as", help="Interpret the first bytes as float, u32, i32, u16, or i16."
    ),
    as_json: bool = typer.Option(False, "--json", help="Emit the structured result as JSON."),
) -> None:
    """Resolve identity, field, import, and data facts without decompiling."""
    from .. import command_support as cli
    from ..ghidra.inspect import format_sym_text, lookup_symbols

    payload = lookup_symbols(
        cli.settings(), list(selectors), program_selector=program, interpret=interpret
    )
    cli.emit(payload, as_json=as_json, text=format_sym_text(payload))
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


@app.command("class")
def class_command(
    names: Annotated[
        list[str],
        typer.Argument(help="Reviewed Ghidra class names."),
    ],
    program: str = typer.Option("wiz8", "--program"),
    as_json: bool = typer.Option(False, "--json", help="Emit the structured result as JSON."),
) -> None:
    """Report class fields, unknown regions, base subobjects, and vtable slots."""
    from .. import command_support as cli
    from ..ghidra.env import open_program
    from ..ghidra.inspect import class_report, format_class_text

    with open_program(cli.settings(), program) as live:
        payload = class_report(live, list(names))
    cli.emit(payload, as_json=as_json, text=format_class_text(payload))


@app.command("flow")
def flow_command(
    selector: Annotated[str, typer.Argument(help="Function address or reviewed Ghidra name")],
    root: str = typer.Option(..., "--root", help="Parameter or receiver root to trace."),
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Answer one rooted field-flow question from HighFunction P-code."""
    from .. import command_support as cli
    from ..ghidra.env import open_program
    from ..ghidra.semantic import field_accesses

    def action():
        with open_program(cli.settings(), program) as live:
            return field_accesses(live, selector, root)

    cli.emit(action())


@app.command("import")
def import_command(
    program: str | None = typer.Argument(None),
    replace: bool = typer.Option(False, "--replace"),
) -> None:
    """Import and analyze a materialized binary from the configured corpus."""
    from .. import command_support as cli
    from ..ghidra.import_programs import import_programs

    cli.emit(import_programs(cli.settings(), requested_program=program, replace_existing=replace))


@seed_app.command("refresh")
def seed_refresh_command(program: str | None = typer.Argument(None)) -> None:
    """Pack an intentionally reviewed canonical project checkpoint."""
    from .. import command_support as cli
    from ..ghidra.export_programs import export_project

    cli.emit(export_project(cli.settings(), program))


@fid_app.command("status")
def fid_status_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import fid_status

    cli.emit(fid_status(cli.settings()))


@fid_app.command("inventory")
def fid_inventory_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import static_inventory

    cli.emit(static_inventory(cli.settings()))


@fid_app.command("fetch-sources")
def fid_fetch_sources_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import fetch_seed_sources

    cli.emit(fetch_seed_sources(cli.settings()))


@fid_app.command("build-image")
def fid_build_image_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import build_toolchain_images

    cli.emit(build_toolchain_images(cli.settings(), toolchain))


@fid_app.command("probe-toolchain")
def fid_probe_toolchain_command(
    toolchain: Annotated[
        list[str] | None,
        typer.Option("--toolchain", help="Pinned candidate ID; repeat to select several."),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid_seeds import probe_toolchains

    cli.emit(probe_toolchains(cli.settings(), toolchain))


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

    cli.emit(build_seed_objects(cli.settings(), toolchain, library))


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

    cli.emit(extract_precompiled_objects(cli.settings(), toolchain))


@fid_app.command("build")
def fid_build_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import build_fid

    cli.emit(build_fid(cli.settings()))


@fid_app.command("build-srs")
def fid_build_srs_command() -> None:
    from .. import command_support as cli
    from ..ghidra.fid import build_srs_fid

    cli.emit(build_srs_fid(cli.settings()))


@fid_app.command("match")
def fid_match_command(
    program: str = typer.Option(..., "--program"),
    threshold: float | None = typer.Option(None, "--threshold"),
    database: str = typer.Option("static", "--database", help="static or srs"),
) -> None:
    from .. import command_support as cli
    from ..ghidra.fid import match_fid

    cli.emit(match_fid(cli.settings(), program, threshold, database))
