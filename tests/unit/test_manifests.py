from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.manifest_models import (
    VariantModuleInventory,
    VariantModuleSummary,
    VariantProvenance,
    VariantProvenanceManifest,
    load_variant_module_inventory,
    load_variant_provenance,
    variant_module_inventory_path,
    variant_provenance_path,
    write_generated_document,
)
from wiz8decomp.paths import tree_hash, tree_manifest


def test_tree_manifest_is_sorted_and_mtime_independent(tmp_path: Path) -> None:
    (tmp_path / "z.txt").write_text("z")
    (tmp_path / "a.txt").write_text("a")
    first = tree_manifest(tmp_path)
    (tmp_path / "a.txt").touch()
    second = tree_manifest(tmp_path)
    assert [item["path"] for item in first] == ["a.txt", "z.txt"]
    assert first == second
    assert tree_hash(first) == tree_hash(second)


def _settings(tmp_path: Path) -> Settings:
    for name in ("ghidra", "inputs", "work", "repo"):
        (tmp_path / name).mkdir()
    return Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "inputs",
            "WIZ8_WORK_DIR": tmp_path / "work",
            "repo_dir": tmp_path / "repo",
        }
    )


def test_variant_documents_have_distinct_paths_and_strict_schemas(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    provenance = VariantProvenanceManifest(
        variants=[
            VariantProvenance(
                variant="gog-base",
                base_extraction="gog-base",
                patch_extraction=None,
                patch_chain=[],
                overlay_files=[],
                input_hashes=["1" * 64],
                configuration_hash="2" * 64,
                implementation_revision="3" * 64,
                tool_versions={},
                files=[],
                output_tree_hash="0" * 64,
                created_at_utc_non_authoritative="ignored",
            )
        ]
    )
    module_inventory = VariantModuleInventory(
        variants=[VariantModuleSummary(id="gog-base", module_count=3)]
    )

    write_generated_document(variant_provenance_path(settings), provenance)
    write_generated_document(variant_module_inventory_path(settings), module_inventory)

    assert variant_provenance_path(settings) != variant_module_inventory_path(settings)
    assert load_variant_provenance(settings) == provenance
    assert load_variant_module_inventory(settings) == module_inventory
    assert not (settings.build_dir / "manifests" / "variants.json").exists()


@pytest.mark.parametrize(
    "payload",
    [
        '{"schema":"wiz8.variant-inventory","variants":[]}',
        '{"schema":"wiz8.variants","format_version":1,"variants":[]}',
        '{"schema":"wiz8.variants","variants":[],"extra":true}',
    ],
)
def test_variant_provenance_loader_rejects_incompatible_documents(
    tmp_path: Path, payload: str
) -> None:
    settings = _settings(tmp_path)
    path = variant_provenance_path(settings)
    path.parent.mkdir(parents=True)
    path.write_text(payload, encoding="utf-8")

    with pytest.raises(RuntimeError, match="invalid VariantProvenanceManifest"):
        load_variant_provenance(settings)
