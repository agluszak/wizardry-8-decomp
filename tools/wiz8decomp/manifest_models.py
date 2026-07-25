from __future__ import annotations

from pathlib import Path
from typing import Literal, TypeVar

from pydantic import BaseModel, ConfigDict, Field, ValidationError

from .config import Settings
from .paths import atomic_json


class GeneratedDocument(BaseModel):
    model_config = ConfigDict(extra="forbid", populate_by_name=True)


class TreeFile(GeneratedDocument):
    path: str
    size: int
    sha256: str


class VariantProvenance(GeneratedDocument):
    schema_id: Literal["wiz8.variant-provenance"] = Field(
        "wiz8.variant-provenance", alias="schema"
    )
    variant: str
    base_extraction: str
    patch_extraction: str | None
    patch_chain: list[str]
    overlay_files: list[str]
    files: list[TreeFile]
    output_tree_sha256: str
    created_at_utc_non_authoritative: str


class VariantProvenanceManifest(GeneratedDocument):
    schema_id: Literal["wiz8.variants"] = Field("wiz8.variants", alias="schema")
    variants: list[VariantProvenance]


class VariantModuleSummary(GeneratedDocument):
    id: str
    module_count: int


class VariantModuleInventory(GeneratedDocument):
    schema_id: Literal["wiz8.variant-inventory"] = Field(
        "wiz8.variant-inventory", alias="schema"
    )
    variants: list[VariantModuleSummary]


DocumentT = TypeVar("DocumentT", bound=GeneratedDocument)


def load_generated_document(path: Path, model: type[DocumentT]) -> DocumentT:
    if not path.is_file():
        raise RuntimeError(f"required generated document is missing: {path}")
    try:
        return model.model_validate_json(path.read_text(encoding="utf-8"))
    except ValidationError as error:
        raise RuntimeError(f"invalid {model.__name__} document at {path}: {error}") from error


def variant_provenance_path(settings: Settings) -> Path:
    return settings.build_dir / "manifests" / "variant-provenance.json"


def variant_module_inventory_path(settings: Settings) -> Path:
    return settings.build_dir / "manifests" / "variant-module-inventory.json"


def write_generated_document(path: Path, document: GeneratedDocument) -> None:
    atomic_json(path, document.model_dump(mode="json", by_alias=True))


def load_variant_provenance(settings: Settings) -> VariantProvenanceManifest:
    return load_generated_document(variant_provenance_path(settings), VariantProvenanceManifest)


def load_variant_module_inventory(settings: Settings) -> VariantModuleInventory:
    return load_generated_document(
        variant_module_inventory_path(settings), VariantModuleInventory
    )
