"""On-demand address-outlier audit for proved original translation units.

Ordinary non-COMDAT bodies from one original TU cluster in address space, but
VC6 COMDAT folding and compiler emissions legitimately break contiguity. A hard
``function must lie inside TU min/max`` gate is therefore wrong. This report
flags large address outliers among FUNCTION markers owned by recovered
original-TU sources and annotates whether each outlier address also carries
TEMPLATE or SYNTHETIC emission evidence so agents can review suspicious
ownership manually.

The report is informational only: presence in the listing is a review signal,
not automatic proof a symbol is mis-owned. Regression coverage for emission
annotation lives in unit tests with synthetic markers, not a hard-coded
production fixture address.
"""

from __future__ import annotations

import statistics
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..paths import atomic_json
from ..source_index import load_source_index
from ..source_units import ORIGINAL_TU, source_unit_records

DEFAULT_MIN_GAP = 0x100000
DEFAULT_MIN_PEERS = 3
_HEADER_SUFFIXES = (".h", ".hpp", ".hxx", ".inl")


def _is_header(path: str) -> bool:
    return path.casefold().endswith(_HEADER_SUFFIXES)


def _evidence_by_address(markers: list[dict[str, Any]]) -> dict[int, dict[str, Any]]:
    """Collect TEMPLATE/SYNTHETIC emission claims keyed by retail address."""

    by_address: dict[int, dict[str, Any]] = defaultdict(
        lambda: {
            "template": False,
            "synthetic": False,
            "marker_kinds": [],
        }
    )
    for marker in markers:
        address = int(marker["address"])
        kind = str(marker.get("marker_kind") or "")
        entry = by_address[address]
        if kind and kind not in entry["marker_kinds"]:
            entry["marker_kinds"].append(kind)
        if kind == "TEMPLATE":
            entry["template"] = True
        if kind == "SYNTHETIC":
            entry["synthetic"] = True
    return dict(by_address)


def placement_outliers(
    markers: list[dict[str, Any]],
    original_tu_files: set[str],
    *,
    min_gap: int = DEFAULT_MIN_GAP,
    min_peers: int = DEFAULT_MIN_PEERS,
) -> list[dict[str, Any]]:
    """FUNCTION markers whose addresses sit far from their owning TU's cluster."""

    evidence = _evidence_by_address(markers)
    by_source: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for marker in markers:
        if marker.get("marker_kind") != "FUNCTION":
            continue
        source = str(marker.get("source_file") or "")
        if not source or _is_header(source):
            continue
        if source not in original_tu_files:
            continue
        by_source[source].append(marker)

    outliers: list[dict[str, Any]] = []
    for source, functions in sorted(by_source.items()):
        if len(functions) < min_peers:
            continue
        addresses = [int(marker["address"]) for marker in functions]
        center = int(statistics.median(addresses))
        lower = min(addresses)
        upper = max(addresses)
        for marker in functions:
            address = int(marker["address"])
            gap = abs(address - center)
            if gap < min_gap:
                continue
            claims = evidence.get(
                address,
                {
                    "template": False,
                    "synthetic": False,
                    "marker_kinds": ["FUNCTION"],
                },
            )
            outliers.append(
                {
                    "address": f"0x{address:08x}",
                    "name": marker.get("marker_name")
                    or (marker.get("declaration_key") or [None, None])[-1]
                    or "",
                    "source_file": source,
                    "cluster_median": f"0x{center:08x}",
                    "cluster_lower": f"0x{lower:08x}",
                    "cluster_upper": f"0x{upper:08x}",
                    "gap_bytes": gap,
                    "template": bool(claims.get("template")),
                    "synthetic": bool(claims.get("synthetic")),
                    "marker_kinds": list(claims.get("marker_kinds") or ["FUNCTION"]),
                    "has_emission_evidence": bool(claims.get("template"))
                    or bool(claims.get("synthetic")),
                }
            )
    outliers.sort(key=lambda row: (-int(row["gap_bytes"]), row["address"]))
    return outliers


def placement_outlier_report(
    repo_dir: Path,
    *,
    min_gap: int = DEFAULT_MIN_GAP,
    min_peers: int = DEFAULT_MIN_PEERS,
) -> dict[str, Any]:
    """Write the outlier listing under ``build/reports/`` and return a summary."""

    records = source_unit_records(repo_dir)
    original_tu_files = {
        path for path, record in records.items() if record.get("class") == ORIGINAL_TU
    }
    index = load_source_index(repo_dir)
    outliers = placement_outliers(
        index["markers"],
        original_tu_files,
        min_gap=min_gap,
        min_peers=min_peers,
    )
    artifact_dir = repo_dir / "build" / "reports" / "placement-outliers"
    artifact_dir.mkdir(parents=True, exist_ok=True)
    artifact = artifact_dir / "outliers.json"
    payload = {
        "schema": "wiz8.placement-outliers-v1",
        "informational": True,
        "policy": (
            "Large address gaps in an original TU are review signals. TEMPLATE "
            "and SYNTHETIC evidence often explain legitimate "
            "non-contiguity; absence of that evidence warrants manual ownership review."
        ),
        "min_gap_bytes": min_gap,
        "min_peers": min_peers,
        "outlier_count": len(outliers),
        "outliers": outliers,
    }
    atomic_json(artifact, payload)
    return {
        "schema": "wiz8.placement-outliers-v1",
        "informational": True,
        "outlier_count": len(outliers),
        "artifact": str(artifact.relative_to(repo_dir)),
        "outliers": outliers,
    }
