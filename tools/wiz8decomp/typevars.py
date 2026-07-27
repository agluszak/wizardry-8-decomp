"""Anonymous type variables: partial type-shape constraints with identities.

The reviewed field model can say "pointer" about a member; what a destructor
actually proves is richer - *the pointee has a virtual destructor in slot 0*,
or *its non-virtual destructor is 0x004B6ED0* - and until now that lived as
prose in evidence columns. This module turns the structured access stream from
`field-accesses` into named type variables carrying those constraints, so the
partial fact is usable before anyone knows the type's name, and unification
with a known class is a computation instead of a rewrite.

A type variable is an identity, not a guess: `AnonymousType_0044bec0_field38`
means "whatever `this+0x38` of the function at 0x0044BEC0 points at", and its
constraints only ever narrow. Unification returns the known types consistent
with the constraints, ranked by how specifically they were selected - and an
empty answer is the correct answer for a type nobody has recovered yet.
"""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any


def _first_level_paths(root: str, accesses: list[dict[str, Any]]) -> dict[int, list[dict[str, Any]]]:
    """Member offset -> the access records describing that member's pointee."""

    members: dict[int, list[dict[str, Any]]] = {}
    prefix = root + "["
    for access in accesses:
        path = access["path"]
        if path == root and access["kind"] == "load":
            members.setdefault(int(access["offset"], 16), [])
        elif path.startswith(prefix):
            head = path[len(prefix) : path.index("]", len(prefix))]
            members.setdefault(int(head, 16), []).append(access)
    return members


def derive_type_variables(
    entry: str,
    root: str,
    accesses: list[dict[str, Any]],
    calls: list[dict[str, Any]],
    deleters: set[str],
) -> list[dict[str, Any]]:
    """One type variable per pointer member the access stream constrains.

    The rules are the VC6 destruction shapes, read off the same structure a
    human read off the Prop destructor:

    * a chain member -> [0x0] -> [slot] ending in an indirect call is a
      virtual call through the vtable; in a teardown path that is the deleting
      destructor, and the slot index is part of the constraint;
    * a member passed to operator delete with no other call in the delete's
      basic block was freed without any destructor body - the shape an empty,
      user-declared destructor compiles to (a trivially destructible member
      would skip the null test entirely, and the test is recorded);
    * a member passed to operator delete with a preceding direct call in the
      same block had that call as its non-virtual destructor.
    """

    calls_by_site = {call["site"]: call for call in calls}
    variables: list[dict[str, Any]] = []
    for offset, events in sorted(_first_level_paths(root, accesses).items()):
        if not events:
            continue
        member_path = f"{root}[{offset:#x}]"
        constraints: dict[str, Any] = {"pointer": True}
        sources = sorted({event["site"] for event in events})
        if any(event["kind"] == "null-test" for event in events):
            constraints["null_checked"] = True

        for event in events:
            if event["kind"] == "indirect-call-target" and event["path"].startswith(
                member_path + "[0x0]["
            ):
                slot_offset = int(event["path"].rsplit("[", 1)[-1].rstrip("]"), 16)
                constraints["has_virtual_destructor"] = True
                constraints["deleting_destructor_slot"] = slot_offset // 4

        for event in events:
            if event["kind"] != "call-arg" or event.get("target") not in deleters:
                continue
            constraints["deleted"] = True
            delete_call = calls_by_site.get(event["site"])
            if delete_call is None:
                continue
            preceding = [
                call
                for call in calls
                if call["block"] == delete_call["block"]
                and call["order"] < delete_call["order"]
                and isinstance(call["target"], str)
                and call["target"] not in deleters
            ]
            if preceding:
                constraints["complete_destructor"] = preceding[-1]["target"]
                constraints["destructor_is_virtual"] = False
            elif "has_virtual_destructor" not in constraints:
                constraints["declared_empty_destructor"] = True

        if len(constraints) == 1:
            # A bare pointer with no shape evidence is not worth a variable.
            continue
        variables.append(
            {
                "name": f"AnonymousType_{entry}_field{offset:x}",
                "function": entry,
                "root_offset": f"0x{offset:x}",
                "constraints": constraints,
                "sources": sources,
            }
        )
    return variables


def load_knowledge(repo: Path) -> list[dict[str, Any]]:
    """Known types as unification targets, from the reviewed model and census.

    Reviewed classes contribute their vtable slot-0 targets and lifecycle
    addresses; the polymorphism census contributes each candidate vtable's
    slot-0 target. Tier rides along so a unification can say how strong its
    match is - and nothing here invents a type.
    """

    knowledge: list[dict[str, Any]] = []
    reviewed = repo / "evidence" / "reviewed" / "wiz8"

    slot0: dict[str, str] = {}
    with (reviewed / "vtable-slots.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["slot_index"] == "0":
                slot0[row["vtable_id"]] = row["target"].strip().lower()

    with (reviewed / "classes.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            destructors = {
                row[key].strip().lower()
                for key in ("destructor", "scalar_deleting_destructor")
                if row[key].strip()
            }
            entry = {
                "type": row["class_name"],
                "tier": "reviewed",
                "polymorphic": bool(row["primary_vtable_id"]),
                "slot0": slot0.get(row["primary_vtable_id"]),
                "destructors": destructors,
            }
            knowledge.append(entry)

    with (
        repo / "evidence" / "snapshots" / "polymorphism" / "slots.csv"
    ).open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["slot_index"] != "0" or row["kind"] != "local":
                continue
            knowledge.append(
                {
                    "type": f"Candidate_{row['vtable']}",
                    "tier": "observation",
                    "polymorphic": True,
                    "slot0": row["target"].strip().lower(),
                    "destructors": set(),
                    "program": row["program"],
                }
            )
    return knowledge


def unify(variable: dict[str, Any], knowledge: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """The known types consistent with a variable's constraints, with reasons.

    A `complete_destructor` constraint is decisive: only a type owning that
    exact address matches. Shape-only constraints (a virtual destructor in a
    slot) select the class of polymorphic types, which is a filter rather than
    an identification - the caller sees the difference in the reasons.
    """

    constraints = variable["constraints"]
    matches: list[dict[str, Any]] = []
    for entry in knowledge:
        reasons: list[str] = []
        destructor = constraints.get("complete_destructor")
        if destructor is not None:
            if destructor.strip().lower() in entry["destructors"]:
                reasons.append(f"complete destructor {destructor} is this type's")
            else:
                continue
        if constraints.get("destructor_is_virtual") is False and entry.get("slot0") and destructor:
            # A non-virtual destructor identified by address does not exclude
            # a polymorphic type - the class may have both.
            pass
        if constraints.get("has_virtual_destructor"):
            if not entry["polymorphic"] or not entry.get("slot0"):
                continue
            reasons.append("polymorphic with a slot-0 destructor")
        if not reasons:
            continue
        matches.append({"type": entry["type"], "tier": entry["tier"], "reasons": reasons})
    decisive = [match for match in matches if any("complete destructor" in r for r in match["reasons"])]
    return decisive if decisive else matches
