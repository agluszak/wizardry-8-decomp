from __future__ import annotations

import logging
from typing import Any

from ..config import Settings
from ..paths import atomic_json
from .environment import start_pyghidra
from .project import configured_modules
from .query_daemon import stop_daemon

LOG = logging.getLogger(__name__)
HASH_OPTION = "WIZ8_IMPORTED_SHA256"
PATH_OPTION = "WIZ8_SOURCE_RELATIVE_PATH"


def _existing_hash(settings: Settings, name: str) -> str | None:
    import pyghidra

    try:
        project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    except FileNotFoundError:
        return None
    try:
        domain_file = project.getProjectData().getFile("/" + name)
        if domain_file is None:
            return None
        with pyghidra.program_context(project, "/" + name) as program:
            return program.getOptions("Program Information").getString(HASH_OPTION, None)
    finally:
        project.close()


def _delete_existing_program(settings: Settings, name: str, expected_hash: str) -> bool:
    """Delete one exact hash-validated domain file before a deterministic rebuild."""

    import pyghidra

    try:
        project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    except FileNotFoundError:
        return False
    try:
        domain_file = project.getProjectData().getFile("/" + name)
        if domain_file is None:
            return False
        with pyghidra.program_context(project, "/" + name) as program:
            actual_hash = program.getOptions("Program Information").getString(HASH_OPTION, None)
        if actual_hash != expected_hash:
            raise RuntimeError(
                f"refusing to delete {name}: project hash {actual_hash} != "
                f"configured input hash {expected_hash}"
            )
        # GhidraFile.delete() is a Java void method. Confirm the postcondition
        # instead of interpreting JPype's None return as failure.
        domain_file.delete()
        if project.getProjectData().getFile("/" + name) is not None:
            raise RuntimeError(f"Ghidra did not delete program {name}")
        return True
    finally:
        project.close()


def import_programs(
    settings: Settings,
    *,
    all_modules: bool = False,
    variant: str | None = None,
    requested_program: str | None = None,
    replace_existing: bool = False,
) -> dict[str, Any]:
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    modules = configured_modules(
        settings, all_modules=all_modules, variant=variant, requested_program=requested_program
    )
    if not modules:
        raise RuntimeError("no modules selected for Ghidra import")
    settings.project_dir.mkdir(parents=True, exist_ok=True)
    records = []
    for module in modules:
        name = module["program_name"]
        replaced = False
        if replace_existing:
            replaced = _delete_existing_program(settings, name, module["sha256"])
        existing = _existing_hash(settings, name)
        if existing:
            if existing != module["sha256"]:
                raise RuntimeError(
                    f"refusing to replace {name}: project hash {existing} != input hash {module['sha256']}"
                )
            records.append({"program": name, "sha256": existing, "status": "already-imported"})
            continue
        binary = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not binary.is_file():
            raise RuntimeError(f"materialized module is missing: {binary}")
        LOG.info("importing and analyzing %s", name)
        import pyghidra

        with pyghidra.open_program(
            binary,
            project_location=settings.project_dir,
            project_name=settings.project_name,
            analyze=True,
            language="x86:LE:32:default",
            compiler="windows",
            program_name=name,
            nested_project_location=False,
        ) as flat_api:
            program = flat_api.getCurrentProgram()
            transaction = program.startTransaction("record reproducible import identity")
            try:
                options = program.getOptions("Program Information")
                options.setString(HASH_OPTION, module["sha256"])
                options.setString(PATH_OPTION, f"{module['variant']}/{module['relative_path']}")
            finally:
                program.endTransaction(transaction, True)
            record = {
                "program": name,
                "sha256": module["sha256"],
                "status": "imported",
                "language": str(program.getLanguageID()),
                "compiler_spec": str(program.getCompilerSpec().getCompilerSpecID()),
                "function_count": program.getFunctionManager().getFunctionCount(),
                "memory_block_count": len(program.getMemory().getBlocks()),
                "replaced": replaced,
            }
            records.append(record)
    result = {"schema": "wiz8.ghidra-import", "programs": records}
    atomic_json(settings.build_dir / "manifests" / "ghidra-import.json", result)
    return result
