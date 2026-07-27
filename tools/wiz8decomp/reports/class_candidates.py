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

One limitation is structural and stated in every template: outside Ghidra
there is no reliable way to say which function a vptr write belongs to. The
census attributes writes by inter-function padding, which merges adjacent
small bodies, and the tracked function census proposes starts inside real
functions; adjudicating the 295 canonical disagreements against Ghidra found
the padding attribution right 55 times, the function census right 119, and
neither right 118. So writer *roles* in this report are leads to confirm, and
the authoritative attribution is the one the replay materializes into the
program from Ghidra's own containment.
"""

from __future__ import annotations

import csv
import io
from pathlib import Path
from typing import Any

from ..ghidra.candidate_model import (
    candidate_name,
    classify_candidates,
    derive_skeletons,
)
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


_BANNER = (
    "GENERATED CANDIDATE - machine-derived from the polymorphism census.\n"
    "Nothing here is reviewed evidence: verify every value against the\n"
    "writers' decompiles before promoting anything into evidence/ or src/."
)


def header_skeleton(skeleton: dict[str, Any]) -> str:
    """A compilable W8-convention struct skeleton for one candidate."""

    name = skeleton["name"]
    lines = [
        "/*",
        *(f"   {line}" for line in _BANNER.splitlines()),
        "*/",
        "",
        "#pragma pack(push, 1)",
        f"struct {name} {{",
    ]
    cursor = 0
    for offset, vtable in skeleton["vptr_offsets"]:
        if offset > cursor:
            lines.append(
                f"    unsigned char unknown_{cursor:03x}[0x{offset - cursor:x}];"
            )
        field = "vptr" if offset == 0 else f"vptr_{offset:x}"
        lines.append(
            f"    void* {field};{' ' * max(1, 24 - len(field))}"
            f"/* 0x{offset:03x}: candidate vtable 0x{vtable:08x} */"
        )
        cursor = offset + 4
    if skeleton["size"] > cursor:
        lines.append(
            f"    unsigned char unknown_{cursor:03x}[0x{skeleton['size'] - cursor:x}];"
        )
    lines.append("};")
    lines.append("#pragma pack(pop)")
    lines.append("")
    origin = (
        "allocation hint" if skeleton["size_is_allocation_hint"] else "vptr extent only"
    )
    lines.append(f"/* size origin: {origin} */")
    lines.append(
        f"typedef char {name}_size_must_be_0x{skeleton['size']:x}["
    )
    lines.append(f"    sizeof(struct {name}) == 0x{skeleton['size']:x} ? 1 : -1];")
    lines.append("")
    return "\n".join(lines)


def promotion_template(
    candidate: dict[str, Any],
    skeleton: dict[str, Any],
    slot_targets: dict[int, list[str]],
    slot_counts: dict[int, int],
) -> str:
    """Prefilled reviewed-CSV row snippets for one candidate's promotion."""

    name = skeleton["name"]
    vtable = candidate["vtable"]
    deleting = candidate["scalar_deleting_destructor"]
    slot0 = candidate["slot0_target"]
    writers = ", ".join(f"0x{w:08x}" for w in candidate["constructor_or_destructor"])
    hints = ", ".join(f"0x{s:x}" for s in candidate["allocation_sizes"]) or "none"
    if slot0:
        deleting_note = (
            f"- vtable slot 0: 0x{slot0:08x} - MSVC's scalar deleting destructor "
            "position when the class has a virtual destructor"
            + (
                "; the census also attributes a vtable write to it"
                if deleting
                else ", and it may delegate the vtable restore to a complete "
                "destructor that writes instead"
            )
        )
    else:
        deleting_note = "- vtable slot 0: not resolved"
    lines = [
        f"# {name} promotion template",
        "",
        *(f"> {line}" for line in _BANNER.splitlines()),
        "",
        f"- writers to review: {writers or 'none'}",
        "  (census padding attribution, which merges adjacent small bodies -"
        " resolve the write sites below with"
        " `just ghidra query <program> function-of <sites>`, or read the"
        " candidate-class comments in the materialized program, which use"
        " Ghidra's own containment)",
        deleting_note,
        f"- allocation hints: {hints}",
        f"- vptr-write sites: {', '.join(candidate['write_sites'])}",
        "- base class: the constructor's *first* call is usually the base"
        " constructor; if it is another candidate's writer, that candidate is"
        " the base and its allocation hint is the base extent",
        "",
        "## evidence/reviewed/wiz8/classes.csv",
        "```csv",
        f"wiz8,<class-name>,<confidence>,<class-name>.primary,<constructor>,"
        f"<destructor>,{f'{deleting:08x}' if deleting else '<scalar-deleting>'},"
        f"0x{skeleton['size']:x},<base-classes>,<base-name-origin>,<source-path>,"
        '"<evidence>",<layout-proof>',
        "```",
        "",
        "## evidence/reviewed/wiz8/vtables.csv",
        "```csv",
        f"wiz8,<class-name>.primary,<class-name>,{vtable:08x},0x0,primary,"
        f"{candidate['slot_count']},<confidence>,classes:wiz8:<class-name>",
    ]
    for other, offset in candidate["subobject_vtables"]:
        lines.append(
            f"wiz8,<class-name>.secondary_0x{offset:x},<class-name>,{other:08x},"
            f"0x{offset:x},secondary,{slot_counts.get(other, 0)},<confidence>,"
            "classes:wiz8:<class-name>"
        )
    lines.append("```")
    lines.append("")
    lines.append("## evidence/reviewed/wiz8/vtable-slots.csv")
    lines.append("```csv")
    for index, target in enumerate(slot_targets.get(vtable, [])):
        lines.append(
            f"wiz8,<class-name>.primary,{index},{target},,<confidence>,"
            "classes:wiz8:<class-name>"
        )
    lines.append("```")
    lines.append("")
    lines.append("## evidence/reviewed/wiz8/fields.csv")
    lines.append("")
    lines.append(
        "One row per established field; opaque runs stay `bytes`. Offsets must "
        "not overlap and must fit the class size."
    )
    lines.append("```csv")
    lines.append(
        "wiz8,<class-name>,0x0,0x4,vptr,pointer,virtual_function *,<confidence>,"
        "classes:wiz8:<class-name>,primary vtable <class-name>.primary"
    )
    for _, offset in [(0, offset) for offset, _ in skeleton["vptr_offsets"][1:]]:
        lines.append(
            f"wiz8,<class-name>,0x{offset:x},0x4,secondary_vptr_{offset:x},pointer,"
            f"virtual_function *,<confidence>,classes:wiz8:<class-name>,"
            f"subobject vtable at +0x{offset:x}"
        )
    lines.append("```")
    lines.append("")
    return "\n".join(lines)


def class_candidates_report(settings: Any) -> dict[str, Any]:
    program, candidates = load_candidates(settings)
    skeletons = {item["vtable"]: item for item in derive_skeletons(candidates)}

    snapshots = settings.repo_dir / "evidence" / "snapshots" / "polymorphism"
    slot_targets: dict[int, list[str]] = {}
    for row in _read(snapshots / "slots.csv"):
        if row["program"] == program and row["kind"] != "base-displacement":
            slot_targets.setdefault(int(row["vtable"], 16), []).append(row["target"])
    slot_counts = {
        int(row["address"], 16): int(row["slot_count"] or 0)
        for row in _read(snapshots / "vtables.csv")
        if row["program"] == program
    }

    report_dir = settings.build_dir / "reports" / _REPORT_NAME
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
                "slot0_target": (
                    f"{item['slot0_target']:08x}"
                    if item["slot0_target"] is not None
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
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=stream_fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    atomic_write(report_dir / "candidates.csv", stream.getvalue())

    templates = 0
    headers = 0
    for item in candidates:
        skeleton = skeletons.get(item["vtable"])
        if skeleton is None:
            continue
        stem = candidate_name(item["vtable"]).lower()
        atomic_write(
            report_dir / "promotion" / f"{stem}.md",
            promotion_template(item, skeleton, slot_targets, slot_counts),
        )
        templates += 1
        atomic_write(report_dir / "headers" / f"{stem}.h", header_skeleton(skeleton))
        headers += 1

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
        "promotion_templates": templates,
        "header_skeletons": headers,
        "outputs": [
            f"build/reports/{_REPORT_NAME}/candidates.csv",
            f"build/reports/{_REPORT_NAME}/promotion/",
            f"build/reports/{_REPORT_NAME}/headers/",
        ],
    }
    atomic_json(report_dir / "summary.json", summary)
    return summary
