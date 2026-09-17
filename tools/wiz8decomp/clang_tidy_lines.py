"""Compute the changed-line filter for wiz8-redundant-scalar-cast."""

from __future__ import annotations

import os
import re
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

FILTER_ENV = "WIZ8_REDUNDANT_CAST_LINES"
SCOPES = (
    "src/wiz8/",
    "include/wiz8/",
    "src/surrender/",
    "include/surrender/",
    "src/srext_jpegimporter/",
    "src/srext_unzip/",
)
CPP_SUFFIXES = (".cpp", ".cc", ".cxx", ".h", ".hpp")
HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@")


def _run(command: list[str], *, cwd: Path) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def _target_path(header: str) -> str | None:
    target = header[4:].strip()
    if target == "/dev/null":
        return None
    if target.startswith(("a/", "b/")):
        target = target[2:]
    if not target.startswith(SCOPES) or not target.lower().endswith(CPP_SUFFIXES):
        return None
    return target


def added_line_filter(diff: str) -> str:
    """Encode added lines from a unified diff into the plugin filter format."""

    additions: list[tuple[str, int, str]] = []
    removals: Counter[str] = Counter()
    current: str | None = None
    line_number = 0
    in_hunk = False

    for raw in diff.splitlines():
        if raw.startswith("diff --git "):
            current = None
            in_hunk = False
            continue
        if raw.startswith("+++ "):
            current = _target_path(raw)
            continue
        if raw.startswith("@@ "):
            match = HUNK.match(raw)
            in_hunk = match is not None
            if match is not None:
                line_number = int(match.group(1))
            continue
        if not in_hunk:
            continue
        if raw.startswith("+"):
            if current is not None:
                additions.append((current, line_number, raw[1:].strip()))
            line_number += 1
        elif raw.startswith("-"):
            removals[raw[1:].strip()] += 1
        elif raw.startswith(" "):
            line_number += 1
        elif raw.startswith("\\"):
            continue
        else:
            in_hunk = False

    lines: dict[str, list[int]] = defaultdict(list)
    for filename, line, text in additions:
        if removals[text]:
            removals[text] -= 1
            continue
        lines[filename].append(line)

    encoded: list[str] = []
    for filename in sorted(lines):
        numbers = sorted(set(lines[filename]))
        ranges: list[tuple[int, int]] = []
        for number in numbers:
            if ranges and number == ranges[-1][1] + 1:
                ranges[-1] = (ranges[-1][0], number)
            else:
                ranges.append((number, number))
        encoded_ranges = ",".join(
            str(first) if first == last else f"{first}-{last}" for first, last in ranges
        )
        encoded.append(f"{filename}@{encoded_ranges}")
    return ";".join(encoded)


def _jj_changed_line_filter(repository: Path) -> str | None:
    if not (repository / ".jj").exists():
        return None
    result = _run(
        [
            "jj",
            "diff",
            "--from",
            "main@origin",
            "--git",
            "--",
            *SCOPES,
        ],
        cwd=repository,
    )
    if result.returncode != 0:
        return None
    return added_line_filter(result.stdout)


def _git_changed_line_filter(repository: Path) -> str | None:
    if not (repository / ".git").exists():
        return None
    baseline = None
    for candidate in ("origin/main", "origin/master", "main", "master"):
        probe = _run(
            ["git", "rev-parse", "--verify", "--quiet", f"{candidate}^{{commit}}"],
            cwd=repository,
        )
        if probe.returncode != 0 or not probe.stdout.strip():
            continue
        merge = _run(["git", "merge-base", candidate, "HEAD"], cwd=repository)
        if merge.returncode == 0 and merge.stdout.strip():
            baseline = merge.stdout.strip()
        else:
            baseline = probe.stdout.strip()
        break
    if baseline is None:
        return None
    diff = _run(
        [
            "git",
            "diff",
            "--no-color",
            "--no-ext-diff",
            "--unified=0",
            baseline,
            "--",
            *SCOPES,
        ],
        cwd=repository,
    )
    if diff.returncode != 0:
        return None
    return added_line_filter(diff.stdout)


def redundant_cast_line_filter(repository: Path) -> str:
    """Resolve the plugin filter for this checkout.

    An explicit environment value wins, including `*` for a full-corpus audit.
    Otherwise prefer Jujutsu, then Git. Missing VCS information yields an empty
    filter so the check stays quiet instead of auditing the whole corpus.
    """

    if FILTER_ENV in os.environ:
        return os.environ[FILTER_ENV]
    for resolver in (_jj_changed_line_filter, _git_changed_line_filter):
        resolved = resolver(repository)
        if resolved is not None:
            return resolved
    return ""
