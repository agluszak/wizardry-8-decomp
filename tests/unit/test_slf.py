from __future__ import annotations

import struct
from pathlib import Path

import pytest
from wiz8decomp.binary.slf import (
    SLF_DIRECTORY_ENTRY_SIZE,
    SLF_HEADER_SIZE,
    read_slf,
)


def _fixed(text: bytes, size: int = 256) -> bytes:
    return text + b"\0" * (size - len(text))


def test_read_slf_uses_the_eof_directory_and_low_status_byte(tmp_path: Path) -> None:
    header = struct.pack(
        "<256s256siiHHB3xi",
        _fixed(b"TEST.SLF"),
        _fixed(b"Data\\"),
        2,
        2,
        0xFFFF,
        0x0200,
        1,
        0,
    )
    active = struct.pack("<256sIIBB2xQH2x", _fixed(b"one.bin"), SLF_HEADER_SIZE, 3, 0, 0, 1234, 0)
    deleted = struct.pack(
        "<256sIIBB2xQH2x", _fixed(b"old.bin"), SLF_HEADER_SIZE + 3, 2, 1, 1, 5678, 9
    )
    archive_path = tmp_path / "synthetic.slf"
    archive_path.write_bytes(header + b"abcde" + active + deleted)

    archive = read_slf(archive_path)

    assert archive.header.archive_name == "TEST.SLF"
    assert archive.header.base_path == "Data\\"
    assert archive.header.sort_order == 0xFFFF
    assert archive.header.version == 0x0200
    assert archive.header.contains_subdirectories
    assert archive.directory_offset == SLF_HEADER_SIZE + 5
    assert [entry.path for entry in archive.entries] == ["one.bin", "old.bin"]
    assert archive.entries[0].is_active
    assert not archive.entries[1].is_active
    assert SLF_DIRECTORY_ENTRY_SIZE == 0x118


def test_read_slf_rejects_a_directory_that_overlaps_the_header(tmp_path: Path) -> None:
    archive_path = tmp_path / "invalid.slf"
    archive_path.write_bytes(
        struct.pack(
            "<256s256siiHHB3xi",
            _fixed(b"INVALID"),
            _fixed(b""),
            2,
            2,
            0,
            0,
            0,
            0,
        )
    )

    with pytest.raises(ValueError, match="directory overlaps"):
        read_slf(archive_path)
