from __future__ import annotations

import struct
from collections import Counter
from dataclasses import dataclass

from wiz8decomp.binary.code import disassembler
from wiz8decomp.msvc_table_analysis import (
    _classify_slot,
    _construction_families,
    _import_slots,
    _receiver_provenance,
    compare_table_reports,
    table_shape_fingerprint,
    vtable_recovery_debt,
)


def test_debt_does_not_promote_a_named_construction_phase_destructor() -> None:
    table = {
        "address": "0x005ebfe8",
        "slots": [
            {
                "target": "0x00429fb0",
                "resolution": {
                    "source_name": "Vector::`scalar deleting destructor' (construction-phase copy)",
                    "shared_count": 2,
                },
            }
        ],
        "writes": [
            {"function": "0x00429fb0", "receiver_provenance": "incoming-ecx", "receiver_offset": 0}
        ],
    }
    result = vtable_recovery_debt({"vftables": [table]}, {})
    assert not result["groups"]["unreviewed-high-confidence-final"]
    candidate = result["groups"]["probable-construction-phase"][0]
    assert candidate["address"] == "0x005ebfe8"
    assert candidate["deleting_destructor_evidence"]
    assert candidate["shared_slots"] == 1


def test_debt_keeps_unknown_receivers_ambiguous_and_vbtables_separate() -> None:
    result = vtable_recovery_debt(
        {
            "vftables": [
                {
                    "address": "0x1000",
                    "slots": [
                        {
                            "target": "0x2000",
                            "resolution": {"source_name": "Widget::`scalar deleting destructor'"},
                        }
                    ],
                    "writes": [{"receiver_provenance": "unknown", "receiver_offset": None}],
                }
            ],
            "vbtables": [
                {
                    "address": "0x3000",
                    "entries": [{"displacement": -4}, {"displacement": 12}],
                    "writes": [],
                }
            ],
        },
        {},
    )
    assert result["groups"]["ambiguous"][0]["address"] == "0x1000"
    assert result["groups"]["vbtable"][0]["entries"][1]["displacement"] == 12
    assert not result["groups"]["unreviewed-high-confidence-final"]


@dataclass(frozen=True)
class _Section:
    executable: bool


class _Image:
    def __init__(self, base: int, data: bytes) -> None:
        self.base = base
        self.image_base = base
        self.data = data

    def read(self, address: int, size: int) -> bytes:
        offset = address - self.base
        if offset < 0:
            return b""
        return self.data[offset : offset + size]

    def read_u32(self, address: int) -> int | None:
        raw = self.read(address, 4)
        return struct.unpack("<I", raw)[0] if len(raw) == 4 else None

    def read_cstring(self, address: int, limit: int = 512) -> str | None:
        raw = self.read(address, limit)
        end = raw.find(b"\0")
        if end < 0:
            return None
        return raw[:end].decode("latin-1")

    def offset(self, address: int) -> int | None:
        offset = address - self.base
        return offset if 0 <= offset < len(self.data) else None

    def virtual_address(self, offset: int) -> int | None:
        return self.base + offset if 0 <= offset < len(self.data) else None

    def is_code(self, _address: int) -> bool:
        return True


def _write(**updates: object) -> dict[str, object]:
    row: dict[str, object] = {
        "instruction": "0x00001004",
        "base_register": "esi",
        "index_register": None,
        "scale": 1,
        "displacement": 0,
        "absolute_destination": False,
    }
    row.update(updates)
    return row


def test_receiver_provenance_tracks_saved_incoming_ecx() -> None:
    # nop padding; push esi; mov esi, ecx; table store starts at +4.
    image = _Image(0x1000, bytes.fromhex("90 56 8b f1 c7 06 00 30 00 00"))

    result = _receiver_provenance(image, disassembler(), _write())

    assert result == {
        "function": "0x00001001",
        "receiver_provenance": "incoming-ecx",
        "receiver_offset": 0,
    }


def test_receiver_provenance_keeps_virtual_base_index_dynamic() -> None:
    image = _Image(0x1000, bytes.fromhex("90 56 8b f1 c7 44 32 04 00 30 00 00"))

    result = _receiver_provenance(
        image,
        disassembler(),
        _write(base_register="edx", index_register="esi", displacement=4),
    )

    assert result["receiver_provenance"] == "incoming-ecx-plus-dynamic"
    assert result["receiver_offset"] == 4


def test_receiver_provenance_survives_call_in_callee_saved_register() -> None:
    image = _Image(
        0x1000,
        bytes.fromhex("90 56 8b f1 e8 00 00 00 00 c7 06 00 30 00 00"),
    )
    write = _write(instruction="0x00001009")

    result = _receiver_provenance(image, disassembler(), write)

    assert result["receiver_provenance"] == "incoming-ecx"
    assert result["receiver_offset"] == 0


def test_import_slots_reads_named_pe32_import() -> None:
    base = 0x10000000
    raw = bytearray(0x400)
    struct.pack_into("<I", raw, 0x3C, 0x80)
    optional = 0x80 + 4 + 20
    struct.pack_into("<H", raw, optional, 0x10B)
    struct.pack_into("<I", raw, optional + 92, 2)
    struct.pack_into("<II", raw, optional + 96 + 8, 0x200, 0x40)
    struct.pack_into("<IIIII", raw, 0x200, 0x280, 0, 0, 0x2C0, 0x290)
    struct.pack_into("<II", raw, 0x280, 0x2D0, 0)
    raw[0x2C0 : 0x2C0 + len(b"MSVCRT.dll\0")] = b"MSVCRT.dll\0"
    raw[0x2D0:0x2D2] = b"\0\0"
    raw[0x2D2 : 0x2D2 + len(b"_purecall\0")] = b"_purecall\0"

    slots = _import_slots(_Image(base, bytes(raw)))

    assert slots == {base + 0x290: "MSVCRT.dll!_purecall"}


def test_slot_resolution_identifies_purecall_import() -> None:
    result = _classify_slot(
        _Image(0x1000, b""),
        0x4000,
        {0x4000: "MSVCRT.dll!_purecall"},
        {},
        Counter({0x4000: 21}),
    )

    assert result == {
        "kind": "pure-virtual",
        "name": "MSVCRT.dll!_purecall",
        "shared_count": 21,
    }


def test_slot_resolution_identifies_scalar_deleting_destructor_export() -> None:
    result = _classify_slot(
        _Image(0x1000, b""),
        0x4000,
        {},
        {0x4000: ["??_GFoo@@UAEPAXI@Z"]},
        Counter({0x4000: 1}),
    )

    assert result["kind"] == "scalar-deleting-destructor"


def test_construction_family_groups_same_receiver_transitions() -> None:
    report = {
        "vftables": [
            {
                "address": "0x00500000",
                "slot_count": 8,
                "export_names": [],
                "writes": [
                    {
                        "instruction": "0x00401010",
                        "function": "0x00401000",
                        "receiver_provenance": "incoming-ecx",
                        "receiver_offset": 0,
                    }
                ],
            },
            {
                "address": "0x00500040",
                "slot_count": 13,
                "export_names": ["??_7Foo@@6B@"],
                "writes": [
                    {
                        "instruction": "0x00401040",
                        "function": "0x00401000",
                        "receiver_provenance": "incoming-ecx",
                        "receiver_offset": 0,
                    }
                ],
            },
        ],
        "vbtables": [],
    }

    families = _construction_families(report)

    assert len(families) == 1
    assert [row["table"] for row in families[0]["transitions"]] == [
        "0x00500000",
        "0x00500040",
    ]


def test_shape_fingerprint_ignores_local_addresses_but_keeps_alias_pattern() -> None:
    first = {
        "slots": [
            {"target": "0x1000", "resolution": {"kind": "local-body"}},
            {"target": "0x2000", "resolution": {"kind": "local-body"}},
            {"target": "0x1000", "resolution": {"kind": "local-body"}},
            {
                "target": "0x3000",
                "resolution": {"kind": "pure-virtual", "name": "MSVCRT.dll!_purecall"},
            },
        ]
    }
    second = {
        "slots": [
            {"target": "0x9000", "resolution": {"kind": "local-body"}},
            {"target": "0xa000", "resolution": {"kind": "local-body"}},
            {"target": "0x9000", "resolution": {"kind": "local-body"}},
            {
                "target": "0xb000",
                "resolution": {"kind": "pure-virtual", "name": "MSVCRT.dll!_purecall"},
            },
        ]
    }

    assert table_shape_fingerprint(first) == table_shape_fingerprint(second)


def test_cross_build_compare_uses_shape_fingerprint() -> None:
    table_a = {
        "address": "0x5000",
        "slots": [{"target": "0x1000", "resolution": {"kind": "local-body"}}],
    }
    table_b = {
        "address": "0x9000",
        "slots": [{"target": "0x7000", "resolution": {"kind": "local-body"}}],
    }

    result = compare_table_reports(
        [
            {"binary": "a.exe", "vftables": [table_a]},
            {"binary": "b.exe", "vftables": [table_b]},
        ]
    )

    assert len(result["matches"]) == 1
    assert {row["binary"] for row in result["matches"][0]["tables"]} == {"a.exe", "b.exe"}
