from __future__ import annotations

from pathlib import Path
from typing import Literal

from pydantic import BaseModel, ConfigDict, Field, ValidationError

from .config import Settings
from .paths import atomic_json


class GeneratedDocument(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)


class TreeFile(GeneratedDocument):
    path: str
    size: int
    sha256: str


class ToolIdentity(GeneratedDocument):
    executable: str | None
    version: str | None


class CommandReceipt(GeneratedDocument):
    argv: list[str]
    executable: str
    cwd: str
    exit_status: int
    stdout: str
    stderr: str


class ExtractionReceipt(GeneratedDocument):
    schema_id: Literal["wiz8.extraction-receipt"] = Field("wiz8.extraction-receipt", alias="schema")
    role: str
    input_relative_path: str
    input_hashes: list[str]
    input_size: int
    configuration_hash: str
    implementation_revision: str
    tool_versions: dict[str, ToolIdentity]
    commands: list[CommandReceipt]
    wine_used: bool
    output_tree_hash: str
    files: list[TreeFile]
    created_at_utc_non_authoritative: str


class VariantProvenance(GeneratedDocument):
    schema_id: Literal["wiz8.variant-receipt"] = Field("wiz8.variant-receipt", alias="schema")
    variant: str
    base_extraction: str
    patch_extraction: str | None
    patch_chain: list[str]
    overlay_files: list[str]
    input_hashes: list[str]
    configuration_hash: str
    implementation_revision: str
    tool_versions: dict[str, ToolIdentity]
    files: list[TreeFile]
    output_tree_hash: str
    created_at_utc_non_authoritative: str


class VariantProvenanceManifest(GeneratedDocument):
    schema_id: Literal["wiz8.variant-provenance"] = Field("wiz8.variant-provenance", alias="schema")
    variants: list[VariantProvenance]


class VariantModuleSummary(GeneratedDocument):
    id: str
    module_count: int


class VariantModuleInventory(GeneratedDocument):
    schema_id: Literal["wiz8.variant-inventory"] = Field("wiz8.variant-inventory", alias="schema")
    variants: list[VariantModuleSummary]


def load_generated_document[DocumentT: GeneratedDocument](
    path: Path, model: type[DocumentT]
) -> DocumentT:
    if not path.is_file():
        raise RuntimeError(f"required generated document is missing: {path}")
    try:
        return model.model_validate_json(path.read_text(encoding="utf-8"))
    except ValidationError as error:
        failures = error.errors(include_input=False, include_url=False)
        details = [
            f"{'.'.join(str(part) for part in failure['loc'])}: {failure['msg']}"
            for failure in failures[:6]
        ]
        if len(failures) > len(details):
            details.append(f"and {len(failures) - len(details)} more validation errors")
        raise RuntimeError(
            f"invalid {model.__name__} document at {path}: " + "; ".join(details)
        ) from error


def variant_provenance_path(settings: Settings) -> Path:
    return settings.build_dir / "manifests" / "variant-provenance.json"


def variant_module_inventory_path(settings: Settings) -> Path:
    return settings.build_dir / "manifests" / "variant-module-inventory.json"


def write_generated_document(path: Path, document: GeneratedDocument) -> None:
    atomic_json(path, document.model_dump(mode="json", by_alias=True))


def load_variant_provenance(settings: Settings) -> VariantProvenanceManifest:
    return load_generated_document(variant_provenance_path(settings), VariantProvenanceManifest)


def load_variant_module_inventory(settings: Settings) -> VariantModuleInventory:
    return load_generated_document(variant_module_inventory_path(settings), VariantModuleInventory)
