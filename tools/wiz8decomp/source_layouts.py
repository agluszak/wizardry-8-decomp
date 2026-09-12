"""Run a focused on-demand source-layout audit against live Ghidra types.

The audit reports current PDB-to-Ghidra disagreements. It does not keep a
committed failure baseline: source and Ghidra evolve together, and a ratchet
of tolerated mismatches becomes a parallel inventory of the source model.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from .paths import atomic_json


def verify_source_layouts(settings: Any, pdb: Path | None = None) -> dict[str, Any]:
    """Run the audit in a cached derived project without touching reviewed state."""

    from .ghidra.env import open_project
    from .ghidra.layout_audit import audit_source_layouts
    from .ghidra.reccmp_import import import_reccmp_source
    from .ghidra.recovery import _program_name
    from .ghidra.workspace import restore_seed, seed_record
    from .paths import sha256_file

    path = pdb or (settings.repo_dir / "build/decomp/Wiz8.pdb")
    if not path.is_file():
        raise ValueError(f"compiled VC6 PDB does not exist: {path}; build WIZ8 first")
    source_index = settings.build_dir / "source-index.json"
    if not source_index.is_file():
        raise ValueError(
            f"source index does not exist: {source_index}; "
            "run `uv run wiz8 analyze source-index` first"
        )
    seed = seed_record(settings, "wiz8")
    digest = hashlib.sha256()
    audit_script = settings.repo_dir / "tools/wiz8decomp/ghidra/layout_audit.py"
    for value in (
        sha256_file(path),
        sha256_file(source_index),
        sha256_file(audit_script),
        str(seed["sha256"]),
    ):
        digest.update(value.encode("ascii"))
        digest.update(b"\0")
    cache = settings.build_dir / "ghidra-verify" / digest.hexdigest()[:20]
    cached_report = cache / "source-layouts.json"
    if cached_report.is_file():
        report = json.loads(cached_report.read_text(encoding="utf-8"))
        destination = settings.build_dir / "reports/source-layouts/report.json"
        atomic_json(destination, report)
        report["report"] = str(destination)
        return report

    derived = settings.model_copy(update={"ghidra_project_dir_override": cache / "project"})
    derived.project_dir.mkdir(parents=True, exist_ok=True)
    with open_project(derived, create=True) as project:
        restore_seed(derived, project, "wiz8")
    import_reccmp_source(derived, "wiz8")
    program_name = _program_name(derived, "wiz8")
    with open_project(derived) as project:
        import pyghidra

        with pyghidra.program_context(project, "/" + program_name) as program:
            report = audit_source_layouts(
                program, json.loads(source_index.read_text(encoding="utf-8"))
            )
    report["pdb"] = str(path)
    atomic_json(cached_report, report)
    destination = settings.build_dir / "reports/source-layouts/report.json"
    atomic_json(destination, report)
    report["report"] = str(destination)
    return report


def _stable_value(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, (dict, list)):
        return json.dumps(value, sort_keys=True, separators=(",", ":"))
    return str(value)


def normalize_layout_failure(failure: dict[str, Any]) -> dict[str, str]:
    """Project every audit failure onto a stable semantic comparison key."""

    expected = failure.get("expected", failure.get("expected_pointer_depth"))
    actual = failure.get("actual", failure.get("actual_types"))
    return {
        "kind": str(failure["kind"]),
        "class": str(failure.get("class") or ""),
        "field": str(failure.get("field") or ""),
        "expected": _stable_value(expected),
        "actual": _stable_value(actual),
    }


def layout_failure_key(failure: dict[str, Any]) -> tuple[str, str, str, str, str]:
    normalized = normalize_layout_failure(failure)
    return (
        normalized["kind"],
        normalized["class"],
        normalized["field"],
        normalized["expected"],
        normalized["actual"],
    )
