"""External symbols of a COFF object file."""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

_EXTERNAL = 2
_SYMBOL_SIZE = 18


@dataclass(frozen=True)
class CoffSymbol:
    name: str
    value: int
    section: int
    storage_class: int

    @property
    def is_undefined(self) -> bool:
        return self.section == 0 and self.value == 0

    @property
    def is_common(self) -> bool:
        """An uninitialized external the linker allocates (``value`` is its size)."""
        return self.section == 0 and self.value > 0


def coff_symbols(path: Path) -> list[CoffSymbol]:
    """Primary symbol records of an object file, auxiliary records skipped."""

    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError(f"{path}: too short for a COFF header")
    _machine, _sections, _stamp, table, count = struct.unpack_from("<HHLLL", data, 0)
    strings_at = table + count * _SYMBOL_SIZE
    if strings_at > len(data):
        raise ValueError(f"{path}: symbol table runs past the end of the file")
    symbols: list[CoffSymbol] = []
    index = 0
    while index < count:
        offset = table + index * _SYMBOL_SIZE
        raw_name = data[offset : offset + 8]
        value, section, _type, storage, aux = struct.unpack_from("<LhHBB", data, offset + 8)
        if raw_name[:4] == b"\0\0\0\0":
            (name_at,) = struct.unpack_from("<L", raw_name, 4)
            start = strings_at + name_at
            name = data[start : data.index(b"\0", start)].decode("latin-1")
        else:
            name = raw_name.rstrip(b"\0").decode("latin-1")
        symbols.append(CoffSymbol(name, value, section, storage))
        index += 1 + aux
    return symbols


def external_symbols(path: Path) -> tuple[set[str], set[str]]:
    """The externals an object defines and the ones it only refers to."""

    defined: set[str] = set()
    referenced: set[str] = set()
    for symbol in coff_symbols(path):
        if symbol.storage_class != _EXTERNAL:
            continue
        if symbol.is_undefined:
            referenced.add(symbol.name)
        elif symbol.section > 0 or symbol.is_common:
            defined.add(symbol.name)
    return defined, referenced
