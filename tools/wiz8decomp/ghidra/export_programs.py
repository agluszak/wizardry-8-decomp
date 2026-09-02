from __future__ import annotations

import os
from time import perf_counter
from typing import Any

from ..config import Settings
from ..paths import atomic_json, sha256_file
from .env import open_project, validate_environment
from .import_programs import HASH_OPTION
from .project import module_for_program, resolve_program_name
from .workspace import SEED_SCHEMA, resolve_seed_program, seed_manifest_path, seed_records


def export_project(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    """Pack an intentionally reviewed canonical project checkpoint."""

    program_name = (
        resolve_seed_program(settings, None)
        if selector is None
        else resolve_program_name(settings, selector)
    )
    module = module_for_program(settings, program_name)
    previous_records = seed_records(settings)
    runtime = validate_environment(settings)

    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    output_dir = settings.repo_dir / "vendor" / "ghidra" / "exports"
    output_dir.mkdir(parents=True, exist_ok=True)
    output = output_dir / f"{program_name}.gzf"
    temporary = output_dir / f".{program_name}.{os.getpid()}.gzf.partial"
    if temporary.exists():
        temporary.unlink()

    pack_started = perf_counter()
    with open_project(settings) as project:
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
    temporary.replace(output)
    pack_seconds = perf_counter() - pack_started

    record = {
        "program": program_name,
        "path": output.relative_to(settings.repo_dir).as_posix(),
        "sha256": sha256_file(output),
        "binary_sha256": module["sha256"],
        **runtime,
        **program_summary,
    }
    records = [item for item in previous_records if item.get("program") != program_name]
    records.append(record)
    records.sort(key=lambda item: str(item["program"]))
    manifest = {"schema": SEED_SCHEMA, "seeds": records}
    atomic_json(seed_manifest_path(settings), manifest)
    report = {
        "schema": "wiz8.ghidra-seed-build",
        "seed": record,
        "pack_seconds": round(pack_seconds, 3),
        "total_seconds": round(pack_seconds, 3),
    }
    atomic_json(
        settings.build_dir / "reports" / "ghidra-seed-refresh" / f"seed-{program_name}.json",
        report,
    )
    return report
