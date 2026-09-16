"""Higher-level analysis for the structural MSVC table census.

This layer deliberately consumes the binary-only census instead of participating
in discovery. It adds three kinds of evidence that are useful for class recovery
but are not required to decide where a table starts:

* receiver provenance: whether a vfptr/vbptr store is derived from incoming ECX
  (the x86 this-register for __thiscall), including dynamic virtual-base stores;
* construction families: multiple table transitions written by one function to
  the same incoming-ECX-derived receiver;
* slot resolution: import thunks, _purecall, exported local symbols, or ordinary
  local bodies.

``incoming-ecx`` is deliberately a provenance label rather than a claim that the
containing function has already been proved to be a C++ member. None of these
labels are allowed to make a structural candidate disappear. A failed provenance
trace stays ``unknown`` rather than becoming negative evidence.
"""

from __future__ import annotations

import struct
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

from .binary.code import decode_chain_ending_at, disassembler, function_start, import_thunks
from .binary.image import PeImage

_GPRS = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"}
_VOLATILE = {"eax", "ecx", "edx"}


def _hex(address: int) -> str:
    return f"0x{address:08x}"


def _pe32_optional(image: PeImage) -> int | None:
    data = image.data
    if len(data) < 0x40:
        return None
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    optional = pe_offset + 4 + 20
    if optional + 2 > len(data) or struct.unpack_from("<H", data, optional)[0] != 0x10B:
        return None
    return optional


def _directory(image: PeImage, index: int) -> tuple[int, int]:
    optional = _pe32_optional(image)
    if optional is None:
        return 0, 0
    data = image.data
    if optional + 96 > len(data):
        return 0, 0
    count = struct.unpack_from("<I", data, optional + 92)[0]
    if index >= count:
        return 0, 0
    rva, size = struct.unpack_from("<II", data, optional + 96 + index * 8)
    return (image.image_base + rva if rva else 0), size


def _read_u16(image: PeImage, address: int) -> int | None:
    raw = image.read(address, 2)
    return struct.unpack("<H", raw)[0] if len(raw) == 2 else None


def _import_slots(image: PeImage) -> dict[int, str]:
    """IAT slot -> ``dll!symbol`` for a PE32 image."""

    directory, _size = _directory(image, 1)
    if not directory:
        return {}

    slots: dict[int, str] = {}
    descriptor = directory
    for _ in range(4096):
        raw = image.read(descriptor, 20)
        if len(raw) != 20:
            break
        original_first_thunk, _stamp, _chain, name_rva, first_thunk = struct.unpack("<IIIII", raw)
        if not any((original_first_thunk, name_rva, first_thunk)):
            break

        dll = image.read_cstring(image.image_base + name_rva) or "?"
        lookup = image.image_base + (original_first_thunk or first_thunk)
        iat = image.image_base + first_thunk
        for index in range(65536):
            value = image.read_u32(lookup + index * 4)
            if value in (None, 0):
                break
            if value & 0x80000000:
                symbol = f"#{value & 0xFFFF}"
            else:
                name = image.read_cstring(image.image_base + value + 2)
                symbol = name or "?"
            slots[iat + index * 4] = f"{dll}!{symbol}"
        descriptor += 20
    return slots


def _export_names(image: PeImage) -> dict[int, list[str]]:
    """All named PE exports grouped by provider address."""

    directory, _size = _directory(image, 0)
    if not directory:
        return {}
    raw = image.read(directory, 40)
    if len(raw) != 40:
        return {}

    (
        _characteristics,
        _timestamp,
        _version,
        _name_rva,
        _ordinal_base,
        function_count,
        name_count,
        functions_rva,
        names_rva,
        ordinals_rva,
    ) = struct.unpack("<III III IIII", raw)
    if not function_count or not name_count:
        return {}

    functions = image.image_base + functions_rva
    names = image.image_base + names_rva
    ordinals = image.image_base + ordinals_rva
    result: dict[int, list[str]] = defaultdict(list)
    for index in range(name_count):
        name_rva = image.read_u32(names + index * 4)
        ordinal = _read_u16(image, ordinals + index * 2)
        if name_rva is None or ordinal is None or ordinal >= function_count:
            continue
        function_rva = image.read_u32(functions + ordinal * 4)
        name = image.read_cstring(image.image_base + name_rva)
        if function_rva is None or not name:
            continue
        result[image.image_base + function_rva].append(name)
    return dict(result)


def _canonical_reg(name: str) -> str | None:
    families = {
        "al": "eax",
        "ah": "eax",
        "ax": "eax",
        "eax": "eax",
        "cl": "ecx",
        "ch": "ecx",
        "cx": "ecx",
        "ecx": "ecx",
        "dl": "edx",
        "dh": "edx",
        "dx": "edx",
        "edx": "edx",
        "bl": "ebx",
        "bh": "ebx",
        "bx": "ebx",
        "ebx": "ebx",
        "sp": "esp",
        "esp": "esp",
        "bp": "ebp",
        "ebp": "ebp",
        "si": "esi",
        "esi": "esi",
        "di": "edi",
        "edi": "edi",
    }
    return families.get(name)


def _instruction_chain(image: PeImage, engine: Any, address: int) -> tuple[int | None, list[Any]]:
    start = function_start(image, address)
    if start is not None and 0 < address - start <= 0x2000:
        raw = image.read(start, address - start)
        chain = list(engine.disasm(raw, start))
        if chain and chain[-1].address + chain[-1].size == address:
            return start, chain
    chain = decode_chain_ending_at(image, engine, address, window=256)
    return start, chain


def _receiver_provenance(image: PeImage, engine: Any, write: dict[str, Any]) -> dict[str, Any]:
    """Track aliases of incoming ECX up to one table store.

    Values are affine ``incoming_ecx + constant`` when known. The indexed
    virtual-base form is retained as ``incoming-ecx-plus-dynamic`` when exactly
    one address register is incoming-ECX-derived.
    """

    instruction = int(write["instruction"], 16)
    start, chain = _instruction_chain(image, engine, instruction)
    offsets: dict[str, int | None] = {register: None for register in _GPRS}
    offsets["ecx"] = 0

    from capstone.x86_const import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

    for insn in chain:
        mnemonic = insn.mnemonic
        handled: set[str] = set()
        operands = list(insn.operands)

        if mnemonic == "mov" and len(operands) >= 2 and operands[0].type == X86_OP_REG:
            dst = _canonical_reg(insn.reg_name(operands[0].reg))
            if dst in _GPRS:
                handled.add(dst)
                if operands[1].type == X86_OP_REG:
                    src = _canonical_reg(insn.reg_name(operands[1].reg))
                    offsets[dst] = offsets.get(src) if src is not None else None
                else:
                    offsets[dst] = None
        elif mnemonic == "lea" and len(operands) >= 2 and operands[0].type == X86_OP_REG:
            dst = _canonical_reg(insn.reg_name(operands[0].reg))
            if dst in _GPRS:
                handled.add(dst)
                value: int | None = None
                if operands[1].type == X86_OP_MEM:
                    mem = operands[1].mem
                    base = _canonical_reg(insn.reg_name(mem.base)) if mem.base else None
                    index = _canonical_reg(insn.reg_name(mem.index)) if mem.index else None
                    if index is None and base is not None and offsets.get(base) is not None:
                        value = int(offsets[base]) + mem.disp
                    elif (
                        base is None
                        and index is not None
                        and mem.scale == 1
                        and offsets.get(index) is not None
                    ):
                        value = int(offsets[index]) + mem.disp
                offsets[dst] = value
        elif (
            mnemonic in {"add", "sub"}
            and len(operands) >= 2
            and operands[0].type == X86_OP_REG
            and operands[1].type == X86_OP_IMM
        ):
            dst = _canonical_reg(insn.reg_name(operands[0].reg))
            if dst in _GPRS:
                handled.add(dst)
                value = offsets.get(dst)
                if value is not None:
                    delta = operands[1].imm if mnemonic == "add" else -operands[1].imm
                    offsets[dst] = value + delta
        elif mnemonic in {"inc", "dec"} and operands and operands[0].type == X86_OP_REG:
            dst = _canonical_reg(insn.reg_name(operands[0].reg))
            if dst in _GPRS:
                handled.add(dst)
                value = offsets.get(dst)
                if value is not None:
                    offsets[dst] = value + (1 if mnemonic == "inc" else -1)

        if mnemonic == "call":
            for register in _VOLATILE:
                offsets[register] = None
            continue

        try:
            _reads, writes = insn.regs_access()
        except Exception:
            writes = []
        for register_id in writes:
            register = _canonical_reg(insn.reg_name(register_id))
            if register in _GPRS and register not in handled:
                offsets[register] = None

    if write.get("absolute_destination"):
        kind = "absolute"
        receiver_offset = None
    else:
        base = write.get("base_register")
        index = write.get("index_register")
        base_value = offsets.get(base) if base else None
        index_value = offsets.get(index) if index else None
        displacement = int(write.get("displacement") or 0)
        scale = int(write.get("scale") or 1)
        if index is None and base_value is not None:
            kind = "incoming-ecx"
            receiver_offset = base_value + displacement
        elif base_value is not None and index is not None and index_value is None:
            kind = "incoming-ecx-plus-dynamic"
            receiver_offset = base_value + displacement
        elif base is not None and base_value is None and index_value is not None and scale == 1:
            kind = "incoming-ecx-plus-dynamic"
            receiver_offset = index_value + displacement
        else:
            kind = "unknown"
            receiver_offset = None

    return {
        "function": _hex(start) if start is not None else None,
        "receiver_provenance": kind,
        "receiver_offset": receiver_offset,
    }


def _classify_slot(
    image: PeImage,
    target: int,
    thunks: dict[int, str],
    exports: dict[int, list[str]],
    frequency: Counter[int],
) -> dict[str, Any]:
    names = exports.get(target, [])
    imported = thunks.get(target)
    if imported is not None:
        symbol = imported.rsplit("!", 1)[-1]
        kind = "pure-virtual" if symbol == "_purecall" else "import-thunk"
        return {"kind": kind, "name": imported, "shared_count": frequency[target]}
    if names:
        special = next((name for name in names if name.startswith("??_G")), None)
        kind = "scalar-deleting-destructor" if special else "exported-local"
        return {"kind": kind, "names": names, "shared_count": frequency[target]}
    if image.is_code(target):
        return {"kind": "local-body", "shared_count": frequency[target]}
    return {"kind": "unresolved", "shared_count": frequency[target]}


def _construction_families(report: dict[str, Any]) -> list[dict[str, Any]]:
    groups: dict[tuple[str, str, int | None], list[dict[str, Any]]] = defaultdict(list)
    for kind in ("vftables", "vbtables"):
        table_kind = "vftable" if kind == "vftables" else "vbtable"
        for table in report.get(kind, []):
            for write in table.get("writes", []):
                function = write.get("function")
                provenance = write.get("receiver_provenance")
                offset = write.get("receiver_offset")
                if function is None or provenance not in {
                    "incoming-ecx",
                    "incoming-ecx-plus-dynamic",
                }:
                    continue
                groups[(function, provenance, offset)].append(
                    {
                        "instruction": write["instruction"],
                        "table": table["address"],
                        "kind": table_kind,
                        "size": table.get("slot_count", table.get("entry_count")),
                        "export_names": table.get("export_names", []),
                    }
                )

    families: list[dict[str, Any]] = []
    for (function, provenance, offset), events in sorted(groups.items()):
        events.sort(key=lambda row: int(row["instruction"], 16))
        distinct = []
        seen: set[tuple[str, str]] = set()
        for event in events:
            identity = (event["kind"], event["table"])
            if identity in seen:
                continue
            seen.add(identity)
            distinct.append(event)
        if len(distinct) < 2:
            continue
        families.append(
            {
                "function": function,
                "receiver_provenance": provenance,
                "receiver_offset": offset,
                "transitions": distinct,
            }
        )
    return families


def enrich_msvc_table_report(path: Path, report: dict[str, Any]) -> dict[str, Any]:
    """Add receiver provenance, construction families, and slot resolution."""

    image = PeImage(path)
    engine = disassembler()
    slots = _import_slots(image)
    thunks = import_thunks(image, slots)
    exports = _export_names(image)

    frequency: Counter[int] = Counter()
    for table in report.get("vftables", []):
        for slot in table.get("slots", []):
            frequency[int(slot["target"], 16)] += 1

    provenance_counts: Counter[str] = Counter()
    for kind in ("vftables", "vbtables"):
        for table in report.get(kind, []):
            for write in table.get("writes", []):
                detail = _receiver_provenance(image, engine, write)
                write.update(detail)
                provenance_counts[detail["receiver_provenance"]] += 1

    slot_counts: Counter[str] = Counter()
    for table in report.get("vftables", []):
        for slot in table.get("slots", []):
            target = int(slot["target"], 16)
            resolution = _classify_slot(image, target, thunks, exports, frequency)
            slot["resolution"] = resolution
            slot_counts[resolution["kind"]] += 1

    families = _construction_families(report)
    report["analysis"] = {
        "receiver_provenance": dict(sorted(provenance_counts.items())),
        "slot_resolution": dict(sorted(slot_counts.items())),
        "construction_family_count": len(families),
        "construction_families": families,
    }
    return report


def table_shape_fingerprint(table: dict[str, Any]) -> str:
    """Address-independent coarse fingerprint for cross-build candidate matching.

    Local addresses are normalized by equality pattern (L0, L1, ...), while
    imported/pure slots retain their semantic name. This is intentionally a
    candidate matcher rather than an identity proof.
    """

    locals_seen: dict[str, str] = {}
    tokens: list[str] = []
    for slot in table.get("slots", []):
        resolution = slot.get("resolution", {})
        kind = resolution.get("kind")
        if kind in {"import-thunk", "pure-virtual"}:
            tokens.append(f"I:{resolution.get('name')}")
            continue
        target = slot["target"]
        token = locals_seen.setdefault(target, f"L{len(locals_seen)}")
        tokens.append(token)
    return f"{len(tokens)}|" + ",".join(tokens)


def compare_table_reports(reports: list[dict[str, Any]]) -> dict[str, Any]:
    """Group vftables from several variants by address-independent shape."""

    groups: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for index, report in enumerate(reports):
        for table in report.get("vftables", []):
            groups[table_shape_fingerprint(table)].append(
                {"report": index, "binary": report.get("binary"), "address": table["address"]}
            )
    matches = [
        {"fingerprint": fingerprint, "tables": tables}
        for fingerprint, tables in sorted(groups.items())
        if len({row["report"] for row in tables}) > 1
    ]
    return {"schema": "wiz8.msvc-table-cross-build", "matches": matches}
