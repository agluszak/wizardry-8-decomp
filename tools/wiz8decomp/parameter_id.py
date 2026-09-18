"""Targeted Decompiler Parameter ID planning for unrecovered DEFAULT/ANALYSIS functions.

Never overwrites IMPORTED/USER_DEFINED signatures. Recovered ``FUNCTION``
markers are skipped. This is an investigative collect-only analysis; it is not
part of ``ghidra sync``.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Sequence
from pathlib import Path
from typing import Any

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
