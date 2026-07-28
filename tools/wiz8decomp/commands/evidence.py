from __future__ import annotations

import json
from pathlib import Path
from typing import Annotated

import typer

app = typer.Typer(help="Validate and update canonical evidence.", no_args_is_help=True)


def register_root(root: typer.Typer) -> None:
    root.command("resolve-evidence-conflict", hidden=True)(resolve_conflict_command)


def resolve_conflict_command(
    paths: Annotated[list[Path], typer.Argument(help="Conflicted evidence CSVs to resolve.")],
) -> None:
    from .. import cli
    from ..evidence_merge import resolve_evidence_conflict

    cli._run_action(lambda: [resolve_evidence_conflict(path) for path in paths])


@app.command("upsert")
def upsert_command(
    path: Annotated[Path, typer.Argument(help="Canonical evidence CSV to update.")],
    row_json: Annotated[
        str,
        typer.Option("--row-json", help="One complete evidence row encoded as JSON."),
    ],
) -> None:
    """Atomically insert or monotonically enrich one canonical evidence row."""
    from .. import cli
    from ..evidence.io import upsert_row

    def action() -> dict[str, object]:
        decoded = json.loads(row_json)
        if not isinstance(decoded, dict) or not all(isinstance(key, str) for key in decoded):
            raise ValueError("--row-json must encode one JSON object with string keys")
        return upsert_row(path, {key: str(value) for key, value in decoded.items()})

    cli._run_action(action)
