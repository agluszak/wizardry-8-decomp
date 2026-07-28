"""Lay the recovered aggregates over program memory, in an overlay clone.

The member names and their storage are recovered elsewhere; this is the step
that makes them visible. Until the block has a type, a function that touches
five members of `gXStatus` decompiles as five unrelated `DAT_00683f..` reads
and nothing connects them. Placing a structure over the same bytes turns each
one into a named field of one object, which is both easier to read and a
falsifiable claim: a field that renders in the middle of another field's span
is a layout error the decompiler will now show.

Two kinds of aggregate need two applications. A global object is a *placement*
- a structure defined over its own bytes, so accesses to those addresses render
as its fields. A pointer's target is a *type* with no address of its own, so it
is created in the type manager where the pointer's declared type can reach it,
and this module stops there rather than guessing which variables point at it.

Every width is measured, never assumed. A member's size comes from what the
program already records at that address - Ghidra sizes an undefined datum from
the width of the accesses that reach it - and is then capped by the distance to
the next member, because a member cannot overlap its successor. Where nothing
is recorded the member gets one undefined byte and stays deliberately small: an
under-sized field leaves honest undefined bytes beside it, while an over-sized
one swallows its neighbour and hides the evidence that would correct it.

The block's own bounds are a separate question from its members', and this does
not answer it. A contiguous clear would - `memset(&block, 0, size)` names both
ends - and the first version of this module went looking for one: a constant
pushed near a call whose range covered the members. It found ranges, and they
were wrong. `gEl01` and `gEl02` came back with the same base, which two distinct
objects cannot have, because a large scalar near a call in this program is far
more often a pointer or an unrelated count than a size. A wrong base is not a
small error: every offset in the model shifts by whatever it was wrong by, and
the structure would still look plausible. So the search is gone rather than
merely tightened, and each structure spans exactly the members evidence placed
and says so in its name. What the block begins at stays an open question with
nothing pretending to answer it.
"""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..config import Settings
from .candidate_facts import record_contradiction, stamp, upsert_fact
from .overlay import _overlay_settings, _scratch_dir
from .project import resolve_program_name

CATEGORY = "/wiz8/overlay"
# The suffix says the structure is the members that were placed, not the block
# they belong to: `gXStatus_members` cannot be mistaken for `gXStatus`.
SPAN_SUFFIX = "_members"


def _rows(repo: Path, storage: Path | None) -> list[dict[str, str]]:
    if storage is None:
        raise ValueError("an explicit exported review CSV path is required")
    path = storage
    if not path.is_file():
        raise ValueError(f"aggregate review CSV does not exist: {path}")
    with path.open(newline="", encoding="utf-8") as stream:
        return [row for row in csv.DictReader(stream) if row["storage"]]


def plan_placements(rows: list[dict[str, str]]) -> dict[str, list[tuple[str, str]]]:
    """Global-storage members grouped by aggregate, as `(storage, member)`.

    Only members whose sites agreed contribute, and only aggregates with more
    than one - a single named address is already a named address, and wrapping
    it in a one-field structure asserts a block that nothing has shown. The
    storage stays a name here: a `DAT_` carries its address in its spelling but
    a reviewed name does not, and only the program can say where that one sits.
    """

    grouped: dict[str, list[tuple[str, str]]] = {}
    for row in rows:
        if row["kind"] != "global" or row["agreed"] != "True":
            continue
        grouped.setdefault(row["aggregate"], []).append((row["storage"], row["member"]))
    return {
        aggregate: sorted(members) for aggregate, members in grouped.items() if len(members) > 1
    }


def address_in_name(storage: str) -> int | None:
    """The address a Ghidra default name spells, or None for a real name."""

    marker = storage.rfind("_")
    digits = storage[marker + 1 :]
    if marker < 0 or len(digits) < 6 or any(digit not in "0123456789abcdef" for digit in digits):
        return None
    return int(digits, 16)


def plan_types(rows: list[dict[str, str]]) -> dict[str, list[tuple[int, str]]]:
    """Pointer-reached members grouped by aggregate, as `(offset, member)`."""

    grouped: dict[str, list[tuple[int, str]]] = {}
    for row in rows:
        if row["kind"] != "offset" or row["agreed"] != "True":
            continue
        grouped.setdefault(row["aggregate"], []).append((int(row["storage"], 16), row["member"]))
    return {
        aggregate: sorted(members) for aggregate, members in grouped.items() if len(members) > 1
    }


def component_widths(
    members: list[tuple[int, str]], observed: dict[int, int], default: int = 1
) -> list[tuple[int, str, int]]:
    """`(offset, member, width)`, each width capped by the next member's start.

    The cap is what keeps a measured width honest: an access four bytes wide at
    an address two bytes before the next member says the access is wider than
    the field, not that the field overlaps its neighbour.
    """

    placed = [member for member in members if member[0] >= 0]
    sized: list[tuple[int, str, int]] = []
    for index, (address, name) in enumerate(placed):
        room = placed[index + 1][0] - address if index + 1 < len(placed) else None
        width = observed.get(address, default) or default
        if room is not None:
            width = min(width, room)
        sized.append((address, name, max(1, width)))
    return sized


def _derive_rows(
    program: Any,
    repo: Path,
    program_name: str,
    scopes: dict[str, int],
) -> list[dict[str, str]]:
    """Resolve explicitly scoped assertion members directly from ProgramDB."""

    from collections import defaultdict

    from ..aggregates import member_references
    from .semantic import condition_accesses

    references = [
        reference
        for reference in member_references(repo, {program_name})
        if reference.owner in scopes and reference.call_site
    ]
    by_site: dict[str, list[Any]] = defaultdict(list)
    for reference in references:
        by_site[reference.call_site].append(reference)
    observations: dict[tuple[str, str, str], list[dict[str, Any]]] = defaultdict(list)
    for site, group in sorted(by_site.items()):
        try:
            sliced = condition_accesses(program, site)
        except (RuntimeError, ValueError):
            continue
        if sliced.get("confidence") != "exact-control-slice":
            continue
        # Several vocabulary leaves at one assertion cannot be paired to loads
        # without guessing. Keep the slice reviewable, but place none of them.
        if len(group) != 1:
            continue
        reference = group[0]
        if reference.pointer_access:
            candidates = [
                item
                for item in sliced["accesses"]
                if item.get("kind") == "root-relative"
            ]
            keys = {(item.get("root"), item.get("offset")) for item in candidates}
            if len(keys) != 1:
                continue
            root, storage = next(iter(keys))
            kind = "offset"
        else:
            candidates = [
                item for item in sliced["accesses"] if item.get("kind") == "absolute"
            ]
            keys = {item.get("storage") for item in candidates if item.get("storage")}
            if len(keys) != 1:
                continue
            root, storage = None, next(iter(keys))
            kind = "global"
        observations[(reference.owner, reference.member, kind)].append(
            {
                "storage": storage,
                "root": root,
                "widths": {str(item["width"]) for item in candidates if item.get("width")},
            }
        )

    rows: list[dict[str, str]] = []
    for (aggregate, member, kind), items in sorted(observations.items()):
        identities = {(item["root"], item["storage"]) for item in items}
        minimum = scopes[aggregate]
        agreed = len(identities) == 1 and len(items) >= minimum
        root, storage = next(iter(identities)) if len(identities) == 1 else (None, "")
        rows.append(
            {
                "aggregate": aggregate,
                "member": member,
                "kind": kind,
                "storage": str(storage) if agreed else "",
                "root": str(root or ""),
                "sites": str(len(items)),
                "agreed": str(agreed),
                "access_widths": " ".join(
                    sorted({width for item in items for width in item["widths"]})
                ),
            }
        )
    return rows


def _placement_conflicts(
    listing: Any,
    start: Any,
    end: Any,
    members: list[tuple[int, str, int]],
) -> list[dict[str, Any]]:
    """Defined data in a candidate span that the member proof does not own."""

    ranges = [(address, address + width) for address, _name, width in members]
    conflicts = []
    walker = listing.getData(start, True)
    while walker.hasNext():
        datum = walker.next()
        if datum.getAddress().compareTo(end) > 0:
            break
        address = int(datum.getAddress().getOffset())
        data_end = address + int(datum.getLength())
        display = str(datum.getDataType().getDisplayName()).lower()
        member_owned = any(address >= first and data_end <= last for first, last in ranges)
        if member_owned or display.startswith("undefined"):
            continue
        conflicts.append(
            {
                "address": str(datum.getAddress()),
                "type": datum.getDataType().getDisplayName(),
                "length": datum.getLength(),
            }
        )
    return conflicts


def apply_aggregates(
    settings: Settings,
    selector: str,
    overlay_id: str,
    storage: Path | None = None,
    *,
    aggregates: list[str] | None = None,
    aggregate_seeds: list[dict[str, Any]] | None = None,
    hypothesis: str | None = None,
) -> dict[str, Any]:
    """Place the resolved aggregates over the clone's memory."""

    from .cache import materialize_program
    from .environment import start_pyghidra

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, overlay_id)
    if not _scratch_dir(effective, overlay_id).exists():
        raise ValueError(f"overlay does not exist; create it first: {overlay_id}")
    fact_hypothesis = hypothesis or overlay_id

    start_pyghidra(settings)
    import pyghidra
    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    report: dict[str, Any] = {"placed": [], "types": [], "skipped": []}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            if storage is not None:
                rows = _rows(settings.repo_dir, storage)
                wanted = set(aggregates or [])
                if wanted:
                    rows = [row for row in rows if row["aggregate"] in wanted]
            else:
                scopes = {
                    seed["name"]: int(seed.get("minimum_agreeing_sites", 2))
                    for seed in (aggregate_seeds or [])
                }
                if not scopes:
                    raise ValueError("aggregate inference requires explicit aggregate seeds")
                rows = _derive_rows(program, settings.repo_dir, program_name, scopes)
            placements = plan_placements(rows)
            types = plan_types(rows)
            transaction = program.startTransaction("aggregate overlay")
            try:
                dtm = program.getDataTypeManager()
                listing = program.getListing()
                space = program.getAddressFactory().getDefaultAddressSpace()
                category = CategoryPath(CATEGORY)

                symbols = program.getSymbolTable()
                row_by_member = {(row["aggregate"], row["member"]): row for row in rows}
                for aggregate, members in sorted(placements.items()):
                    resolved: list[tuple[int, str]] = []
                    for named, member in members:
                        address = address_in_name(named)
                        if address is None:
                            found = list(symbols.getGlobalSymbols(named))
                            if not found:
                                report["skipped"].append(
                                    {"aggregate": aggregate, "member": member, "storage": named}
                                )
                                continue
                            address = int(found[0].getAddress().getOffset())
                        resolved.append((address, member))
                    resolved.sort()
                    observed = {}
                    for address, member_name in resolved:
                        measured = (
                            row_by_member.get((aggregate, member_name), {})
                            .get("access_widths", "")
                            .split()
                        )
                        if len(measured) == 1:
                            observed[address] = int(measured[0])
                            continue
                        datum = listing.getDataAt(space.getAddress(f"{address:08x}"))
                        if datum is not None:
                            observed[address] = int(datum.getLength())
                    sized = component_widths(resolved, observed)
                    if len(sized) < 2:
                        continue
                    base = sized[0][0]
                    extent = sized[-1][0] + sized[-1][2] - base
                    name = f"{aggregate}{SPAN_SUFFIX}"
                    overlap = next(
                        (
                            other
                            for other in report["placed"]
                            if int(other["base"], 16) < base + extent
                            and base < int(other["base"], 16) + other["bytes"]
                        ),
                        None,
                    )
                    if overlap is not None:
                        # Two aggregates cannot share bytes. One of the two
                        # resolutions is wrong and this cannot say which.
                        report["skipped"].append(
                            {
                                "aggregate": aggregate,
                                "reason": f"span overlaps {overlap['structure']}",
                            }
                        )
                        continue

                    structure = StructureDataType(category, name, extent)
                    for address, member, width in sized:
                        structure.replaceAtOffset(
                            address - base,
                            _sized_type(width),
                            width,
                            member,
                            "recovered from assertion text",
                        )
                    applied = dtm.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)
                    start = space.getAddress(f"{base:08x}")
                    end = start.add(extent - 1)
                    conflicts = _placement_conflicts(listing, start, end, sized)
                    fact_id = f"aggregate:{aggregate}"
                    payload = {
                        "structure": name,
                        "members": [
                            {"name": member, "offset": f"0x{address - base:x}", "width": width}
                            for address, member, width in sized
                        ],
                        "bounds": "placed members only",
                    }
                    if conflicts:
                        upsert_fact(
                            program,
                            start,
                            fact_id=fact_id,
                            hypothesis=fact_hypothesis,
                            kind="aggregate",
                            depends_on=["pcode:exact-control-slices"],
                            payload=payload,
                        )
                        record_contradiction(
                            program,
                            start,
                            fact_id,
                            reason="candidate span contains unrelated defined data",
                            incoming={"conflicts": conflicts},
                        )
                        report["skipped"].append(
                            {
                                "aggregate": aggregate,
                                "reason": "defined data conflicts with sparse candidate span",
                                "conflicts": conflicts,
                            }
                        )
                        continue
                    listing.clearCodeUnits(start, end, False)
                    listing.createData(start, applied)
                    # The block's label has to be the primary one at its base.
                    # A member's own reviewed name usually already sits there,
                    # and leaving that primary renders every access to the
                    # block as `g_item_record_count.uiMonstersInDatabase` - a
                    # sibling member standing in for the object. The reviewed
                    # name is kept, just no longer first.
                    label = symbols.createLabel(start, name, SourceType.ANALYSIS)
                    label.setPrimary()
                    report["placed"].append(
                        {
                            "aggregate": aggregate,
                            "structure": name,
                            "base": f"{base:08x}",
                            "bytes": extent,
                            "members": len(sized),
                            "displaced_definitions": 0,
                            "bounds": "the placed members only; the block's own start is unknown",
                        }
                    )
                    stamp(
                        program,
                        start,
                        hypothesis=fact_hypothesis,
                        fact_id=fact_id,
                        depends_on=[
                            "pcode:assertion-control-slices",
                            "aggregate-member-vocabulary",
                        ],
                        constraints=payload,
                    )

                for aggregate, members in sorted(types.items()):
                    report["skipped"].append(
                        {
                            "aggregate": aggregate,
                            "reason": "pointer base is not identified; no unbound type created",
                            "candidate_members": len(members),
                        }
                    )
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            program.save("aggregate overlay", None)
    finally:
        project.close()
    return report


def _sized_type(width: int) -> Any:
    from ghidra.program.model.data import (
        Undefined1DataType,
        Undefined2DataType,
        Undefined4DataType,
        Undefined8DataType,
    )

    return {
        1: Undefined1DataType.dataType,
        2: Undefined2DataType.dataType,
        4: Undefined4DataType.dataType,
        8: Undefined8DataType.dataType,
    }.get(width, Undefined1DataType.dataType)
