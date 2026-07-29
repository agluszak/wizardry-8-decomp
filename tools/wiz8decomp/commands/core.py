from __future__ import annotations

from pathlib import Path
from typing import Annotated, Any

import typer
from rich.markup import escape

toolchain_app = typer.Typer(help="Build the pinned analysis toolchain.", no_args_is_help=True)
analyze_app = typer.Typer(help="Run project-specific binary analysis.", no_args_is_help=True)


def doctor_command() -> None:
    """Validate paths, pinned tools, extractors, and repository safety."""
    from .. import command_support as cli
    from ..doctor import validate_environment

    cli.run_action(lambda: validate_environment(cli.settings()))


def prepare_command() -> None:
    """Idempotently prepare extracted variants and pinned source dependencies."""
    from .. import command_support as cli
    from ..build import prepare

    cli.run_action(lambda: prepare(cli.settings()))


def check_command() -> None:
    """Run the fast public validation lane."""
    from .. import command_support as cli
    from ..build import check
    from ..config import repository_root

    cli.run_action(lambda: check(repository_root()))


def build_command(
    target: Annotated[str, typer.Argument(help="Friendly alias or CMake target.")] = "match",
    jobs: Annotated[int | None, typer.Option("--jobs", "-j")] = None,
) -> None:
    """Configure when needed and build one product target."""
    from .. import command_support as cli
    from ..build import build_target

    cli.run_action(lambda: build_target(cli.settings(), target, jobs))


def compare_command(
    ctx: typer.Context,
    addresses: Annotated[
        list[str] | None,
        typer.Argument(help="Original addresses; omit for whole-image diagnostics."),
    ] = None,
    files: Annotated[
        list[Path] | None,
        typer.Option("--file", help="Compare every FUNCTION marker in this source file."),
    ] = None,
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    no_build: bool = typer.Option(False, "--no-build"),
) -> None:
    """Compare selected functions in one process, or diagnose the whole image."""
    from .. import command_support as cli
    from ..build import build_target, compare
    from ..reccmp_workflows import compare_selected, selected_addresses

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if addresses or files:
            if ctx.args:
                raise ValueError("raw reccmp options cannot be combined with selected functions")
            if not no_build:
                build_target(settings, target)
            selected = selected_addresses(addresses or [], files or [])
            return compare_selected(settings.repo_dir, target, selected)
        return compare(settings, target, list(ctx.args), build_first=not no_build)

    cli.run_action(action)


def triage_command(
    addresses: Annotated[list[str] | None, typer.Argument(help="Original addresses.")] = None,
    files: Annotated[
        list[Path] | None,
        typer.Option("--file", help="Triage every FUNCTION marker in this source file."),
    ] = None,
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    no_build: bool = typer.Option(False, "--no-build"),
) -> None:
    """Interpret reccmp's structured first divergence without parsing assembly."""
    from .. import command_support as cli
    from ..build import build_target
    from ..reccmp_workflows import selected_addresses, triage_selected

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        selected = selected_addresses(addresses or [], files or [])
        return triage_selected(settings.repo_dir, target, selected)

    cli.run_action(action)


def vtable_command(
    class_filter: Annotated[str | None, typer.Argument(help="Class-name substring.")] = None,
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    verbose: bool = typer.Option(False, "--verbose", "-v"),
    no_build: bool = typer.Option(False, "--no-build"),
) -> None:
    """Compare vtables and refuse a vacuous zero-entity success."""
    from .. import command_support as cli
    from ..build import build_target
    from ..reccmp_workflows import compare_vtables

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        return compare_vtables(settings.repo_dir, target, class_filter, verbose=verbose)

    cli.run_action(action)


def datacmp_command(
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    show_all: bool = typer.Option(False, "--all", "-a"),
    verbose: bool = typer.Option(False, "--verbose", "-v"),
    no_build: bool = typer.Option(False, "--no-build"),
) -> None:
    """Compare reviewed global data through reccmp."""
    from .. import command_support as cli
    from ..build import build_target
    from ..reccmp_workflows import compare_data

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        return compare_data(settings.repo_dir, target, show_all=show_all, verbose=verbose)

    cli.run_action(action)


def address_command(
    addresses: Annotated[list[str], typer.Argument(help="Original or recompiled addresses.")],
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    no_build: bool = typer.Option(False, "--no-build"),
) -> None:
    """Translate paired original and recompiled addresses in one process."""
    from .. import command_support as cli
    from ..build import build_target
    from ..reccmp_workflows import parse_address, translate_addresses

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        queries = sorted({parse_address(address) for address in addresses})
        if not queries:
            raise ValueError("pass one or more addresses")
        return translate_addresses(settings.repo_dir, target, queries)

    cli.run_action(action)


def run_command() -> None:
    """Build, stage, and run the recovered executable under Wine."""
    from .. import command_support as cli
    from ..build import build_target
    from ..runtime import run_game

    def action() -> dict[str, Any]:
        build_target(cli.settings(), "runtime")
        return run_game(cli.settings())

    cli.run_action(action)


def runtime_test_command() -> None:
    """Build and run deterministic in-process semantic scenarios."""
    from .. import command_support as cli
    from ..build import build_target
    from ..runtime import run_runtime_suite

    def action() -> dict[str, Any]:
        build_target(cli.settings(), "runtime-test")
        return run_runtime_suite(cli.settings())

    cli.run_action(action)


def verify_command(
    compare_image: Annotated[bool, typer.Option("--compare/--no-compare")] = True,
) -> None:
    """Run build, boundary, linked-image, and test validation."""
    from .. import command_support as cli
    from ..build import verify

    cli.run_action(lambda: verify(cli.settings(), compare_image=compare_image))


@toolchain_app.command("build")
def toolchain_build_command() -> None:
    from .. import command_support as cli
    from ..build import build_toolchain

    cli.run_action(lambda: build_toolchain(cli.settings()))


def register(app: typer.Typer) -> None:
    app.command("doctor")(doctor_command)
    app.command("prepare")(prepare_command)
    app.command("check")(check_command)
    app.command("build")(build_command)
    app.command(
        "compare", context_settings={"allow_extra_args": True, "ignore_unknown_options": True}
    )(compare_command)
    app.command("triage")(triage_command)
    app.command("vtable")(vtable_command)
    app.command("datacmp")(datacmp_command)
    app.command("addr")(address_command)
    app.command("run")(run_command)
    app.command("runtime-test")(runtime_test_command)
    app.command("verify")(verify_command)
    app.add_typer(analyze_app, name="analyze")
    app.command("check-build-dir", hidden=True)(check_build_dir_command)
    app.command("check-markers", hidden=True)(check_markers_command)
    app.command("check-repository", hidden=True)(check_repository_command)
    app.command("check-reccmp", hidden=True)(check_reccmp_command)
    app.command("verify-boundaries", hidden=True)(verify_boundaries_command)
    analyze_app.command("unresolved")(unresolved_report_command)
    analyze_app.command("diff-boundary")(diff_boundary_command)
    analyze_app.command("inventory")(inventory_command)
    analyze_app.command("trace")(trace_command)
    analyze_app.command("source-layouts")(verify_source_layouts_command)


def unresolved_report_command(
    objects: Annotated[Path | None, typer.Option(help="Object root.")] = None,
    link_map: Annotated[Path | None, typer.Option(help="Linker MAP.")] = None,
) -> None:
    from .. import command_support as cli
    from ..unresolved import unresolved_report

    def action() -> dict[str, Any]:
        settings = cli.settings()
        build = settings.repo_dir / "build" / "decomp"
        return unresolved_report(objects or build / "CMakeFiles", link_map or build / "Wiz8.map")

    cli.run_action(action)


def check_build_dir_command(
    build_dir: Annotated[Path | None, typer.Option(help="CMake build directory.")] = None,
) -> None:
    from .. import command_support as cli
    from ..build_dir import check_build_directory

    cli.run_action(
        lambda: check_build_directory(
            build_dir or cli.settings().repo_dir / "build" / "decomp",
            cli.settings().repo_dir,
        )
    )


def check_markers_command(
    paths: Annotated[list[Path] | None, typer.Option(help="Files or directories to scan.")] = None,
) -> None:
    from .. import command_support as cli
    from ..config import repository_root
    from ..markers import check_marker_hygiene

    def action() -> dict[str, Any]:
        repository = repository_root()
        roots = paths or [repository / "src", repository / "include"]
        return check_marker_hygiene(list(roots), repository)

    cli.run_action(action)


def check_repository_command() -> None:
    """Reject tracked generated, proprietary, editor, and oversized artifacts."""
    from .. import command_support as cli
    from ..config import repository_root
    from ..repository import validate_repository_hygiene

    cli.run_action(lambda: validate_repository_hygiene(repository_root()))


def check_reccmp_command() -> None:
    """Run reccmp's annotation parser and project lint policy."""
    from .. import command_support as cli
    from ..config import repository_root
    from ..reccmp_lint import validate_reccmp_annotations

    cli.run_action(lambda: validate_reccmp_annotations(repository_root()))


def verify_boundaries_command(
    mapping: Annotated[Path | None, typer.Option(help="Reviewed boundary map.")] = None,
    objects: Annotated[Path | None, typer.Option(help="Root of built objects.")] = None,
    image: Annotated[Path | None, typer.Option(help="Original Wiz8.exe.")] = None,
) -> None:
    from .. import command_support as cli
    from ..boundaries import BoundariesDisagree, verify_boundaries
    from ..build import boundary_object_roots

    settings = cli.settings()
    mapping_path = mapping or settings.repo_dir / "config/reccmp/wiz8-gameplay-boundaries.csv"
    object_root = [objects] if objects is not None else boundary_object_roots(settings)
    try:
        cli.emit(verify_boundaries(mapping_path, object_root, image or cli.reccmp_original("WIZ8")))
    except BoundariesDisagree as error:
        cli.emit(error.report)
        cli.console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error
    except Exception as error:
        cli.console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error


def diff_boundary_command(
    selector: Annotated[str, typer.Argument(help="Reviewed address or unambiguous symbol.")],
    mapping: Annotated[Path | None, typer.Option(help="Reviewed boundary map.")] = None,
    objects: Annotated[Path | None, typer.Option(help="Root of built objects.")] = None,
    image: Annotated[Path | None, typer.Option(help="Original Wiz8.exe.")] = None,
    all_lines: Annotated[bool, typer.Option("--all", help="Show matching instructions.")] = False,
) -> None:
    from .. import command_support as cli
    from ..boundaries import diff_boundary

    def action() -> dict[str, Any]:
        settings = cli.settings()
        original = image or cli.reccmp_original("WIZ8")
        if original is None:
            raise RuntimeError("no original Wiz8.exe configured; pass --image")
        result = diff_boundary(
            mapping or settings.repo_dir / "config/reccmp/wiz8-gameplay-boundaries.csv",
            objects or settings.repo_dir / "build/decomp/CMakeFiles/WIZ8_GAMEPLAY_BOUNDARIES.dir",
            original,
            selector,
        )
        cli.console.print(
            f"[bold]{result['symbol']}[/bold] {result['address']} ({result['confidence']}): "
            f"canonical {result['canonical_size']}B/{result['canonical_instructions']} insns, "
            f"ours {result['our_size']}B/{result['our_instructions']} insns, "
            f"{result['differing']} differing"
        )
        marker = {"differ": "[red]>>[/red]", "reloc": "[yellow]~~[/yellow]", "same": "  "}
        for line in result["lines"]:
            if line["state"] == "same" and not all_lines:
                continue
            cli.console.print(
                f"{marker[line['state']]} \\[{line['index']:3}] "
                f"{escape(line['canonical']):<38} | {escape(line['ours'])}",
                highlight=False,
            )
        return {key: value for key, value in result.items() if key != "lines"}

    cli.run_action(action)


def inventory_command(json_output: bool = typer.Option(False, "--json", help="Emit JSON.")) -> None:
    from .. import command_support as cli
    from ..binary.inventory import inventory

    cli.run_action(lambda: inventory(cli.settings()), force_json=json_output)


def trace_command(
    scenario: Annotated[str, typer.Argument(help="bring-up or screens.")] = "bring-up",
    seconds: Annotated[int, typer.Option(help="How long to let the scenario run.")] = 120,
    port: Annotated[int | None, typer.Option(help="winedbg gdb proxy port.")] = None,
    plan_only: Annotated[bool, typer.Option(help="Print the breakpoint plan only.")] = False,
) -> None:
    from .. import command_support as cli
    from ..dynamic import Sandbox, run_trace, trace_plan, write_report

    def action() -> dict[str, Any]:
        settings = cli.settings()
        if plan_only:
            points = trace_plan(settings.repo_dir, scenario)
            return {
                "scenario": scenario,
                "points": [
                    {"address": point.address, "name": point.name, "kind": point.kind}
                    for point in points
                ],
            }
        result = run_trace(
            settings.repo_dir,
            Sandbox.from_environment(),
            scenario,
            seconds=seconds,
            port=port,
        )
        return write_report(result, settings.repo_dir / "build/reports/trace")

    cli.run_action(action)


def verify_source_layouts_command(
    pdb: Annotated[
        Path | None,
        typer.Option("--pdb", exists=True, dir_okay=False, readable=True),
    ] = None,
) -> None:
    from .. import command_support as cli
    from ..source_layouts import verify_source_layouts

    def action() -> dict[str, Any]:
        report = verify_source_layouts(cli.settings(), pdb)
        if not report["ok"]:
            raise ValueError(
                f"compiled source layout differs at {report['failure_count']} checks; "
                f"see {report['report']}"
            )
        return report

    cli.run_action(action)
