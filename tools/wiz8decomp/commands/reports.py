from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


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
            "policy": "Retail call sites and bodies govern fidelity; folding does not imply a no-op.",
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
    from ..merge_preservation import merge_preservation_report, parse_allowed

    report = merge_preservation_report(
        cli.settings().repo_dir, base, head, parse_allowed(allow or [])
    )
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
    """Flag large address outliers in proved original TUs, with fold/emission notes."""

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
