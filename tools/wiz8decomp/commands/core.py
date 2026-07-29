from __future__ import annotations

from pathlib import Path
from typing import Annotated, Any

import typer

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


def lint_command() -> None:
    """Compile recovered C++ with clang-cl structural diagnostics."""
    from .. import command_support as cli
    from ..build import lint

    cli.run_action(lambda: lint(cli.settings()))


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
            selected = selected_addresses(settings.repo_dir, addresses or [], files or [])
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
        selected = selected_addresses(settings.repo_dir, addresses or [], files or [])
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
    """Run compiler, source-model, linked-image, unit, and runtime validation."""
    from .. import command_support as cli
    from ..build import verify

    cli.run_action(lambda: verify(cli.settings(), compare_image=compare_image))


@toolchain_app.command("build")
def toolchain_build_command(
    toolchain: Annotated[list[str] | None, typer.Argument()] = None,
) -> None:
    from .. import command_support as cli
    from ..build import build_toolchain

    cli.run_action(lambda: build_toolchain(cli.settings(), toolchain))


def register(app: typer.Typer) -> None:
    app.command("doctor")(doctor_command)
    app.command("prepare")(prepare_command)
    app.command("check")(check_command)
    app.command("lint")(lint_command)
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
    app.command("check-reccmp", hidden=True)(check_reccmp_command)
    analyze_app.command("unresolved")(unresolved_report_command)
    analyze_app.command("inventory")(inventory_command)
    analyze_app.command("trace")(trace_command)
    analyze_app.command("source-layouts")(verify_source_layouts_command)
    analyze_app.command("source-index")(source_index_command)


def source_index_command() -> None:
    """Generate build/source-index.json from reccmp markers and Clang AST."""
    from .. import command_support as cli
    from ..source_index import write_source_index

    cli.run_action(lambda: write_source_index(cli.settings()))


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


def check_reccmp_command() -> None:
    """Run reccmp's annotation parser and project lint policy."""
    from .. import command_support as cli
    from ..config import repository_root
    from ..reccmp_lint import validate_reccmp_annotations

    cli.run_action(lambda: validate_reccmp_annotations(repository_root()))


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
    from ..source_layouts import require_source_layouts, verify_source_layouts

    def action() -> dict[str, Any]:
        return require_source_layouts(verify_source_layouts(cli.settings(), pdb))

    cli.run_action(action)
