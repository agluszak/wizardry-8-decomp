from __future__ import annotations

import logging

import typer

from . import command_support
from .commands.core import register as register_core
from .commands.core import toolchain_app
from .commands.evidence import app as evidence_app
from .commands.evidence import register_root as register_evidence_root
from .commands.ghidra import app as ghidra_app
from .commands.inputs import app as corpus_app
from .commands.recover import app as recover_app
from .commands.reports import app as report_app
from .commands.sgp import app as sgp_app

app = typer.Typer(
    help="Wizardry 8 reproducible decompilation bootstrap CLI.",
    no_args_is_help=True,
    add_completion=False,
)
app.add_typer(corpus_app, name="corpus")
app.add_typer(ghidra_app, name="ghidra")
app.add_typer(recover_app, name="recover")
app.add_typer(report_app, name="report")
app.add_typer(toolchain_app, name="toolchain")
app.add_typer(evidence_app, name="evidence")
app.add_typer(sgp_app, name="sgp", hidden=True)
register_core(app)
register_evidence_root(app)


class CliState:
    verbose = False
    json_output = False


@app.callback()
def main(
    ctx: typer.Context,
    verbose: bool = typer.Option(False, "--verbose", "-v", help="Enable debug logging."),
    json_output: bool = typer.Option(False, "--json", help="Render command results as JSON."),
) -> None:
    state = CliState()
    state.verbose = verbose
    state.json_output = json_output
    command_support.set_json_output(json_output)
    ctx.obj = state
    logging.basicConfig(
        level=logging.DEBUG if verbose else logging.INFO,
        format="%(levelname)s %(name)s: %(message)s",
    )
