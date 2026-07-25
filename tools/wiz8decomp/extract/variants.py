from __future__ import annotations

import json
import shutil
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

from ..config import Settings
from ..inputs.manifest import InputRecord
from ..inputs.scan import load_manifest
from ..manifest_models import (
    VariantProvenance,
    VariantProvenanceManifest,
    load_generated_document,
    variant_provenance_path,
    write_generated_document,
)
from ..paths import atomic_json, ensure_safe_generated_target, tree_hash, tree_manifest
from ..subprocesses import CommandResult
from .archives import extract_inno, extract_with_7z
from .installers import extract_installshield
from .iso import extract_iso

EXTRACTED_NAMES = {
    "gog-media": "gog-base",
    "patch-1261": "patch-1261",
    "patch-128": "patch-128",
    "demo": "demo",
    "official-2001-12-23-patch": "official-2001-12-23-patch",
    "compatibility-fix": "compatibility-fix",
}


def _record_for_role(settings: Settings, role: str) -> tuple[Path, InputRecord]:
    manifest = load_manifest(settings)
    matches = [item for item in manifest.files if item.configured_role == role]
    if len(matches) != 1:
        raise RuntimeError(f"expected exactly one {role} input, found {len(matches)}")
    return settings.input_dir / matches[0].relative_path, matches[0]


def _existing_matches(destination: Path, input_hash: str) -> bool:
    marker = destination / ".wiz8-extraction.json"
    if not marker.is_file():
        return False
    try:
        return json.loads(marker.read_text(encoding="utf-8"))["input_sha256"] == input_hash
    except (KeyError, ValueError):
        return False


def _provenance(
    settings: Settings,
    role: str,
    source: Path,
    record: InputRecord,
    destination: Path,
    commands: list[CommandResult],
    *,
    wine_used: bool = False,
) -> dict[str, Any]:
    files = tree_manifest(destination)
    value = {
        "schema": "wiz8.extraction-provenance",
        "role": role,
        "input_relative_path": record.relative_path,
        "input_sha256": record.sha256,
        "input_size": record.size,
        "tool": record.extraction_tool,
        "tool_version": record.extraction_tool_version,
        "commands": [
            {
                "argv": command.argv,
                "executable": command.executable,
                "exit_status": command.exit_status,
                "stdout": command.stdout,
                "stderr": command.stderr,
            }
            for command in commands
        ],
        "wine_used": wine_used,
        "created_at_utc_non_authoritative": datetime.now(UTC).isoformat(),
        "output_tree_sha256": tree_hash(files),
        "files": files,
    }
    atomic_json(settings.build_dir / "manifests" / "extractions" / f"{role}.json", value)
    atomic_json(destination / ".wiz8-extraction.json", {"input_sha256": record.sha256, "output_tree_sha256": value["output_tree_sha256"]})
    return value


def extract_role(settings: Settings, role: str) -> dict[str, Any]:
    source, record = _record_for_role(settings, role)
    destination = settings.work_dir / "extracted" / EXTRACTED_NAMES[role]
    ensure_safe_generated_target(destination, settings.work_dir)
    if destination.exists():
        if _existing_matches(destination, record.sha256):
            path = settings.build_dir / "manifests" / "extractions" / f"{role}.json"
            if path.is_file():
                existing = json.loads(path.read_text(encoding="utf-8"))
                existing["tool"] = record.extraction_tool
                existing["tool_version"] = record.extraction_tool_version
                atomic_json(path, existing)
                return existing
            return {"role": role, "status": "already-extracted"}
        raise RuntimeError(f"refusing to replace existing extraction with different or missing provenance: {destination}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    log_dir = settings.build_dir / "logs" / "extract" / role
    log_dir.mkdir(parents=True, exist_ok=True)
    commands: list[CommandResult]
    if role == "gog-media" and record.installer_technology == "Inno Setup":
        commands = [extract_inno(source, destination, log_path=log_dir / "innoextract.json")]
    elif role == "demo":
        destination.mkdir(parents=True, exist_ok=False)
        commands = extract_installshield(source, destination, log_dir=log_dir)
    elif record.detected_type == "iso-image":
        commands = [extract_iso(source, destination, log_path=log_dir / "7z-iso.json")]
    else:
        commands = [extract_with_7z(source, destination, log_path=log_dir / "7z.json")]
    return _provenance(settings, role, source, record, destination, commands)


def extract_all(settings: Settings) -> list[dict[str, Any]]:
    return [extract_role(settings, role) for role in EXTRACTED_NAMES]


def _copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(source, destination, copy_function=shutil.copy2)


def _game_root(extracted: Path, *, demo: bool = False) -> Path:
    if demo:
        installed = extracted / "installed"
        if installed.is_dir():
            exe_parents = sorted({path.parent for path in installed.rglob("*.exe")}, key=lambda p: len(p.parts))
            return exe_parents[0] if exe_parents else installed
    exe_parents = sorted({path.parent for path in extracted.rglob("Wiz8.exe")}, key=lambda p: (len(p.parts), p.as_posix().casefold()))
    if exe_parents:
        return exe_parents[0]
    app = extracted / "app"
    if app.is_dir():
        return app
    return exe_parents[0] if exe_parents else extracted


def _overlay_root(extracted: Path) -> Path:
    children = [path for path in extracted.iterdir() if path.name != ".wiz8-extraction.json"]
    directories = [path for path in children if path.is_dir()]
    files = [path for path in children if path.is_file()]
    return directories[0] if len(directories) == 1 and not files else extracted


def _apply_overlay(source: Path, destination: Path) -> list[str]:
    changed: list[str] = []
    windows_paths = {
        path.relative_to(destination).as_posix().casefold(): path.relative_to(destination)
        for path in destination.rglob("*")
    }
    for path in sorted(source.rglob("*"), key=lambda p: p.relative_to(source).as_posix().casefold()):
        if not path.is_file() or path.name == ".wiz8-extraction.json":
            continue
        relative = path.relative_to(source)
        target_relative = windows_paths.get(relative.as_posix().casefold(), relative)
        target = destination / target_relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        windows_paths[relative.as_posix().casefold()] = target_relative
        changed.append(target_relative.as_posix())
    return changed


def materialize_variants(settings: Settings) -> dict[str, Any]:
    extracted = settings.work_dir / "extracted"
    variants = settings.work_dir / "variants"
    variants.mkdir(parents=True, exist_ok=True)
    specs = {
        "gog-base": ("gog-base", None),
        "retail-2001-12-23": ("gog-base", "official-2001-12-23-patch"),
        "gog-1261": ("gog-base", "patch-1261"),
        "gog-128": ("gog-base", "patch-128"),
        "demo": ("demo", None),
    }
    records: list[VariantProvenance] = []
    for variant, (base_name, patch_name) in specs.items():
        destination = variants / variant
        marker = destination / ".wiz8-variant.json"
        if destination.exists():
            if marker.is_file():
                records.append(load_generated_document(marker, VariantProvenance))
                continue
            raise RuntimeError(f"refusing to replace unmarked variant tree: {destination}")
        base = _game_root(extracted / base_name, demo=variant == "demo")
        if not base.is_dir():
            raise RuntimeError(f"base extraction is missing for {variant}: {base}")
        _copy_tree(base, destination)
        overlay_files: list[str] = []
        if variant == "demo":
            shared = extracted / "demo" / "installed" / "Shared_Files"
            if shared.is_dir():
                overlay_files.extend(_apply_overlay(shared, destination))
        if patch_name:
            overlay_files.extend(_apply_overlay(_overlay_root(extracted / patch_name), destination))
        files = tree_manifest(destination)
        record = VariantProvenance(
            variant=variant,
            base_extraction=base_name,
            patch_extraction=patch_name,
            patch_chain=[patch_name] if patch_name else [],
            overlay_files=overlay_files,
            files=files,
            output_tree_sha256=tree_hash(files),
            created_at_utc_non_authoritative=datetime.now(UTC).isoformat(),
        )
        write_generated_document(marker, record)
        write_generated_document(
            settings.build_dir / "manifests" / "variants" / f"{variant}.json", record
        )
        records.append(record)
    output = VariantProvenanceManifest(variants=records)
    write_generated_document(variant_provenance_path(settings), output)
    return output.model_dump(mode="json", by_alias=True)


def variant_diff(settings: Settings) -> dict[str, Any]:
    manifests: dict[str, dict[str, dict[str, Any]]] = {}
    for path in sorted((settings.work_dir / "variants").glob("*/.wiz8-variant.json")):
        document = load_generated_document(path, VariantProvenance)
        data = document.model_dump(mode="json", by_alias=True)
        manifests[document.variant] = {
            entry["path"].casefold(): entry for entry in data["files"]
        }
    base = manifests.get("gog-base", {})
    comparisons: dict[str, Any] = {}
    for variant, files in sorted(manifests.items()):
        if variant == "gog-base":
            continue
        comparisons[variant] = {
            "added": sorted(entry["path"] for key, entry in files.items() if key not in base),
            "removed": sorted(entry["path"] for key, entry in base.items() if key not in files),
            "changed": sorted(files[key]["path"] for key in files.keys() & base.keys() if files[key]["sha256"] != base[key]["sha256"]),
            "identical": sum(1 for key in files.keys() & base.keys() if files[key]["sha256"] == base[key]["sha256"]),
        }
    result = {"schema": "wiz8.variant-diff", "base": "gog-base", "comparisons": comparisons}
    atomic_json(settings.build_dir / "reports" / "variant-diff.json", result)
    return result
