from __future__ import annotations

import importlib.metadata
import json
import logging
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Annotated, Any

import typer
from rich.console import Console
from rich.json import JSON
from rich.markup import escape

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
    help="Own Ghidra projects, queries, daemon, and validated seeds.", no_args_is_help=True
)
daemon_app = typer.Typer(help="Manage the persistent read-only query daemon.", no_args_is_help=True)
cache_app = typer.Typer(
    help="Build and materialize the validated canonical GZF seed.", no_args_is_help=True
)
fid_app = typer.Typer(
    help="Build and query project-owned Function ID databases.", no_args_is_help=True
)
sgp_app = typer.Typer(
    help="Compile and compare pinned SGP source-oracle units.", no_args_is_help=True
)
report_app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)
pipeline_app = typer.Typer(help="Verify and clean generated pipeline stages.", no_args_is_help=True)
app.add_typer(inputs_app, name="inputs")
app.add_typer(extract_app, name="extract")
app.add_typer(variants_app, name="variants")
app.add_typer(ghidra_app, name="ghidra")
ghidra_app.add_typer(daemon_app, name="daemon")
ghidra_app.add_typer(cache_app, name="cache")
ghidra_app.add_typer(fid_app, name="fid")
app.add_typer(report_app, name="report")
app.add_typer(pipeline_app, name="pipeline")
app.add_typer(sgp_app, name="sgp")
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


@app.command("unresolved-report")
def unresolved_report_command(
    objects: Annotated[
        Path | None,
        typer.Option(help="Object root; defaults to the configured decomp build."),
    ] = None,
    link_map: Annotated[
        Path | None,
        typer.Option(help="Linker MAP; defaults to the bring-up image's."),
    ] = None,
) -> None:
    """Report first-party symbols the recovered image still cannot resolve.

    /FORCE:UNRESOLVED is what lets the bring-up image link at all, and the cost
    is that the gap stops being visible: the linker names each missing symbol
    once, in output nobody keeps. This recomputes it from the objects.
    """

    def action() -> dict[str, Any]:
        from .unresolved import unresolved_report

        settings = _settings()
        build = settings.repo_dir / "build" / "decomp"
        return unresolved_report(
            objects or (build / "CMakeFiles"),
            link_map or (build / "Wiz8.map"),
        )

    _run_action(action)


@app.command("check-build-dir")
def check_build_dir(
    build_dir: Annotated[
        Path | None,
        typer.Option(help="CMake build directory; defaults to the configured decomp build."),
    ] = None,
) -> None:
    """Refuse a build directory configured by a different checkout."""

    def action() -> dict[str, Any]:
        from .build_dir import check_build_directory

        settings = _settings()
        target = build_dir or (settings.repo_dir / "build" / "decomp")
        return check_build_directory(target, settings.repo_dir)

    _run_action(action)


def _reccmp_original(target: str) -> Path | None:
    """Where reccmp was told the original binary lives, if it was told."""

    import yaml

    user_config = _settings().repo_dir / "reccmp-user.yml"
    if not user_config.is_file():
        return None
    configured = yaml.safe_load(user_config.read_text(encoding="utf-8")) or {}
    path = (configured.get("targets") or {}).get(target, {}).get("path")
    if not path:
        return None
    resolved = Path(str(path).strip())
    return resolved if resolved.is_file() else None


@app.command("resolve-evidence-conflict")
def resolve_evidence_conflict_command(
    paths: Annotated[list[Path], typer.Argument(help="Conflicted evidence CSVs to resolve.")],
) -> None:
    """Merge a conflicted evidence CSV, keeping the stronger row per identity."""

    def action() -> list[dict[str, Any]]:
        from .evidence_merge import resolve_evidence_conflict

        return [resolve_evidence_conflict(path) for path in paths]

    _run_action(action)


@app.command("check-markers")
def check_markers_command(
    paths: Annotated[
        list[Path] | None,
        typer.Option(help="Files or directories to scan; defaults to src and include."),
    ] = None,
) -> None:
    """Check that each reccmp address marker names the declaration below it."""

    def action() -> dict[str, Any]:
        from .markers import check_marker_hygiene

        settings = _settings()
        roots = paths or [settings.repo_dir / "src", settings.repo_dir / "include"]
        return check_marker_hygiene(list(roots), settings.repo_dir)

    _run_action(action)


@app.command("verify-boundaries")
def verify_boundaries_command(
    mapping: Annotated[
        Path | None,
        typer.Option(help="Reviewed boundary map; defaults to the gameplay boundaries."),
    ] = None,
    objects: Annotated[
        Path | None,
        typer.Option(help="Root of built objects; defaults to the gameplay-boundaries target."),
    ] = None,
    image: Annotated[
        Path | None,
        typer.Option(help="Original Wiz8.exe; defaults to the reccmp WIZ8 target."),
    ] = None,
) -> None:
    """Check reviewed bodies against built objects by relocation-masked hash.

    This is the repository's matching criterion. `just compare` measures the
    linked image and scores byte-exact bodies well under 100%, so it cannot
    detect a regression in an already-exact body.
    """

    from .boundaries import BoundariesDisagree, verify_boundaries

    def action() -> dict[str, Any]:
        settings = _settings()
        mapping_path = mapping or (
            settings.repo_dir / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv"
        )
        # Every matching target, not just the one object library. The map's
        # owner column already spans units the other targets build - the SGP
        # clock and random bodies among them - and under the narrower root those
        # rows could never be checked against the original at all.
        object_root = objects or (settings.repo_dir / "build" / "decomp" / "CMakeFiles")
        return verify_boundaries(mapping_path, object_root, image or _reccmp_original("WIZ8"))

    # A failed verdict still emits the whole report first: one regressed row
    # used to print a sentence and discard the states of every other row, which
    # each investigation then regenerated by hand. _run_action would swallow
    # the carried report into its generic handler, so the split is done here.
    try:
        _emit(action())
    except BoundariesDisagree as error:
        _emit(error.report)
        console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error
    except Exception as error:
        console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error


@app.command("diff-boundary")
def diff_boundary_command(
    symbol: Annotated[str, typer.Argument(help="Reviewed symbol, e.g. IListInit.")],
    mapping: Annotated[
        Path | None,
        typer.Option(help="Reviewed boundary map; defaults to the gameplay boundaries."),
    ] = None,
    objects: Annotated[
        Path | None,
        typer.Option(help="Root of built objects; defaults to the gameplay-boundaries target."),
    ] = None,
    image: Annotated[
        Path | None, typer.Option(help="Original Wiz8.exe; defaults to the reccmp WIZ8 target.")
    ] = None,
    all_lines: Annotated[
        bool, typer.Option("--all", help="Show matching instructions too, not only differences.")
    ] = False,
) -> None:
    """Align a near miss against the original, instruction by instruction.

    Relocated operands and branch displacements that only moved because the
    bodies differ in size are reported as such, so what is left is the real
    difference.
    """

    def action() -> dict[str, Any]:
        from .boundaries import diff_boundary

        settings = _settings()
        original = image or _reccmp_original("WIZ8")
        if original is None:
            raise RuntimeError("no original Wiz8.exe configured; pass --image")
        result = diff_boundary(
            mapping or (settings.repo_dir / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv"),
            objects
            or (
                settings.repo_dir
                / "build"
                / "decomp"
                / "CMakeFiles"
                / "WIZ8_GAMEPLAY_BOUNDARIES.dir"
            ),
            original,
            symbol,
        )
        console.print(
            f"[bold]{result['symbol']}[/bold] {result['address']} "
            f"({result['confidence']}): canonical {result['canonical_size']}B/"
            f"{result['canonical_instructions']} insns, ours {result['our_size']}B/"
            f"{result['our_instructions']} insns, {result['differing']} differing"
        )
        marker = {"differ": "[red]>>[/red]", "reloc": "[yellow]~~[/yellow]", "same": "  "}
        for line in result["lines"]:
            if line["state"] == "same" and not all_lines:
                continue
            # Memory operands are written [reg + disp]; rich would read them as
            # markup and silently delete the operand being compared.
            console.print(
                f"{marker[line['state']]} \\[{line['index']:3}] "
                f"{escape(line['canonical']):<38} | {escape(line['ours'])}",
                highlight=False,
            )
        return {key: value for key, value in result.items() if key != "lines"}

    _run_action(action)


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


@app.command("debug-artifacts")
def debug_artifacts_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
    archive_password: str | None = typer.Option(
        None,
        "--archive-password",
        envvar="WIZ8_DEBUG_ARCHIVE_PASSWORD",
        help="Password for an encrypted unassigned archive; it is never written to reports.",
    ),
) -> None:
    """Sweep every available EXE/DLL and every configured input container."""
    from .debug_artifacts import sweep_debug_artifacts

    _run_action(
        lambda: sweep_debug_artifacts(
            _settings(), update_snapshot=update_snapshot, archive_password=archive_password
        )
    )


@app.command("eh-metadata")
def eh_metadata_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Extract MSVC exception metadata from every first-party executable."""
    from .eh_metadata import sweep_eh_metadata

    _run_action(lambda: sweep_eh_metadata(_settings(), update_snapshot=update_snapshot))


@app.command("surrender-abi")
def surrender_abi_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Decode every SurRender export table into an original-ABI class surface."""
    from .surrender_abi import sweep_surrender_abi

    _run_action(lambda: sweep_surrender_abi(_settings(), update_snapshot=update_snapshot))


@app.command("call-sites")
def call_sites_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Recover assertion and runtime-class-name literals from every build."""
    from .call_sites import sweep_call_sites

    _run_action(lambda: sweep_call_sites(_settings(), update_snapshot=update_snapshot))


@app.command("polymorphism")
def polymorphism_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Census vtables, slots and constructor vptr writes from the relocation table."""
    from .polymorphism import sweep_polymorphism

    _run_action(lambda: sweep_polymorphism(_settings(), update_snapshot=update_snapshot))


@app.command("globals")
def globals_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Census global variables and their references from the relocation table."""
    from .data_globals import sweep_globals

    _run_action(lambda: sweep_globals(_settings(), update_snapshot=update_snapshot))


@app.command("function-census")
def function_census_command(
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot after a complete sweep.",
    ),
) -> None:
    """Enumerate candidate function starts and the static call graph."""
    from .function_census import sweep_function_census

    _run_action(lambda: sweep_function_census(_settings(), update_snapshot=update_snapshot))


@sgp_app.command("sweep")
def sgp_sweep(
    unit: list[str] | None = typer.Option(
        None, "--unit", help="Configured SGP unit ID; repeat to select several."
    ),
    update_snapshot: bool = typer.Option(
        False,
        "--update-snapshot",
        help="Replace the tracked proprietary-input snapshot; requires a complete sweep.",
    ),
) -> None:
    """Compile the declarative flag matrix and compare every reviewed build."""
    from .sgp_oracle import sweep_sgp_units

    _run_action(lambda: sweep_sgp_units(_settings(), unit, update_snapshot=update_snapshot))


@ghidra_app.command("import")
def ghidra_import(
    all_programs: bool = typer.Option(
        False, "--all", help="Import every configured analysis target."
    ),
    variant: str | None = typer.Option(None, "--variant"),
    program: str | None = typer.Option(None, "--program"),
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


@ghidra_app.command("rebuild")
def ghidra_rebuild(program: str) -> None:
    """Fresh-import one program, replay all reviewed knowledge, and validate it."""
    from .ghidra.rebuild import rebuild_program

    _run_action(lambda: rebuild_program(_settings(), program))


@cache_app.command("build")
def ghidra_cache_build(program: str | None = typer.Argument(None)) -> None:
    """Validate and pack the canonical project as the shared GZF seed."""
    from .ghidra.export_programs import export_project

    _run_action(lambda: export_project(_settings(), program))


@cache_app.command("materialize")
def ghidra_cache_materialize(program: str | None = typer.Argument(None)) -> None:
    """Restore, replay, and validate this agent's isolated Ghidra project."""
    from .ghidra.cache import materialize_program

    _run_action(lambda: materialize_program(_settings(), program)[1])


@cache_app.command("status")
def ghidra_cache_status(program: str | None = typer.Argument(None)) -> None:
    from .ghidra.cache import cache_status

    _run_action(lambda: cache_status(_settings(), program))


@cache_app.command("prune")
def ghidra_cache_prune(
    keep: Annotated[
        int, typer.Option(help="How many of this agent's materializations to retain.")
    ] = 3,
) -> None:
    """Drop this agent's older materialized projects.

    Only this agent's own root is touched. Another agent's projects are not ours
    to remove and one of them may be open, so other roots are only reported.
    """

    def action() -> dict[str, Any]:
        from .config import ghidra_agent_id
        from .ghidra.cache import prune_materializations

        settings = _settings()
        agents_root = settings.work_dir / "ghidra-agents"
        mine = agents_root / ghidra_agent_id()
        removed = prune_materializations(mine, max(1, keep))
        others = {}
        if agents_root.is_dir():
            for path in sorted(agents_root.iterdir()):
                if not path.is_dir() or path == mine:
                    continue
                projects = path / "projects"
                if projects.is_dir():
                    others[path.name] = len([p for p in projects.iterdir() if p.is_dir()])
        return {
            "agent": ghidra_agent_id(),
            "kept": keep,
            "evicted": removed,
            "other_agents": others,
        }

    _run_action(action)


@ghidra_app.command("validate-replay")
def ghidra_validate_replay(program: str) -> None:
    """Validate an existing materialized program against canonical reviewed evidence."""
    from .ghidra.cache import materialize_program
    from .ghidra.validate_replay import validate_reviewed_replay

    def action() -> Any:
        settings, _ = materialize_program(_settings(), program)
        return validate_reviewed_replay(settings, program, evidence_program="wiz8")

    _run_action(action)


@ghidra_app.command("apply-unzip-model")
def ghidra_apply_unzip_model(
    program: Annotated[str, typer.Argument()] = "srEXT_Unzip.dll",
) -> None:
    """Apply the reviewed srEXT_Unzip object, callback, and vtable types."""
    from .ghidra.apply_unzip_model import apply_unzip_model

    _run_action(lambda: apply_unzip_model(_settings(), program))


@ghidra_app.command("apply-zlib-model")
def ghidra_apply_zlib_model(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Apply the reviewed zlib 1.0.4 layouts and function signatures."""
    from .ghidra.apply_zlib_model import apply_zlib_model

    _run_action(lambda: apply_zlib_model(_settings(), program))


@ghidra_app.command("apply-sgp-model")
def ghidra_apply_sgp_model(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Apply source-backed SGP DirectDraw types and signatures."""
    from .ghidra.apply_sgp_model import apply_sgp_model

    _run_action(lambda: apply_sgp_model(_settings(), program))


@ghidra_app.command("apply-wiz8-class-model")
def ghidra_apply_wiz8_class_model(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Apply reviewed first-party Wiz8 class and vtable evidence."""
    from .ghidra.apply_wiz8_class_model import apply_wiz8_class_model

    _run_action(lambda: apply_wiz8_class_model(_settings(), program))


@ghidra_app.command("apply-wiz8-format-model")
def ghidra_apply_wiz8_format_model(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Apply reviewed Wizardry 8 container and data-format types."""
    from .ghidra.apply_wiz8_format_model import apply_wiz8_format_model

    _run_action(lambda: apply_wiz8_format_model(_settings(), program))


@ghidra_app.command("apply-wiz8-signature-fixes")
def ghidra_apply_wiz8_signature_fixes(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Correct known-wrong Ghidra auto-analysis signatures found while porting owned functions."""
    from .ghidra.apply_wiz8_signature_fixes import apply_wiz8_signature_fixes

    _run_action(lambda: apply_wiz8_signature_fixes(_settings(), program))


@ghidra_app.command("apply-observation-evidence")
def ghidra_apply_observation_evidence(
    program: Annotated[str, typer.Argument()] = "wiz8",
) -> None:
    """Apply neutral snapshot facts without creating semantic names."""
    from .ghidra.apply_observation_evidence import apply_observation_evidence

    _run_action(lambda: apply_observation_evidence(_settings(), program))


@ghidra_app.command("apply-eh-frame-types")
def ghidra_apply_eh_frame_types(
    program: Annotated[str, typer.Argument()] = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> None:
    """Type EH-destroyed stack slots from unwind evidence and reviewed identities."""
    from .ghidra.apply_eh_frame_types import apply_eh_frame_types

    _run_action(lambda: apply_eh_frame_types(_settings(), program))


@daemon_app.command("start")
def daemon_start(program: str | None = typer.Option(None, "--program")) -> None:
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
def ghidra_query(
    ctx: typer.Context,
    program: str,
    command: Annotated[str | None, typer.Argument()] = None,
    queries: Annotated[
        list[str] | None,
        typer.Option(
            "--query",
            "-q",
            help="Complete query to run; repeat to execute an ordered batch in one Ghidra request.",
        ),
    ] = None,
) -> None:
    """Query PROGRAM through the automatically managed persistent Ghidra process.

    The original COMMAND [ARGUMENTS] form executes one query. Repeat --query
    with a shell-style quoted command to execute several queries against the
    same open program in one daemon request.
    """
    from .ghidra.query_daemon import query, query_many

    def action() -> dict[str, Any]:
        if queries:
            if command is not None or ctx.args:
                raise ValueError("use either COMMAND [ARGUMENTS] or repeated --query clauses")
            parsed = [shlex.split(specification) for specification in queries]
            if any(not fields for fields in parsed):
                raise ValueError("--query clauses must not be empty")
            requests = [(fields[0], fields[1:]) for fields in parsed]
            results, transport = query_many(_settings(), program, requests)
            return {"transport": transport, "program": program, "results": results}
        if command is None:
            raise ValueError("provide COMMAND [ARGUMENTS] or at least one --query clause")
        result, transport = query(_settings(), program, command, list(ctx.args))
        return {"transport": transport, "program": program, "command": command, "result": result}

    # Query payloads contain decompiler listings and comments whose long lines
    # Rich would reflow to the current terminal width. Emit ordinary JSON so
    # callers receive the same parseable bytes in a TTY, a pipe, and CI.
    _run_action(action, force_json=True)


@ghidra_app.command("export-evidence")
def ghidra_export_evidence(
    program: str | None = typer.Option(None, "--program"),
    address: str | None = typer.Option(None, "--address"),
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
def ghidra_export_project(program: str | None = typer.Option(None, "--program")) -> None:
    """Compatibility alias for `ghidra cache build`."""
    from .ghidra.export_programs import export_project

    _run_action(lambda: export_project(_settings(), program))


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
    toolchain: list[str] | None = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
) -> None:
    """Build pinned MSVC600/Wine seed compiler images."""
    from .ghidra.fid_seeds import build_toolchain_images

    _run_action(lambda: build_toolchain_images(_settings(), toolchain))


@fid_app.command("probe-toolchain")
def ghidra_fid_probe_toolchain(
    toolchain: list[str] | None = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
) -> None:
    """Compile a probe and export its Rich records for toolchain comparison."""
    from .ghidra.fid_seeds import probe_toolchains

    _run_action(lambda: probe_toolchains(_settings(), toolchain))


@fid_app.command("build-seeds")
def ghidra_fid_build_seeds(
    toolchain: list[str] | None = typer.Option(
        None, "--toolchain", help="Pinned candidate ID; repeat to select several."
    ),
    library: list[str] | None = typer.Option(
        None, "--library", help="Static-library ID; repeat to select several."
    ),
) -> None:
    """Build CMake-defined static-library object seeds with pinned toolchains."""
    from .ghidra.fid_seeds import build_seed_objects

    _run_action(lambda: build_seed_objects(_settings(), toolchain, library))


@fid_app.command("extract-libraries")
def ghidra_fid_extract_libraries(
    toolchain: list[str] | None = typer.Option(
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
    threshold: float | None = typer.Option(None, "--threshold"),
    database: str = typer.Option("static", "--database", help="static or srs"),
) -> None:
    from .ghidra.fid import match_fid

    _run_action(lambda: match_fid(_settings(), program, threshold, database))


@ghidra_app.command("restore-project")
def ghidra_restore_project(program: str | None = typer.Option(None, "--program")) -> None:
    """Compatibility alias for `ghidra cache materialize`."""
    from .ghidra.export_programs import restore_project

    _run_action(lambda: restore_project(_settings(), program))


@report_app.command("bootstrap")
def report_bootstrap() -> None:
    from .reports.bootstrap import bootstrap_report

    _run_action(lambda: bootstrap_report(_settings()))


@report_app.command("translation-units")
def report_translation_units() -> None:
    """Regenerate assertion-bounded source intervals and gameplay ownership."""
    from .reports.translation_units import translation_unit_report

    _run_action(lambda: translation_unit_report(_settings()))


@report_app.command("data-segmentation")
def report_data_segmentation(
    update_snapshot: bool = typer.Option(
        False, "--update-snapshot", help="Refresh the tracked unit-data-intervals snapshot."
    ),
) -> None:
    """Fit the .text unit order to the data sections and attribute globals."""
    from .reports.data_segmentation import data_segmentation_report

    _run_action(
        lambda: data_segmentation_report(_settings(), update_snapshot=update_snapshot)
    )


@report_app.command("class-family")
def report_class_family(
    vtable: Annotated[str, typer.Argument(help="A vtable address in the family, e.g. 005ed5bc.")],
) -> None:
    """Show every vptr write around one class, so a family reads at a glance."""
    from .reports.class_family import class_family_report

    def action() -> str:
        return class_family_report(_settings(), vtable)["rendered"]

    _run_action(action)


@report_app.command("class-candidates")
def report_class_candidates() -> None:
    """Generate reviewable class candidates from the polymorphism snapshots."""
    from .reports.class_candidates import class_candidates_report

    _run_action(lambda: class_candidates_report(_settings()))


@report_app.command("status")
def report_status() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .reports.status import status_report

    _run_action(lambda: status_report(_settings()))


@report_app.command("context")
def report_context(
    address: Annotated[str, typer.Argument(help="Address inside the function to inspect")],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .reports.recovery_context import recovery_context_report

    _run_action(lambda: recovery_context_report(_settings(), address, program))
