"""Static MSVC vftable/vbtable census for VC6-era PE images.

Discovery and confirmation are intentionally separate. The PE relocation directory
is the complete index of absolute pointers materialized by code, so code references
into ``.rdata`` define the candidate universe and, crucially, every possible table
boundary. A vftable is a run of relocated pointers into executable sections; a
vbtable is a run of small aligned unrelocated displacements. A structural table is
only confirmed when code also installs its address into memory.

For binaries that export MSVC table symbols, ``??_7`` (vftable) and ``??_8``
(vbtable) provide an independent positive-label oracle. They validate detector
recall without teaching the detector where those tables are. Wiz8.exe exports no
such symbols, so its census remains binary-structural and can additionally be
checked against reviewed ``VTABLE: WIZ8`` source markers.
"""

from __future__ import annotations

import re
import struct
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .binary.code import relocation_sites
from .binary.image import PeImage

_REGISTERS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
_MAX_VBTABLE_DISPLACEMENT = 0x100000
_MAX_VBTABLE_ENTRIES = 32
_MAX_VFTABLE_SLOTS = 512
_SOURCE_VTABLE_RE = re.compile(r"^\s*//\s*VTABLE:\s*WIZ8\s+(0x[0-9a-f]+)\b", re.IGNORECASE)
_SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx"}


@dataclass(frozen=True)
class TableStore:
    instruction: int
    relocation_site: int
    target: int
    base_register: str | None
    index_register: str | None
    scale: int
    displacement: int
    absolute_destination: bool

    @property
    def object_offset(self) -> int | None:
        """Direct destination offset when the store is ``[base+disp]``."""
        if (
            self.absolute_destination
            or self.base_register is None
            or self.index_register is not None
        ):
            return None
        return self.displacement


def _decode_table_store(image: PeImage, relocation_site: int) -> TableStore | None:
    """Decode ``mov r/m32, imm32`` whose immediate begins at one relocation.

    VC6 normally installs vfptrs/vbptrs with a base-relative C7 store. Virtual
    base vfptrs can use indexed destinations such as ``[edx+esi+4]``; those are
    retained because they are still direct evidence that the immediate is a
    table address. Absolute destinations are decoded but remain weak evidence.
    """

    target = image.read_u32(relocation_site)
    if target is None:
        return None

    for back in range(2, 12):
        instruction = relocation_site - back
        raw = image.read(instruction, back + 4)
        if len(raw) != back + 4 or raw[0] != 0xC7:
            continue

        modrm = raw[1]
        mode = modrm >> 6
        extension = (modrm >> 3) & 7
        rm = modrm & 7
        if extension != 0 or mode == 3:
            continue

        cursor = 2
        base: int | None = rm
        index: int | None = None
        scale = 1
        if rm == 4:
            if cursor >= len(raw):
                continue
            sib = raw[cursor]
            cursor += 1
            scale = 1 << ((sib >> 6) & 3)
            raw_index = (sib >> 3) & 7
            base = sib & 7
            if raw_index != 4:
                index = raw_index

        displacement = 0
        absolute_destination = mode == 0 and (rm == 5 or (rm == 4 and base == 5))
        if absolute_destination:
            if cursor + 4 > len(raw):
                continue
            displacement = int.from_bytes(raw[cursor : cursor + 4], "little")
            cursor += 4
            base = None
        elif mode == 1:
            if cursor >= len(raw):
                continue
            displacement = int.from_bytes(raw[cursor : cursor + 1], "little", signed=True)
            cursor += 1
        elif mode == 2:
            if cursor + 4 > len(raw):
                continue
            displacement = int.from_bytes(raw[cursor : cursor + 4], "little", signed=True)
            cursor += 4

        if instruction + cursor != relocation_site:
            continue

        return TableStore(
            instruction=instruction,
            relocation_site=relocation_site,
            target=target,
            base_register=_REGISTERS[base] if base is not None else None,
            index_register=_REGISTERS[index] if index is not None else None,
            scale=scale,
            displacement=displacement,
            absolute_destination=absolute_destination,
        )

    return None


def _code_data_references(image: PeImage, sites: list[int]) -> dict[int, list[int]]:
    """Every non-code address named by a relocated operand in executable code."""

    references: dict[int, list[int]] = defaultdict(list)
    for site in sites:
        site_section = image.section_at(site)
        if site_section is None or not site_section.executable:
            continue
        target = image.read_u32(site)
        target_section = image.section_at(target) if target is not None else None
        if target_section is not None and not target_section.executable:
            references[target].append(site)
    return dict(references)


def _decode_vftable(
    image: PeImage, start: int, relocated: set[int], boundaries: set[int]
) -> list[int]:
    slots: list[int] = []
    cursor = start
    while len(slots) < _MAX_VFTABLE_SLOTS:
        if cursor != start and cursor in boundaries:
            break
        if cursor not in relocated:
            break
        target = image.read_u32(cursor)
        section = image.section_at(target) if target is not None else None
        if section is None or not section.executable:
            break
        slots.append(target)
        cursor += 4
    return slots


def _decode_vbtable(
    image: PeImage, start: int, relocated: set[int], boundaries: set[int]
) -> list[int]:
    """Decode a possible MSVC vbtable independently of a particular write.

    Entry zero is the displacement from the vbptr back to the top of the base
    subobject that owns that vbptr. It is therefore non-positive, but it is *not*
    generally the negation of the vbptr's complete-object offset: secondary base
    subobjects can begin later in the complete object. Later entries locate the
    virtual bases from the vbptr.
    """

    entries: list[int] = []
    cursor = start
    while len(entries) < _MAX_VBTABLE_ENTRIES:
        if cursor != start and cursor in boundaries:
            break
        if cursor in relocated:
            break
        displacement = image.read_i32(cursor)
        if (
            displacement is None
            or abs(displacement) > _MAX_VBTABLE_DISPLACEMENT
            or displacement % 4 != 0
        ):
            break
        entries.append(displacement)
        cursor += 4

    if len(entries) < 2 or entries[0] > 0:
        return []
    if not any(entry != 0 for entry in entries[1:]):
        return []
    return entries


def _vbtable_store_matches(entries: list[int], store: TableStore) -> bool:
    """A vbtable needs a direct object/subobject-relative pointer installation.

    Do not compare entry zero with the complete-object displacement. The supplied
    SurRender stream hierarchy proves why: one vbptr is installed at complete
    object +0x1c while its vbtable starts [-4, 8], because the owning base
    subobject starts at +0x18.
    """

    return bool(
        entries
        and not store.absolute_destination
        and store.base_register is not None
        and store.index_register is None
    )


def _vbtable_owner_offset_hint(entries: list[int], store: TableStore) -> int | None:
    """Owner offset relative to the store's base register, when directly expressible."""

    offset = store.object_offset
    if not entries or offset is None:
        return None
    return offset + entries[0]


def _read_u16(image: PeImage, address: int) -> int | None:
    raw = image.read(address, 2)
    return struct.unpack("<H", raw)[0] if len(raw) == 2 else None


def _exported_table_oracle(image: PeImage) -> dict[str, dict[int, list[str]]]:
    """Read exported MSVC vftable/vbtable symbols directly from the PE export directory."""

    data = image.data
    if len(data) < 0x40:
        return {"vftables": {}, "vbtables": {}}
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    optional = pe_offset + 4 + 20
    if optional + 104 > len(data):
        return {"vftables": {}, "vbtables": {}}
    directory_count = struct.unpack_from("<I", data, optional + 92)[0]
    if directory_count == 0:
        return {"vftables": {}, "vbtables": {}}

    export_rva, export_size = struct.unpack_from("<II", data, optional + 96)
    if not export_rva or not export_size:
        return {"vftables": {}, "vbtables": {}}
    export = image.image_base + export_rva
    raw = image.read(export, 40)
    if len(raw) != 40:
        return {"vftables": {}, "vbtables": {}}

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
        return {"vftables": {}, "vbtables": {}}

    vftables: dict[int, list[str]] = defaultdict(list)
    vbtables: dict[int, list[str]] = defaultdict(list)
    functions = image.image_base + functions_rva
    names = image.image_base + names_rva
    ordinals = image.image_base + ordinals_rva

    for index in range(name_count):
        name_rva = image.read_u32(names + index * 4)
        ordinal = _read_u16(image, ordinals + index * 2)
        if name_rva is None or ordinal is None or ordinal >= function_count:
            continue
        name = image.read_cstring(image.image_base + name_rva)
        function_rva = image.read_u32(functions + ordinal * 4)
        if not name or function_rva is None:
            continue
        address = image.image_base + function_rva
        if name.startswith("??_7"):
            vftables[address].append(name)
        elif name.startswith("??_8"):
            vbtables[address].append(name)

    return {"vftables": dict(vftables), "vbtables": dict(vbtables)}


def _source_vtable_markers(repo_dir: Path) -> dict[int, list[str]]:
    """Reviewed WIZ8 vtable addresses already asserted by source annotations."""

    markers: dict[int, list[str]] = defaultdict(list)
    for root_name in ("src/wiz8", "include/wiz8"):
        root = repo_dir / root_name
        if not root.is_dir():
            continue
        for path in sorted(item for item in root.rglob("*") if item.is_file()):
            if path.suffix.casefold() not in _SOURCE_SUFFIXES:
                continue
            for line_number, line in enumerate(
                path.read_text(encoding="utf-8", errors="replace").splitlines(), start=1
            ):
                match = _SOURCE_VTABLE_RE.match(line)
                if match:
                    markers[int(match.group(1), 16)].append(
                        f"{path.relative_to(repo_dir).as_posix()}:{line_number}"
                    )
    return dict(markers)


def _hex(address: int) -> str:
    return f"0x{address:08x}"


def _write_row(store: TableStore) -> dict[str, Any]:
    return {
        "instruction": _hex(store.instruction),
        "relocation_site": _hex(store.relocation_site),
        "base_register": store.base_register,
        "index_register": store.index_register,
        "scale": store.scale,
        "displacement": store.displacement,
        "object_offset": store.object_offset,
        "absolute_destination": store.absolute_destination,
    }


def _candidate_row(address: int, references: list[int], size: int, kind: str) -> dict[str, Any]:
    return {
        "address": _hex(address),
        "kind": kind,
        "entry_count": size,
        "references": [_hex(site) for site in references],
    }


def _oracle_missing(oracle: dict[int, list[str]], detected: set[int]) -> list[dict[str, Any]]:
    return [
        {"address": _hex(address), "names": names}
        for address, names in sorted(oracle.items())
        if address not in detected
    ]


def scan_msvc_tables(path: Path, *, repo_dir: Path | None = None) -> dict[str, Any]:
    """Recover structurally confirmed MSVC vftables/vbtables from one PE image."""

    image = PeImage(path)
    sites = relocation_sites(image)
    relocated = set(sites)
    references = _code_data_references(image, sites)
    boundaries = set(references)
    export_oracle = _exported_table_oracle(image)

    stores: list[TableStore] = []
    for site in sites:
        site_section = image.section_at(site)
        if site_section is None or not site_section.executable:
            continue
        store = _decode_table_store(image, site)
        if store is None:
            continue
        target_section = image.section_at(store.target)
        if target_section is None or target_section.name != ".rdata":
            continue
        stores.append(store)

    stores_by_target: dict[int, list[TableStore]] = defaultdict(list)
    for store in stores:
        stores_by_target[store.target].append(store)

    structural_vftables: dict[int, list[int]] = {}
    structural_vbtables: dict[int, list[int]] = {}
    for target in sorted(references):
        section = image.section_at(target)
        if section is None or section.name != ".rdata":
            continue
        slots = _decode_vftable(image, target, relocated, boundaries)
        if slots:
            structural_vftables[target] = slots
            continue
        entries = _decode_vbtable(image, target, relocated, boundaries)
        if entries:
            structural_vbtables[target] = entries

    vftables: list[dict[str, Any]] = []
    confirmed_vftable_addresses: set[int] = set()
    unconfirmed_vftables: list[dict[str, Any]] = []
    for target, slots in sorted(structural_vftables.items()):
        target_stores = [
            store for store in stores_by_target.get(target, []) if not store.absolute_destination
        ]
        if not target_stores:
            unconfirmed_vftables.append(
                _candidate_row(target, references[target], len(slots), "vftable")
            )
            continue
        confirmed_vftable_addresses.add(target)
        vftables.append(
            {
                "address": _hex(target),
                "slot_count": len(slots),
                "slots": [
                    {"index": index, "target": _hex(slot)} for index, slot in enumerate(slots)
                ],
                "writes": [_write_row(store) for store in target_stores],
                "references": [_hex(site) for site in references[target]],
                "export_names": export_oracle["vftables"].get(target, []),
            }
        )

    vbtables: list[dict[str, Any]] = []
    confirmed_vbtable_addresses: set[int] = set()
    unconfirmed_vbtables: list[dict[str, Any]] = []
    for target, entries in sorted(structural_vbtables.items()):
        matching_stores = [
            store
            for store in stores_by_target.get(target, [])
            if _vbtable_store_matches(entries, store)
        ]
        if not matching_stores:
            unconfirmed_vbtables.append(
                _candidate_row(target, references[target], len(entries), "vbtable")
            )
            continue
        confirmed_vbtable_addresses.add(target)
        writes = []
        for store in matching_stores:
            row = _write_row(store)
            row["owner_subobject_offset_hint"] = _vbtable_owner_offset_hint(entries, store)
            writes.append(row)
        vbtables.append(
            {
                "address": _hex(target),
                "entry_count": len(entries),
                "entries": [
                    {"index": index, "displacement": displacement}
                    for index, displacement in enumerate(entries)
                ],
                "writes": writes,
                "references": [_hex(site) for site in references[target]],
                "export_names": export_oracle["vbtables"].get(target, []),
            }
        )

    classified_targets = set(structural_vftables) | set(structural_vbtables)
    non_table_rdata_stores = [
        {
            "address": _hex(target),
            "writes": [_write_row(store) for store in target_stores],
        }
        for target, target_stores in sorted(stores_by_target.items())
        if target not in classified_targets
    ]

    marker_rows: list[dict[str, Any]] = []
    missing_markers: list[dict[str, Any]] = []
    marker_count: int | None = None
    if repo_dir is not None:
        markers = _source_vtable_markers(repo_dir)
        marker_count = len(markers)
        for address, locations in sorted(markers.items()):
            row = {"address": _hex(address), "locations": locations}
            if address in confirmed_vftable_addresses:
                marker_rows.append(row)
            else:
                missing_markers.append(row)

    exported_vftables = export_oracle["vftables"]
    exported_vbtables = export_oracle["vbtables"]
    missing_structural_exported_vftables = _oracle_missing(
        exported_vftables, set(structural_vftables)
    )
    missing_structural_exported_vbtables = _oracle_missing(
        exported_vbtables, set(structural_vbtables)
    )
    missing_confirmed_exported_vftables = _oracle_missing(
        exported_vftables, confirmed_vftable_addresses
    )
    missing_confirmed_exported_vbtables = _oracle_missing(
        exported_vbtables, confirmed_vbtable_addresses
    )

    wrong_kind_exports = [
        {"address": _hex(address), "expected": "vftable", "names": names}
        for address, names in sorted(exported_vftables.items())
        if address in structural_vbtables
    ] + [
        {"address": _hex(address), "expected": "vbtable", "names": names}
        for address, names in sorted(exported_vbtables.items())
        if address in structural_vftables
    ]

    validation_ok = not (
        unconfirmed_vftables
        or unconfirmed_vbtables
        or missing_markers
        or missing_structural_exported_vftables
        or missing_structural_exported_vbtables
        or missing_confirmed_exported_vftables
        or missing_confirmed_exported_vbtables
        or wrong_kind_exports
    )
    reviewed_found = len(marker_rows) if marker_count is not None else None
    reviewed_recall = (
        reviewed_found / marker_count
        if marker_count not in (None, 0) and reviewed_found is not None
        else None
    )

    return {
        "schema": "wiz8.msvc-table-census",
        "binary": str(path),
        "scope": (
            "PE-relocation census of code-referenced .rdata tables; every code-referenced datum "
            "bounds table runs, and only memory-installed structural tables are confirmed"
        ),
        "summary": {
            "relocations": len(sites),
            "code_referenced_data_addresses": len(references),
            "rdata_immediate_stores": len(stores),
            "rdata_immediate_store_targets": len(stores_by_target),
            "vftables": len(vftables),
            "vbtables": len(vbtables),
            "unconfirmed_vftable_candidates": len(unconfirmed_vftables),
            "unconfirmed_vbtable_candidates": len(unconfirmed_vbtables),
            "non_table_rdata_store_targets": len(non_table_rdata_stores),
        },
        "validation": {
            "status": "passed" if validation_ok else "needs-review",
            "reviewed_source_vtables": marker_count,
            "reviewed_source_vtables_found": reviewed_found,
            "reviewed_source_vtable_recall": reviewed_recall,
            "missing_reviewed_vtables": missing_markers,
            "export_oracle": {
                "exported_vftables": len(exported_vftables),
                "exported_vbtables": len(exported_vbtables),
                "missing_structural_vftables": missing_structural_exported_vftables,
                "missing_structural_vbtables": missing_structural_exported_vbtables,
                "missing_confirmed_vftables": missing_confirmed_exported_vftables,
                "missing_confirmed_vbtables": missing_confirmed_exported_vbtables,
                "wrong_kind": wrong_kind_exports,
            },
        },
        "vftables": vftables,
        "vbtables": vbtables,
        "unconfirmed_vftable_candidates": unconfirmed_vftables,
        "unconfirmed_vbtable_candidates": unconfirmed_vbtables,
        "non_table_rdata_stores": non_table_rdata_stores,
    }
