"""Every vptr write around one class, so a family can be read at a glance.

Recovering a class stalls in a predictable place: a destructor restores a
vtable that is not the one you expected, and it is impossible to tell from a
single body whether you are looking at a derived class whose own store was
dropped as dead, a base whose destructor was inlined into its derived, or two
adjacent tables the census split.

What answers it is the whole write map at once - which function stores which
table at which object offset - because construction order and destruction
order read straight off it. This turns that into one command rather than an
ad-hoc scan of the snapshot.

The report is a projection of the polymorphism census under ``build/``; it
concludes nothing and writes no evidence.
"""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..paths import atomic_json

_REPORT_NAME = "class-family"


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def collect_family(
    writes: list[dict[str, str]],
    tables: list[dict[str, str]],
    slots: list[dict[str, str]],
    seed: str,
) -> dict[str, Any]:
    """Everything transitively linked to SEED through shared writers or tables.

    Two tables belong to one family when some function writes both: that is
    exactly a constructor installing a base then a derived table, or a
    destructor restoring a subobject and its owner.
    """

    tables_by_write: dict[str, set[str]] = {}
    writers_by_table: dict[str, set[str]] = {}
    for row in writes:
        if not row["function_start"]:
            continue
        tables_by_write.setdefault(row["function_start"], set()).add(row["vtable"])
        writers_by_table.setdefault(row["vtable"], set()).add(row["function_start"])

    family = {seed}
    frontier = [seed]
    while frontier:
        table = frontier.pop()
        for writer in writers_by_table.get(table, ()):
            for other in tables_by_write.get(writer, ()):
                if other not in family:
                    family.add(other)
                    frontier.append(other)

    table_info = {row["address"]: row for row in tables}
    slot_counts: dict[str, int] = {}
    for row in slots:
        slot_counts[row["vtable"]] = slot_counts.get(row["vtable"], 0) + 1

    members = []
    for address in sorted(family):
        info = table_info.get(address, {})
        members.append(
            {
                "vtable": address,
                "slot_count": int(info.get("slot_count") or slot_counts.get(address, 0)),
                "kind": info.get("kind", ""),
                "written_by": sorted(writers_by_table.get(address, ())),
            }
        )

    ordered_writes = sorted(
        (
            {
                "function": row["function_start"],
                "site": row["site"],
                "object_offset": row["object_offset"],
                "vtable": row["vtable"],
            }
            for row in writes
            if row["vtable"] in family and row["function_start"]
        ),
        key=lambda item: (item["function"], item["site"]),
    )
    return {"seed": seed, "tables": members, "writes": ordered_writes}


def render_family(family: dict[str, Any]) -> str:
    """A per-function view: what each writer installs, in address order.

    Reading down one function's rows gives construction or destruction order
    directly, and a function that writes two tables at the same offset is
    installing a base then a derived one.
    """

    lines = [f"family seeded at {family['seed']}", ""]
    lines.append("tables:")
    for table in family["tables"]:
        writers = ", ".join(table["written_by"]) or "no decodable writer"
        lines.append(
            f"  {table['vtable']}  {table['slot_count']:>3} slots  {table['kind']:<8} "
            f"written by {writers}"
        )
    lines.append("")
    lines.append("writes, grouped by function:")
    current = None
    for write in family["writes"]:
        if write["function"] != current:
            current = write["function"]
            lines.append(f"  {current}:")
        # No square brackets: this string is printed through Rich, which would
        # read them as markup and silently drop the offset.
        lines.append(
            f"    site {write['site']}  this+{write['object_offset']:<6} = {write['vtable']}"
        )
    return "\n".join(lines) + "\n"


def class_family_report(settings: Any, vtable: str) -> dict[str, Any]:
    snapshots = settings.repo_dir / "evidence" / "snapshots" / "polymorphism"
    tables = _rows(snapshots / "vtables.csv")
    program = next(row["program"] for row in tables if "--gog-base--" in row["program"])
    tables = [row for row in tables if row["program"] == program]
    slots = [row for row in _rows(snapshots / "slots.csv") if row["program"] == program]
    writes = [
        row for row in _rows(snapshots / "vptr-writes.csv") if row["program"] == program
    ]

    seed = vtable.lower().removeprefix("0x").rjust(8, "0")
    known = {row["address"] for row in tables} | {row["vtable"] for row in writes}
    if seed not in known:
        raise ValueError(f"{seed} is not a table the census reports for {program}")

    family = collect_family(writes, tables, slots, seed)
    report_dir = settings.build_dir / "reports" / _REPORT_NAME
    atomic_json(report_dir / f"{seed}.json", family)
    return {
        "schema": "wiz8.class-family",
        "program": program,
        **family,
        "rendered": render_family(family),
        "outputs": [f"build/reports/{_REPORT_NAME}/{seed}.json"],
    }
