"""Bounded inspection of existing reccmp reports; never runs analysis."""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from typing import Any

from reccmp.ghidriff.report import comparison_changes
from reccmp.ghidriff.results import Outcome

from ..comparison import report_directory
from ..paths import compile_database_relative


def comparison_report(
    repository: Path,
    target: str,
    *,
    report: Path | None = None,
    against: Path | None = None,
    addresses: list[int] | None = None,
    files: list[Path] | None = None,
    outcome: str | None = None,
    limit: int = 20,
    diff_lines: int = 0,
) -> dict[str, Any]:
    if outcome is not None:
        Outcome(outcome)
    path = report or report_directory(repository, target) / "summary.json"
    path = path.resolve()
    if not path.is_file():
        raise FileNotFoundError(
            f"comparison report missing: {path}; run `uv run wiz8 compare` with a selection"
        )
    current = json.loads(path.read_text())
    if current["target"].upper() != target.upper():
        raise ValueError("report target does not match --program")
    previous = json.loads(against.read_text()) if against else None
    changes = comparison_changes(current, previous) if previous is not None else None
    rows = {int(row["orig"], 16): row for row in current["functions"]}
    if previous is not None:
        for row in previous["functions"]:
            rows.setdefault(int(row["orig"], 16), row)
    wanted = set(addresses or [])
    wanted_files = {compile_database_relative(str(file), repository) for file in files or []}
    if None in wanted_files:
        raise ValueError("source file filter must be inside the repository")
    selected = []
    for address, row in sorted(rows.items()):
        if wanted and address not in wanted:
            continue
        if wanted_files and (
            not row["source"]
            or compile_database_relative(row["source"]["path"], repository) not in wanted_files
        ):
            continue
        if outcome is not None and row["outcome"] != outcome:
            continue
        if changes is not None and address not in changes:
            continue
        selected.append((address, row))
    remaining_lines = diff_lines
    functions = []
    for address, row in selected[:limit]:
        result = {key: row[key] for key in ("orig", "name", "outcome", "basis", "source")}
        result["data_findings"] = len(row["data"])
        result["analysis_failures"] = len(row["failures"])
        result["diff_lines"] = len(row["code_diff"])
        if changes is not None:
            result["change"] = changes[address]
        if diff_lines:
            result["code_diff"] = row["code_diff"][:remaining_lines]
            remaining_lines -= len(result["code_diff"])
            result["diff_truncated"] = len(result["code_diff"]) < len(row["code_diff"])
        functions.append(result)
    return {
        "report": str(path),
        "inputs": current["inputs"],
        "matched": len(selected),
        "omitted": max(0, len(selected) - limit),
        "counts": dict(Counter(row["outcome"] for _, row in selected)),
        "changes": dict(Counter(changes[address] for address, _ in selected))
        if changes is not None
        else None,
        "functions": functions,
    }
