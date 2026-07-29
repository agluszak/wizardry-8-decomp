from __future__ import annotations

import json
from pathlib import Path
from time import perf_counter
from typing import Any

from ..config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
)
from ..paths import atomic_json, sha256_file
from .environment import start_pyghidra
from .import_programs import HASH_OPTION
from .project import module_for_program, resolve_program_name
from .rebuild import observation_replay_actions, reviewed_replay_actions
from .validate_replay import validate_reviewed_replay

SEED_SCHEMA = "wiz8.ghidra-seeds"


def seed_manifest_path(settings: Settings) -> Path:
    return settings.repo_dir / "vendor" / "ghidra" / "exports" / "manifest.json"


def _seed_record(settings: Settings, program_name: str) -> dict[str, Any]:
    path = seed_manifest_path(settings)
    if not path.is_file():
        raise RuntimeError(f"Ghidra seed manifest is missing: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SEED_SCHEMA:
        raise RuntimeError(f"unsupported Ghidra seed manifest schema: {manifest.get('schema')!r}")
    matches = [record for record in manifest.get("seeds", []) if record.get("program") == program_name]
    if len(matches) != 1:
        raise RuntimeError(
            f"expected one validated GZF seed for {program_name}; found {len(matches)}"
        )
    record = matches[0]
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
    archive = settings.repo_dir / record["path"]
    if not archive.is_file():
        raise RuntimeError(f"validated GZF seed is missing: {archive}")
    actual_archive_hash = sha256_file(archive)
    if actual_archive_hash != record.get("sha256"):
        raise RuntimeError(
            f"GZF seed hash mismatch for {archive}: "
            f"{actual_archive_hash} != {record.get('sha256')}"
        )
    module = module_for_program(settings, program_name)
    if record.get("binary_sha256") != module["sha256"]:
        raise RuntimeError(f"GZF seed binary hash differs from configured {program_name}")
    return {**record, "archive": archive}


def _program_exists(settings: Settings, program_name: str) -> bool:
    start_pyghidra(settings)
    import pyghidra

    try:
        project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    except FileNotFoundError:
        return False
    try:
        return project.getProjectData().getFile("/" + program_name) is not None
    finally:
        project.close()


def materialize_program(
    settings: Settings,
    selector: str | None = None,
) -> tuple[Settings, dict[str, Any]]:
    """Restore the canonical seed once; never create per-agent project clones."""

    program_name = resolve_program_name(settings, selector)
    report_path = settings.build_dir / "reports" / "ghidra-project" / f"{program_name}.json"
    if _program_exists(settings, program_name):
        report = {
            "schema": "wiz8.ghidra-project",
            "program": program_name,
            "status": "existing",
            "project_dir": str(settings.project_dir),
        }
        atomic_json(report_path, report)
        return settings, report

    seed = _seed_record(settings, program_name)
    started = perf_counter()
    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    settings.project_dir.mkdir(parents=True, exist_ok=True)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=True)
    try:
        root_folder = project.getProjectData().getRootFolder()
        root_folder.createFile(program_name, File(str(seed["archive"])), TaskMonitor.DUMMY)
        with pyghidra.program_context(project, "/" + program_name) as program:
            actual_hash = program.getOptions("Program Information").getString(HASH_OPTION, None)
            if actual_hash != seed["binary_sha256"]:
                raise RuntimeError(f"restored program hash metadata mismatch for {program_name}")
    finally:
        project.close()

    phases: list[str] = []
    for name, action in reviewed_replay_actions(settings, program_name):
        action()
        phases.append(name)
    for name, action in observation_replay_actions(settings, program_name):
        action()
        phases.append(name)
    validation = validate_reviewed_replay(settings, program_name, evidence_program="wiz8")
    if not validation["ok"]:
        raise RuntimeError(
            f"Ghidra restore validation failed with {validation['failure_count']} differences"
        )
    report = {
        "schema": "wiz8.ghidra-project",
        "program": program_name,
        "status": "restored",
        "project_dir": str(settings.project_dir),
        "seed": str(seed["archive"]),
        "phases": phases,
        "validation": validation,
        "total_seconds": round(perf_counter() - started, 3),
    }
    atomic_json(report_path, report)
    return settings, report


def cache_status(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    """Compatibility status for the single canonical project."""

    program_name = resolve_program_name(settings, selector)
    return {
        "schema": "wiz8.ghidra-project-status",
        "program": program_name,
        "project_dir": str(settings.project_dir),
        "present": _program_exists(settings, program_name),
    }


def open_for_mutation(
    settings: Settings,
    selector: str | None = None,
    *,
    materialize: bool = True,
) -> Settings:
    """Open mutations against the checkout's one canonical Ghidra project."""

    if materialize:
        materialize_program(settings, selector)
    start_pyghidra(settings)
    return settings
