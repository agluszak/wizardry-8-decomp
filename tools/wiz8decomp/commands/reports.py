from __future__ import annotations

from typing import Annotated, Any

import typer

from ..config import Settings

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)

_SEMANTIC_FLOW_REQUESTS = {
    "WIZ8": (
        {
            "case": "S05 polygon-normal consumer",
            "selector": "0x00470380",
            "root": "3",
            "source_parameter_index": 2,
        },
        {
            "case": "E03 status-tail scope",
            "selector": "0x004FA4D0",
            "root": "party_slot",
            "source_parameter_index": 0,
        },
        {
            "case": "E01 scene traversal receiver",
            "selector": "0x00426500",
            "root": "scene",
            "source_parameter_index": 0,
        },
    ),
    "SURRENDER": (
        {"case": "S01 packed texture state consumer", "selector": "0x10028FB0", "root": "this"},
        {
            "case": "S01 correction default setter",
            "selector": "0x10018550",
            "root": "this",
        },
        {
            "case": "S01 magnification default setter",
            "selector": "0x10018650",
            "root": "this",
        },
        {
            "case": "S01 minification default setter",
            "selector": "0x100186D0",
            "root": "this",
        },
    ),
}


def _semantic_debt_field_flows(
    settings: Settings, program: str, target: str
) -> list[dict[str, Any]]:
    from ..ghidra.env import open_program
    from ..ghidra.semantic import field_accesses

    requests = _SEMANTIC_FLOW_REQUESTS.get(target.upper(), ())
    results = []
    with open_program(settings, program) as live:
        for request in requests:
            results.append(
                {
                    **request,
                    "flow": field_accesses(live, request["selector"], request["root"]),
                }
            )
    return results


@app.command("status")
def status_command(
    build: Annotated[
        bool, typer.Option("--build", help="Build products before reporting.")
    ] = False,
) -> None:
    """Report project-wide decomp and matching progress."""
    from .. import command_support as cli
    from ..reports.status import status_report

    settings = cli.settings()
    if build:
        from ..build import build_target

        build_target(settings, "reccmp-products")
    cli.emit(status_report(settings))


@app.command("surrender-frontier")
def surrender_frontier_command(
    class_filter: str | None = typer.Option(
        None, "--class", help="Restrict the per-import listing to one SurRender class."
    ),
    priority: str | None = typer.Option(
        None, "--priority", help="Restrict the per-import listing to P0/P3."
    ),
    compare: bool = typer.Option(
        True, "--compare/--no-compare", help="Annotate recovered bodies with reccmp status."
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
    with_flow: bool = typer.Option(
        False,
        "--with-flow",
        help="Join bounded live Ghidra field flows to compiler source layouts.",
    ),
) -> None:
    """Rank provisional recovery work without turning it into a gate."""

    from .. import command_support as cli
    from ..reports.semantic_debt import semantic_debt_report
    from ..source_index import target_for_program, warn_if_source_index_may_be_stale

    settings = cli.settings()
    target = target_for_program(settings.repo_dir, program)
    warn_if_source_index_may_be_stale(settings.repo_dir, target)
    field_flows = _semantic_debt_field_flows(settings, program, target) if with_flow else None
    cli.emit(semantic_debt_report(settings.repo_dir, target, field_flows=field_flows))


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
    allow: Annotated[
        list[str] | None,
        typer.Option(
            "--allow",
            help="TARGET:KIND:0xADDRESS:TRANSITION=reason for an intentional loss, duplicate, or demotion.",
        ),
    ] = None,
) -> None:
    """Compare FUNCTION/GLOBAL/VTABLE marker identities by retail address between two revisions."""

    from .. import command_support as cli
    from ..merge_preservation import (
        base_ancestry_report,
        merge_preservation_report,
        parse_allowed,
    )

    repository = cli.settings().repo_dir
    ancestry = base_ancestry_report(repository, base, head)
    if ancestry["status"] != "passed":
        cli.emit({"status": "failed", "base": base, "base_ancestry": ancestry})
        raise typer.Exit(code=1)
    report = merge_preservation_report(
        repository,
        ancestry["base"],
        ancestry["head"] if head is not None else None,
        parse_allowed(allow or []),
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


@app.command("placement-outliers")
def placement_outliers_command(
    min_gap: int = typer.Option(
        0x100000,
        "--min-gap",
        help="Minimum |address - TU median| in bytes before a FUNCTION is listed.",
    ),
    min_peers: int = typer.Option(
        3,
        "--min-peers",
        help="Require this many FUNCTION markers in an original TU before scoring outliers.",
    ),
) -> None:
    """Flag large address outliers in proved original TUs, with emission notes."""

    from .. import command_support as cli
    from ..reports.placement_outliers import placement_outlier_report
    from ..source_index import warn_if_source_index_may_be_stale

    settings = cli.settings()
    warn_if_source_index_may_be_stale(settings.repo_dir, "WIZ8")
    cli.emit(
        placement_outlier_report(
            settings.repo_dir,
            min_gap=min_gap,
            min_peers=min_peers,
        )
    )
