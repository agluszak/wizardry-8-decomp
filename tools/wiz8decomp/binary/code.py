"""Shared x86 code-reading helpers for the static evidence producers.

Everything here answers a question the section table cannot: which instruction
covers an address, where the enclosing function began, and which import a jump
thunk stands for.
"""

from __future__ import annotations

import struct
from typing import Any

from .image import PeImage

# How far back a backward decode may search for an alignment.
DEFAULT_WINDOW = 96
# How far back to look for the padding that precedes a function's first byte.
_FUNCTION_SCAN = 0x3000
_PADDING = {0xCC, 0x90}
_PROLOGUES = (
    b"\x55\x8b\xec",
    b"\x6a\xff",
    b"\x83\xec",
    b"\x81\xec",
    b"\x53",
    b"\x56",
    b"\x57",
    b"\x8b\xff",
)


def disassembler() -> Any:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    engine = Cs(CS_ARCH_X86, CS_MODE_32)
    engine.detail = True
    return engine


def decode_chain_ending_at(
    image: PeImage, engine: Any, address: int, window: int = DEFAULT_WINDOW
) -> list[Any]:
    """Instructions in a short window that decode to end exactly on ``address``.

    x86 cannot be disassembled backwards, so every start offset in the window is
    tried and the longest chain that lands precisely on the address is kept.
    """
    best: list[Any] = []
    for back in range(4, window):
        start = address - back
        raw = image.read(start, back)
        if len(raw) != back:
            continue
        chain = list(engine.disasm(raw, start))
        if chain and chain[-1].address + chain[-1].size == address:
            best = chain
    return best


def instruction_covering(image: PeImage, engine: Any, operand: int, span: int = 4) -> Any | None:
    """The instruction whose encoding contains the bytes at ``operand``.

    Used to ask what an individual relocated operand is part of.
    """
    for back in range(1, 12):
        raw = image.read(operand - back, back + span + 8)
        if len(raw) < back + span:
            continue
        chain = list(engine.disasm(raw, operand - back))
        if not chain:
            continue
        head = chain[0]
        if head.address <= operand and head.address + head.size >= operand + span:
            return head
    return None


def sweep_text(image: PeImage, engine: Any) -> list[Any]:
    """Every instruction in `.text`, decoded by a resynchronising linear sweep.

    A plain sweep stops at the first jump table or alignment blob embedded in the
    code and then reports a few thousand instructions for a two-megabyte section.
    Restarting one byte past each failure recovers the stream, which is what
    makes a single pass cheaper than decoding backwards from tens of thousands of
    individual operands.
    """
    section = image.text
    data = image.read(section.virtual_address, section.raw_size)
    instructions: list[Any] = []
    cursor = 0
    limit = len(data)
    while cursor < limit:
        produced = 0
        for instruction in engine.disasm(data[cursor:], section.virtual_address + cursor):
            instructions.append(instruction)
            produced += instruction.size
        cursor += produced + 1 if produced else 1
    return instructions


def covering_index(instructions: list[Any]) -> tuple[list[int], list[Any]]:
    """A bisectable index from any address to the instruction covering it."""
    ordered = sorted(instructions, key=lambda item: item.address)
    return [item.address for item in ordered], ordered


def lookup_covering(
    starts: list[int], ordered: list[Any], address: int, span: int = 4
) -> Any | None:
    import bisect

    index = bisect.bisect_right(starts, address) - 1
    if index < 0:
        return None
    instruction = ordered[index]
    if instruction.address <= address and instruction.address + instruction.size >= address + span:
        return instruction
    return None


def function_start(image: PeImage, address: int) -> int | None:
    """The entry point of the function containing ``address``.

    VC6 pads between functions with int3 or nop, so the first byte after the
    nearest preceding padding run is the entry point. The candidate is only
    accepted when it actually starts with a prologue, which keeps a jump table
    or an unpadded tail from inventing a boundary.
    """
    offset = image.offset(address)
    if offset is None:
        return None
    data = image.data
    limit = max(0, offset - _FUNCTION_SCAN)
    cursor = offset
    while cursor > limit:
        cursor -= 1
        if data[cursor] not in _PADDING:
            continue
        run_end = cursor + 1
        while cursor > limit and data[cursor - 1] in _PADDING:
            cursor -= 1
        candidate = image.virtual_address(run_end)
        if candidate is None:
            return None
        if any(image.read(candidate, 3).startswith(prologue) for prologue in _PROLOGUES):
            return candidate
        # A lone 0x90 inside an instruction stream is not padding; keep going.
    return None


def import_thunks(image: PeImage, slots: dict[int, str]) -> dict[int, str]:
    """Map each `jmp dword ptr [slot]` thunk to the import it stands for.

    A vtable slot inherited from a library class points at one of these rather
    than at any body in this image, so resolving them turns such a slot into a
    named method.
    """
    text = image.text
    data = image.data
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    thunks: dict[int, str] = {}
    offset = data.find(b"\xff\x25", low, high)
    while offset != -1:
        slot = struct.unpack_from("<I", data, offset + 2)[0]
        name = slots.get(slot)
        if name:
            address = image.virtual_address(offset)
            if address is not None:
                thunks[address] = name
        offset = data.find(b"\xff\x25", offset + 1, high)
    return thunks


def relocation_sites(image: PeImage) -> list[int]:
    """Every address holding an absolute operand the loader fixes up.

    This is the image's own complete index of pointer-shaped values, which is
    what makes a vtable scan exhaustive rather than pattern-matched.
    """
    data = image.data
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    optional = pe_offset + 4 + 20
    directory_count = struct.unpack_from("<I", data, optional + 92)[0]
    if directory_count <= 5:
        return []
    rva, size = struct.unpack_from("<II", data, optional + 96 + 5 * 8)
    if not rva or not size:
        return []
    offset = image.offset(image.image_base + rva)
    if offset is None:
        return []
    end = offset + size
    sites: list[int] = []
    while offset < end - 8:
        page, block = struct.unpack_from("<II", data, offset)
        if block < 8:
            break
        for index in range((block - 8) // 2):
            entry = struct.unpack_from("<H", data, offset + 8 + index * 2)[0]
            # Type 3 is HIGHLOW: a full 32-bit absolute address.
            if entry >> 12 == 3:
                sites.append(image.image_base + page + (entry & 0xFFF))
        offset += block
    sites.sort()
    return sites
