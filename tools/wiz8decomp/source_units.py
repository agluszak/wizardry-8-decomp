"""Recovered source-unit classification and original-path mapping.

A normal ``.cpp`` is an original translation unit, an unresolved fragment, or
compiler-emitted material. Classification lives next to ``sources.cmake``.
Original-TU identity is directory + filename from the retail source path, never
a basename coincidence.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

ORIGINAL_TU = "original-tu"
UNRESOLVED_FRAGMENT = "unresolved-fragment"
COMPILER_EMISSION = "compiler-emission"
CLASSIFICATIONS = frozenset({ORIGINAL_TU, UNRESOLVED_FRAGMENT, COMPILER_EMISSION})
CLASSIFICATION_PATH = Path("src/wiz8/source_units.json")
SOURCE_TREE_PATH = Path("evidence/observations/wiz8/source-tree.csv")
SOURCES_CMAKE = Path("src/wiz8/sources.cmake")

UNIT_DIRECTORIES = {
    "3d code": "3d_code",
    "dialog code": "dialog_code",
    "engine code": "engine_code",
    "level specific code": "level_specific_code",
    "local code": "local_code",
    "local screens": "local_screens",
}
ORIGINAL_DIRECTORIES = {value: key for key, value in UNIT_DIRECTORIES.items()}

# Filenames that are semantic catch-alls unless explicitly classified otherwise.
CATCH_ALL_NAMES = frozenset(
    {
        "state_getters.cpp",
        "registry_classes.cpp",
        "gameplay_teardown.cpp",
        "bringup_gates.cpp",
        "unattributed_helpers.cpp",
        "message_box.cpp",
        "monster_info_dialog.cpp",
    }
)

_SOURCE_UNIT_LINE = re.compile(r'^\s*(?:"([^"]+)"|(\S+))\s*$')
_CODE_MARKERS = re.compile(
    r"^\s*//\s*(?:FUNCTION|TEMPLATE|VTABLE|GLOBAL|LIBRARY):\s+", re.IGNORECASE
)


class SourceUnitError(RuntimeError):
    """Source-unit classification or original-path mapping failed."""


def cmake_source_units(repo_dir: Path) -> list[str]:
    """The recovered C++ files listed in ``sources.cmake``, in link order."""

    text = (repo_dir / SOURCES_CMAKE).read_text(encoding="utf-8")
    block = re.search(
        r"set\(\s*WIZ8_SOURCE_UNITS\s*(.*?)^\s*\)", text, re.MULTILINE | re.DOTALL
    )
    if block is None:
        raise SourceUnitError(f"{SOURCES_CMAKE} has no WIZ8_SOURCE_UNITS list")
    units: list[str] = []
    for line in block.group(1).splitlines():
        stripped = line.split("#", 1)[0].strip()
        match = _SOURCE_UNIT_LINE.match(stripped)
        if match:
            units.append(match.group(1) or match.group(2))
    return units


def load_source_unit_document(repo_dir: Path) -> dict[str, Any]:
    path = repo_dir / CLASSIFICATION_PATH
    if not path.is_file():
        raise SourceUnitError(f"{CLASSIFICATION_PATH} is missing")
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "wiz8.source-units-v1":
        raise SourceUnitError(f"{CLASSIFICATION_PATH} has an unsupported schema")
    return document


def original_source_paths(repo_dir: Path) -> dict[str, str]:
    """Map recovered repository paths onto evidence-backed original unit paths."""

    import csv

    path = repo_dir / SOURCE_TREE_PATH
    originals: dict[str, str] = {}
    if not path.is_file():
        return originals
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            relative = (row.get("relative_path") or "").replace("\\", "/")
            if not relative:
                continue
            recovered = mapped_repository_source_file(repo_dir, relative.replace("/", "\\"))
            if recovered:
                originals[recovered] = relative.replace("/", "\\")
    return originals


def mapped_repository_source_file(repo_dir: Path, unit: str) -> str | None:
    """Map one original unit onto its recovered file by directory + filename.

    Basename-only matches do not prove original-TU identity. The mapped
    directory must exist; a same-named file in another directory is not a hit.
    ``original-path-map`` records recovered files whose filesystem name is a
    documented spelling of the original path, not a basename coincidence.
    """

    normalized = unit.replace("/", "\\")
    path = repo_dir / CLASSIFICATION_PATH
    if path.is_file():
        try:
            document = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            document = {}
        aliases = {
            str(key).replace("/", "\\"): str(value)
            for key, value in (document.get("original-path-map") or {}).items()
        }
        mapped = aliases.get(normalized)
        if mapped and (repo_dir / mapped).is_file():
            return Path(mapped).as_posix()
    parts = [part for part in normalized.replace("\\", "/").split("/") if part]
    if not parts:
        return None
    name = parts[-1]
    directory = "/".join(parts[:-1])
    if directory:
        mapped_dir = UNIT_DIRECTORIES.get(directory.casefold())
        if mapped_dir is None:
            return None
        parent = repo_dir / "src/wiz8" / mapped_dir
    else:
        parent = repo_dir / "src/wiz8"
    if not parent.is_dir():
        return None
    matches = [
        path
        for path in parent.iterdir()
        if path.is_file() and path.name.casefold() == name.casefold()
    ]
    if len(matches) != 1:
        return None
    return matches[0].relative_to(repo_dir).as_posix()


def classification_for(path: str, document: dict[str, Any], originals: dict[str, str]) -> str:
    compiler = {str(item) for item in document.get("compiler-emission") or ()}
    unresolved = {str(item) for item in document.get("unresolved-fragment") or ()}
    if path in compiler:
        return COMPILER_EMISSION
    if path in unresolved:
        return UNRESOLVED_FRAGMENT
    if path in originals:
        return ORIGINAL_TU
    return UNRESOLVED_FRAGMENT


def source_unit_records(repo_dir: Path) -> dict[str, dict[str, str]]:
    document = load_source_unit_document(repo_dir)
    originals = original_source_paths(repo_dir)
    records: dict[str, dict[str, str]] = {}
    for path in cmake_source_units(repo_dir):
        kind = classification_for(path, document, originals)
        record = {"class": kind, "path": path}
        if kind == ORIGINAL_TU:
            record["original_path"] = originals[path]
        records[path] = record
    return records


def file_emits_code_or_data(path: Path) -> bool:
    """Whether a translation unit contains a recovered definition or emission marker."""

    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    for line in text.splitlines():
        if _CODE_MARKERS.match(line):
            return True
    return False


def source_unit_violations(repo_dir: Path) -> list[dict[str, Any]]:
    document = load_source_unit_document(repo_dir)
    originals = original_source_paths(repo_dir)
    listed = cmake_source_units(repo_dir)
    compiler = {str(item) for item in document.get("compiler-emission") or ()}
    unresolved = {str(item) for item in document.get("unresolved-fragment") or ()}
    classified = compiler | unresolved
    violations: list[dict[str, Any]] = []

    unknown_classes = classified - set(listed)
    for path in sorted(unknown_classes):
        violations.append(
            {
                "kind": "unknown-classified-unit",
                "file": path,
                "detail": f"{path} is classified but missing from {SOURCES_CMAKE}",
            }
        )

    overlap = compiler & unresolved
    for path in sorted(overlap):
        violations.append(
            {
                "kind": "duplicate-classification",
                "file": path,
                "detail": f"{path} is both compiler-emission and unresolved-fragment",
            }
        )

    for path in listed:
        kind = classification_for(path, document, originals)
        name = Path(path).name.casefold()
        catch_all = name in {item.casefold() for item in CATCH_ALL_NAMES}
        if catch_all and path not in unresolved and path not in compiler:
            violations.append(
                {
                    "kind": "catch-all-unclassified",
                    "file": path,
                    "detail": (
                        f"{path} is a semantic catch-all and must be listed as "
                        "unresolved-fragment or compiler-emission"
                    ),
                }
            )
        if kind == ORIGINAL_TU and catch_all:
            violations.append(
                {
                    "kind": "catch-all-original-tu",
                    "file": path,
                    "detail": (
                        f"{path} is a semantic catch-all and cannot be original-tu "
                        "without an evidence-backed original path"
                    ),
                }
            )
        if kind == ORIGINAL_TU and path not in originals:
            violations.append(
                {
                    "kind": "original-tu-without-path",
                    "file": path,
                    "detail": f"{path} is original-tu but has no evidence-backed original path",
                }
            )
        if kind != COMPILER_EMISSION and not file_emits_code_or_data(repo_dir / path):
            violations.append(
                {
                    "kind": "empty-translation-unit",
                    "file": path,
                    "detail": (
                        f"{path} contains no FUNCTION/TEMPLATE/VTABLE/GLOBAL/LIBRARY "
                        "marker that emits code or data"
                    ),
                }
            )
    return violations


def validate_source_units(repo_dir: Path) -> dict[str, Any]:
    violations = source_unit_violations(repo_dir)
    if violations:
        rendered = [f"{item['file']}: {item['kind']}: {item['detail']}" for item in violations]
        raise SourceUnitError(
            "source-unit classification failed:\n  " + "\n  ".join(rendered)
        )
    records = source_unit_records(repo_dir)
    counts = {
        ORIGINAL_TU: 0,
        UNRESOLVED_FRAGMENT: 0,
        COMPILER_EMISSION: 0,
    }
    for record in records.values():
        counts[record["class"]] += 1
    return {"ok": True, "gate": "source-units", "units": len(records), **counts}
