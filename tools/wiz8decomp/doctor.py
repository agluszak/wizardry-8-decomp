"""Validate the configured local toolchain and repository environment."""

from __future__ import annotations

import importlib.metadata
import os
import tempfile
from pathlib import Path
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


def _ghidra_project_check(settings: Settings) -> dict[str, Any]:
    """Check checkout ownership and reviewed-seed provenance without opening Ghidra."""

    from .ghidra.workspace import project_seed_freshness, seed_record

    try:
        seed = seed_record(settings, "wiz8", validate_archive=False)
        archive = Path(seed["archive"])
        if not archive.is_file():
            return {
                "name": "ghidra-reviewed-seed",
                "ok": False,
                "status": "missing-seed",
                "project_dir": str(settings.project_dir),
                "program": str(seed["program"]),
                "seed": str(archive),
                "detail": "reviewed GZF listed by the seed manifest is missing",
            }
        freshness = project_seed_freshness(settings, seed)
    except (OSError, RuntimeError, ValueError) as error:
        return {
            "name": "ghidra-reviewed-seed",
            "ok": False,
            "status": "invalid",
            "project_dir": str(settings.project_dir),
            "detail": str(error),
        }
    return {
        "name": "ghidra-reviewed-seed",
        **freshness,
        "project_dir": str(settings.project_dir),
        "program": str(seed["program"]),
        "seed": str(seed["archive"]),
    }


def _ghidra_source_projection_check(settings: Settings) -> dict[str, Any]:
    """Label source-projection freshness separately from reviewed-seed origin."""

    from .ghidra.workspace import resolve_seed_program, source_projection_freshness

    try:
        program = resolve_seed_program(settings, "wiz8")
        freshness = source_projection_freshness(settings, program)
    except (OSError, RuntimeError, ValueError) as error:
        return {
            "name": "ghidra-source-projection",
            "ok": True,
            "status": "unavailable",
            "detail": str(error),
        }
    return {
        "name": "ghidra-source-projection",
        **freshness,
        "program": program,
    }


def _product_inputs_check(settings: Settings) -> dict[str, Any]:
    """Label product/library mount readiness without failing Ghidra inspection."""

    from .build import ContainerBuild, prepared_mount_ready
    from .extract.variants import EXTRACTED_NAMES, verify_extraction

    build = ContainerBuild.from_settings(settings)
    missing = [
        str(mount.host)
        for mount in build.mounts
        if mount.container not in {"/repo", "/out"} and not prepared_mount_ready(mount)
    ]
    stale_recipes: list[dict[str, Any]] = []
    extracted_root = settings.work_dir / "extracted"
    if extracted_root.is_dir():
        for role, name in EXTRACTED_NAMES.items():
            destination = extracted_root / name
            if not (destination / ".wiz8-extraction.json").is_file():
                continue
            try:
                result = verify_extraction(settings, role)
            except Exception as error:  # noqa: BLE001 - doctor remains read-only
                stale_recipes.append({"role": role, "errors": [str(error)]})
                continue
            if not result.get("ok"):
                stale_recipes.append({"role": role, "errors": result.get("errors") or []})
    if missing:
        status = "missing"
        detail = "run `uv run wiz8 prepare`"
    elif stale_recipes:
        status = "stale-recipe"
        named = ", ".join(row["role"] for row in stale_recipes)
        detail = f"extraction recipe identity differs for {named}; run `uv run wiz8 extract`"
    else:
        status = "ready"
        detail = None
    return {
        "name": "product-inputs",
        "ok": True,
        "status": status,
        "detail": detail,
        "missing": missing,
        "stale_recipes": stale_recipes,
    }


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
    checks.append(_ghidra_project_check(settings))
    checks.append(_ghidra_source_projection_check(settings))
    checks.append(_product_inputs_check(settings))
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
    failed = [item for item in checks if not item["ok"]]
    if failed:
        descriptions = []
        for item in failed:
            detail = item.get("detail")
            status = item.get("status")
            if detail:
                qualifier = f"{status}: {detail}" if status else str(detail)
                descriptions.append(f"{item['name']} ({qualifier})")
            else:
                descriptions.append(str(item["name"]))
        raise RuntimeError("doctor failed: " + "; ".join(descriptions))
    return {"ok": True, "checks": checks, "failures": []}
