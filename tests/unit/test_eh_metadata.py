from __future__ import annotations

import csv
import struct
from pathlib import Path

from wiz8decomp.binary.image import PeImage
from wiz8decomp.eh_metadata import (
    FUNC_INFO_MAGICS,
    Record,
    _link_owning_functions,
    decode_funclet,
)

_IMAGE_BASE = 0x400000
_TEXT_VA = _IMAGE_BASE + 0x1000
_TEXT_RAW = 0x400
_TEXT_SIZE = 0x400


def _build_image(path: Path, code: bytes) -> PeImage:
    """A one-section PE whose .text holds `code`, for decoder tests."""
    data = bytearray(_TEXT_RAW + _TEXT_SIZE)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", data, 0x84, 0x14C, 1, 0, 0, 0, 0xE0, 0x010F)
    optional = 0x98
    struct.pack_into("<HBB", data, optional, 0x10B, 6, 0)
    struct.pack_into("<I", data, optional + 28, _IMAGE_BASE)
    section = optional + 0xE0
    data[section : section + 8] = b".text\0\0\0"
    struct.pack_into(
        "<IIIIIIHHI",
        data,
        section + 8,
        _TEXT_SIZE,
        _TEXT_VA - _IMAGE_BASE,
        _TEXT_SIZE,
        _TEXT_RAW,
        0,
        0,
        0,
        0,
        0x60000020,
    )
    data[_TEXT_RAW : _TEXT_RAW + len(code)] = code
    path.write_bytes(bytes(data))
    return PeImage(path)


def _relative(source: int, target: int, instruction_length: int) -> bytes:
    return struct.pack("<i", target - (source + instruction_length))


def test_direct_thiscall_funclet_yields_frame_slot_and_destructor(tmp_path: Path) -> None:
    destructor = _TEXT_VA + 0x100
    # lea ecx, [ebp-0x2c] ; jmp <destructor>
    code = b"\x8d\x4d\xd4\xe9" + _relative(_TEXT_VA + 3, destructor, 5)
    image = _build_image(tmp_path / "a.bin", code)

    funclet = decode_funclet(image, _TEXT_VA, {})

    assert funclet.kind == "object"
    assert funclet.frame_offset == -0x2C
    assert funclet.target == destructor


def test_pointer_funclet_is_distinguished_from_a_direct_object(tmp_path: Path) -> None:
    """`mov ecx, [ebp-N]` destroys the object a slot points at, not the slot."""
    destructor = _TEXT_VA + 0x100
    code = b"\x8b\x4d\xf0\xe9" + _relative(_TEXT_VA + 3, destructor, 5)
    image = _build_image(tmp_path / "b.bin", code)

    funclet = decode_funclet(image, _TEXT_VA, {})

    assert funclet.kind == "pointer"
    assert funclet.frame_offset == -0x10


def test_import_thunk_funclet_resolves_the_library_destructor(tmp_path: Path) -> None:
    """A SurRender class is destroyed through the import table, not a call."""
    slot = _IMAGE_BASE + 0x2000
    # mov ecx, [ebp-0x10] ; jmp dword ptr [slot]
    code = b"\x8b\x4d\xf0\xff\x25" + struct.pack("<I", slot)
    image = _build_image(tmp_path / "c.bin", code)

    funclet = decode_funclet(image, _TEXT_VA, {slot: "sr.dll!??1srNode@@MAE@XZ"})

    assert funclet.kind == "pointer-import"
    assert funclet.import_slot == slot
    assert funclet.import_name == "sr.dll!??1srNode@@MAE@XZ"
    assert funclet.target is None


def test_vector_destructor_funclet_records_the_element_destructor(tmp_path: Path) -> None:
    element = _TEXT_VA + 0x180
    helper = _TEXT_VA + 0x100
    # push <element> ; push 2 ; push 8 ; mov eax,[ebp-0x10] ; push eax ; call <helper>
    prefix = b"\x68" + struct.pack("<I", element) + b"\x6a\x02\x6a\x08\x8b\x45\xf0\x50"
    code = prefix + b"\xe8" + _relative(_TEXT_VA + len(prefix), helper, 5)
    image = _build_image(tmp_path / "d.bin", code)

    funclet = decode_funclet(image, _TEXT_VA, {})

    assert funclet.element_destructor == element
    assert funclet.target == helper
    assert funclet.frame_offset == -0x10


def test_a_funclet_branching_outside_code_is_rejected(tmp_path: Path) -> None:
    code = b"\x8d\x4d\xd4\xe9" + _relative(_TEXT_VA + 3, _IMAGE_BASE + 0x900000, 5)
    image = _build_image(tmp_path / "e.bin", code)

    funclet = decode_funclet(image, _TEXT_VA, {})

    assert funclet.kind == "bad-target"
    assert funclet.target is None


def test_eh_setup_is_not_promoted_to_a_function_start(tmp_path: Path) -> None:
    """VC6 may load FS before the exact `push -1; push <thunk>` setup."""
    record_address = _TEXT_VA + 0x300
    handler = _TEXT_VA + 0x200
    function_start = _TEXT_VA + 0x20
    code = bytearray(b"\xcc" * _TEXT_SIZE)
    # mov eax,fs:[0] ; push -1 ; push <handler>
    code[0x20:0x28] = b"\x64\xa1\x00\x00\x00\x00\x6a\xff"
    code[0x28:0x2D] = b"\x68" + struct.pack("<I", handler)
    # mov eax,<FuncInfo> is the one-to-one handler thunk anchor.
    code[0x200:0x205] = b"\xb8" + struct.pack("<I", record_address)
    image = _build_image(tmp_path / "eh-setup.bin", bytes(code))
    record = Record(0x19930520, record_address, 0, 0, 0, 0, 0, 0)

    _link_owning_functions(image, [record])

    assert record.handler_thunk == handler
    assert record.frame_setup == function_start + 8
    assert record.eh_setup_start == function_start + 6


def _snapshot(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/eh-metadata" / name).open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_function_snapshot_is_keyed_by_program_and_record_address() -> None:
    rows = _snapshot("functions.csv")

    keys = [(row["program"], row["funcinfo"]) for row in rows]
    assert len(keys) == len(set(keys))
    assert all(row["magic"] in {f"{magic:08x}" for magic in FUNC_INFO_MAGICS} for row in rows)


def test_every_resolved_record_reaches_exactly_one_eh_setup() -> None:
    """The record -> thunk -> frame-setup chain is one-to-one, so setups do not collide."""
    rows = [row for row in _snapshot("functions.csv") if row["eh_setup_start"]]

    keys = [(row["program"], row["eh_setup_start"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_unwind_rows_reference_a_declared_record() -> None:
    functions = {(row["program"], row["funcinfo"]) for row in _snapshot("functions.csv")}
    unwind = _snapshot("unwind.csv")

    assert unwind
    assert all((row["program"], row["funcinfo"]) in functions for row in unwind)


def test_readable_builds_resolve_every_cleanup_to_a_destructor() -> None:
    """A blank target and a blank import together mean the decoder gave up."""
    rows = [row for row in _snapshot("unwind.csv") if row["kind"] != "protected-code"]

    unresolved = [
        row
        for row in rows
        if row["kind"] != "none" and not row["target"] and not row["import_name"]
    ]
    assert not unresolved, unresolved[:5]


def test_catch_rows_keep_the_only_surviving_type_descriptors() -> None:
    rows = _snapshot("catch.csv")

    typed = [row for row in rows if row["type_name"]]
    assert typed, "the demo build retains at least one catch of a class type"
    assert all(row["type_name"].startswith(".?A") for row in typed)
