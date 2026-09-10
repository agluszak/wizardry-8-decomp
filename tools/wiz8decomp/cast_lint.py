"""Reject newly added ``reinterpret_cast`` lines without an allow marker.

The recovered model keeps a cast only where the type system cannot yet express
the storage: an external ABI, raw serialized/pixel memory, tagged storage, a
deliberate bit reinterpretation, or an explicitly unresolved site. A new cast
must say so where it is written::

    value = reinterpret_cast<Type*>(raw); // reinterpret-ok: SGP userdata slot

This gate inspects the added lines of the current Jujutsu change (or change
stack) against ``main@origin``. Existing casts are not re-litigated; only new
ones need the comment. The gate covers the recovered product headers and
sources under ``src/wiz8`` and ``include/wiz8``.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .subprocesses import run

MARKER = "reinterpret-ok"
SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")

_HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")
_CAST = "reinterpret_cast"


class CastGateError(RuntimeError):
    """A new cast did not declare its boundary."""


def _diff_base(repository: Path) -> str | None:
    if not (repository / ".jj").is_dir():
        return None
    probe = run(
        ["jj", "log", "-r", "main@origin", "--no-graph", "-T", "change_id"],
        cwd=repository,
        check=False,
    )
    return "main@origin" if probe.exit_status == 0 and probe.stdout.strip() else "@-"


def _added_casts(diff: str) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    current: str | None = None
    line_number = 0
    for raw in diff.splitlines():
        if raw.startswith("+++ "):
            target = raw[4:].strip()
            current = (
                None
                if target == "/dev/null"
                else (target[2:] if target[:2] in ("a/", "b/") else target)
            )
            continue
        hunk = _HUNK.match(raw)
        if hunk:
            line_number = int(hunk.group(1))
            continue
        if raw.startswith("+") and not raw.startswith("+++"):
            if current and current.startswith(SCOPE_PREFIXES):
                content = raw[1:]
                if _CAST in content and MARKER not in content.lower():
                    violations.append(
                        {"file": current, "line": line_number, "text": content.strip()[:200]}
                    )
            line_number += 1
        elif raw.startswith(" ") or raw == "":
            line_number += 1
    return violations


def validate_cast_markers(repository: Path) -> dict[str, Any]:
    base = _diff_base(repository)
    if base is None:
        return {"ok": True, "gate": "reinterpret-cast", "skipped": "no Jujutsu checkout"}
    diff = run(
        ["jj", "diff", "--git", "--from", base, "--to", "@"],
        cwd=repository,
        check=False,
    )
    if diff.exit_status != 0:
        raise CastGateError("could not read the change diff:\n" + diff.stderr.strip())
    violations = _added_casts(diff.stdout)
    if violations:
        rendered = [f"{item['file']}:{item['line']}: {item['text']}" for item in violations]
        raise CastGateError(
            "new reinterpret_cast lines need a 'reinterpret-ok: reason' comment "
            "(or an evidence-backed typed replacement):\n  " + "\n  ".join(rendered)
        )
    return {
        "ok": True,
        "gate": "reinterpret-cast",
        "base": base,
        "marker": MARKER,
        "scope": list(SCOPE_PREFIXES),
    }
