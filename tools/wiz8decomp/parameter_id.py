"""Targeted Decompiler Parameter ID for unrecovered DEFAULT/ANALYSIS functions.

Never overwrites IMPORTED/USER_DEFINED signatures. Recovered ``FUNCTION``
markers are skipped. Intended for disposable enrichment candidates; measure
callers and require decompiler-quality debt ≤ 0 before promotion.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .ghidra.mutations import apply_rows
from .paths import atomic_json
from .source_index import source_functions

_SCHEMA = "wiz8.parameter-id-v1"
_APPLY_SOURCES = frozenset({"DEFAULT", "ANALYSIS"})
_PROTECTED_SOURCES = frozenset({"IMPORTED", "USER_DEFINED"})


def _signature_source(function: Any) -> str:
    source = function.getSignatureSource()
    name = getattr(source, "name", None)
    if callable(name):
        try:
            name = name()
        except TypeError:
            name = None
    if name:
        return str(name)
    return str(source)


def collect_parameter_id_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Plan Parameter ID for unrecovered functions with weak signature sources."""

    recovered = {
        address
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION"
    }
    space = program.getAddressFactory().getDefaultAddressSpace()
    manager = program.getFunctionManager()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()

    if addresses is not None:
        candidates = [manager.getFunctionAt(space.getAddress(address)) for address in addresses]
    else:
        iterator = manager.getFunctions(True)
        candidates = []
        while iterator.hasNext():
            candidates.append(iterator.next())

    for function in candidates:
        if function is None:
            counts["missing-function"] += 1
            continue
        entry = int(function.getEntryPoint().getOffset())
        source = _signature_source(function)
        if function.isThunk() or function.isExternal():
            action = "skip-thunk-or-external"
        elif entry in recovered:
            action = "skip-recovered"
        elif source in _PROTECTED_SOURCES:
            action = "skip-protected-signature"
        elif source in _APPLY_SOURCES:
            action = "commit-params"
        else:
            action = "skip-other-source"
        counts[action] += 1
        if action != "commit-params":
            continue
        rows.append(
            {
                "address": f"0x{entry:08x}",
                "name": function.getName(True),
                "signature_source": source,
                "action": action,
            }
        )
        if limit is not None and len(rows) >= limit:
            break
    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": counts["commit-params"],
        "functions": rows,
    }


def _commit_high_params(high: Any) -> None:
    from ghidra.program.model.pcode import HighFunctionDBUtil  # type: ignore[import-not-found]
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    option = HighFunctionDBUtil.ReturnCommitOption.COMMIT_NO_VOID
    HighFunctionDBUtil.commitParamsToDatabase(high, True, option, SourceType.ANALYSIS)


def _apply_parameter_id_row(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
    from .ghidra.semantic import _high_function

    if row.get("action") != "commit-params":
        return {**row, "error": f"unexpected-action:{row.get('action')}"}
    space = program.getAddressFactory().getDefaultAddressSpace()
    address = int(str(row["address"]), 0)
    function = program.getFunctionManager().getFunctionAt(space.getAddress(address))
    if function is None:
        return {**row, "error": "missing-function"}
    source = _signature_source(function)
    if source in _PROTECTED_SOURCES:
        return {**row, "error": "protected-signature", "signature_source": source}
    high = _high_function(program, function, "paramid", profile="analysis")
    _commit_high_params(high)
    return {
        "address": row["address"],
        "name": row.get("name"),
        "action": "commit-params",
        "signature_source_before": source,
        "signature_source_after": _signature_source(function),
    }


def apply_parameter_id(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    rows = [row for row in plan.get("functions", []) if row.get("action") == "commit-params"]
    return apply_rows(
        program, rows, _apply_parameter_id_row, description="Commit decompiler parameter ID"
    )


def run_parameter_id(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        try:
            plan = collect_parameter_id_plan(
                settings.repo_dir,
                program,
                target=target,
                addresses=addresses,
                limit=limit,
            )
            result: dict[str, Any] = {
                "schema": _SCHEMA,
                "program": program_name,
                "apply": apply,
                "counts": plan["counts"],
                "actionable": plan["actionable"],
            }
            if apply:
                applied = apply_parameter_id(program, plan)
                dispose_sessions()
                program.save("Commit decompiler parameter ID", pyghidra.task_monitor())
                result["applied"] = applied.get("applied")
                result["errors"] = applied.get("errors")
                result["apply_errors"] = len(applied.get("errors") or [])
            else:
                result["functions"] = plan["functions"][:50]
                result["planned"] = len(plan["functions"])
            destination = settings.build_dir / "parameter-id"
            atomic_json(destination / "plan.json", plan)
            return result
        finally:
            dispose_sessions()
