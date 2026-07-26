from __future__ import annotations

import os
from time import perf_counter
from typing import Any

from ..config import Settings
from ..paths import atomic_json, sha256_file
from .cache import SEED_SCHEMA, materialize_program, seed_manifest_path
from .environment import start_pyghidra, validate_environment
from .import_programs import HASH_OPTION
from .project import module_for_program, resolve_program_name
from .query_daemon import stop_daemon
from .validate_replay import validate_reviewed_replay


def export_project(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    """Pack the validated canonical program as the repository's reviewed GZF seed."""

    program_name = resolve_program_name(settings, selector)
    canonical_name = resolve_program_name(settings, None)
    if program_name != canonical_name:
        raise ValueError(
            "only the canonical Wiz8 executable has a reviewed replay profile and shared seed"
        )
    module = module_for_program(settings, program_name)
    stop_daemon(settings, quiet=True)
    runtime = validate_environment(settings)

    validation_started = perf_counter()
    validation = validate_reviewed_replay(settings, program_name, evidence_program="wiz8")
    validation_seconds = perf_counter() - validation_started
    if not validation["ok"]:
        raise RuntimeError(
            f"refusing to pack an invalid Ghidra view: {validation['failure_count']} differences"
        )

    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    output_dir = settings.repo_dir / "vendor" / "ghidra" / "exports"
    output_dir.mkdir(parents=True, exist_ok=True)
    output = output_dir / f"{program_name}.gzf"
    temporary = output_dir / f".{program_name}.{os.getpid()}.gzf.partial"
    if temporary.exists():
        temporary.unlink()

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    pack_started = perf_counter()
    try:
        domain_file = project.getProjectData().getFile("/" + program_name)
        if domain_file is None:
            raise RuntimeError(f"canonical Ghidra program is missing: {program_name}")
        with pyghidra.program_context(project, "/" + program_name) as program:
            actual_hash = program.getOptions("Program Information").getString(HASH_OPTION, None)
            if actual_hash != module["sha256"]:
                raise RuntimeError(
                    f"canonical program binary hash metadata mismatch for {program_name}"
                )
            program_summary = {
                "function_count": program.getFunctionManager().getFunctionCount(),
                "memory_block_count": len(program.getMemory().getBlocks()),
                "language": str(program.getLanguageID()),
                "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()),
            }
        domain_file.packFile(File(str(temporary)), TaskMonitor.DUMMY)
    finally:
        project.close()
    temporary.replace(output)
    pack_seconds = perf_counter() - pack_started

    record = {
        "program": program_name,
        "path": output.relative_to(settings.repo_dir).as_posix(),
        "sha256": sha256_file(output),
        "binary_sha256": module["sha256"],
        **runtime,
        **program_summary,
        "validation_checks": validation["checks"],
    }
    manifest = {"schema": SEED_SCHEMA, "seeds": [record]}
    atomic_json(seed_manifest_path(settings), manifest)
    report = {
        "schema": "wiz8.ghidra-seed-build",
        "seed": record,
        "validation_seconds": round(validation_seconds, 3),
        "pack_seconds": round(pack_seconds, 3),
        "total_seconds": round(validation_seconds + pack_seconds, 3),
    }
    atomic_json(
        settings.build_dir / "reports" / "ghidra-cache" / f"seed-{program_name}.json",
        report,
    )
    return report


def restore_project(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    """Compatibility alias for the agent-local validated materializer."""

    _, report = materialize_program(settings, selector)
    return report
