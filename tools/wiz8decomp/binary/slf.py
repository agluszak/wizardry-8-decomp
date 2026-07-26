from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

SLF_HEADER_SIZE = 0x214
SLF_DIRECTORY_ENTRY_SIZE = 0x118

_HEADER = struct.Struct("<256s256siiHHB3xi")
_DIRECTORY_ENTRY = struct.Struct("<256sIIBB2xQH2x")


def _cstring(raw: bytes) -> str:
    return raw.split(b"\0", 1)[0].decode("cp1252")


@dataclass(frozen=True, slots=True)
class SlfHeader:
    archive_name: str
    base_path: str
    file_count: int
    used_count: int
    sort_order: int
    version: int
    contains_subdirectories: bool
    reserved: int


@dataclass(frozen=True, slots=True)
class SlfDirectoryEntry:
    path: str
    data_offset: int
    data_size: int
    status: int
    reserved_byte: int
    file_time: int
    reserved_word: int

    @property
    def is_active(self) -> bool:
        """Match the executable's low-byte filter at 0x00412BB0."""

        return self.status & 0xFF == 0


@dataclass(frozen=True, slots=True)
class SlfArchive:
    path: Path
    header: SlfHeader
    directory_offset: int
    entries: tuple[SlfDirectoryEntry, ...]


def read_slf(path: Path) -> SlfArchive:
    """Read the header and EOF directory without extracting copyrighted payloads."""

    file_size = path.stat().st_size
    if file_size < SLF_HEADER_SIZE:
        raise ValueError(f"SLF is shorter than its 0x214-byte header: {path}")

    with path.open("rb") as stream:
        raw_header = stream.read(SLF_HEADER_SIZE)
        unpacked_header = _HEADER.unpack(raw_header)
        header = SlfHeader(
            archive_name=_cstring(unpacked_header[0]),
            base_path=_cstring(unpacked_header[1]),
            file_count=unpacked_header[2],
            used_count=unpacked_header[3],
            sort_order=unpacked_header[4],
            version=unpacked_header[5],
            contains_subdirectories=bool(unpacked_header[6]),
            reserved=unpacked_header[7],
        )

        directory_size = header.file_count * SLF_DIRECTORY_ENTRY_SIZE
        directory_offset = file_size - directory_size
        if directory_offset < SLF_HEADER_SIZE:
            raise ValueError(
                f"SLF directory overlaps its header: {header.file_count} entries in {file_size} bytes"
            )

        stream.seek(directory_offset)
        entries: list[SlfDirectoryEntry] = []
        for index in range(header.file_count):
            raw_entry = stream.read(SLF_DIRECTORY_ENTRY_SIZE)
            if len(raw_entry) != SLF_DIRECTORY_ENTRY_SIZE:
                raise ValueError(f"truncated SLF directory entry {index}: {path}")
            unpacked_entry = _DIRECTORY_ENTRY.unpack(raw_entry)
            entry = SlfDirectoryEntry(
                path=_cstring(unpacked_entry[0]),
                data_offset=unpacked_entry[1],
                data_size=unpacked_entry[2],
                status=unpacked_entry[3],
                reserved_byte=unpacked_entry[4],
                file_time=unpacked_entry[5],
                reserved_word=unpacked_entry[6],
            )
            if entry.data_offset + entry.data_size > directory_offset:
                raise ValueError(f"SLF entry {entry.path!r} extends into the directory: {path}")
            entries.append(entry)

    return SlfArchive(path, header, directory_offset, tuple(entries))
