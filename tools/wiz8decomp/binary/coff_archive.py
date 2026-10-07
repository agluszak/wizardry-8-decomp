from __future__ import annotations

import struct
from pathlib import Path

ARCHIVE_MAGIC = b"!<arch>\n"


def named_iat_archive(dll: str, caller: str, provider: str) -> bytes:
    """VC6 long-form import data with independently named IAT and PE import.

    There is deliberately no code/thunk section. LIB combines this member with
    its ordinary DEF-generated descriptor and terminators. The member must have
    the same DLL name so LINK groups its tables before that DLL's terminators.
    """
    strings = bytearray(b"\0" * 4)
    symbols = bytearray()

    def symbol(name: str, section: int, storage: int = 3, aux: bytes = b"") -> None:
        encoded = name.encode("ascii")
        if len(encoded) > 8:
            value = struct.pack("<LL", 0, len(strings))
            strings.extend(encoded + b"\0")
        else:
            value = encoded.ljust(8, b"\0")
        symbols.extend(value + struct.pack("<LhHBB", 0, section, 0, storage, bool(aux)))
        symbols.extend(aux)

    hint = b"\0\0" + provider.encode("ascii") + b"\0"
    hint += b"\0" * (len(hint) % 2)
    sections = (
        (".idata$5", b"\0" * 4, struct.pack("<LLH", 0, 4, 7), 0xC0301040),
        (".idata$4", b"\0" * 4, struct.pack("<LLH", 0, 4, 7), 0xC0301040),
        (".idata$6", hint, b"", 0xC0201040),
    )
    raw = bytearray()
    headers = bytearray()
    start = 20 + len(sections) * 40
    for section, (name, data, relocations, flags) in enumerate(sections, 1):
        # NODUPLICATES IAT; the lookup table and hint/name are associative.
        symbol(
            name,
            section,
            aux=struct.pack(
                "<LHHLhB3x",
                len(data),
                len(relocations) // 10,
                0,
                0,
                0 if section == 1 else 1,
                1 if section == 1 else 5,
            ),
        )
        offset = start + len(raw)
        headers.extend(
            struct.pack(
                "<8sLLLLLLHHL",
                name.encode("ascii"),
                0,
                0,
                len(data),
                offset,
                offset + len(data) if relocations else 0,
                0,
                len(relocations) // 10,
                0,
                flags,
            )
        )
        raw.extend(data + relocations)
    symbol("__imp_" + caller, 1, 2)
    symbol("__IMPORT_DESCRIPTOR_" + Path(dll).stem, 0, 2)
    struct.pack_into("<L", strings, 0, len(strings))
    obj = (
        struct.pack("<HHLLLHH", 0x14C, 3, 0, start + len(raw), len(symbols) // 18, 0, 0x100)
        + headers
        + raw
        + symbols
        + strings
    )
    header = f"{dll + '/':<16}{0:<12}{0:<6}{0:<6}{100666:<8}{len(obj):<10}`\n".encode("ascii")
    return ARCHIVE_MAGIC + header + obj + (b"\n" if len(obj) % 2 else b"")
