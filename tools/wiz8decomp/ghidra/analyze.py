from __future__ import annotations

from typing import Any


def rtti_inventory(program: Any) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    """Export RTTI-grounded type names and candidate vtables without applying names."""
    classes: dict[str, dict[str, Any]] = {}
    vtables: list[dict[str, Any]] = []
    listing = program.getListing()
    data_iterator = listing.getDefinedData(True)
    while data_iterator.hasNext():
        data = data_iterator.next()
        if data.hasStringValue():
            value = str(data.getValue())
            if value.startswith((".?AV", ".?AU")):
                classes[value] = {
                    "mangled_name": value,
                    "type_descriptor_string": str(data.getAddress()),
                    "evidence": "MSVC RTTI type-descriptor string",
                }
    symbols = program.getSymbolTable().getAllSymbols(True)
    while symbols.hasNext():
        symbol = symbols.next()
        name = symbol.getName(True)
        if "vftable" in name.casefold() or name.startswith("??_7"):
            vtables.append(
                {
                    "address": str(symbol.getAddress()),
                    "name": name,
                    "evidence": "Ghidra symbol name",
                }
            )
    return sorted(classes.values(), key=lambda item: item["mangled_name"]), sorted(
        vtables, key=lambda item: item["address"]
    )
