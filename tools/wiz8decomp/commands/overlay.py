from __future__ import annotations

from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(help="Analyze and review disposable candidate overlays.", no_args_is_help=True)
debug_app = typer.Typer(
    help="Internal overlay mutation primitives.", no_args_is_help=True, hidden=True
)
app.add_typer(debug_app, name="debug")


@app.command("analyze")
def analyze_command(
    selector: Annotated[str, typer.Argument(help="Program selector.")],
    plan: Annotated[Path, typer.Argument(exists=True, dir_okay=False, readable=True)],
    resume: bool = typer.Option(False, "--resume", help="Resume the exact identified overlay."),
) -> None:
    """Analyze a strict speculative plan in a fresh identified overlay."""
    from .. import cli
    from ..ghidra.inference import analyze_overlay

    cli._run_action(
        lambda: analyze_overlay(cli._settings(), selector, str(plan), resume=resume)
    )


@app.command("inspect")
def inspect_command(
    selector: Annotated[str, typer.Argument(help="Program selector.")],
    overlay_id: Annotated[str, typer.Argument(help="Overlay ID returned by analyze.")],
    address: Annotated[str, typer.Argument(help="Address to review.")],
) -> None:
    """Inspect facts, semantic delta, dependencies and promotion candidates."""
    from .. import cli
    from ..ghidra.overlay import inspect_overlay

    cli._run_action(lambda: inspect_overlay(cli._settings(), selector, overlay_id, address))


@app.command("discard")
def discard_command(
    selector: Annotated[str, typer.Argument(help="Program selector.")],
    overlay_id: Annotated[str, typer.Argument(help="Overlay ID returned by analyze.")],
) -> None:
    """Discard one disposable overlay."""
    from .. import cli
    from ..ghidra.overlay import discard_overlay

    cli._run_action(lambda: discard_overlay(cli._settings(), selector, overlay_id))


@debug_app.command("create")
def debug_create(selector: str, hypothesis: str) -> None:
    from .. import cli
    from ..ghidra.overlay import create_overlay

    cli._run_action(lambda: create_overlay(cli._settings(), selector, hypothesis))


@debug_app.command("apply-vtable")
def debug_vtable(selector: str, overlay_id: str, class_name: str) -> None:
    from .. import cli
    from ..ghidra.overlay import apply_typed_vtable

    cli._run_action(
        lambda: apply_typed_vtable(cli._settings(), selector, overlay_id, class_name)
    )


@debug_app.command("apply-reconstructed")
def debug_reconstructed(selector: str, overlay_id: str) -> None:
    from .. import cli
    from ..ghidra.reconstructed_transfer import transfer_into_overlay

    cli._run_action(lambda: transfer_into_overlay(cli._settings(), selector, overlay_id))


@debug_app.command("apply-aggregates")
def debug_aggregates(
    selector: str,
    overlay_id: str,
    aggregate: Annotated[list[str], typer.Option("--aggregate")],
) -> None:
    from .. import cli
    from ..ghidra.aggregate_overlay import apply_aggregates

    seeds = [
        {"kind": "aggregate", "name": name, "minimum_agreeing_sites": 2}
        for name in aggregate
    ]
    cli._run_action(
        lambda: apply_aggregates(
            cli._settings(), selector, overlay_id, aggregate_seeds=seeds
        )
    )


@debug_app.command("impact")
def debug_impact(selector: str, overlay_id: str, class_name: str) -> None:
    from .. import cli
    from ..ghidra.overlay import measure_impact

    cli._run_action(lambda: measure_impact(cli._settings(), selector, overlay_id, class_name))
