from __future__ import annotations

from pathlib import Path

from wiz8decomp.inputs.file_types import detect, extension_mismatch


def test_zip_signature_beats_misleading_extension(tmp_path: Path) -> None:
    path = tmp_path / "not-an-exe.exe"
    path.write_bytes(b"PK\x03\x04" + b"\0" * 32)
    result = detect(path)
    assert result.detected_type == "archive"
    assert result.container == "ZIP"
    assert extension_mismatch(path, result.detected_type, result.container)


def test_pe_signature(synthetic_pe: Path) -> None:
    result = detect(synthetic_pe)
    assert result.detected_type == "pe-executable"
    assert result.container is None


def test_iso_signature(tmp_path: Path) -> None:
    path = tmp_path / "media.dat"
    data = bytearray(0x9000)
    data[0x8001:0x8006] = b"CD001"
    path.write_bytes(data)
    assert detect(path).detected_type == "iso-image"
