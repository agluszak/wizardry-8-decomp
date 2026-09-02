"""Shared command adapters kept independent of CLI composition."""

from __future__ import annotations

import json
import logging
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import typer
import yaml
from rich.console import Console

from .config import load_settings

console = Console()
logger = logging.getLogger(__name__)
_JSON_OUTPUT = False


@dataclass(frozen=True)
class HumanResult:
    """Concise terminal text paired with the complete machine-readable result."""

    text: str
    data: Any


def human(text: str, data: Any) -> HumanResult:
    return HumanResult(text=text.rstrip(), data=data)


def summary(data: Any, *, label: str | None = None) -> HumanResult:
    """Render a deliberately small public summary while retaining complete JSON."""

    if not isinstance(data, dict):
        count = len(data) if isinstance(data, list) else None
        text = label or (f"{count} item(s)" if count is not None else str(data))
        return human(text, data)
    heading = label or str(data.get("schema") or data.get("status") or "complete")
    details = []
    if "ok" in data:
        details.append("ok" if data["ok"] else "failed")
    for key in ("status", "program", "target", "failure_count", "count"):
        if key in data and data[key] not in (None, "", heading):
            details.append(f"{key.replace('_', ' ')}: {data[key]}")
    if not details:
        details.extend(
            f"{key.replace('_', ' ')}: {value}"
            for key, value in data.items()
            if isinstance(value, (bool, int, float))
        )
    return human(" — ".join([heading, *details]), data)


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
    if isinstance(value, HumanResult):
        if _JSON_OUTPUT or force_json:
            sys.stdout.write(
                json.dumps(value.data, indent=2, sort_keys=False, ensure_ascii=False) + "\n"
            )
        else:
            console.print(value.text, highlight=False, markup=False)
        return
    if _JSON_OUTPUT or force_json:
        sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")
    elif isinstance(value, (dict, list)):
        raise TypeError("public commands must wrap structured results with human()")
    else:
        console.print(value)


def run_action(action: Any, *, force_json: bool = False) -> None:
    try:
        value = action()
        if value is not None:
            if isinstance(value, (dict, list)) and not (_JSON_OUTPUT or force_json):
                value = summary(value)
            emit(value, force_json=force_json)
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
