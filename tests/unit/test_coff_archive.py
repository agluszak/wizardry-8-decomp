from __future__ import annotations

from pathlib import Path

import pytest
from wiz8decomp.binary.coff_archive import coff_member_kind, read_coff_archive


def _header(name: str, size: int) -> bytes:
    return (
        name.encode("ascii").ljust(16)
        + b"0".ljust(12)
        + b"0".ljust(6)
        + b"0".ljust(6)
        + b"0".ljust(8)
        + str(size).encode("ascii").ljust(10)
        + b"`\n"
    )


def _member(name: str, data: bytes) -> bytes:
    return _header(name, len(data)) + data + (b"\n" if len(data) & 1 else b"")


def test_reads_long_names_and_preserves_duplicate_members(tmp_path: Path) -> None:
    long_names = b"build\\one.obj\0build\\two.obj\0"
    first = b"\x4c\x01" + b"\0" * 18
    second = b"\x00\x00\xff\xff" + b"\0" * 16
    second_name_offset = long_names.index(b"build\\two.obj")
    archive = tmp_path / "fixture.lib"
    archive.write_bytes(
        b"!<arch>\n"
        + _member("/", b"symbols")
        + _member("//", long_names)
        + _member("/0", first)
        + _member(f"/{second_name_offset}", second)
        + _member("same.obj/", first)
    )

    members = read_coff_archive(archive)
    assert [member.name for member in members] == ["build\\one.obj", "build\\two.obj", "same.obj"]
    assert [member.index for member in members] == [0, 1, 2]
    assert coff_member_kind(members[0].data) == "coff-i386"
    assert coff_member_kind(members[1].data) == "coff-import"


def test_rejects_non_archive(tmp_path: Path) -> None:
    path = tmp_path / "bad.lib"
    path.write_bytes(b"not an archive")
    with pytest.raises(RuntimeError, match="not a COFF archive"):
        read_coff_archive(path)
