"""Check recovered source placement against original binary TU ownership.

The source index owns the current physical placement of a recovered function.
The translation-unit layout owns the original translation unit the retail
binary attributed to that address. This gate compares the two and fails only
when the binary evidence is strong enough to place the function: a direct
anchor, a hard hull, or a uniquely projected cross-build anchor. Advisory
``cross-build-similar`` attributions are reported but never enforced.

The gate runs against the reviewed assertion anchors by default, so it stays
fast and Ghidra-independent; ``live=True`` uses the richer live cross-build
layout when a checkout has the project open.
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
from .source_index import load_source_index
from .source_units import (
    CLASSIFICATION_PATH,
    ORIGINAL_TU,
    SourceUnitError,
    classification_for,
    load_source_unit_document,
    mapped_repository_source_file,
    original_source_paths,
    source_unit_records,
)

PLACED_ATTRIBUTIONS = frozenset({"direct", "bounded", "cross-build"})
ADVISORY_ATTRIBUTIONS = frozenset({"cross-build-similar"})
_HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")


class PlacementGateError(RuntimeError):
    """A recovered function sits in the wrong original translation unit."""


def _expected_recovered_source(repo_dir: Path, unit: str) -> str | None:
    """Map an original path onto a recovered original-tu file.

    A matching basename in an unresolved-fragment or compiler-emission file
    does not prove original-TU identity.
    """

    mapped = mapped_repository_source_file(repo_dir, unit)
    if mapped is None:
        return None
    if not (repo_dir / CLASSIFICATION_PATH).is_file():
        return mapped
    try:
        document = load_source_unit_document(repo_dir)
        originals = original_source_paths(repo_dir)
    except SourceUnitError:
        return mapped
    if classification_for(mapped, document, originals) != ORIGINAL_TU:
        return None
    return mapped


def _assertion_layout(repo_dir: Path) -> TranslationUnitLayout:
    units, headers = assertion_anchors(read_assertions(repo_dir))
    return TranslationUnitLayout(units, header_anchors=headers)


def placement_violations(
    repo_dir: Path, layout: TranslationUnitLayout, markers: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    function_markers = [marker for marker in markers if marker["marker_kind"] == "FUNCTION"]
    expected_by_unit: dict[str, str | None] = {}
    classes: dict[str, str] = {}
    if (repo_dir / CLASSIFICATION_PATH).is_file():
        try:
            classes = {
                path: record["class"] for path, record in source_unit_records(repo_dir).items()
            }
        except SourceUnitError:
            classes = {}
    for marker in function_markers:
        source_file = str(marker.get("source_file") or "")
        if source_file.casefold().endswith(_HEADER_SUFFIXES):
            continue
        address = int(marker["address"])
        owner = layout.owner(address)
        attribution = str(owner.get("attribution") or "")
        if attribution in ADVISORY_ATTRIBUTIONS:
            continue
        if attribution not in PLACED_ATTRIBUTIONS:
            continue
        unit = str(owner.get("source_path") or "")
        if not unit:
            violations.append(
                {
                    "kind": "unknown-original-unit",
                    "address": f"0x{address:08x}",
                    "name": marker.get("marker_name") or marker.get("name") or "",
                    "original_unit": "",
                    "attribution": attribution,
                    "evidence": owner.get("evidence", []),
                    "current_source": source_file,
                    "expected_source": "",
                    "detail": (
                        f"0x{address:08x} has {attribution} attribution but no original "
                        "translation-unit path"
                    ),
                }
            )
            continue
        if unit not in expected_by_unit:
            expected_by_unit[unit] = _expected_recovered_source(repo_dir, unit)
        expected = expected_by_unit[unit]
        current_class = classes.get(source_file, "")
        if expected is None:
            violations.append(
                {
                    "kind": "unresolved-placement",
                    "address": f"0x{address:08x}",
                    "name": marker.get("marker_name") or marker.get("name") or "",
                    "original_unit": unit,
                    "attribution": attribution,
                    "evidence": owner.get("evidence", []),
                    "current_source": source_file,
                    "current_class": current_class,
                    "expected_source": "",
                    "detail": (
                        f"0x{address:08x} {marker.get('marker_name') or ''}: original {unit} "
                        f"({attribution}) has no recovered physical source file"
                    ).strip(),
                }
            )
            continue
        if Path(expected).as_posix() == Path(source_file).as_posix():
            continue
        violations.append(
            {
                "kind": "wrong-translation-unit",
                "address": f"0x{address:08x}",
                "name": marker.get("marker_name") or marker.get("name") or "",
                "original_unit": unit,
                "attribution": attribution,
                "evidence": owner.get("evidence", []),
                "current_source": source_file,
                "current_class": current_class,
                "expected_source": expected,
                "detail": (
                    f"0x{address:08x} {marker.get('marker_name') or ''}: original {unit} "
                    f"({attribution}) but implemented in {source_file}"
                ).strip(),
            }
        )
    return violations


def validate_source_placement(settings: Any, *, live: bool = False) -> dict[str, Any]:
    repo_dir = settings.repo_dir
    markers = load_source_index(repo_dir)["markers"]
    layout = translation_unit_layout_if_available(settings) if live else None
    source = "live-ghidra"
    if layout is None:
        layout = _assertion_layout(repo_dir)
        source = "assertions-only"
    violations = placement_violations(repo_dir, layout, markers)
    if violations:
        rendered = [
            item.get("detail")
            or (
                f"{item['address']} {item['name']}: original {item['original_unit']} "
                f"({item['attribution']}) but implemented in {item['current_source']}"
            )
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
