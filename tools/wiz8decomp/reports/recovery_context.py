"""Generate one joined, disposable source-recovery packet for a function."""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..evidence.claims import load_claims
from ..ghidra.session import query_many
from ..ghidra.unit_intervals import TranslationUnitResolver
from ..ghidra.workspace import resolve_seed_program
from ..paths import atomic_json, atomic_write
from ..source_model import build_source_model, load_source_index


def _read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _source_unit(path: str) -> str:
    marker = "wizardry 8\\"
    folded = path.casefold()
    index = folded.find(marker)
    return path[index + len(marker) :] if index >= 0 else path


def _assertion_boundary_defects(
    assertions: list[dict[str, str]], containing: dict[int, int | None]
) -> list[dict[str, str | None]]:
    defects: list[dict[str, str | None]] = []
    for row in assertions:
        expected = int(row["containing_function"], 16)
        site = int(row["call_site"], 16)
        actual_anchor = containing.get(expected)
        actual_site = containing.get(site)
        if actual_anchor != expected or actual_site != expected:
            defects.append(
                {
                    "kind": "invalid-assertion-function-boundary",
                    "containing_function": f"0x{expected:08x}",
                    "call_site": f"0x{site:08x}",
                    "anchor_owner": (
                        f"0x{actual_anchor:08x}" if actual_anchor is not None else None
                    ),
                    "call_site_owner": (
                        f"0x{actual_site:08x}" if actual_site is not None else None
                    ),
                }
            )
    return defects


def _markdown(context: dict[str, Any]) -> str:
    function = context["ghidra"]["function"]
    unit = context["translation_unit"]
    reviewed_function = context["reviewed"]["function"]
    reviewed_name = reviewed_function.get("name") or "none"
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
                "| Site | Store displacement | Vtable | Slots |",
                "|---|---:|---|---:|",
            ]
        )
        tables = context["polymorphism"]["tables"]
        for row in context["polymorphism"]["vptr_writes"]:
            table = tables[row["vtable"]]
            lines.append(
                f"| {row['site']} | {row['store_displacement']} | {row['vtable']} | "
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
    discover: bool = False,
) -> dict[str, Any]:
    requested = int(address, 0)
    program_name = resolve_seed_program(settings, selector)
    canonical_program = resolve_seed_program(settings, "wiz8")
    has_canonical_addresses = program_name == canonical_program
    reviewed_assertions = _read(
        settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    )
    requested_assertions = [
        row
        for row in reviewed_assertions
        if row.get("containing_function") and int(row["containing_function"], 16) == requested
    ]
    boundary_addresses = {requested}
    boundary_addresses.update(int(row["call_site"], 16) for row in requested_assertions)
    boundary_argument = ",".join(f"0x{item:08x}" for item in sorted(boundary_addresses))
    boundary_results, _ = query_many(settings, selector, [("function-of", [boundary_argument])])
    raw_boundaries = boundary_results[0]["result"]["functions"]
    boundaries = {
        int(address, 0): int(owner, 16) if owner is not None else None
        for address, owner in raw_boundaries.items()
    }
    boundary_defects = _assertion_boundary_defects(requested_assertions, boundaries)
    auto_discover = boundaries.get(requested) is None and bool(requested_assertions)
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
    function_seeds = [f"0x{requested:08x}"] if discover or auto_discover else None
    results, transport = query_many(
        settings,
        selector,
        queries,
        function_seeds=function_seeds,
    )
    by_command = {item["command"]: item["result"] for item in results}
    function = by_command["function"]["function"]
    entry = int(function["entry"], 16)
    semantic_facts: dict[str, Any] = {}
    high = by_command.get("high-function", {})
    semantic_fields: dict[str, Any] = {}
    if deep and high.get("parameters"):
        semantic_results, _ = query_many(
            settings,
            selector,
            [("field-accesses", [f"0x{entry:08x}", root])],
            function_seeds=function_seeds,
        )
        semantic_fields = semantic_results[0]["result"]
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
            settings,
            selector,
            [("callsite", [f"0x{site}"]) for site in indirect_sites],
            function_seeds=function_seeds,
        )
        indirect_calls = [item["result"] for item in call_results]

    assertions = [
        row
        for row in reviewed_assertions
        if row.get("containing_function") and int(row["containing_function"], 16) == entry
    ]
    from ..ghidra.audits import function_facts

    focused = function_facts(settings, {entry})
    function_fact = (
        focused[0]
        if focused
        else {"calls": [], "data_references": [], "vptr_references": [], "exception_metadata": []}
    )
    runtime_names: list[dict[str, str]] = []
    unwind: list[dict[str, str]] = []
    globals_joined = list(function_fact["data_references"])
    vptr_writes = [
        {**row, "vtable": row["target"], "store_displacement": ""}
        for row in function_fact["vptr_references"]
    ]
    tables = {
        row["target"]: {"address": row["target"], "name": row["name"], "slot_count": ""}
        for row in function_fact["vptr_references"]
    }
    table_addresses = set(tables)

    source_function = (
        build_source_model(settings.repo_dir).functions.get(entry)
        if has_canonical_addresses
        else None
    )
    function_claims = [
        row
        for row in load_claims(settings.repo_dir)
        if has_canonical_addresses
        and row["entity_kind"] == "function"
        and int(row["entity_key"], 16) == entry
    ]
    accepted_identity = next(
        (row for row in function_claims if row["predicate"] == "accepted-identity"), None
    )
    reviewed_function = (
        {
            "address": f"{entry:08x}",
            "name": source_function.name,
            "kind": source_function.kind,
            "source_path": source_function.file,
            "source_line": source_function.line,
        }
        if source_function is not None
        else {
            "address": f"{entry:08x}",
            "name": accepted_identity["value"] if accepted_identity else function["name"],
            "kind": "GHIDRA",
            "source_path": "",
            "source_line": "",
        }
    )
    reviewed_signature = {
        "prototype": source_function.prototype if source_function else function["prototype"],
        "calling_convention": function.get("calling_convention", ""),
        "authority": "source" if source_function else "ghidra",
    }
    source_classes = load_source_index(settings.repo_dir)["classes"]
    source_vtables = {
        f"{int(row['vtable_address']):08x}": row
        for row in source_classes
        if row.get("vtable_address") is not None
        and f"{int(row['vtable_address']):08x}" in table_addresses
    }
    class_names = {row["qualified_name"] for row in source_vtables.values()}
    if source_function is not None and source_function.owning_class:
        class_names.add(source_function.owning_class)
    selected_classes = [row for row in source_classes if row["qualified_name"] in class_names]
    from ..ghidra.audits import class_fields

    selected_fields = [
        {"class_name": item["name"], **field}
        for item in class_fields(settings, class_names)
        for field in item["fields"]
    ]
    unit = TranslationUnitResolver(reviewed_assertions).resolve(entry)

    context = {
        "schema": "wiz8.recovery-context",
        "program": program_name,
        "requested_address": f"0x{requested:08x}",
        "entry": entry,
        "transport": transport,
        "deep": deep,
        "discovered": discover or auto_discover,
        "boundary_defects": boundary_defects,
        "root": root if deep else None,
        "translation_unit": unit
        if has_canonical_addresses
        else {"source_path": "", "attribution": "non-canonical", "alternatives": []},
        "reviewed": {
            "function": reviewed_function,
            "signature": reviewed_signature,
            "function_claims": function_claims,
            "vtables": source_vtables,
            "class_names": sorted(class_names, key=str.casefold),
            "classes": selected_classes,
            "fields": selected_fields,
        },
        "assertions": assertions,
        "runtime_class_names": runtime_names,
        "eh": {"metadata": function_fact["exception_metadata"], "unwind": unwind},
        "globals": globals_joined,
        "polymorphism": {"vptr_writes": vptr_writes, "tables": tables},
        "semantic": {
            "facts": semantic_facts,
            "high_function": high,
            "normalized_pcode": by_command.get("pcode", {}),
            "field_accesses": semantic_fields,
            "indirect_calls": indirect_calls,
        },
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
        "discovered": discover or auto_discover,
        "boundary_defects": boundary_defects,
        "translation_unit": context["translation_unit"],
        "counts": {
            "assertions": len(assertions),
            "eh_cleanups": len(unwind),
            "global_references": len(globals_joined),
            "vptr_writes": len(vptr_writes),
            "program_facts": len(semantic_facts.get("properties", {})),
            "field_accesses": len(semantic_fields.get("accesses", [])),
            "indirect_calls": len(indirect_calls),
        },
        "outputs": [
            str(json_path.relative_to(settings.repo_dir)),
            str(markdown_path.relative_to(settings.repo_dir)),
        ],
    }
