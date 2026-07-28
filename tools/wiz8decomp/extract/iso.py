from __future__ import annotations

from pathlib import Path

from ..subprocesses import CommandResult
from .archives import extract_with_7z


def extract_iso(source: Path, destination: Path, *, log_path: Path) -> CommandResult:
    """Extract ISO 9660 media without mounting it."""
    return extract_with_7z(source, destination, log_path=log_path)
