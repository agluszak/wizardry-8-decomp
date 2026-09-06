"""Shared command adapters kept independent of CLI composition."""

from __future__ import annotations

import json
import logging
import sys
from pathlib import Path
from typing import Any

import typer
import yaml

from .config import load_settings

logger = logging.getLogger(__name__)


def settings():
    try:
        resolved = load_settings()
        assert resolved is not None
        return resolved
    except Exception as error:
        raise typer.BadParameter(str(error)) from error


def emit(value: Any) -> None:
    """Emit the one public, agent-facing result representation."""

    sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")


def run_action(action: Any) -> None:
    try:
        value = action()
        if value is not None:
            emit(value)
    except Exception as error:
        if logger.isEnabledFor(logging.DEBUG):
            logger.exception("command failed")
        emit(
            {
                "ok": False,
                "error": {"type": type(error).__name__, "message": str(error)},
            }
        )
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
