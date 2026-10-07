from __future__ import annotations

import sys
from pathlib import Path
from typing import Annotated, Any

import typer

toolchain_app = typer.Typer(help="Build the pinned analysis toolchain.", no_args_is_help=True)
analyze_app = typer.Typer(help="Run project-specific binary analysis.", no_args_is_help=True)


def prepare_command(
    comparison_target: Annotated[
        list[str] | None,
        typer.Option(
            "--comparison-target",
            help="Prepare only reviewed original binaries for this reccmp target; repeatable.",
        ),
    ] = None,
    sources_only: Annotated[
        bool,
        typer.Option("--sources-only", help="Fetch public library sources without game inputs."),
    ] = False,
) -> None:
    """Prepare public sources, full runtime inputs or a minimal comparison corpus."""
    from .. import command_support as cli
    from ..build import prepare, prepare_comparison
    from ..build_inputs import fetch_sources

    if sources_only and comparison_target:
        raise typer.BadParameter("--sources-only cannot be combined with --comparison-target")
    settings = cli.settings()
    if sources_only:
        cli.emit(fetch_sources(settings))
    else:
        cli.emit(
            prepare_comparison(settings, comparison_target)
            if comparison_target
            else prepare(settings)
        )


def check_command() -> None:
    """Run the fast public validation lane."""
    from .. import command_support as cli
    from ..build import check
    from ..config import repository_root

    cli.emit(check(repository_root()))


def lint_command() -> None:
    """Compile recovered C++ with clang-cl structural diagnostics."""
    from .. import command_support as cli
    from ..build import lint

    cli.emit(lint(cli.settings()))


def build_command(
    target: Annotated[str, typer.Argument(help="Friendly alias or CMake target.")] = "match",
    jobs: Annotated[int | None, typer.Option("--jobs", "-j")] = None,
) -> None:
    """Configure when needed and build one product target."""
    from .. import command_support as cli
    from ..build import build_target

    cli.emit(build_target(cli.settings(), target, jobs))


def compare_command(
    ctx: typer.Context,
    addresses: Annotated[
        list[str] | None,
        typer.Argument(help="Original addresses."),
    ] = None,
    files: Annotated[
        list[Path] | None,
        typer.Option("--file", help="Compare every FUNCTION marker in this source file."),
    ] = None,
    changed: bool = typer.Option(
        False,
        "--changed",
        help="Compare changed C/C++ files and consumers of changed headers, including template emissions.",
    ),
    all_source: bool = typer.Option(
        False,
        "--all-source",
        help="Compare every recovered FUNCTION marker for this target.",
    ),
    since: Annotated[
        str | None,
        typer.Option("--since", help="With --changed, compare files changed since this revision."),
    ] = None,
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    build: Annotated[
        bool, typer.Option("--build", help="Build fresh products and metadata before comparing.")
    ] = False,
    side_by_side: Annotated[
        bool, typer.Option("--sxs", help="Also write Ghidriff's side-by-side HTML diffs.")
    ] = False,
) -> None:
    """Decompile and diff selected functions against retail with reccmp (Ghidriff).

    Differences are review material. Fails only when a comparison did not
    complete or a selected function has no retail counterpart."""
    from .. import command_support as cli
    from ..build import build_target
    from ..comparison import (
        addresses_from_files,
        all_source_addresses,
        changed_source_files,
        compare_selected,
        header_dependent_files,
        selected_addresses,
        selectors_require_source_index,
    )

    def action() -> Any:
        settings = cli.settings()
        from ..source_index import target_for_program

        target = target_for_program(settings.repo_dir, program)
        if since is not None and not changed:
            raise ValueError("--since requires --changed")
        if all_source and (addresses or files or changed):
            raise ValueError("--all-source cannot be combined with addresses, --file, or --changed")
        if ctx.args:
            raise ValueError("raw reccmp options are not accepted by selected comparison")
        if addresses or files or changed or all_source:
            if build:
                # Building owns source-index refresh and product generation. Keep
                # the comparison path below identical for both modes.
                from ..source_index import write_source_index

                write_source_index(settings)
                build_target(settings, target)
            selected_files = list(files or [])
            changed_files: list[Path] = []
            dependent_files: list[Path] = []
            if changed:
                changed_files = changed_source_files(settings.repo_dir, since)
                selected_files.extend(changed_files)
                if not selected_files and not addresses:
                    raise ValueError("no changed C/C++ files; no functions selected")
            needs_index = (
                all_source
                or bool(selected_files)
                or selectors_require_source_index(addresses or [])
            )
            index_stale = False
            if needs_index:
                from ..source_index import warn_if_source_index_may_be_stale

                index_stale = warn_if_source_index_may_be_stale(settings.repo_dir, target)
            if changed:
                changed_headers = [
                    path for path in changed_files if path.suffix.lower() in {".h", ".hpp", ".hxx"}
                ]
                if changed_headers and index_stale:
                    raise ValueError(
                        "source index is stale for changed-header selection; "
                        "run `uv run wiz8 check`"
                    )
                dependent_files = header_dependent_files(settings, target, changed_files)
                selected_files.extend(dependent_files)
            if (
                changed
                and not addresses
                and not addresses_from_files(
                    settings.repo_dir, target, selected_files, include_templates=True
                )
            ):
                # Changed files without this target's FUNCTION markers: there
                # is nothing to compare, which is not a failure.
                result: dict[str, Any] = {"ok": True, "selected": 0, "functions": []}
            else:
                selected = (
                    all_source_addresses(settings.repo_dir, target)
                    if all_source
                    else selected_addresses(
                        settings.repo_dir,
                        target,
                        addresses or [],
                        selected_files,
                        include_templates=changed,
                    )
                )
                if not selected:
                    raise ValueError(f"no recovered FUNCTION markers for target {target}")
                result = compare_selected(
                    settings.repo_dir,
                    target,
                    selected,
                    settings.ghidra_install_dir,
                    side_by_side=side_by_side,
                    classify_source_non_emissions=needs_index,
                    classify_template_emissions=changed,
                )
            if changed:
                baseline = since or "working-copy parent"
                result["selection"] = {
                    "mode": "changed-and-dependent-files",
                    "baseline": baseline,
                    "changed_files": [
                        str(path.relative_to(settings.repo_dir)) for path in changed_files
                    ],
                    "dependent_files": [
                        str(path.relative_to(settings.repo_dir))
                        for path in dependent_files
                        if path not in changed_files
                    ],
                }
            return result
        raise ValueError("select functions by address, --file, --changed, or --all-source")

    result = action()
    # Changed-header selections can contain thousands of functions. Their
    # detailed results already live in the comparison report under build/.
    functions = result.get("functions", [])
    cli.emit(
        {
            **{key: value for key, value in result.items() if key != "functions"},
            "functions": functions[:20],
            "omitted": max(0, len(functions) - 20),
        }
    )
    if not result["ok"]:
        raise typer.Exit(code=1)


def vtable_command(
    class_filter: Annotated[str | None, typer.Argument(help="Class-name substring.")] = None,
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    build: Annotated[bool, typer.Option("--build", help="Build before comparing.")] = False,
) -> None:
    """Compare vtables and refuse a vacuous zero-entity success."""
    from .. import command_support as cli
    from ..build import build_target
    from ..comparison import compare_vtables

    def action() -> Any:
        settings = cli.settings()
        from ..source_index import target_for_program

        target = target_for_program(settings.repo_dir, program)
        if build:
            from ..source_index import write_source_index

            write_source_index(settings)
            build_target(settings, target)
        result = compare_vtables(settings.repo_dir, target, class_filter)
        return result

    result = action()
    cli.emit(result)
    if not result.get("ok"):
        raise typer.Exit(code=1)


def datacmp_command(
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    build: Annotated[bool, typer.Option("--build", help="Build before comparing.")] = False,
) -> None:
    """Compare reviewed global data through reccmp."""
    from .. import command_support as cli
    from ..build import build_target
    from ..comparison import compare_data

    def action() -> Any:
        settings = cli.settings()
        from ..source_index import target_for_program

        target = target_for_program(settings.repo_dir, program)
        if build:
            from ..source_index import write_source_index

            write_source_index(settings)
            build_target(settings, target)
        result = compare_data(settings.repo_dir, target)
        return result

    result = action()
    cli.emit(result)
    if not result.get("ok"):
        raise typer.Exit(code=1)


def address_command(
    addresses: Annotated[list[str], typer.Argument(help="Original or recompiled addresses.")],
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    build: Annotated[bool, typer.Option("--build", help="Build before translating.")] = False,
) -> None:
    """Translate paired original and recompiled addresses in one process."""
    from .. import command_support as cli
    from ..build import build_target
    from ..comparison import parse_address, translate_addresses

    def action() -> Any:
        settings = cli.settings()
        from ..source_index import target_for_program

        target = target_for_program(settings.repo_dir, program)
        queries = sorted({parse_address(address) for address in addresses})
        if not queries:
            raise ValueError("pass one or more addresses")
        if build:
            from ..source_index import write_source_index

            write_source_index(settings)
            build_target(settings, target)
        result = translate_addresses(settings.repo_dir, target, queries)
        return result

    cli.emit(action())


def runtime_test_command(
    scenario: Annotated[
        list[str] | None,
        typer.Option("--scenario", help="Run a named scenario; repeat to select several."),
    ] = None,
    tier: Annotated[
        str, typer.Option(help="Registry tier: pr, main, or nightly (cumulative).")
    ] = "pr",
    repeat: Annotated[
        int, typer.Option(min=1, help="Repeat selected scenarios in fresh stages.")
    ] = 1,
    check_order: Annotated[
        bool,
        typer.Option(
            "--check-order", "--full", help="Repeat in reverse order and compare observations."
        ),
    ] = False,
    build: Annotated[
        bool,
        typer.Option("--build", help="Build a fresh runtime-test product before running."),
    ] = False,
    renderer: Annotated[
        str | None,
        typer.Option(
            "--renderer",
            help="Set GALLIUM_DRIVER for this run (e.g. softpipe, llvmpipe); "
            "default keeps the caller's environment.",
        ),
    ] = None,
    isolate: Annotated[
        bool,
        typer.Option(
            "--isolate",
            help="Give every case a fresh process instead of batching "
            "batch-eligible cases that share a fixture.",
        ),
    ] = False,
    workers: Annotated[
        int,
        typer.Option(
            "--workers",
            min=1,
            help="Run independent cases on this many isolated workers; each "
            "gets its own stage, Wine prefix, and virtual display.",
        ),
    ] = 2,
) -> None:
    """Run deterministic in-process semantic scenarios using the existing product."""
    from .. import command_support as cli
    from ..build import build_target, warn_if_product_may_be_stale
    from ..runtime import run_runtime_suite

    settings = cli.settings()
    if build:
        build_target(settings, "runtime-test")
    warn_if_product_may_be_stale(settings, "runtime-test")
    cli.emit(
        run_runtime_suite(
            settings,
            scenarios=tuple(dict.fromkeys(scenario)) if scenario else None,
            tier=tier,
            repeat=repeat,
            check_order=check_order,
            renderer=renderer,
            batch=not isolate,
            workers=workers,
        )
    )


def run_command(
    arguments: Annotated[
        list[str] | None,
        typer.Argument(help="Game arguments; /WINDOW is always passed."),
    ] = None,
    original: Annotated[
        bool,
        typer.Option("--original", help="Run the retail executable instead of the recomp."),
    ] = False,
) -> None:
    """Stage the primary game under build/runtime and run it under Wine."""
    from .. import command_support as cli
    from ..runtime import run_product

    result = run_product(cli.settings(), list(arguments or []), original=original)
    cli.emit(
        {
            "status": result["status"],
            "stage": result["stage"],
            "executable": result["executable"],
            "crash": result["crash"],
        }
    )
    if result["status"]:
        raise typer.Exit(result["status"])


@toolchain_app.command("build")
def toolchain_build_command(
    toolchain: Annotated[list[str] | None, typer.Argument()] = None,
) -> None:
    from .. import command_support as cli
    from ..build import build_toolchain

    cli.emit(build_toolchain(cli.settings(), toolchain))


def register(app: typer.Typer) -> None:
    app.command("prepare")(prepare_command)
    app.command("check")(check_command)
    app.command("lint")(lint_command)
    app.command("build")(build_command)
    app.command("compare")(compare_command)
    app.command("vtable")(vtable_command)
    app.command("datacmp")(datacmp_command)
    app.command("addr")(address_command)
    app.command("runtime-test")(runtime_test_command)
    app.command("run")(run_command)
    app.command("debug")(debug_command)
    app.add_typer(analyze_app, name="analyze")
    analyze_app.command("source-index")(source_index_command)


def source_index_command(
    jobs: Annotated[
        int | None, typer.Option(min=1, help="Maximum native collector workers.")
    ] = None,
) -> None:
    """Generate build/source-index.json from reccmp markers and Clang AST."""
    from .. import command_support as cli
    from ..source_index import write_source_index

    cli.emit(write_source_index(cli.settings(), jobs=jobs))


def debug_command(
    arguments: Annotated[
        list[str] | None,
        typer.Argument(help="Runtime product arguments."),
    ] = None,
    break_at: Annotated[
        list[str] | None,
        typer.Option("--break", help="Recomp address, optionally followed by :GDB_CONDITION."),
    ] = None,
    scenario: Annotated[
        str | None,
        typer.Option(help="Run one runtime-test scenario under GDB."),
    ] = None,
    timeout: Annotated[
        int,
        typer.Option(min=1, help="Seconds to wait for a debugger stop."),
    ] = 180,
    build: Annotated[
        bool, typer.Option("--build", help="Build a fresh runtime product before debugging.")
    ] = False,
) -> None:
    """Debug an existing runtime product through a deterministic GDB session."""
    from .. import command_support as cli
    from ..build import build_target, warn_if_product_may_be_stale
    from ..debug.debugger import run_debugger

    settings = cli.settings()
    if scenario is not None and arguments:
        raise ValueError("runtime product arguments cannot be combined with --scenario")
    target = "runtime-test" if scenario is not None else "runtime"
    if build:
        build_target(settings, target)
    warn_if_product_may_be_stale(settings, target)
    breakpoints: list[tuple[int, str | None]] = []
    for specification in break_at or []:
        address_text, separator, condition = specification.partition(":")
        try:
            address = int(address_text, 0)
        except ValueError as error:
            raise ValueError(f"invalid debugger address: {address_text}") from error
        breakpoints.append((address, condition if separator else None))
    result = run_debugger(
        settings,
        list(arguments or []),
        breakpoints=breakpoints or None,
        scenario=scenario,
        timeout=timeout,
    )
    sys.stdout.write(result["report"])
    sys.stderr.write(
        f"reason: {result['reason']}\nraw gdb: {result['log']}\nsession: {result['session']}\n"
    )
    if result["exit_code"]:
        raise typer.Exit(result["exit_code"])
