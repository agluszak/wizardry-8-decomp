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
        if row["kind"] == "import-thunk" and row["import_name"]
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
    with (
        repo / "evidence" / "snapshots" / "polymorphism" / "slots.csv"
    ).open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] == program:
                slots_by_table[row["vtable"]].append(row)
    return {
        table: classify_vtable(rows) for table, rows in sorted(slots_by_table.items())
    }
