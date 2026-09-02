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
        f"- Function: `{function['name']}`",
        f"- Prototype: `{function['prototype']}` ({function['size']} bytes)",
        (
            f"- Translation unit: `{unit.get('source_path') or 'unresolved'}` "
            f"({unit['attribution']})"
        ),
        f"- Reviewed identity: `{reviewed_name}`",
        f"- Reviewed classes: {', '.join(context['reviewed']['class_names']) or 'none'}",
        f"- Assertions: {len(context['assertions'])}",
        f"- EH cleanups: {len(context['eh']['unwind'])}",
        f"- Named globals: {len(context['globals'])}",
        f"- Vptr writes: {len(context['polymorphism']['vptr_writes'])}",
        f"- Program facts: {len(context['semantic']['facts'].get('properties', {}))}",
        f"- Field accesses: {len(context['semantic'].get('field_accesses', {}).get('accesses', []))}",
        f"- Indirect call sites: {len(context['semantic'].get('indirect_calls', []))}",
        "",
    ]

    match = context.get("match", {})
    if match:
        row = (match.get("functions") or [{}])[0]
        score = row.get("raw_matching")
        score_text = f" ({float(score) * 100:.1f}%)" if isinstance(score, int | float) else ""
        lines.extend(["## Match", "", f"{row.get('status', 'unknown')}{score_text}"])
        difference = row.get("difference") or {}
        if difference:
            lines.append(f"First divergence: {difference.get('kind', 'unknown')}")
        lines.append("")

    if context.get("calls"):
        lines.extend(["## Calls", ""])
        for row in context["calls"]:
            lines.append(f"- `{row['site']}` -> `{row.get('name') or row['target']}`")
        lines.append("")

    callers = function.get("callers") or []
    if callers:
        lines.extend(["## Called by", "", *(f"- `{value}`" for value in callers), ""])

    field_accesses = context["semantic"].get("field_accesses", {}).get("accesses", [])
    if field_accesses:
        lines.extend(["## Fields", ""])
        for row in field_accesses:
            location = row.get("path") or row.get("offset") or row.get("address") or "unknown"
            lines.append(f"- `{row.get('site', '')}` {row.get('kind', '')} `{location}`")
        lines.append("")

    indirect_calls = context["semantic"].get("indirect_calls", [])
    if indirect_calls:
        lines.extend(["## Virtual / indirect calls", ""])
        for row in indirect_calls:
            target = row.get("target") or {}
            offset = target.get("offset") if isinstance(target, dict) else target
            lines.append(
                f"- `{row.get('address') or row.get('site', '')}` target `{offset or 'unknown'}`"
            )
        lines.append("")

    strings = function.get("referenced_strings") or []
    if strings:
        lines.extend(["## Strings", "", *(f"- {value}" for value in strings), ""])

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
                "## Named globals",
                "",
                "| Site | Global | Access | Width | Kind | Storage |",
                "|---|---|---|---:|---|---|",
            ]
        )
        for row in context["globals"]:
            lines.append(
                f"| {row['site']} | {row.get('name') or row['target']} ({row['target']}) | "
                f"{row['access']} | "
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


def recovery_context_reports(
    settings: Any,
    selectors: list[str],
    selector: str = "wiz8",
    *,
    deep: bool = False,
    root: str = "this",
    discover: bool = False,
) -> list[dict[str, Any]]:
    """Build one or more contexts with one Ghidra session and one reccmp report."""

    from ..reccmp_workflows import compare_selected
    from ..selectors import resolve_function_selectors

    requested_entries = resolve_function_selectors(settings.repo_dir, selectors)
    program_name = resolve_seed_program(settings, selector)
    canonical_program = resolve_seed_program(settings, "wiz8")
    has_canonical_addresses = program_name == canonical_program
    reviewed_assertions = _read(
        settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv"
    )
    source_model = build_source_model(settings.repo_dir) if has_canonical_addresses else None
    assertions_by_entry = {
        requested: [
            row
            for row in reviewed_assertions
            if row.get("containing_function") and int(row["containing_function"], 16) == requested
        ]
        for requested in requested_entries
    }
    boundary_addresses = set(requested_entries)
    boundary_addresses.update(
        int(row["call_site"], 16) for rows in assertions_by_entry.values() for row in rows
    )
    boundary_argument = ",".join(f"0x{item:08x}" for item in sorted(boundary_addresses))
    queries: list[tuple[str, list[str]]] = [("function-of", [boundary_argument])]
    class_names = sorted(
        {
            function.owning_class
            for requested in requested_entries
            if source_model is not None
            and (function := source_model.functions.get(requested)) is not None
            and function.owning_class
        }
    )
    for requested in requested_entries:
        address = f"0x{requested:08x}"
        queries.extend(
            [
                ("function", [address]),
                ("decompile", [address]),
                ("function-facts", [address]),
                ("indirect-calls", [address]),
            ]
        )
        source_function = source_model.functions.get(requested) if source_model else None
        if source_function is not None and source_function.owning_class:
            queries.append(("field-accesses", [address, root]))
        if deep:
            queries.extend(
                [
                    ("listing", [address]),
                    ("high-function", [address]),
                ]
            )
    if class_names:
        queries.append(("class-fields", class_names))
    function_seeds = [
        f"0x{requested:08x}"
        for requested in requested_entries
        if discover
        or (
            bool(assertions_by_entry[requested])
            and (source_model is None or requested not in source_model.functions)
        )
    ]
    results, transport = query_many(
        settings,
        selector,
        queries,
        function_seeds=function_seeds or None,
    )
    raw_boundaries = results[0]["result"]["functions"]
    boundaries = {
        int(item_address, 0): int(owner, 16) if owner is not None else None
        for item_address, owner in raw_boundaries.items()
    }
    source_classes = load_source_index(settings.repo_dir)["classes"]
    class_field_rows = next(
        (item["result"]["classes"] for item in results if item["command"] == "class-fields"),
        [],
    )
    claims = load_claims(settings.repo_dir)
    match_bundle = (
        compare_selected(settings.repo_dir, "WIZ8", requested_entries)
        if has_canonical_addresses
        else {}
    )
    matches = {int(row["address"], 16): row for row in match_bundle.get("functions", [])}

    def result_for(command: str, requested: int) -> dict[str, Any]:
        address = f"0x{requested:08x}"
        return next(
            item["result"]
            for item in results
            if item["command"] == command and item["arguments"][0] == address
        )

    contexts: list[dict[str, Any]] = []
    for requested in requested_entries:
        function = result_for("function", requested)["function"]
        entry = int(function["entry"], 16)
        function_fact = result_for("function-facts", requested)
        source_function = source_model.functions.get(entry) if source_model is not None else None
        requested_assertions = assertions_by_entry[requested]
        assertions = [
            row
            for row in reviewed_assertions
            if row.get("containing_function") and int(row["containing_function"], 16) == entry
        ]
        boundary_defects = _assertion_boundary_defects(requested_assertions, boundaries)
        auto_discover = boundaries.get(requested) is None and bool(requested_assertions)
        raw_references = list(function_fact["data_references"])
        globals_joined = [
            row
            for row in raw_references
            if row.get("kind") == "program-data"
            and row.get("name")
            and not row["name"].casefold().startswith(("dat_", "lab_", "switchd_", "case_"))
            and not any(
                marker in row["name"].casefold()
                for marker in ("vftable", "funcinfo", "unwind", "ehhandler")
            )
        ]
        vptr_writes = [
            {**row, "vtable": row["target"], "store_displacement": ""}
            for row in function_fact["vptr_references"]
        ]
        tables = {
            row["target"]: {
                "address": row["target"],
                "name": row["name"],
                "slot_count": "",
            }
            for row in function_fact["vptr_references"]
        }
        table_addresses = set(tables)
        function_claims = [
            row
            for row in claims
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
        source_vtables = {
            f"{int(row['vtable_address']):08x}": row
            for row in source_classes
            if row.get("vtable_address") is not None
            and f"{int(row['vtable_address']):08x}" in table_addresses
        }
        context_classes = {row["qualified_name"] for row in source_vtables.values()}
        if source_function is not None and source_function.owning_class:
            context_classes.add(source_function.owning_class)
        semantic_fields = (
            result_for("field-accesses", requested)
            if source_function is not None and source_function.owning_class
            else {}
        )
        high = result_for("high-function", requested) if deep else {}
        indirect_result = result_for("indirect-calls", requested)
        normalized_pcode = indirect_result.get("normalized_pcode", {}) if deep else {}
        listing = result_for("listing", requested).get("listing", "") if deep else ""
        indirect_calls = indirect_result["calls"]
        match = {**match_bundle, "functions": [matches[entry]]} if entry in matches else {}
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
            "root": root if semantic_fields else None,
            "translation_unit": unit
            if has_canonical_addresses
            else {"source_path": "", "attribution": "non-canonical", "alternatives": []},
            "reviewed": {
                "function": reviewed_function,
                "signature": {
                    "prototype": source_function.prototype
                    if source_function
                    else function["prototype"],
                    "calling_convention": function.get("calling_convention", ""),
                    "authority": "source" if source_function else "ghidra",
                },
                "function_claims": function_claims,
                "vtables": source_vtables,
                "class_names": sorted(context_classes, key=str.casefold),
                "classes": [
                    row for row in source_classes if row["qualified_name"] in context_classes
                ],
                "fields": [
                    {"class_name": item["name"], **field}
                    for item in class_field_rows
                    if item["name"] in context_classes
                    for field in item["fields"]
                ],
            },
            "assertions": assertions,
            "runtime_class_names": [],
            "eh": {"metadata": function_fact["exception_metadata"], "unwind": []},
            "calls": function_fact["calls"],
            "globals": globals_joined,
            "raw_references": raw_references,
            "match": match,
            "polymorphism": {"vptr_writes": vptr_writes, "tables": tables},
            "semantic": {
                "facts": {},
                "high_function": high,
                "normalized_pcode": normalized_pcode,
                "field_accesses": semantic_fields,
                "indirect_calls": indirect_calls,
            },
            "ghidra": {
                "function": function,
                "decompiled": result_for("decompile", requested)["decompiled"] or "",
                "listing": listing,
            },
            "counts": {
                "assertions": len(assertions),
                "eh_cleanups": 0,
                "global_references": len(globals_joined),
                "raw_references": len(raw_references),
                "vptr_writes": len(vptr_writes),
                "program_facts": 0,
                "field_accesses": len(semantic_fields.get("accesses", [])),
                "indirect_calls": len(indirect_calls),
            },
        }
        report_dir = settings.build_dir / "reports" / "recovery-context"
        json_path = report_dir / f"{entry:08x}.json"
        markdown_path = report_dir / f"{entry:08x}.md"
        context["outputs"] = [
            str(json_path.relative_to(settings.repo_dir)),
            str(markdown_path.relative_to(settings.repo_dir)),
        ]
        atomic_json(json_path, context)
        atomic_write(markdown_path, _markdown(context))
        contexts.append(context)
    return contexts


def recovery_context_report(
    settings: Any,
    address: str,
    selector: str = "wiz8",
    **options: Any,
) -> dict[str, Any]:
    """Compatibility entry point for callers requesting one context."""

    return recovery_context_reports(settings, [address], selector, **options)[0]


def render_context(context: dict[str, Any]) -> str:
    footer = "artifacts: " + ", ".join(context["outputs"])
    return _markdown(context).rstrip() + "\n\n" + footer
