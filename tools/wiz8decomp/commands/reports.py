from __future__ import annotations

from typing import Annotated, Any

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("class")
def class_command(
    name: Annotated[str, typer.Argument(help="Reviewed Ghidra class name")],
    program: str = typer.Option("wiz8", "--program"),
    json_output: bool = typer.Option(False, "--json", help="Emit complete JSON."),
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
        lines = [name]
        for item in result["classes"]:
            lines.extend(
                f"  +0x{field['offset']:x} {field['type']} {field['field']}"
                for field in item["fields"]
            )
        for table in result["vtables"]:
            lines.append(
                f"  vtable {table['address']} {table['name']} "
                f"({len(table['references'])} references)"
            )
        return cli.human("\n".join(lines), result)

    cli.run_action(action, force_json=json_output)


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

        def human_row(row: dict[str, Any]) -> str:
            interpretation = row.get("interpretation") or {}
            value = (
                f" {interpretation['type']}={interpretation['value']}"
                if interpretation
                else ""
            )
            writes = len(row.get("write_references", []))
            mutability = f"; {writes} write reference(s)" if writes else "; no write references"
            return (
                f"{row['address']} {row.get('name') or '(unnamed)'} "
                f"{row.get('type', 'undefined')} bytes={row.get('hex', 'unavailable')}"
                f"{value}{mutability}"
            )

        return cli.human(
            "\n".join(human_row(row) for row in facts),
            result,
        )

    cli.run_action(action)


@app.command("status")
def status_command() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .. import command_support as cli
    from ..reports.status import status_report

    def action():
        result = status_report(cli.settings())
        return cli.summary(result, label="repository status")

    cli.run_action(action)


@app.command("context")
def context_command(
    selectors: Annotated[
        list[str], typer.Argument(help="Function addresses, ranges, or exact reviewed Ghidra names")
    ],
    program: str = typer.Option("wiz8", "--program"),
    listing: bool = typer.Option(
        False, "--listing", help="Include the retail instruction listing."
    ),
    no_match: bool = typer.Option(
        False,
        "--no-match",
        help="Skip comparison with the current product build when gathering evidence.",
    ),
    deep: bool = typer.Option(False, "--deep", help="Include listing, P-code and rooted flow."),
    root: str = typer.Option("this", "--root", help="Deep-analysis parameter root."),
    discover: bool = typer.Option(
        False,
        "--discover",
        help="Create a missing function transactionally for this report, then roll it back.",
    ),
    view: str = typer.Option(
        "full",
        "--view",
        help="Human view: full, summary, code, listing, or dependencies.",
    ),
    json_output: bool = typer.Option(False, "--json", help="Emit the complete context payload."),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .. import command_support as cli
    from ..reports.recovery_context import recovery_context_reports, render_context

    def action():
        if view not in {"full", "summary", "code", "listing", "dependencies"}:
            raise ValueError("--view must be full, summary, code, listing, or dependencies")
        settings = cli.settings()
        contexts = recovery_context_reports(
            settings,
            selectors,
            program,
            listing=listing,
            match=not no_match,
            deep=deep,
            root=root,
            discover=discover,
        )
        text = "\n\n".join(render_context(context, view) for context in contexts)
        data: object = contexts[0] if len(contexts) == 1 else {"contexts": contexts}
        return cli.human(text, data)

    cli.run_action(action, force_json=json_output)


@app.command("translation-units")
def translation_units_command() -> None:
    """Generate source ownership and bounded address-quarantine projections."""

    from .. import command_support as cli
    from ..reports.translation_units import translation_unit_report

    def action():
        result = translation_unit_report(cli.settings())
        return cli.summary(result, label="translation units")

    cli.run_action(action)
