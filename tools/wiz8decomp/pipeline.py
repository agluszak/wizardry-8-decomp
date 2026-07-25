from __future__ import annotations

import shutil
from enum import Enum
from pathlib import Path
from typing import Any

from .config import Settings
from .extract.variants import (
    EXTRACTED_NAMES,
    VARIANT_SPECS,
    verify_extraction,
    verify_variant,
)
from .paths import ensure_safe_generated_target


class PipelineStage(str, Enum):
    extractions = "extractions"
    variants = "variants"


def _failed_check(stage: str, identifier: str, error: Exception) -> dict[str, Any]:
    return {
        "stage": stage,
        "id": identifier,
        "ok": False,
        "errors": [str(error)],
    }


def verify_pipeline(settings: Settings) -> dict[str, Any]:
    checks: list[dict[str, Any]] = []
    extraction_checks: dict[str, dict[str, Any]] = {}
    for role in EXTRACTED_NAMES:
        try:
            check = verify_extraction(settings, role)
        except Exception as error:  # noqa: BLE001 - verification must report every stage.
            check = _failed_check("extractions", role, error)
        extraction_checks[role] = check
        checks.append(check)
    for variant, spec in VARIANT_SPECS.items():
        upstream_roles = [_role for _role, name in EXTRACTED_NAMES.items() if name in spec]
        invalid_upstream = [role for role in upstream_roles if not extraction_checks[role]["ok"]]
        if invalid_upstream:
            checks.append(
                {
                    "stage": "variants",
                    "id": variant,
                    "ok": False,
                    "errors": [
                        "upstream extraction verification failed: " + ", ".join(invalid_upstream)
                    ],
                }
            )
            continue
        try:
            checks.append(verify_variant(settings, variant, spec, verify_inputs=False))
        except Exception as error:  # noqa: BLE001 - verification must report every stage.
            checks.append(_failed_check("variants", variant, error))
    return {
        "schema": "wiz8.pipeline-verification",
        "ok": all(check["ok"] for check in checks),
        "checks": checks,
    }


def _remove_work_tree(settings: Settings, path: Path, removed: list[str]) -> None:
    ensure_safe_generated_target(path, settings.work_dir)
    if path.exists():
        shutil.rmtree(path)
        removed.append(str(path))


def _remove_build_path(settings: Settings, path: Path, removed: list[str]) -> None:
    resolved = path.resolve()
    build_root = settings.build_dir.resolve()
    if resolved == build_root or build_root not in resolved.parents:
        raise ValueError(f"generated build target must be below build/: {resolved}")
    if not path.exists():
        return
    if path.is_dir():
        shutil.rmtree(path)
    else:
        path.unlink()
    removed.append(str(path))


def _clean_variants(settings: Settings, removed: list[str]) -> None:
    _remove_work_tree(settings, settings.work_dir / "variants", removed)
    for path in (
        settings.build_dir / "manifests" / "variants",
        settings.build_dir / "manifests" / "variant-provenance.json",
        settings.build_dir / "manifests" / "variant-module-inventory.json",
        settings.build_dir / "manifests" / "modules.json",
        settings.build_dir / "reports" / "variant-diff.json",
        settings.build_dir / "reports" / "module-diff.json",
        settings.build_dir / "reports" / "modules.md",
        settings.build_dir / "reports" / "compiler-evidence.json",
        settings.build_dir / "reports" / "compiler-evidence.md",
        settings.build_dir / "evidence" / "source-paths.csv",
        settings.build_dir / "evidence" / "assertions.csv",
    ):
        _remove_build_path(settings, path, removed)


def clean_pipeline(settings: Settings, stage: PipelineStage) -> dict[str, Any]:
    removed: list[str] = []
    if stage is PipelineStage.extractions:
        _clean_variants(settings, removed)
        _remove_work_tree(settings, settings.work_dir / "extracted", removed)
        _remove_build_path(settings, settings.build_dir / "manifests" / "extractions", removed)
    elif stage is PipelineStage.variants:
        _clean_variants(settings, removed)
    else:  # pragma: no cover - Enum prevents this through normal callers.
        raise ValueError(f"unsupported pipeline stage: {stage}")
    return {
        "schema": "wiz8.pipeline-clean",
        "stage": stage.value,
        "removed": removed,
    }
