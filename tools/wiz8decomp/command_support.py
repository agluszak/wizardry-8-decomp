"""Shared command adapters kept independent of CLI composition."""

from __future__ import annotations

import json
import logging
import sys
from pathlib import Path
from typing import Any

import typer
import yaml
from rich.console import Console
from rich.json import JSON

from .config import load_settings

console = Console()
logger = logging.getLogger(__name__)
_JSON_OUTPUT = False


def set_json_output(enabled: bool) -> None:
    global _JSON_OUTPUT
    _JSON_OUTPUT = enabled


def settings():
    try:
        resolved = load_settings()
        assert resolved is not None
        return resolved
    except Exception as error:
        raise typer.BadParameter(str(error)) from error


def emit(value: Any, *, force_json: bool = False) -> None:
    if _JSON_OUTPUT or force_json:
        sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")
    elif isinstance(value, (dict, list)):
        console.print(JSON.from_data(value))
    else:
        console.print(value)


def run_action(action: Any, *, force_json: bool = False) -> None:
    try:
        emit(action(), force_json=force_json)
    except Exception as error:
        if logger.isEnabledFor(logging.DEBUG):
            logger.exception("command failed")
        console.print(f"[red]error:[/red] {error}", highlight=False)
        raise typer.Exit(1) from error


def reccmp_original(target: str) -> Path | None:
    user_config = settings().repo_dir / "reccmp-user.yml"
    if not user_config.is_file():
        return None
    configured = yaml.safe_load(user_config.read_text(encoding="utf-8")) or {}
    path = (configured.get("targets") or {}).get(target, {}).get("path")
    if not path:
        return None
    resolved = Path(str(path).strip())
    return resolved if resolved.is_file() else None
