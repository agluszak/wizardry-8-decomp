"""Stamp reviewed-fact provenance into the program itself.

Ghidra owns the reviewed program objects as functions, names, classes and data
types. The tracked ledger still owns why a conclusion was accepted and at what
authority. This replay step writes compact address-indexed provenance properties
into the canonical project:

    wiz8.layer      exact | reviewed
    wiz8.evidence   file:line|file:line

Anchors are the natural address of each fact: a function fact at its entry, a
vtable fact at the table, a class fact at its primary vtable (or constructor
when no vtable is reviewed), and a boundary row at its function address. Facts
with no address anchor are not forced into one.
"""

from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Any

from ..config import Settings
from .project import resolve_program_name

_LAYER = "wiz8.layer"
_EVIDENCE = "wiz8.evidence"


def _tier(confidence: str) -> str:
    return "exact" if confidence.strip() == "exact" else "reviewed"


def _rows(path: Path) -> list[tuple[int, dict[str, str]]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(enumerate(csv.DictReader(stream), start=2))


def _collect_anchors(repo: Path) -> dict[str, dict[str, Any]]:
    """address -> {layer, citations} from every reviewed channel with an anchor."""
    anchors: dict[str, dict[str, Any]] = {}

    def stamp(address: str, tier: str, citation: str) -> None:
        address = address.strip().lower()
        if not address:
            return
        entry = anchors.setdefault(address, {"layer": tier, "citations": []})
        entry["citations"].append(citation)
        if tier == "exact":
            entry["layer"] = "exact"

    reviewed = repo / "evidence" / "reviewed" / "wiz8"
    for line, row in _rows(reviewed / "functions.csv"):
        if row["address"]:
            stamp(
                row["address"],
                _tier(row["confidence"]),
                f"evidence/reviewed/wiz8/functions.csv:{line}",
            )

    vtable_addresses: dict[str, str] = {}
    for line, row in _rows(reviewed / "vtables.csv"):
        vtable_addresses[row["vtable_id"]] = row["address"]
        stamp(
            row["address"],
            _tier(row["confidence"]),
            f"evidence/reviewed/wiz8/vtables.csv:{line}",
        )

    for line, row in _rows(reviewed / "classes.csv"):
        anchor = vtable_addresses.get(row["primary_vtable_id"], "") or row["constructor"]
        if anchor:
            stamp(
                anchor,
                _tier(row["confidence"]),
                f"evidence/reviewed/wiz8/classes.csv:{line}",
            )

    for line, row in _rows(repo / "config" / "reccmp" / "wiz8-gameplay-boundaries.csv"):
        stamp(
            row["address"],
            _tier(row["confidence"]),
            f"config/reccmp/wiz8-gameplay-boundaries.csv:{line}",
        )

    return anchors


def apply_provenance(
    settings: Settings,
    selector: str,
    *,
    materialize: bool = True,
) -> dict[str, Any]:
    from .cache import open_for_mutation

    settings = open_for_mutation(settings, selector, materialize=materialize)
    import pyghidra

    anchors = _collect_anchors(settings.repo_dir)
    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    stats = {"anchors": len(anchors), "stamped": 0, "unmapped": 0}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("wiz8 provenance")
            try:
                manager = program.getUsrPropertyManager()
                layer_map = manager.getStringPropertyMap(_LAYER) or manager.createStringPropertyMap(
                    _LAYER
                )
                evidence_map = manager.getStringPropertyMap(
                    _EVIDENCE
                ) or manager.createStringPropertyMap(_EVIDENCE)
                factory = program.getAddressFactory().getDefaultAddressSpace()
                for address_text, entry in sorted(anchors.items()):
                    address = factory.getAddress(address_text)
                    if address is None or not program.getMemory().contains(address):
                        stats["unmapped"] += 1
                        continue
                    layer_map.add(address, entry["layer"])
                    evidence_map.add(address, "|".join(entry["citations"]))
                    stats["stamped"] += 1
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            program.save("wiz8 provenance", None)
    finally:
        project.close()
    return stats


def facts_at(program: Any, argument: str) -> dict[str, Any]:
    """Return every wiz8.* property at an address with its program context."""
    from .query import _address

    address = _address(program, argument)
    manager = program.getUsrPropertyManager()
    properties: dict[str, Any] = {}
    decoded: dict[str, Any] = {}
    names = manager.propertyManagers()
    while names.hasNext():
        name = names.next()
        if not str(name).startswith("wiz8."):
            continue
        property_map = manager.getPropertyMap(str(name))
        if property_map is None or not property_map.hasProperty(address):
            continue
        value = property_map.get(address)
        properties[str(name)] = str(value) if value is not None else None
        if value is not None:
            try:
                decoded[str(name)] = json.loads(str(value))
            except json.JSONDecodeError:
                pass
    function = program.getFunctionManager().getFunctionContaining(address)
    symbol = program.getSymbolTable().getPrimarySymbol(address)
    return {
        "address": str(address),
        "properties": properties,
        "decoded_properties": decoded,
        "citations": (
            properties.get(_EVIDENCE, "").split("|") if properties.get(_EVIDENCE) else []
        ),
        "function": str(function.getEntryPoint()) if function is not None else None,
        "function_name": function.getName() if function is not None else None,
        "symbol": symbol.getName(True) if symbol is not None else None,
    }
