from __future__ import annotations

import struct
from pathlib import Path

import pytest
from wiz8decomp.binary.coff_archive import coff_member_kind, named_iat_archive, read_coff_archive


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


def test_named_iat_separates_caller_symbol_from_provider_name(tmp_path: Path) -> None:
    caller = "?srAssertFail@@YAXPBD0J0@Z"
    provider = "?srAssertFail@@YAXPBD0J0ZZ"
    archive = tmp_path / "assert.lib"
    archive.write_bytes(named_iat_archive("SR.dll", caller, provider))
    (member,) = read_coff_archive(archive)
    assert member.name == "SR.dll"  # DLL grouping controls terminator placement.
    data = member.data
    machine, sections, _, symbols, count, _, _ = struct.unpack_from("<HHLLLHH", data)
    assert machine == 0x14C
    headers = [struct.unpack_from("<8sLLLLLLHHL", data, 20 + i * 40) for i in range(sections)]
    by_name = {header[0].rstrip(b"\0"): header for header in headers}
    assert set(by_name) == {b".idata$4", b".idata$5", b".idata$6"}
    hint = by_name[b".idata$6"]
    assert data[hint[4] + 2 : hint[4] + hint[3]].rstrip(b"\0") == provider.encode()
    strings = data[symbols + count * 18 :]
    assert b"__imp_" + caller.encode() + b"\0" in strings
    assert b"__IMPORT_DESCRIPTOR_SR\0" in strings
    for name in (b".idata$4", b".idata$5"):
        offset, symbol, relocation = struct.unpack_from("<LLH", data, by_name[name][5])
        assert offset == 0
        assert relocation == 7  # IMAGE_REL_I386_DIR32NB, not an absolute pointer.
        assert data[symbols + symbol * 18 : symbols + symbol * 18 + 8] == b".idata$6"
