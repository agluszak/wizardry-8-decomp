"""Validate the configured local toolchain and repository environment."""

from __future__ import annotations

import importlib.metadata
import os
import tempfile
from typing import Any

from .config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
    ghidra_version,
)
from .repository import validate_repository_hygiene
from .subprocesses import tool_version


def validate_environment(settings: Settings) -> dict[str, Any]:
    """Return the complete machine-dependent doctor report or fail loudly."""

    checks: list[dict[str, Any]] = []
    version, release = ghidra_version(settings.ghidra_install_dir)
    checks.append(
        {
            "name": "ghidra",
            "ok": version == REQUIRED_GHIDRA_VERSION and release == REQUIRED_GHIDRA_RELEASE,
            "expected": f"{REQUIRED_GHIDRA_VERSION} {REQUIRED_GHIDRA_RELEASE}",
            "actual": f"{version} {release}",
            "path": str(settings.ghidra_install_dir),
        }
    )
    try:
        pyghidra_version = importlib.metadata.version("pyghidra")
    except importlib.metadata.PackageNotFoundError:
        pyghidra_version = None
    checks.append(
        {
            "name": "pyghidra",
            "ok": pyghidra_version == REQUIRED_PYGHIDRA_VERSION,
            "expected": REQUIRED_PYGHIDRA_VERSION,
            "actual": pyghidra_version,
        }
    )
    checks.append(
        {
            "name": "input-directory",
            "ok": settings.input_dir.is_dir() and os.access(settings.input_dir, os.R_OK),
            "path": str(settings.input_dir),
        }
    )
    settings.work_dir.mkdir(parents=True, exist_ok=True)
    try:
        with tempfile.NamedTemporaryFile(prefix="wiz8-doctor-", dir=settings.work_dir):
            work_writable = True
    except OSError:
        work_writable = False
    checks.append({"name": "work-directory", "ok": work_writable, "path": str(settings.work_dir)})
    required = {"7z": ["--help"], "innoextract": ["--version"], "cabextract": ["--version"]}
    optional = {"unshield": ["-V"], "wine": ["--version"], "git-lfs": ["version"]}
    for name, args in required.items():
        info = tool_version(name, args)
        checks.append({"name": name, "ok": bool(info["executable"]), **info, "required": True})
    for name, args in optional.items():
        info = tool_version(name, args)
        checks.append({"name": name, "ok": True, **info, "required": False})
    hygiene = validate_repository_hygiene(settings.repo_dir)
    checks.append({"name": "repository-hygiene", **hygiene})
    failed = [item["name"] for item in checks if not item["ok"]]
    if failed:
        raise RuntimeError("doctor failed: " + ", ".join(failed))
    return {"ok": True, "checks": checks, "failures": []}
