"""Recover the original aggregates' member vocabulary from assertion text.

`gXStatus` is 0x49C2 bytes of almost entirely opaque global state, and the
decompiler renders every access to it as an unrelated `g_dword_...`. The names
of its members are not lost, though: a release build keeps the text of every
assertion it did not compile out, and an assertion reads
`gXStatus.uiMonstersInDatabase <= MAX_MONSTERS_IN_DATABASE`. Seventy aggregates
name themselves this way, `pWorld` with fifteen members, `pLevel` with twelve,
`gXStatus` with eleven.

What the text proves is worth separating from what it suggests.

* **Proven**: the aggregate has a member of that name; the containing function
  touches it; the assertion's own source path names the unit that does. The
  access operator is proof too - `gXStatus.fCombatMode` says the base is an
  object and `gpCombat->TargetHit` says it is a pointer - and a subscript says
  the member is an array. Nesting is proven the same way:
  `pMonsterInfo->pCombat->plsCombatActionList` establishes that `pCombat`
  points at something with that member.
* **Suggested**: this codebase's naming convention encodes a kind in each
  member's prefix. It is recorded in a column of its own, labelled as the
  convention it is, because a name is not a layout.

Offsets are deliberately absent. Which address a member sits at is a fact about
instructions, not about text, and it needs the access table only Ghidra can
produce; this module is the vocabulary and the constraints that table will be
matched against.
"""

from __future__ import annotations

import csv
import json
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

_ACCESS = re.compile(
    r"(?P<base>[A-Za-z_][A-Za-z0-9_]*)"
    r"(?P<tail>(?:\s*(?:\.|->)\s*[A-Za-z_][A-Za-z0-9_]*(?:\s*\[[^\]]*\])?)+)"
)
_STEP = re.compile(r"\s*(?P<operator>\.|->)\s*(?P<member>[A-Za-z_][A-Za-z0-9_]*)(?P<index>\s*\[)?")

# The codebase's own prefix convention. Recorded as a hypothesis about kind,
# never as a width or a type: `ui` is an unsigned count in this source, but a
# member's storage is settled by an access, not by its spelling.
CONVENTION = {
    "pls": "pointer to list",
    "pac": "pointer to array of pointers",
    "str": "string",
    "vec": "vector",
    "fl": "float",
    "ui": "unsigned integer",
    "ub": "unsigned byte",
    "ab": "array of bytes",
    "e": "enumeration",
    "f": "flag",
    "i": "integer",
    "l": "long",
    "b": "byte",
    "p": "pointer",
}
# `m_` marks a C++ class member and sits *before* the kind prefix, so it is
# stripped rather than being a kind of its own.
MEMBER_PREFIX = "m_"


@dataclass(frozen=True)
class MemberReference:
    """One member named by one assertion."""

    program: str
    base: str
    owner: str
    member: str
    pointer_access: bool
    subscripted: bool
    function_start: str
    source_path: str
    line: str


def _convention(member: str) -> str:
    name = member.removeprefix(MEMBER_PREFIX)
    for prefix in sorted(CONVENTION, key=len, reverse=True):
        if name.startswith(prefix) and len(name) > len(prefix) and name[len(prefix)].isupper():
            return CONVENTION[prefix]
    return ""


def member_references(repo: Path, programs: set[str] | None = None) -> list[MemberReference]:
    """Every `base.member` an assertion expression names, with its context."""

    path = repo / "evidence" / "snapshots" / "call-sites" / "assertions.csv"
    references: list[MemberReference] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if programs is not None and row["program"] not in programs:
                continue
            for match in _ACCESS.finditer(row["expression"]):
                owner = match.group("base")
                for step in _STEP.finditer(match.group("tail")):
                    references.append(
                        MemberReference(
                            program=row["program"],
                            base=match.group("base"),
                            owner=owner,
                            member=step.group("member"),
                            pointer_access=step.group("operator") == "->",
                            subscripted=bool(step.group("index")),
                            function_start=row["function_start"].strip().lower(),
                            source_path=row["source_path"],
                            line=row["line"],
                        )
                    )
                    owner = f"{owner}{'->' if step.group('operator') == '->' else '.'}"
                    owner = owner + step.group("member")
    return references


def aggregate_model(references: list[MemberReference]) -> dict[str, Any]:
    """Group references into one entry per aggregate expression.

    An aggregate is keyed by the expression that reaches it, so
    `pMonsterInfo->pCombat` is its own entry: what that pointer points at has
    members of its own, and only the chain says so.
    """

    members: dict[str, dict[str, dict[str, Any]]] = defaultdict(lambda: defaultdict(dict))
    pointer: dict[str, set[bool]] = defaultdict(set)
    for reference in references:
        pointer[reference.owner].add(reference.pointer_access)
        entry = members[reference.owner].setdefault(
            reference.member,
            {
                "member": reference.member,
                "convention": _convention(reference.member),
                "array": False,
                "functions": set(),
                "programs": set(),
                "units": set(),
            },
        )
        entry["array"] = entry["array"] or reference.subscripted
        if reference.function_start:
            entry["functions"].add(reference.function_start)
        entry["programs"].add(reference.program)
        if reference.source_path:
            entry["units"].add(reference.source_path)

    model: dict[str, Any] = {}
    for owner, entries in sorted(members.items()):
        model[owner] = {
            "accessed_through_pointer": sorted(pointer[owner]) == [True],
            "members": [
                {
                    **entry,
                    "functions": sorted(entry["functions"]),
                    "programs": sorted(entry["programs"]),
                    "units": sorted(entry["units"]),
                }
                for entry in sorted(entries.values(), key=lambda item: item["member"])
            ],
        }
    return model


def write_report(model: dict[str, Any], destination: Path) -> dict[str, Any]:
    """Emit the vocabulary as a reviewable table plus a summary."""

    destination.mkdir(parents=True, exist_ok=True)
    with (destination / "members.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=[
                "aggregate",
                "member",
                "convention",
                "array",
                "through_pointer",
                "functions",
                "programs",
                "units",
            ],
        )
        writer.writeheader()
        for aggregate, entry in sorted(model.items()):
            for member in entry["members"]:
                writer.writerow(
                    {
                        "aggregate": aggregate,
                        "member": member["member"],
                        "convention": member["convention"],
                        "array": "yes" if member["array"] else "",
                        "through_pointer": "yes" if entry["accessed_through_pointer"] else "",
                        "functions": " ".join(member["functions"]),
                        "programs": len(member["programs"]),
                        "units": " | ".join(member["units"]),
                    }
                )

    ranked = sorted(model.items(), key=lambda item: -len(item[1]["members"]))
    summary = {
        "aggregates": len(model),
        "members": sum(len(entry["members"]) for entry in model.values()),
        "largest": [
            {"aggregate": aggregate, "members": len(entry["members"])}
            for aggregate, entry in ranked[:10]
        ],
        "nested": len([aggregate for aggregate in model if "." in aggregate or "->" in aggregate]),
    }
    (destination / "summary.json").write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    return summary
