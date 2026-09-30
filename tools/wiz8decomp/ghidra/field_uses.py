"""Owner-scoped census over existing Ghidra field-flow observations."""

from __future__ import annotations

import hashlib
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..paths import atomic_json


def _private_accesses(functions: list[dict], summary: dict, manifest: dict) -> list[dict]:
    from .inspect import DecompileSession, open_saved_comparison_programs
    from .semantic import field_accesses

    pairs = {int(obj["orig"], 16): obj for obj in manifest["objects"] if obj["recomp"]}
    observations = []
    with open_saved_comparison_programs(summary, manifest) as programs:
        sessions = {side: DecompileSession(program) for side, program in programs.items()}
        try:
            for function in functions:
                pair = pairs.get(int(function["address"], 16))
                if pair is None:
                    continue
                for root in function["roots"]:
                    storage = root["root"].get("storage")
                    if storage is None:
                        continue
                    row = {
                        "function": function["address"],
                        "owner": root["modeled_owner"],
                        "storage": storage,
                        "sides": {},
                    }
                    for side, program in programs.items():
                        entry = (
                            program.getAddressFactory()
                            .getDefaultAddressSpace()
                            .getAddress(int(pair[side], 16))
                        )
                        native = program.getFunctionManager().getFunctionAt(entry)
                        fact = {"entry": pair[side], "flow": None, "error": None}
                        row["sides"][side] = fact
                        if native is None:
                            fact["error"] = "paired function is absent from the private program"
                            continue
                        result = sessions[side].decompile(native, c_output=False)
                        high = result.getHighFunction()
                        if high is None:
                            fact["error"] = str(result.getErrorMessage())
                            continue
                        prototype = high.getFunctionPrototype()
                        matches = [
                            index
                            for index in range(prototype.getNumParams())
                            if str(prototype.getParam(index).getStorage()) == storage
                        ]
                        if len(matches) != 1:
                            fact["error"] = (
                                "no unique HighFunction parameter with this exact storage"
                            )
                            continue
                        try:
                            fact["flow"] = field_accesses(
                                program, pair[side], str(matches[0]), session=sessions[side]
                            )
                        except (ValueError, RuntimeError) as error:
                            fact["error"] = str(error)
                    observations.append(row)
        finally:
            for session in sessions.values():
                session.close()
    return observations


def _access_fingerprints(observations: list[dict]) -> list[dict]:
    """Owner-specific P-code access observations, without semantic conclusions."""
    groups = {}
    for row in observations:
        flows = {side: fact["flow"] for side, fact in row["sides"].items()}
        if any(flow is None for flow in flows.values()):
            continue
        widths = {side: defaultdict(set) for side in flows}
        for side, flow in flows.items():
            for access in flow["accesses"]:
                if (
                    access["kind"] in {"load", "store"}
                    and access["path"] == flow["root"]["name"]
                    and access["offset"] is not None
                ):
                    widths[side][(int(access["offset"], 16), access["kind"])].add(access["width"])
        for offset, operation in widths["orig"].keys() | widths["recomp"].keys():
            old = tuple(sorted(widths["orig"][(offset, operation)]))
            new = tuple(sorted(widths["recomp"][(offset, operation)]))
            if old == new:
                continue
            key = (row["owner"], offset, operation, old, new)
            group = groups.setdefault(
                key,
                {
                    "key": ["field-access-observation", *key],
                    "owner": row["owner"],
                    "offset": offset,
                    "operation": operation,
                    "retail_observed_widths": list(old),
                    "rebuild_observed_widths": list(new),
                    "functions": set(),
                },
            )
            group["functions"].add(row["function"])
    result = []
    for group in groups.values():
        group["functions"] = sorted(group["functions"])
        group["functions_count"] = len(group["functions"])
        result.append(group)
    return sorted(result, key=lambda row: (-row["functions_count"], str(row["key"])))


def _pointee(data_type: Any) -> Any | None:
    from ghidra.program.model.data import Pointer, TypeDef

    while isinstance(data_type, TypeDef):
        data_type = data_type.getBaseDataType()
    if not isinstance(data_type, Pointer):
        return None
    data_type = data_type.getDataType()
    while isinstance(data_type, TypeDef):
        data_type = data_type.getBaseDataType()
    return data_type


def _member(data_type: Any, offset: int, width: int) -> dict[str, Any] | None:
    """A modeled containing member, without inferring a field from access width."""
    from ghidra.program.model.data import Structure

    if not isinstance(data_type, Structure) or offset < 0:
        return None
    component = data_type.getComponentContaining(offset)
    if component is None or component.getFieldName() is None:
        return None
    start = int(component.getOffset())
    length = int(component.getLength())
    return {
        "name": str(component.getFieldName()),
        "offset": start,
        "size": length,
        "type": str(component.getDataType().getPathName()),
        "interior_offset": offset - start,
        "access_contained": offset + width <= start + length,
    }


def field_uses(program: Any, repository: Path, names: list[str], directory: Path) -> dict[str, Any]:
    from ..source_index import load_source_index, warn_if_source_index_may_be_stale
    from .inspect import DecompileSession
    from .semantic import field_accesses

    summary = json.loads((directory / "summary.json").read_text())
    index_stale = warn_if_source_index_may_be_stale(repository, summary["target"])
    source_index = load_source_index(repository)
    source_fields = defaultdict(list)
    for record in source_index["classes"]:
        if record["target"] != summary["target"] or not record.get("layout_trusted"):
            continue
        for field in record["fields"]:
            if (
                field.get("offset") is not None
                and field.get("size") is not None
                and field not in source_fields[record["qualified_name"]]
            ):
                source_fields[record["qualified_name"]].append(field)
    manifest = json.loads((directory / "manifest.json").read_text())
    digest = hashlib.sha256(json.dumps(manifest, sort_keys=True).encode()).hexdigest()
    if digest != summary["inputs"]["manifest_sha256"]:
        raise ValueError("field census summary and manifest use different catalogs")
    if str(program.getExecutableSHA256()) != manifest["orig"]["sha256"]:
        raise ValueError("field census retail program and comparison use different binaries")
    outcomes = {int(row["orig"], 16): row for row in summary["functions"]}
    clusters_path = directory / "mismatch-clusters.json"
    memberships: dict[int, list[Any]] = defaultdict(list)
    if clusters_path.is_file():
        clusters = json.loads(clusters_path.read_text())
        if clusters["inputs"]["manifest_sha256"] != summary["inputs"]["manifest_sha256"]:
            raise ValueError("field census comparison and mismatch clusters use different catalogs")
        for cluster in clusters["clusters"]:
            for row in cluster["functions"]:
                memberships[int(row, 16)].append(cluster["key"])

    wanted = set(names)
    groups: dict[tuple[str, int, int, str], dict[str, Any]] = {}
    functions = []
    unresolved = []
    session = DecompileSession(program, profile="analysis")
    try:
        for function in program.getFunctionManager().getFunctions(True):
            roots = []
            for parameter in function.getParameters():
                owner = _pointee(parameter.getDataType())
                if owner is not None and (
                    str(owner.getName()) in wanted or str(owner.getPathName()) in wanted
                ):
                    roots.append((parameter, owner))
            if not roots:
                continue
            address = int(function.getEntryPoint().getOffset())
            saved = outcomes.get(address, {})
            function_row = {
                "address": f"0x{address:x}",
                "name": str(function.getName(True)),
                "source": saved.get("source"),
                "outcome": saved.get("outcome", "not-compared"),
                "clusters": memberships.get(address, []),
                "roots": [],
            }
            functions.append(function_row)
            for parameter, owner in roots:
                root = str(parameter.getName())
                try:
                    flow = field_accesses(program, f"0x{address:x}", root, session=session)
                except (ValueError, RuntimeError) as error:
                    unresolved.append(
                        {"function": f"0x{address:x}", "root": root, "error": str(error)}
                    )
                    continue
                function_row["roots"].append(flow)
                flow["modeled_owner"] = str(owner.getPathName())
                for access in flow["accesses"]:
                    if access["kind"] not in {"load", "store"}:
                        continue
                    # A loaded pointer starts a new object. Its offset cannot
                    # be assigned to the parameter owner merely by adjacency.
                    if access["path"] != flow["root"]["name"] or access["offset"] is None:
                        unresolved.append(
                            {
                                "function": f"0x{address:x}",
                                "root": root,
                                "access": access,
                                "reason": "loaded-pointer owner or indexed offset unresolved",
                            }
                        )
                        continue
                    offset = int(access["offset"], 16)
                    width = int(access["width"])
                    owner_path = str(owner.getPathName())
                    key = (owner_path, offset, width, access["kind"])
                    group = groups.setdefault(
                        key,
                        {
                            "owner": owner_path,
                            "offset": offset,
                            "width": width,
                            "operation": "read" if access["kind"] == "load" else "write",
                            "modeled_member": _member(owner, offset, width),
                            "source_members": [
                                field
                                for field in source_fields[str(owner.getName())]
                                if field["offset"] <= offset < field["offset"] + field["size"]
                            ],
                            "uses": [],
                        },
                    )
                    group["uses"].append(
                        {
                            "function": f"0x{address:x}",
                            "root": root,
                            "site": access["site"],
                            "outcome": function_row["outcome"],
                            "source": function_row["source"],
                        }
                    )
    finally:
        session.close()
    rows = sorted(
        groups.values(),
        key=lambda row: (row["owner"], row["offset"], row["width"], row["operation"]),
    )
    for row in rows:
        row["functions"] = sorted({use["function"] for use in row["uses"]})
        row["functions_count"] = len(row["functions"])
        row["differing_functions"] = sorted(
            {use["function"] for use in row["uses"] if use["outcome"] == "differences"}
        )
    private = _private_accesses(functions, summary, manifest)
    fingerprints = _access_fingerprints(private)
    payload = {
        "comparison": str(directory),
        "comparison_inputs": summary["inputs"],
        "program": str(program.getName()),
        "requested_owners": names,
        "source_index_stale": index_stale,
        "evidence": "Live retail and saved private comparison HighFunction P-code. Owners and member names are current ProgramDB/source model, potentially source-projected, not independent retail facts. Private roots are joined only by exact parameter storage within catalog-paired functions. Width lists describe exposed P-code accesses; an empty list means not observed, not proven absent. Flow completeness and unresolved owners remain attached. No equivalence, source field width, or cross-offset member pairing is inferred.",
        "functions": functions,
        "fields": rows,
        "unresolved": unresolved,
        "private_access_observations": private,
        "access_fingerprints": fingerprints,
    }
    output = directory / "field-uses.json"
    atomic_json(output, payload)
    return {
        "report": str(output),
        "functions": len(functions),
        "field_access_groups": len(rows),
        "unresolved_accesses_or_roots": len(unresolved),
        "private_root_observations": len(private),
        "access_fingerprints": len(fingerprints),
        "top_access_fingerprints": fingerprints[:15],
        "top_fields": [
            {
                key: row[key]
                for key in (
                    "owner",
                    "offset",
                    "width",
                    "operation",
                    "modeled_member",
                    "functions_count",
                )
            }
            for row in sorted(rows, key=lambda row: -row["functions_count"])[:15]
        ],
    }
