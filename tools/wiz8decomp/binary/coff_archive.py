from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

ARCHIVE_MAGIC = b"!<arch>\n"
HEADER_SIZE = 60


@dataclass(frozen=True)
class CoffArchiveMember:
    index: int
    name: str
    data: bytes


def _parse_size(field: bytes, archive: Path) -> int:
    try:
        return int(field.decode("ascii").strip())
    except ValueError as error:
        raise RuntimeError(f"invalid member size in COFF archive {archive}") from error


def _long_name(table: bytes, offset: int, archive: Path) -> str:
    if offset < 0 or offset >= len(table):
        raise RuntimeError(f"invalid long-name offset {offset} in COFF archive {archive}")
    end = table.find(b"\0", offset)
    if end < 0:
        end = table.find(b"\n", offset)
    if end < 0:
        end = len(table)
    return table[offset:end].decode("utf-8", errors="replace").rstrip("/")


def read_coff_archive(path: Path) -> list[CoffArchiveMember]:
    """Read ordinary members from a Microsoft/Unix COFF library archive.

    Linker symbol tables and the long-name table are metadata rather than
    importable objects, so they are deliberately omitted from the result.
    Member order and duplicate names are retained.
    """
    payload = path.read_bytes()
    if not payload.startswith(ARCHIVE_MAGIC):
        raise RuntimeError(f"not a COFF archive: {path}")

    offset = len(ARCHIVE_MAGIC)
    long_names = b""
    members: list[CoffArchiveMember] = []
    while offset < len(payload):
        if offset + HEADER_SIZE > len(payload):
            raise RuntimeError(f"truncated member header in COFF archive {path}")
        header = payload[offset : offset + HEADER_SIZE]
        if header[58:60] != b"`\n":
            raise RuntimeError(f"invalid member header in COFF archive {path} at 0x{offset:x}")
        name_field = header[:16].decode("ascii", errors="replace").strip()
        size = _parse_size(header[48:58], path)
        data_start = offset + HEADER_SIZE
        data_end = data_start + size
        if data_end > len(payload):
            raise RuntimeError(f"truncated member data in COFF archive {path} at 0x{offset:x}")
        data = payload[data_start:data_end]

        if name_field == "//":
            long_names = data
        elif name_field not in {"/", "/SYM64/"}:
            if name_field.startswith("#1/"):
                name_size = int(name_field[3:])
                if name_size > len(data):
                    raise RuntimeError(f"invalid BSD member name in COFF archive {path}")
                name = data[:name_size].decode("utf-8", errors="replace").rstrip("\0")
                data = data[name_size:]
            elif name_field.startswith("/") and name_field[1:].isdigit():
                name = _long_name(long_names, int(name_field[1:]), path)
            else:
                name = name_field.rstrip("/")
            members.append(CoffArchiveMember(index=len(members), name=name, data=data))

        offset = data_end + (size & 1)
    return members


def coff_member_kind(data: bytes) -> str:
    if len(data) >= 20 and data[:2] == b"\x4c\x01":
        return "coff-i386"
    if len(data) >= 20 and data[:4] == b"\x00\x00\xff\xff":
        return "coff-import"
    return "unknown"
