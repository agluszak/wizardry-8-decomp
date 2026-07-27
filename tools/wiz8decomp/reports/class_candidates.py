"""Generate candidate class records from the polymorphism snapshots.

The polymorphism census records every constructor-written vtable and every
vptr write site, but only a handful of classes are reviewed. This report
turns that gap into triage: one candidate row per constructor-written
vftable, carrying the writer functions with their ctor/dtor classification,
co-installed subobject vtables, slot-count and pure-virtual facts, and the
allocation-size hints the census scanned at the writers' call sites.

Candidates are a generated projection under ``build/reports/`` and are never
auto-promoted. The promotion path is: review a candidate against decompiles
of its writers, then record the accepted identity as rows in
``evidence/reviewed/wiz8/classes.csv`` / ``vtables.csv`` / ``vtable-slots.csv``
(and the writers in ``functions.csv``), citing the writer sites this report
points at. The generator reads snapshots and reviewed evidence; it writes
neither, and it needs no proprietary inputs - the classification core is
shared with the candidate replay in
``tools/wiz8decomp/ghidra/candidate_model.py``.
"""

from __future__ import annotations

import csv
import io
from pathlib import Path
from typing import Any

from ..ghidra.candidate_model import classify_candidates
from ..paths import atomic_json, atomic_write

_REPORT_NAME = "class-candidates"


def _read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _canonical_program(rows: list[dict[str, str]]) -> str:
    return next(row["program"] for row in rows if "--gog-base--" in row["program"])


def load_candidates(settings: Any) -> tuple[str, list[dict[str, Any]]]:
    """The canonical program's candidates from the tracked snapshots."""

    snapshots = settings.repo_dir / "evidence" / "snapshots" / "polymorphism"
    vtables = _read(snapshots / "vtables.csv")
    program = _canonical_program(vtables)
    vtables = [row for row in vtables if row["program"] == program]
    slots = [row for row in _read(snapshots / "slots.csv") if row["program"] == program]
    writes = [
        row
        for row in _read(snapshots / "vptr-writes.csv")
        if row["program"] == program
    ]
    reviewed = {
        int(row["address"], 16)
        for row in _read(
            settings.repo_dir / "evidence" / "reviewed" / "wiz8" / "vtables.csv"
        )
    }
    return program, classify_candidates(vtables, slots, writes, reviewed)


def class_candidates_report(settings: Any) -> dict[str, Any]:
    program, candidates = load_candidates(settings)

    rows: list[dict[str, str]] = []
    for item in sorted(candidates, key=lambda entry: entry["vtable"]):
        rows.append(
            {
                "program": program,
                "vtable": f"{item['vtable']:08x}",
                "section": item["section"],
                "slot_count": str(item["slot_count"]),
                "pure_virtual_slots": str(item["pure_virtual_slots"]),
                "import_slots": str(item["import_slots"]),
                "subobject_offsets": item["subobject_offsets"],
                "scalar_deleting_destructor": (
                    f"{item['scalar_deleting_destructor']:08x}"
                    if item["scalar_deleting_destructor"] is not None
                    else ""
                ),
                "constructor_or_destructor": "|".join(
                    f"{writer:08x}" for writer in item["constructor_or_destructor"]
                ),
                "allocation_size_hints": "|".join(
                    f"0x{size:x}" for size in item["allocation_sizes"]
                ),
                "co_installed_vtables": "|".join(
                    f"{vtable:08x}@0x{offset:x}"
                    for vtable, offset in item["co_installed_vtables"]
                ),
                "reviewed_class": "yes" if item["reviewed"] else "",
                "evidence": "vptr-writes sites " + ",".join(item["write_sites"]),
            }
        )

    stream_fields = list(rows[0].keys()) if rows else []
    report_dir = settings.build_dir / "reports" / _REPORT_NAME
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=stream_fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    atomic_write(report_dir / "candidates.csv", stream.getvalue())

    summary = {
        "schema": "wiz8.class-candidates",
        "program": program,
        "candidates": len(rows),
        "reviewed": sum(1 for row in rows if row["reviewed_class"]),
        "unreviewed": sum(1 for row in rows if not row["reviewed_class"]),
        "with_deleting_destructor": sum(
            1 for row in rows if row["scalar_deleting_destructor"]
        ),
        "with_allocation_hint": sum(1 for row in rows if row["allocation_size_hints"]),
        "outputs": [f"build/reports/{_REPORT_NAME}/candidates.csv"],
    }
    atomic_json(report_dir / "summary.json", summary)
    return summary
