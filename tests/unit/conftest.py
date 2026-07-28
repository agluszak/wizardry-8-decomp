from __future__ import annotations

import struct
from pathlib import Path

import pytest


@pytest.fixture
def synthetic_pe(tmp_path: Path) -> Path:
    data = bytearray(0x400)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x14C, 1, 0x12345678, 0, 0, 0xE0, 0x010F)
    optional = 0x98
    struct.pack_into("<HBB", data, optional, 0x10B, 6, 0)
    struct.pack_into("<III", data, optional + 4, 0x200, 0, 0)
    struct.pack_into("<III", data, optional + 16, 0x1000, 0x1000, 0x2000)
    struct.pack_into("<I", data, optional + 28, 0x400000)
    struct.pack_into("<II", data, optional + 32, 0x1000, 0x200)
    struct.pack_into("<HHHHHH", data, optional + 40, 4, 0, 0, 0, 4, 0)
    struct.pack_into("<III", data, optional + 52, 0, 0x2000, 0x200)
    struct.pack_into("<I", data, optional + 64, 0xDEADBEEF)
    struct.pack_into("<HH", data, optional + 68, 3, 0)
    struct.pack_into("<IIIIII", data, optional + 72, 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    section = optional + 0xE0
    data[section : section + 8] = b".text\0\0\0"
    struct.pack_into(
        "<IIIIIIHHI", data, section + 8, 1, 0x1000, 0x200, 0x200, 0, 0, 0, 0, 0x60000020
    )
    data[0x200] = 0xC3
    path = tmp_path / "fixture.bin"
    path.write_bytes(data)
    return path
