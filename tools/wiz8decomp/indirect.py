"""Finite target sets for the calls the static call graph cannot see.

The call census records direct edges. The program's real control flow runs
through the ones it cannot: virtual calls through a vtable slot, and handler
tables indexed by state. The screen dispatcher is the extreme case - the whole
application's navigation is `g_screen_handlers[state](...)`, and to a
direct-edge graph the screens are unreachable islands.

Both shapes resolve to *finite target sets* rather than single edges, and the
distinction is load-bearing: a virtual call through a slot can land on any
override of that slot, and a table call on any handler the index can take.
Nothing here narrows a set by guessing which is likely; a runtime trace can
later mark which were observed, and that is a different claim from possible.

Identical-COMDAT folding makes one more caveat structural rather than
incidental. The dispatch table's shared `mov al,1; ret` stub occupies 17
slots, and those are seventeen trivial handlers the linker merged - not one
handler used seventeen ways - so a resolver that reports them as a single
target would be inventing a relationship the binary does not have.
"""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Any


def resolve_handler_table(repo: Path) -> dict[str, Any]:
    """The screen dispatcher's slots as reachable targets, folding accounted for."""

    path = repo / "evidence" / "observations" / "wiz8" / "frame-dispatch-table.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))

    handlers: dict[str, list[int]] = defaultdict(list)
    stubs: dict[str, list[int]] = defaultdict(list)
    for row in rows:
        slot = int(row["slot"])
        if row["kind"] == "handler":
            handlers[row["handler_address"]].append(slot)
        else:
            stubs[row["handler_address"]].append(slot)

    folded = {
        address: slots for address, slots in stubs.items() if len(slots) > 1
    }
    return {
        "table_slots": len(rows),
        "distinct_handlers": len(handlers),
        "handler_targets": {address: sorted(slots) for address, slots in sorted(handlers.items())},
        "folded_stubs": {address: sorted(slots) for address, slots in sorted(folded.items())},
        "note": (
            "a folded stub's slots are that many trivial handlers the linker merged, "
            "not one handler shared by that many states"
        ),
    }


def slot_override_sets(repo: Path, program: str) -> dict[int, dict[str, Any]]:
    """Per slot index, every target any censused vtable puts there.

    A virtual call through slot n can reach any override of slot n among the
    receiver's possible types. With no receiver type this is the widest honest
    answer; the object model narrows it by supplying candidate receivers.
    """

    by_slot: dict[int, set[str]] = defaultdict(set)
    tables_by_slot: dict[int, set[str]] = defaultdict(set)
    with (
        repo / "evidence" / "snapshots" / "polymorphism" / "slots.csv"
    ).open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] != program or not row["target"]:
                continue
            index = int(row["slot_index"])
            by_slot[index].add(row["target"])
            tables_by_slot[index].add(row["vtable"])
    return {
        index: {
            "targets": sorted(targets),
            "tables": len(tables_by_slot[index]),
        }
        for index, targets in sorted(by_slot.items())
    }


def resolve_virtual_call(
    slot: int,
    receiver_tables: list[str],
    repo: Path,
    program: str,
) -> dict[str, Any]:
    """The targets slot `slot` can reach given a receiver's candidate tables.

    Supplying the receiver's possible vtables is what turns "any override of
    this slot in the image" into a set small enough to add as computed
    references. An empty receiver list returns the unnarrowed set and says so.
    """

    rows: dict[str, dict[int, str]] = defaultdict(dict)
    with (
        repo / "evidence" / "snapshots" / "polymorphism" / "slots.csv"
    ).open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] == program and row["target"]:
                rows[row["vtable"]][int(row["slot_index"])] = row["target"]

    if receiver_tables:
        targets = sorted(
            {
                rows[table][slot]
                for table in receiver_tables
                if table in rows and slot in rows[table]
            }
        )
        narrowed = True
    else:
        targets = sorted({table_slots[slot] for table_slots in rows.values() if slot in table_slots})
        narrowed = False
    return {
        "slot": slot,
        "receiver_tables": sorted(receiver_tables),
        "targets": targets,
        "narrowed_by_receiver": narrowed,
        "finite": bool(targets),
    }
