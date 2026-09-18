"""Repair Ghidra calling conventions from high-confidence source signatures.

Retail/source-backed calling conventions outrank Ghidra's initial
``unknown``/``default`` guess. This module reports disagreements and, when
explicitly requested, applies the source convention to the live program.

Binary heuristics for unrecovered bodies (``ret N``, incoming ECX, vtable
membership) are a follow-on pass; the first slice only projects compiler-owned
``FUNCTION`` marker conventions.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .source_index import source_functions

_SCHEMA = "wiz8.prototype-repair-v1"
_GHIDRA_MODELS = frozenset({"__cdecl", "__stdcall", "__fastcall", "__thiscall"})
# Ghidra's Windows default stack convention is cdecl; treat as soft agreement
# when source says __cdecl, but still promote to an explicit model on apply.
_SOFT_CDECL = frozenset({"default", "unknown"})


def _normalize_ghidra(name: str | None) -> str:
    return str(name or "unknown")


def _normalize_source(name: str | None) -> str | None:
    if not name:
        return None
    return name if name in _GHIDRA_MODELS else None


def classify_pair(ghidra: str, source: str | None) -> str:
    """Classify one (ghidra, source) convention pair."""

    ghidra = _normalize_ghidra(ghidra)
    source = _normalize_source(source)
    if source is None:
        return "no-source-convention"
    if ghidra == source:
        return "agree"
    if ghidra in _SOFT_CDECL and source == "__cdecl":
        return "promote-default-cdecl"
    if ghidra in _SOFT_CDECL:
        return "set-from-source"
    return "hard-disagree"


def collect_source_convention_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
) -> dict[str, Any]:
    """Diff source-index conventions against the live Ghidra program."""

    markers = {
        address: marker
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION"
    }
    if addresses is not None:
        wanted = set(addresses)
        markers = {address: marker for address, marker in markers.items() if address in wanted}

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()

    for address, marker in sorted(markers.items()):
        declaration = marker.declaration
        source_cc = declaration.calling_convention if declaration is not None else None
        function = functions.getFunctionAt(space.getAddress(address))
        if function is None:
            action = "missing-function"
            ghidra_cc = None
        else:
            ghidra_cc = _normalize_ghidra(function.getCallingConventionName())
            action = classify_pair(ghidra_cc, source_cc)
        counts[action] += 1
        if action in {"agree", "no-source-convention"}:
            continue
        rows.append(
            {
                "address": f"0x{address:08x}",
                "name": marker.name,
                "source_file": marker.source_file,
                "ghidra": ghidra_cc,
                "source": source_cc,
                "action": action,
            }
        )

    actionable = [
        row for row in rows if row["action"] in {"promote-default-cdecl", "set-from-source"}
    ]
    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": len(actionable),
        "hard_disagree": counts["hard-disagree"],
        "missing_function": counts["missing-function"],
        "functions": rows,
    }


def apply_source_conventions(
    program: Any,
    plan: Mapping[str, Any],
    *,
    actions: frozenset[str] = frozenset({"promote-default-cdecl", "set-from-source"}),
) -> dict[str, Any]:
    """Apply planned calling conventions inside an open transaction."""

    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    applied: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []

    for row in plan.get("functions", []):
        if row.get("action") not in actions:
            continue
        address = int(row["address"], 0)
        source_cc = row.get("source")
        if source_cc not in _GHIDRA_MODELS:
            continue
        function = functions.getFunctionAt(space.getAddress(address))
        if function is None:
            errors.append({**row, "error": "missing-function"})
            continue
        try:
            function.setCallingConvention(source_cc)
            # Convention-only repair must not claim a full-signature IMPORTED
            # contract. ANALYSIS marks the convention as non-default without
            # implying Param ID should treat the whole prototype as imported.
            # A future path that applies a complete source signature may set
            # SourceType.IMPORTED at that point.
            function.setSignatureSource(SourceType.ANALYSIS)
        except Exception as exc:  # noqa: BLE001 - surface per-row apply failures
            errors.append({**row, "error": str(exc)})
            continue
        applied.append(
            {
                "address": row["address"],
                "name": row.get("name"),
                "from": row.get("ghidra"),
                "to": source_cc,
                "action": row.get("action"),
            }
        )
    return {"applied": len(applied), "errors": errors, "functions": applied}
