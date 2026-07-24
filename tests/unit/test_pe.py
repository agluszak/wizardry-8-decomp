from __future__ import annotations

import struct
from pathlib import Path

from wiz8decomp.binary.fingerprints import metadata_normalized_pe_hash
from wiz8decomp.binary.pe import inspect_pe, is_pe


def test_minimal_pe_metadata(synthetic_pe: Path) -> None:
    assert is_pe(synthetic_pe)
    result = inspect_pe(synthetic_pe, "fixture", "fixture.bin")
    assert result["machine"] == "x86"
    assert result["image_base"] == 0x400000
    assert result["entry_point"] == 0x401000
    assert result["linker_version"] == "6.0"
    assert result["sections"][0]["name"] == ".text"


def test_metadata_normalization_ignores_timestamp_and_checksum(synthetic_pe: Path, tmp_path: Path) -> None:
    modified = bytearray(synthetic_pe.read_bytes())
    struct.pack_into("<I", modified, 0x88, 0xCAFEBABE)
    struct.pack_into("<I", modified, 0x98 + 64, 0x12345678)
    other = tmp_path / "other.exe"
    other.write_bytes(modified)
    assert synthetic_pe.read_bytes() != other.read_bytes()
    assert metadata_normalized_pe_hash(synthetic_pe) == metadata_normalized_pe_hash(other)

