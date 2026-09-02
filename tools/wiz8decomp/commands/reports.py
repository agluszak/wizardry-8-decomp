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
    from ..ghidra.query import query_many
    from ..paths import atomic_json

    def action() -> Any:
        settings = cli.settings()
        rows, _transport = query_many(
            settings, program, [("class-facts", [name]), ("class-fields", [name])]
        )
        result = {
            **rows[0]["result"],
            "schema": "wiz8.class-report",
            "classes": rows[1]["result"]["classes"],
        }
        output = settings.build_dir / "reports" / "classes" / f"{name.replace('::', '_')}.json"
        atomic_json(output, result)
        result["outputs"] = [str(output.relative_to(settings.repo_dir))]
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
        lines.append(f"artifact: {result['outputs'][0]}")
        return cli.human("\n".join(lines), result)

    cli.run_action(action, force_json=json_output)


@app.command("data")
def data_command(address: Annotated[str, typer.Argument(help="Data address")]) -> None:
    """Report one typed datum and its live Ghidra references."""
    from .. import command_support as cli
    from ..ghidra.query import data_facts
    from ..paths import atomic_json

    def action() -> dict[str, object]:
        settings = cli.settings()
        entry = int(address, 0)
        result = {"schema": "wiz8.data-report", "data": data_facts(settings, {entry})}
        output = settings.build_dir / "reports" / "data" / f"{entry:08x}.json"
        atomic_json(output, result)
        return {**result, "outputs": [str(output.relative_to(settings.repo_dir))]}

    cli.run_action(action)


@app.command("status")
def status_command() -> None:
    """Summarize canonical identities, ownership, matching, and source-unit coverage."""
    from .. import command_support as cli
    from ..reports.status import status_report

    cli.run_action(lambda: status_report(cli.settings()))


@app.command("context")
def context_command(
    selectors: Annotated[
        list[str], typer.Argument(help="Function addresses, ranges, or exact source-owned names")
    ],
    program: str = typer.Option("wiz8", "--program"),
    deep: bool = typer.Option(False, "--deep", help="Include listing, P-code and rooted flow."),
    root: str = typer.Option("this", "--root", help="Deep-analysis parameter root."),
    discover: bool = typer.Option(
        False,
        "--discover",
        help="Create a missing function transactionally for this report, then roll it back.",
    ),
    json_output: bool = typer.Option(False, "--json", help="Emit the complete context payload."),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .. import command_support as cli
    from ..reports.recovery_context import recovery_context_reports, render_context

    def action():
        settings = cli.settings()
        contexts = recovery_context_reports(
            settings, selectors, program, deep=deep, root=root, discover=discover
        )
        text = "\n\n".join(render_context(context) for context in contexts)
        data: object = contexts[0] if len(contexts) == 1 else {"contexts": contexts}
        return cli.human(text, data)

    cli.run_action(action, force_json=json_output)


@app.command("translation-units")
def translation_units_command() -> None:
    """Generate source ownership and bounded address-quarantine projections."""

    from .. import command_support as cli
    from ..reports.translation_units import translation_unit_report

    cli.run_action(lambda: translation_unit_report(cli.settings()))
