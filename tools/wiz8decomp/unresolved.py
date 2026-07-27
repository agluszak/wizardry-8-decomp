"""Report the first-party symbols the recovered image still cannot resolve.

`/FORCE:UNRESOLVED` is what lets the bring-up image link while recovery is
incomplete, and it is doing a real job: without it there is no inspectable PE at
all. The cost is that the gap stops being visible. The linker names each missing
symbol once, in build output nobody keeps, and the MAP does not carry them --
it lists what was defined, not what was wanted.

So the gap is computed instead: every external a matching object refers to but
no object defines. Grouping that by the referring translation unit turns it into
a work list, because a unit with one missing callee is a different proposition
from one with thirty.

Imports are excluded. A symbol satisfied by an import library is resolved, not
missing, and the decorated `__imp_` spellings only exist because the linker
rewrote a call it had already resolved.
"""

from __future__ import annotations

import re
import struct
from collections import defaultdict
from pathlib import Path
from typing import Any

from .sgp_oracle import _coff_name

# The linker prefixes an imported symbol's thunk this way. It is never a name a
# recovered unit writes, so matching on it cannot hide a real gap.
IMPORT_PREFIXES = ("__imp_", "__IMPORT_DESCRIPTOR", "__NULL_IMPORT_DESCRIPTOR")
MAP_PUBLIC = re.compile(r"^\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(?P<symbol>\S+)\s")


def object_symbols(path: Path) -> tuple[set[str], set[str]]:
    """Return the externals this object defines and the ones it only refers to."""

    data = path.read_bytes()
    if len(data) < 20:
        return set(), set()
    machine, section_count, _stamp, symbol_table, symbol_count, optional_size, _flags = (
        struct.unpack_from("<HHIIIHH", data, 0)
    )
    if machine != 0x14C or optional_size != 0:
        return set(), set()
    string_table = symbol_table + symbol_count * 18
    defined: set[str] = set()
    referenced: set[str] = set()
    index = 0
    while index < symbol_count:
        raw = data[symbol_table + index * 18 : symbol_table + index * 18 + 18]
        name = _coff_name(data, raw, string_table)
        value, section, _type, storage, auxiliary = struct.unpack_from("<IhHBB", raw, 8)
        if storage == 2:
            # Section zero with a zero value is the COFF spelling of "wanted but
            # not supplied here"; a nonzero value is a common block, which the
            # linker allocates rather than reports.
            if section == 0 and value == 0:
                referenced.add(name)
            elif section > 0:
                defined.add(name)
        index += 1 + auxiliary
    return defined, referenced


def parse_map_publics(path: Path) -> set[str]:
    """Every symbol the linked image ended up defining."""

    publics: set[str] = set()
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        match = MAP_PUBLIC.match(line)
        if match is not None:
            publics.add(match.group("symbol"))
    return publics


def unresolved_report(object_root: Path, map_path: Path | None = None) -> dict[str, Any]:
    """Group every unsatisfied first-party external by the unit that wants it."""

    if not object_root.is_dir():
        raise RuntimeError(f"no built objects to report on: {object_root}")
    defined: set[str] = set()
    wanted: dict[str, set[str]] = {}
    for obj in sorted(object_root.rglob("*.obj")):
        if not any(part.endswith(".dir") for part in obj.parts):
            continue
        provides, refers = object_symbols(obj)
        defined |= provides
        if refers:
            wanted[obj.name] = refers
    if map_path is not None and map_path.is_file():
        defined |= parse_map_publics(map_path)

    by_unit: dict[str, list[str]] = {}
    by_symbol: dict[str, list[str]] = defaultdict(list)
    for unit, refers in wanted.items():
        missing = sorted(
            name
            for name in refers
            if name not in defined and not name.startswith(IMPORT_PREFIXES)
        )
        if missing:
            by_unit[unit] = missing
            for name in missing:
                by_symbol[name].append(unit)
    return {
        "objects": len(wanted),
        "unresolved_symbols": len(by_symbol),
        "units_with_unresolved": len(by_unit),
        "by_unit": dict(sorted(by_unit.items(), key=lambda item: (-len(item[1]), item[0]))),
        "by_symbol": {name: sorted(units) for name, units in sorted(by_symbol.items())},
    }
