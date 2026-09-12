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

import csv
import io
import re
from collections import defaultdict
from pathlib import Path
from typing import Any

from reccmp.formats.coff import parse_coff_object

from .paths import atomic_write

# The linker prefixes an imported symbol's thunk this way. It is never a name a
# recovered unit writes, so matching on it cannot hide a real gap.
IMPORT_PREFIXES = ("__imp_", "__IMPORT_DESCRIPTOR", "__NULL_IMPORT_DESCRIPTOR")
MAP_PUBLIC = re.compile(r"^\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(?P<symbol>\S+)\s")
BASELINE_COLUMNS = ("symbol",)
DEFAULT_BASELINE = Path("config/verification/unresolved-baseline.csv")


def object_symbols(path: Path) -> tuple[set[str], set[str]]:
    """Return the externals this object defines and the ones it only refers to."""

    defined: set[str] = set()
    referenced: set[str] = set()
    for symbol in parse_coff_object(path).symbols:
        if symbol.storage_class == 2:
            # Section zero with a zero value is the COFF spelling of "wanted but
            # not supplied here"; a nonzero value is a common block, which the
            # linker allocates rather than reports.
            if symbol.section == 0 and symbol.value == 0:
                referenced.add(symbol.name)
            elif symbol.section > 0 or symbol.is_common:
                defined.add(symbol.name)
    return defined, referenced


def parse_map_publics(path: Path) -> set[str]:
    """Every symbol the linked image ended up defining."""

    publics: set[str] = set()
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        match = MAP_PUBLIC.match(line)
        if match is not None:
            publics.add(match.group("symbol"))
    return publics


def unresolved_report(
    object_root: Path, map_path: Path | None = None, objects: list[Path] | None = None
) -> dict[str, Any]:
    """Group every unsatisfied first-party external by the unit that wants it.

    ``objects`` restricts the scan to an explicit object list, which callers use
    when the object directory still holds files a previous source layout left
    behind and only the linked objects are authoritative.
    """

    if not object_root.is_dir():
        raise RuntimeError(f"no built objects to report on: {object_root}")
    if objects is None:
        candidates = [
            obj
            for obj in sorted(object_root.rglob("*.obj"))
            if any(part.endswith(".dir") for part in obj.parts)
        ]
    else:
        candidates = [path for path in objects if path.is_file()]
    defined: set[str] = set()
    wanted: dict[str, set[str]] = {}
    for obj in candidates:
        provides, refers = object_symbols(obj)
        defined |= provides
        if refers:
            wanted[obj.relative_to(object_root).as_posix()] = refers
    if map_path is not None and map_path.is_file():
        defined |= parse_map_publics(map_path)

    by_unit: dict[str, list[str]] = {}
    by_symbol: dict[str, list[str]] = defaultdict(list)
    imports_by_unit: dict[str, list[str]] = {}
    imports_by_symbol: dict[str, list[str]] = defaultdict(list)
    for unit, refers in wanted.items():
        imports = sorted(name for name in refers if name.startswith(IMPORT_PREFIXES))
        if imports:
            imports_by_unit[unit] = imports
            for name in imports:
                imports_by_symbol[name].append(unit)
        missing = sorted(
            name for name in refers if name not in defined and not name.startswith(IMPORT_PREFIXES)
        )
        if missing:
            by_unit[unit] = missing
            for name in missing:
                by_symbol[name].append(unit)
    ranked_units = [
        {"unit": unit, "unresolved_count": len(symbols), "symbols": symbols}
        for unit, symbols in sorted(by_unit.items(), key=lambda item: (-len(item[1]), item[0]))
    ]
    return {
        "objects": len(wanted),
        "unresolved_symbols": len(by_symbol),
        "units_with_unresolved": len(by_unit),
        "by_unit": {item["unit"]: item["symbols"] for item in ranked_units},
        "by_symbol": {name: sorted(units) for name, units in sorted(by_symbol.items())},
        "ranked_units": ranked_units,
        "near_link_complete_units": [
            item for item in ranked_units if item["unresolved_count"] <= 2
        ],
        "canonical_import_symbols": len(imports_by_symbol),
        "canonical_imports_by_unit": dict(sorted(imports_by_unit.items())),
        "canonical_imports_by_symbol": {
            name: sorted(units) for name, units in sorted(imports_by_symbol.items())
        },
    }


def load_unresolved_baseline(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValueError(
            f"unresolved-symbol baseline does not exist: {path}; "
            "pass a path to write with wiz8 analyze unresolved --write-baseline"
        )
    with path.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != BASELINE_COLUMNS:
            raise ValueError(
                f"unresolved-symbol baseline has unexpected columns: {reader.fieldnames}"
            )
        rows = list(reader)
    return {
        "schema": "wiz8.unresolved-baseline",
        "symbol_count": len(rows),
        "symbols": rows,
    }


def write_unresolved_baseline(path: Path, report: dict[str, Any]) -> dict[str, Any]:
    """Initialize the unresolved frontier or ratchet it strictly downward."""

    rows = [{"symbol": symbol} for symbol in sorted(report["by_symbol"])]
    if path.is_file():
        previous = load_unresolved_baseline(path)
        previous_symbols = {str(row["symbol"]) for row in previous["symbols"]}
        additions = [row["symbol"] for row in rows if row["symbol"] not in previous_symbols]
        if additions:
            raise ValueError(f"refusing to add {len(additions)} symbols to the unresolved baseline")
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=BASELINE_COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)  # pyright: ignore[reportArgumentType]
    atomic_write(path, output.getvalue())
    return {"baseline": str(path), "symbol_count": len(rows)}
