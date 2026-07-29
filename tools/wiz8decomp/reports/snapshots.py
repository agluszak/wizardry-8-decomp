"""Shared serialization and publication for derived evidence snapshots."""

from __future__ import annotations

import csv
import io
from collections.abc import Iterable, Mapping, Sequence
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_write


def csv_text(fields: Sequence[str], rows: Iterable[Mapping[str, Any]]) -> str:
    """Serialize deterministic CSV text using the repository's line endings."""
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def publish_report_snapshot(
    settings: Settings,
    *,
    name: str,
    outputs: Mapping[str, str],
    snapshot_files: Sequence[str],
    snapshot_readme: str,
    update_snapshot: bool,
    stale_error: str,
) -> tuple[Path, Path, bool]:
    """Publish derived reports and enforce their reviewed snapshot atomically."""
    report_dir = settings.build_dir / "reports" / name
    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / name
    for filename, value in outputs.items():
        atomic_write(report_dir / filename, value)
    if update_snapshot:
        for filename, value in outputs.items():
            atomic_write(snapshot_dir / filename, value)
        atomic_write(snapshot_dir / "README.md", snapshot_readme)
    snapshot_fresh = all(
        (snapshot_dir / filename).is_file()
        and (snapshot_dir / filename).read_text(encoding="utf-8") == outputs[filename]
        for filename in snapshot_files
    )
    if not update_snapshot and not snapshot_fresh:
        raise RuntimeError(stale_error)
    return report_dir, snapshot_dir, snapshot_fresh
