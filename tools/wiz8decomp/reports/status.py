from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path
from typing import Any

from ..paths import atomic_json, atomic_write
from ..source_model import build_source_model, load_source_index
from .translation_units import (
    derive_intervals,
    function_inventory,
    load_call_site_anchors,
    render_gameplay_map_csv,
)


def _rows(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def _counts(rows: list[dict[str, str]], field: str) -> dict[str, int]:
    return dict(sorted(Counter(row[field] or "unassigned" for row in rows).items()))


def derive_status(repo_dir: Path, ghidra_functions: list[dict[str, str]]) -> dict[str, Any]:
    catalogs = sorted((repo_dir / "evidence" / "reviewed").glob("*/functions.csv"))
    programs = []
    for path in catalogs:
        rows = _rows(path)
        program = path.parent.name
        programs.append(
            {
                "program": program,
                "identities": len(rows),
                "authority": _counts(rows, "authority"),
                "confidence": _counts(rows, "confidence"),
            }
        )

    claims = _rows(repo_dir / "evidence/reviewed/wiz8/claims.csv")
    identity_claims = [
        row
        for row in claims
        if row["entity_kind"] == "function"
        and row["predicate"] in {"accepted-identity", "identity-provenance"}
    ]
    source_functions = build_source_model(repo_dir).functions
    function_addresses = set(source_functions) | {
        int(row["entity_key"], 16)
        for row in identity_claims
        if row["predicate"] == "accepted-identity"
    }
    programs.append(
        {
            "program": "wiz8",
            "identities": len(function_addresses),
            "authority": _counts(identity_claims, "authority"),
            "confidence": _counts(identity_claims, "confidence"),
        }
    )
    classes = load_source_index(repo_dir)["classes"]
    source_units = _rows(repo_dir / "evidence/observations/wiz8/source-tree.csv")
    assertions = _rows(repo_dir / "evidence/observations/wiz8/assertions.csv")
    gameplay = function_inventory(repo_dir, ghidra_functions)
    extra_anchors = load_call_site_anchors(repo_dir)
    intervals = derive_intervals(assertions, extra_anchors)
    gameplay_map, attribution = render_gameplay_map_csv(
        assertions, gameplay, intervals, extra_anchors
    )
    attributed_rows = list(csv.DictReader(gameplay_map.splitlines()))
    attributed_units = {row["source_path"] for row in attributed_rows if row["source_path"]}

    return {
        "schema": "wiz8.recovery-status",
        "programs": programs,
        "wiz8": {
            "function_identities": len(function_addresses),
            "source_functions": len(source_functions),
            "analysis_only_identities": len(function_addresses - set(source_functions)),
            "claims": len(claims),
            "authority": _counts(identity_claims, "authority"),
            "classes": len(classes),
            "source_units": len(source_units),
            "source_units_by_subsystem": _counts(source_units, "subsystem"),
            "gameplay": {
                "functions": len(gameplay),
                "owners": _counts(gameplay, "owner"),
                "translation_unit_attribution": attribution,
                "attributed_source_units": len(attributed_units),
                "unowned_functions": attribution["gap"],
            },
        },
    }


def _table(counts: dict[str, int]) -> list[str]:
    return [f"| `{name}` | {count} |" for name, count in counts.items()]


def render_status_markdown(report: dict[str, Any]) -> str:
    wiz8 = report["wiz8"]
    gameplay = wiz8["gameplay"]
    lines = [
        "# Wizardry recovery status",
        "",
        "Generated from canonical configuration and evidence. Do not edit this report by hand.",
        "",
        "## Programs",
        "",
        "| Program | Canonical identities |",
        "| --- | ---: |",
    ]
    lines.extend(
        f"| `{program['program']}` | {program['identities']} |" for program in report["programs"]
    )
    lines.extend(
        [
            "",
            "## Wiz8.exe",
            "",
            f"- Canonical function identities: {wiz8['function_identities']}",
            f"- Source-owned functions: {wiz8['source_functions']}",
            f"- Analysis-only identities: {wiz8['analysis_only_identities']}",
            f"- Provenance claims: {wiz8['claims']}",
            f"- Source classes: {wiz8['classes']}",
            f"- Observed original source units: {wiz8['source_units']}",
            "",
            "### Name authority",
            "",
            "| Authority | Functions |",
            "| --- | ---: |",
            *_table(wiz8["authority"]),
            "",
            "### Owned gameplay matching",
            "",
            f"- Reviewed functions: {gameplay['functions']}",
            f"- Functions outside an assertion-bounded source interval: {gameplay['unowned_functions']}",
            f"- Original source units represented by attributed functions: {gameplay['attributed_source_units']}",
            "",
            "| Translation-unit attribution | Functions |",
            "| --- | ---: |",
            *_table(gameplay["translation_unit_attribution"]),
            "",
        ]
    )
    return "\n".join(lines)


def status_report(settings: Any) -> dict[str, Any]:
    from ..ghidra.audits import function_inventory as ghidra_function_inventory

    report = derive_status(settings.repo_dir, ghidra_function_inventory(settings))
    report_dir = settings.build_dir / "reports"
    json_path = report_dir / "status.json"
    markdown_path = report_dir / "status.md"
    atomic_json(json_path, report)
    atomic_write(markdown_path, render_status_markdown(report))
    return {
        **report,
        "outputs": [
            str(json_path.relative_to(settings.build_dir.parent)),
            str(markdown_path.relative_to(settings.build_dir.parent)),
        ],
    }
