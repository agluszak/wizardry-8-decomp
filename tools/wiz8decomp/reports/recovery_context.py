"""Generate one joined, disposable source-recovery packet for a function."""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path
from typing import Any

from ..ghidra.observation_evidence import load_observation_bundle
from ..ghidra.session import query_many
from ..ghidra.workspace import resolve_seed_program
from ..paths import atomic_json, atomic_write


def _read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _in_body(row: dict[str, str], field: str, instruction_addresses: set[int]) -> bool:
    return bool(row.get(field)) and int(row[field], 16) in instruction_addresses


def _reviewed(repo_dir: Path, name: str) -> list[dict[str, str]]:
    return _read(repo_dir / "evidence" / "reviewed" / "wiz8" / name)


def _source_unit(path: str) -> str:
    marker = "wizardry 8\\"
    folded = path.casefold()
    index = folded.find(marker)
    return path[index + len(marker) :] if index >= 0 else path


def _translation_unit(
    assertions: list[dict[str, str]], intervals: list[dict[str, str]], entry: int
) -> dict[str, Any]:
    direct = sorted(
        {
            _source_unit(row["source_path"])
            for row in assertions
            if row["source_path"].casefold().endswith(".cpp")
        },
        key=str.casefold,
    )
    interval = next(
        (
            row
            for row in intervals
            if row["record_type"] == "translation-unit"
            and int(row["lower_address"], 16) <= entry <= int(row["upper_address"], 16)
        ),
        None,
    )
    if len(direct) == 1:
        return {"source_path": direct[0], "attribution": "direct", "alternatives": []}
    if len(direct) > 1:
        return {
            "source_path": "",
            "attribution": "inlined-or-conflicting",
            "alternatives": direct,
        }
    if interval is not None:
        return {
            "source_path": interval["source_path"],
            "attribution": "interval-inference",
            "alternatives": [],
            "interval_lower": interval["lower_address"],
            "interval_upper": interval["upper_address"],
        }
    return {"source_path": "", "attribution": "gap", "alternatives": []}


def _markdown(context: dict[str, Any]) -> str:
    function = context["ghidra"]["function"]
    unit = context["translation_unit"]
    reviewed_function = context["reviewed"]["function"]
    reviewed_name = (
        reviewed_function.get("current_name") or reviewed_function.get("provisional_name") or "none"
    )
    lines = [
        f"# Recovery context for 0x{context['entry']:08X}",
        "",
        f"- Ghidra: `{function['prototype']}` ({function['size']} bytes)",
        (
            f"- Translation unit: `{unit.get('source_path') or 'unresolved'}` "
            f"({unit['attribution']})"
        ),
        f"- Reviewed identity: `{reviewed_name}`",
        f"- Reviewed classes: {', '.join(context['reviewed']['class_names']) or 'none'}",
        f"- Assertions: {len(context['assertions'])}",
        f"- EH cleanups: {len(context['eh']['unwind'])}",
        f"- Global references: {len(context['globals'])}",
        f"- Vptr writes: {len(context['polymorphism']['vptr_writes'])}",
        f"- Program facts: {len(context['semantic']['facts'].get('properties', {}))}",
        f"- Field accesses: {len(context['semantic'].get('field_accesses', {}).get('accesses', []))}",
        f"- Type variables: {len(context['semantic'].get('type_variables', {}).get('variables', []))}",
        f"- Indirect call sites: {len(context['semantic'].get('indirect_calls', []))}",
        "",
    ]

    if context["assertions"]:
        lines.extend(
            [
                "## Assertions",
                "",
                "| Site | Source | Line | Expression | Message |",
                "|---|---|---:|---|---|",
            ]
        )
        for row in context["assertions"]:
            values = [
                row["call_site"],
                _source_unit(row["source_path"]),
                row["line"],
                row["expression"],
                row["message"],
            ]
            lines.append("| " + " | ".join(value.replace("|", "\\|") for value in values) + " |")
        lines.append("")

    if context["eh"]["unwind"]:
        lines.extend(
            [
                "## EH locals",
                "",
                "| State | Frame slot | Kind | Destructor |",
                "|---:|---:|---|---|",
            ]
        )
        for row in context["eh"]["unwind"]:
            destructor = row["import_signature"] or row["import_name"] or row["target"]
            lines.append(
                f"| {row['state']} | {row['frame_offset']} | {row['kind']} | "
                f"{destructor.replace('|', '\\|')} |"
            )
        lines.append("")

    if context["globals"]:
        lines.extend(
            [
                "## Global references",
                "",
                "| Site | Global | Access | Width | Kind | Storage |",
                "|---|---|---|---:|---|---|",
            ]
        )
        for row in context["globals"]:
            lines.append(
                f"| {row['site']} | {row['target']} | {row['access']} | "
                f"{row.get('width', row.get('widths', ''))} | "
                f"{row.get('kind', '')} | {row.get('storage', '')} |"
            )
        lines.append("")

    if context["polymorphism"]["vptr_writes"]:
        lines.extend(
            [
                "## Vptr writes",
                "",
                "| Site | Object offset | Vtable | Slots |",
                "|---|---:|---|---:|",
            ]
        )
        tables = context["polymorphism"]["tables"]
        for row in context["polymorphism"]["vptr_writes"]:
            table = tables[row["vtable"]]
            lines.append(
                f"| {row['site']} | {row['object_offset']} | {row['vtable']} | "
                f"{table['slot_count']} |"
            )
        lines.append("")

    lines.extend(
        [
            "## Decompiled",
            "",
            "```cpp",
            context["ghidra"]["decompiled"].rstrip(),
            "```",
            "",
        ]
    )
    if context["ghidra"].get("listing"):
        lines.extend(
            [
                "## Listing",
                "",
                "```asm",
                context["ghidra"]["listing"].rstrip(),
                "```",
                "",
            ]
        )
    return "\n".join(lines)


def recovery_context_report(
    settings: Any,
    address: str,
    selector: str = "wiz8",
    *,
    deep: bool = False,
    root: str = "this",
) -> dict[str, Any]:
    requested = int(address, 0)
    program_name = resolve_seed_program(settings, selector)
    canonical_program = resolve_seed_program(settings, "wiz8")
    has_canonical_addresses = program_name == canonical_program
    queries = [
        ("function", [f"0x{requested:08x}"]),
        ("decompile", [f"0x{requested:08x}"]),
    ]
    if deep:
        queries.extend(
            [
                ("listing", [f"0x{requested:08x}"]),
                ("high-function", [f"0x{requested:08x}"]),
                ("pcode", [f"0x{requested:08x}", "normalize"]),
            ]
        )
    results, transport = query_many(settings, selector, queries)
    by_command = {item["command"]: item["result"] for item in results}
    function = by_command["function"]["function"]
    entry = int(function["entry"], 16)
    instruction_addresses = {int(value, 16) for value in function.get("instruction_addresses", [])}
    semantic_facts: dict[str, Any] = {}
    high = by_command.get("high-function", {})
    semantic_fields: dict[str, Any] = {}
    semantic_variables: dict[str, Any] = {}
    if deep and high.get("parameters"):
        semantic_results, _ = query_many(
            settings,
            selector,
            [
                ("field-accesses", [f"0x{entry:08x}", root]),
                ("type-variables", [f"0x{entry:08x}", root]),
            ],
        )
        semantic_fields = semantic_results[0]["result"]
        semantic_variables = semantic_results[1]["result"]
    indirect_sites = sorted(
        {
            operation["address"]
            for operation in by_command.get("pcode", {}).get("operations", [])
            if operation["op"] == "CALLIND"
        }
    )
    indirect_calls = []
    if indirect_sites:
        call_results, _ = query_many(
            settings, selector, [("callsite", [f"0x{site}"]) for site in indirect_sites]
        )
        indirect_calls = [item["result"] for item in call_results]

    bundle = load_observation_bundle(program_name, settings.repo_dir)
    assertions = [
        row for row in bundle["assertions"] if _in_body(row, "call_site", instruction_addresses)
    ]
    runtime_names = [
        row
        for row in bundle["runtime_class_names"]
        if _in_body(row, "call_site", instruction_addresses)
    ]
    eh_functions = [
        row for row in bundle["eh_functions"] if _in_body(row, "frame_setup", instruction_addresses)
    ]
    funcinfos = {row["funcinfo"] for row in eh_functions}
    unwind = [row for row in bundle["eh_unwind"] if row["funcinfo"] in funcinfos]

    global_observations = {row["address"]: row for row in bundle["globals"]}
    global_references = function.get("data_references", [])
    globals_joined = []
    for row in global_references:
        globals_joined.append({**row, **global_observations.get(row["target"], {})})

    vptr_writes = [
        row for row in bundle["vptr_writes"] if _in_body(row, "site", instruction_addresses)
    ]
    table_addresses = {row["vtable"] for row in vptr_writes}
    slots_by_table: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in bundle["vtable_slots"]:
        if row["vtable"] in table_addresses:
            slots_by_table[row["vtable"]].append(row)
    tables = {
        row["address"]: {**row, "slots": slots_by_table[row["address"]]}
        for row in bundle["vtables"]
        if row["address"] in table_addresses
    }

    reviewed_functions = [
        row
        for row in _reviewed(settings.repo_dir, "functions.csv")
        if has_canonical_addresses
        and row["program"] == "wiz8"
        and row["address"]
        and int(row["address"], 16) == entry
    ]
    reviewed_signatures = [
        row
        for row in _reviewed(settings.repo_dir, "signatures.csv")
        if has_canonical_addresses and row["program"] == "wiz8" and int(row["address"], 16) == entry
    ]
    reviewed_vtables = {
        row["address"]: row
        for row in _reviewed(settings.repo_dir, "vtables.csv")
        if has_canonical_addresses
        and row["program"] == "wiz8"
        and row["address"] in table_addresses
    }
    reviewed_classes = _reviewed(settings.repo_dir, "classes.csv")
    class_names = {
        row["class_name"]
        for row in reviewed_classes
        if any(
            row.get(field) and int(row[field], 16) == entry
            for field in ("constructor", "destructor", "scalar_deleting_destructor")
        )
    }
    class_names.update(row["class_name"] for row in reviewed_vtables.values())
    if reviewed_functions:
        reviewed_name = reviewed_functions[0]["current_name"]
        if "::" in reviewed_name:
            class_names.add(reviewed_name.split("::", 1)[0])
    selected_classes = [row for row in reviewed_classes if row["class_name"] in class_names]
    selected_fields = [
        row
        for row in _reviewed(settings.repo_dir, "fields.csv")
        if row["class_name"] in class_names
    ]
    interval_path = (
        settings.build_dir / "reports" / "translation-units" / "translation-unit-intervals.csv"
    )
    intervals = _read(interval_path) if interval_path.is_file() else []

    context = {
        "schema": "wiz8.recovery-context",
        "program": program_name,
        "requested_address": f"0x{requested:08x}",
        "entry": entry,
        "transport": transport,
        "deep": deep,
        "root": root if deep else None,
        "translation_unit": _translation_unit(
            assertions, intervals if has_canonical_addresses else [], entry
        ),
        "reviewed": {
            "function": reviewed_functions[0] if reviewed_functions else {},
            "signature": reviewed_signatures[0] if reviewed_signatures else {},
            "vtables": reviewed_vtables,
            "class_names": sorted(class_names, key=str.casefold),
            "classes": selected_classes,
            "fields": selected_fields,
        },
        "assertions": assertions,
        "runtime_class_names": runtime_names,
        "eh": {"functions": eh_functions, "unwind": unwind},
        "globals": globals_joined,
        "polymorphism": {"vptr_writes": vptr_writes, "tables": tables},
        "semantic": {
            "facts": semantic_facts,
            "high_function": high,
            "normalized_pcode": by_command.get("pcode", {}),
            "field_accesses": semantic_fields,
            "type_variables": semantic_variables,
            "indirect_calls": indirect_calls,
        },
        "candidate_relations": _candidate_relations(settings.repo_dir, program_name, entry),
        "ghidra": {
            "function": function,
            "decompiled": by_command["decompile"]["decompiled"] or "",
            "listing": by_command.get("listing", {}).get("listing", ""),
        },
    }

    report_dir = settings.build_dir / "reports" / "recovery-context"
    stem = f"{entry:08x}"
    json_path = report_dir / f"{stem}.json"
    markdown_path = report_dir / f"{stem}.md"
    atomic_json(json_path, context)
    atomic_write(markdown_path, _markdown(context))
    return {
        "program": program_name,
        "entry": f"0x{entry:08x}",
        "transport": transport,
        "translation_unit": context["translation_unit"],
        "counts": {
            "assertions": len(assertions),
            "eh_cleanups": len(unwind),
            "global_references": len(globals_joined),
            "vptr_writes": len(vptr_writes),
            "program_facts": len(semantic_facts.get("properties", {})),
            "field_accesses": len(semantic_fields.get("accesses", [])),
            "type_variables": len(semantic_variables.get("variables", [])),
            "indirect_calls": len(indirect_calls),
        },
        "outputs": [
            str(json_path.relative_to(settings.repo_dir)),
            str(markdown_path.relative_to(settings.repo_dir)),
        ],
    }


def _candidate_relations(repo: Path, program: str, entry: int) -> dict[str, Any]:
    """Relevant disposable cross-build and object-map rows."""

    build = repo / "build" / "reports"
    relations: dict[str, Any] = {"cross_build": [], "object_map": []}
    for path in sorted((build / "cross-build").glob("*/alignment.csv")):
        for row in _read(path):
            if any(
                value and value.lower().removeprefix("0x").zfill(8) == f"{entry:08x}"
                for key, value in row.items()
                if "address" in key or key in {"left", "right"}
            ):
                relations["cross_build"].append({"report": str(path.relative_to(repo)), **row})
    for path in sorted((build / "object-map" / program).glob("*.csv")):
        for row in _read(path):
            if any(
                value.lower().removeprefix("0x").zfill(8) == f"{entry:08x}"
                for value in row.values()
                if value
            ):
                relations["object_map"].append(row)
    return relations
