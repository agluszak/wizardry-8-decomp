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
import struct
from collections import defaultdict
from pathlib import Path
from typing import Any

from reccmp.compare.exact import coff_name

from .paths import atomic_json, atomic_write

# The linker prefixes an imported symbol's thunk this way. It is never a name a
# recovered unit writes, so matching on it cannot hide a real gap.
IMPORT_PREFIXES = ("__imp_", "__IMPORT_DESCRIPTOR", "__NULL_IMPORT_DESCRIPTOR")
MAP_PUBLIC = re.compile(r"^\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(?P<symbol>\S+)\s")
BASELINE_COLUMNS = ("symbol",)
DEFAULT_BASELINE = Path("config/verification/unresolved-baseline.csv")


def object_symbols(path: Path) -> tuple[set[str], set[str]]:
    """Return the externals this object defines and the ones it only refers to."""

    data = path.read_bytes()
    if len(data) < 20:
        return set(), set()
    machine, _section_count, _stamp, symbol_table, symbol_count, optional_size, _flags = (
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
        name = coff_name(data, raw, string_table)
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


def compare_unresolved_reports(
    current: dict[str, Any], baseline: dict[str, Any], *, baseline_name: str
) -> dict[str, Any]:
    """Compare the current first-party missing-symbol frontier to reviewed debt."""

    current_symbols = set(current["by_symbol"])
    baseline_symbols = {str(row["symbol"]) for row in baseline["symbols"]}
    introduced = sorted(current_symbols - baseline_symbols)
    resolved = sorted(baseline_symbols - current_symbols)
    unchanged = sorted(current_symbols & baseline_symbols)
    return {
        "schema": "wiz8.unresolved-delta",
        "ok": not introduced,
        "baseline": baseline_name,
        "baseline_symbol_count": len(baseline_symbols),
        "current_symbol_count": len(current_symbols),
        "introduced_count": len(introduced),
        "resolved_count": len(resolved),
        "unchanged_count": len(unchanged),
        "introduced": introduced,
        "resolved": resolved,
        "unchanged": unchanged,
        "ranked_units": current["ranked_units"],
        "near_link_complete_units": current["near_link_complete_units"],
        "canonical_import_symbols": current["canonical_import_symbols"],
        "canonical_imports_by_symbol": current["canonical_imports_by_symbol"],
    }


def load_unresolved_baseline(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValueError(
            f"unresolved-symbol baseline does not exist: {path}; "
            "run wiz8 analyze unresolved --write-baseline once on reviewed main"
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


def verify_unresolved_delta(
    settings: Any, current: dict[str, Any], against: Path | None = None
) -> dict[str, Any]:
    baseline_path = against or (settings.repo_dir / DEFAULT_BASELINE)
    if not baseline_path.is_absolute():
        baseline_path = settings.repo_dir / baseline_path
    delta = compare_unresolved_reports(
        current,
        load_unresolved_baseline(baseline_path),
        baseline_name=str(baseline_path),
    )
    destination = settings.build_dir / "reports/unresolved/delta.json"
    atomic_json(destination, delta)
    delta["report"] = str(destination)
    return delta


def require_unresolved_delta(report: dict[str, Any]) -> dict[str, Any]:
    if not report["ok"]:
        raise ValueError(
            f"link verification introduced {report['introduced_count']} first-party "
            f"unresolved symbols against {report['baseline']}; see {report['report']}"
        )
    return report
