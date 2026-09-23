from __future__ import annotations

import shutil
from pathlib import Path

from ..subprocesses import CommandResult, run


def extract_with_7z(source: Path, destination: Path, *, log_path: Path) -> CommandResult:
    destination.mkdir(parents=True, exist_ok=False)
    return run(
        ["7z", "x", "-y", f"-o{destination}", source], cwd=destination.parent, log_path=log_path
    )


def extract_inno(
    source: Path,
    destination: Path,
    *,
    log_path: Path,
    includes: tuple[str, ...] = (),
) -> CommandResult:
    if not shutil.which("innoextract"):
        raise RuntimeError("innoextract is required for the detected Inno Setup installer")
    destination.mkdir(parents=True, exist_ok=False)
    command: list[str | Path] = ["innoextract", "--output-dir", destination]
    for include in includes:
        command.extend(("--include", include))
    command.append(source)
    return run(command, cwd=destination.parent, log_path=log_path)
