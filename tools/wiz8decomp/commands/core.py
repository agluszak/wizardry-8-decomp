from __future__ import annotations

import sys
from pathlib import Path
from typing import Annotated, Any

import typer

toolchain_app = typer.Typer(help="Build the pinned analysis toolchain.", no_args_is_help=True)
analyze_app = typer.Typer(help="Run project-specific binary analysis.", no_args_is_help=True)


def doctor_command() -> None:
    """Validate paths, pinned tools, extractors, and repository safety."""
    from .. import command_support as cli
    from ..doctor import validate_environment

    cli.emit(validate_environment(cli.settings()))


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


def tidy_audit_command() -> None:
    """Report the whole corpus under the non-gating clang-tidy audit profile."""
    from .. import command_support as cli
    from ..build import tidy_audit

    cli.emit(tidy_audit(cli.settings()))


def pr_check_command(
    base: Annotated[str, typer.Option("--base", help="PR base revision.")] = "main@origin",
) -> None:
    """Run every validation lane required by the files changed in a PR."""
    from .. import command_support as cli
    from ..build import check, lint, lint_required
    from ..comparison import changed_files
    from ..config import repository_root
    from ..merge_preservation import base_ancestry_report
    from ..paths import atomic_json

    repository = repository_root()
    ancestry = base_ancestry_report(repository, base)
    if ancestry["status"] != "passed":
        cli.emit({"status": "failed", "base": base, "base_ancestry": ancestry})
        raise typer.Exit(code=1)
    changed_paths = changed_files(repository, base)
    changed = [path.relative_to(repository).as_posix() for path in changed_paths]
    result: dict[str, Any] = {
        "status": "passed",
        "base": base,
        "base_ancestry": ancestry,
        "changed_files": changed,
        "check": check(repository),
        "lint": None,
    }
    if lint_required(repository, changed_paths):
        result["lint"] = lint(cli.settings(), since=base, changed_paths=changed_paths)
    path = repository / "build/reports/pr-check.json"
    atomic_json(path, result)
    cli.emit(
        {
            "status": result["status"],
            "base": base,
            "head": ancestry.get("head"),
            "changed_files": len(changed),
            "check": result["check"].get("status"),
            "lint": result["lint"].get("status") if result["lint"] is not None else "not-required",
            "report": str(path.relative_to(repository)),
        }
    )


def diagnostics_command(
    x64: Annotated[
        bool, typer.Option("--x64", help="Audit Windows pointer-width changes.")
    ] = False,
) -> None:
    """Emit non-gating recovery-relevant clang diagnostics."""
    from .. import command_support as cli
    from ..build import lint

    cli.emit(lint(cli.settings(), full_diagnostics=True, x64=x64))


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
    app.command("doctor")(doctor_command)
    app.command("prepare")(prepare_command)
    app.command("check")(check_command)
    app.command("lint")(lint_command)
    app.command("tidy-audit")(tidy_audit_command)
    app.command("pr-check")(pr_check_command)
    app.command("diagnostics")(diagnostics_command)
    app.command("build")(build_command)
    app.command("compare")(compare_command)
    app.command("vtable")(vtable_command)
    app.command("datacmp")(datacmp_command)
    app.command("addr")(address_command)
    app.command("runtime-test")(runtime_test_command)
    app.command("run")(run_command)
    app.command("debug")(debug_command)
    app.add_typer(analyze_app, name="analyze")
    analyze_app.command("crash")(crash_report_command)
    analyze_app.command("inventory")(inventory_command)
    analyze_app.command("trace")(trace_command)
    analyze_app.command("differential")(differential_command)
    analyze_app.command("freeze-oracle")(freeze_oracle_command)
    analyze_app.command("smoke")(smoke_command)
    analyze_app.command("source-layouts")(verify_source_layouts_command)
    analyze_app.command("source-index")(source_index_command)
    analyze_app.command("scalar-facts")(scalar_facts_command)
    analyze_app.command("scalar-evidence")(scalar_evidence_command)
    analyze_app.command("emissions")(emissions_command)
    analyze_app.command("decompiler-quality")(decompiler_quality_command)


def scalar_facts_command(
    evidence: Annotated[
        Path | None, typer.Option(help="Reviewed scalar-evidence-v1 claims.")
    ] = None,
    patch: Annotated[
        bool, typer.Option(help="Emit a reviewable patch; never edit source.")
    ] = False,
) -> None:
    """Collect the complete configured corpus and solve shared type constraints."""
    from .. import command_support as cli
    from ..build import scalar_campaign

    cli.emit(scalar_campaign(cli.settings(), evidence=evidence, patch=patch))


def scalar_evidence_command(
    campaign: Annotated[
        Path, typer.Option(help="Completed whole-program scalar campaign directory.")
    ],
    oracle: Annotated[
        Path | None, typer.Option(help="Pinned source-oracle correspondences.")
    ] = None,
    exports: Annotated[
        bool, typer.Option(help="Harvest already-paired canonical SurRender exports.")
    ] = False,
) -> None:
    """Parse independent declaration contracts into shared scalar evidence."""
    from .. import command_support as cli
    from ..build import scalar_evidence_campaign

    cli.emit(scalar_evidence_campaign(cli.settings(), campaign, oracle=oracle, exports=exports))


def emissions_command(
    derive: Annotated[
        bool, typer.Option(help="Resolve linker symbols through the built PDB/reccmp catalog.")
    ] = False,
    target: Annotated[str | None, typer.Option(help="Generate one configured target.")] = None,
) -> None:
    """Generate compiler/template emission CSVs outside authored source."""
    from .. import command_support as cli
    from ..emissions import generate_emissions

    cli.emit(
        generate_emissions(
            cli.settings().repo_dir,
            derive=derive,
            targets=(target.upper(),) if target else None,
        )
    )


def source_index_command(
    jobs: Annotated[
        int | None, typer.Option(min=1, help="Maximum native collector workers.")
    ] = None,
) -> None:
    """Generate build/source-index.json from reccmp markers and Clang AST."""
    from .. import command_support as cli
    from ..source_index import write_source_index

    cli.emit(write_source_index(cli.settings(), jobs=jobs))


def decompiler_quality_command(
    limit: Annotated[
        int,
        typer.Option(min=1, help="Maximum matched functions to decompile."),
    ] = 200,
    seed: Annotated[
        int,
        typer.Option(help="Stable corpus sample seed."),
    ] = 1,
    corpus_kind: Annotated[
        str,
        typer.Option(
            "--corpus-kind",
            help="oracle (exact|effective; default) or pain (any FUNCTION, stratified).",
        ),
    ] = "oracle",
    require_match: Annotated[
        bool | None,
        typer.Option(
            "--require-match/--any-recovered",
            help="Override corpus match filter. Default follows --corpus-kind.",
        ),
    ] = None,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Explicit corpus address; repeatable. Disables stratified sampling."),
    ] = None,
    profile: Annotated[
        str,
        typer.Option(help="Decompiler option profile: analysis (default), recovery, or program."),
    ] = "analysis",
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Score Ghidra decompiler debt on a high-confidence recovered corpus."""
    from .. import command_support as cli
    from ..decompiler_quality import run_decompiler_quality

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_decompiler_quality(
            cli.settings(),
            target=target,
            program_name=program,
            limit=limit,
            seed=seed,
            addresses=addresses,
            require_match=require_match,
            corpus_kind=corpus_kind,
            profile=profile,
        )

    cli.emit(action())


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


def crash_report_command(
    log: Annotated[
        Path,
        typer.Option("--log", exists=True, dir_okay=False, readable=True),
    ],
    link_map: Annotated[
        Path | None,
        typer.Option("--map", help="Linker MAP of the crashed runnable image."),
    ] = None,
    objects: Annotated[
        Path | None,
        typer.Option(help="Object root used to correlate unresolved externals."),
    ] = None,
) -> None:
    """Symbolize a captured runnable-image crash through its link MAP."""
    from .. import command_support as cli
    from ..runtime import analyze_runtime_crash

    def action() -> Any:
        settings = cli.settings()
        return analyze_runtime_crash(
            log,
            link_map or settings.product_build_dir / "Wiz8Runtime.map",
            objects or settings.recovered_objects_dir,
        )

    cli.emit(action())


def inventory_command() -> None:
    from .. import command_support as cli
    from ..binary.inventory import inventory

    cli.emit(inventory(cli.settings()))


def trace_command(
    scenario: Annotated[str, typer.Argument(help="bring-up, screens or load.")] = "bring-up",
    seconds: Annotated[int, typer.Option(help="How long to let the scenario run.")] = 120,
    port: Annotated[int | None, typer.Option(help="winedbg gdb proxy port.")] = None,
    executable: Annotated[str, typer.Option(help="Sandboxed image to trace.")] = "Wiz8.exe",
    link_map: Annotated[
        Path | None,
        typer.Option(
            "--link-map",
            exists=True,
            dir_okay=False,
            help="Rebuilt image's linker MAP; rebases the plan's retail addresses.",
        ),
    ] = None,
    save: Annotated[
        Path | None,
        typer.Option("--save", exists=True, dir_okay=False, help="Fixture save to stage."),
    ] = None,
    plan_only: Annotated[bool, typer.Option(help="Print the breakpoint plan only.")] = False,
) -> None:
    from .. import command_support as cli
    from ..dynamic import Sandbox, run_trace, trace_plan, write_report

    def action():
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
            executable=executable,
            link_map=link_map,
            save=save,
        )
        return write_report(result, settings.repo_dir / "build/reports/trace")

    cli.emit(action())


def differential_command(
    scenario: Annotated[str, typer.Argument(help="Scenario to compare.")] = "load",
    seconds: Annotated[int, typer.Option(help="How long to let each run go.")] = 120,
    executable: Annotated[
        str, typer.Option(help="Sandboxed rebuilt image to compare against retail.")
    ] = "Wiz8Runtime.exe",
    link_map: Annotated[
        Path | None,
        typer.Option(
            "--link-map",
            exists=True,
            dir_okay=False,
            help="Rebuilt image's linker MAP for breakpoint rebasing.",
        ),
    ] = None,
    save: Annotated[
        Path | None,
        typer.Option(
            "--save", exists=True, dir_okay=False, help="Shared fixture save for all runs."
        ),
    ] = None,
) -> None:
    """Retail repeatability first, then a retail/rebuilt event comparison.

    All three runs share one sandbox, one fixture save and one scenario, so
    the streams differ only in the image that produced them. Retail runs
    twice first: a mismatch retail cannot reproduce against itself is a
    repeatability finding, not a rebuild defect."""
    from .. import command_support as cli
    from ..dynamic import (
        SCENARIO_TERMINAL,
        Event,
        Sandbox,
        compare_states,
        compare_streams,
        run_trace,
        write_report,
    )

    def action():
        settings = cli.settings()
        sandbox = Sandbox.from_environment()
        reports = settings.repo_dir / "build/reports/trace"
        if link_map is None:
            candidate = settings.product_build_dir / "Wiz8Runtime.map"
            resolved_map = candidate if candidate.is_file() else None
        else:
            resolved_map = link_map
        # The rebuilt image gets retail breakpoints without its own MAP;
        # tracing that is silent nonsense, so it fails before any run.
        if resolved_map is None:
            raise RuntimeError(f"no linker MAP for {executable}; pass --link-map or build it")

        runs = {}
        capture_receipts = {}
        for label, image, plan_map in (
            ("retail-a", "Wiz8.exe", None),
            ("retail-b", "Wiz8.exe", None),
            ("recomp", executable, resolved_map),
        ):
            result = run_trace(
                settings.repo_dir,
                sandbox,
                scenario,
                seconds=seconds,
                executable=image,
                link_map=plan_map,
                save=save,
            )
            runs[label] = result
            capture_receipts[label] = write_report(
                {**result, "scenario": f"{scenario}-{label}"}, reports
            )

        def events(run: dict) -> list[Event]:
            return [
                Event(
                    order=event["order"],
                    kind=event["kind"],
                    name=event["name"],
                    address=event["address"],
                )
                for event in run["events"]
            ]

        # The scenario's semantic claim ends at its declared terminal event:
        # after it the game sits in its steady frame loop, and how many
        # frames a run captured before the timeout is capture noise, not
        # behavior. The bounded comparison is therefore the verdict; the raw
        # streams stay in the reports and the post-terminal event set
        # confirms each run still reached the same steady state. A run that
        # never reaches the terminal event compares in full.
        terminal = SCENARIO_TERMINAL.get(scenario)

        def bound(evts: list[Event]) -> int:
            if terminal is None:
                return len(evts)
            for index, event in enumerate(evts):
                if event.name == terminal:
                    return index + 1
            return len(evts)

        streams = {label: events(run) for label, run in runs.items()}
        bounds = {label: bound(stream) for label, stream in streams.items()}
        repeatability = compare_streams(streams["retail-a"], streams["retail-b"])
        differential = compare_streams(streams["retail-a"], streams["recomp"])
        bounded_repeatability = compare_streams(
            streams["retail-a"][: bounds["retail-a"]],
            streams["retail-b"][: bounds["retail-b"]],
        )
        bounded_differential = compare_streams(
            streams["retail-a"][: bounds["retail-a"]],
            streams["recomp"][: bounds["recomp"]],
        )
        # The checkpoint fingerprint runs through the same verdicts: the
        # state the terminal event left behind must reproduce within retail
        # before the cross-build comparison can claim equivalence.
        states = {label: run.get("state", {}) for label, run in runs.items()}
        state_repeatability = compare_states(states["retail-a"], states["retail-b"])
        state_differential = compare_states(states["retail-a"], states["recomp"])
        # An affirmative verdict needs every precondition, not just equal
        # prefixes: each run must have started under the debugger and reached
        # the scenario's terminal event, retail must be repeatable against
        # itself, and every watched point must exist in the rebuilt image.
        reached_terminal = {
            label: terminal is None or any(e.name == terminal for e in stream)
            for label, stream in streams.items()
        }
        capture_completed = {
            label: run["timed_out"] or (terminal is not None and run["capture_returncode"] == 0)
            for label, run in runs.items()
        }
        unwatched = {label: run["provenance"].get("unwatched", []) for label, run in runs.items()}
        requirements = {
            "all_started": all(run["started"] for run in runs.values()),
            "no_capture_failures": all(capture_completed.values()),
            "all_reached_terminal": all(reached_terminal.values()),
            "retail_repeatable": bounded_repeatability["agrees"],
            "no_unwatched_points": not any(unwatched.values()),
            "streams_agree": bounded_differential["agrees"],
            "state_repeatable": state_repeatability["agrees"],
            "state_agrees": state_differential["agrees"],
        }
        return {
            "scenario": scenario,
            "affirmative": all(requirements.values()),
            "requirements": requirements,
            "reached_terminal": reached_terminal,
            "capture": {
                label: {
                    "returncode": run["capture_returncode"],
                    "timed_out": run["timed_out"],
                    "ok": capture_completed[label],
                }
                for label, run in runs.items()
            },
            "state": {
                "retail_repeatability": state_repeatability,
                "differential": state_differential,
            },
            "bounded": {
                "events": bounds,
                "retail_repeatability": bounded_repeatability,
                "differential": bounded_differential,
                "terminal": terminal,
                "post_terminal_events": {
                    label: sorted({e.name for e in stream[bounds[label] :]})
                    for label, stream in streams.items()
                },
            },
            "retail_repeatability": repeatability,
            "differential": differential,
            "runs": {
                label: {
                    "events": len(run["events"]),
                    "started": run["started"],
                    "state": states[label],
                    "provenance": run["provenance"],
                    "capture_sha256": capture_receipts[label]["capture_sha256"],
                }
                for label, run in runs.items()
            },
        }

    cli.emit(action())


def freeze_oracle_command(
    report: Annotated[Path, typer.Argument(exists=True, dir_okay=False)],
    output: Annotated[Path, typer.Option("--output", help="New reference JSON file.")],
    captures: Annotated[Path, typer.Option("--captures", exists=True, file_okay=False)] = Path(
        "build/reports/trace"
    ),
) -> None:
    """Freeze a successful saved differential and its provenance-complete captures."""
    from .. import command_support as cli
    from ..dynamic import freeze_reference

    cli.emit(freeze_reference(report, captures, output))


def smoke_command(
    seconds: Annotated[int, typer.Option(help="How long to let the run go.")] = 120,
    executable: Annotated[
        str, typer.Option(help="Sandboxed product image to exercise.")
    ] = "Wiz8Runtime.exe",
    link_map: Annotated[
        Path | None,
        typer.Option(
            "--link-map",
            exists=True,
            dir_okay=False,
            help="Rebuilt image's linker MAP for breakpoint rebasing.",
        ),
    ] = None,
) -> None:
    """The product's own entry/exit lifecycle: WinMain, menu, player quit,
    process exit - the path the runtime-test executable's explicit SGPExit
    plus TerminateProcess does not exercise."""
    from .. import command_support as cli
    from ..dynamic import Sandbox, run_smoke, write_report

    def action():
        settings = cli.settings()
        sandbox = Sandbox.from_environment()
        if link_map is None:
            candidate = settings.product_build_dir / "Wiz8Runtime.map"
            resolved_map = candidate if candidate.is_file() else None
        else:
            resolved_map = link_map
        result = run_smoke(
            settings.repo_dir,
            sandbox,
            seconds=seconds,
            executable=executable,
            link_map=resolved_map,
        )
        write_report({**result, "scenario": "smoke"}, settings.repo_dir / "build/reports/trace")
        return result

    cli.emit(action())


def verify_source_layouts_command(
    pdb: Annotated[
        Path | None,
        typer.Option("--pdb", exists=True, dir_okay=False, readable=True),
    ] = None,
) -> None:
    """Compare compiled source layouts with Ghidra types; report disagreements."""
    from .. import command_support as cli
    from ..source_layouts import verify_source_layouts

    def action():
        return verify_source_layouts(cli.settings(), pdb)

    cli.emit(action())
