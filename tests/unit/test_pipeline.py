from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.extract import variants
from wiz8decomp.inputs.manifest import InputRecord
from wiz8decomp.manifest_models import TreeFile
from wiz8decomp.paths import build_directory_atomically, sha256_file
from wiz8decomp.pipeline import PipelineStage, clean_pipeline


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


def test_modified_output_tree_is_rejected(tmp_path: Path) -> None:
    output = tmp_path / "tree"
    output.mkdir()
    target = output / "payload.bin"
    target.write_bytes(b"original")
    recorded = [TreeFile(path="payload.bin", size=8, sha256=sha256_file(target))]
    recorded_hash = variants.tree_hash([item.model_dump() for item in recorded])

    errors, _ = variants._tree_receipt_errors(output, ".receipt", recorded, recorded_hash)
    assert errors == []

    target.write_bytes(b"modified")
    errors, _ = variants._tree_receipt_errors(output, ".receipt", recorded, recorded_hash)
    assert "per-file output manifest differs from receipt" in errors
    assert any(error.startswith("output tree hash differs") for error in errors)


def test_changed_extraction_recipe_changes_configuration_hash() -> None:
    common = {
        "relative_path": "patch.exe",
        "size": 1,
        "sha256": "0" * 64,
        "detected_type": "pe-executable",
        "configured_role": "patch-1261",
        "role_source": "local-config",
    }
    first = InputRecord(**common, installer_technology="Inno Setup")
    second = InputRecord(**common, installer_technology=None)
    assert variants._extraction_configuration_hash(
        "patch-1261", first
    ) != variants._extraction_configuration_hash("patch-1261", second)


def test_failed_extraction_leaves_no_final_destination(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    settings = _settings(tmp_path)
    source = settings.input_dir / "patch.bin"
    source.write_bytes(b"input")
    record = InputRecord(
        relative_path="patch.bin",
        size=source.stat().st_size,
        sha256=sha256_file(source),
        detected_type="archive",
        configured_role="patch-1261",
        role_source="local-config",
    )
    monkeypatch.setattr(variants, "_record_for_role", lambda _settings, _role: (source, record))
    monkeypatch.setattr(variants, "_extraction_tool_versions", lambda _role, _record: {})

    def fail(_source: Path, destination: Path, *, log_path: Path):
        destination.mkdir(parents=True)
        (destination / "partial.bin").write_bytes(b"partial")
        raise RuntimeError("synthetic extractor failure")

    monkeypatch.setattr(variants, "extract_with_7z", fail)
    destination = settings.work_dir / "extracted" / "patch-1261"
    with pytest.raises(RuntimeError, match="synthetic extractor failure"):
        variants.extract_role(settings, "patch-1261")

    assert not destination.exists()
    assert list(destination.parent.glob(".patch-1261.building-*")) == []


def test_atomic_directory_rejects_target_outside_work_dir(tmp_path: Path) -> None:
    work = tmp_path / "work"
    work.mkdir()
    with pytest.raises(ValueError, match="must be a child"):
        build_directory_atomically(tmp_path / "outside", work, lambda candidate: None)


def test_clean_variants_is_scoped_and_invalidates_downstream_manifests(tmp_path: Path) -> None:
    settings = _settings(tmp_path)
    extracted = settings.work_dir / "extracted"
    variants_root = settings.work_dir / "variants"
    extracted.mkdir()
    variants_root.mkdir()
    (extracted / "keep.bin").write_bytes(b"keep")
    (variants_root / "remove.bin").write_bytes(b"remove")
    manifest = settings.build_dir / "manifests" / "variant-provenance.json"
    manifest.parent.mkdir(parents=True)
    manifest.write_text("generated", encoding="utf-8")
    sentinel = settings.repo_dir / "keep.txt"
    sentinel.write_text("keep", encoding="utf-8")

    result = clean_pipeline(settings, PipelineStage.variants)

    assert result["stage"] == "variants"
    assert extracted.is_dir()
    assert not variants_root.exists()
    assert not manifest.exists()
    assert sentinel.read_text(encoding="utf-8") == "keep"
