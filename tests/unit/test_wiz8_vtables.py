from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

from wiz8decomp.msvc_tables import (
    _decode_table_store,
    _decode_vbtable,
    _decode_vftable,
    _exported_table_oracle,
    _source_vtable_markers,
    _vbtable_owner_offset_hint,
    _vbtable_store_matches,
)


@dataclass(frozen=True)
class _Section:
    executable: bool
    name: str


class _Image:
    def __init__(
        self, base: int, data: bytes, executable_ranges: list[range] | None = None
    ) -> None:
        self.base = base
        self.image_base = base
        self.data = data
        self.executable_ranges = executable_ranges or []

    def read(self, address: int, size: int) -> bytes:
        offset = address - self.base
        if offset < 0:
            return b""
        return self.data[offset : offset + size]

    def read_u32(self, address: int) -> int | None:
        raw = self.read(address, 4)
        return struct.unpack("<I", raw)[0] if len(raw) == 4 else None

    def read_i32(self, address: int) -> int | None:
        raw = self.read(address, 4)
        return struct.unpack("<i", raw)[0] if len(raw) == 4 else None

    def read_cstring(self, address: int, limit: int = 512) -> str | None:
        raw = self.read(address, limit)
        end = raw.find(b"\0")
        if end < 0:
            return None
        return raw[:end].decode("latin-1")

    def section_at(self, address: int) -> _Section | None:
        if any(address in values for values in self.executable_ranges):
            return _Section(executable=True, name=".text")
        return _Section(executable=False, name=".rdata")


def test_decode_table_store_with_byte_displacement() -> None:
    image = _Image(0x1000, bytes.fromhex("c7 46 04 a8 c6 5e 00"))

    store = _decode_table_store(image, 0x1003)

    assert store is not None
    assert store.instruction == 0x1000
    assert store.target == 0x005EC6A8
    assert store.base_register == "esi"
    assert store.index_register is None
    assert store.object_offset == 4


def test_decode_table_store_with_dword_displacement() -> None:
    image = _Image(0x1000, bytes.fromhex("c7 86 9c 00 00 00 94 d8 5e 00"))

    store = _decode_table_store(image, 0x1006)

    assert store is not None
    assert store.base_register == "esi"
    assert store.object_offset == 0x9C
    assert store.target == 0x005ED894


def test_decode_table_store_keeps_indexed_virtual_base_destination() -> None:
    image = _Image(0x1000, bytes.fromhex("c7 44 32 04 8c c6 5e 00"))

    store = _decode_table_store(image, 0x1004)

    assert store is not None
    assert store.base_register == "edx"
    assert store.index_register == "esi"
    assert store.scale == 1
    assert store.displacement == 4
    assert store.object_offset is None
    assert store.target == 0x005EC68C


def test_decode_table_store_marks_absolute_destination_as_weak() -> None:
    image = _Image(0x1000, bytes.fromhex("c7 05 00 20 40 00 a8 c6 5e 00"))

    store = _decode_table_store(image, 0x1006)

    assert store is not None
    assert store.absolute_destination
    assert store.base_register is None
    assert store.object_offset is None


def test_decode_vftable_stops_at_any_referenced_data_boundary() -> None:
    raw = bytearray(0x40)
    struct.pack_into("<III", raw, 0, 0x4000, 0x4010, 0x4020)
    image = _Image(0x2000, bytes(raw), [range(0x4000, 0x5000)])

    slots = _decode_vftable(image, 0x2000, {0x2000, 0x2004, 0x2008}, {0x2000, 0x2008})

    assert slots == [0x4000, 0x4010]


def test_decode_vbtable_keeps_secondary_base_back_adjustment() -> None:
    raw = struct.pack("<ii", -4, 8) + bytes(24)
    image = _Image(0x3000, raw)
    entries = _decode_vbtable(image, 0x3000, set(), {0x3000, 0x3008})

    # Real SR example shape: the vbptr can be at complete-object +0x1c while
    # entry zero is -4 because the owning base subobject starts at +0x18.
    store = _decode_table_store(_Image(0x1000, bytes.fromhex("c7 46 1c 00 30 00 00")), 0x1003)

    assert entries == [-4, 8]
    assert store is not None and _vbtable_store_matches(entries, store)
    assert _vbtable_owner_offset_hint(entries, store) == 0x18


def test_vbtable_rejects_absolute_pointer_install() -> None:
    entries = [-4, 12]
    store = _decode_table_store(
        _Image(0x1000, bytes.fromhex("c7 05 00 20 40 00 00 30 00 00")), 0x1006
    )

    assert store is not None
    assert not _vbtable_store_matches(entries, store)


def test_exported_table_oracle_reads_msvc_special_names() -> None:
    base = 0x10000000
    raw = bytearray(0x400)
    struct.pack_into("<I", raw, 0x3C, 0x80)
    optional = 0x80 + 4 + 20
    struct.pack_into("<I", raw, optional + 92, 1)
    struct.pack_into("<II", raw, optional + 96, 0x200, 0x100)
    struct.pack_into(
        "<III III IIII",
        raw,
        0x200,
        0,
        0,
        0,
        0,
        1,
        2,
        2,
        0x280,
        0x290,
        0x2A0,
    )
    struct.pack_into("<II", raw, 0x280, 0x350, 0x360)
    struct.pack_into("<II", raw, 0x290, 0x2B0, 0x2C0)
    struct.pack_into("<HH", raw, 0x2A0, 0, 1)
    raw[0x2B0 : 0x2B0 + len(b"??_7Foo@@6B@\0")] = b"??_7Foo@@6B@\0"
    raw[0x2C0 : 0x2C0 + len(b"??_8Bar@@7B@\0")] = b"??_8Bar@@7B@\0"

    oracle = _exported_table_oracle(_Image(base, bytes(raw)))

    assert oracle == {
        "vftables": {base + 0x350: ["??_7Foo@@6B@"]},
        "vbtables": {base + 0x360: ["??_8Bar@@7B@"]},
    }


def test_source_vtable_markers_are_an_independent_recall_oracle(tmp_path: Path) -> None:
    source = tmp_path / "src" / "wiz8" / "thing.cpp"
    header = tmp_path / "include" / "wiz8" / "thing.h"
    source.parent.mkdir(parents=True)
    header.parent.mkdir(parents=True)
    source.write_text("// VTABLE: WIZ8 0x005EC520 Foo\n", encoding="utf-8")
    header.write_text("// VTABLE: WIZ8 0x005ec68c Bar\n", encoding="utf-8")

    markers = _source_vtable_markers(tmp_path)

    assert markers == {
        0x005EC520: ["src/wiz8/thing.cpp:1"],
        0x005EC68C: ["include/wiz8/thing.h:1"],
    }
