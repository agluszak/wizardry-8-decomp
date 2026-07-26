"""Extract MSVC C++ exception-handling metadata as boundary and type evidence.

Wiz8.exe was linked without RTTI, so no type descriptors survive for its own
classes. The exception tables survive anyway, because the runtime needs them to
destroy locals while unwinding, and they carry three things nothing else in the
image states directly:

* every ``FuncInfo`` record reaches exactly one function through its handler
  thunk, which pins that function's first byte without any heuristic;
* every unwind state names a cleanup funclet, and each funclet names both a
  frame slot and the destructor that runs on it, which places a typed local
  object at a known offset;
* any ``catch`` of a class type still points at a real ``TypeDescriptor``, so a
  build that kept one keeps the decorated class name with it.

The record layout is the published MSVC one (``ehdata.h``); nothing here is
recovered from the runtime's own code.
"""

from __future__ import annotations

import csv
import io
import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

import yaml

from .binary.demangle import demangle
from .binary.image import PeImage
from .binary.inventory import load_inventory
from .config import Settings
from .ghidra.project import program_name
from .paths import atomic_write

FUNC_INFO_MAGICS = (0x19930520, 0x19930521, 0x19930522)
_REPORT_FILES = ("functions.csv", "unwind.csv", "catch.csv")
_SNAPSHOT_NAME = "eh-metadata"
_MAX_FUNCLET_BYTES = 48


@dataclass
class Funclet:
    kind: str
    frame_offset: int | None
    target: int | None
    import_slot: int | None = None
    import_name: str = ""
    element_destructor: int | None = None


@dataclass
class Record:
    magic: int
    address: int
    max_state: int
    unwind_map: int
    try_block_count: int
    try_block_map: int
    ip_map_count: int
    ip_map: int
    handler_thunk: int | None = None
    frame_setup: int | None = None
    function_start: int | None = None
    states: list[tuple[int, int, Funclet]] = field(default_factory=list)
    catches: list[dict[str, Any]] = field(default_factory=list)

    @property
    def signature(self) -> str:
        """A relocation-independent shape hash, for cross-build joining.

        Deliberately excludes every address: the frame offsets and the cleanup
        shapes are what stay stable when the same source is rebuilt at another
        base with other neighbours.
        """
        parts = [str(self.max_state), str(self.try_block_count)]
        for _, to_state, funclet in self.states:
            offset = "?" if funclet.frame_offset is None else str(funclet.frame_offset)
            parts.append(f"{to_state}:{funclet.kind}:{offset}")
        return "|".join(parts)


def _disassembler() -> Any:
    from capstone import CS_ARCH_X86, CS_MODE_32, Cs

    engine = Cs(CS_ARCH_X86, CS_MODE_32)
    engine.detail = True
    return engine


def decode_funclet(
    image: PeImage,
    address: int,
    imports: dict[int, str],
    engine: Any | None = None,
) -> Funclet:
    """Decode a cleanup funclet into the frame slot and destructor it names.

    A funclet is a handful of instructions ending in one tail branch. It reaches
    the object either directly (``lea ecx, [ebp-N]``) or through a stored
    pointer, and then branches to the destructor - which may be a direct call, an
    indirect jump through the import table when the class belongs to SurRender,
    or a vector-destructor helper that takes the element destructor as a pushed
    literal.
    """
    from capstone import CS_OP_IMM, CS_OP_MEM
    from capstone.x86 import X86_REG_EBP

    decoder: Any = engine if engine is not None else _disassembler()
    raw = image.read(address, _MAX_FUNCLET_BYTES)
    if len(raw) < 4:
        return Funclet("unreadable", None, None)

    frame_offset: int | None = None
    addressing: str | None = None
    pushed_code: list[int] = []
    for instruction in decoder.disasm(raw, address):
        if instruction.mnemonic in {"ret", "retn", "leave", "int3"}:
            break
        for operand in instruction.operands:
            if (
                operand.type == CS_OP_MEM
                and operand.mem.base == X86_REG_EBP
                and operand.mem.index == 0
                and frame_offset is None
            ):
                frame_offset = operand.mem.disp
                addressing = "object" if instruction.mnemonic == "lea" else "pointer"
        if instruction.mnemonic == "push":
            operand = instruction.operands[0]
            if operand.type == CS_OP_IMM and image.is_code(operand.imm):
                pushed_code.append(operand.imm)
        if instruction.mnemonic in {"call", "jmp"}:
            operand = instruction.operands[0]
            element = pushed_code[0] if pushed_code else None
            if operand.type == CS_OP_IMM:
                target = operand.imm
                if not image.is_code(target):
                    return Funclet("bad-target", frame_offset, None, element_destructor=element)
                kind = addressing or ("vector" if element else "direct")
                return Funclet(kind, frame_offset, target, element_destructor=element)
            if operand.type == CS_OP_MEM and operand.mem.base == 0 and operand.mem.index == 0:
                slot = operand.mem.disp & 0xFFFFFFFF
                name = imports.get(slot, "")
                kind = f"{addressing or 'direct'}-import"
                return Funclet(
                    kind,
                    frame_offset,
                    None,
                    import_slot=slot,
                    import_name=name,
                    element_destructor=element,
                )
            return Funclet("indirect-register", frame_offset, None, element_destructor=element)
    return Funclet("no-branch", frame_offset, None)


def import_slots(path: Path) -> dict[int, str]:
    """Map each import-table slot address to its decorated symbol name."""
    import pefile

    pe = pefile.PE(str(path), fast_load=True)
    pe.parse_data_directories(
        directories=[
            pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"],
            pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT"],
        ]
    )
    slots: dict[int, str] = {}
    for attribute in ("DIRECTORY_ENTRY_IMPORT", "DIRECTORY_ENTRY_DELAY_IMPORT"):
        for module in getattr(pe, attribute, []) or []:
            module_name = (module.dll or b"").decode("latin-1")
            for symbol in module.imports:
                if symbol.address is None:
                    continue
                name = (symbol.name or b"").decode("latin-1") or f"#{symbol.ordinal}"
                slots[symbol.address] = f"{module_name}!{name}"
    pe.close()
    return slots


def _type_name(image: PeImage, descriptor: int) -> str:
    if not descriptor:
        return ""
    return image.read_cstring(descriptor + 8, 512) or ""


def _find_records(image: PeImage) -> list[Record]:
    records: list[Record] = []
    for magic in FUNC_INFO_MAGICS:
        for address in image.find_all(struct.pack("<I", magic)):
            raw = image.read(address, 28)
            if len(raw) != 28:
                continue
            _, max_state, unwind_map, try_count, try_map, ip_count, ip_map = struct.unpack("<IiIiIiI", raw)
            # A bare magic constant elsewhere in .rdata is possible; require the
            # counts to be sane and every non-null table pointer to be mapped.
            if not 0 <= max_state < 4096 or not 0 <= try_count < 512:
                continue
            if unwind_map and image.offset(unwind_map) is None:
                continue
            if try_map and image.offset(try_map) is None:
                continue
            records.append(
                Record(
                    magic=magic,
                    address=address,
                    max_state=max_state,
                    unwind_map=unwind_map,
                    try_block_count=try_count,
                    try_block_map=try_map,
                    ip_map_count=ip_count,
                    ip_map=ip_map,
                )
            )
    records.sort(key=lambda record: record.address)
    return records


def _link_owning_functions(image: PeImage, records: list[Record]) -> None:
    """Resolve each record to the function that installs it.

    MSVC emits one ``__ehhandler`` thunk per record (``mov eax, <FuncInfo>``
    followed by a jump into the runtime), and the function's frame setup pushes
    that thunk. Both links are one-to-one in practice, so each is only accepted
    when it is unambiguous.
    """
    text = image.text
    data = image.data
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    by_address = {record.address: record for record in records}
    thunk_sites: dict[int, list[int]] = {}
    for offset in range(low, max(low, high - 5)):
        if data[offset] != 0xB8:
            continue
        immediate = struct.unpack_from("<I", data, offset + 1)[0]
        if immediate in by_address:
            site = image.virtual_address(offset)
            if site is not None:
                thunk_sites.setdefault(immediate, []).append(site)
    thunks: dict[int, int] = {}
    for address, sites in thunk_sites.items():
        if len(sites) == 1:
            by_address[address].handler_thunk = sites[0]
            thunks[sites[0]] = address

    push_sites: dict[int, list[int]] = {}
    for offset in range(low, max(low, high - 5)):
        if data[offset] != 0x68:
            continue
        immediate = struct.unpack_from("<I", data, offset + 1)[0]
        if immediate in thunks:
            site = image.virtual_address(offset)
            if site is not None:
                push_sites.setdefault(immediate, []).append(site)
    for thunk, sites in push_sites.items():
        if len(sites) != 1:
            continue
        record = by_address[thunks[thunk]]
        record.frame_setup = sites[0]
        # The frame setup is `push -1; push <thunk>`, and VC6 emits it as the
        # first instruction of the function, so the two `push -1` bytes are the
        # entry point. Only claim a start when those bytes are actually there.
        if image.read(sites[0] - 2, 2) == b"\x6a\xff":
            record.function_start = sites[0] - 2


def _read_tables(
    image: PeImage, records: list[Record], imports: dict[int, str], *, code_readable: bool
) -> None:
    engine = _disassembler()
    for record in records:
        if record.unwind_map:
            for state in range(record.max_state):
                raw = image.read(record.unwind_map + state * 8, 8)
                if len(raw) != 8:
                    break
                to_state, action = struct.unpack("<iI", raw)
                if not action or not image.is_code(action):
                    funclet = Funclet("none", None, None)
                elif not code_readable:
                    # The tables live in .rdata and stay readable even when the
                    # code section is protected, but disassembling ciphertext
                    # would manufacture confident-looking nonsense.
                    funclet = Funclet("protected-code", None, None)
                else:
                    funclet = decode_funclet(image, action, imports, engine)
                record.states.append((state, to_state, funclet))
        for index in range(record.try_block_count):
            raw = image.read(record.try_block_map + index * 20, 20)
            if len(raw) != 20:
                break
            try_low, try_high, catch_high, catch_count, handler_array = struct.unpack("<iiiiI", raw)
            if not 0 <= catch_count < 256 or not handler_array:
                continue
            for catch_index in range(catch_count):
                handler = image.read(handler_array + catch_index * 16, 16)
                if len(handler) != 16:
                    break
                adjectives, descriptor, catch_object, handler_address = struct.unpack("<IIiI", handler)
                record.catches.append(
                    {
                        "try_index": index,
                        "try_low": try_low,
                        "try_high": try_high,
                        "catch_high": catch_high,
                        "catch_index": catch_index,
                        "adjectives": adjectives,
                        "type_descriptor": descriptor,
                        "type_name": _type_name(image, descriptor),
                        "catch_object_offset": catch_object,
                        "handler": handler_address,
                    }
                )


def analyse_image(path: Path) -> list[Record]:
    image = PeImage(path)
    records = _find_records(image)
    _link_owning_functions(image, records)
    # No record reaching its handler thunk means the code section is not plain
    # text - the protected retail build is exactly this case.
    code_readable = any(record.handler_thunk is not None for record in records)
    _read_tables(image, records, import_slots(path), code_readable=code_readable)
    return records


def _csv_text(fields: list[str], rows: list[dict[str, Any]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def _hex(value: int | None) -> str:
    return "" if value is None else f"{value:08x}"


def _snapshot_readme() -> str:
    return """# MSVC exception-metadata snapshot

Generated observations of the C++ exception tables in every first-party Wizardry executable.
They are tracked because reproduction needs the proprietary binaries, which are never committed.
Each row records the program identity that produced it, so an unavailable build is visible as
missing rows rather than as a silent gap.

The producer is `wiz8decomp.eh_metadata`. Normal runs write the same CSVs under
`build/reports/eh-metadata/` and fail when they differ from this snapshot:

```sh
uv run wiz8 eh-metadata                  # verify against the snapshot
uv run wiz8 eh-metadata --update-snapshot
```

`functions.csv` has one row per `FuncInfo` record. `function_start` is exact rather than
inferred: the record reaches exactly one handler thunk, the thunk is pushed by exactly one frame
setup, and that setup's `push -1` is the function's first instruction. `unwind_signature` excludes
every address, so the same source compiled into another build hashes the same and the column can be
joined across programs.

`unwind.csv` has one row per unwind state. `frame_offset` is the `ebp`-relative slot the cleanup
funclet addresses and `target` is the destructor it branches to, so a row places a typed local
object at a known offset. `kind` records how the object was reached: `object` for a direct `lea`,
`pointer` for an indirect load, `pushed-pointer`/`pushed-literal` for the `__cdecl` shapes.

`catch.csv` has one row per catch handler, including the `TypeDescriptor` address and its decorated
name where the build kept one. These are the only surviving MSVC type descriptors in the corpus.
"""


def _representative_modules(settings: Settings) -> tuple[list[dict[str, Any]], dict[str, str]]:
    """One module per distinct payload, preferring the canonical variant.

    Several variants ship byte-identical executables. Emitting each one would
    multiply every row without adding an observation, and attributing the rows
    to whichever variant happened to sort first would bury the canonical
    matching target under an incidental name.
    """
    modules = [
        module
        for module in load_inventory(settings)["modules"]
        if module.get("classification") == "first-party-game"
    ]
    if not modules:
        raise RuntimeError("no first-party-game modules in the inventory; run 'wiz8 inventory' first")
    canonical = yaml.safe_load(
        (settings.repo_dir / "config" / "variants.yml").read_text(encoding="utf-8")
    )["canonical_matching_target"]["variant"]

    groups: dict[str, list[dict[str, Any]]] = {}
    for module in modules:
        groups.setdefault(module["sha256"], []).append(module)
    chosen: list[dict[str, Any]] = []
    aliases: dict[str, str] = {}
    for members in groups.values():
        members.sort(key=lambda item: (item["variant"] != canonical, item["variant"], item["relative_path"]))
        chosen.append(members[0])
        for other in members[1:]:
            aliases[program_name(other)] = program_name(members[0])
    chosen.sort(key=lambda item: (item["variant"], item["relative_path"]))
    return chosen, dict(sorted(aliases.items()))


def sweep_eh_metadata(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    modules, aliases = _representative_modules(settings)

    function_rows: list[dict[str, Any]] = []
    unwind_rows: list[dict[str, Any]] = []
    catch_rows: list[dict[str, Any]] = []
    per_program: dict[str, int] = {}

    for module in modules:
        program = program_name(module)
        path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not path.is_file():
            raise RuntimeError(f"module payload is missing: {path}")
        records = analyse_image(path)
        per_program[program] = len(records)
        for record in records:
            function_rows.append(
                {
                    "program": program,
                    "funcinfo": _hex(record.address),
                    "magic": f"{record.magic:08x}",
                    "handler_thunk": _hex(record.handler_thunk),
                    "frame_setup": _hex(record.frame_setup),
                    "function_start": _hex(record.function_start),
                    "max_state": record.max_state,
                    "try_block_count": record.try_block_count,
                    "unwind_signature": record.signature,
                }
            )
            for state, to_state, funclet in record.states:
                unwind_rows.append(
                    {
                        "program": program,
                        "funcinfo": _hex(record.address),
                        "state": state,
                        "to_state": to_state,
                        "kind": funclet.kind,
                        "frame_offset": "" if funclet.frame_offset is None else funclet.frame_offset,
                        "target": _hex(funclet.target),
                        "import_slot": _hex(funclet.import_slot),
                        "import_name": funclet.import_name,
                        "element_destructor": _hex(funclet.element_destructor),
                    }
                )
            for entry in record.catches:
                catch_rows.append(
                    {
                        "program": program,
                        "funcinfo": _hex(record.address),
                        "try_index": entry["try_index"],
                        "try_low": entry["try_low"],
                        "try_high": entry["try_high"],
                        "catch_high": entry["catch_high"],
                        "catch_index": entry["catch_index"],
                        "adjectives": f"{entry['adjectives']:08x}",
                        "type_descriptor": _hex(entry["type_descriptor"]),
                        "type_name": entry["type_name"],
                        "catch_object_offset": entry["catch_object_offset"],
                        "handler": _hex(entry["handler"]),
                    }
                )

    # Imported cleanups name a library class outright; decoding them turns a
    # frame slot into a named type rather than just an address.
    signatures = demangle([row["import_name"].split("!", 1)[-1] for row in unwind_rows])
    for row in unwind_rows:
        symbol = row["import_name"].split("!", 1)[-1]
        row["import_signature"] = signatures.get(symbol, "")

    function_rows.sort(key=lambda row: (row["program"], row["funcinfo"]))
    unwind_rows.sort(key=lambda row: (row["program"], row["funcinfo"], row["state"]))
    catch_rows.sort(key=lambda row: (row["program"], row["funcinfo"], row["try_index"], row["catch_index"]))

    outputs = {
        "functions.csv": _csv_text(
            [
                "program",
                "funcinfo",
                "magic",
                "handler_thunk",
                "frame_setup",
                "function_start",
                "max_state",
                "try_block_count",
                "unwind_signature",
            ],
            function_rows,
        ),
        "unwind.csv": _csv_text(
            [
                "program",
                "funcinfo",
                "state",
                "to_state",
                "kind",
                "frame_offset",
                "target",
                "import_slot",
                "import_name",
                "import_signature",
                "element_destructor",
            ],
            unwind_rows,
        ),
        "catch.csv": _csv_text(
            [
                "program",
                "funcinfo",
                "try_index",
                "try_low",
                "try_high",
                "catch_high",
                "catch_index",
                "adjectives",
                "type_descriptor",
                "type_name",
                "catch_object_offset",
                "handler",
            ],
            catch_rows,
        ),
    }

    report_dir = settings.build_dir / "reports" / _SNAPSHOT_NAME
    snapshot_dir = settings.repo_dir / "evidence" / "snapshots" / _SNAPSHOT_NAME
    for name, value in outputs.items():
        atomic_write(report_dir / name, value)
    if update_snapshot:
        for name, value in outputs.items():
            atomic_write(snapshot_dir / name, value)
        atomic_write(snapshot_dir / "README.md", _snapshot_readme())
    snapshot_fresh = all(
        (snapshot_dir / name).is_file() and (snapshot_dir / name).read_text(encoding="utf-8") == outputs[name]
        for name in _REPORT_FILES
    )
    if not update_snapshot and not snapshot_fresh:
        raise RuntimeError(
            "exception-metadata report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        )

    resolved = sum(1 for row in function_rows if row["function_start"])
    typed_catches = sum(1 for row in catch_rows if row["type_name"])
    # A build whose code section is protected still yields readable tables but no
    # reachable thunks. Saying so beats a silent column of blanks, and its rows
    # must not dilute the resolution rates for the builds that did decode.
    opaque = sorted(
        program
        for program, count in per_program.items()
        if count and not any(row["program"] == program and row["handler_thunk"] for row in function_rows)
    )
    readable = [row for row in unwind_rows if row["program"] not in opaque]
    cleaned_up = [row for row in readable if row["target"] or row["import_name"]]
    placed = sum(1 for row in cleaned_up if row["frame_offset"] != "")
    unresolved = sorted(
        {row["kind"] for row in readable if not row["target"] and not row["import_name"]}
    )
    return {
        "schema": "wiz8.eh-metadata",
        "programs": per_program,
        "byte_identical_aliases": aliases,
        "programs_without_readable_code": opaque,
        "funcinfo_records": len(function_rows),
        "resolved_function_starts": resolved,
        "unwind_states": len(unwind_rows),
        "resolved_cleanups": len(cleaned_up),
        "placed_local_objects": placed,
        "unresolved_cleanup_kinds": unresolved,
        "distinct_destructors": len({row["target"] for row in unwind_rows if row["target"]}),
        "imported_destructors": len({row["import_name"] for row in unwind_rows if row["import_name"]}),
        "catch_handlers": len(catch_rows),
        "typed_catch_handlers": typed_catches,
        "type_names": sorted({row["type_name"] for row in catch_rows if row["type_name"]}),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
