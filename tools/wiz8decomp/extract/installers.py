from __future__ import annotations

import os
import shutil
from pathlib import Path

from ..subprocesses import CommandResult, run
from .archives import extract_with_7z


def extract_installshield(source: Path, destination: Path, *, log_dir: Path) -> list[CommandResult]:
    """Extract an InstallShield 5 installer without executing it when unshield is available."""
    media = destination / "installer-media"
    results = [extract_with_7z(source, media, log_path=log_dir / "demo-stage1.json")]
    candidates = sorted(media.rglob("data1.cab"), key=lambda p: p.as_posix().casefold())
    if not candidates:
        raise RuntimeError("InstallShield probe succeeded but no data1.cab was found")
    unshield = shutil.which("unshield")
    if unshield:
        installed = destination / "installed"
        installed.mkdir(parents=True, exist_ok=False)
        results.append(run([unshield, "-d", installed, "x", candidates[0]], cwd=candidates[0].parent, log_path=log_dir / "demo-unshield.json"))
        return results
    raise RuntimeError(
        "the demo uses an InstallShield CAB unsupported by 7z/cabextract; install the open-source "
        "'unshield' tool, then rerun. Wine execution was not attempted because no verified silent "
        "response file is present and interactive installation cannot be made deterministic."
    )


def isolated_wine_prefix(work_dir: Path, label: str) -> Path:
    prefix = work_dir / "wine-prefixes" / label
    prefix.mkdir(parents=True, exist_ok=True)
    return prefix


def run_under_wine(
    executable: Path,
    args: list[str],
    *,
    prefix: Path,
    cwd: Path,
    log_path: Path,
) -> CommandResult:
    if not shutil.which("wine"):
        raise RuntimeError("Wine is required for this installer, but 'wine' is not on PATH")
    environment = dict(os.environ)
    environment.update({"WINEPREFIX": str(prefix), "WINEDLLOVERRIDES": "winemenubuilder.exe=d"})
    return run(["wine", executable, *args], cwd=cwd, env=environment, log_path=log_path)

