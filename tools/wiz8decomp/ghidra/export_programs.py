from __future__ import annotations

import json
from typing import Any

from ..config import Settings
from ..paths import atomic_json, sha256_file
from .environment import start_pyghidra
from .import_programs import HASH_OPTION
from .project import configured_modules
from .query_daemon import stop_daemon


def export_project(settings: Settings) -> dict[str, Any]:
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    output_dir = settings.repo_dir / "vendor" / "ghidra" / "exports"
    output_dir.mkdir(parents=True, exist_ok=True)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    records = []
    try:
        for module in configured_modules(settings, all_modules=True):
            name = module["program_name"]
            domain_file = project.getProjectData().getFile("/" + name)
            if domain_file is None:
                continue
            output = output_dir / f"{name}.gzf"
            domain_file.packFile(File(str(output)), TaskMonitor.DUMMY)
            records.append({"program": name, "path": output.relative_to(settings.repo_dir).as_posix(), "sha256": sha256_file(output), "binary_sha256": module["sha256"]})
    finally:
        project.close()
    result = {"schema": "wiz8.ghidra-exports", "exports": records}
    atomic_json(output_dir / "manifest.json", result)
    return result


def restore_project(settings: Settings) -> dict[str, Any]:
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.util.task import TaskMonitor
    from java.io import File

    manifest_path = settings.repo_dir / "vendor" / "ghidra" / "exports" / "manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError("no committed Ghidra export manifest exists")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    settings.project_dir.mkdir(parents=True, exist_ok=True)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=True)
    restored = []
    try:
        root = project.getProjectData().getRootFolder()
        for record in manifest["exports"]:
            archive = settings.repo_dir / record["path"]
            if sha256_file(archive) != record["sha256"]:
                raise RuntimeError(f"GZF hash mismatch: {archive}")
            existing = project.getProjectData().getFile("/" + record["program"])
            if existing is None:
                restored_file = root.createFile(record["program"], File(str(archive)), TaskMonitor.DUMMY)
                restored.append({"program": record["program"], "status": "restored", "domain_file": str(restored_file)})
            else:
                restored.append({"program": record["program"], "status": "already-present"})
            with pyghidra.program_context(project, "/" + record["program"]) as program:
                actual = program.getOptions("Program Information").getString(HASH_OPTION, None)
                if actual != record["binary_sha256"]:
                    raise RuntimeError(f"restored program binary hash metadata mismatch for {record['program']}")
                restored[-1].update({"language": str(program.getLanguageID()), "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()), "function_count": program.getFunctionManager().getFunctionCount(), "memory_block_count": len(program.getMemory().getBlocks())})
    finally:
        project.close()
    return {"schema": "wiz8.ghidra-restore", "programs": restored}
