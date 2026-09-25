"""Wizardry-driven SurRender relevance frontier.

Joins the audited WIZ8 import table (``wiz8-sr-imports.csv``) with the live
Wiz8 and sr.dll ProgramDBs and the SURRENDER source index:

- every import's IAT cell is resolved to its actual Wiz8 references and the
  functions containing them, split into call sites vs data reads. A reference
  from an import thunk's own ``jmp [iat]`` body is not a use: P0 requires a
  non-thunk reference or a real caller of the thunk;
- imported data (``srVectorProcessor::vp`` and friends) is followed through
  straight-line ``mov reg,[iat]`` load chains to indirect ``call [reg+off]``
  slot sites. The register-provenance scan is intra-linear-run only (it stops
  at jumps, calls, and branch targets) and is reported as heuristic;
- each imported symbol is mapped to its sr.dll export and checked against the
  SURRENDER marker set for provider-side recovery status;
- direct call targets of referenced provider bodies become the dependency
  seeds: ``CALL`` targets plus ``JMP`` tailcalls that leave the current
  function body. Ordinary intra-function jumps are not dependencies.

The report is generated, not maintained: rerun ``wiz8 report
surrender-frontier`` after source or analysis changes. ``--class`` and
``--priority`` filter the per-import listing; the summary always covers all
imports.
"""

from __future__ import annotations

import csv
import re
from collections import defaultdict
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from ..config import Settings
from ..ghidra.env import open_program
from ..source_index import (
    declarations_by_semantic_key,
    load_source_index,
    source_functions,
    warn_if_source_index_may_be_stale,
)
from ..surrender_iat_typing import _imported_data_value_type, load_surrender_imports

_EXPORTS = Path("evidence/snapshots/surrender-abi/exports.csv")
_SCHEMA = "wiz8.surrender-frontier-v2"
_CALL_MNEMONICS = frozenset({"CALL", "JMP"})
_EMISSION_KINDS = frozenset({"SYNTHETIC", "TEMPLATE"})
_CALLABLE_KINDS = frozenset(
    {"method", "constructor", "destructor", "operator", "vbase-destructor", "free-function"}
)
_DATA_KINDS = frozenset({"global-object", "vftable", "vbtable"})
# Ghidra OperandType: memory reads are DYNAMIC (0x400000) or ADDRESS|INDIRECT
# (0x2080). A plain register source has neither bit set.
_MEMORY_OPERAND_MASK = 0x400000 | 0x2000 | 0x80
_VIRTUAL_DECL = re.compile(r"^\s*virtual\b.*?\b(~?\w+)\s*\(")
# Flat proven interface: vp's slots match srVP.h virtual declaration order.
_SLOT_NAME_CLASSES = frozenset({"srVP"})


@dataclass(frozen=True)
class FrontierInsn:
    """ProgramDB-independent instruction shape the flow scan consumes.

    ``operands`` holds per-operand object tuples tagged ``reg``/``scalar``/
    ``addr``; ``results`` lists written register names; ``branch_target``
    marks an instruction that an intra-function jump can reach.
    """

    address: int
    mnemonic: str
    operands: tuple[tuple[tuple[str, Any], ...], ...] = ()
    results: tuple[str, ...] = ()
    branch_target: bool = False
    memory_operands: frozenset[int] = frozenset()


def _op_register(ins: FrontierInsn, operand: int) -> str | None:
    if operand >= len(ins.operands):
        return None
    for kind, value in ins.operands[operand]:
        if kind == "reg":
            return str(value)
    return None


def _op_scalar(ins: FrontierInsn, operand: int) -> int | None:
    if operand >= len(ins.operands):
        return None
    for kind, value in ins.operands[operand]:
        if kind == "scalar":
            return int(value)
    return None


def _op_address(ins: FrontierInsn, operand: int) -> int | None:
    if operand >= len(ins.operands):
        return None
    for kind, value in ins.operands[operand]:
        if kind == "addr":
            return int(value)
    return None


def _op_is_memory(ins: FrontierInsn, operand: int) -> bool:
    """Operand touches memory: dynamic ``[reg+disp]`` forms or an absolute address."""

    return operand in ins.memory_operands or _op_address(ins, operand) is not None


def _breaks_flow(mnemonic: str) -> bool:
    """Instruction ends a straight-line provenance run for our purposes."""

    return (
        mnemonic in _CALL_MNEMONICS
        or mnemonic.startswith(("J", "RET"))
        or mnemonic in {"INT", "IRET", "SYSCALL"}
    )


def indirect_call_sites(
    instructions: Iterable[FrontierInsn], cell_offset: int
) -> list[dict[str, Any]]:
    """Heuristic straight-line scan for ``mov reg,[cell]``-rooted indirect calls.

    Tracks ``mov rX, [cell]`` (depth 0), ``mov rX, [rY]`` derefs (depth+1), and
    register copies. A ``call/jmp [rX+off]`` through a register at depth >= 1 is
    a call through the imported object - for ``vp`` an ``srVP`` slot. Register
    facts never cross jumps, calls, or branch targets, so a site can only be
    reported when the whole load chain sits in one linear run; results remain a
    heuristic approximation, not CFG dataflow.
    """

    provenance: dict[str, int] = {}
    sites: list[dict[str, Any]] = []
    for ins in instructions:
        if ins.branch_target:
            provenance.clear()
        mnemonic = ins.mnemonic
        if mnemonic in _CALL_MNEMONICS:
            base = _op_register(ins, 0)
            if base is not None and provenance.get(base, 0) >= 1:
                offset = _op_scalar(ins, 0)
                sites.append(
                    {
                        "address": f"0x{ins.address:08x}",
                        "via_register": base,
                        "depth": provenance[base],
                        "slot_offset": (f"0x{offset:x}" if offset is not None else None),
                        "slot_index": (offset // 4 if offset is not None else None),
                    }
                )
            provenance.clear()
            continue
        if _breaks_flow(mnemonic):
            provenance.clear()
            continue
        dest = _op_register(ins, 0)
        if mnemonic in {"MOV", "MOVSX", "MOVZX"} and dest is not None:
            if _op_address(ins, 1) == cell_offset:
                provenance[dest] = 0
            else:
                src = _op_register(ins, 1)
                if src is not None and src in provenance:
                    is_memory = _op_is_memory(ins, 1)
                    provenance[dest] = provenance[src] + (1 if is_memory else 0)
                else:
                    provenance.pop(dest, None)
        else:
            for result in ins.results:
                provenance.pop(result, None)
    return sites


def provider_call_targets(
    instructions: Iterable[FrontierInsn], body_min: int, body_max: int
) -> list[int]:
    """Direct dependencies of one provider body.

    ``CALL`` operands always count. A ``JMP`` counts only as a tailcall: its
    absolute target must leave the current function body. Intra-function jumps
    are control flow, not dependencies.
    """

    targets: list[int] = []
    for ins in instructions:
        address = _op_address(ins, 0)
        if ins.mnemonic == "CALL":
            if address is not None:
                targets.append(address)
        elif (
            ins.mnemonic == "JMP" and address is not None and not (body_min <= address <= body_max)
        ):
            targets.append(address)
    return targets


def classify_priority(sites: Iterable[dict[str, Any]], thunk_callers: Iterable[str]) -> str:
    """P0 requires a real Wiz8 reference or a caller of the import thunk."""

    if any(site["kind"] != "thunk" for site in sites):
        return "P0"
    if list(thunk_callers):
        return "P0"
    return "P3"


def qualified_member_name(signature: str | None) -> str | None:
    """Qualified ``Scope::Class::member`` name from a demangled signature.

    Handles special members structurally: ``srConfig::srConfig`` for
    constructors, ``~``-prefixed destructors, and ``operator`` spellings all
    come out of the signature text rather than the decorated name.
    """

    if not signature:
        return None
    text = signature.strip()
    paren = text.find("(")
    if paren < 0:
        # Data export: the trailing token is the qualified name when the
        # signature names a member (e.g. ``class srVP * srVectorProcessor::vp``).
        tail = text.rsplit(None, 1)[-1].lstrip("*&")
        return tail if "::" in tail else None
    prefix = text[:paren]
    # The qualified name is the final whitespace-free (at angle depth 0) token.
    depth = 0
    start = 0
    for index, char in enumerate(prefix):
        if char == "<":
            depth += 1
        elif char == ">":
            depth = max(0, depth - 1)
        elif char.isspace() and depth == 0:
            start = index + 1
    name = prefix[start:].strip()
    return name or None


def _load_exports(repository: Path) -> dict[str, dict[str, str]]:
    """Canonical sr.dll export snapshot keyed by decorated name."""

    path = repository / _EXPORTS
    rows: dict[str, dict[str, str]] = {}
    if not path.is_file():
        return rows
    with path.open(encoding="utf-8", newline="") as handle:
        for row in csv.DictReader(handle):
            if row.get("module") != "sr.dll" or "--gog-base--sr--" not in str(
                row.get("program") or ""
            ):
                continue
            name = str(row.get("decorated_name") or "")
            if name:
                rows[name] = row
    return rows


def _declared_names(document: Any) -> dict[str, set[str]]:
    """Qualified member name → source-index targets that declare it.

    Shared provider headers such as ``srGERD.h`` are currently included only
    by WIZ8 consumer TUs, so their declarations index under ``WIZ8`` even
    though the provider owns the interface. Both targets count as
    declarations; ``declared_in`` records which side carries them.
    """

    names: dict[str, set[str]] = {}
    for (entry_target, _key, _unit), entry in declarations_by_semantic_key(document).items():
        qualified = str(entry.get("qualified_name") or "")
        if qualified:
            names.setdefault(qualified, set()).add(str(entry_target))
    return names


def _vtable_addresses(document: Any) -> set[int]:
    """Retail addresses owned by provider ``VTABLE`` markers.

    reccmp joins ``VTABLE`` markers onto the indexed class record rather than
    the marker collection, so vftable imports never appear in
    ``source_functions``; their ownership is the class entry's
    ``vtable_address``/``base_vtables`` instead.
    """

    addresses: set[int] = set()
    for entry in document.get("classes") or []:
        if str(entry.get("target") or "").upper() != "SURRENDER":
            continue
        primary = entry.get("vtable_address")
        if primary is not None:
            addresses.add(int(primary))
        for base in entry.get("base_vtables") or []:
            address = base.get("address")
            if address is not None:
                addresses.add(int(address))
    return addresses


def _defined_globals(document: Any) -> set[str]:
    """Decorated names of globals the provider defines.

    ``GLOBAL`` markers join to the indexed variable definition rather than
    the marker collection, so ``?name@@3...`` imports are owned by
    ``variables`` entries whose ``definition_kind`` is ``definition``.
    """

    names: set[str] = set()
    for entry in document.get("variables") or []:
        if str(entry.get("target") or "").upper() != "SURRENDER":
            continue
        if str(entry.get("definition_kind") or "") != "definition":
            continue
        names.add(str(entry.get("semantic_id") or ""))
    return names


def _provider_status(
    address: int | None,
    markers: Any,
    declared: dict[str, set[str]],
    qualified_name: str | None,
    decorated_name: str | None = None,
    vtables: set[int] | None = None,
    defined_globals: set[str] | None = None,
) -> str:
    if address is None:
        return "no-export"
    marker = markers.get(address)
    if marker is not None:
        kind = str(marker.marker_kind or "FUNCTION")
        if kind == "FUNCTION":
            return "recovered"
        if kind in _EMISSION_KINDS:
            return "emission"
        return kind.lower()
    if decorated_name and decorated_name.startswith("??_7"):
        if vtables is not None and address in vtables:
            return "vtable"
    elif decorated_name and defined_globals is not None and decorated_name in defined_globals:
        return "global"
    if qualified_name and qualified_name in declared:
        return "declared"
    return "unowned"


def _ghidra_insn(instruction: Any, branch_targets: set[int]) -> FrontierInsn:
    from ghidra.program.model.address import Address  # type: ignore[import-not-found]
    from ghidra.program.model.lang import Register  # type: ignore[import-not-found]
    from ghidra.program.model.scalar import Scalar  # type: ignore[import-not-found]

    operands: list[tuple[tuple[str, Any], ...]] = []
    memory_operands: set[int] = set()
    for index in range(instruction.getNumOperands()):
        objects: list[tuple[str, Any]] = []
        for obj in instruction.getOpObjects(index):
            if isinstance(obj, Register):
                objects.append(("reg", obj.getName()))
            elif isinstance(obj, Scalar):
                objects.append(("scalar", int(obj.getUnsignedValue())))
            elif isinstance(obj, Address):
                objects.append(("addr", int(obj.getOffset())))
        operands.append(tuple(objects))
        if instruction.getOperandType(index) & _MEMORY_OPERAND_MASK:
            memory_operands.add(index)
    results = tuple(
        obj.getName() for obj in instruction.getResultObjects() if isinstance(obj, Register)
    )
    address = int(instruction.getAddress().getOffset())
    return FrontierInsn(
        address=address,
        mnemonic=instruction.getMnemonicString(),
        operands=tuple(operands),
        results=results,
        branch_target=address in branch_targets,
        memory_operands=frozenset(memory_operands),
    )


def _function_instructions(program: Any, function: Any) -> list[FrontierInsn]:
    """Adapted instruction list with intra-function branch targets marked."""

    listing = program.getListing()
    body = function.getBody()
    instructions = list(listing.getInstructions(body, True))
    targets: set[int] = set()
    for inst in instructions:
        for flow in inst.getFlows():
            if body.contains(flow):
                targets.add(int(flow.getOffset()))
        # Fall-through into the next instruction is not a branch target.
        fall = inst.getFallThrough()
        if fall is not None and body.contains(fall):
            targets.discard(int(fall.getOffset()))
    return [_ghidra_insn(inst, targets) for inst in instructions]


def _wiz8_usage(
    program: Any, row: dict[str, str]
) -> tuple[list[dict[str, Any]], list[str], list[dict[str, Any]]]:
    """IAT references, resolved caller names, and indirect slot calls.

    A reference whose containing function is an import thunk is recorded as
    ``thunk``: it is the thunk's own ``jmp [iat]`` body, not a use. Real thunk
    callers are found by following references to the thunk entry point.
    """

    space = program.getAddressFactory().getDefaultAddressSpace()
    listing = program.getListing()
    manager = program.getFunctionManager()
    iat = int(row["iat_address"], 16)
    references = program.getReferenceManager().getReferencesTo(space.getAddress(iat))
    sites: list[dict[str, Any]] = []
    functions: dict[int, Any] = {}
    while references.hasNext():
        ref = references.next()
        source = ref.getFromAddress()
        instruction = listing.getInstructionAt(source)
        owner = manager.getFunctionContaining(source)
        mnemonic = instruction.getMnemonicString() if instruction is not None else None
        if owner is not None and owner.isThunk():
            kind = "thunk"
        elif mnemonic in _CALL_MNEMONICS:
            kind = "call"
        else:
            kind = "data"
        sites.append(
            {
                "address": f"0x{source.getOffset():08x}",
                "kind": kind,
                "function": owner.getName() if owner is not None else None,
                "function_entry": (
                    f"0x{owner.getEntryPoint().getOffset():08x}" if owner is not None else None
                ),
            }
        )
        if owner is None:
            continue
        if owner.isThunk():
            thunk_refs = program.getReferenceManager().getReferencesTo(owner.getEntryPoint())
            while thunk_refs.hasNext():
                thunk_ref = thunk_refs.next()
                caller = manager.getFunctionContaining(thunk_ref.getFromAddress())
                if caller is not None and not caller.isThunk():
                    functions[caller.getEntryPoint().getOffset()] = caller
        else:
            functions[owner.getEntryPoint().getOffset()] = owner

    signature = str(row.get("demangled_signature") or "")
    is_data = row.get("kind") in _DATA_KINDS or "(" not in signature
    indirect_sites: list[dict[str, Any]] = []
    if is_data:
        for function in functions.values():
            if function.isThunk() or function.isExternal():
                continue
            for site in indirect_call_sites(_function_instructions(program, function), iat):
                sites_entry = dict(site)
                sites_entry["function"] = function.getName()
                sites_entry["function_entry"] = f"0x{function.getEntryPoint().getOffset():08x}"
                indirect_sites.append(sites_entry)
    callers = sorted(function.getName() for function in functions.values())
    return sites, callers, indirect_sites


def _slot_names(repository: Path, type_spelling: str | None) -> dict[int, str]:
    """Vtable slot names for the flat ``srVP`` interface only.

    Declaration order equals slot order for ``srVP`` specifically (validated
    against the retail vtable); this shortcut must not be applied to classes
    with inheritance, destructors, or vbases.
    """

    if not type_spelling:
        return {}
    name = type_spelling.strip().rstrip("*").strip()
    if name not in _SLOT_NAME_CLASSES:
        return {}
    header = repository / "include" / "surrender" / f"{name}.h"
    if not header.is_file():
        return {}
    slots: dict[int, str] = {}
    for line in header.read_text(encoding="utf-8", errors="replace").splitlines():
        match = _VIRTUAL_DECL.match(line)
        if match is None:
            continue
        slots[len(slots)] = match.group(1)
    return slots


def _provider_callees(
    program: Any, function: Any, markers: Any, declared: dict[str, set[str]]
) -> list[dict[str, Any]]:
    """Direct dependencies of one provider body (calls + true tailcalls)."""

    manager = program.getFunctionManager()
    imagebase = program.getImageBase().getOffset()
    body = function.getBody()
    body_min = int(body.getMinAddress().getOffset())
    body_max = int(body.getMaxAddress().getOffset())
    space = program.getAddressFactory().getDefaultAddressSpace()
    seen: dict[int, dict[str, Any]] = {}
    for offset in provider_call_targets(
        _function_instructions(program, function), body_min, body_max
    ):
        callee = manager.getFunctionAt(space.getAddress(offset))
        if callee is None:
            callee = manager.getFunctionContaining(space.getAddress(offset))
        if callee is not None and (
            callee.getEntryPoint().getOffset() == function.getEntryPoint().getOffset()
        ):
            continue
        entry: dict[str, Any] = seen.get(offset) or {
            "address": f"0x{offset:08x}",
            "rva": (f"0x{offset - imagebase:x}" if offset >= imagebase else None),
            "name": None,
            "external": False,
            "status": "unowned",
        }
        if callee is not None:
            if callee.isThunk():
                thunked = callee.getThunkedFunction(True)
                if thunked is not None:
                    callee = thunked
            entry["name"] = callee.getName(True)
            entry["external"] = bool(callee.isExternal())
            entry["status"] = _provider_status(
                callee.getEntryPoint().getOffset(), markers, declared, None
            )
        seen[offset] = entry
    return [seen[key] for key in sorted(seen)]


def surrender_frontier_report(
    settings: Settings,
    *,
    class_filter: str | None = None,
    priority_filter: str | None = None,
    compare: bool = True,
) -> dict[str, Any]:
    repository = settings.repo_dir
    warn_if_source_index_may_be_stale(repository, "SURRENDER")

    imports = load_surrender_imports(repository)
    exports = _load_exports(repository)
    document = load_source_index(repository)
    markers = source_functions(repository, "SURRENDER")
    declared = _declared_names(document)
    vtables = _vtable_addresses(document)
    defined_globals = _defined_globals(document)

    usage: list[tuple[list[dict[str, Any]], list[str], list[dict[str, Any]]]] = []
    with open_program(settings, "wiz8") as wiz8:
        for row in imports:
            usage.append(_wiz8_usage(wiz8, row))

    provider: list[dict[str, Any]] = []
    with open_program(settings, "sr.dll") as sr:
        imagebase = sr.getImageBase().getOffset()
        manager = sr.getFunctionManager()
        space = sr.getAddressFactory().getDefaultAddressSpace()
        for row, (sites, _callers, _indirect) in zip(imports, usage, strict=True):
            export = exports.get(row["decorated_name"])
            signature = str(
                (export or {}).get("demangled_signature") or row.get("demangled_signature") or ""
            )
            entry: dict[str, Any] = {"qualified_name": qualified_member_name(signature)}
            if export is None:
                entry["status"] = "no-export"
            else:
                rva = int(str(export["rva"]), 16)
                address = imagebase + rva
                entry["rva"] = f"0x{rva:x}"
                entry["address"] = f"0x{address:08x}"
                entry["export_kind"] = export.get("kind")
                entry["status"] = _provider_status(
                    address,
                    markers,
                    declared,
                    entry["qualified_name"],
                    row["decorated_name"],
                    vtables,
                    defined_globals,
                )
                if entry["status"] == "declared":
                    targets = declared.get(entry["qualified_name"]) or set()
                    if "SURRENDER" not in targets:
                        entry["declared_in"] = "consumer-header"
                if sites and str(export.get("kind") or "") in _CALLABLE_KINDS:
                    function = manager.getFunctionAt(space.getAddress(address))
                    if function is not None and not function.isExternal():
                        entry["direct_callees"] = _provider_callees(sr, function, markers, declared)
            provider.append(entry)

    records: list[dict[str, Any]] = []
    for row, (sites, callers, indirect_sites), provider_entry in zip(
        imports, usage, provider, strict=True
    ):
        real_sites = [site for site in sites if site["kind"] != "thunk"]
        value_type = _imported_data_value_type(str(row.get("demangled_signature") or ""))
        slot_names = _slot_names(repository, value_type) if indirect_sites else {}
        for site in indirect_sites:
            index = site.pop("slot_index", None)
            if index is not None and index in slot_names:
                site["slot_name"] = slot_names[index]
            site["heuristic"] = True
        record = {
            "decorated_name": row["decorated_name"],
            "signature": row.get("demangled_signature") or None,
            "kind": row.get("kind"),
            # The import table prefixes static-member owners with '*'.
            "class_name": (row.get("class_name") or "").lstrip("*") or None,
            "iat_address": f"0x{int(row['iat_address'], 16):08x}",
            "priority": classify_priority(sites, callers),
            "usage": {
                "call_sites": sum(1 for site in sites if site["kind"] == "call"),
                "data_references": sum(1 for site in sites if site["kind"] == "data"),
                "thunk_only": not real_sites,
                "wiz8_callers": callers,
                "references": sites,
                "indirect_calls": indirect_sites,
            },
            "provider": provider_entry,
        }
        records.append(record)

    # reccmp enrichment: per-import match status for emitted provider bodies.
    compare_available: bool | None = None
    if compare:
        compared = [
            int(str(record["provider"]["address"]), 16)
            for record in records
            if record["provider"].get("status") in {"recovered", "emission"}
            and record["provider"].get("address")
        ]
        if compared:
            try:
                from ..comparison import compare_selected

                result = compare_selected(
                    repository,
                    "SURRENDER",
                    sorted(set(compared)),
                    include_windows=False,
                )
                by_address = {
                    str(item.get("address")): item.get("status")
                    for item in result.get("functions") or []
                }
                for record in records:
                    address = record["provider"].get("address")
                    if address in by_address:
                        record["provider"]["match_status"] = by_address[address]
                compare_available = True
            except FileNotFoundError:
                compare_available = False

    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for record in records:
        grouped[record["class_name"] or "(free)"].append(record)
    classes: dict[str, dict[str, Any]] = {}
    for name, members in sorted(grouped.items()):
        p0 = [member for member in members if member["priority"] == "P0"]
        statuses = [member["provider"].get("status") for member in p0]
        deps: dict[str, str] = {}
        for member in p0:
            for callee in member["provider"].get("direct_callees") or []:
                if callee["external"]:
                    continue
                deps[callee["name"] or callee["address"]] = callee["status"]
        classes[name] = {
            "wiz8_imports": len(members),
            "imports_with_wiz8_xrefs": len(p0),
            "provider_recovered": statuses.count("recovered"),
            "provider_emission": statuses.count("emission"),
            "provider_global": statuses.count("global"),
            "provider_vtable": statuses.count("vtable"),
            "provider_declared": statuses.count("declared"),
            "provider_unowned": statuses.count("unowned"),
            "provider_no_export": statuses.count("no-export"),
            "wiz8_callers": sorted(
                {caller for member in p0 for caller in member["usage"]["wiz8_callers"]}
            ),
            "direct_provider_dependencies": dict(sorted(deps.items())),
            "priority": "P0" if p0 else "P3",
        }

    wanted_class = class_filter.casefold() if class_filter else None
    wanted_priority = priority_filter.upper() if priority_filter else None
    listed = [
        record
        for record in records
        if (wanted_class is None or (record["class_name"] or "(free)").casefold() == wanted_class)
        and (wanted_priority is None or record["priority"] == wanted_priority)
    ]

    p0_records = [record for record in records if record["priority"] == "P0"]
    p0_statuses = [record["provider"].get("status") for record in p0_records]
    named_classes = {name: info for name, info in classes.items() if name != "(free)"}
    summary = {
        "wiz8_imports": len(records),
        "p0_referenced": len(p0_records),
        "p3_no_real_xrefs": len(records) - len(p0_records),
        "p0_provider_recovered": p0_statuses.count("recovered"),
        "p0_provider_emission": p0_statuses.count("emission"),
        "p0_provider_global": p0_statuses.count("global"),
        "p0_provider_vtable": p0_statuses.count("vtable"),
        "p0_provider_declared": p0_statuses.count("declared"),
        "p0_provider_unowned": p0_statuses.count("unowned"),
        "p0_provider_no_export": p0_statuses.count("no-export"),
        "classes_with_wiz8_references": sum(
            1 for info in named_classes.values() if info["priority"] == "P0"
        ),
        "classes_direct_imports_covered": sum(
            1
            for info in named_classes.values()
            if info["priority"] == "P0"
            and info["provider_unowned"] == 0
            and info["provider_declared"] == 0
            and info["provider_no_export"] == 0
        ),
        "classes_untouched": sum(
            1
            for info in named_classes.values()
            if info["priority"] == "P0"
            and info["provider_recovered"] == 0
            and info["provider_emission"] == 0
            and info["provider_global"] == 0
            and info["provider_vtable"] == 0
        ),
        "p0_match": {
            status: sum(
                1 for record in p0_records if record["provider"].get("match_status") == status
            )
            for status in ("exact", "effective", "mismatch", "inconclusive", "missing")
        },
    }

    return {
        "schema": _SCHEMA,
        "informational": True,
        "compare_available": compare_available,
        "summary": summary,
        "classes": classes,
        "filters": {"class": class_filter, "priority": priority_filter},
        "imports": listed,
    }
