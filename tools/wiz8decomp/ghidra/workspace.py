from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from ..config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
)
from ..paths import sha256_file
from .environment import start_pyghidra
from .import_programs import HASH_OPTION

SEED_SCHEMA = "wiz8.ghidra-seeds"


def seed_manifest_path(settings: Settings) -> Path:
    return settings.repo_dir / "vendor" / "ghidra" / "exports" / "manifest.json"


def seed_records(settings: Settings) -> list[dict[str, Any]]:
    path = seed_manifest_path(settings)
    if not path.is_file():
        raise RuntimeError(f"Ghidra seed manifest is missing: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SEED_SCHEMA:
        raise RuntimeError(f"unsupported Ghidra seed manifest schema: {manifest.get('schema')!r}")
    records = manifest.get("seeds")
    if not isinstance(records, list) or not records:
        raise RuntimeError(f"Ghidra seed manifest contains no programs: {path}")
    return records


def resolve_seed_program(settings: Settings, selector: str | None = None) -> str:
    records = seed_records(settings)
    names = [str(record["program"]) for record in records]
    if selector is None or selector.casefold() == "wiz8":
        canonical = [name for name in names if "--gog-base--wiz8--" in name]
        if len(canonical) == 1:
            return canonical[0]
    if selector is not None:
        matches = [name for name in names if name == selector or name.startswith(selector)]
        if len(matches) == 1:
            return matches[0]
    raise ValueError(
        f"Ghidra selector {selector!r} did not identify one tracked seed: {', '.join(names)}"
    )


def seed_record(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    program_name = resolve_seed_program(settings, selector)
    record = next(
        record for record in seed_records(settings) if record.get("program") == program_name
    )
    expected_runtime = (
        REQUIRED_GHIDRA_VERSION,
        REQUIRED_GHIDRA_RELEASE,
        REQUIRED_PYGHIDRA_VERSION,
    )
    actual_runtime = (
        record.get("ghidra_version"),
        record.get("ghidra_release"),
        record.get("pyghidra_version"),
    )
    if actual_runtime != expected_runtime:
        raise RuntimeError(
            "GZF seed runtime differs from the pinned runtime: "
            f"seed={actual_runtime}, required={expected_runtime}"
        )
    archive = settings.repo_dir / str(record["path"])
    if not archive.is_file():
        raise RuntimeError(f"validated GZF seed is missing: {archive}")
    actual_hash = sha256_file(archive)
    if actual_hash != record.get("sha256"):
        raise RuntimeError(
            f"GZF seed hash mismatch for {archive}: {actual_hash} != {record.get('sha256')}"
        )
    return {**record, "archive": archive}


def _program_hash(project: Any, program_name: str) -> str | None:
    import pyghidra

    domain_file = project.getProjectData().getFile("/" + program_name)
    if domain_file is None:
        return None
    with pyghidra.program_context(project, "/" + program_name) as program:
        return program.getOptions("Program Information").getString(HASH_OPTION, None)


def restore_seed(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    """Restore the reviewed checkpoint once into the canonical local project.

    The project is operational analysis state. Evidence edits do not clone,
    replay, invalidate, or replace it.
    """

    seed = seed_record(settings, selector)
    program_name = str(seed["program"])
    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    settings.project_dir.mkdir(parents=True, exist_ok=True)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=True)
    try:
        existing_hash = _program_hash(project, program_name)
        if existing_hash is not None:
            if existing_hash != seed["binary_sha256"]:
                raise RuntimeError(
                    f"existing {program_name} hash {existing_hash} differs from the seed's "
                    f"{seed['binary_sha256']}; use a different WIZ8_WORK_DIR"
                )
            status = "already-restored"
        else:
            project.getProjectData().getRootFolder().createFile(
                program_name, File(str(seed["archive"])), TaskMonitor.DUMMY
            )
            restored_hash = _program_hash(project, program_name)
            if restored_hash != seed["binary_sha256"]:
                raise RuntimeError(f"restored program hash metadata mismatch for {program_name}")
            status = "restored"
    finally:
        project.close()
    return {
        "schema": "wiz8.ghidra-workspace",
        "program": program_name,
        "status": status,
        "project_dir": str(settings.project_dir),
        "seed": str(seed["archive"]),
        "binary_sha256": seed["binary_sha256"],
    }


def ensure_seed(settings: Settings, selector: str | None = None) -> str:
    result = restore_seed(settings, selector)
    return str(result["program"])
