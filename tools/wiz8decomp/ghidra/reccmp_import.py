"""Project already-built PDB/reccmp metadata into a selected Ghidra project."""

from __future__ import annotations

from dataclasses import asdict
from typing import Any

from ..config import Settings
from ..source_model import target_for_program
from ..subprocesses import run
from .workspace import resolve_seed_program


def import_reccmp_source(settings: Settings, selector: str = "wiz8") -> dict[str, Any]:
    """Let reccmp import existing build metadata; callers own the build step."""

    program_name = resolve_seed_program(settings, selector)
    target = target_for_program(program_name)
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
    return {
        "program": program_name,
        "importer": "reccmp",
        "command": asdict(result),
    }
