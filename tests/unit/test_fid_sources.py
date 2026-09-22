from __future__ import annotations

import zipfile
from pathlib import Path

import pytest
from wiz8decomp.config import Settings
from wiz8decomp.ghidra.fid_seeds import (
    _cmake_seed_target,
    _download_verified,
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
    by_key = {(item["toolchain"], item["library"], item["variant"]): item for item in merged}
    assert by_key[("vc6-sp5", "ijg-jpeg-6", "release-md-o2")]["marker"] == "new"
    assert (
        by_key[("vc6-sp5", "msvc-crt-static", "vc6-sp5-multithreaded-static")]["marker"]
        == "preserved"
    )


def _pin(payload: bytes) -> str:
    import hashlib

    return hashlib.sha256(payload).hexdigest()


def test_download_verified_falls_through_after_network_failure(monkeypatch, capfd) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead.example" in url:
            raise OSError("connection refused")
        return payload

    monkeypatch.setattr("wiz8decomp.ghidra.fid_seeds._download", fake_download)

    result = _download_verified(
        ["https://dead.example/a.tar.gz", "https://live.example/a.tar.gz"],
        _pin(payload),
        label="zlib-1.0.4",
    )

    assert result == payload
    assert "dead.example" in capfd.readouterr().err


def test_download_verified_rejects_mutated_mirror_and_uses_next(monkeypatch, capfd) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        return b"mutated mirror bytes" if "mutated" in url else payload

    monkeypatch.setattr("wiz8decomp.ghidra.fid_seeds._download", fake_download)

    result = _download_verified(
        ["https://mutated.example/a.tar.gz", "https://good.example/a.tar.gz"],
        _pin(payload),
        label="jpeg-6",
    )

    assert result == payload
    assert "hash mismatch" in capfd.readouterr().err


def test_download_verified_exhaustion_reports_each_source(monkeypatch) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead" in url:
            raise OSError("gone")
        return b"different bytes"

    monkeypatch.setattr("wiz8decomp.ghidra.fid_seeds._download", fake_download)

    with pytest.raises(RuntimeError, match="all reviewed sources exhausted") as error:
        _download_verified(
            ["https://dead.example/a.zip", "https://mutated.example/a.zip"],
            _pin(payload),
            label="unzip540",
        )

    message = str(error.value)
    assert "dead.example/a.zip: network error" in message
    assert "mutated.example/a.zip: hash mismatch" in message
