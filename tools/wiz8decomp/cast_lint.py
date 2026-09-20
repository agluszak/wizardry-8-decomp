"""Reject new source-level escape hatches in recovered C++.

The recovered model keeps ``reinterpret_cast`` only where the type system cannot
yet express the storage: an external ABI, raw serialized/pixel memory, tagged
storage, deliberate bit reinterpretation, or an explicitly unresolved site. A
new cast must say so where it is written::

    value = reinterpret_cast<Type*>(raw); // reinterpret-ok: SGP userdata slot

The marker may also sit in an immediately preceding comment or on a wrapped
continuation of that statement. Formatting a cast must not require disabling
the formatter. A marker on another statement does not apply.

C-style casts are not an acceptable way to bypass that rule. New C-style casts
in recovered C++ need an equally explicit ``c-style-cast-ok: <reason>`` marker;
ordinary recovery should instead correct the canonical type or use the specific
C++ cast that describes the proven conversion.

Likewise, ``clang-format off`` is a source-shaping escape hatch, not a matching
tool. A newly introduced suppression needs ``format-off-ok: <reason>`` on the
same line and should cover the smallest construct the formatter genuinely
cannot preserve. Unlike casts, moving an old suppression into a new source
region is deliberately re-reviewed.

Literal byte-offset arithmetic after converting a typed object to
``char *``/``unsigned char *`` is also gated. Known layout must use the named
member. Only genuinely unresolved or external layouts may carry
``raw-offset-ok: <reason>``; the marker never justifies bypassing an already
modeled field.

The gate also protects the released SGP baseline. A changed ``src/sgp`` C/C++
file must already carry the dated Wizardry-reconstruction modification notice
used by accepted SGP derivatives. This catches incidental edits to pristine
source while allowing evidence-backed work in files already established as
Wizardry revisions.

New unions in recovered Wizardry and SurRender headers require a nearby
``union-ok:`` comment citing positive evidence for overlapping source storage.
Two accesses with different types at one offset are a reason to audit the
record or class boundary, not positive union evidence.

The gate inspects added lines of the current Jujutsu change stack (or of a Git
checkout against its baseline branch). Existing casts are not re-litigated;
ones moved between files are recognized by their removed counterpart. Cast and
format checks cover recovered product headers and sources under ``src/wiz8``,
``include/wiz8`` and ``include/surrender``. The union check covers headers.
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
RAW_OFFSET_MARKER = "raw-offset-ok"
UNION_MARKER = "union-ok"
SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/", "include/surrender/")
_CPP_SUFFIXES = (".cpp", ".cc", ".cxx", ".h", ".hpp")
_SGP_SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp")
_GIT_BASES: tuple[str, ...] = ("@{upstream}", "origin/main", "origin/master", "main", "master")

_HUNK = re.compile(r"^@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@")
_MARKER = re.compile(r"reinterpret-ok:\s*\S", re.IGNORECASE)
_CAST = re.compile(r"reinterpret_cast")
_C_STYLE_MARKER = re.compile(r"c-style-cast-ok:\s*\S", re.IGNORECASE)
_FORMAT_OFF = re.compile(r"clang-format\s+off", re.IGNORECASE)
_FORMAT_OFF_MARKER = re.compile(r"format-off-ok:\s*\S", re.IGNORECASE)
_RAW_OFFSET_MARKER = re.compile(r"raw-offset-ok:\s*\S", re.IGNORECASE)
_UNION = re.compile(r"^\s*(?:typedef\s+)?union\b")
_UNION_MARKER = re.compile(r"union-ok:\s*\S", re.IGNORECASE)
_RAW_BYTE_OFFSET = re.compile(
    r"reinterpret_cast\s*<\s*(?:const\s+)?(?:unsigned\s+)?char\s*\*\s*>\s*"
    r"\((?:(?![;{}]).)*?\)\s*(?:\+\s*(?:0[xX][0-9A-Fa-f]+|\d+)\b\s*)+",
    re.DOTALL,
)
_STATEMENT_TOKEN = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[;{}]',
    re.DOTALL,
)
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


def _added_source_lines(diff: str) -> dict[str, set[int]]:
    """Return added line numbers for recovered C++ files, keyed by path."""
    added: dict[str, set[int]] = {}
    current: str | None = None
    line_number = 0
    old_remaining = new_remaining = 0
    for raw in diff.splitlines():
        if old_remaining or new_remaining:
            if raw.startswith("+"):
                if current and current.startswith(SCOPE_PREFIXES):
                    added.setdefault(current, set()).add(line_number)
                new_remaining -= 1
                line_number += 1
            elif raw.startswith("-"):
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
    return added


def _statement_has_marker(text: str, start: int, marker: re.Pattern[str]) -> bool:
    line_start = text.rfind("\n", 0, start) + 1
    if line_start > 0:
        previous_end = line_start - 1
        previous_start = text.rfind("\n", 0, previous_end) + 1
        previous = text[previous_start:previous_end]
        if previous.lstrip().startswith("//") and marker.search(previous):
            return True

    for token in _STATEMENT_TOKEN.finditer(text, start):
        value = token.group()
        if value.startswith(("//", "/*")):
            if marker.search(value):
                return True
        elif value in {";", "{", "}"}:
            trailing = re.match(r"[ \t]*(//[^\n]*|/\*.*?\*/)", text[token.end() :], re.DOTALL)
            return trailing is not None and marker.search(trailing.group()) is not None
    return False


def _raw_offset_violations(repository: Path, diff: str) -> list[dict[str, Any]]:
    """Find new literal byte offsets applied to typed-object byte casts."""
    added = _added_source_lines(diff)
    violations: list[dict[str, Any]] = []
    for relative, line_numbers in sorted(added.items()):
        if not relative.lower().endswith(_CPP_SUFFIXES):
            continue
        path = repository / relative
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        for match in _RAW_BYTE_OFFSET.finditer(text):
            start_line = text.count("\n", 0, match.start()) + 1
            end_line = text.count("\n", 0, match.end()) + 1
            if not any(line in line_numbers for line in range(start_line, end_line + 1)):
                continue
            if _statement_has_marker(text, match.start(), _RAW_OFFSET_MARKER):
                continue
            violations.append(
                {
                    "file": relative,
                    "line": start_line,
                    "text": " ".join(match.group().split())[:200],
                }
            )
    return violations


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


def _unmarked_statements(
    repository: Path, items: list[dict[str, Any]], marker: re.Pattern[str]
) -> list[dict[str, Any]]:
    """Associate a wrapped cast with its own comment, using current source.

    Diff context may end before a long statement's closing line. Read only the
    affected files; strings and comments cannot terminate a C++ statement.
    """
    sources: dict[str, list[str]] = {}
    violations = []
    for item in items:
        filename = item["file"]
        if filename not in sources:
            sources[filename] = (
                (repository / filename).read_text(encoding="utf-8").splitlines(keepends=True)
            )
        lines = sources[filename]
        index = item["line"] - 1
        if (
            index > 0
            and lines[index - 1].lstrip().startswith("//")
            and marker.search(lines[index - 1])
        ):
            continue
        statement = "".join(lines[index:])
        # Skip any function/block opener before the cast on its first line.
        cast = _CAST.search(lines[index]) or _C_STYLE_CAST.search(lines[index])
        start = cast.start() if cast else 0
        marked = False
        for token in _STATEMENT_TOKEN.finditer(statement, start):
            value = token.group()
            if value.startswith(("//", "/*")):
                if marker.search(value):
                    marked = True
                    break
            elif value in {";", "{", "}"}:
                # A trailing comment belongs to the statement that just ended,
                # but never cross a newline or the next statement to find one.
                trailing = re.match(
                    r"[ \t]*(//[^\n]*|/\*.*?\*/)", statement[token.end() :], re.DOTALL
                )
                marked = trailing is not None and marker.search(trailing.group()) is not None
                break
        if not marked:
            violations.append(item)
    return violations


def validate_cast_markers(repository: Path) -> dict[str, Any]:
    base, diff = baseline_diff(repository)
    reinterpret_violations = _unmarked_statements(
        repository, added_lines_without_marker(diff, _CAST, _MARKER), _MARKER
    )
    c_style_violations = _unmarked_statements(
        repository, _added_c_style_casts(diff), _C_STYLE_MARKER
    )
    format_violations = _added_format_off(diff)
    raw_offset_violations = _raw_offset_violations(repository, diff)
    sgp_violations = _sgp_notice_violations(repository, diff)
    union_violations = []
    for item in added_lines_without_marker(diff, _UNION, _UNION_MARKER):
        if not item["file"].startswith(("include/wiz8/", "include/surrender/")):
            continue
        lines = (repository / item["file"]).read_text(encoding="utf-8").splitlines()
        index = item["line"] - 1
        if index > 0 and _UNION_MARKER.search(lines[index - 1]):
            continue
        union_violations.append(item)

    errors: list[str] = []
    if reinterpret_violations:
        errors.append(
            "new reinterpret_cast statements need a 'reinterpret-ok: reason' comment "
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
    if raw_offset_violations:
        errors.append(
            "new literal byte offsets into typed objects need the named field instead; only "
            "genuinely unresolved/external layouts may use 'raw-offset-ok: reason':\n  "
            + _render(raw_offset_violations)
        )
    if sgp_violations:
        errors.append(
            "changed pristine SGP source needs the dated Wizardry-reconstruction modification "
            "notice before it can become a product derivative:\n  " + _render(sgp_violations)
        )
    if union_violations:
        errors.append(
            "new recovered layout unions need a preceding 'union-ok: positive source evidence' "
            "comment; differing types at one offset alone do not establish a source union:\n  "
            + _render(union_violations)
        )
    if errors:
        raise CastGateError("source hygiene gate failed:\n" + "\n".join(errors))

    return {
        "ok": True,
        "gate": "source-cast-format-hygiene",
        "base": base,
        "markers": [MARKER, C_STYLE_MARKER, FORMAT_OFF_MARKER, RAW_OFFSET_MARKER, UNION_MARKER],
        "scope": [*SCOPE_PREFIXES, "src/sgp/"],
    }
