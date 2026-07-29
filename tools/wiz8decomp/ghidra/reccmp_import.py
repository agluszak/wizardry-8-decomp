"""Project rebuilt PDB/reccmp metadata into the canonical Ghidra program."""

from __future__ import annotations

from dataclasses import asdict
from typing import Any

from ..build import build_target
from ..config import Settings
from ..source_model import target_for_program
from ..subprocesses import run
from .workspace import ensure_seed


def import_reccmp_source(settings: Settings, selector: str = "wiz8") -> dict[str, Any]:
    """Build the paired image and let reccmp import source/PDB metadata."""

    program_name = ensure_seed(settings, selector)
    target = target_for_program(program_name)
    build_target(settings, target)
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
    return {"program": program_name, "importer": "reccmp", "command": asdict(result)}
