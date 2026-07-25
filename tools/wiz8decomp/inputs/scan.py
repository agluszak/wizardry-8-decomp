from __future__ import annotations

import json
from pathlib import Path
from typing import Literal

import yaml
from pydantic import BaseModel, ConfigDict, Field

from ..config import Settings
from ..paths import atomic_json, sha256_file
from .file_types import detect, extension_mismatch
from .manifest import Evidence, InputManifest, InputRecord


class LocalInputs(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)
    schema_id: Literal["wiz8.local-inputs"] = Field(alias="schema")
    inputs: dict[str, str]


def load_local_inputs(settings: Settings) -> dict[str, str]:
    path = settings.repo_dir / "config" / "local-inputs.yml"
    if not path.is_file():
        raise ValueError(
            "missing config/local-inputs.yml; copy config/local-inputs.example.yml and set paths "
            "relative to WIZ8_INPUT_DIR"
        )
    config = LocalInputs.model_validate(yaml.safe_load(path.read_text(encoding="utf-8")))
    resolved: dict[str, str] = {}
    seen: set[str] = set()
    for role, raw_path in sorted(config.inputs.items()):
        relative = Path(raw_path)
        if relative.is_absolute() or ".." in relative.parts:
            raise ValueError(f"local input path must be relative and may not escape WIZ8_INPUT_DIR: {raw_path}")
        normalized = relative.as_posix()
        if normalized.casefold() in seen:
            raise ValueError(f"the same input path is assigned more than once: {normalized}")
        seen.add(normalized.casefold())
        if not (settings.input_dir / relative).is_file():
            raise ValueError(f"configured {role} input does not exist: {normalized}")
        resolved[normalized.casefold()] = role
    return resolved


def _listing(path: Path) -> str:
    from ..subprocesses import run

    result = run(["7z", "l", "-slt", path], cwd=path.parent, check=False)
    return result.stdout + result.stderr


def scan_inputs(settings: Settings) -> InputManifest:
    if not settings.input_dir.is_dir():
        raise ValueError(f"WIZ8_INPUT_DIR is not readable: {settings.input_dir}")
    configured = load_local_inputs(settings)
    records: list[InputRecord] = []
    for path in sorted((p for p in settings.input_dir.rglob("*") if p.is_file()), key=lambda p: p.relative_to(settings.input_dir).as_posix().casefold()):
        detection = detect(path)
        relative_path = path.relative_to(settings.input_dir).as_posix()
        role = configured.get(relative_path.casefold())
        evidence = list(detection.evidence)
        if role:
            evidence.append(Evidence(kind="role-config", value=f"config/local-inputs.yml: {role}"))
        records.append(InputRecord(
            relative_path=relative_path,
            size=path.stat().st_size,
            sha256=sha256_file(path),
            detected_type=detection.detected_type,
            extension_type_mismatch=extension_mismatch(path, detection.detected_type, detection.container),
            container_technology=detection.container,
            installer_technology=detection.installer,
            configured_role=role,
            role_source="local-config" if role else "unassigned",
            extraction_tool=detection.extraction_tool,
            extraction_tool_version=detection.extraction_version,
            evidence=evidence,
        ))
    manifest = InputManifest(
        files=records,
        ambiguities=[],
    )
    atomic_json(settings.build_dir / "manifests" / "input-manifest.json", manifest.model_dump(mode="json", by_alias=True))
    return manifest


def load_manifest(settings: Settings) -> InputManifest:
    path = settings.build_dir / "manifests" / "input-manifest.json"
    if not path.is_file():
        return scan_inputs(settings)
    return InputManifest.model_validate(json.loads(path.read_text(encoding="utf-8")))
