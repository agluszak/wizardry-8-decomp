"""Project-wide source and matching statistics from reccmp's project model."""

from __future__ import annotations

import logging
from collections import Counter
from collections.abc import Iterator, Mapping
from contextlib import contextmanager
from pathlib import Path
from typing import Any

from reccmp.compare import Compare
from reccmp.parser.marker import MarkerType
from reccmp.project.detect import RecCmpPartialTarget, RecCmpProject

from ..comparison import warn_if_build_may_be_stale
from ..ghidra.workspace import seed_records

# Only FUNCTION markers are recovered authored source. The remaining kinds
# have their own accounting row and are never counted as progress.
_SOURCE_KINDS = (
    (MarkerType.FUNCTION, "functions"),
    (MarkerType.STUB, "stubs"),
    (MarkerType.LIBRARY, "library"),
    (MarkerType.SYNTHETIC, "synthetic"),
    (MarkerType.TEMPLATE, "template"),
)
_DIAGNOSTIC_LIMIT = 50


class _ReccmpDiagnostics(logging.Handler):
    """Collect reccmp WARNING+ records emitted while an engine is built/queried."""

    def __init__(self) -> None:
        super().__init__(level=logging.WARNING)
        self.records: list[logging.LogRecord] = []

    def emit(self, record: logging.LogRecord) -> None:
        self.records.append(record)


@contextmanager
def _capture_reccmp_diagnostics() -> Iterator[_ReccmpDiagnostics]:
    capture = _ReccmpDiagnostics()
    reccmp_logger = logging.getLogger("reccmp")
    reccmp_logger.addHandler(capture)
    try:
        yield capture
    finally:
        reccmp_logger.removeHandler(capture)


def _source_statistics(engine: Compare) -> tuple[dict[str, int], set[int], set[int]]:
    """Count function-like source markers by kind; return FUNCTION addresses and
    the subset bound by name (SYMBOL/name-reference markers) rather than line."""

    codebase = engine.codebase
    markers = (
        [*codebase.iter_line_functions(), *codebase.iter_name_functions()]
        if codebase is not None
        else []
    )
    source: dict[str, int] = {}
    for marker_type, key in _SOURCE_KINDS:
        count = len({marker.offset for marker in markers if marker.type == marker_type})
        if count:
            source[key] = count
    source_addresses = {marker.offset for marker in markers if marker.type == MarkerType.FUNCTION}
    name_ref_addresses = {
        marker.offset
        for marker in markers
        if marker.type == MarkerType.FUNCTION and marker.is_nameref()
    }
    return source, source_addresses, name_ref_addresses


def _comparison_statistics(
    engine: Compare,
    target: RecCmpPartialTarget,
    source_addresses: set[int],
    name_ref_addresses: set[int],
) -> tuple[dict[str, Any], float]:
    """Classify every recovered source function and total its effective score."""

    ignore = set(target.report_config.ignore_functions) if target.report_config else set()
    entities = {
        entity.orig_addr: entity
        for entity in engine.compare_addresses(
            orig_addrs=sorted(source_addresses),
            include_diff=False,
            include_exact_diff=False,
        )
    }
    ignored = {address for address, entity in entities.items() if entity.name in ignore}
    counts: Counter[str] = Counter()
    effective_score = 0.0
    for address, entity in entities.items():
        if address in ignored:
            continue
        counts[entity.analysis.status.value] += 1
        effective_score += entity.effective_accuracy
    paired = sum(counts.values())
    unpaired = source_addresses - entities.keys()
    return (
        {
            "paired": paired,
            "exact": counts["exact"],
            "effective": counts["effective"],
            "mismatch": counts["mismatch"],
            "inconclusive": counts["inconclusive"],
            "unpaired": len(unpaired),
            # Line-reference FUNCTION markers are bound through recomp PDB line
            # records; an unpaired one is exactly the failure reccmp logs as
            # "Failed to find function symbol" at engine-build time. Counting
            # them here keeps that diagnostic visible when the prepared
            # analysis cache serves the engine without re-emitting log records.
            # Name-reference markers (SYMBOL) may legitimately stay unpaired
            # when the recomp emits no standalone copy of a header inline.
            "unpaired_line_refs": len(unpaired - name_ref_addresses),
            "ignored": len(ignored),
            "accuracy": effective_score / paired if paired else 0.0,
        },
        effective_score,
    )


def _diagnostics(capture: _ReccmpDiagnostics) -> dict[str, Any]:
    def render(record: logging.LogRecord) -> str:
        return f"{record.name}: {record.getMessage()}"

    errors = [render(r) for r in capture.records if r.levelno >= logging.ERROR]
    warnings = [render(r) for r in capture.records if r.levelno < logging.ERROR]
    return {
        "error_count": len(errors),
        "warning_count": len(warnings),
        "errors": errors[:_DIAGNOSTIC_LIMIT],
        "warnings": warnings[:_DIAGNOSTIC_LIMIT],
        "truncated": (
            max(0, len(errors) - _DIAGNOSTIC_LIMIT) + max(0, len(warnings) - _DIAGNOSTIC_LIMIT)
        ),
    }


def _comparison_product_available(target: RecCmpPartialTarget) -> bool:
    """Return whether the configured recomp executable and PDB both exist."""

    paths = (target.recompiled_path, target.recompiled_pdb)
    return all(path is not None and Path(path).is_file() for path in paths)


def _target_status(
    project: RecCmpProject,
    target_id: str,
    known_functions_by_hash: Mapping[str, int],
) -> tuple[dict[str, Any], float]:
    partial = project.targets[target_id]
    binary = {"binary": partial.filename}
    if partial.recompiled_path is None or partial.recompiled_pdb is None:
        return {**binary, "state": "original-only"}, 0.0
    if not _comparison_product_available(partial):
        return {**binary, "state": "unbuilt"}, 0.0

    with _capture_reccmp_diagnostics() as capture:
        engine = Compare.from_target(project.get(target_id))
        source, source_addresses, name_ref_addresses = _source_statistics(engine)
        comparison, effective_score = _comparison_statistics(
            engine, partial, source_addresses, name_ref_addresses
        )
    original_functions = known_functions_by_hash.get(partial.sha256)
    row = {
        **binary,
        "state": "comparison",
        "source": source,
        "comparison": comparison,
        "diagnostics": _diagnostics(capture),
        "original_functions": original_functions,
        "source_coverage": (
            source.get("functions", 0) / original_functions if original_functions else None
        ),
        "progress": (effective_score / original_functions if original_functions else None),
    }
    return row, effective_score


def _totals(targets: Mapping[str, dict[str, Any]], scores: Mapping[str, float]) -> dict[str, Any]:
    """Aggregate summed numerators and denominators, never per-target means."""

    comparison = [target_id for target_id, row in targets.items() if row["state"] == "comparison"]
    known = [
        target_id
        for target_id in comparison
        if targets[target_id]["original_functions"] is not None
    ]
    paired = sum(targets[target_id]["comparison"]["paired"] for target_id in comparison)
    effective_score = sum(scores[target_id] for target_id in comparison)
    known_functions = sum(targets[target_id]["original_functions"] for target_id in known)
    known_source = sum(targets[target_id]["source"].get("functions", 0) for target_id in known)
    known_score = sum(scores[target_id] for target_id in known)
    return {
        "targets": len(targets),
        "comparison_targets": len(comparison),
        "source_functions": sum(
            targets[target_id]["source"].get("functions", 0) for target_id in comparison
        ),
        "paired": paired,
        "exact": sum(targets[target_id]["comparison"]["exact"] for target_id in comparison),
        "effective": sum(targets[target_id]["comparison"]["effective"] for target_id in comparison),
        "mismatch": sum(targets[target_id]["comparison"]["mismatch"] for target_id in comparison),
        "inconclusive": sum(
            targets[target_id]["comparison"]["inconclusive"] for target_id in comparison
        ),
        "unpaired": sum(targets[target_id]["comparison"]["unpaired"] for target_id in comparison),
        "unpaired_line_refs": sum(
            targets[target_id]["comparison"]["unpaired_line_refs"] for target_id in comparison
        ),
        "ignored": sum(targets[target_id]["comparison"]["ignored"] for target_id in comparison),
        "diagnostic_errors": sum(
            targets[target_id]["diagnostics"]["error_count"] for target_id in comparison
        ),
        "accuracy": effective_score / paired if paired else 0.0,
        "known_original_scope": {
            "targets": len(known),
            "original_functions": known_functions,
            "source_functions": known_source,
            "source_coverage": (known_source / known_functions if known_functions else None),
            "progress": known_score / known_functions if known_functions else None,
        },
    }


def status_report(settings: Any) -> dict[str, Any]:
    project = RecCmpProject.from_directory(settings.repo_dir / "build" / "decomp")
    known_functions_by_hash = {
        record["binary_sha256"]: record["function_count"] for record in seed_records(settings)
    }
    targets: dict[str, dict[str, Any]] = {}
    scores: dict[str, float] = {}
    for target_id in project.targets:
        partial = project.targets[target_id]
        if _comparison_product_available(partial):
            warn_if_build_may_be_stale(settings.repo_dir, target_id, project.get(target_id))
        row, score = _target_status(project, target_id, known_functions_by_hash)
        targets[target_id] = row
        scores[target_id] = score
    return {
        "schema": "wiz8.status",
        "totals": _totals(targets, scores),
        "targets": targets,
    }
