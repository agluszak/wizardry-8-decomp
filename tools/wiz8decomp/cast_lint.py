"""Reject newly added ``reinterpret_cast`` lines without an allow marker.

The recovered model keeps a cast only where the type system cannot yet express
the storage: an external ABI, raw serialized/pixel memory, tagged storage, a
deliberate bit reinterpretation, or an explicitly unresolved site. A new cast
must say so where it is written::

    value = reinterpret_cast<Type*>(raw); // reinterpret-ok: SGP userdata slot

This gate inspects the added lines of the current Jujutsu change stack (or of a
Git checkout against its baseline branch). Existing casts are not re-litigated;
only ones introduced by the change need the comment, and a cast line that moved
between files is recognized by its removed counterpart. The gate covers the
recovered product headers and sources under ``src/wiz8`` and ``include/wiz8``.
"""

from __future__ import annotations

import os
import re
from collections import Counter
from pathlib import Path
from typing import Any

from .subprocesses import run

MARKER = "reinterpret-ok"
SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")
_CAST = "reinterpret_cast"
_GIT_BASES: tuple[str, ...] = ("@{upstream}", "origin/main", "origin/master", "main", "master")

_HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")
_MARKER = re.compile(r"reinterpret-ok:\s*\S", re.IGNORECASE)


class CastGateError(RuntimeError):
    """A new cast did not declare its boundary, or the baseline was unusable."""


def _git_base(repository: Path) -> str:
    candidates = list(_GIT_BASES)
    ci_base = os.environ.get("GITHUB_BASE_REF", "").strip()
    if ci_base:
        candidates[:0] = [f"origin/{ci_base}", ci_base]
    for candidate in candidates:
        probe = run(
            ["git", "rev-parse", "--verify", "--quiet", f"{candidate}^{{commit}}"],
            cwd=repository,
            check=False,
        )
        if probe.exit_status != 0 or not probe.stdout.strip():
            continue
        base = probe.stdout.strip()
        merge = run(["git", "merge-base", base, "HEAD"], cwd=repository, check=False)
        if merge.exit_status == 0 and merge.stdout.strip():
            return merge.stdout.strip()
        return base
    raise CastGateError(
        "no Git baseline branch found; fetch a main branch or create a local main "
        "so the gate can tell which casts the change introduces"
    )


def _jj_base(repository: Path) -> str:
    for candidate in ("main@origin", "trunk()", "@-"):
        probe = run(
            ["jj", "log", "-r", candidate, "--no-graph", "-T", "change_id"],
            cwd=repository,
            check=False,
        )
        if probe.exit_status == 0 and probe.stdout.strip():
            return candidate
    raise CastGateError("no Jujutsu baseline found; cannot tell which casts the change introduces")


def _baseline(repository: Path) -> tuple[str, str]:
    """Return the baseline name and the unified diff from it to the current tree."""
    if (repository / ".jj").is_dir():
        base = _jj_base(repository)
        diff = run(
            ["jj", "diff", "--git", "--from", base, "--to", "@"],
            cwd=repository,
            check=False,
        )
    else:
        base = _git_base(repository)
        diff = run(
            ["git", "diff", "--no-color", "--no-ext-diff", base],
            cwd=repository,
            check=False,
        )
    if diff.exit_status != 0:
        raise CastGateError("could not read the change diff:\n" + diff.stderr.strip())
    return base, diff.stdout


def _added_casts(diff: str) -> list[dict[str, Any]]:
    added: list[dict[str, Any]] = []
    removed: Counter[str] = Counter()
    current: str | None = None
    line_number = 0
    old_remaining = new_remaining = 0
    for raw in diff.splitlines():
        if old_remaining or new_remaining:
            # Hunk body: line counts are the only reliable way to tell an added
            # source line (``+++i`` for ``++i``) from a ``+++`` file header.
            if raw.startswith("+"):
                content = raw[1:]
                stripped = content.strip()
                if (
                    current
                    and current.startswith(SCOPE_PREFIXES)
                    and _CAST in content
                    and not _MARKER.search(content)
                ):
                    added.append({"file": current, "line": line_number, "text": stripped[:200]})
                new_remaining -= 1
                line_number += 1
            elif raw.startswith("-"):
                content = raw[1:]
                if _CAST in content:
                    removed[content.strip()] += 1
                old_remaining -= 1
            elif raw.startswith(" "):
                old_remaining -= 1
                new_remaining -= 1
                line_number += 1
            continue
        if raw.startswith("diff --git "):
            current = None
        elif raw.startswith("+++ "):
            target = raw[4:].strip()
            current = (
                None
                if target == "/dev/null"
                else (target[2:] if target[:2] in ("a/", "b/") else target)
            )
        elif raw.startswith("@@ "):
            hunk = _HUNK.match(raw)
            if hunk:
                old_remaining = int(hunk.group(2) or 1)
                new_remaining = int(hunk.group(4) or 1)
                line_number = int(hunk.group(3))
    violations = []
    for item in added:
        if removed[item["text"]]:
            removed[item["text"]] -= 1
            continue
        violations.append(item)
    return violations


def validate_cast_markers(repository: Path) -> dict[str, Any]:
    base, diff = _baseline(repository)
    violations = _added_casts(diff)
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
