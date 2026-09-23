#!/usr/bin/env python3
"""Classify changed paths into independent CI lanes."""

from __future__ import annotations

import sys
from collections.abc import Iterable

_DOC_PREFIXES = ("docs/",)
_DOC_FILES = {
    "README.md",
    "AGENTS.md",
    "CONTRIBUTING.md",
    "LICENSE",
}

_ALL_HEAVY_PREFIXES = (
    ".github/actions/",
    ".github/scripts/",
    ".github/workflows/",
    "cmake/",
    "docker/msvc600/",
    "tools/wiz8decomp/extract/",
    "tools/wiz8decomp/inputs/",
)
_ALL_HEAVY_FILES = {
    "CMakeLists.txt",
    "pyproject.toml",
    "uv.lock",
    "config/static-libraries.yml",
    "tools/wiz8decomp/build.py",
    "tools/wiz8decomp/config.py",
    "tools/wiz8decomp/paths.py",
    "tools/wiz8decomp/subprocesses.py",
    "tools/wiz8decomp/commands/core.py",
    "tools/wiz8decomp/ghidra/fid_seeds.py",
}

_WIZ8_SOURCE_PREFIXES = (
    "include/wiz8/",
    "src/wiz8/",
    "src/sgp/",
)
_WIZ8_RUNTIME_PREFIXES = (
    "config/runtime/",
    "tests/runtime/",
    "tools/wiz8decomp/debug/",
)
_WIZ8_RUNTIME_FILES = {
    "tools/wiz8decomp/runtime.py",
    "tools/wiz8decomp/runtime_stubs.py",
}
_WIZ8_COMPARISON_PREFIXES = (
    "config/reccmp/",
    "tests/licensed/",
)
_WIZ8_COMPARISON_FILES = {
    "reccmp-project.yml",
    "tools/wiz8decomp/comparison.py",
    "tools/wiz8decomp/reccmp_data.py",
    "tools/wiz8decomp/reports/status.py",
}
_SURRENDER_PREFIXES = (
    "src/surrender/",
    "evidence/reviewed/surrender/",
)
_ANALYSIS_PREFIXES = (
    "tests/ghidra/",
    "tests/recovery/",
    "tools/ghidra-scripts/",
    "tools/recovery-fixture/",
    "tools/wiz8decomp/ghidra/",
    "evidence/seeds/",
    "evidence/snapshots/",
)
_SOURCE_INDEX_FILES = {
    "tools/wiz8decomp/source_index.py",
    "tools/wiz8decomp/clang_tidy_lines.py",
}


def _matches(path: str, prefixes: tuple[str, ...]) -> bool:
    return path.startswith(prefixes)


def classify(paths: Iterable[str]) -> dict[str, bool]:
    normalized = set()
    for raw in paths:
        path = raw.strip()
        while path.startswith("./"):
            path = path[2:]
        if path:
            normalized.add(path)

    public = False
    analysis = False
    wiz8_compare = False
    wiz8_runtime = False
    surrender = False

    for path in normalized:
        docs_only = path in _DOC_FILES or _matches(path, _DOC_PREFIXES) or path.endswith(".md")
        if not docs_only:
            public = True

        if path in _ALL_HEAVY_FILES or _matches(path, _ALL_HEAVY_PREFIXES):
            analysis = True
            wiz8_compare = True
            wiz8_runtime = True
            surrender = True
            continue

        if path in _SOURCE_INDEX_FILES:
            analysis = True
            wiz8_runtime = True

        if path.startswith("include/surrender/"):
            # Provider headers are compile-time inputs to both the provider and Wiz8.
            surrender = True
            wiz8_compare = True
            wiz8_runtime = True
            continue

        if _matches(path, _WIZ8_SOURCE_PREFIXES) or path == "include/bink.h":
            wiz8_compare = True
            wiz8_runtime = True

        if path in _WIZ8_RUNTIME_FILES or _matches(path, _WIZ8_RUNTIME_PREFIXES):
            wiz8_runtime = True

        if path in _WIZ8_COMPARISON_FILES or _matches(path, _WIZ8_COMPARISON_PREFIXES):
            wiz8_compare = True
            if path == "reccmp-project.yml" or path == "tools/wiz8decomp/reports/status.py":
                surrender = True

        if path.startswith("evidence/reviewed/wiz8/"):
            wiz8_compare = True
            analysis = True

        if _matches(path, _SURRENDER_PREFIXES):
            surrender = True
            if path.startswith("evidence/reviewed/"):
                analysis = True

        if _matches(path, _ANALYSIS_PREFIXES):
            analysis = True

    return {
        "public": public,
        "analysis": analysis,
        "wiz8_compare": wiz8_compare,
        "wiz8_runtime": wiz8_runtime,
        "surrender": surrender,
    }


def main() -> int:
    result = classify(sys.stdin)
    for name, enabled in result.items():
        print(f"{name}={'true' if enabled else 'false'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
