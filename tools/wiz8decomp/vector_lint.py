"""Reject newly added ``W8GrowableVector<void*>`` element claims.

An unresolved pointer-vector specialization keeps its element type unknown
until a producer or consumer identifies it; erasing it to ``void*`` hides that
missing evidence. A new occurrence needs ``vector-void-ok: <reason>`` when
``void*`` is genuinely the source element type. This gate inspects the added
lines of the current Jujutsu change stack (or of a Git checkout against its
baseline branch); existing occurrences are not re-litigated.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .cast_lint import added_lines_without_marker, baseline_diff

MARKER = "vector-void-ok"
_VOID_ELEMENT = re.compile(r"W8GrowableVector\s*<\s*void\s*\*>")
_MARKER = re.compile(r"vector-void-ok:\s*\S", re.IGNORECASE)


class VectorGateError(RuntimeError):
    """A new void* vector element did not justify the erased element type."""


def validate_void_vector_elements(repository: Path) -> dict[str, Any]:
    base, diff = baseline_diff(repository)
    violations = added_lines_without_marker(diff, _VOID_ELEMENT, _MARKER)
    if violations:
        rendered = [f"{item['file']}:{item['line']}: {item['text']}" for item in violations]
        raise VectorGateError(
            "new W8GrowableVector<void*> lines need a 'vector-void-ok: reason' "
            "comment unless a producer/consumer proves the element type:\n  "
            + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "unresolved-vector-elements",
        "base": base,
        "marker": MARKER,
    }
