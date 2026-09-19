"""Project already-built PDB/reccmp metadata into a selected Ghidra project."""

from __future__ import annotations

from dataclasses import asdict
from typing import Any

from ..config import Settings
from ..source_index import target_for_program
from ..subprocesses import run
from .workspace import compiler_import_identity, resolve_seed_program


def import_reccmp_source(settings: Settings, selector: str = "wiz8") -> dict[str, Any]:
    """Let reccmp import existing build metadata; callers own the build step.

    The importer must open the Ghidra project exclusively. Call this only while
    the Python owner does not already have the ProgramDB open.
    """

    program_name = resolve_seed_program(settings, selector)
    target = target_for_program(settings.repo_dir, program_name)
    result = run(
        [
            "reccmp-ghidra-import",
            "--target",
            target,
            "--image",
            "original",
            "--local-project-name",
            settings.project_name,
            "--local-project-dir",
            settings.project_dir,
            "--file",
            f"/{program_name}",
        ],
        cwd=settings.repo_dir / "build/decomp",
    )
    identity = compiler_import_identity(settings, target)
    return {
        "program": program_name,
        "importer": "reccmp",
        "command": asdict(result),
        **identity,
    }
