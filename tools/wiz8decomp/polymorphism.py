"""Census the polymorphism ABI from relocations rather than by pattern search.

The loader's relocation table is the image's own complete index of every
absolute address it contains, so it bounds this scan exactly: a vtable is a run
of consecutive relocated slots that point into code, and nothing that is not a
relocated slot can be part of one. That is what makes the census exhaustive
instead of an open-ended search for pointer-shaped bytes.

The hard part is not finding runs but ending them. Distinct vftables can sit
adjacent in `.rdata`, so a naive maximal run merges them and reports one table
holding the sum of both slot counts. The boundary rule is that a table has to be
referred to to be used at all: any `.rdata` slot address that appears as a
relocated operand in code begins a table. Splitting there reproduces every
reviewed slot count that is independently correct, and shows that three reviewed
counts were each the sum of a table and the one following it.

Vptr stores are collected separately, but their memory displacement stays a raw
ABI observation. `mov [reg+N], offset table` proves only the displacement from
that register at that instruction. It does not prove a root-relative placement
unless register provenance establishes that the register still denotes the
complete object. These writes are evidence about the table, not the rule that
bounds it - an additional table whose store is not decodable still has to be
split off.

Slot kinds are read, not guessed. `_purecall` is resolved through the import
table by name, so pure-virtual slots need no hardcoded address, and a slot
pointing at a jump thunk is reported with the library method it stands for.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .binary.code import (
    disassembler,
    function_start,
    import_thunks,
    instruction_covering,
    relocation_sites,
)
from .binary.demangle import demangle
from .binary.image import PeImage
from .binary.inventory import is_first_party, representative_modules
from .class_candidates import allocation_size_before, allocation_size_hints
from .config import Settings
from .eh_metadata import import_slots
from .ghidra.project import program_name
from .reports.snapshots import csv_text, publish_report_snapshot

_SNAPSHOT_NAME = "polymorphism"
_REPORT_FILES = ("vtables.csv", "slots.csv", "vptr-writes.csv")
_PURECALL = "_purecall"
# MSVC's global scalar operator new; construction sites push the object size
# immediately before calling it (directly through its import thunk).
_OPERATOR_NEW = "??2@YAPAXI@Z"
# SurRender's heap, the other family a construction site allocates through.
_SRHEAP_ALLOCATE = "?allocate@srHeap@@QAEPAXK@Z"
# A vbtable holds small signed displacements, not addresses, so its entries are
# never relocated. The first is the offset back to the vbptr itself.
_VBTABLE_FIRST = {0, 0xFFFFFFFC}
_VBTABLE_LIMIT = 0x10000


@dataclass
class VptrWrite:
    site: int
    function: int | None
    store_displacement: int
    table: int
    allocation_size: int | None = None


@dataclass
class Slot:
    index: int
    target: int
    kind: str
    import_name: str = ""
    adjust: int | None = None
    thunk_target: int | None = None


@dataclass
class Vtable:
    address: int
    section: str
    kind: str
    slots: list[Slot] = field(default_factory=list)
    boundary: str = ""
    writes: list[VptrWrite] = field(default_factory=list)


def _data_sections(image: PeImage) -> list[Any]:
    """Sections a vptr may point into.

    Only `.rdata` is scanned for tables: MSVC emits vftables as const data, and
    the consecutive code pointers in `.data` are dispatch tables rather than
    vtables, which no constructor ever stores into an object.
    """
    return [section for section in image.sections if section.name == ".rdata"]


def _purecall_thunk(image: PeImage, thunks: dict[int, str]) -> int | None:
    for address, name in thunks.items():
        if name.split("!", 1)[-1] == _PURECALL:
            return address
    return None


def _adjustor(image: PeImage, engine: Any, target: int) -> tuple[int, int] | None:
    """Decode `sub ecx, N ; jmp real` - the thunk that shifts `this` to a base."""
    from capstone import CS_OP_IMM, CS_OP_REG
    from capstone.x86 import X86_REG_ECX

    chain = list(engine.disasm(image.read(target, 16), target))
    if len(chain) < 2:
        return None
    head, tail = chain[0], chain[1]
    if head.mnemonic not in {"sub", "add"} or len(head.operands) != 2:
        return None
    destination, amount = head.operands
    if destination.type != CS_OP_REG or destination.reg != X86_REG_ECX:
        return None
    if amount.type != CS_OP_IMM:
        return None
    if tail.mnemonic != "jmp" or tail.operands[0].type != CS_OP_IMM:
        return None
    delta = amount.imm if head.mnemonic == "sub" else -amount.imm
    return delta, tail.operands[0].imm


def _collect_vptr_writes(
    image: PeImage, engine: Any, sites: list[int], data_ranges: list[tuple[int, int]]
) -> list[VptrWrite]:
    """Relocated operands decoded as register-relative vftable stores."""
    from capstone import CS_OP_IMM, CS_OP_MEM

    text = image.text
    writes: list[VptrWrite] = []
    for site in sites:
        if not (text.virtual_address <= site < text.virtual_address + text.raw_size):
            continue
        value = image.read_u32(site)
        if value is None or not any(low <= value < high for low, high in data_ranges):
            continue
        instruction = instruction_covering(image, engine, site)
        if instruction is None or instruction.mnemonic != "mov":
            continue
        if len(instruction.operands) != 2:
            continue
        destination, source = instruction.operands
        if destination.type != CS_OP_MEM or source.type != CS_OP_IMM:
            continue
        if source.imm & 0xFFFFFFFF != value:
            continue
        # `mov [reg+N], imm` only; an absolute `mov [addr], imm` writes a global.
        if destination.mem.base == 0 or destination.mem.index != 0:
            continue
        writes.append(
            VptrWrite(
                site=instruction.address,
                function=function_start(image, instruction.address),
                store_displacement=destination.mem.disp,
                table=value,
            )
        )
    return writes


def _vbtables(image: PeImage, relocated: set[int], targets: set[int]) -> list[Vtable]:
    """Tables of base displacements rather than pointers.

    Virtual inheritance is expected to be rare, so each one found is worth
    recording explicitly instead of being silently dropped for not looking like
    a vftable.
    """
    found: list[Vtable] = []
    for address in sorted(targets):
        if address in relocated:
            continue
        section = image.section_at(address)
        if section is None or section.name not in {".rdata", ".data"}:
            continue
        first = image.read_u32(address)
        if first is None or first not in _VBTABLE_FIRST:
            continue
        slots: list[Slot] = []
        cursor = address
        while True:
            value = image.read_u32(cursor)
            if value is None or cursor in relocated:
                break
            signed = struct.unpack("<i", struct.pack("<I", value))[0]
            if slots and not 0 < signed < _VBTABLE_LIMIT:
                break
            if len(slots) > 64:
                break
            slots.append(Slot(index=len(slots), target=value, kind="base-displacement"))
            cursor += 4
        if len(slots) >= 2:
            found.append(
                Vtable(
                    address=address,
                    section=section.name,
                    kind="vbtable",
                    slots=slots,
                    boundary="displacement-shape",
                )
            )
    return found


def analyse_image(path: Path) -> dict[str, Any]:
    image = PeImage(path)
    engine = disassembler()
    slots = import_slots(path)
    thunks = import_thunks(image, slots)
    purecall = _purecall_thunk(image, thunks)
    sites = relocation_sites(image)
    relocated = set(sites)
    data_ranges = [
        (section.virtual_address, section.virtual_address + section.raw_size)
        for section in _data_sections(image)
    ]

    writes = _collect_vptr_writes(image, engine, sites, data_ranges)
    writes_by_table: dict[int, list[VptrWrite]] = {}
    for write in writes:
        writes_by_table.setdefault(write.table, []).append(write)

    # Every table address the code refers to at all, not only the ones stored by
    # a decodable `mov [reg+N], imm`. An additional table whose constructor store
    # is not decodable is still referenced, and without it the preceding table
    # absorbs the additional table's slots and reports the sum of both counts.
    text = image.text
    referenced: set[int] = set()
    for site in sites:
        if text.virtual_address <= site < text.virtual_address + text.raw_size:
            value = image.read_u32(site)
            if value is not None:
                referenced.add(value)
    starts = referenced | {write.table for write in writes}

    tables: list[Vtable] = []
    for section in _data_sections(image):
        low, high = section.virtual_address, section.virtual_address + section.raw_size
        in_section = [site for site in sites if low <= site < high]
        current: list[int] = []
        boundary = ""

        def flush(run: list[int], reason: str, section_name: str = section.name) -> None:
            # An isolated code pointer is a function pointer in some structure,
            # not a one-slot vtable, unless a constructor actually stores it.
            if not run or (len(run) < 2 and run[0] not in starts):
                return
            tables.append(
                Vtable(
                    address=run[0],
                    section=section_name,
                    kind="vftable",
                    slots=[
                        Slot(index=index, target=image.read_u32(address) or 0, kind="local")
                        for index, address in enumerate(run)
                    ],
                    boundary=reason,
                    writes=writes_by_table.get(run[0], []),
                )
            )

        for address in in_section:
            value = image.read_u32(address)
            is_slot = value is not None and image.is_code(value)
            if not is_slot:
                flush(current, boundary or "non-code-slot")
                current, boundary = [], ""
                continue
            if current and (address != current[-1] + 4 or address in starts):
                flush(current, "code-reference-boundary" if address in starts else "gap")
                current, boundary = [], ""
            current.append(address)
        flush(current, boundary or "section-end")

    for table in tables:
        for slot in table.slots:
            if purecall is not None and slot.target == purecall:
                slot.kind = "pure-virtual"
                continue
            name = thunks.get(slot.target)
            if name:
                slot.kind = "import-thunk"
                slot.import_name = name
                continue
            adjustor = _adjustor(image, engine, slot.target)
            if adjustor is not None:
                slot.kind = "adjustor-thunk"
                slot.adjust, slot.thunk_target = adjustor

    tables.extend(_vbtables(image, relocated, {write.table for write in writes}))
    tables.sort(key=lambda table: table.address)

    # Allocation-size hints: for each vftable, the push-immediates found before
    # operator-new calls at its zero-displacement writers' call sites. The slot 0 target
    # is the deleting destructor and never a construction entry, so it is
    # excluded from the scanned writers.
    allocators = {
        address for address, name in thunks.items() if name.split("!", 1)[-1] == _OPERATOR_NEW
    }
    # The second allocation family the image actually constructs objects with.
    # It is called straight through its import slot rather than a jump thunk,
    # so it needs the slot address, not a target.
    allocator_slots = {
        address for address, name in slots.items() if name.split("!", 1)[-1] == _SRHEAP_ALLOCATE
    }
    zero_displacement_writers_by_table: dict[int, set[int]] = {}
    for table in tables:
        if table.kind != "vftable":
            continue
        deleting = table.slots[0].target if table.slots else None
        zero_displacement_writers_by_table[table.address] = {
            write.function
            for write in table.writes
            if write.store_displacement == 0
            and write.function is not None
            and write.function != deleting
        }
    zero_displacement_writers = sorted(
        set().union(*zero_displacement_writers_by_table.values(), set())
    )
    hints = (
        allocation_size_hints(image, zero_displacement_writers, allocators, allocator_slots)
        if (allocators or allocator_slots) and zero_displacement_writers
        else {}
    )
    # The hints above are read at calls to a constructor, so they see nothing
    # when the constructor is inlined - which is how most of this image builds a
    # heap object. Reading back from the vptr store itself covers that shape,
    # and it is the only one available for a class whose construction never
    # becomes a call.
    if allocators or allocator_slots:
        for write in writes:
            if write.store_displacement == 0:
                write.allocation_size = allocation_size_before(
                    image, write.site, write.function, allocators, allocator_slots
                )
    sizes_by_table = {
        address: sorted(
            {size for writer in writers for size in hints.get(writer, [])}
            | {
                write.allocation_size
                for write in writes_by_table.get(address, [])
                if write.allocation_size is not None
            }
        )
        for address, writers in zero_displacement_writers_by_table.items()
    }
    return {
        "tables": tables,
        "writes": writes,
        "purecall": purecall,
        "allocation_sizes": sizes_by_table,
    }


def _hex(value: Any) -> str:
    return f"{value:08x}" if isinstance(value, int) else ""


def _observation_counts(
    table_rows: list[dict[str, Any]],
    slot_rows: list[dict[str, Any]],
    write_rows: list[dict[str, Any]],
) -> dict[str, int]:
    """Summarize raw observations without assigning source-level roles."""

    zero_displacement_tables = [
        row for row in table_rows if "0x0" in row["store_displacements"].split()
    ]
    return {
        "vtables": sum(1 for row in table_rows if row["kind"] == "vftable"),
        "vbtables": sum(1 for row in table_rows if row["kind"] == "vbtable"),
        "slots": len(slot_rows),
        "tables_with_a_decoded_vptr_write": sum(1 for row in table_rows if row["vptr_write_count"]),
        "tables_with_zero_store_displacement": len(zero_displacement_tables),
        "tables_with_only_nonzero_store_displacements": sum(
            1
            for row in table_rows
            if row["store_displacements"] and "0x0" not in row["store_displacements"].split()
        ),
        "pure_virtual_slots": sum(1 for row in slot_rows if row["kind"] == "pure-virtual"),
        "adjustor_thunk_slots": sum(1 for row in slot_rows if row["kind"] == "adjustor-thunk"),
        "import_slots": sum(1 for row in slot_rows if row["kind"] == "import-thunk"),
        "vptr_writes": len(write_rows),
    }


def _snapshot_readme() -> str:
    return """# Polymorphism-ABI snapshot

Every vtable, vtable slot and decoded vptr write in the first-party Wizardry executables whose
code is readable. Tracked because reproduction needs the proprietary binaries.

The producer is `wiz8decomp.polymorphism`. Normal runs write the same CSVs under
`build/reports/polymorphism/` and fail when they differ from this snapshot:

```sh
uv run wiz8 evidence refresh polymorphism                  # verify against the snapshot
uv run wiz8 evidence refresh polymorphism --update-snapshot
```

The scan is bounded by the image's own relocation table rather than by a byte pattern: a vtable is a
run of consecutive relocated slots pointing into code, and a value that is not a relocated slot
cannot belong to one.

`boundary` records why each table ended, which is the part that is easy to get wrong. Adjacent
tables merge into a single run unless something splits them, so a run is cut at any table address
referenced from code (`code-reference-boundary`). This remains valid even when the corresponding
constructor store cannot be decoded. Without that rule a table absorbs the next one and reports
the sum of both slot counts.

`vptr-writes.csv` records `store_displacement`, the raw displacement in the decoded memory operand.
It is not a root-relative object offset unless a separate receiver-provenance analysis establishes
the register's relation to the complete object. Zero and nonzero displacements therefore do not by
themselves classify primary tables, embedded polymorphic members, or secondary bases. A table
written at several displacements by several functions is normal.

`allocation_size` on a zero-displacement write is the size pushed to `operator new` just before it.
It is an allocation hint until receiver provenance associates the write with that allocation. It is
read back from the store rather than
from a call to a constructor, so an inlined construction has one too. It is empty when the object is
embedded, stack-placed, or - as the srMaterial builders do - allocated through a register holding the
allocator, where no size is visible at the site at all.

Slot `kind` is `pure-virtual` when the slot holds the `_purecall` thunk, resolved through the import
table by name rather than by a hardcoded address; `import-thunk` when the slot points at a jump
thunk, in which case `import_name` and `import_signature` name the library method the class
inherited; `adjustor-thunk` for the `sub ecx, N; jmp` entries that shift `this` onto a base, with the
adjustment and real target recorded; and `local` otherwise.

`kind` on a table is `vftable` or `vbtable`. A vbtable holds base displacements rather than
addresses, so its entries are never relocated and it is detected by shape.
"""


def sweep_polymorphism(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    chosen, aliases = representative_modules(settings, is_first_party)

    table_rows: list[dict[str, Any]] = []
    slot_rows: list[dict[str, Any]] = []
    write_rows: list[dict[str, Any]] = []
    per_program: dict[str, dict[str, int]] = {}
    skipped: list[str] = []

    for module in chosen:
        program = program_name(module)
        path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not path.is_file():
            raise RuntimeError(f"module payload is missing: {path}")
        if module.get("packed"):
            # The protected retail build's code section is not plain text, so no
            # constructor store can be decoded and the boundary rule has nothing
            # to work with. The inventory already establishes this; record it
            # rather than emitting unbounded runs.
            skipped.append(program)
            continue
        result = analyse_image(path)
        tables, writes = result["tables"], result["writes"]
        allocation_sizes = result["allocation_sizes"]
        per_program[program] = {
            "vtables": sum(1 for table in tables if table.kind == "vftable"),
            "vbtables": sum(1 for table in tables if table.kind == "vbtable"),
            "slots": sum(len(table.slots) for table in tables),
            "vptr_writes": len(writes),
        }
        for table in tables:
            displacements = sorted({write.store_displacement for write in table.writes})
            table_rows.append(
                {
                    "program": program,
                    "address": _hex(table.address),
                    "section": table.section,
                    "kind": table.kind,
                    "slot_count": len(table.slots),
                    "boundary": table.boundary,
                    "vptr_write_count": len(table.writes),
                    "store_displacements": " ".join(
                        f"-0x{-displacement:x}" if displacement < 0 else f"0x{displacement:x}"
                        for displacement in displacements
                    ),
                    "pure_virtual_slots": sum(
                        1 for slot in table.slots if slot.kind == "pure-virtual"
                    ),
                    "adjustor_thunk_slots": sum(
                        1 for slot in table.slots if slot.kind == "adjustor-thunk"
                    ),
                    "import_slots": sum(1 for slot in table.slots if slot.kind == "import-thunk"),
                    "allocation_sizes": "|".join(
                        f"0x{size:x}" for size in allocation_sizes.get(table.address, [])
                    ),
                }
            )
            for slot in table.slots:
                slot_rows.append(
                    {
                        "program": program,
                        "vtable": _hex(table.address),
                        "slot_index": slot.index,
                        "target": _hex(slot.target),
                        "kind": slot.kind,
                        "import_name": slot.import_name,
                        "import_signature": "",
                        "adjust": "" if slot.adjust is None else slot.adjust,
                        "thunk_target": _hex(slot.thunk_target),
                    }
                )
        for write in writes:
            write_rows.append(
                {
                    "program": program,
                    "site": _hex(write.site),
                    "function_start": _hex(write.function),
                    "store_displacement": (
                        f"-0x{-write.store_displacement:x}"
                        if write.store_displacement < 0
                        else f"0x{write.store_displacement:x}"
                    ),
                    "vtable": _hex(write.table),
                    "allocation_size": (
                        f"0x{write.allocation_size:x}" if write.allocation_size is not None else ""
                    ),
                }
            )

    signatures = demangle([row["import_name"].split("!", 1)[-1] for row in slot_rows])
    for row in slot_rows:
        row["import_signature"] = signatures.get(row["import_name"].split("!", 1)[-1], "")

    table_rows.sort(key=lambda row: (row["program"], row["address"]))
    slot_rows.sort(key=lambda row: (row["program"], row["vtable"], row["slot_index"]))
    write_rows.sort(key=lambda row: (row["program"], row["site"]))

    outputs = {
        "vtables.csv": csv_text(
            [
                "program",
                "address",
                "section",
                "kind",
                "slot_count",
                "boundary",
                "vptr_write_count",
                "store_displacements",
                "pure_virtual_slots",
                "adjustor_thunk_slots",
                "import_slots",
                "allocation_sizes",
            ],
            table_rows,
        ),
        "slots.csv": csv_text(
            [
                "program",
                "vtable",
                "slot_index",
                "target",
                "kind",
                "import_name",
                "import_signature",
                "adjust",
                "thunk_target",
            ],
            slot_rows,
        ),
        "vptr-writes.csv": csv_text(
            [
                "program",
                "site",
                "function_start",
                "store_displacement",
                "vtable",
                "allocation_size",
            ],
            write_rows,
        ),
    }

    report_dir, snapshot_dir, snapshot_fresh = publish_report_snapshot(
        settings,
        name=_SNAPSHOT_NAME,
        outputs=outputs,
        snapshot_files=_REPORT_FILES,
        snapshot_readme=_snapshot_readme(),
        update_snapshot=update_snapshot,
        stale_error=(
            "polymorphism report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        ),
    )

    return {
        "schema": "wiz8.polymorphism",
        "programs": per_program,
        "byte_identical_aliases": aliases,
        "programs_without_readable_code": skipped,
        **_observation_counts(table_rows, slot_rows, write_rows),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
