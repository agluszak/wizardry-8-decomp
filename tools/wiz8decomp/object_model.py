"""VC6 object-model rules: classify what a vtable artifact *is*.

A constructor-written vtable address is not automatically a first-party class.
The srTimer correction proved the cost of assuming it is: vtable 0x005EC078
carried a candidate class, a ported subclass, and a wrong reviewed note before
the bytes showed it to be the local copy VC6 materializes when a translation
unit instantiates a dllimport class. This module makes that distinction a
computation over the polymorphism census instead of a post-mortem.

The classifier is deliberately conservative: it distinguishes artifact kinds
it has positive evidence for and says `unclassified` otherwise. Its verdicts
are observations feeding the candidate layer - reclassification input, never
automatic promotion or deletion of a reviewed fact.
"""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Any

# An import thunk's decorated name carries its class between the first '@' pair:
# ?method@srTimer@@... - enough to group a table's slots by imported owner.


def _import_class(import_name: str) -> str | None:
    decorated = import_name.split("!", 1)[-1]
    if not decorated.startswith("?"):
        return None
    body = decorated.lstrip("?").split("@@", 1)[0]
    if "@" not in body:
        return None
    return body.rsplit("@", 1)[-1]


def classify_vtable(slots: list[dict[str, str]]) -> dict[str, Any]:
    """One verdict for one table's slot rows from the polymorphism census.

    The rules, in order of specificity:

    * `local-import-vtable-copy`: every ordinary slot reaches one imported
      class through an import thunk and slot 0 is the only local body - the
      shape VC6 materializes for `new ImportedClass(...)`, whose slot-0 body
      is the locally generated deleting destructor. The srTimer table.
    * `derived-from-import`: import thunks of one class beside two or more
      local slots - a first-party subclass inheriting what it does not
      override. The stMaterial table.
    * `abstract-first-party`: no imports, one or more pure-virtual slots.
    * `first-party`: local slots only.
    * `mixed-import`: thunks of more than one imported class; multiple
      inheritance or a mis-bounded run, and worth a human look either way.
    """

    ordered = sorted(slots, key=lambda row: int(row["slot_index"]))
    import_owners = {
        owner
        for row in ordered
        if row["kind"] == "import-thunk"
        and row["import_name"]
        and (owner := _import_class(row["import_name"])) is not None
    }
    local = [row for row in ordered if row["kind"] == "local"]
    pure = [row for row in ordered if row["kind"] == "pure-virtual"]

    verdict: dict[str, Any] = {
        "slots": len(ordered),
        "import_slots": len(ordered) - len(local) - len(pure),
        "local_slots": len(local),
        "pure_slots": len(pure),
        "imported_classes": sorted(import_owners),
    }
    if len(import_owners) > 1:
        verdict["kind"] = "mixed-import"
    elif len(import_owners) == 1:
        owner = next(iter(import_owners))
        if len(local) == 1 and ordered and ordered[0]["kind"] == "local" and not pure:
            verdict["kind"] = "local-import-vtable-copy"
            verdict["imported_class"] = owner
            verdict["local_deleting_destructor"] = ordered[0]["target"]
        else:
            verdict["kind"] = "derived-from-import"
            verdict["imported_class"] = owner
    elif pure:
        verdict["kind"] = "abstract-first-party"
    elif local:
        verdict["kind"] = "first-party"
    else:
        verdict["kind"] = "unclassified"
    return verdict


def classify_program_vtables(repo: Path, program: str) -> dict[str, dict[str, Any]]:
    """Every censused vftable of one program, classified."""

    slots_by_table: dict[str, list[dict[str, str]]] = defaultdict(list)
    with (repo / "evidence" / "snapshots" / "polymorphism" / "slots.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        for row in csv.DictReader(stream):
            if row["program"] == program:
                slots_by_table[row["vtable"]].append(row)
    return {table: classify_vtable(rows) for table, rows in sorted(slots_by_table.items())}


def attribute_writers(
    writes: list[dict[str, str]], containment: dict[str, str | None]
) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    """Re-attribute each vptr write to its real containing function.

    The census guesses containment from inter-function padding and the class
    -triage skill measures that guess wrong in most disagreements; Ghidra's
    function bodies are the authority. Returns the corrected writes and the
    corrections themselves, because a correction severs a derivation the
    candidate layer may have built on the wrong writer.
    """

    corrected: list[dict[str, str]] = []
    corrections: list[dict[str, str]] = []
    for write in writes:
        real = containment.get("0x" + write["site"]) or containment.get(write["site"])
        entry = dict(write)
        if real:
            real = real.zfill(8)
            if real != write["function_start"]:
                corrections.append(
                    {
                        "site": write["site"],
                        "census": write["function_start"],
                        "containment": real,
                    }
                )
            entry["function_start"] = real
        corrected.append(entry)
    return corrected, corrections


def lifecycle_unifications(
    corrected_writes: list[dict[str, str]],
    reviewed_lifecycles: dict[str, set[str]],
) -> list[dict[str, Any]]:
    """Candidate tables whose writers are a reviewed class's own lifecycle.

    This is the anti-duplicate rule the widget owner needed: an agent meeting
    vtable 0x005ED5BC cold would invent an owner type, but its writers are the
    reviewed W8WidgetBase005ED5BC's destructor - the table already has an
    identity, and inventing a second one recreates the duplicate-model problem.
    """

    writers_by_table: dict[str, set[str]] = defaultdict(set)
    for write in corrected_writes:
        if write["object_offset"] == "0x0":
            writers_by_table[write["vtable"]].add(write["function_start"])
    proposals = []
    for table, writers in sorted(writers_by_table.items()):
        for class_name, lifecycle in sorted(reviewed_lifecycles.items()):
            shared = sorted(writers & lifecycle)
            if shared:
                proposals.append(
                    {
                        "vtable": table,
                        "unifies_with": class_name,
                        "shared_lifecycle": shared,
                        "reason": "writers are this reviewed class's own lifecycle functions",
                    }
                )
    return proposals


def load_reviewed_lifecycles(repo: Path) -> dict[str, set[str]]:
    lifecycles: dict[str, set[str]] = {}
    with (repo / "evidence" / "reviewed" / "wiz8" / "classes.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        for row in csv.DictReader(stream):
            addresses = {
                row[key].strip().lower().zfill(8)
                for key in ("constructor", "destructor", "scalar_deleting_destructor")
                if row[key].strip()
            }
            if addresses:
                lifecycles[row["class_name"]] = addresses
    return lifecycles


def destructor_family(
    seed: str,
    corrected_writes: list[dict[str, str]],
    calls: list[dict[str, str]],
) -> dict[str, Any]:
    """The class family around one shared base destructor, scored by fan-out.

    Every derived destructor calls the base destructor last, and every derived
    destructor restores its own vtable first - so the callers of the seed,
    joined against the corrected writes, are the family's tables. The score is
    what the family unlocks: recovering the base types every derived table's
    slots at once.
    """

    callers = {
        call["caller"].lstrip("0").zfill(8)
        for call in calls
        if call["callee"].lstrip("0") == seed.lstrip("0")
    }
    members: dict[str, set[str]] = defaultdict(set)
    for write in corrected_writes:
        writer = write["function_start"].lstrip("0").zfill(8)
        if writer in callers and write["object_offset"] == "0x0":
            members[write["vtable"]].add(writer)
    return {
        "seed_destructor": seed,
        "caller_count": len(callers),
        "member_tables": {table: sorted(writers) for table, writers in sorted(members.items())},
        "fan_out_score": len(members) * 10 + len(callers),
    }
