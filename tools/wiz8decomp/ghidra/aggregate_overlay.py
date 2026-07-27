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
from .overlay import _overlay_settings, _scratch_dir
from .project import resolve_program_name

CATEGORY = "/wiz8/overlay"
# The suffix says the structure is the members that were placed, not the block
# they belong to: `gXStatus_members` cannot be mistaken for `gXStatus`.
SPAN_SUFFIX = "_members"


def _rows(repo: Path, storage: Path | None) -> list[dict[str, str]]:
    path = storage or (repo / "build" / "reports" / "aggregates" / "storage.csv")
    if not path.is_file():
        raise ValueError(
            f"no resolved aggregate storage at {path}; run `wiz8 report aggregates --resolve` first"
        )
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


def apply_aggregates(
    settings: Settings,
    selector: str,
    hypothesis: str,
    storage: Path | None = None,
) -> dict[str, Any]:
    """Place the resolved aggregates over the clone's memory."""

    from .cache import materialize_program
    from .environment import start_pyghidra

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, hypothesis)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay does not exist; create it first: {hypothesis}")
    rows = _rows(settings.repo_dir, storage)
    placements = plan_placements(rows)
    types = plan_types(rows)

    start_pyghidra(settings)
    import pyghidra
    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        StructureDataType,
        Undefined1DataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    report: dict[str, Any] = {"placed": [], "types": [], "skipped": []}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("aggregate overlay")
            try:
                dtm = program.getDataTypeManager()
                listing = program.getListing()
                space = program.getAddressFactory().getDefaultAddressSpace()
                category = CategoryPath(CATEGORY)

                symbols = program.getSymbolTable()
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
                    for address, _name in resolved:
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
                    # A placement is a blunt instrument while the block's real
                    # bounds are unknown: everything already defined inside the
                    # span is cleared to make room, including definitions that
                    # belong to other globals. The count is reported so the
                    # cost of a sparse aggregate is visible rather than silent.
                    displaced = 0
                    walker = listing.getData(start, True)
                    while walker.hasNext():
                        datum = walker.next()
                        if datum.getAddress().compareTo(end) > 0:
                            break
                        displaced += 1
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
                            "displaced_definitions": displaced,
                            "bounds": "the placed members only; the block's own start is unknown",
                        }
                    )

                for aggregate, members in sorted(types.items()):
                    extent = members[-1][0] + 4
                    structure = StructureDataType(
                        category, aggregate.replace("->", "_").replace(".", "_"), extent
                    )
                    for offset, member in members:
                        structure.replaceAtOffset(
                            offset,
                            Undefined1DataType.dataType,
                            1,
                            member,
                            "recovered from assertion text",
                        )
                    dtm.addDataType(structure, DataTypeConflictHandler.REPLACE_HANDLER)
                    report["types"].append(
                        {
                            "aggregate": aggregate,
                            "members": len(members),
                            "highest_offset": f"0x{members[-1][0]:x}",
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
