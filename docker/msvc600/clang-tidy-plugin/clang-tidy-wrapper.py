#!/usr/bin/env python3
"""Load Wizardry's clang-tidy plugin and restrict debt checks to added lines."""

from __future__ import annotations

import os
import re
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

REAL_CLANG_TIDY = "/usr/bin/clang-tidy-19"
PLUGIN = "/usr/local/lib/wiz8-clang-tidy.so"
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


def _git(repository: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", "-C", str(repository), *args],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )


def _baseline(repository: Path) -> str | None:
    for candidate in ("origin/main", "origin/master", "main", "master"):
        probe = _git(repository, "rev-parse", "--verify", "--quiet", f"{candidate}^{{commit}}")
        if probe.returncode != 0 or not probe.stdout.strip():
            continue
        merge = _git(repository, "merge-base", candidate, "HEAD")
        if merge.returncode == 0 and merge.stdout.strip():
            return merge.stdout.strip()
        return probe.stdout.strip()
    return None


def _target_path(header: str) -> str | None:
    target = header[4:].strip()
    if target == "/dev/null":
        return None
    if target.startswith(("a/", "b/")):
        target = target[2:]
    if not target.startswith(SCOPES) or not target.lower().endswith(CPP_SUFFIXES):
        return None
    return target


def _added_line_filter(diff: str) -> str:
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


def _changed_line_filter(repository: Path) -> str:
    base = _baseline(repository)
    if base is None:
        return ""
    diff = _git(
        repository,
        "diff",
        "--no-color",
        "--no-ext-diff",
        "--unified=0",
        base,
        "--",
        *SCOPES,
    )
    if diff.returncode != 0:
        return ""
    return _added_line_filter(diff.stdout)


def _self_test() -> None:
    diff = """diff --git a/src/wiz8/example.cpp b/src/wiz8/example.cpp
--- a/src/wiz8/example.cpp
+++ b/src/wiz8/example.cpp
@@ -1,2 +1,5 @@
 old();
+first();
+moved();
+second();
 keep();
diff --git a/src/wiz8/old.cpp b/src/wiz8/old.cpp
--- a/src/wiz8/old.cpp
+++ b/src/wiz8/old.cpp
@@ -4 +3,0 @@
-moved();
diff --git a/include/surrender/example.h b/include/surrender/example.h
--- a/include/surrender/example.h
+++ b/include/surrender/example.h
@@ -0,0 +1 @@
+header_change();
"""
    actual = _added_line_filter(diff)
    expected = "include/surrender/example.h@1;src/wiz8/example.cpp@2,4"
    if actual != expected:
        raise SystemExit(f"changed-line filter self-test failed: {actual!r} != {expected!r}")


def main() -> None:
    if sys.argv[1:] == ["--wiz8-wrapper-self-test"]:
        _self_test()
        return

    if FILTER_ENV not in os.environ:
        repository = Path("/repo")
        # Prefer an explicit empty filter when VCS metadata is unavailable so the
        # plugin does not fall back to a silent no-op without the host having
        # decided the scope. Host-side `wiz8 lint` normally injects the filter.
        if (repository / ".git").exists():
            os.environ[FILTER_ENV] = _changed_line_filter(repository)
        else:
            os.environ[FILTER_ENV] = ""

    arguments = list(sys.argv[1:])
    if os.environ.get(FILTER_ENV) == "*":
        # Full-corpus audits intentionally produce more than Clang's default
        # diagnostic error limit. Do not truncate the cleanup inventory.
        arguments.insert(0, "--extra-arg=-ferror-limit=0")

    os.execv(REAL_CLANG_TIDY, [REAL_CLANG_TIDY, f"--load={PLUGIN}", *arguments])


if __name__ == "__main__":
    main()
