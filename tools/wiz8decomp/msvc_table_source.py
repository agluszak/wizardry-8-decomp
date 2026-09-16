"""Overlay canonical source-index identities on a binary MSVC table report.

This is intentionally optional.  Structural recovery must work on a pristine PE
without a source index, and a custom/other-version binary must not inherit
address-based identities from the canonical Wiz8 image.  The CLI therefore calls
this only for its default canonical Wiz8 scan.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any


def annotate_source_identities(report: dict[str, Any], repository: Path) -> dict[str, Any]:
    """Attach known WIZ8 function names to local slots and construction functions.

    A missing/stale-unavailable source index is not an analysis failure: the
    binary report remains usable and records why the optional overlay was skipped.
    """

    from reccmp.source import SourceIndexError

    from .source_index import source_functions

    try:
        functions = source_functions(repository, "WIZ8")
    except SourceIndexError as exc:
        report.setdefault("analysis", {})["source_identity_overlay"] = {
            "status": "unavailable",
            "reason": str(exc),
        }
        return report

    names = {address: marker.name for address, marker in functions.items() if marker.name}
    slot_matches = 0
    write_function_matches = 0
    family_function_matches = 0

    for table in report.get("vftables", []):
        for slot in table.get("slots", []):
            target = int(slot["target"], 16)
            name = names.get(target)
            if name is None:
                continue
            resolution = slot.setdefault("resolution", {})
            resolution["source_name"] = name
            if resolution.get("kind") == "local-body":
                resolution["kind"] = "source-function"
            slot_matches += 1

    for kind in ("vftables", "vbtables"):
        for table in report.get(kind, []):
            for write in table.get("writes", []):
                raw = write.get("function")
                if raw is None:
                    continue
                name = names.get(int(raw, 16))
                if name is not None:
                    write["function_source_name"] = name
                    write_function_matches += 1

    analysis = report.setdefault("analysis", {})
    for family in analysis.get("construction_families", []):
        raw = family.get("function")
        if raw is None:
            continue
        name = names.get(int(raw, 16))
        if name is not None:
            family["function_source_name"] = name
            family_function_matches += 1

    # Recount the slot categories because local-body rows may now be promoted to
    # source-function. Keep this derived from the actual row data rather than
    # trying to adjust the earlier counters arithmetically.
    counts: dict[str, int] = {}
    for table in report.get("vftables", []):
        for slot in table.get("slots", []):
            kind = str(slot.get("resolution", {}).get("kind", "unresolved"))
            counts[kind] = counts.get(kind, 0) + 1
    analysis["slot_resolution"] = dict(sorted(counts.items()))
    analysis["source_identity_overlay"] = {
        "status": "available",
        "source_functions": len(functions),
        "slot_matches": slot_matches,
        "write_function_matches": write_function_matches,
        "construction_family_function_matches": family_function_matches,
    }
    return report
