"""Recover literal arguments from diagnostic call sites in every official build.

Two imported SurRender entry points are passed original developer strings at
almost every call:

* ``srAssertFail(expr, file, line, msg, ...)`` names a source file, a line, the
  asserted expression - which is where member, parameter and enum names come
  from - and often a message naming the class outright;
* ``srRuntimeClass::setName(name)`` attaches a runtime name to an object the
  game registers with the renderer's class registry.

Both are recovered statically. The arguments are pushed immediately before the
call, so decoding a short window that ends exactly on the call instruction
recovers them without a decompiler, which means every build can be swept the
same way rather than only the one that has a Ghidra project.
"""

from __future__ import annotations

import csv
import io
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .binary.image import PeImage
from .binary.inventory import load_inventory
from .config import Settings
from .eh_metadata import _disassembler, import_slots
from .ghidra.project import program_name
from .paths import atomic_write

_SNAPSHOT_NAME = "call-sites"
_REPORT_FILES = ("assertions.csv", "runtime-class-names.csv")

ASSERT_SYMBOL = "?srAssertFail@@YAXPBD0J0ZZ"
SET_NAME_SYMBOL = "?setName@srRuntimeClass@@QAEXPBD@Z"

# How far back a call's arguments may be pushed. Generous enough for the
# four-argument assert plus interleaved argument evaluation.
_WINDOW = 96
# How far back to look for the padding that precedes a function's first byte.
_FUNCTION_SCAN = 0x3000
_PADDING = {0xCC, 0x90}
_PROLOGUES = (b"\x55\x8b\xec", b"\x6a\xff", b"\x83\xec", b"\x81\xec", b"\x53", b"\x56", b"\x57", b"\x8b\xff")


@dataclass
class CallSite:
    address: int
    kind: str
    arguments: list[int | None]
    function_start: int | None


def _string_at(image: PeImage, address: int | None) -> str | None:
    """A printable C string at ``address``, or None if that is not what is there."""
    if not address:
        return None
    value = image.read_cstring(address, 512)
    if value is None or not value:
        return None
    if any(character < " " or character > "~" for character in value):
        return None
    return value


def _function_start(image: PeImage, address: int) -> int | None:
    """The entry point of the function containing ``address``.

    VC6 pads between functions with int3 or nop, so the first byte after the
    nearest preceding padding run is the entry point. The candidate is only
    accepted when it actually starts with a prologue, which keeps a jump table
    or an unpadded tail from inventing a boundary.
    """
    offset = image.offset(address)
    if offset is None:
        return None
    data = image.data
    limit = max(0, offset - _FUNCTION_SCAN)
    cursor = offset
    while cursor > limit:
        cursor -= 1
        if data[cursor] not in _PADDING:
            continue
        run_end = cursor + 1
        while cursor > limit and data[cursor - 1] in _PADDING:
            cursor -= 1
        if run_end - cursor < 1:
            continue
        candidate = image.virtual_address(run_end)
        if candidate is None:
            return None
        body = image.read(candidate, 3)
        if any(body.startswith(prologue) for prologue in _PROLOGUES):
            return candidate
        # A lone 0x90 inside an instruction stream is not padding; keep going.
    return None


def _decode_chain(image: PeImage, engine: Any, call: int) -> list[Any]:
    """Instructions in a short window that decode to end exactly on ``call``.

    x86 cannot be disassembled backwards, so every start offset in the window is
    tried and the longest chain that lands precisely on the call is kept.
    """
    best: list[Any] = []
    for back in range(4, _WINDOW):
        start = call - back
        raw = image.read(start, back)
        if len(raw) != back:
            continue
        chain = list(engine.disasm(raw, start))
        if chain and chain[-1].address + chain[-1].size == call:
            best = chain
    return best


def _pushed_arguments(image: PeImage, engine: Any, call: int, count: int) -> list[int | None]:
    """The first ``count`` __cdecl/__thiscall arguments of a call.

    Arguments are pushed right to left, so the push nearest the call is the
    first argument.
    """
    from capstone import CS_OP_IMM

    chain = _decode_chain(image, engine, call)
    pushes = [item for item in chain if item.mnemonic == "push"]
    values: list[int | None] = []
    for instruction in reversed(pushes):
        if len(values) >= count:
            break
        operand = instruction.operands[0]
        values.append(operand.imm if operand.type == CS_OP_IMM else None)
    while len(values) < count:
        values.append(None)
    return values


def _direct_sites(image: PeImage, slot: int) -> list[tuple[int, str]]:
    """Call sites reaching an import slot, however they were emitted."""
    text = image.text
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    data = image.data
    sites: list[tuple[int, str]] = []

    packed = struct.pack("<I", slot)
    offset = data.find(b"\xff\x15" + packed, low, high)
    while offset != -1:
        address = image.virtual_address(offset)
        if address is not None:
            sites.append((address, "direct"))
        offset = data.find(b"\xff\x15" + packed, offset + 1, high)

    # Linker-generated jump thunks: `jmp dword ptr [slot]`, reached by a
    # relative call. Both hops have to be followed to see the real call sites.
    thunks: list[int] = []
    offset = data.find(b"\xff\x25" + packed, low, high)
    while offset != -1:
        address = image.virtual_address(offset)
        if address is not None:
            thunks.append(address)
        offset = data.find(b"\xff\x25" + packed, offset + 1, high)
    for thunk in thunks:
        for candidate in range(low, high - 5):
            if data[candidate] != 0xE8:
                continue
            source = image.virtual_address(candidate)
            if source is None:
                continue
            relative = struct.unpack_from("<i", data, candidate + 1)[0]
            if source + 5 + relative == thunk:
                sites.append((source, "thunk"))
    return sites


def _register_indirect_sites(image: PeImage, engine: Any, slot: int) -> list[tuple[int, str]]:
    """Sites that load the import slot into a register and call through it.

    These are invisible to a byte search for `call [slot]`, and they are exactly
    the calls a per-function loop hoists out of a loop body.
    """
    from capstone import CS_OP_MEM, CS_OP_REG

    text = image.text
    low, high = text.raw_offset, text.raw_offset + text.raw_size
    data = image.data
    loads: list[int] = []
    # `mov <reg>, dword ptr [slot]` is 8B /r with a disp32 operand.
    packed = struct.pack("<I", slot)
    offset = data.find(packed, low, high)
    while offset != -1:
        for start in range(max(low, offset - 8), offset):
            raw = image.data[start : offset + 4]
            chain = list(engine.disasm(raw, image.virtual_address(start) or 0))
            if not chain:
                continue
            head = chain[0]
            if head.address + head.size != (image.virtual_address(offset) or 0) + 4:
                continue
            if head.mnemonic != "mov" or len(head.operands) != 2:
                continue
            destination, source = head.operands
            if destination.type != CS_OP_REG or source.type != CS_OP_MEM:
                continue
            if source.mem.base or source.mem.index or (source.mem.disp & 0xFFFFFFFF) != slot:
                continue
            loads.append(head.address)
            break
        offset = data.find(packed, offset + 1, high)

    sites: list[tuple[int, str]] = []
    for load in loads:
        raw = image.read(load, 512)
        chain = list(engine.disasm(raw, load))
        if not chain:
            continue
        register = chain[0].operands[0].reg
        for instruction in chain[1:]:
            if instruction.mnemonic == "call":
                operand = instruction.operands[0]
                if operand.type == CS_OP_REG and operand.reg == register:
                    sites.append((instruction.address, "register-indirect"))
            # Stop at the first write that clobbers the register.
            if instruction.mnemonic in {"mov", "lea", "pop", "xor"} and instruction.operands:
                first = instruction.operands[0]
                if first.type == CS_OP_REG and first.reg == register:
                    break
    return sites


def _all_sites(image: PeImage, engine: Any, slot: int) -> list[tuple[int, str]]:
    seen: dict[int, str] = {}
    for address, kind in _direct_sites(image, slot) + _register_indirect_sites(image, engine, slot):
        seen.setdefault(address, kind)
    return sorted(seen.items())


def _slot_for(slots: dict[int, str], symbol: str) -> int | None:
    for address, name in slots.items():
        if name.split("!", 1)[-1] == symbol:
            return address
    return None


def analyse_image(path: Path) -> dict[str, list[dict[str, Any]]]:
    image = PeImage(path)
    engine = _disassembler()
    slots = import_slots(path)
    assertions: list[dict[str, Any]] = []
    names: list[dict[str, Any]] = []

    assert_slot = _slot_for(slots, ASSERT_SYMBOL)
    if assert_slot is not None:
        for address, kind in _all_sites(image, engine, assert_slot):
            expression, source, line, message = _pushed_arguments(image, engine, address, 4)
            assertions.append(
                {
                    "call_site": address,
                    "call_kind": kind,
                    "function_start": _function_start(image, address),
                    "source_path": _string_at(image, source) or "",
                    "line": line if line is not None and 0 < line < 1_000_000 else "",
                    "expression": _string_at(image, expression) or "",
                    "message": _string_at(image, message) or "",
                }
            )

    name_slot = _slot_for(slots, SET_NAME_SYMBOL)
    if name_slot is not None:
        for address, kind in _all_sites(image, engine, name_slot):
            (argument,) = _pushed_arguments(image, engine, address, 1)
            names.append(
                {
                    "call_site": address,
                    "call_kind": kind,
                    "function_start": _function_start(image, address),
                    "name": _string_at(image, argument) or "",
                }
            )
    return {"assertions": assertions, "runtime_class_names": names}


def _csv_text(fields: list[str], rows: list[dict[str, Any]]) -> str:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    return stream.getvalue()


def _hex(value: Any) -> str:
    return f"{value:08x}" if isinstance(value, int) else ""


def _snapshot_readme() -> str:
    return """# Diagnostic call-site snapshot

Literal arguments recovered from `srAssertFail` and `srRuntimeClass::setName` call sites in every
first-party Wizardry executable whose code section is readable. Tracked because reproduction needs
the proprietary binaries.

The producer is `wiz8decomp.call_sites`. Normal runs write the same CSVs under
`build/reports/call-sites/` and fail when they differ from this snapshot:

```sh
uv run wiz8 call-sites                  # verify against the snapshot
uv run wiz8 call-sites --update-snapshot
```

Recovery is static. Arguments are pushed immediately before the call, so a short instruction window
that decodes to end exactly on the call yields them; no decompiler and no Ghidra project is
involved, which is why every build is covered rather than only the canonical one.

`call_kind` records how the import was reached: `direct` for `call dword ptr [slot]`, `thunk` for a
relative call into a linker jump thunk, and `register-indirect` for the sites that load the slot
into a register first - the last of which a byte search for the call encoding cannot see.

`function_start` is derived, not read: it is the first byte after the nearest preceding inter-
function padding run, accepted only when that byte begins a recognised prologue. It is blank when no
candidate qualified. It is not a substitute for a reviewed function identity.

`assertions.csv` keeps the message argument as well as the expression. Messages frequently name the
owning class in prose where the expression only names a member.

These rows are observations across all builds and do not replace
`evidence/observations/wiz8/assertions.csv`, which is the reviewed canonical-retail table carrying
`containing_function` from Ghidra.
"""


def _representative_modules(settings: Settings) -> tuple[list[dict[str, Any]], dict[str, str]]:
    import yaml

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


def sweep_call_sites(settings: Settings, *, update_snapshot: bool = False) -> dict[str, Any]:
    modules, aliases = _representative_modules(settings)
    assertion_rows: list[dict[str, Any]] = []
    name_rows: list[dict[str, Any]] = []
    per_program: dict[str, dict[str, int]] = {}

    for module in modules:
        program = program_name(module)
        path = settings.work_dir / "variants" / module["variant"] / module["relative_path"]
        if not path.is_file():
            raise RuntimeError(f"module payload is missing: {path}")
        result = analyse_image(path)
        per_program[program] = {
            "assertions": len(result["assertions"]),
            "runtime_class_names": len(result["runtime_class_names"]),
        }
        for row in result["assertions"]:
            assertion_rows.append(
                {
                    "program": program,
                    "call_site": _hex(row["call_site"]),
                    "call_kind": row["call_kind"],
                    "function_start": _hex(row["function_start"]),
                    "source_path": row["source_path"],
                    "line": row["line"],
                    "expression": row["expression"],
                    "message": row["message"],
                }
            )
        for row in result["runtime_class_names"]:
            name_rows.append(
                {
                    "program": program,
                    "call_site": _hex(row["call_site"]),
                    "call_kind": row["call_kind"],
                    "function_start": _hex(row["function_start"]),
                    "name": row["name"],
                }
            )

    assertion_rows.sort(key=lambda row: (row["program"], row["call_site"]))
    name_rows.sort(key=lambda row: (row["program"], row["call_site"]))
    outputs = {
        "assertions.csv": _csv_text(
            [
                "program",
                "call_site",
                "call_kind",
                "function_start",
                "source_path",
                "line",
                "expression",
                "message",
            ],
            assertion_rows,
        ),
        "runtime-class-names.csv": _csv_text(
            ["program", "call_site", "call_kind", "function_start", "name"], name_rows
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
            "call-site report differs from the tracked snapshot; review "
            f"build/reports/{_SNAPSHOT_NAME} and rerun with --update-snapshot"
        )

    resolved = [row for row in assertion_rows if row["source_path"]]
    return {
        "schema": "wiz8.call-sites",
        "programs": per_program,
        "byte_identical_aliases": aliases,
        "assertion_sites": len(assertion_rows),
        "assertion_sites_with_source": len(resolved),
        "assertion_call_kinds": {
            kind: sum(1 for row in assertion_rows if row["call_kind"] == kind)
            for kind in sorted({row["call_kind"] for row in assertion_rows})
        },
        "assertions_with_message": sum(1 for row in assertion_rows if row["message"]),
        "source_units": len({row["source_path"] for row in resolved}),
        "runtime_class_name_sites": len(name_rows),
        "runtime_class_names": len({row["name"] for row in name_rows if row["name"]}),
        "resolved_function_starts": sum(1 for row in assertion_rows if row["function_start"]),
        "report": str(report_dir.relative_to(settings.repo_dir)),
        "snapshot": str(snapshot_dir.relative_to(settings.repo_dir)),
        "snapshot_fresh": snapshot_fresh,
        "snapshot_updated": update_snapshot,
    }
