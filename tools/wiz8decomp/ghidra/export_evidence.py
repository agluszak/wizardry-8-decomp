from __future__ import annotations

import csv
import io
import json
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_json, atomic_write
from .analyze import rtti_inventory
from .environment import start_pyghidra
from .project import resolve_program_name
from .query import execute_query, function_metadata
from .query_daemon import stop_daemon


FUNCTION_FIELDS = [
    "program", "variant", "entry", "size", "name", "namespace", "thunk", "thunk_target",
    "calling_convention", "prototype", "caller_count", "callee_count", "referenced_strings",
    "raw_body_sha256", "instruction_fingerprint_sha256", "decompiler_status", "library_or_middleware",
]


def _csv(path: Path, fields: list[str], rows: list[dict[str, Any]]) -> None:
    stream = io.StringIO(newline="")
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n", extrasaction="ignore")
    writer.writeheader()
    for row in rows:
        cooked = {key: json.dumps(value, ensure_ascii=False, sort_keys=True) if isinstance(value, (list, dict)) else value for key, value in row.items()}
        writer.writerow(cooked)
    atomic_write(path, stream.getvalue())


def _decompile(program: Any, entry: str) -> tuple[str, str | None]:
    result = execute_query(program, "decompile", [entry])
    if result["completed"]:
        return "completed", result["decompiled"]
    return "failed: " + (result.get("error") or "unknown"), None


def export_evidence(
    settings: Settings,
    *,
    selector: str | None,
    address: str | None,
    export_all: bool,
) -> dict[str, Any]:
    if bool(address) == bool(export_all):
        raise ValueError("choose exactly one of --address ADDRESS or --all")
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra

    program_name = resolve_program_name(settings, selector)
    variant = program_name.split("--")[1]
    output = settings.build_dir / "evidence" / "ghidra" / program_name
    output.mkdir(parents=True, exist_ok=True)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    rows: list[dict[str, Any]] = []
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            functions = []
            if address:
                target = program.getAddressFactory().getAddress(address)
                if target is None:
                    target = program.getAddressFactory().getDefaultAddressSpace().getAddress(address.removeprefix("0x"))
                function = program.getFunctionManager().getFunctionContaining(target)
                if function is None:
                    raise ValueError(f"no function contains {address}")
                functions = [function]
            else:
                iterator = program.getFunctionManager().getFunctions(True)
                while iterator.hasNext():
                    functions.append(iterator.next())
            for function in functions:
                row = function_metadata(program, function)
                row.update({"program": program_name, "variant": variant, "library_or_middleware": function.getName().startswith(("FUN_", "thunk_")) is False and function.isExternal()})
                if address:
                    status, decompiled = _decompile(program, row["entry"])
                else:
                    status, decompiled = "not-requested", None
                row["decompiler_status"] = status
                rows.append(row)
                if address:
                    listing = execute_query(program, "listing", [row["entry"]])["listing"]
                    atomic_write(output / "listings" / f"{row['entry']}.txt", listing + "\n")
                    if decompiled:
                        atomic_write(output / "decompiled" / f"{row['entry']}.c", decompiled)
                    atomic_json(output / f"function-{row['entry']}.json", row)
            strings = execute_query(program, "strings", [])["strings"]
            classes, vtables = rtti_inventory(program)
            _csv(output / "functions.csv", FUNCTION_FIELDS, rows)
            _csv(output / "strings.csv", ["address", "value", "references"], strings)
            _csv(output / "classes.csv", ["mangled_name", "type_descriptor_string", "evidence"], classes)
            _csv(output / "vtables.csv", ["address", "name", "evidence"], vtables)
            source_rows = [{"address": item["address"], "value": item["value"]} for item in strings if any(extension in item["value"].casefold() for extension in (".c", ".cpp", ".h", ".pdb"))]
            _csv(output / "source-paths.csv", ["address", "value"], source_rows)
    finally:
        project.close()
    _csv(settings.build_dir / "evidence" / "functions" / f"{program_name}.csv", FUNCTION_FIELDS, rows)
    summary = {"schema": "wiz8.ghidra-evidence", "format_version": 1, "program": program_name, "function_count": len(rows), "mode": "targeted" if address else "all", "output": str(output.relative_to(settings.repo_dir))}
    atomic_json(output / "manifest.json", summary)
    return summary


def cross_build_candidates(settings: Settings) -> dict[str, Any]:
    files = sorted((settings.build_dir / "evidence" / "functions").glob("*.csv"))
    rows = []
    for path in files:
        with path.open(encoding="utf-8", newline="") as stream:
            rows.extend(csv.DictReader(stream))
    groups_exact: dict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
    groups_instruction: dict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
    groups_strings: dict[tuple[str, ...], list[dict[str, str]]] = defaultdict(list)
    for row in rows:
        strings = tuple(sorted(json.loads(row["referenced_strings"] or "[]"), key=str.casefold))
        groups_exact[(row["raw_body_sha256"],)].append(row)
        groups_instruction[(row["instruction_fingerprint_sha256"], row["size"], row["caller_count"], row["callee_count"], *strings)].append(row)
        if strings:
            groups_strings[(row["size"], *strings)].append(row)
    candidates = []
    seen: set[tuple[str, str, str, str]] = set()
    for category, groups in (("exact", groups_exact), ("strong-candidate", groups_instruction), ("weak-candidate", groups_strings)):
        for members in groups.values():
            by_program: dict[str, list[dict[str, str]]] = defaultdict(list)
            for member in members:
                by_program[member["program"]].append(member)
            # Repeated stubs and wrapper bodies do not yield a defensible mapping.
            unique = [values[0] for values in by_program.values() if len(values) == 1]
            if len(unique) < 2:
                continue
            unique.sort(key=lambda item: (item["program"], item["entry"]))
            for left_index, left in enumerate(unique):
                for right in unique[left_index + 1 :]:
                    if left["variant"] == right["variant"]:
                        continue
                    key = (left["program"], left["entry"], right["program"], right["entry"])
                    if key in seen:
                        continue
                    seen.add(key)
                    candidates.append({"category": category, "left_program": left["program"], "left_entry": left["entry"], "right_program": right["program"], "right_entry": right["entry"], "size": left["size"], "evidence": "unique byte-identical function body" if category == "exact" else ("unique instruction fingerprint, size, call degree, and referenced-string signature" if category == "strong-candidate" else "unique equal size and identical nonempty referenced-string set")})
    candidates.sort(key=lambda item: (item["category"], item["left_program"], item["left_entry"], item["right_program"], item["right_entry"]))
    path = settings.build_dir / "evidence" / "cross-build-candidates.csv"
    _csv(path, ["category", "left_program", "left_entry", "right_program", "right_entry", "size", "evidence"], candidates)
    counts = {category: sum(item["category"] == category for item in candidates) for category in ("exact", "strong-candidate", "weak-candidate")}
    summary = {"schema": "wiz8.cross-build", "format_version": 1, "counts": counts, "unmatched_functions": max(0, len(rows) - len({(item['left_program'], item['left_entry']) for item in candidates} | {(item['right_program'], item['right_entry']) for item in candidates}))}
    atomic_json(settings.build_dir / "reports" / "cross-build-summary.json", summary)
    lines = ["# Cross-build function candidates", "", "These are syntactic candidates, not claims of semantic equivalence.", "", *[f"- {key}: {value}" for key, value in counts.items()], f"- unmatched function records: {summary['unmatched_functions']}", ""]
    atomic_write(settings.build_dir / "reports" / "cross-build-summary.md", "\n".join(lines))
    return summary
