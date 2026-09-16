from __future__ import annotations

from wiz8decomp.binary.code import disassembler
from wiz8decomp.msvc_table_guard import _entry_anchored


class _Image:
    def __init__(self, base: int, data: bytes) -> None:
        self.base = base
        self.data = data

    def read(self, address: int, size: int) -> bytes:
        offset = address - self.base
        if offset < 0:
            return b""
        return self.data[offset : offset + size]

    def offset(self, address: int) -> int | None:
        offset = address - self.base
        return offset if 0 <= offset < len(self.data) else None

    def virtual_address(self, offset: int) -> int | None:
        return self.base + offset if 0 <= offset < len(self.data) else None


def test_entry_anchored_requires_independent_function_boundary() -> None:
    # There is no preceding VC6 padding/prologue boundary, so a backwards decode
    # must not be treated as if ECX at its first instruction were entry ECX.
    image = _Image(0x1000, bytes.fromhex("8b f1 83 c6 04 c7 06 00 30 00 00"))

    anchored, start = _entry_anchored(image, disassembler(), 0x1005)

    assert not anchored
    assert start is None


def test_entry_anchored_accepts_continuous_decode_from_vc6_boundary() -> None:
    # int3 padding followed by `push esi`, one of the repository's recognised
    # VC6 entry shapes, then a continuous stream up to the table store.
    image = _Image(0x1000, bytes.fromhex("cc cc 56 8b f1 c7 06 00 30 00 00"))

    anchored, start = _entry_anchored(image, disassembler(), 0x1005)

    assert anchored
    assert start == 0x1002
