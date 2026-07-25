from __future__ import annotations

import importlib.metadata
import json
import logging
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Annotated, Any, Optional

import typer
from rich.console import Console
from rich.json import JSON

from .config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    ghidra_version,
    load_settings,
)
from .pipeline import PipelineStage
from .subprocesses import tool_version

app = typer.Typer(
    help="Wizardry 8 reproducible decompilation bootstrap CLI.",
    no_args_is_help=True,
    add_completion=False,
)
inputs_app = typer.Typer(help="Discover and inspect immutable local inputs.", no_args_is_help=True)
extract_app = typer.Typer(
    help="Extract configured inputs without modifying them.", no_args_is_help=True
)
variants_app = typer.Typer(
    help="Materialize and compare independent build variants.", no_args_is_help=True
)
ghidra_app = typer.Typer(
    help="Own Ghidra projects, queries, daemon, and exports.", no_args_is_help=True
)
daemon_app = typer.Typer(help="Manage the persistent read-only query daemon.", no_args_is_help=True)
fid_app = typer.Typer(
    help="Build and query project-owned Function ID databases.", no_args_is_help=True
)
report_app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)
pipeline_app = typer.Typer(help="Verify and clean generated pipeline stages.", no_args_is_help=True)
app.add_typer(inputs_app, name="inputs")
app.add_typer(extract_app, name="extract")
app.add_typer(variants_app, name="variants")
app.add_typer(ghidra_app, name="ghidra")
ghidra_app.add_typer(daemon_app, name="daemon")
ghidra_app.add_typer(fid_app, name="fid")
app.add_typer(report_app, name="report")
app.add_typer(pipeline_app, name="pipeline")
console = Console()
_JSON_OUTPUT = False


class CliState:
    verbose = False
    json_output = False


@app.callback()
def main(
    ctx: typer.Context,
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable debug logging."),
    json_output: bool = typer.Option(False, "--json", help="Render command results as JSON."),
) -> None:
    global _JSON_OUTPUT
    state = CliState()
    state.verbose = verbose
    state.json_output = json_output
    _JSON_OUTPUT = json_output
    ctx.obj = state
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )


def _settings():
    try:
        settings = load_settings()
        assert settings is not None
        return settings
    except Exception as error:
        raise typer.BadParameter(str(error)) from error


def _emit(value: Any, *, force_json: bool = False) -> None:
    if _JSON_OUTPUT or force_json:
        sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")
    elif isinstance(value, (dict, list)):
        console.print(JSON.from_data(value))
    else:
        console.print(value)


def _run_action(action: Any, *, force_json: bool = False) -> None:
    try:
        _emit(action(), force_json=force_json)
    except Exception as error:
        if logging.getLogger().isEnabledFor(logging.DEBUG):
            logging.exception("command failed")
        console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error


def _tracked_copyrighted(settings: Any) -> list[str]:
    if not (settings.repo_dir / ".git").exists():
        return []
    completed = subprocess.run(
        ["git", "ls-files", "-z"], cwd=settings.repo_dir, capture_output=True, check=False
    )
    suspect_suffixes = {
        ".exe",
        ".dll",
        ".iso",
        ".zip",
        ".7z",
        ".rar",
        ".cab",
        ".bik",
        ".wav",
        ".mp3",
        ".slf",
        ".asi",
        ".m3d",
    }
    suspects = []
    for raw in completed.stdout.split(b"\0"):
        if not raw:
            continue
        relative = raw.decode("utf-8", errors="replace")
        path = settings.repo_dir / relative
        if path.suffix.casefold() in suspect_suffixes and not relative.startswith(
            "vendor/ghidra/exports/"
        ):
            suspects.append(relative)
    return sorted(suspects)


@app.command()
def doctor() -> None:
    """Validate paths, pinned Ghidra/PyGhidra, extractors, and repository safety."""

    def action() -> dict[str, Any]:
        settings = _settings()
        checks = []
        version, release = ghidra_version(settings.ghidra_install_dir)
        checks.append(
            {
                "name": "ghidra",
                "ok": version == REQUIRED_GHIDRA_VERSION and release == REQUIRED_GHIDRA_RELEASE,
                "expected": f"{REQUIRED_GHIDRA_VERSION} {REQUIRED_GHIDRA_RELEASE}",
                "actual": f"{version} {release}",
                "path": str(settings.ghidra_install_dir),
            }
        )
        try:
            pyghidra_version = importlib.metadata.version("pyghidra")
        except importlib.metadata.PackageNotFoundError:
            pyghidra_version = None
        checks.append(
            {
                "name": "pyghidra",
                "ok": pyghidra_version == REQUIRED_PYGHIDRA_VERSION,
                "expected": REQUIRED_PYGHIDRA_VERSION,
                "actual": pyghidra_version,
            }
        )
        checks.append(
            {
                "name": "input-directory",
                "ok": settings.input_dir.is_dir() and os.access(settings.input_dir, os.R_OK),
                "path": str(settings.input_dir),
            }
        )
        settings.work_dir.mkdir(parents=True, exist_ok=True)
        try:
            with tempfile.NamedTemporaryFile(prefix="wiz8-doctor-", dir=settings.work_dir):
                work_writable = True
        except OSError:
            work_writable = False
        checks.append(
            {"name": "work-directory", "ok": work_writable, "path": str(settings.work_dir)}
        )
        required = {"7z": ["--help"], "innoextract": ["--version"], "cabextract": ["--version"]}
        optional = {"unshield": ["-V"], "wine": ["--version"], "git-lfs": ["version"]}
        for name, args in required.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": bool(info["executable"]), **info, "required": True})
        for name, args in optional.items():
            info = tool_version(name, args)
            checks.append({"name": name, "ok": True, **info, "required": False})
        suspects = _tracked_copyrighted(settings)
        checks.append({"name": "tracked-copyrighted-files", "ok": not suspects, "files": suspects})
        failed = [item["name"] for item in checks if not item["ok"]]
        result = {"ok": not failed, "checks": checks, "failures": failed}
        if failed:
            raise RuntimeError("doctor failed: " + ", ".join(failed))
        return result

    _run_action(action)


@inputs_app.command("scan")
def inputs_scan() -> None:
    """Scan recursively by signatures and bind only explicitly configured roles."""
    from .inputs.scan import scan_inputs

    _run_action(lambda: scan_inputs(_settings()).model_dump(mode="json", by_alias=True))


@inputs_app.command("show")
def inputs_show() -> None:
    from .inputs.scan import load_manifest

    _run_action(lambda: load_manifest(_settings()).model_dump(mode="json", by_alias=True))


def _extract(role: str) -> None:
    from .extract.variants import extract_role

    _run_action(lambda: extract_role(_settings(), role))


@extract_app.command("all")
def extract_all_command() -> None:
    from .extract.variants import extract_all

    _run_action(lambda: extract_all(_settings()))


@extract_app.command("gog")
def extract_gog() -> None:
    _extract("gog-media")


@extract_app.command("demo")
def extract_demo() -> None:
    _extract("demo")


@extract_app.command("patch-1261")
def extract_patch_1261() -> None:
    _extract("patch-1261")


@extract_app.command("patch-128")
def extract_patch_128() -> None:
    _extract("patch-128")


@extract_app.command("official-patch")
def extract_official_patch() -> None:
    _extract("official-2001-12-23-patch")


@extract_app.command("compatibility-fix")
def extract_compatibility_fix() -> None:
    _extract("compatibility-fix")


@variants_app.command("materialize")
def variants_materialize() -> None:
    from .extract.variants import materialize_variants

    _run_action(lambda: materialize_variants(_settings()))


@variants_app.command("diff")
def variants_diff() -> None:
    from .extract.variants import variant_diff

    _run_action(lambda: variant_diff(_settings()))


@pipeline_app.command("verify")
def pipeline_verify() -> None:
    """Rehash generated trees and verify every complete stage recipe."""
    from .pipeline import verify_pipeline

    result = verify_pipeline(_settings())
    _emit(result)
    if not result["ok"]:
        raise typer.Exit(1)


@pipeline_app.command("clean")
def pipeline_clean(
    stage: Annotated[PipelineStage, typer.Option("--stage", help="Generated stage to remove.")],
) -> None:
    """Remove one explicit generated stage and its downstream derived evidence."""
    from .pipeline import clean_pipeline

    _run_action(lambda: clean_pipeline(_settings(), stage))


@app.command("inventory")
def inventory_command(json_output: bool = typer.Option(False, "--json", help="Emit JSON.")) -> None:
    from .binary.inventory import inventory

    _run_action(lambda: inventory(_settings()), force_json=json_output)


@ghidra_app.command("import")
def ghidra_import(
    all_programs: bool = typer.Option(
        False, "--all", help="Import every configured analysis target."
    ),
    variant: Optional[str] = typer.Option(None, "--variant"),
    program: Optional[str] = typer.Option(None, "--program"),
) -> None:
    from .ghidra.import_programs import import_programs

    _run_action(
        lambda: import_programs(
            _settings(), all_modules=all_programs, variant=variant, requested_program=program
        )
    )


@ghidra_app.command("apply-functions")
def ghidra_apply_functions(
    program: str,
    mapping: Annotated[Path, typer.Option("--map", exists=True, dir_okay=False, readable=True)],
    dry_run: Annotated[
        bool,
        typer.Option("--dry-run", help="Report changes without writing the Ghidra program."),
    ] = False,
) -> None:
    """Create and name functions from a reviewed, evidence-backed CSV map."""
    from .ghidra.apply_function_map import apply_function_map

    _run_action(lambda: apply_function_map(_settings(), program, mapping, dry_run=dry_run))


@ghidra_app.command("apply-unzip-model")
def ghidra_apply_unzip_model(
    program: Annotated[str, typer.Argument()] = "srEXT_Unzip.dll",
) -> None:
    """Apply the reviewed srEXT_Unzip object, callback, and vtable types."""
    from .ghidra.apply_unzip_model import apply_unzip_model

    _run_action(lambda: apply_unzip_model(_settings(), program))


@daemon_app.command("start")
def daemon_start(program: Optional[str] = typer.Option(None, "--program")) -> None:
    from .ghidra.query_daemon import start_daemon

    _run_action(lambda: start_daemon(_settings(), program))


@daemon_app.command("status")
def daemon_status_command() -> None:
    from .ghidra.query_daemon import daemon_status

    _run_action(lambda: daemon_status(_settings()))


@daemon_app.command("stop")
def daemon_stop() -> None:
    from .ghidra.query_daemon import stop_daemon

    _run_action(lambda: stop_daemon(_settings()))


@ghidra_app.command(
    "query", context_settings={"allow_extra_args": True, "ignore_unknown_options": True}
)
def ghidra_query(ctx: typer.Context, program: str, command: str) -> None:
    """Run PROGRAM COMMAND [ARGS...] through daemon or one-shot PyGhidra."""
    from .ghidra.query_daemon import query

    def action() -> dict[str, Any]:
        result, transport = query(_settings(), program, command, list(ctx.args))
        return {"transport": transport, "program": program, "command": command, "result": result}

    _run_action(action)


@ghidra_app.command("export-evidence")
def ghidra_export_evidence(
    program: Optional[str] = typer.Option(None, "--program"),
    address: Optional[str] = typer.Option(None, "--address"),
    all_functions: bool = typer.Option(False, "--all"),
) -> None:
    from .ghidra.export_evidence import export_evidence

    _run_action(
        lambda: export_evidence(
            _settings(), selector=program, address=address, export_all=all_functions
        )
    )


@ghidra_app.command("cross-build")
def ghidra_cross_build() -> None:
    from .ghidra.export_evidence import cross_build_candidates

    _run_action(lambda: cross_build_candidates(_settings()))


@ghidra_app.command("export-project")
def ghidra_export_project() -> None:
    from .ghidra.export_programs import export_project

    _run_action(lambda: export_project(_settings()))


@fid_app.command("status")
def ghidra_fid_status() -> None:
    from .ghidra.fid import fid_status

    _run_action(lambda: fid_status(_settings()))


@fid_app.command("inventory")
def ghidra_fid_inventory() -> None:
    """Report binary-confirmed static libraries and seed readiness."""
    from .ghidra.fid_seeds import static_inventory

    _run_action(lambda: static_inventory(_settings()))


@fid_app.command("fetch-sources")
def ghidra_fid_fetch_sources() -> None:
    """Fetch and hash-check pinned open-source library sources."""
    from .ghidra.fid_seeds import fetch_seed_sources

    _run_action(lambda: fetch_seed_sources(_settings()))


@fid_app.command("build-image")
def ghidra_fid_build_image(
    toolchain: Optional[list[str]] = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
) -> None:
    """Build pinned MSVC600/Wine seed compiler images."""
    from .ghidra.fid_seeds import build_toolchain_images

    _run_action(lambda: build_toolchain_images(_settings(), toolchain))


@fid_app.command("probe-toolchain")
def ghidra_fid_probe_toolchain(
    toolchain: Optional[list[str]] = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
) -> None:
    """Compile a probe and export its Rich records for toolchain comparison."""
    from .ghidra.fid_seeds import probe_toolchains

    _run_action(lambda: probe_toolchains(_settings(), toolchain))


@fid_app.command("build-seeds")
def ghidra_fid_build_seeds(
    toolchain: Optional[list[str]] = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
    library: Optional[list[str]] = typer.Option(
        None, "--library", help="Static-library ID; repeat to select several."
    ),
) -> None:
    """Build CMake-defined static-library object seeds with pinned toolchains."""
    from .ghidra.fid_seeds import build_seed_objects

    _run_action(lambda: build_seed_objects(_settings(), toolchain, library))


@fid_app.command("extract-libraries")
def ghidra_fid_extract_libraries(
    toolchain: Optional[list[str]] = typer.Option(
        None,
        "--toolchain",
        help="Pinned precompiled-library snapshot ID; repeat to select several.",
    ),
) -> None:
    """Extract exact COFF objects from pinned VC6 library snapshots."""
    from .ghidra.fid_seeds import extract_precompiled_objects

    _run_action(lambda: extract_precompiled_objects(_settings(), toolchain))


@fid_app.command("build")
def ghidra_fid_build() -> None:
    """Build the primary static-library FID database."""
    from .ghidra.fid import build_fid

    _run_action(lambda: build_fid(_settings()))


@fid_app.command("build-srs")
def ghidra_fid_build_srs() -> None:
    """Build the separate symbol-rich SurRender oracle database."""
    from .ghidra.fid import build_srs_fid

    _run_action(lambda: build_srs_fid(_settings()))


@fid_app.command("match")
def ghidra_fid_match(
    program: str = typer.Option(..., "--program"),
    threshold: Optional[float] = typer.Option(None, "--threshold"),
    database: str = typer.Option("static", "--database", help="static or srs"),
) -> None:
    from .ghidra.fid import match_fid

    _run_action(lambda: match_fid(_settings(), program, threshold, database))


@ghidra_app.command("restore-project")
def ghidra_restore_project() -> None:
    from .ghidra.export_programs import restore_project

    _run_action(lambda: restore_project(_settings()))


@report_app.command("bootstrap")
def report_bootstrap() -> None:
    from .reports.bootstrap import bootstrap_report

    _run_action(lambda: bootstrap_report(_settings()))
