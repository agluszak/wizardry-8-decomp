"""Report the first-party symbols the recovered image still cannot resolve."""

from __future__ import annotations

import re
from collections import defaultdict
from pathlib import Path
from typing import Any

from reccmp.formats.coff import parse_coff_object

IMPORT_PREFIXES = ("__imp_", "__IMPORT_DESCRIPTOR", "__NULL_IMPORT_DESCRIPTOR")
MAP_PUBLIC = re.compile(r"^\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(?P<symbol>\S+)\s")


def object_symbols(path: Path) -> tuple[set[str], set[str]]:
    """Return the externals this object defines and the ones it only refers to."""

    defined: set[str] = set()
    referenced: set[str] = set()
    for symbol in parse_coff_object(path).symbols:
        if symbol.storage_class == 2:
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
    """Group every unsatisfied first-party external by the unit that wants it."""

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
