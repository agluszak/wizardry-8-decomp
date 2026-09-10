"""Check recovered source placement against original binary TU ownership.

The source index owns the current physical placement of a recovered function.
The translation-unit layout owns the original translation unit the retail
binary attributed to that address. This gate compares the two and fails only
when the binary evidence is strong enough to place the function: a direct
anchor, a hard hull, or a uniquely projected cross-build anchor. Advisory
``cross-build-similar`` attributions are reported but never enforced.

The CLI surface is dormant until the provisional Video2 fragment is resolved
(its proven functions depend on unmarked static helpers across the unresolved
tail gap); ``validate_source_placement`` is retained for re-enablement.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any

from .ghidra.unit_intervals import (
    TranslationUnitLayout,
    assertion_anchors,
    read_assertions,
    translation_unit_layout_if_available,
)
from .recover import repository_source_file
from .source_index import load_source_index

PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})
_HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")


class PlacementGateError(RuntimeError):
    """A recovered function sits in the wrong original translation unit."""


def _assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    units, headers = assertion_anchors(read_assertions(repo_dir))
    return TranslationUnitLayout(units, header_anchors=headers)


def placement_violations(
    repo_dir: Path, layout: TranslationUnitLayout, markers: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    function_markers = [marker for marker in markers if marker["marker_kind"] == "FUNCTION"]
    expected_by_unit: dict[str, str | None] = {}
    for marker in function_markers:
        source_file = str(marker.get("source_file") or "")
        if source_file.casefold().endswith(_HEADER_SUFFIXES):
            continue
        address = int(marker["address"])
        owner = layout.owner(address)
        if owner["attribution"] not in PLACED_ATTRIBUTIONS:
            continue
        unit = str(owner.get("source_path") or "")
        if not unit:
            continue
        if unit not in expected_by_unit:
            expected_by_unit[unit] = repository_source_file(repo_dir, unit, function_markers)
        expected = expected_by_unit[unit]
        if expected is None:
            continue
        if Path(expected).as_posix() == Path(source_file).as_posix():
            continue
        violations.append(
            {
                "address": f"0x{address:08x}",
                "name": marker.get("marker_name") or marker.get("name") or "",
                "original_unit": unit,
                "attribution": owner["attribution"],
                "evidence": owner.get("evidence", []),
                "current_source": source_file,
                "expected_source": expected,
            }
        )
    return violations


def validate_source_placement(settings: Any) -> dict[str, Any]:
    repo_dir = settings.repo_dir
    markers = load_source_index(repo_dir)["markers"]
    layout = translation_unit_layout_if_available(settings)
    source = "live-ghidra"
    if layout is None:
        layout = _assertion_layout(repo_dir)
        source = "assertions-only"
    violations = placement_violations(repo_dir, layout, markers)
    if violations:
        rendered = [
            f"{item['address']} {item['name']}: original {item['original_unit']} "
            f"({item['attribution']}) but implemented in {item['current_source']}"
            for item in violations
        ]
        raise PlacementGateError(
            "recovered functions are assigned to the wrong original translation unit:\n  "
            + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "translation-unit-placement",
        "evidence": source,
        "functions": sum(1 for marker in markers if marker["marker_kind"] == "FUNCTION"),
        "hulls": len(layout.intervals),
        "cross_build_anchors": sum(
            1 for anchor in layout.unit_anchors if anchor.evidence == "cross-build"
        ),
        "cross_build_advisory": layout.cross_build_advisory,
    }
