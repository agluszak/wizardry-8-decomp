from __future__ import annotations

from typing import Annotated

import typer

app = typer.Typer(help="Generate reports from collected evidence.", no_args_is_help=True)


@app.command("class")
def class_command(name: Annotated[str, typer.Argument(help="Reviewed Ghidra class name")]) -> None:
    """Report class fields, vtables, and binary references from live Ghidra."""
    from .. import command_support as cli
    from ..ghidra.audits import class_facts, class_fields
    from ..paths import atomic_json

    def action() -> dict[str, object]:
        settings = cli.settings()
        result = {
            **class_facts(settings, {name}),
            "schema": "wiz8.class-report",
            "classes": class_fields(settings, {name}),
        }
        output = settings.build_dir / "reports" / "classes" / f"{name.replace('::', '_')}.json"
        atomic_json(output, result)
        return {**result, "outputs": [str(output.relative_to(settings.repo_dir))]}

    cli.run_action(action)


@app.command("data")
def data_command(address: Annotated[str, typer.Argument(help="Data address")]) -> None:
    """Report one typed datum and its live Ghidra references."""
    from .. import command_support as cli
    from ..ghidra.audits import data_facts
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
    address: Annotated[str, typer.Argument(help="Address inside the function to inspect")],
    program: str = typer.Option("wiz8", "--program"),
    deep: bool = typer.Option(False, "--deep", help="Include listing, P-code and rooted flow."),
    root: str = typer.Option("this", "--root", help="Deep-analysis parameter root."),
    discover: bool = typer.Option(
        False,
        "--discover",
        help="Create a missing function transactionally for this report, then roll it back.",
    ),
) -> None:
    """Join Ghidra and every relevant evidence channel for one function."""
    from .. import command_support as cli
    from ..reports.recovery_context import recovery_context_report

    cli.run_action(
        lambda: recovery_context_report(
            cli.settings(), address, program, deep=deep, root=root, discover=discover
        )
    )


@app.command("translation-units")
def translation_units_command() -> None:
    """Generate source ownership and bounded address-quarantine projections."""

    from .. import command_support as cli
    from ..reports.translation_units import translation_unit_report

    cli.run_action(lambda: translation_unit_report(cli.settings()))
