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

Offsets are deliberately absent from the vocabulary pass.  ``resolve_members``
obtains them separately from a Ghidra P-code slice of the branch controlling
the assertion call; rendered C is never an input to layout recovery.
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
    call_site: str
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
                            call_site=row["call_site"].strip().lower(),
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


UNIQUE = "unique"
SEVERAL = "several candidates"
QUERY_BATCH = 10


def resolve_members(
    settings: Any, program: str, references: list[MemberReference]
) -> list[dict[str, str]]:
    """Pair each member with the P-code load controlling its assertion.

    Assertion text supplies only the member vocabulary.  Storage comes from a
    backward slice of the controlling branch: exact absolute address or root
    offset, access width, extension and indexing stride.  Rendered C is not an
    input, so decompiler presentation changes cannot move a field.
    """

    from .ghidra.query_daemon import query_many

    wanted = [
        reference
        for reference in references
        if reference.program == program and reference.call_site
    ]
    # The snapshot derives its containing function from inter-function padding,
    # which proposes starts the program does not have; the program's own
    # containment is the authority and answers every site in one query.
    sites = sorted({reference.call_site for reference in wanted})
    containment, _ = query_many(settings, program, [("function-of", [",".join(sites)])])
    owner_of = (containment[0].get("result") or {}).get("functions", {})
    sliced: dict[str, dict[str, Any]] = {}
    resolved_sites = sorted(site for site in sites if owner_of.get(site))
    for start in range(0, len(resolved_sites), QUERY_BATCH):
        batch = resolved_sites[start : start + QUERY_BATCH]
        results, _ = query_many(
            settings, program, [("condition-accesses", [f"0x{site}"]) for site in batch]
        )
        for site, result in zip(batch, results, strict=True):
            sliced[site] = result.get("result") or {"accesses": []}

    members_per_site: dict[tuple[str, str, str], list[MemberReference]] = defaultdict(list)
    for reference in wanted:
        owner = owner_of.get(reference.call_site)
        if owner:
            members_per_site[(owner, reference.source_path, reference.line)].append(reference)

    rows: list[dict[str, str]] = []
    for (function, source_path, line), group in sorted(members_per_site.items()):
        site = group[0].call_site
        slice_result = sliced.get(site, {})
        accesses = (
            slice_result.get("accesses", [])
            if slice_result.get("confidence") == "exact-control-slice"
            else []
        )
        leaves = sorted(set(group), key=lambda item: item.member)
        for reference in leaves:
            if reference.pointer_access:
                relevant = [item for item in accesses if item.get("kind") == "root-relative"]
                rooted = list(
                    dict.fromkeys(
                        (item.get("root"), item.get("offset"))
                        for item in relevant
                        if item.get("root") and item.get("offset")
                    )
                )
                candidates = [f"{root}@{offset}" for root, offset in rooted]
            else:
                relevant = [item for item in accesses if item.get("kind") == "absolute"]
                candidates = list(
                    dict.fromkeys(item["storage"] for item in relevant if item.get("storage"))
                )
            basis = UNIQUE if len(candidates) == 1 and len(leaves) == 1 else SEVERAL
            widths = sorted({str(item["width"]) for item in relevant if item.get("width")})
            extensions = sorted(
                {str(item["extension"]) for item in relevant if item.get("extension")}
            )
            strides = sorted({str(item["stride"]) for item in relevant if item.get("stride")})
            rows.append(
                {
                    "aggregate": reference.owner,
                    "member": reference.member,
                    "storage": (
                        rooted[0][1]
                        if reference.pointer_access and basis == UNIQUE
                        else (candidates[0] if basis == UNIQUE else "")
                    ),
                    "kind": "offset" if reference.pointer_access else "global",
                    "candidates": " ".join(candidates),
                    "basis": basis if candidates else "no guarding condition found",
                    "function": function,
                    "source_path": source_path,
                    "line": line,
                    "access_widths": " ".join(widths),
                    "extensions": " ".join(extensions),
                    "index_strides": " ".join(strides),
                    "branch": str(sliced.get(site, {}).get("branch") or ""),
                }
            )
    return rows


def consensus(rows: list[dict[str, str]]) -> list[dict[str, Any]]:
    """One row per member, from every site that resolved it.

    A member read at several assertions should resolve to the same storage
    every time, and it is the repetition that makes the answer worth anything:
    one site is an inference, six agreeing sites is a measurement. A member
    whose sites disagree keeps both answers and says so.
    """

    grouped: dict[tuple[str, str], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        if row["basis"] == UNIQUE and row["storage"]:
            grouped[(row["aggregate"], row["member"])].append(row)

    resolved = []
    for (aggregate, member), group in sorted(grouped.items()):
        storages = sorted({row["storage"] for row in group})
        resolved.append(
            {
                "aggregate": aggregate,
                "member": member,
                "kind": group[0]["kind"],
                "storage": storages[0] if len(storages) == 1 else "",
                "candidates": " ".join(storages),
                "sites": len(group),
                "agreed": len(storages) == 1,
                "access_widths": " ".join(
                    sorted(
                        {value for row in group for value in row.get("access_widths", "").split()}
                    )
                ),
                "extensions": " ".join(
                    sorted({value for row in group for value in row.get("extensions", "").split()})
                ),
                "index_strides": " ".join(
                    sorted(
                        {value for row in group for value in row.get("index_strides", "").split()}
                    )
                ),
            }
        )
    return resolved


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
