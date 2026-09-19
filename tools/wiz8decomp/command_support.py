"""Shared command adapters kept independent of CLI composition."""

from __future__ import annotations

import json
import sys
from typing import Any

from .config import load_settings


def settings():
    resolved = load_settings()
    assert resolved is not None
    return resolved


def emit(value: Any, *, as_json: bool = False, text: str | None = None) -> None:
    """Emit the one public, agent-facing result representation."""

    if text is not None and not as_json:
        sys.stdout.write(text if text.endswith("\n") else text + "\n")
        return
    sys.stdout.write(json.dumps(value, indent=2, sort_keys=False, ensure_ascii=False) + "\n")
