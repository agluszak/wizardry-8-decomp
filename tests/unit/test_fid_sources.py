from __future__ import annotations

import zipfile
from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.ghidra.fid_seeds import (
    _cmake_seed_target,
    _merge_seed_records,
    _safe_extract_zip,
    load_static_libraries,
)


def test_safe_zip_extraction_rejects_parent_traversal(tmp_path: Path) -> None:
    archive = tmp_path / "bad.zip"
    with zipfile.ZipFile(archive, "w") as stream:
        stream.writestr("../escaped.txt", "bad")
    with pytest.raises(RuntimeError, match="escapes extraction root"):
        _safe_extract_zip(archive, tmp_path / "output")
    assert not (tmp_path / "escaped.txt").exists()


def test_safe_zip_extraction_accepts_normal_tree(tmp_path: Path) -> None:
    archive = tmp_path / "good.zip"
    with zipfile.ZipFile(archive, "w") as stream:
        stream.writestr("source/unit.c", "int unit(void) { return 1; }")
    output = tmp_path / "output"
    _safe_extract_zip(archive, output)
    assert (output / "source" / "unit.c").is_file()


def test_cmake_seed_target_matches_declared_object_library() -> None:
    assert _cmake_seed_target("ijg-jpeg-6", "release-md-o2") == "fid_ijg_jpeg_6_release_md_o2"
    assert (
        _cmake_seed_target("infozip-unzip-5.4", "upstream-release-mt-o2")
        == "fid_infozip_unzip_5_4_upstream_release_mt_o2"
    )


def test_seed_record_merge_replaces_stable_key_and_preserves_other_kinds(
    tmp_path: Path,
) -> None:
    settings = Settings.model_validate(
        {
            "GHIDRA_INSTALL_DIR": tmp_path / "ghidra",
            "WIZ8_INPUT_DIR": tmp_path / "input",
            "WIZ8_WORK_DIR": tmp_path / "work",
        }
    )
    config = load_static_libraries(settings)
    source_old = {
        "toolchain": "vc6-sp5",
        "library": "ijg-jpeg-6",
        "variant": "release-md-o2",
        "marker": "old",
    }
    source_new = {**source_old, "marker": "new"}
    precompiled = {
        "toolchain": "vc6-sp5",
        "library": "msvc-crt-static",
        "variant": "vc6-sp5-multithreaded-static",
        "marker": "preserved",
    }
    merged = _merge_seed_records(config, [source_old, precompiled], [source_new])
    by_key = {
        (item["toolchain"], item["library"], item["variant"]): item for item in merged
    }
    assert by_key[("vc6-sp5", "ijg-jpeg-6", "release-md-o2")]["marker"] == "new"
    assert (
        by_key[("vc6-sp5", "msvc-crt-static", "vc6-sp5-multithreaded-static")][
            "marker"
        ]
        == "preserved"
    )
