from __future__ import annotations

from typing import Annotated, Any

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("instructions")
def instructions_command(
    selector: Annotated[str, typer.Argument(help="Function address or reviewed Ghidra name")],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Write the selected retail instruction listing and return its path."""
    from .. import command_support as cli
    from ..ghidra.env import open_program
    from ..ghidra.query import query_many
    from ..paths import atomic_write

    def action() -> dict[str, Any]:
        import re

        settings = cli.settings()
        with open_program(settings, program) as live:
            result = query_many(live, [("listing", [selector])])[0]["result"]
        safe_name = re.sub(r"[^a-z0-9_-]+", "-", selector.casefold().removeprefix("0x"))
        artifact = settings.build_dir / "context" / f"{safe_name}.asm"
        atomic_write(artifact, str(result["listing"]).rstrip() + "\n")
        return {"selector": selector, "instructions": str(artifact.relative_to(settings.repo_dir))}

    cli.emit(action())


@app.command("flow")
def flow_command(
    selector: Annotated[str, typer.Argument(help="Function address or reviewed Ghidra name")],
    root: str = typer.Option(..., "--root", help="Parameter or receiver root to trace."),
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Answer one rooted field-flow question."""
    from .. import command_support as cli
    from ..ghidra.env import open_program
    from ..ghidra.query import query_many

    def action() -> dict[str, Any]:
        with open_program(cli.settings(), program) as live:
            return query_many(live, [("field-accesses", [selector, root])])[0]["result"]

    cli.emit(action())


@app.command("class")
def class_command(
    name: Annotated[str, typer.Argument(help="Reviewed Ghidra class name")],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Report class fields, vtables, and binary references from live Ghidra."""
    from .. import command_support as cli
    from ..ghidra.env import open_program
    from ..ghidra.query import query_many

    def action() -> Any:
        settings = cli.settings()
        with open_program(settings, program) as live:
            rows = query_many(live, [("class-facts", [name]), ("class-fields", [name])])
        result = {
            **rows[0]["result"],
            "schema": "wiz8.class-report",
            "classes": rows[1]["result"]["classes"],
        }
        return result

    cli.emit(action())


@app.command("data")
def data_command(
    address: Annotated[str, typer.Argument(help="Data address")],
    interpret: str | None = typer.Option(
        None, "--as", help="Interpret the first bytes as float, u32, i32, u16, or i16."
    ),
) -> None:
    """Report one typed datum and its live Ghidra references."""
    from .. import command_support as cli
    from ..ghidra.query import data_facts

    def action() -> Any:
        import struct

        settings = cli.settings()
        entry = int(address, 0)
        result = {"schema": "wiz8.data-report", "data": data_facts(settings, {entry})}
        facts = result["data"]
        formats = {"float": "<f", "u32": "<I", "i32": "<i", "u16": "<H", "i16": "<h"}
        if interpret is not None and interpret not in formats:
            raise ValueError("--as must be float, u32, i32, u16, or i16")
        for row in facts:
            if interpret is not None:
                raw = bytes.fromhex(row.get("hex") or "")
                size = struct.calcsize(formats[interpret])
                if len(raw) < size:
                    raise ValueError(f"{row['address']} has fewer than {size} readable bytes")
                row["interpretation"] = {
                    "type": interpret,
                    "value": struct.unpack(formats[interpret], raw[:size])[0],
                }

        return result

    cli.emit(action())


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


@app.command("context")
def context_command(
    selectors: Annotated[
        list[str], typer.Argument(help="Function addresses, ranges, or exact reviewed Ghidra names")
    ],
    program: str = typer.Option("wiz8", "--program"),
) -> None:
    """Return source and retail context for a selected function batch."""
    from .. import command_support as cli
    from ..reports.recovery_context import recovery_context_reports

    cli.emit(
        {
            "schema": "wiz8.recovery-contexts",
            "functions": recovery_context_reports(cli.settings(), selectors, program),
        }
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
