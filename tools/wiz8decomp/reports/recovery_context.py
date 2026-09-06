"""Build bounded source and retail context for selected functions."""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..ghidra.env import open_program
from ..ghidra.query import query_many, resolve_function_selectors
from ..ghidra.unit_intervals import TranslationUnitResolver
from ..ghidra.workspace import resolve_seed_program
from ..paths import atomic_write
from ..source_index import load_source_index, source_functions


def _read(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


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


def _one_line(text: str) -> str:
    return " ".join(text.replace("\n", " ").split())


def _decompiled_signature(text: str) -> str:
    return _one_line(text.split("{", 1)[0].strip())


def recovery_context_reports(
    settings: Any,
    selectors: list[str],
    selector: str = "wiz8",
) -> list[dict[str, Any]]:
    """Return a stable function-record collection without comparison or deep analysis."""

    if not selectors:
        raise ValueError("pass one or more function selectors")
    program_name = resolve_seed_program(settings, selector)
    canonical = program_name == resolve_seed_program(settings, "wiz8")
    assertions = _read(settings.repo_dir / "evidence" / "observations" / "wiz8" / "assertions.csv")
    source_model = source_functions(settings.repo_dir) if canonical else {}

    with open_program(settings, selector) as program:
        entries = resolve_function_selectors(program, selectors)
        by_requested = {
            entry: [
                row
                for row in assertions
                if row.get("containing_function") and int(row["containing_function"], 16) == entry
            ]
            for entry in entries
        }
        boundary_addresses = set(entries)
        boundary_addresses.update(
            int(row["call_site"], 16) for rows in by_requested.values() for row in rows
        )
        queries: list[tuple[str, list[str]]] = [
            ("function-of", [",".join(f"0x{entry:08x}" for entry in sorted(boundary_addresses))])
        ]
        class_names = sorted(
            {
                marker.declaration.owning_class
                for entry in entries
                if (marker := source_model.get(entry)) is not None
                and marker.declaration is not None
                and marker.declaration.owning_class
            }
        )
        for entry in entries:
            address = f"0x{entry:08x}"
            queries.extend(
                [
                    ("function", [address]),
                    ("decompile", [address]),
                    ("indirect-calls", [address]),
                ]
            )
        if class_names:
            queries.append(("class-fields", class_names))
        seeds = [
            f"0x{entry:08x}"
            for entry in entries
            if by_requested[entry] and entry not in source_model
        ]
        results = query_many(program, queries, function_seeds=seeds or None)

    boundaries = {
        int(address, 0): int(owner, 16) if owner is not None else None
        for address, owner in results[0]["result"]["functions"].items()
    }
    indexed_classes = load_source_index(settings.repo_dir)["classes"]
    field_rows = next(
        (item["result"]["classes"] for item in results if item["command"] == "class-fields"), []
    )

    def result_for(command: str, requested: int) -> dict[str, Any]:
        address = f"0x{requested:08x}"
        return next(
            item["result"]
            for item in results
            if item["command"] == command and item["arguments"][0] == address
        )

    def source_boundary(address: str | None) -> dict[str, Any] | None:
        if not address:
            return None
        try:
            marker = source_model.get(int(address, 16))
        except ValueError:
            return None
        if marker is None:
            return None
        declaration = marker.declaration
        return {
            "name": marker.name,
            "prototype": declaration.prototype if declaration is not None else None,
            "location": f"{marker.source_file}:{marker.line}",
        }

    output_dir = settings.build_dir / "context"
    output_dir.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, Any]] = []
    for requested in entries:
        function = result_for("function", requested)["function"]
        entry = int(function["entry"], 16)
        marker = source_model.get(entry)
        decompiled = result_for("decompile", requested).get("decompiled") or ""
        artifact = output_dir / f"{entry:08x}.cpp"
        atomic_write(artifact, decompiled.rstrip() + "\n")
        declaration = marker.declaration if marker is not None else None
        source = (
            {
                "translation_unit": marker.source_file,
                "location": f"{marker.source_file}:{marker.line}",
                "prototype": declaration.prototype if declaration is not None else None,
                "implementation": (
                    "definition"
                    if declaration is not None and declaration.is_definition
                    else "declaration"
                ),
            }
            if marker is not None
            else {
                "translation_unit": TranslationUnitResolver(assertions).resolve(entry),
                "location": None,
                "prototype": None,
                "implementation": "unrecovered",
            }
        )
        calls = []
        for call in function.get("calls", []):
            target = call.get("function") or {}
            calls.append(
                {
                    "site": call.get("site"),
                    "target": call.get("target"),
                    "name": call.get("name"),
                    "source": source_boundary(target.get("entry") or call.get("target")),
                }
            )
        callers = [
            {
                "site": row.get("site"),
                "entry": row.get("entry"),
                "name": row.get("name"),
                "source": source_boundary(row.get("entry")),
            }
            for row in function.get("caller_functions", [])
        ]
        globals_ = [
            row
            for row in function.get("data_references", [])
            if row.get("kind") == "program-data"
            and row.get("name")
            and not str(row["name"]).casefold().startswith(("dat_", "lab_", "switchd_", "case_"))
        ]
        related_classes = {
            declaration.owning_class
            for declaration in [marker.declaration if marker is not None else None]
            if declaration is not None and declaration.owning_class
        }
        records.append(
            {
                "entry": f"0x{entry:08x}",
                "program": program_name,
                "identity": {"name": marker.name if marker is not None else function["name"]},
                "source": source,
                "retail": {
                    "prototype": _one_line(function["prototype"]),
                    "calling_convention": function.get("calling_convention"),
                    "decompiled": str(artifact.relative_to(settings.repo_dir)),
                },
                "signature_evidence": {
                    "decompiler": _decompiled_signature(decompiled),
                    "limitation": "representations are not automatically classified as ABI conflicts",
                },
                "dependencies": {
                    "calls": calls,
                    "callers": callers,
                    "globals": globals_,
                    "indirect_calls": result_for("indirect-calls", requested).get("calls", []),
                },
                "classes": {
                    "names": sorted(related_classes, key=str.casefold),
                    "layouts": [row for row in field_rows if row["name"] in related_classes],
                    "source": [
                        row for row in indexed_classes if row["qualified_name"] in related_classes
                    ],
                },
                "unresolved": {
                    "boundary_defects": _assertion_boundary_defects(
                        by_requested[requested], boundaries
                    ),
                    "assertions": by_requested[requested],
                },
            }
        )
    return records


def recovery_context_report(settings: Any, address: str, selector: str = "wiz8") -> dict[str, Any]:
    return recovery_context_reports(settings, [address], selector)[0]
