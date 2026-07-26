from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    raw_offset: int
    raw_size: int
    executable: bool

    @property
    def virtual_end(self) -> int:
        return self.virtual_address + max(self.virtual_size, self.raw_size)

    @property
    def mapped_end(self) -> int:
        """End of the range that is backed by file bytes."""
        return self.virtual_address + min(max(self.virtual_size, self.raw_size), self.raw_size)


class PeImage:
    """A minimal read-only PE view keyed by virtual address.

    ``pefile`` already parses directories, but every analysis here walks raw
    bytes at arbitrary virtual addresses and needs the inverse mapping too, so a
    direct section table is both simpler and considerably faster than repeatedly
    asking pefile to translate.
    """

    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        data = self.data
        if data[:2] != b"MZ" or len(data) < 0x40:
            raise ValueError(f"not a PE image: {path}")
        pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe_offset : pe_offset + 4] != b"PE\0\0":
            raise ValueError(f"not a PE image: {path}")
        coff = pe_offset + 4
        section_count, optional_size = struct.unpack_from("<H", data, coff + 2)[0], struct.unpack_from("<H", data, coff + 16)[0]
        optional = coff + 20
        self.image_base = struct.unpack_from("<I", data, optional + 28)[0]
        self.machine = struct.unpack_from("<H", data, coff)[0]
        self.characteristics = struct.unpack_from("<H", data, coff + 18)[0]
        self.symbol_table_pointer, self.symbol_count = struct.unpack_from("<II", data, coff + 8)
        self.linker_version = f"{data[optional + 2]}.{data[optional + 3]}"
        sections: list[Section] = []
        table = optional + optional_size
        for index in range(section_count):
            entry = table + index * 40
            name = data[entry : entry + 8].rstrip(b"\0").decode("latin-1")
            virtual_size, virtual_address, raw_size, raw_offset = struct.unpack_from("<IIII", data, entry + 8)
            flags = struct.unpack_from("<I", data, entry + 36)[0]
            sections.append(
                Section(
                    name=name,
                    virtual_address=self.image_base + virtual_address,
                    virtual_size=virtual_size,
                    raw_offset=raw_offset,
                    raw_size=raw_size,
                    executable=bool(flags & 0x20000000),
                )
            )
        self.sections = sections

    @property
    def text(self) -> Section:
        for section in self.sections:
            if section.name == ".text":
                return section
        for section in self.sections:
            if section.executable:
                return section
        raise ValueError(f"no executable section: {self.path}")

    def section_at(self, virtual_address: int) -> Section | None:
        for section in self.sections:
            if section.virtual_address <= virtual_address < section.mapped_end:
                return section
        return None

    def offset(self, virtual_address: int) -> int | None:
        section = self.section_at(virtual_address)
        if section is None:
            return None
        return section.raw_offset + (virtual_address - section.virtual_address)

    def virtual_address(self, offset: int) -> int | None:
        for section in self.sections:
            if section.raw_offset <= offset < section.raw_offset + section.raw_size:
                return section.virtual_address + (offset - section.raw_offset)
        return None

    def read(self, virtual_address: int, size: int) -> bytes:
        offset = self.offset(virtual_address)
        if offset is None:
            return b""
        return self.data[offset : offset + size]

    def read_u32(self, virtual_address: int) -> int | None:
        raw = self.read(virtual_address, 4)
        if len(raw) != 4:
            return None
        return struct.unpack("<I", raw)[0]

    def read_i32(self, virtual_address: int) -> int | None:
        raw = self.read(virtual_address, 4)
        if len(raw) != 4:
            return None
        return struct.unpack("<i", raw)[0]

    def read_cstring(self, virtual_address: int, limit: int = 512) -> str | None:
        raw = self.read(virtual_address, limit)
        if not raw:
            return None
        end = raw.find(b"\0")
        if end < 0:
            return None
        return raw[:end].decode("latin-1")

    def is_code(self, virtual_address: int) -> bool:
        section = self.section_at(virtual_address)
        return section is not None and section.executable

    def find_all(self, needle: bytes) -> list[int]:
        """Virtual addresses of every mapped occurrence of ``needle``."""
        results: list[int] = []
        offset = self.data.find(needle)
        while offset != -1:
            virtual_address = self.virtual_address(offset)
            if virtual_address is not None:
                results.append(virtual_address)
            offset = self.data.find(needle, offset + 1)
        return results
