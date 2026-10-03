from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("field-uses")
def field_uses_command(
    owners: Annotated[list[str], typer.Argument(help="Exact modeled type names or paths.")],
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    directory: Annotated[Path | None, typer.Option(help="Saved comparison directory.")] = None,
) -> None:
    """Census retail field uses by owner; reads Ghidra and saved comparisons."""
    from .. import command_support as cli
    from ..comparison import report_directory
    from ..ghidra.env import open_program
    from ..ghidra.field_uses import field_uses
    from ..source_index import target_for_program

    settings = cli.settings()
    target = target_for_program(settings.repo_dir, program)
    saved = directory or report_directory(settings.repo_dir, target)
    with open_program(settings, program) as live:
        payload = field_uses(live, settings.repo_dir, owners, saved)
    cli.emit(payload)


@app.command("signature-census")
def signature_census_command(
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    directory: Annotated[Path | None, typer.Option(help="Saved comparison directory.")] = None,
) -> None:
    """Join callee signatures and provenance; never rebuilds or compares binaries."""
    from .. import command_support as cli
    from ..comparison import report_directory
    from ..ghidra.env import open_program
    from ..ghidra.signature_census import signature_census
    from ..source_index import target_for_program

    settings = cli.settings()
    target = target_for_program(settings.repo_dir, program)
    saved = directory or report_directory(settings.repo_dir, target)
    with open_program(settings, program) as live:
        payload = signature_census(live, settings.repo_dir, saved)
    cli.emit(payload)


@app.command("compare")
def comparison_command(
    addresses: Annotated[
        list[str] | None, typer.Argument(help="Original addresses to inspect.")
    ] = None,
    program: Annotated[str, typer.Option("--program")] = "wiz8",
    report: Annotated[
        Path | None, typer.Option(help="Saved summary.json; defaults to the latest run.")
    ] = None,
    against: Annotated[
        Path | None, typer.Option(help="Earlier summary.json; show changed results only.")
    ] = None,
    files: Annotated[
        list[Path] | None, typer.Option("--file", help="Restrict to these source owners.")
    ] = None,
    outcome: Annotated[str | None, typer.Option(help="Filter by reccmp outcome.")] = None,
    limit: Annotated[int, typer.Option(min=1, max=100, help="Maximum function rows.")] = 20,
    diff_lines: Annotated[
        int, typer.Option(min=0, max=200, help="Total code-diff line budget.")
    ] = 0,
) -> None:
    """Inspect a saved comparison without compiling or starting Ghidra."""
    from .. import command_support as cli
    from ..comparison import parse_address
    from ..reports.comparison import comparison_report
    from ..source_index import target_for_program

    settings = cli.settings()
    cli.emit(
        comparison_report(
            settings.repo_dir,
            target_for_program(settings.repo_dir, program),
            report=report,
            against=against,
            addresses=[parse_address(address) for address in addresses or []],
            files=files,
            outcome=outcome,
            limit=limit,
            diff_lines=diff_lines,
        )
    )


@app.command("status")
def status_command(
    build: Annotated[
        bool, typer.Option("--build", help="Build products before reporting.")
    ] = False,
) -> None:
    """Report source coverage and reccmp pairing; never runs a comparison."""
    from .. import command_support as cli
    from ..reports.status import status_report

    settings = cli.settings()
    if build:
        from ..build import build_target

        build_target(settings, "reccmp-products")
    cli.emit(status_report(settings))


_INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)


def _includes_directly(source: Path, header: str) -> bool:
    """Whether `source` names repository header `header` in an #include line.

    The dependents come from the base source index, so a source this change
    deletes is gone from the head checkout and includes nothing there."""
    if not source.is_file():
        return False
    text = source.read_text(encoding="utf-8", errors="replace")
    return any(
        header.endswith("/" + included.replace("\\", "/").lstrip("./"))
        for included in _INCLUDE.findall(text)
    )


@app.command("pr-comparison")
def pr_comparison_command(
    target: Annotated[str, typer.Option("--target")],
    head_status: Annotated[Path, typer.Option("--head-status")],
    base_status: Annotated[Path, typer.Option("--base-status")],
    head_summary: Annotated[Path | None, typer.Option("--head-summary")] = None,
    base_summary: Annotated[Path | None, typer.Option("--base-summary")] = None,
    head_ghidriff: Annotated[Path | None, typer.Option("--head-ghidriff")] = None,
    base_ghidriff: Annotated[Path | None, typer.Option("--base-ghidriff")] = None,
    head_datacmp: Annotated[Path | None, typer.Option("--head-datacmp")] = None,
    base_datacmp: Annotated[Path | None, typer.Option("--base-datacmp")] = None,
    head_direct_calls: Annotated[Path | None, typer.Option("--head-direct-calls")] = None,
    base_direct_calls: Annotated[Path | None, typer.Option("--base-direct-calls")] = None,
    since: Annotated[
        str | None,
        typer.Option(
            "--since",
            help="Merge base: group regressions by the changed headers their files include directly.",
        ),
    ] = None,
) -> None:
    """Summarize PR-head comparison health and its change from the merge base."""

    from .. import command_support as cli
    from ..reports.comparison_delta import pr_comparison_report

    header_includers = None
    if since is not None:
        from ..comparison import changed_files, header_dependent_files

        settings = cli.settings()
        repository = settings.repo_dir.resolve()
        header_includers = {}
        for path in changed_files(repository, since):
            if path.suffix.lower() not in {".h", ".hpp", ".hxx"} or not path.is_file():
                continue
            header = path.resolve().relative_to(repository).as_posix()
            dependents = header_dependent_files(settings, target, [path])
            header_includers[header] = {str(path.resolve())} | {
                str(file.resolve()) for file in dependents if _includes_directly(file, header)
            }

    cli.emit(
        pr_comparison_report(
            target,
            head_status,
            base_status,
            head_summary_path=head_summary,
            base_summary_path=base_summary,
            head_ghidriff_path=head_ghidriff,
            base_ghidriff_path=base_ghidriff,
            head_datacmp_path=head_datacmp,
            base_datacmp_path=base_datacmp,
            head_direct_calls_path=head_direct_calls,
            base_direct_calls_path=base_direct_calls,
            header_includers=header_includers,
        )
    )


@app.command("surrender-frontier")
def surrender_frontier_command(
    class_filter: str | None = typer.Option(
        None, "--class", help="Restrict the per-import listing to one SurRender class."
    ),
    priority: str | None = typer.Option(
        None, "--priority", help="Restrict the per-import listing to P0/P3."
    ),
    compare: bool = typer.Option(
        False,
        "--compare/--no-compare",
        help="Annotate recovered bodies with reccmp's comparison outcome (runs Ghidra).",
    ),
) -> None:
    """Rank SurRender provider work by what Wiz8 actually references."""

    from .. import command_support as cli
    from ..reports.surrender_frontier import surrender_frontier_report

    cli.emit(
        surrender_frontier_report(
            cli.settings(),
            class_filter=class_filter,
            priority_filter=priority,
            compare=compare,
        )
    )


@app.command("header-architecture")
def header_architecture_command() -> None:
    """Write the header-role ownership report from recovered TUs and declarations."""

    from .. import command_support as cli
    from ..header_architecture import write_header_architecture_report

    cli.emit(write_header_architecture_report(cli.settings().repo_dir))


@app.command("semantic-debt")
def semantic_debt_command(
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Rank provisional recovery work without turning it into a gate."""

    from .. import command_support as cli
    from ..reports.semantic_debt import semantic_debt_report
    from ..source_index import target_for_program, warn_if_source_index_may_be_stale

    settings = cli.settings()
    target = target_for_program(settings.repo_dir, program)
    warn_if_source_index_may_be_stale(settings.repo_dir, target)
    cli.emit(semantic_debt_report(settings.repo_dir, target))


@app.command("retail-folded")
def retail_folded_command() -> None:
    """Report reviewed retail folds that released-source bodies must not override."""

    from .. import command_support as cli
    from ..evidence.claims import load_claims

    claims = load_claims(cli.settings().repo_dir)
    cli.emit(
        {
            "informational": True,
            "policy": (
                "Retail call sites and bodies govern fidelity; fold claims are evidence only, "
                "do not imply a no-op, and do not create source aliases or FOLDED markers."
            ),
            "functions": [
                {
                    "address": f"0x{claim['entity_key']}",
                    "name": claim["value"],
                    "classification": claim["predicate"],
                    "reference": claim["reference"],
                    "details": claim["details"],
                }
                for claim in claims
                if claim["predicate"] in {"retail-folded", "retail-folded-noop"}
            ],
        }
    )


@app.command("source-oracle")
def source_oracle_command() -> None:
    """Report proven available-source ownership and fail on Wizardry misplaced recoveries."""

    from .. import command_support as cli
    from ..source_index import warn_if_source_index_may_be_stale
    from ..source_oracle import source_oracle_report

    settings = cli.settings()
    warn_if_source_index_may_be_stale(settings.repo_dir, "WIZ8")
    report = source_oracle_report(settings.repo_dir)
    cli.emit(report)
    if report["status"] != "passed":
        raise typer.Exit(code=1)


@app.command("semantic-names")
def semantic_names_command() -> None:
    """Rank frequently referenced FunctionXXXXXXXX declarations for recovery."""

    from .. import command_support as cli
    from ..reports.semantic_debt import semantic_name_opportunity_report
    from ..source_index import warn_if_source_index_may_be_stale

    settings = cli.settings()
    warn_if_source_index_may_be_stale(settings.repo_dir, "WIZ8")
    cli.emit(semantic_name_opportunity_report(settings.repo_dir))


@app.command("merge-preservation")
def merge_preservation_command(
    base: Annotated[str, typer.Option("--base", help="Base revision, e.g. origin/main.")],
    head: str | None = typer.Option(
        None,
        "--head",
        help="Explicit result revision; defaults to the current Jujutsu change or Git working tree.",
    ),
) -> None:
    """Compare FUNCTION/GLOBAL/VTABLE identities by retail address between two revisions."""

    from .. import command_support as cli
    from ..merge_preservation import base_ancestry_report, merge_preservation_report

    repository = cli.settings().repo_dir
    ancestry = base_ancestry_report(repository, base, head)
    if ancestry["status"] != "passed":
        cli.emit({"status": "failed", "base": base, "base_ancestry": ancestry})
        raise typer.Exit(code=1)
    report = merge_preservation_report(
        repository,
        ancestry["base"],
        ancestry["head"] if head is not None else None,
    )
    report["requested_base"] = base
    report["base_ancestry"] = ancestry
    cli.emit(report)
    if report["status"] != "passed":
        raise typer.Exit(code=1)


@app.command("translation-units")
def translation_units_command() -> None:
    """Generate source ownership and hard-hull projections from the live layout."""

    from .. import command_support as cli
    from ..ghidra.unit_intervals import translation_unit_layout
    from ..reports.translation_units import translation_unit_report

    settings = cli.settings()
    layout = translation_unit_layout(settings)
    cli.emit(translation_unit_report(settings, layout=layout))


@app.command("mismatch-clusters")
def mismatch_clusters_command(
    directory: Annotated[
        Path | None, typer.Option("--directory", help="Existing WIZ8 comparison report directory.")
    ] = None,
) -> None:
    """Cluster existing Ghidriff differences and catalog direct-call observations."""
    from .. import command_support as cli
    from ..comparison import report_directory
    from ..reports.mismatch_clusters import mismatch_clusters

    repository = cli.settings().repo_dir
    cli.emit(mismatch_clusters(repository, directory or report_directory(repository, "WIZ8")))
