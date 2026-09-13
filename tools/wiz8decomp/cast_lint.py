"""Reject new source-level escape hatches in recovered C++.

The recovered model keeps ``reinterpret_cast`` only where the type system cannot
yet express the storage: an external ABI, raw serialized/pixel memory, tagged
storage, deliberate bit reinterpretation, or an explicitly unresolved site. A
new cast must say so where it is written::

    value = reinterpret_cast<Type*>(raw); // reinterpret-ok: SGP userdata slot

C-style casts are not an acceptable way to bypass that rule. New C-style casts
in recovered C++ need an equally explicit ``c-style-cast-ok: <reason>`` marker;
ordinary recovery should instead correct the canonical type or use the specific
C++ cast that describes the proven conversion.

Likewise, ``clang-format off`` is a source-shaping escape hatch, not a matching
tool. A newly introduced suppression needs ``format-off-ok: <reason>`` on the
same line and should cover the smallest construct the formatter genuinely
cannot preserve. Unlike casts, moving an old suppression into a new source
region is deliberately re-reviewed.

The gate also protects the released SGP baseline. A changed ``src/sgp`` C/C++
file must already carry the dated Wizardry-reconstruction modification notice
used by accepted SGP derivatives. This catches incidental edits to pristine
source while allowing evidence-backed work in files already established as
Wizardry revisions.

The gate inspects added lines of the current Jujutsu change stack (or of a Git
checkout against its baseline branch). Existing casts are not re-litigated;
ones moved between files are recognized by their removed counterpart. Cast and
format checks cover recovered product headers and sources under ``src/wiz8``
and ``include/wiz8``.
"""

from __future__ import annotations

import os
import re
from collections import Counter
from pathlib import Path
from typing import Any

from .subprocesses import run

MARKER = "reinterpret-ok"
C_STYLE_MARKER = "c-style-cast-ok"
FORMAT_OFF_MARKER = "format-off-ok"
SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")
_CPP_SUFFIXES = (".cpp", ".cc", ".cxx", ".h", ".hpp")
_SGP_SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp")
_GIT_BASES: tuple[str, ...] = ("@{upstream}", "origin/main", "origin/master", "main", "master")

_HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")
_MARKER = re.compile(r"reinterpret-ok:\s*\S", re.IGNORECASE)
_CAST = re.compile(r"reinterpret_cast")
_C_STYLE_MARKER = re.compile(r"c-style-cast-ok:\s*\S", re.IGNORECASE)
_FORMAT_OFF = re.compile(r"clang-format\s+off", re.IGNORECASE)
_FORMAT_OFF_MARKER = re.compile(r"format-off-ok:\s*\S", re.IGNORECASE)
_SGP_NOTICE = re.compile(
    r"(?:Modified for the Wizardry 8 reconstruction|Wizardry[^\n]{0,100}\breconstruct(?:ed|ion)\b)"
    r"[^\n]*\b20\d{2}-\d{2}-\d{2}\b",
    re.IGNORECASE,
)

# Deliberately conservative: catch builtins, common C ABI typedefs, and the
# project type families without guessing that every parenthesized identifier is
# a type. Project types use W8/sr/st followed by a capital letter; this avoids
# treating constants such as W8_MAX_MONSTER_ATTACKS as types. The look-behind
# keeps declarations/calls such as f(int) and sizeof(int) out of the match; the
# look-ahead requires a following expression token.
_C_STYLE_CAST = re.compile(
    r"(?<![\w>])\(\s*(?:const\s+|volatile\s+)*(?:(?:unsigned|signed)\s+)?"
    r"(?:char|short|int|long(?:\s+long)?|float|double|wchar_t|void|"
    r"HWFILE|BOOLEAN|BOOL|BYTE|WORD|DWORD|UINT(?:8|16|32)?|INT(?:8|16|32)?|"
    r"S32|U32|STR8?|STR16|(?:W8|sr|st)[A-Z][A-Za-z0-9_]*(?:::\w+)*)"
    r"\s*(?:\*+\s*)?(?:const\s*)?\)\s*(?=[A-Za-z_(&*+\-!~])"
)


class CastGateError(RuntimeError):
    """A new source escape hatch lacked evidence, or the baseline was unusable."""


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


def baseline_diff(repository: Path) -> tuple[str, str]:
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


def added_lines_without_marker(
    diff: str,
    needle: re.Pattern[str],
    marker: re.Pattern[str],
    *,
    ignore_moved: bool = True,
) -> list[dict[str, Any]]:
    """Added source lines matching ``needle`` that lack ``marker``.

    With ``ignore_moved`` (the default), a line moved between files is
    recognized by its removed counterpart so relocation does not read as a new
    occurrence. Suppressions can opt out because moving one changes which code
    escapes the tool.
    """
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
                    and needle.search(content)
                    and not marker.search(content)
                ):
                    added.append({"file": current, "line": line_number, "text": stripped[:200]})
                new_remaining -= 1
                line_number += 1
            elif raw.startswith("-"):
                content = raw[1:]
                if needle.search(content):
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
    if not ignore_moved:
        return added
    violations = []
    for item in added:
        if removed[item["text"]]:
            removed[item["text"]] -= 1
            continue
        violations.append(item)
    return violations


def _changed_files(diff: str) -> set[str]:
    files: set[str] = set()
    for raw in diff.splitlines():
        if not raw.startswith("+++ "):
            continue
        target = raw[4:].strip()
        if target == "/dev/null":
            continue
        files.add(target[2:] if target[:2] in ("a/", "b/") else target)
    return files


def _added_c_style_casts(diff: str) -> list[dict[str, Any]]:
    return [
        item
        for item in added_lines_without_marker(diff, _C_STYLE_CAST, _C_STYLE_MARKER)
        if str(item["file"]).lower().endswith(_CPP_SUFFIXES)
    ]


def _added_format_off(diff: str) -> list[dict[str, Any]]:
    return added_lines_without_marker(diff, _FORMAT_OFF, _FORMAT_OFF_MARKER, ignore_moved=False)


def _sgp_notice_violations(repository: Path, diff: str) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for relative in sorted(_changed_files(diff)):
        lowered = relative.lower()
        if not lowered.startswith("src/sgp/") or not lowered.endswith(_SGP_SOURCE_SUFFIXES):
            continue
        path = repository / relative
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        if _SGP_NOTICE.search(text[:4096]):
            continue
        violations.append(
            {
                "file": relative,
                "line": 1,
                "text": "changed pristine SGP source lacks a dated Wizardry reconstruction notice",
            }
        )
    return violations


def _render(items: list[dict[str, Any]]) -> str:
    return "\n  ".join(f"{item['file']}:{item['line']}: {item['text']}" for item in items)


def validate_cast_markers(repository: Path) -> dict[str, Any]:
    base, diff = baseline_diff(repository)
    reinterpret_violations = added_lines_without_marker(diff, _CAST, _MARKER)
    c_style_violations = _added_c_style_casts(diff)
    format_violations = _added_format_off(diff)
    sgp_violations = _sgp_notice_violations(repository, diff)

    errors: list[str] = []
    if reinterpret_violations:
        errors.append(
            "new reinterpret_cast lines need a 'reinterpret-ok: reason' comment "
            "(or an evidence-backed typed replacement):\n  " + _render(reinterpret_violations)
        )
    if c_style_violations:
        errors.append(
            "new C-style casts in recovered C++ need a typed/C++-cast replacement or "
            "a 'c-style-cast-ok: reason' comment:\n  " + _render(c_style_violations)
        )
    if format_violations:
        errors.append(
            "new clang-format off directives need a 'format-off-ok: reason' on the same line; "
            "keep suppressions to the smallest necessary construct:\n  "
            + _render(format_violations)
        )
    if sgp_violations:
        errors.append(
            "changed pristine SGP source needs the dated Wizardry-reconstruction modification "
            "notice before it can become a product derivative:\n  " + _render(sgp_violations)
        )
    if errors:
        raise CastGateError("source hygiene gate failed:\n" + "\n".join(errors))

    return {
        "ok": True,
        "gate": "source-cast-format-hygiene",
        "base": base,
        "markers": [MARKER, C_STYLE_MARKER, FORMAT_OFF_MARKER],
        "scope": [*SCOPE_PREFIXES, "src/sgp/"],
    }
