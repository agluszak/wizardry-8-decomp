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

    cli.run_action(lambda: cli.summary(validate_environment(cli.settings()), label="environment"))


def prepare_command() -> None:
    """Idempotently prepare extracted variants and pinned source dependencies."""
    from .. import command_support as cli
    from ..build import prepare

    cli.run_action(lambda: cli.summary(prepare(cli.settings()), label="prepare"))


def check_command(
    json_output: bool = typer.Option(False, "--json", help="Emit complete JSON."),
) -> None:
    """Run the fast public validation lane."""
    from .. import command_support as cli
    from ..build import check, check_human_result
    from ..config import repository_root

    def action():
        result = check(repository_root())
        return cli.human(check_human_result(result), result)

    cli.run_action(action, force_json=json_output)


def lint_command(
    json_output: bool = typer.Option(False, "--json", help="Emit complete JSON."),
) -> None:
    """Compile recovered C++ with clang-cl structural diagnostics."""
    from .. import command_support as cli
    from ..build import lint, lint_human_result

    def action():
        result = lint(cli.settings())
        return cli.human(lint_human_result(result), result)

    cli.run_action(action, force_json=json_output)


def diagnostics_command() -> None:
    """Emit non-gating recovery-relevant clang diagnostics."""
    from .. import command_support as cli
    from ..build import lint, lint_human_result

    def action():
        result = lint(cli.settings(), full_diagnostics=True)
        return cli.human(lint_human_result(result), result)

    cli.run_action(action)


def build_command(
    target: Annotated[str, typer.Argument(help="Friendly alias or CMake target.")] = "match",
    jobs: Annotated[int | None, typer.Option("--jobs", "-j")] = None,
    json_output: bool = typer.Option(False, "--json", help="Emit complete JSON."),
) -> None:
    """Configure when needed and build one product target."""
    from .. import command_support as cli
    from ..build import build_human_result, build_target

    def action():
        settings = cli.settings()
        result = build_target(settings, target, jobs)
        text, data = build_human_result(settings, result)
        return cli.human(text, data)

    cli.run_action(action, force_json=json_output)


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
    changed: bool = typer.Option(
        False, "--changed", help="Compare all FUNCTION markers in C++ files changed in Jujutsu."
    ),
    since: Annotated[
        str | None,
        typer.Option("--since", help="With --changed, compare files changed since this revision."),
    ] = None,
    target: Annotated[str, typer.Option("--target")] = "WIZ8",
    no_build: bool = typer.Option(False, "--no-build"),
    json_output: bool = typer.Option(False, "--json", help="Emit complete JSON."),
) -> None:
    """Compare selected functions in one process, or diagnose the whole image."""
    from .. import command_support as cli
    from ..build import build_target, compare
    from ..reccmp_workflows import (
        changed_source_files,
        compare_selected,
        comparison_human,
        selected_addresses,
    )
    from ..source_index import write_source_index

    def action() -> Any:
        settings = cli.settings()
        if since is not None and not changed:
            raise ValueError("--since requires --changed")
        if addresses or files or changed:
            if ctx.args:
                raise ValueError("raw reccmp options cannot be combined with selected functions")
            selected_files = list(files or [])
            if changed:
                selected_files.extend(changed_source_files(settings.repo_dir, since))
                if not selected_files and not addresses:
                    raise ValueError("no changed C++ files; no functions selected")
            # Selection must see this source state, not the snapshot left by
            # an earlier check/test run. The indexer caches unchanged inputs.
            write_source_index(settings)
            selected = selected_addresses(settings.repo_dir, addresses or [], selected_files)
            if not no_build:
                build_target(settings, target)
            result = compare_selected(
                settings.repo_dir,
                target,
                selected,
                include_windows=bool(addresses) and len(selected) <= 8,
            )
            return cli.human(comparison_human(result), result)
        return cli.summary(compare(settings, target, list(ctx.args), build_first=not no_build))

    cli.run_action(action, force_json=json_output)


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

    def action() -> Any:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        result = compare_vtables(settings.repo_dir, target, class_filter, verbose=verbose)
        return cli.summary(result, label="vtable comparison")

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

    def action() -> Any:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        result = compare_data(settings.repo_dir, target, show_all=show_all, verbose=verbose)
        return cli.summary(result, label="data comparison")

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

    def action() -> Any:
        settings = cli.settings()
        if not no_build:
            build_target(settings, target)
        queries = sorted({parse_address(address) for address in addresses})
        if not queries:
            raise ValueError("pass one or more addresses")
        result = translate_addresses(settings.repo_dir, target, queries)
        lines = [
            f"{row.get('query', row.get('address', ''))}: "
            f"{row.get('original') or row.get('recompiled') or row.get('result') or 'unpaired'}"
            for row in result.get("addresses", result.get("results", []))
        ]
        return cli.human("\n".join(lines) or "no paired addresses", result)

    cli.run_action(action)


def run_command() -> None:
    """Build, stage, and run the recovered executable under Wine."""
    from .. import command_support as cli
    from ..build import build_target
    from ..runtime import run_game

    def action() -> Any:
        build_target(cli.settings(), "runtime")
        result = run_game(cli.settings())
        return cli.summary(result, label="runtime launched")

    cli.run_action(action)


def runtime_test_command() -> None:
    """Build and run deterministic in-process semantic scenarios."""
    from .. import command_support as cli
    from ..build import build_target
    from ..runtime import run_runtime_suite

    def action() -> Any:
        build_target(cli.settings(), "runtime-test")
        result = run_runtime_suite(cli.settings())
        return cli.summary(result, label="runtime tests")

    cli.run_action(action)


def verify_command(
    compare_image: Annotated[bool, typer.Option("--compare/--no-compare")] = True,
    against: Annotated[
        Path | None,
        typer.Option(
            "--against",
            exists=True,
            dir_okay=False,
            readable=True,
            help="Alternate normalized source-layout baseline CSV.",
        ),
    ] = None,
) -> None:
    """Run compiler, source-model, linked-image, unit, and runtime validation."""
    from .. import command_support as cli
    from ..build import verify

    def action():
        result = verify(cli.settings(), compare_image=compare_image, against=against)
        return cli.summary(result, label="verification")

    cli.run_action(action)


@toolchain_app.command("build")
def toolchain_build_command(
    toolchain: Annotated[list[str] | None, typer.Argument()] = None,
) -> None:
    from .. import command_support as cli
    from ..build import build_toolchain

    cli.run_action(lambda: cli.summary(build_toolchain(cli.settings(), toolchain)))


def register(app: typer.Typer) -> None:
    app.command("doctor")(doctor_command)
    app.command("prepare")(prepare_command)
    app.command("check")(check_command)
    app.command("lint")(lint_command)
    app.command("diagnostics")(diagnostics_command)
    app.command("build")(build_command)
    app.command(
        "compare", context_settings={"allow_extra_args": True, "ignore_unknown_options": True}
    )(compare_command)
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

    def action():
        result = write_source_index(cli.settings())
        counts = result.get("counts", result)
        functions = counts.get("functions") or counts.get("markers") or "updated"
        return cli.human(f"source index: {functions} functions", result)

    cli.run_action(action)


def unresolved_report_command(
    objects: Annotated[Path | None, typer.Option(help="Object root.")] = None,
    link_map: Annotated[Path | None, typer.Option(help="Linker MAP.")] = None,
    write_baseline: Annotated[
        bool,
        typer.Option("--write-baseline", help="Initialize or reduce the reviewed baseline."),
    ] = False,
) -> None:
    from .. import command_support as cli
    from ..unresolved import DEFAULT_BASELINE, unresolved_report, write_unresolved_baseline

    def action():
        settings = cli.settings()
        build = settings.repo_dir / "build" / "decomp"
        report = unresolved_report(
            objects or build / "CMakeFiles" / "wiz8_recovered_objects.dir",
            link_map or build / "Wiz8.map",
        )
        if write_baseline:
            return cli.summary(
                write_unresolved_baseline(settings.repo_dir / DEFAULT_BASELINE, report)
            )
        return cli.summary(report)

    cli.run_action(action)


def check_build_dir_command(
    build_dir: Annotated[Path | None, typer.Option(help="CMake build directory.")] = None,
) -> None:
    from .. import command_support as cli
    from ..build_dir import check_build_directory

    cli.run_action(
        lambda: cli.summary(
            check_build_directory(
                build_dir or cli.settings().repo_dir / "build" / "decomp",
                cli.settings().repo_dir,
            )
        )
    )


def check_reccmp_command() -> None:
    """Run reccmp's annotation parser and project lint policy."""
    from .. import command_support as cli
    from ..config import repository_root
    from ..reccmp_lint import validate_reccmp_annotations

    cli.run_action(lambda: cli.summary(validate_reccmp_annotations(repository_root())))


def inventory_command(json_output: bool = typer.Option(False, "--json", help="Emit JSON.")) -> None:
    from .. import command_support as cli
    from ..binary.inventory import inventory

    cli.run_action(
        lambda: cli.summary(inventory(cli.settings()), label="inventory"),
        force_json=json_output,
    )


def trace_command(
    scenario: Annotated[str, typer.Argument(help="bring-up or screens.")] = "bring-up",
    seconds: Annotated[int, typer.Option(help="How long to let the scenario run.")] = 120,
    port: Annotated[int | None, typer.Option(help="winedbg gdb proxy port.")] = None,
    plan_only: Annotated[bool, typer.Option(help="Print the breakpoint plan only.")] = False,
) -> None:
    from .. import command_support as cli
    from ..dynamic import Sandbox, run_trace, trace_plan, write_report

    def action():
        settings = cli.settings()
        if plan_only:
            points = trace_plan(settings.repo_dir, scenario)
            return cli.summary(
                {
                    "scenario": scenario,
                    "points": [
                        {"address": point.address, "name": point.name, "kind": point.kind}
                        for point in points
                    ],
                }
            )
        result = run_trace(
            settings.repo_dir,
            Sandbox.from_environment(),
            scenario,
            seconds=seconds,
            port=port,
        )
        return cli.summary(write_report(result, settings.repo_dir / "build/reports/trace"))

    cli.run_action(action)


def verify_source_layouts_command(
    pdb: Annotated[
        Path | None,
        typer.Option("--pdb", exists=True, dir_okay=False, readable=True),
    ] = None,
    write_baseline: Annotated[
        bool,
        typer.Option(
            "--write-baseline",
            help="Initialize or strictly reduce the tracked failure baseline.",
        ),
    ] = False,
) -> None:
    from .. import command_support as cli
    from ..source_layouts import (
        DEFAULT_BASELINE,
        require_source_layouts,
        verify_source_layouts,
        write_source_layout_baseline,
    )

    def action():
        settings = cli.settings()
        report = verify_source_layouts(settings, pdb)
        if write_baseline:
            return cli.summary(
                write_source_layout_baseline(settings.repo_dir / DEFAULT_BASELINE, report)
            )
        return cli.summary(require_source_layouts(report))

    cli.run_action(action)
