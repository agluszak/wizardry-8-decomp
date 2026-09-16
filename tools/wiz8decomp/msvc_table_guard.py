"""Conservatively validate receiver-provenance labels from table enrichment.

The local backward decoder is useful for inspecting a write when a function
boundary cannot be recovered, but it must never be treated as if its first
instruction were the real function entry: ECX at an arbitrary mid-function
window is not necessarily incoming ECX.  This pass withdraws receiver
provenance unless a continuous decode from the independently recovered function
entry reaches the table write exactly.
"""

from __future__ import annotations

from collections import Counter
from pathlib import Path
from typing import Any

from .binary.code import disassembler, function_start
from .binary.image import PeImage
from .msvc_table_analysis import _construction_families

_MAX_ENTRY_TRACE = 0x2000


def _entry_anchored(image: PeImage, engine: Any, instruction: int) -> tuple[bool, int | None]:
    start = function_start(image, instruction)
    if start is None or not (0 < instruction - start <= _MAX_ENTRY_TRACE):
        return False, start
    raw = image.read(start, instruction - start)
    chain = list(engine.disasm(raw, start))
    if not chain or chain[-1].address + chain[-1].size != instruction:
        return False, start
    return True, start


def sanitize_receiver_provenance(path: Path, report: dict[str, Any]) -> dict[str, Any]:
    """Withdraw incoming-ECX labels that are not anchored at a function entry."""

    image = PeImage(path)
    engine = disassembler()
    cache: dict[int, tuple[bool, int | None]] = {}
    withdrawn = 0

    for table_kind in ("vftables", "vbtables"):
        for table in report.get(table_kind, []):
            for write in table.get("writes", []):
                provenance = write.get("receiver_provenance")
                if provenance not in {"incoming-ecx", "incoming-ecx-plus-dynamic"}:
                    continue
                instruction = int(write["instruction"], 16)
                anchored = cache.get(instruction)
                if anchored is None:
                    anchored = _entry_anchored(image, engine, instruction)
                    cache[instruction] = anchored
                valid, start = anchored
                if valid:
                    continue
                write["function"] = f"0x{start:08x}" if start is not None else None
                write["receiver_provenance"] = "unknown"
                write["receiver_offset"] = None
                write.pop("function_source_name", None)
                withdrawn += 1

    analysis = report.setdefault("analysis", {})
    counts: Counter[str] = Counter()
    for table_kind in ("vftables", "vbtables"):
        for table in report.get(table_kind, []):
            for write in table.get("writes", []):
                counts[str(write.get("receiver_provenance", "unknown"))] += 1
    analysis["receiver_provenance"] = dict(sorted(counts.items()))
    families = _construction_families(report)
    analysis["construction_family_count"] = len(families)
    analysis["construction_families"] = families
    analysis["receiver_provenance_guard"] = {
        "entry_anchored_writes_checked": len(cache),
        "labels_withdrawn": withdrawn,
    }
    return report
