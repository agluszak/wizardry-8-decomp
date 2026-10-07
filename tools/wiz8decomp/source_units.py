"""Recovered source-unit classification and original-path mapping.

Each source file either maps to an evidenced original translation unit or has
no original-path mapping. Absence of a mapping says nothing about its provenance.
Original-TU identity is directory + filename from the retail source path, never
a basename coincidence.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

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

_SOURCE_UNIT_LINE = re.compile(r'^\s*(?:"([^"]+)"|(\S+))\s*$')
_CODE_MARKERS = re.compile(r"^\s*//\s*(?:FUNCTION|VTABLE|GLOBAL):\s+", re.IGNORECASE)


class SourceUnitError(RuntimeError):
    """Source-unit classification or original-path mapping failed."""


def cmake_source_units(repo_dir: Path) -> list[str]:
    """The recovered C++ files listed in ``sources.cmake``, in link order."""

    text = (repo_dir / SOURCES_CMAKE).read_text(encoding="utf-8")
    block = re.search(r"set\(\s*WIZ8_SOURCE_UNITS\s*(.*?)^\s*\)", text, re.MULTILINE | re.DOTALL)
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
    load_source_unit_document(repo_dir)
    violations: list[dict[str, Any]] = []
    for path in cmake_source_units(repo_dir):
        if not file_emits_code_or_data(repo_dir / path):
            violations.append(
                {
                    "kind": "empty-translation-unit",
                    "file": path,
                    "detail": f"{path} contains no FUNCTION/VTABLE/GLOBAL definition marker",
                }
            )
    return violations


def validate_source_units(repo_dir: Path) -> dict[str, Any]:
    violations = source_unit_violations(repo_dir)
    if violations:
        rendered = [f"{item['file']}: {item['kind']}: {item['detail']}" for item in violations]
        raise SourceUnitError("source-unit classification failed:\n  " + "\n  ".join(rendered))
    return {"ok": True, "gate": "source-units"}
