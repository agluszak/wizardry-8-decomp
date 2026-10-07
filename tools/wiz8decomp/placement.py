"""Check recovered source placement against original binary TU ownership."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .source_index import load_source_index
from .source_units import mapped_repository_source_file, original_source_paths
from .unit_intervals import TranslationUnitLayout, assertion_layout

_HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")


class PlacementGateError(RuntimeError):
    """A recovered function sits in the wrong original translation unit."""


def placement_violations(
    repo_dir: Path, layout: TranslationUnitLayout, markers: list[dict[str, Any]]
) -> list[str]:
    """Anchored FUNCTION markers implemented outside their original unit's recovered file.

    A matching basename alone does not prove original-TU identity: the expected
    file must be an evidenced mapping of the original path.
    """

    originals = original_source_paths(repo_dir)
    expected_by_unit: dict[str, str | None] = {}
    violations: list[str] = []
    for marker in markers:
        source_file = str(marker.get("source_file") or "")
        if marker["marker_kind"] != "FUNCTION" or source_file.casefold().endswith(_HEADER_SUFFIXES):
            continue
        address = int(marker["address"])
        owner = layout.owner(address)
        attribution = owner["attribution"]
        unit = owner["source_path"]
        if attribution not in {"direct", "bounded"}:
            continue
        if unit not in expected_by_unit:
            mapped = mapped_repository_source_file(repo_dir, unit)
            expected_by_unit[unit] = mapped if mapped in originals else None
        expected = expected_by_unit[unit]
        name = marker.get("marker_name") or marker.get("name") or ""
        if expected is None:
            violations.append(
                f"0x{address:08x} {name}: original {unit} ({attribution}) "
                "has no recovered physical source file"
            )
        elif Path(expected).as_posix() != Path(source_file).as_posix():
            violations.append(
                f"0x{address:08x} {name}: original {unit} ({attribution}) "
                f"but implemented in {source_file}"
            )
    return violations


def validate_source_placement(repo_dir: Path) -> dict[str, Any]:
    layout = assertion_layout(repo_dir)
    violations = placement_violations(repo_dir, layout, load_source_index(repo_dir)["markers"])
    if violations:
        raise PlacementGateError(
            "recovered functions are assigned to the wrong original translation unit:\n  "
            + "\n  ".join(violations)
        )
    return {"ok": True, "gate": "translation-unit-placement", "hulls": len(layout.intervals)}
