from __future__ import annotations

import shutil
from datetime import UTC, datetime
from pathlib import Path
from typing import Any

from ..config import Settings
from ..inputs.manifest import InputRecord
from ..inputs.scan import load_manifest
from ..manifest_models import (
    CommandReceipt,
    ExtractionReceipt,
    ToolIdentity,
    VariantProvenance,
    VariantProvenanceManifest,
    load_generated_document,
    variant_provenance_path,
    write_generated_document,
)
from ..paths import (
    atomic_json,
    build_directory_atomically,
    ensure_safe_generated_target,
    json_hash,
    sha256_file,
    tree_hash,
    tree_manifest,
)
from ..subprocesses import CommandResult, tool_version
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

VARIANT_SPECS: dict[str, tuple[str, str | None]] = {
    "gog-base": ("gog-base", None),
    "retail-2001-12-23": ("gog-base", "official-2001-12-23-patch"),
    "gog-1261": ("gog-base", "patch-1261"),
    "gog-128": ("gog-base", "patch-128"),
    "demo": ("demo", None),
}

_PACKAGE_ROOT = Path(__file__).parents[1]
_RECEIPT_IMPLEMENTATION_FILES = (
    _PACKAGE_ROOT / "manifest_models.py",
    _PACKAGE_ROOT / "paths.py",
)
EXTRACTION_IMPLEMENTATION_FILES = _RECEIPT_IMPLEMENTATION_FILES + (
    Path(__file__),
    Path(__file__).with_name("archives.py"),
    Path(__file__).with_name("installers.py"),
    Path(__file__).with_name("iso.py"),
)
VARIANT_IMPLEMENTATION_FILES = _RECEIPT_IMPLEMENTATION_FILES + (Path(__file__),)


def _record_for_role(settings: Settings, role: str) -> tuple[Path, InputRecord]:
    manifest = load_manifest(settings)
    matches = [item for item in manifest.files if item.configured_role == role]
    if len(matches) != 1:
        raise RuntimeError(f"expected exactly one {role} input, found {len(matches)}")
    return settings.input_dir / matches[0].relative_path, matches[0]


def _implementation_revision(paths: tuple[Path, ...]) -> str:
    return json_hash(
        [
            {"path": path.name, "sha256": sha256_file(path)}
            for path in sorted(paths, key=lambda item: item.name)
        ]
    )


def _tool_identity(name: str, args: tuple[str, ...]) -> ToolIdentity:
    return ToolIdentity.model_validate(tool_version(name, args))


def _extraction_tool_versions(role: str, record: InputRecord) -> dict[str, ToolIdentity]:
    if role == "gog-media" and record.installer_technology == "Inno Setup":
        return {"innoextract": _tool_identity("innoextract", ("--version",))}
    if role == "demo":
        return {
            "7z": _tool_identity("7z", ("--help",)),
            "unshield": _tool_identity("unshield", ("-V",)),
        }
    return {"7z": _tool_identity("7z", ("--help",))}


def _extraction_configuration_hash(role: str, record: InputRecord) -> str:
    return json_hash(
        {
            "role": role,
            "destination_name": EXTRACTED_NAMES[role],
            "input": {
                "relative_path": record.relative_path,
                "size": record.size,
                "sha256": record.sha256,
                "detected_type": record.detected_type,
                "container_technology": record.container_technology,
                "installer_technology": record.installer_technology,
            },
        }
    )


def _content_manifest(destination: Path, receipt_name: str) -> list[dict[str, Any]]:
    return [entry for entry in tree_manifest(destination) if entry["path"] != receipt_name]


def _command_receipts(commands: list[CommandResult]) -> list[CommandReceipt]:
    return [
        CommandReceipt(
            argv=command.argv,
            executable=command.executable,
            cwd=command.cwd,
            exit_status=command.exit_status,
            stdout=command.stdout,
            stderr=command.stderr,
        )
        for command in commands
    ]


def _tree_receipt_errors(
    destination: Path,
    receipt_name: str,
    recorded_files: list[Any],
    recorded_hash: str,
) -> tuple[list[str], str | None]:
    if not destination.is_dir():
        return [f"output directory is missing: {destination}"], None
    actual_files = _content_manifest(destination, receipt_name)
    actual_hash = tree_hash(actual_files)
    expected_files = [item.model_dump(mode="json") for item in recorded_files]
    errors = []
    if actual_files != expected_files:
        errors.append("per-file output manifest differs from receipt")
    if actual_hash != recorded_hash:
        errors.append(f"output tree hash differs: expected {recorded_hash}, got {actual_hash}")
    return errors, actual_hash


def _extraction_receipt_path(settings: Settings, role: str) -> Path:
    return settings.build_dir / "manifests" / "extractions" / f"{role}.json"


def _load_extraction_receipt(destination: Path) -> ExtractionReceipt:
    return load_generated_document(destination / ".wiz8-extraction.json", ExtractionReceipt)


def _provenance(
    settings: Settings,
    role: str,
    record: InputRecord,
    destination: Path,
    commands: list[CommandResult],
    *,
    wine_used: bool = False,
) -> ExtractionReceipt:
    files = _content_manifest(destination, ".wiz8-extraction.json")
    value = ExtractionReceipt(
        role=role,
        input_relative_path=record.relative_path,
        input_hashes=[record.sha256],
        input_size=record.size,
        configuration_hash=_extraction_configuration_hash(role, record),
        implementation_revision=_implementation_revision(EXTRACTION_IMPLEMENTATION_FILES),
        tool_versions=_extraction_tool_versions(role, record),
        commands=_command_receipts(commands),
        wine_used=wine_used,
        output_tree_hash=tree_hash(files),
        files=files,
        created_at_utc_non_authoritative=datetime.now(UTC).isoformat(),
    )
    write_generated_document(destination / ".wiz8-extraction.json", value)
    return value


def verify_extraction(settings: Settings, role: str) -> dict[str, Any]:
    source, record = _record_for_role(settings, role)
    destination = settings.work_dir / "extracted" / EXTRACTED_NAMES[role]
    ensure_safe_generated_target(destination, settings.work_dir)
    errors: list[str] = []
    actual_input_hash = sha256_file(source) if source.is_file() else None
    if actual_input_hash != record.sha256:
        errors.append(
            f"input hash differs from scanned manifest: expected {record.sha256}, "
            f"got {actual_input_hash or 'missing'}"
        )
    try:
        receipt = _load_extraction_receipt(destination)
    except RuntimeError as error:
        return {
            "stage": "extractions",
            "id": role,
            "ok": False,
            "errors": errors + [str(error)],
        }
    expected_tools = _extraction_tool_versions(role, record)
    expected = {
        "role": role,
        "input_relative_path": record.relative_path,
        "input_hashes": [record.sha256],
        "input_size": record.size,
        "configuration_hash": _extraction_configuration_hash(role, record),
        "implementation_revision": _implementation_revision(EXTRACTION_IMPLEMENTATION_FILES),
        "tool_versions": expected_tools,
    }
    for field, value in expected.items():
        if getattr(receipt, field) != value:
            errors.append(f"receipt {field} differs from the current recipe")
    tree_errors, actual_tree_hash = _tree_receipt_errors(
        destination,
        ".wiz8-extraction.json",
        receipt.files,
        receipt.output_tree_hash,
    )
    errors.extend(tree_errors)
    return {
        "stage": "extractions",
        "id": role,
        "ok": not errors,
        "errors": errors,
        "output_tree_hash": actual_tree_hash,
    }


def extract_role(settings: Settings, role: str) -> dict[str, Any]:
    source, record = _record_for_role(settings, role)
    destination = settings.work_dir / "extracted" / EXTRACTED_NAMES[role]
    ensure_safe_generated_target(destination, settings.work_dir)
    if destination.exists():
        verification = verify_extraction(settings, role)
        if not verification["ok"]:
            detail = "; ".join(verification["errors"])
            raise RuntimeError(
                f"existing extraction failed verification for {role}: {detail}. "
                "Run 'wiz8 corpus clean --stage extractions' to rebuild generated state."
            )
        existing = _load_extraction_receipt(destination)
        write_generated_document(_extraction_receipt_path(settings, role), existing)
        return existing.model_dump(mode="json", by_alias=True)
    if sha256_file(source) != record.sha256:
        raise RuntimeError(
            f"configured input changed since 'wiz8 corpus scan': {record.relative_path}"
        )
    destination.parent.mkdir(parents=True, exist_ok=True)
    log_dir = settings.build_dir / "logs" / "extract" / role
    log_dir.mkdir(parents=True, exist_ok=True)

    def build(candidate: Path) -> ExtractionReceipt:
        commands: list[CommandResult]
        if role == "gog-media" and record.installer_technology == "Inno Setup":
            commands = [extract_inno(source, candidate, log_path=log_dir / "innoextract.json")]
        elif role == "demo":
            commands = extract_installshield(source, candidate, log_dir=log_dir)
        elif record.detected_type == "iso-image":
            commands = [extract_iso(source, candidate, log_path=log_dir / "7z-iso.json")]
        else:
            commands = [extract_with_7z(source, candidate, log_path=log_dir / "7z.json")]
        return _provenance(settings, role, record, candidate, commands)

    receipt = build_directory_atomically(destination, settings.work_dir, build)
    write_generated_document(_extraction_receipt_path(settings, role), receipt)
    return receipt.model_dump(mode="json", by_alias=True)


def extract_all(settings: Settings) -> list[dict[str, Any]]:
    return [extract_role(settings, role) for role in EXTRACTED_NAMES]


def _copy_tree(source: Path, destination: Path) -> None:
    shutil.copytree(
        source,
        destination,
        copy_function=shutil.copy2,
        ignore=shutil.ignore_patterns(".wiz8-extraction.json"),
    )


def _game_root(extracted: Path, *, demo: bool = False) -> Path:
    if demo:
        installed = extracted / "installed"
        if installed.is_dir():
            exe_parents = sorted(
                {path.parent for path in installed.rglob("*.exe")}, key=lambda p: len(p.parts)
            )
            return exe_parents[0] if exe_parents else installed
    exe_parents = sorted(
        {path.parent for path in extracted.rglob("Wiz8.exe")},
        key=lambda p: (len(p.parts), p.as_posix().casefold()),
    )
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
    for path in sorted(
        source.rglob("*"), key=lambda p: p.relative_to(source).as_posix().casefold()
    ):
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


def _role_for_extraction_name(name: str) -> str:
    return next(
        role for role, destination_name in EXTRACTED_NAMES.items() if destination_name == name
    )


def _variant_inputs(
    settings: Settings,
    base_name: str,
    patch_name: str | None,
    *,
    verify_inputs: bool = True,
) -> list[ExtractionReceipt]:
    names = [base_name, *([patch_name] if patch_name else [])]
    receipts = []
    for name in names:
        assert name is not None
        role = _role_for_extraction_name(name)
        if verify_inputs:
            verification = verify_extraction(settings, role)
            if not verification["ok"]:
                raise RuntimeError(
                    f"cannot materialize from invalid {role} extraction: "
                    + "; ".join(verification["errors"])
                )
        receipts.append(_load_extraction_receipt(settings.work_dir / "extracted" / name))
    return receipts


def _variant_configuration_hash(
    variant: str,
    base_name: str,
    patch_name: str | None,
    inputs: list[ExtractionReceipt],
) -> str:
    return json_hash(
        {
            "variant": variant,
            "base_extraction": base_name,
            "patch_extraction": patch_name,
            "upstream_recipes": [
                {
                    "role": receipt.role,
                    "configuration_hash": receipt.configuration_hash,
                    "implementation_revision": receipt.implementation_revision,
                }
                for receipt in inputs
            ],
        }
    )


def _variant_input_hashes(inputs: list[ExtractionReceipt]) -> list[str]:
    return [
        value for receipt in inputs for value in [*receipt.input_hashes, receipt.output_tree_hash]
    ]


def _variant_tool_versions(inputs: list[ExtractionReceipt]) -> dict[str, ToolIdentity]:
    return {
        f"{receipt.role}:{name}": identity
        for receipt in inputs
        for name, identity in sorted(receipt.tool_versions.items())
    }


def _variant_receipt_path(settings: Settings, variant: str) -> Path:
    return settings.build_dir / "manifests" / "variants" / f"{variant}.json"


def verify_variant(
    settings: Settings,
    variant: str,
    spec: tuple[str, str | None] | None = None,
    *,
    verify_inputs: bool = True,
    input_receipts: list[ExtractionReceipt] | None = None,
) -> dict[str, Any]:
    base_name, patch_name = spec or VARIANT_SPECS[variant]
    destination = settings.work_dir / "variants" / variant
    errors: list[str] = []
    try:
        inputs = input_receipts or _variant_inputs(
            settings, base_name, patch_name, verify_inputs=verify_inputs
        )
    except RuntimeError as error:
        return {"stage": "variants", "id": variant, "ok": False, "errors": [str(error)]}
    try:
        receipt = load_generated_document(destination / ".wiz8-variant.json", VariantProvenance)
    except RuntimeError as error:
        return {"stage": "variants", "id": variant, "ok": False, "errors": [str(error)]}
    expected = {
        "variant": variant,
        "base_extraction": base_name,
        "patch_extraction": patch_name,
        "patch_chain": [patch_name] if patch_name else [],
        "input_hashes": _variant_input_hashes(inputs),
        "configuration_hash": _variant_configuration_hash(variant, base_name, patch_name, inputs),
        "implementation_revision": _implementation_revision(VARIANT_IMPLEMENTATION_FILES),
        "tool_versions": _variant_tool_versions(inputs),
    }
    for field, value in expected.items():
        if getattr(receipt, field) != value:
            errors.append(f"receipt {field} differs from the current recipe")
    tree_errors, actual_tree_hash = _tree_receipt_errors(
        destination,
        ".wiz8-variant.json",
        receipt.files,
        receipt.output_tree_hash,
    )
    errors.extend(tree_errors)
    return {
        "stage": "variants",
        "id": variant,
        "ok": not errors,
        "errors": errors,
        "output_tree_hash": actual_tree_hash,
    }


def materialize_variants(settings: Settings) -> dict[str, Any]:
    extracted = settings.work_dir / "extracted"
    variants = settings.work_dir / "variants"
    variants.mkdir(parents=True, exist_ok=True)
    input_receipts: dict[str, ExtractionReceipt] = {}
    for name in dict.fromkeys(
        name
        for base_name, patch_name in VARIANT_SPECS.values()
        for name in (base_name, patch_name)
        if name is not None
    ):
        role = _role_for_extraction_name(name)
        verification = verify_extraction(settings, role)
        if not verification["ok"]:
            raise RuntimeError(
                f"cannot materialize from invalid {role} extraction: "
                + "; ".join(verification["errors"])
            )
        input_receipts[name] = _load_extraction_receipt(extracted / name)
    records: list[VariantProvenance] = []
    for variant, (base_name, patch_name) in VARIANT_SPECS.items():
        inputs = [input_receipts[name] for name in (base_name, patch_name) if name is not None]
        destination = variants / variant
        marker = destination / ".wiz8-variant.json"
        if destination.exists():
            verification = verify_variant(
                settings,
                variant,
                (base_name, patch_name),
                verify_inputs=False,
                input_receipts=inputs,
            )
            if not verification["ok"]:
                raise RuntimeError(
                    f"existing variant failed verification for {variant}: "
                    + "; ".join(verification["errors"])
                    + ". Run 'wiz8 corpus clean --stage variants' to rebuild generated state."
                )
            receipt = load_generated_document(marker, VariantProvenance)
            write_generated_document(_variant_receipt_path(settings, variant), receipt)
            records.append(receipt)
            continue
        base = _game_root(extracted / base_name, demo=variant == "demo")
        if not base.is_dir():
            raise RuntimeError(f"base extraction is missing for {variant}: {base}")

        def build(
            candidate: Path,
            base: Path = base,
            variant: str = variant,
            base_name: str = base_name,
            patch_name: str | None = patch_name,
            inputs: list[ExtractionReceipt] = inputs,
        ) -> VariantProvenance:
            _copy_tree(base, candidate)
            overlay_files: list[str] = []
            if variant == "demo":
                shared = extracted / "demo" / "installed" / "Shared_Files"
                if shared.is_dir():
                    overlay_files.extend(_apply_overlay(shared, candidate))
            if patch_name:
                overlay_files.extend(
                    _apply_overlay(_overlay_root(extracted / patch_name), candidate)
                )
            files = _content_manifest(candidate, ".wiz8-variant.json")
            receipt = VariantProvenance(
                variant=variant,
                base_extraction=base_name,
                patch_extraction=patch_name,
                patch_chain=[patch_name] if patch_name else [],
                overlay_files=overlay_files,
                input_hashes=_variant_input_hashes(inputs),
                configuration_hash=_variant_configuration_hash(
                    variant, base_name, patch_name, inputs
                ),
                implementation_revision=_implementation_revision(VARIANT_IMPLEMENTATION_FILES),
                tool_versions=_variant_tool_versions(inputs),
                files=files,
                output_tree_hash=tree_hash(files),
                created_at_utc_non_authoritative=datetime.now(UTC).isoformat(),
            )
            write_generated_document(candidate / ".wiz8-variant.json", receipt)
            return receipt

        record = build_directory_atomically(destination, settings.work_dir, build)
        write_generated_document(_variant_receipt_path(settings, variant), record)
        records.append(record)
    output = VariantProvenanceManifest(variants=records)
    write_generated_document(variant_provenance_path(settings), output)
    return output.model_dump(mode="json", by_alias=True)


def variant_diff(settings: Settings) -> dict[str, Any]:
    manifests: dict[str, dict[str, dict[str, Any]]] = {}
    for path in sorted((settings.work_dir / "variants").glob("*/.wiz8-variant.json")):
        document = load_generated_document(path, VariantProvenance)
        data = document.model_dump(mode="json", by_alias=True)
        manifests[document.variant] = {entry["path"].casefold(): entry for entry in data["files"]}
    base = manifests.get("gog-base", {})
    comparisons: dict[str, Any] = {}
    for variant, files in sorted(manifests.items()):
        if variant == "gog-base":
            continue
        comparisons[variant] = {
            "added": sorted(entry["path"] for key, entry in files.items() if key not in base),
            "removed": sorted(entry["path"] for key, entry in base.items() if key not in files),
            "changed": sorted(
                files[key]["path"]
                for key in files.keys() & base.keys()
                if files[key]["sha256"] != base[key]["sha256"]
            ),
            "identical": sum(
                1
                for key in files.keys() & base.keys()
                if files[key]["sha256"] == base[key]["sha256"]
            ),
        }
    result = {"schema": "wiz8.variant-diff", "base": "gog-base", "comparisons": comparisons}
    atomic_json(settings.build_dir / "reports" / "variant-diff.json", result)
    return result
