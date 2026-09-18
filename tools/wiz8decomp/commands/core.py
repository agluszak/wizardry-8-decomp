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


def prepare_command() -> None:
    """Idempotently prepare extracted variants and pinned source dependencies."""
    from .. import command_support as cli
    from ..build import prepare

    cli.emit(prepare(cli.settings()))


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


def pr_check_command(
    base: Annotated[str, typer.Option("--base", help="PR base revision.")] = "main@origin",
) -> None:
    """Run every validation lane required by the files changed in a PR."""
    from .. import command_support as cli
    from ..build import check, lint, lint_required
    from ..comparison import changed_files
    from ..config import repository_root

    repository = repository_root()
    changed_paths = changed_files(repository, base)
    changed = [path.relative_to(repository).as_posix() for path in changed_paths]
    result: dict[str, Any] = {
        "status": "passed",
        "base": base,
        "changed_files": changed,
        "check": check(repository),
        "lint": None,
    }
    if lint_required(repository, changed_paths):
        result["lint"] = lint(cli.settings(), since=base, changed_paths=changed_paths)
    cli.emit(result)


def diagnostics_command() -> None:
    """Emit non-gating recovery-relevant clang diagnostics."""
    from .. import command_support as cli
    from ..build import lint

    cli.emit(lint(cli.settings(), full_diagnostics=True))


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
        False, "--changed", help="Compare changed C/C++ files and consumers of changed headers."
    ),
    since: Annotated[
        str | None,
        typer.Option("--since", help="With --changed, compare files changed since this revision."),
    ] = None,
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    build: Annotated[
        bool, typer.Option("--build", help="Build fresh products and metadata before comparing.")
    ] = False,
) -> None:
    """Compare existing products; optionally build fresh products first."""
    from .. import command_support as cli
    from ..build import build_target
    from ..comparison import (
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
        if ctx.args:
            raise ValueError("raw reccmp options are not accepted by selected comparison")
        if addresses or files or changed:
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
            needs_index = bool(selected_files) or selectors_require_source_index(addresses or [])
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
                        "run `uv run wiz8 analyze source-index`"
                    )
                dependent_files = header_dependent_files(settings, target, changed_files)
                selected_files.extend(dependent_files)
            selected = selected_addresses(
                settings.repo_dir, target, addresses or [], selected_files
            )
            result = compare_selected(
                settings.repo_dir,
                target,
                selected,
                include_windows=True,
                classify_header_emissions=needs_index,
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
        raise ValueError("select functions by address, --file, or --changed")

    cli.emit(action())


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
            build_target(settings, target)
        result = translate_addresses(settings.repo_dir, target, queries)
        return result

    cli.emit(action())


def runtime_test_command(
    scenario: Annotated[
        list[str] | None,
        typer.Option("--scenario", help="Run a named scenario; repeat to select several."),
    ] = None,
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
) -> None:
    """Run deterministic in-process semantic scenarios using the existing product."""
    from .. import command_support as cli
    from ..build import build_target, warn_if_product_may_be_stale
    from ..runtime import RUNTIME_SCENARIOS, run_runtime_suite

    if scenario and (unknown := set(scenario) - set(RUNTIME_SCENARIOS)):
        raise typer.BadParameter(f"unknown runtime scenarios: {', '.join(sorted(unknown))}")
    settings = cli.settings()
    if build:
        build_target(settings, "runtime-test")
    warn_if_product_may_be_stale(settings, "runtime-test")
    cli.emit(
        run_runtime_suite(
            settings,
            scenarios=tuple(dict.fromkeys(scenario)) if scenario else RUNTIME_SCENARIOS,
            check_order=check_order,
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
    analyze_app.command("unresolved")(unresolved_report_command)
    analyze_app.command("crash")(crash_report_command)
    analyze_app.command("inventory")(inventory_command)
    analyze_app.command("trace")(trace_command)
    analyze_app.command("source-layouts")(verify_source_layouts_command)
    analyze_app.command("source-index")(source_index_command)
    analyze_app.command("decompiler-quality")(decompiler_quality_command)
    analyze_app.command("prototype-repair")(prototype_repair_command)
    analyze_app.command("enrichment-checkpoint")(enrichment_checkpoint_command)
    analyze_app.command("enrichment-promote")(enrichment_promote_command)
    analyze_app.command("class-this-typing")(class_this_typing_command)
    analyze_app.command("class-structures")(class_structures_command)
    analyze_app.command("global-typing")(global_typing_command)
    analyze_app.command("callback-typing")(callback_typing_command)
    analyze_app.command("function-attributes")(function_attributes_command)
    analyze_app.command("vftable-typing")(vftable_typing_command)
    analyze_app.command("cosmic-forge-globals")(cosmic_forge_globals_command)
    analyze_app.command("convention-heuristics")(convention_heuristics_command)


def source_index_command() -> None:
    """Generate build/source-index.json from reccmp markers and Clang AST."""
    from .. import command_support as cli
    from ..source_index import write_source_index

    cli.emit(write_source_index(cli.settings()))


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


def prototype_repair_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write source-backed conventions into the live program."),
    ] = False,
    promote_default_cdecl: Annotated[
        bool,
        typer.Option(
            "--promote-default-cdecl/--skip-default-cdecl",
            help="Also promote Ghidra default/unknown to explicit __cdecl when source says so.",
        ),
    ] = True,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Limit to these addresses; repeatable."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Report or apply calling-convention repairs from recovered source signatures."""
    from .. import command_support as cli
    from ..prototype_repair import run_prototype_repair

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_prototype_repair(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            addresses=addresses,
            promote_default_cdecl=promote_default_cdecl,
        )

    cli.emit(action())


def enrichment_checkpoint_command(
    apply_conventions: Annotated[
        bool,
        typer.Option(
            "--apply-conventions",
            help="Apply source-backed calling conventions (disposable project unless --live).",
        ),
    ] = False,
    apply_enrichment: Annotated[
        bool,
        typer.Option(
            "--apply-enrichment",
            help=(
                "Apply source-safe enrichment stages (class/this/vftable/globals/"
                "callbacks/CF/attributes; thunks stay off). Uses a disposable "
                "restored project unless --live. Promote accepted candidates with "
                "`wiz8 analyze enrichment-promote`."
            ),
        ),
    ] = False,
    import_source: Annotated[
        bool,
        typer.Option(
            "--import-source",
            help=(
                "Run reccmp-ghidra-import (requires build/PDB). Uses a disposable "
                "restored project unless --live."
            ),
        ),
    ] = False,
    live: Annotated[
        bool,
        typer.Option(
            "--live",
            help=(
                "Opt-in: apply mutations to the canonical checkout Ghidra project "
                "instead of a disposable restore under work_dir/enrichment-checkpoint/. "
                "Prefer disposable apply + enrichment-promote for accepted candidates."
            ),
        ),
    ] = False,
    skip_quality: Annotated[
        bool,
        typer.Option("--skip-quality", help="Skip the matched-function decompiler-quality sample."),
    ] = False,
    measure_pain: Annotated[
        bool,
        typer.Option(
            "--measure-pain",
            help=(
                "Also score a pinned pain corpus (mismatch|inconclusive) before/after enrichment."
            ),
        ),
    ] = False,
    quality_limit: Annotated[
        int,
        typer.Option(min=1, help="Matched functions in the quality sample."),
    ] = 50,
    quality_seed: Annotated[int, typer.Option(help="Quality corpus seed.")] = 1,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Measure (and optionally apply) a high-confidence analysis enrichment checkpoint."""
    from .. import command_support as cli
    from ..enrichment_checkpoint import run_enrichment_checkpoint

    payload = run_enrichment_checkpoint(
        cli.settings(),
        program_name=program,
        target=target,
        quality_limit=quality_limit,
        quality_seed=quality_seed,
        apply_conventions=apply_conventions,
        apply_enrichment=apply_enrichment,
        import_source=import_source,
        measure_quality=not skip_quality,
        measure_pain=measure_pain,
        live=live,
    )
    cli.emit(payload)
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


def enrichment_promote_command(
    run_dir: Annotated[
        Path | None,
        typer.Argument(
            help=(
                "Disposable enrichment-checkpoint run directory containing report.json "
                "and ghidra-project/."
            ),
        ),
    ] = None,
    from_latest: Annotated[
        bool,
        typer.Option(
            "--from-latest",
            help=(
                "Promote the newest work_dir/enrichment-checkpoint/run-* whose report "
                "has safe_application and preserved_recovery."
            ),
        ),
    ] = False,
    force: Annotated[
        bool,
        typer.Option(
            "--force",
            help="Replace live project even when seed freshness is stale/untracked/unknown.",
        ),
    ] = False,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
) -> None:
    """Promote a tested disposable enrichment candidate into the checkout live project."""
    from .. import command_support as cli
    from ..enrichment_promote import run_enrichment_promote

    payload = run_enrichment_promote(
        cli.settings(),
        run_dir=run_dir,
        from_latest=from_latest,
        force=force,
        program_name=program,
    )
    cli.emit(payload)
    if payload.get("ok") is False:
        raise typer.Exit(code=1)


def class_this_typing_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write Class* this types into the live program."),
    ] = False,
    allow_custom_storage: Annotated[
        bool,
        typer.Option(
            "--allow-custom-storage",
            help="Enable custom variable storage when auto-parameter this cannot be typed.",
        ),
    ] = False,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Limit to these addresses; repeatable."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Bind source __thiscall methods to the class Structure via GhidraClass."""
    from .. import command_support as cli
    from ..class_this_typing import run_class_this_typing

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_class_this_typing(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            allow_custom_storage=allow_custom_storage,
            addresses=addresses,
        )

    cli.emit(action())


def class_structures_command(
    apply: Annotated[
        bool,
        typer.Option(
            "--apply",
            help="Bind or create opaque class Structures in the live program.",
        ),
    ] = False,
    skip_this_typing: Annotated[
        bool,
        typer.Option(
            "--skip-this-typing",
            help="Do not chain class-this-typing after a successful apply.",
        ),
    ] = False,
    class_name: Annotated[
        list[str] | None,
        typer.Option("--class", help="Limit to these owning class names; repeatable."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Reconcile source class Structures with native GhidraClass binding."""
    from .. import command_support as cli
    from ..class_structure_projection import run_class_structure_projection

    def action():
        return run_class_structure_projection(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            class_names=class_name,
            type_this=not skip_this_typing,
        )

    cli.emit(action())


def global_typing_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write source-backed GLOBAL types into the live listing."),
    ] = False,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Limit to these addresses; repeatable."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Type recovered GLOBAL markers in the canonical Ghidra listing."""
    from .. import command_support as cli
    from ..global_typing import run_global_typing

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_global_typing(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            addresses=addresses,
        )

    cli.emit(action())


def callback_typing_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write curated callback field types into the live program."),
    ] = False,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
) -> None:
    """Create typed callback FunctionDefinitions and apply them to known fields."""
    from .. import command_support as cli
    from ..callback_typing import run_callback_typing

    def action():
        return run_callback_typing(cli.settings(), program_name=program, apply=apply)

    cli.emit(action())


def function_attributes_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write varargs/noreturn attributes into the live program."),
    ] = False,
    apply_thunks: Annotated[
        bool,
        typer.Option(
            "--apply-thunks",
            help="Write planned pure-JMP / ECX-adjustor thunks into the live program.",
        ),
    ] = False,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Limit to these addresses; repeatable."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Report or apply source-backed varargs/noreturn; thunks need --apply-thunks."""
    from .. import command_support as cli
    from ..function_attributes import run_function_attributes

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_function_attributes(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            apply_thunks=apply_thunks,
            addresses=addresses,
        )

    cli.emit(action())


def vftable_typing_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write typed vftable Structures into the live program."),
    ] = False,
    class_name: Annotated[
        list[str] | None,
        typer.Option("--class", help="Limit to these class names; repeatable."),
    ] = None,
    limit: Annotated[
        int | None,
        typer.Option(min=1, help="Optional cap on planned vftables."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Create named vftable Structures at known class vtable addresses."""
    from .. import command_support as cli
    from ..vftable_typing import run_vftable_typing

    def action():
        return run_vftable_typing(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            class_names=class_name,
            limit=limit,
        )

    cli.emit(action())


def cosmic_forge_globals_command(
    apply: Annotated[
        bool,
        typer.Option("--apply", help="Write Cosmic Forge table types into the live listing."),
    ] = False,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
) -> None:
    """Type accepted Cosmic Forge .cfdat destination blobs from reviewed evidence."""
    from .. import command_support as cli
    from ..cosmic_forge_globals import run_cosmic_forge_globals

    def action():
        return run_cosmic_forge_globals(cli.settings(), program_name=program, apply=apply)

    cli.emit(action())


def convention_heuristics_command(
    apply: Annotated[
        bool,
        typer.Option(
            "--apply",
            help="Write high-confidence heuristic conventions into the live program.",
        ),
    ] = False,
    address: Annotated[
        list[str] | None,
        typer.Option(help="Limit to these addresses; repeatable."),
    ] = None,
    limit: Annotated[
        int | None,
        typer.Option(min=1, help="Optional cap on planned repairs."),
    ] = None,
    program: Annotated[str, typer.Option(help="Ghidra program selector.")] = "wiz8",
    target: Annotated[str, typer.Option(help="reccmp target id.")] = "WIZ8",
) -> None:
    """Heuristic calling conventions for unrecovered bodies (vtable/ECX/ret N)."""
    from .. import command_support as cli
    from ..convention_heuristics import run_convention_heuristics

    def action():
        addresses = [int(value, 0) for value in address] if address else None
        return run_convention_heuristics(
            cli.settings(),
            target=target,
            program_name=program,
            apply=apply,
            addresses=addresses,
            limit=limit,
        )

    cli.emit(action())


def unresolved_report_command(
    objects: Annotated[Path | None, typer.Option(help="Object root.")] = None,
    link_map: Annotated[Path | None, typer.Option(help="Linker MAP.")] = None,
) -> None:
    from .. import command_support as cli
    from ..unresolved import unresolved_report

    def action():
        settings = cli.settings()
        report = unresolved_report(
            objects or settings.recovered_objects_dir,
            link_map or settings.product_build_dir / "Wiz8.map",
        )
        return report

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
    from ..runtime import RUNTIME_SCENARIOS

    settings = cli.settings()
    if scenario is not None and arguments:
        raise ValueError("runtime product arguments cannot be combined with --scenario")
    if scenario is not None and scenario not in RUNTIME_SCENARIOS:
        raise ValueError(f"unknown runtime scenario: {scenario}")
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
