"""Generate candidate class records from the polymorphism snapshots.

The polymorphism census records every constructor-written vtable and every
vptr write site, but only a handful of classes are reviewed. This report
turns that gap into triage: one candidate row per constructor-written
vftable, carrying the writer functions with their ctor/dtor classification,
co-installed subobject vtables, slot-count and pure-virtual facts, and -
when the original image is available - allocation-size hints from
push-before-new scans at the writers' call sites.

Candidates are a generated projection under ``build/reports/`` and are never
auto-promoted. The promotion path is: review a candidate against decompiles
of its writers, then record the accepted identity as rows in
``evidence/reviewed/wiz8/classes.csv`` / ``vtables.csv`` / ``vtable-slots.csv``
(and the writers in ``functions.csv``), citing the writer sites this report
points at. The generator reads snapshots and reviewed evidence; it writes
neither.
"""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..paths import atomic_json, atomic_write

_REPORT_NAME = "class-candidates"


def _read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _canonical_program(rows: list[dict[str, str]]) -> str:
    return next(row["program"] for row in rows if "--gog-base--" in row["program"])


def classify_candidates(
    vtables: list[dict[str, str]],
    slots: list[dict[str, str]],
    writes: list[dict[str, str]],
    reviewed_vtables: set[int],
) -> list[dict[str, Any]]:
    """One candidate per constructor-written vftable, writers classified.

    A writer that is also the vtable's slot 0 target is MSVC's scalar
    deleting destructor writing the vtable during destruction; every other
    writer is a constructor or the complete destructor, which this evidence
    alone cannot separate. Vtables the same writers install at non-zero
    object offsets are that candidate's subobject tables.
    """

    slot0: dict[int, int] = {}
    for row in slots:
        if row["slot_index"] == "0" and row["target"]:
            slot0[int(row["vtable"], 16)] = int(row["target"], 16)

    writers_by_vtable: dict[int, list[dict[str, str]]] = defaultdict(list)
    vtables_by_writer: dict[int, set[tuple[int, int]]] = defaultdict(set)
    for row in writes:
        if not row["function_start"]:
            continue
        vtable = int(row["vtable"], 16)
        writer = int(row["function_start"], 16)
        writers_by_vtable[vtable].append(row)
        vtables_by_writer[writer].add((vtable, int(row["object_offset"], 0)))

    candidates: list[dict[str, Any]] = []
    for row in vtables:
        if row["kind"] != "vftable":
            continue
        vtable = int(row["address"], 16)
        write_rows = writers_by_vtable.get(vtable, [])
        primary_writers = sorted(
            {
                int(item["function_start"], 16)
                for item in write_rows
                if int(item["object_offset"], 0) == 0
            }
        )
        if not primary_writers:
            continue
        deleting = slot0.get(vtable)
        constructors = [
            writer for writer in primary_writers if writer != deleting
        ]
        co_installed = sorted(
            {
                (other, offset)
                for writer in primary_writers
                for other, offset in vtables_by_writer[writer]
                if other != vtable and offset != 0
            }
        )
        candidates.append(
            {
                "vtable": vtable,
                "section": row["section"],
                "slot_count": int(row["slot_count"] or 0),
                "pure_virtual_slots": int(row["pure_virtual_slots"] or 0),
                "import_slots": int(row["import_slots"] or 0),
                "subobject_offsets": row["subobject_offsets"],
                "scalar_deleting_destructor": (
                    deleting if deleting in primary_writers else None
                ),
                "constructor_or_destructor": constructors,
                "co_installed_vtables": co_installed,
                "write_sites": sorted(item["site"] for item in write_rows),
                "reviewed": vtable in reviewed_vtables,
            }
        )
    return candidates


def allocation_size_hints(
    image: Any, writers: list[int], allocators: set[int]
) -> dict[int, list[int]]:
    """Push-immediate-before-new hints for each writer's call sites.

    The MSVC shape is ``push size; call operator new; ...; call ctor``. For
    every direct E8 call to a writer, the preceding 48 bytes are scanned for
    a push-immediate followed by a call into a known allocator; the pushed
    immediate is that construction site's allocation size. Absence of a hint
    means the object is embedded, stack-placed, or reached indirectly.
    """

    text = image.text
    data = image.data
    sizes: dict[int, set[int]] = defaultdict(set)
    writer_set = set(writers)
    start = text.raw_offset
    end = text.raw_offset + text.raw_size
    offset = start
    while True:
        offset = data.find(b"\xe8", offset, end)
        if offset < 0:
            break
        site = text.virtual_address + (offset - text.raw_offset)
        target = (site + 5 + int.from_bytes(data[offset + 1 : offset + 5], "little", signed=True)) & 0xFFFFFFFF
        if target in writer_set:
            window = data[max(start, offset - 48) : offset]
            hint = _push_before_allocator(window, site - len(window), allocators)
            if hint is not None:
                sizes[target].add(hint)
        offset += 1
    return {writer: sorted(values) for writer, values in sizes.items()}


def _push_before_allocator(
    window: bytes, window_va: int, allocators: set[int]
) -> int | None:
    """The last push-immediate whose next call lands in an allocator."""

    best: int | None = None
    index = 0
    while index < len(window):
        byte = window[index]
        pushed: int | None = None
        after = index
        if byte == 0x68 and index + 5 <= len(window):
            pushed = int.from_bytes(window[index + 1 : index + 5], "little")
            after = index + 5
        elif byte == 0x6A and index + 2 <= len(window):
            pushed = window[index + 1]
            after = index + 2
        if pushed is not None and after < len(window) and window[after] == 0xE8:
            if after + 5 <= len(window):
                rel = int.from_bytes(window[after + 1 : after + 5], "little", signed=True)
                target = (window_va + after + 5 + rel) & 0xFFFFFFFF
                if target in allocators:
                    best = pushed
        index += 1
    return best


def class_candidates_report(settings: Any) -> dict[str, Any]:
    snapshots = settings.repo_dir / "evidence" / "snapshots" / "polymorphism"
    vtables = _read(snapshots / "vtables.csv")
    program = _canonical_program(vtables)
    vtables = [row for row in vtables if row["program"] == program]
    slots = [
        row
        for row in _read(snapshots / "slots.csv")
        if row["program"] == program
    ]
    writes = [
        row
        for row in _read(snapshots / "vptr-writes.csv")
        if row["program"] == program
    ]
    reviewed = {
        int(row["address"], 16)
        for row in _read(settings.repo_dir / "evidence" / "reviewed" / "wiz8" / "vtables.csv")
    }
    candidates = classify_candidates(vtables, slots, writes, reviewed)

    allocators = {
        int(row["address"], 16)
        for row in _read(
            settings.repo_dir / "evidence" / "reviewed" / "wiz8" / "allocator-layers.csv"
        )
        if row["address_kind"] in {"function", "import-thunk"}
    }
    sizes: dict[int, list[int]] = {}
    image_available = False
    image_path = getattr(settings, "work_dir", None)
    if image_path is not None:
        candidate_image = Path(image_path) / "variants" / "gog-base" / "Wiz8.exe"
        if candidate_image.is_file():
            from ..binary.image import PeImage

            image_available = True
            writers = sorted(
                {
                    writer
                    for item in candidates
                    for writer in item["constructor_or_destructor"]
                }
            )
            sizes = allocation_size_hints(PeImage(candidate_image), writers, allocators)

    rows: list[dict[str, str]] = []
    for item in sorted(candidates, key=lambda entry: entry["vtable"]):
        hints = sorted(
            {
                size
                for writer in item["constructor_or_destructor"]
                for size in sizes.get(writer, [])
            }
        )
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
                "allocation_size_hints": "|".join(f"0x{size:x}" for size in hints),
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
    csv_path = report_dir / "candidates.csv"
    import io

    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=stream_fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    atomic_write(csv_path, stream.getvalue())

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
        "allocation_scan_ran": image_available,
        "outputs": [f"build/reports/{_REPORT_NAME}/candidates.csv"],
    }
    atomic_json(report_dir / "summary.json", summary)
    return summary
