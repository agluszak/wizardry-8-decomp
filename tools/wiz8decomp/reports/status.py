"""Project-wide source and pairing statistics from reccmp's catalog.

Status never runs a comparison. It reports source coverage, which functions
reccmp pairs and why, and the counts of the last `wiz8 compare` report when
that report compared the current build.
"""

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

from ..comparison import last_comparison, warn_if_build_may_be_stale
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


def _pairing_statistics(
    engine: Compare,
    target: RecCmpPartialTarget,
    source_addresses: set[int],
    name_ref_addresses: set[int],
) -> dict[str, Any]:
    """Which recovered source functions reccmp pairs with retail, and why."""

    ignore = set(target.report_config.ignore_functions) if target.report_config else set()
    paired: dict[int, Any] = {}
    ignored = 0
    for address in sorted(source_addresses):
        match = engine.get_match(address)
        if match is None:
            continue
        if match.best_name() in ignore:
            ignored += 1
            continue
        paired[address] = match
    unpaired = source_addresses - paired.keys()
    basis = Counter(
        pair_basis.value if (pair_basis := engine.pair_basis(address)) is not None else "unknown"
        for address in paired
    )
    return {
        "paired": len(paired),
        "unpaired": len(unpaired),
        # Line-reference FUNCTION markers are bound through recomp PDB line
        # records; an unpaired one is exactly the failure reccmp logs as
        # "Failed to find function symbol" while building the catalog.
        # Counting them keeps that visible when the prepared catalog cache
        # serves the engine without re-emitting log records. Name-reference
        # markers (SYMBOL) may legitimately stay unpaired when the recomp
        # emits no standalone copy of a header inline.
        "unpaired_line_refs": len(unpaired - name_ref_addresses),
        "ignored": ignored,
        "pair_basis": dict(sorted(basis.items())),
    }


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
    repository: Path,
) -> dict[str, Any]:
    partial = project.targets[target_id]
    binary = {"binary": partial.filename}
    if partial.recompiled_path is None or partial.recompiled_pdb is None:
        return {**binary, "state": "original-only"}
    if not _comparison_product_available(partial):
        return {**binary, "state": "unbuilt"}

    with _capture_reccmp_diagnostics() as capture:
        engine = Compare.from_target(project.get(target_id))
        source, source_addresses, name_ref_addresses = _source_statistics(engine)
        pairing = _pairing_statistics(engine, partial, source_addresses, name_ref_addresses)
    original_functions = known_functions_by_hash.get(partial.sha256)
    return {
        **binary,
        "state": "comparison",
        "source": source,
        "pairing": pairing,
        "diagnostics": _diagnostics(capture),
        "original_functions": original_functions,
        "source_coverage": (
            source.get("functions", 0) / original_functions if original_functions else None
        ),
        "last_comparison": last_comparison(repository, target_id, Path(partial.recompiled_path)),
    }


def _totals(targets: Mapping[str, dict[str, Any]]) -> dict[str, Any]:
    """Aggregate summed numerators and denominators, never per-target means."""
    comparison = [row for row in targets.values() if row["state"] == "comparison"]
    known = [row for row in comparison if row["original_functions"] is not None]
    known_functions = sum(row["original_functions"] for row in known)
    known_source = sum(row["source"].get("functions", 0) for row in known)
    return {
        "targets": len(targets),
        "comparison_targets": len(comparison),
        "source_functions": sum(row["source"].get("functions", 0) for row in comparison),
        "paired": sum(row["pairing"]["paired"] for row in comparison),
        "unpaired": sum(row["pairing"]["unpaired"] for row in comparison),
        "unpaired_line_refs": sum(row["pairing"]["unpaired_line_refs"] for row in comparison),
        "ignored": sum(row["pairing"]["ignored"] for row in comparison),
        "diagnostic_errors": sum(row["diagnostics"]["error_count"] for row in comparison),
        "known_original_scope": {
            "targets": len(known),
            "original_functions": known_functions,
            "source_functions": known_source,
            "source_coverage": (known_source / known_functions if known_functions else None),
        },
    }


def status_report(settings: Any) -> dict[str, Any]:
    project = RecCmpProject.from_directory(settings.repo_dir / "build" / "decomp")
    known_functions_by_hash = {
        record["binary_sha256"]: record["function_count"] for record in seed_records(settings)
    }
    targets: dict[str, dict[str, Any]] = {}
    for target_id in project.targets:
        partial = project.targets[target_id]
        if _comparison_product_available(partial):
            warn_if_build_may_be_stale(settings.repo_dir, target_id, project.get(target_id))
        targets[target_id] = _target_status(
            project, target_id, known_functions_by_hash, settings.repo_dir
        )
    return {
        "schema": "wiz8.status",
        "totals": _totals(targets),
        "targets": targets,
    }
