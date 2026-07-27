"""Candidate class derivation from the polymorphism snapshots.

Pure logic shared by the class-candidates report and the candidate replay.
It lives under ``ghidra/`` because replay behavior must be reproducible from
the materialization key, which hashes this directory and the tracked
evidence; nothing here reads the proprietary image.

Candidates are machine observations, never reviewed conclusions: the replay
materializes them under candidate-marked names and categories, and promotion
into ``evidence/reviewed/wiz8/`` always passes through a human review of the
writers' decompiles.
"""

from __future__ import annotations

from collections import defaultdict
from typing import Any


def candidate_name(vtable: int) -> str:
    return f"Candidate_{vtable:08x}"


def classify_candidates(
    vtables: list[dict[str, str]],
    slots: list[dict[str, str]],
    writes: list[dict[str, str]],
    reviewed_vtables: set[int],
    resolve_function: Any = None,
) -> list[dict[str, Any]]:
    """One candidate per constructor-written vftable, writers classified.

    A writer that is also the vtable's slot 0 target is MSVC's scalar
    deleting destructor writing the vtable during destruction; every other
    writer is a constructor or the complete destructor, which this evidence
    alone cannot separate. Vtables the same writers install at non-zero
    object offsets are that candidate's subobject tables.

    The census derives each write's owning function from inter-function
    padding, which merges two adjacent bodies when no padding run separates
    them - the complete destructor immediately after its scalar deleting
    destructor is the common case. ``resolve_function`` lets a caller that
    has authoritative containment (the replay, inside Ghidra) map a write
    site to its real function instead; sites it cannot place keep the
    census attribution.
    """

    slot0: dict[int, int] = {}
    for row in slots:
        if row["slot_index"] == "0" and row["target"]:
            slot0[int(row["vtable"], 16)] = int(row["target"], 16)

    writers_by_vtable: dict[int, list[dict[str, str]]] = defaultdict(list)
    vtables_by_writer: dict[int, set[tuple[int, int]]] = defaultdict(set)
    for row in writes:
        vtable = int(row["vtable"], 16)
        writer: int | None = None
        if resolve_function is not None:
            writer = resolve_function(int(row["site"], 16))
        if writer is None and row["function_start"]:
            writer = int(row["function_start"], 16)
        if writer is None:
            continue
        writers_by_vtable[vtable].append({**row, "function_start": f"{writer:08x}"})
        vtables_by_writer[writer].add((vtable, int(row["object_offset"], 0)))

    candidates: list[dict[str, Any]] = []
    for row in vtables:
        if row["kind"] != "vftable":
            continue
        vtable = int(row["address"], 16)
        write_rows = writers_by_vtable.get(vtable, [])
        primary_writers = sorted(
            {
                int(item["function_start"], 16)
                for item in write_rows
                if int(item["object_offset"], 0) == 0
            }
        )
        if not primary_writers:
            continue
        deleting = slot0.get(vtable)
        constructors = [writer for writer in primary_writers if writer != deleting]
        co_installed = sorted(
            {
                (other, offset)
                for writer in primary_writers
                for other, offset in vtables_by_writer[writer]
                if other != vtable and offset != 0
            }
        )
        # A subobject table is installed by every constructor of its owner
        # (and restored by the destructor), while a heap-builder function
        # writes unrelated objects' vtables that only it touches. Requiring
        # unanimity across the constructor writers separates the two; with
        # only the deleting destructor available its writes stand alone.
        deciders = constructors or primary_writers
        subobjects = sorted(
            {
                (other, offset)
                for other, offset in co_installed
                if offset > 0
                and all(
                    (other, offset) in vtables_by_writer[writer]
                    for writer in deciders
                )
            }
        )
        candidates.append(
            {
                "vtable": vtable,
                "section": row["section"],
                "slot_count": int(row["slot_count"] or 0),
                "pure_virtual_slots": int(row["pure_virtual_slots"] or 0),
                "import_slots": int(row["import_slots"] or 0),
                "subobject_offsets": row["subobject_offsets"],
                "allocation_sizes": sorted(
                    int(value, 16)
                    for value in (row.get("allocation_sizes") or "").split("|")
                    if value
                ),
                # MSVC places the scalar deleting destructor in slot 0 of a
                # class with a virtual destructor, but slot 0 is an ordinary
                # virtual otherwise - so the strict claim needs the slot 0
                # target to write the vtable itself, and the bare target is
                # recorded separately as the hedged pointer for review.
                "slot0_target": deleting,
                "scalar_deleting_destructor": (
                    deleting if deleting in primary_writers else None
                ),
                "constructor_or_destructor": constructors,
                "co_installed_vtables": co_installed,
                "subobject_vtables": subobjects,
                "write_sites": sorted(item["site"] for item in write_rows),
                "reviewed": vtable in reviewed_vtables,
            }
        )
    return candidates


def derive_skeletons(candidates: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Struct specs for the unreviewed candidates.

    A skeleton has the primary vptr at 0, one vptr per co-installed vtable at
    its non-negative object offset, and a size that covers the largest
    allocation hint and every vptr. Reviewed candidates are skipped - the
    reviewed class model owns their structures.
    """

    skeletons: list[dict[str, Any]] = []
    for candidate in candidates:
        if candidate["reviewed"]:
            continue
        vptr_offsets: list[tuple[int, int]] = [(0, candidate["vtable"])]
        vptr_offsets.extend(
            (offset, vtable) for vtable, offset in candidate["subobject_vtables"]
        )
        vptr_offsets.sort()
        minimum = vptr_offsets[-1][0] + 4
        hints = candidate["allocation_sizes"]
        size = max([minimum] + [hint for hint in hints if hint >= minimum])
        skeletons.append(
            {
                "name": candidate_name(candidate["vtable"]),
                "vtable": candidate["vtable"],
                "size": size,
                "size_is_allocation_hint": bool(hints) and size in hints,
                "vptr_offsets": vptr_offsets,
            }
        )
    return skeletons


def allocation_size_before(
    image: Any,
    site: int,
    function: int | None,
    allocators: set[int],
    slots: set[int] | None = None,
    window: int = 64,
) -> int | None:
    """The object size allocated just before a vptr store, if there was one.

    `allocation_size_hints` reads the same `push size; call operator new` shape
    at a *call* to a constructor, so it sees nothing once the constructor is
    inlined - which is what most heap construction in this image looks like.
    Reading back from the store covers that, and the scan stops at the enclosing
    function so it cannot borrow an allocation from the body above.
    """

    text = image.text
    start = text.raw_offset
    offset = start + (site - text.virtual_address)
    if not (start <= offset < start + text.raw_size):
        return None
    lowest = offset - window
    if function is not None:
        lowest = max(lowest, start + (function - text.virtual_address))
    if lowest >= offset:
        return None
    return push_before_allocator(
        image.data[lowest:offset], site - (offset - lowest), allocators, slots
    )


def allocation_size_hints(
    image: Any, writers: list[int], allocators: set[int], slots: set[int] | None = None
) -> dict[int, list[int]]:
    """Push-immediate-before-new hints for each writer's call sites.

    The MSVC shape is ``push size; call operator new; ...; call ctor``. For
    every direct E8 call to a writer, the preceding 48 bytes are scanned for
    a push-immediate followed by a call into a known allocator; the pushed
    immediate is that construction site's allocation size. Absence of a hint
    means the object is embedded, stack-placed, or reached indirectly.
    """

    text = image.text
    data = image.data
    sizes: dict[int, set[int]] = defaultdict(set)
    writer_set = set(writers)
    start = text.raw_offset
    end = text.raw_offset + text.raw_size
    offset = start
    while True:
        offset = data.find(b"\xe8", offset, end)
        if offset < 0:
            break
        site = text.virtual_address + (offset - text.raw_offset)
        target = (
            site + 5 + int.from_bytes(data[offset + 1 : offset + 5], "little", signed=True)
        ) & 0xFFFFFFFF
        if target in writer_set:
            window = data[max(start, offset - 48) : offset]
            hint = push_before_allocator(window, site - len(window), allocators, slots)
            if hint is not None:
                sizes[target].add(hint)
        offset += 1
    return {writer: sorted(values) for writer, values in sizes.items()}


def push_before_allocator(
    window: bytes, window_va: int, allocators: set[int], slots: set[int] | None = None
) -> int | None:
    """The last push-immediate whose next call lands in an allocator.

    Two call forms reach one: `call rel32` into an allocator's jump thunk, which
    is how the global `operator new` is reached, and `call dword ptr [slot]`
    straight through an import slot, which is how `srHeap::allocate` is. Only
    the first was recognised, so every class allocated through the SurRender
    heap looked like one with no size at all. `slots` holds the import-slot
    addresses of the indirect form.
    """

    slots = slots or set()
    best: int | None = None
    index = 0
    while index < len(window):
        byte = window[index]
        pushed: int | None = None
        after = index
        if byte == 0x68 and index + 5 <= len(window):
            pushed = int.from_bytes(window[index + 1 : index + 5], "little")
            after = index + 5
        elif byte == 0x6A and index + 2 <= len(window):
            pushed = window[index + 1]
            after = index + 2
        if pushed is not None and after < len(window):
            if window[after] == 0xE8 and after + 5 <= len(window):
                rel = int.from_bytes(window[after + 1 : after + 5], "little", signed=True)
                target = (window_va + after + 5 + rel) & 0xFFFFFFFF
                if target in allocators:
                    best = pushed
            elif (
                window[after : after + 2] == b"\xff\x15"
                and after + 6 <= len(window)
                and int.from_bytes(window[after + 2 : after + 6], "little") in slots
            ):
                best = pushed
        index += 1
    return best


def derived_families(
    writes: list[dict[str, str]],
    slot_counts: dict[int, int],
    function_sizes: dict[int, int] | None = None,
) -> list[dict[str, Any]]:
    """Pairs of one-slot tables one writer installs at offset zero, in order.

    Two such tables in a single body are a base and a class derived from it,
    stored in construction order - so the table written at the *higher* site
    address is the derived class. This is the only place the derivation shows:
    an empty derived destructor stores the base table rather than its own,
    because its store is dead against the inlined base destructor and VC6 drops
    it, so a hierarchy counted from destructors undercounts it.

    The ranking matters more than the pairing. A writer that installs two tables
    and little else is a dedicated constructor and ports in one sitting; a
    thousand-byte body that happens to build two vectors on its way through is a
    heap builder, and the pair is then a fact about its locals rather than a
    class waiting to be recovered. ``writer_size`` carries that distinction and
    orders the result, and is None when the census cannot size the body.
    """

    sizes = function_sizes or {}
    by_writer: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for row in writes:
        if row.get("object_offset") not in ("0x0", "0"):
            continue
        if not row.get("function_start"):
            continue
        vtable = int(row["vtable"], 16)
        if slot_counts.get(vtable) != 1:
            continue
        by_writer[int(row["function_start"], 16)].append((int(row["site"], 16), vtable))

    families: list[dict[str, Any]] = []
    for writer, entries in by_writer.items():
        tables = list(dict.fromkeys(vtable for _, vtable in sorted(entries)))
        if len(tables) != 2:
            continue
        families.append(
            {
                "writer": writer,
                "base_vtable": tables[0],
                "derived_vtable": tables[1],
                "writer_size": sizes.get(writer),
            }
        )
    families.sort(key=lambda item: (item["writer_size"] is None, item["writer_size"] or 0))
    return families
