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

    cli.run_action(action)


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

    cli.run_action(action)


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

    cli.run_action(action)


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

    cli.run_action(action)


@app.command("status")
def status_command() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .. import command_support as cli
    from ..reports.status import status_report

    def action():
        return status_report(cli.settings())

    cli.run_action(action)


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

    def action():
        return {
            "schema": "wiz8.recovery-contexts",
            "functions": recovery_context_reports(cli.settings(), selectors, program),
        }

    cli.run_action(action)


@app.command("translation-units")
def translation_units_command() -> None:
    """Generate source ownership and bounded address-quarantine projections."""

    from .. import command_support as cli
    from ..reports.translation_units import translation_unit_report

    def action():
        return translation_unit_report(cli.settings())

    cli.run_action(action)
