"""Callee-oriented ABI observations with their existing evidence origins."""

from __future__ import annotations

import hashlib
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..paths import atomic_json


def _signature(function: Any | None) -> dict[str, Any] | None:
    if function is None:
        return None

    def typed(variable: Any) -> dict[str, Any]:
        data_type = variable.getDataType()
        size = int(data_type.getLength())
        storage = variable.getVariableStorage()
        return {
            "type": str(data_type.getDisplayName()),
            "width": size if size > 0 and not storage.isUnassignedStorage() else None,
            "storage": str(storage),
        }

    parameters = [
        {"name": str(param.getName()), "auto": bool(param.isAutoParameter()), **typed(param)}
        for param in function.getParameters()
    ]
    return {
        "signature": str(function.getSignature()),
        "calling_convention": str(function.getCallingConventionName()),
        "signature_source": str(function.getSignatureSource()),
        "non_auto_parameter_count": sum(not param["auto"] for param in parameters),
        "parameters": parameters,
        "return": typed(function.getReturn()),
        "variadic": bool(function.hasVarArgs()),
        "custom_storage": bool(function.hasCustomVariableStorage()),
    }


def signature_census(retail: Any, repository: Path, directory: Path) -> dict[str, Any]:
    from reccmp.ghidra.signature_provenance import PROPERTY, independently_reviewed_signature

    from ..source_index import load_source_index, warn_if_source_index_may_be_stale
    from .inspect import open_saved_comparison_programs

    summary = json.loads((directory / "summary.json").read_text())
    manifest = json.loads((directory / "manifest.json").read_text())
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    if digest != summary["inputs"]["manifest_sha256"]:
        raise ValueError("ABI census summary and manifest use different catalogs")
    if str(retail.getExecutableSHA256()) != manifest["orig"]["sha256"]:
        raise ValueError("ABI census retail program and comparison use different binaries")
    index_stale = warn_if_source_index_may_be_stale(repository, summary["target"])
    index = load_source_index(repository)
    declarations = {
        (row["target"], row["semantic_id"], row.get("unit_id")): row
        for row in index["declarations"]
    }
    source = {}
    for marker in index["markers"]:
        if marker["target"] == summary["target"] and marker.get("declaration_key"):
            source[int(marker["address"])] = declarations.get(tuple(marker["declaration_key"]))
    calls = json.loads((directory / "direct-calls.json").read_text())
    if calls["manifest_sha256"] != digest:
        raise ValueError("ABI census call observations use a different catalog")
    outcomes = {row["orig"]: row["outcome"] for row in summary["functions"]}
    callers: dict[str, set[str]] = defaultdict(set)
    for row in calls["functions"]:
        if outcomes.get(row["address"]) != "differences":
            continue
        for side in ("orig", "recomp"):
            for call in row[side]["calls"] or []:
                callers[call["identity"]].add(row["address"])
    rows = []
    with open_saved_comparison_programs(summary, manifest) as programs:
        stamps = retail.getUsrPropertyManager().getStringPropertyMap(PROPERTY)
        managers = {side: program.getFunctionManager() for side, program in programs.items()}
        spaces = {
            side: program.getAddressFactory().getDefaultAddressSpace()
            for side, program in programs.items()
        }
        retail_functions = retail.getFunctionManager()
        retail_space = retail.getAddressFactory().getDefaultAddressSpace()
        for obj in manifest["objects"]:
            if obj["type"] != "FUNCTION" or obj["recomp"] is None:
                continue
            address = int(obj["orig"], 16)
            canonical = retail_functions.getFunctionAt(retail_space.getAddress(address))
            private = {
                side: _signature(
                    managers[side].getFunctionAt(spaces[side].getAddress(int(obj[side], 16)))
                )
                for side in ("orig", "recomp")
            }
            differences = []
            if all(private.values()):
                for key in ("calling_convention", "non_auto_parameter_count", "variadic"):
                    if private["orig"][key] != private["recomp"][key]:
                        differences.append(key)
                if private["orig"]["return"] != private["recomp"]["return"]:
                    differences.append("return-type-width-storage")
                old_params = [
                    (p["type"], p["width"], p["storage"], p["auto"])
                    for p in private["orig"]["parameters"]
                ]
                new_params = [
                    (p["type"], p["width"], p["storage"], p["auto"])
                    for p in private["recomp"]["parameters"]
                ]
                if old_params != new_params:
                    differences.append("parameter-type-width-storage")
            else:
                differences.append("missing-private-signature")
            identity = f"pair:{address:#x}"
            stamped = (
                stamps.getString(canonical.getEntryPoint())
                if stamps is not None and canonical is not None
                else None
            )
            row = {
                "identity": identity,
                "name": obj["name"],
                "address": obj["orig"],
                "source_declaration": source.get(address),
                "recomp_decorated_symbol": obj.get("recomp_symbol"),
                "canonical_retail": _signature(canonical),
                "recorded_signature_origin": str(stamped) if stamped is not None else None,
                "independently_reviewed_retail_signature": bool(
                    canonical is not None and independently_reviewed_signature(retail, canonical)
                ),
                "private_comparison": private,
                "private_discrepancies": differences,
                "differing_callers": sorted(callers[identity]),
                "differing_callers_count": len(callers[identity]),
            }
            rows.append(row)
    output = directory / "signature-census.json"
    atomic_json(
        output,
        {
            "inputs": summary["inputs"],
            "source_index_stale": index_stale,
            "policy": "ABI triage only. Canonical ProgramDB signatures may be source/PDB projected; only explicit snapshot-valid retail-reviewed provenance is independent. Private signatures include reccmp preparation and are not independent corroboration. Recomp decoration comes from the compiler. Absence of a private discrepancy is not equivalence.",
            "callees": rows,
        },
    )
    discrepancies = [row for row in rows if row["private_discrepancies"]]
    return {
        "report": str(output),
        "paired_callees": len(rows),
        "private_signature_discrepancies": len(discrepancies),
        "independently_reviewed": sum(
            row["independently_reviewed_retail_signature"] for row in rows
        ),
        "top_callees": [
            {
                key: row[key]
                for key in ("identity", "name", "private_discrepancies", "differing_callers_count")
            }
            for row in sorted(discrepancies, key=lambda row: -row["differing_callers_count"])[:15]
        ],
    }
