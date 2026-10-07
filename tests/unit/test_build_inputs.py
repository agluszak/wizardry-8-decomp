from __future__ import annotations

import zipfile
from pathlib import Path

import pytest
from wiz8decomp.build_inputs import (
    _download_verified,
    _safe_extract_zip,
)


def test_safe_zip_extraction_rejects_parent_traversal(tmp_path: Path) -> None:
    archive = tmp_path / "bad.zip"
    with zipfile.ZipFile(archive, "w") as stream:
        stream.writestr("../escaped.txt", "bad")
    with pytest.raises(RuntimeError, match="escapes extraction root"):
        _safe_extract_zip(archive, tmp_path / "output")
    assert not (tmp_path / "escaped.txt").exists()


def _pin(payload: bytes) -> str:
    import hashlib

    return hashlib.sha256(payload).hexdigest()


def test_download_verified_falls_through_after_network_failure(monkeypatch, capfd) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead.example" in url:
            raise OSError("connection refused")
        return payload

    monkeypatch.setattr("wiz8decomp.build_inputs._download", fake_download)

    result = _download_verified(
        ["https://dead.example/a.tar.gz", "https://live.example/a.tar.gz"],
        _pin(payload),
        label="zlib-1.0.4",
    )

    assert result == payload
    assert "dead.example" in capfd.readouterr().err


def test_download_verified_exhaustion_reports_each_source(monkeypatch) -> None:
    payload = b"reviewed archive bytes"

    def fake_download(url: str, **kwargs) -> bytes:
        if "dead" in url:
            raise OSError("gone")
        return b"different bytes"

    monkeypatch.setattr("wiz8decomp.build_inputs._download", fake_download)

    with pytest.raises(RuntimeError, match="all reviewed sources exhausted") as error:
        _download_verified(
            ["https://dead.example/a.zip", "https://mutated.example/a.zip"],
            _pin(payload),
            label="unzip540",
        )

    message = str(error.value)
    assert "dead.example/a.zip: network error" in message
    assert "mutated.example/a.zip: hash mismatch" in message
