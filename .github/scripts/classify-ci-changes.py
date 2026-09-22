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

_SHARED_PREFIXES = (
    ".github/actions/",
    ".github/scripts/",
    ".github/workflows/",
    "cmake/",
    "docker/msvc600/",
    "tools/wiz8decomp/",
)
_SHARED_FILES = {
    "CMakeLists.txt",
    "pyproject.toml",
    "reccmp-project.yml",
    "uv.lock",
}

_WIZ8_PREFIXES = (
    "include/wiz8/",
    "src/wiz8/",
    "src/sgp/",
    "tests/runtime/",
    "tests/licensed/",
    "evidence/reviewed/wiz8/",
)
_SURRENDER_PREFIXES = (
    "src/surrender/",
    "evidence/reviewed/surrender/",
)
_ANALYSIS_PREFIXES = (
    "tests/ghidra/",
    "tests/recovery/",
    "tools/ghidra-scripts/",
    "tools/recovery-fixture/",
    "evidence/seeds/",
    "evidence/snapshots/",
)


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
    wiz8 = False
    surrender = False

    for path in normalized:
        docs_only = path in _DOC_FILES or _matches(path, _DOC_PREFIXES) or path.endswith(".md")
        if not docs_only:
            public = True

        shared = path in _SHARED_FILES or _matches(path, _SHARED_PREFIXES)
        config_shared = path.startswith("config/") and not path.startswith("config/runtime/")
        surrender_header = path.startswith("include/surrender/")

        if shared or config_shared:
            analysis = True
            wiz8 = True
            surrender = True
            continue

        if surrender_header:
            # Provider headers are also a compile-time dependency of Wiz8.
            wiz8 = True
            surrender = True
            continue

        if (
            path == "include/bink.h"
            or path.startswith("config/runtime/")
            or _matches(path, _WIZ8_PREFIXES)
        ):
            wiz8 = True

        if _matches(path, _SURRENDER_PREFIXES):
            surrender = True

        if (
            _matches(path, _ANALYSIS_PREFIXES)
            or path.startswith(("evidence/reviewed/", "tools/ghidra/"))
        ):
            analysis = True

    return {
        "public": public,
        "analysis": analysis,
        "wiz8": wiz8,
        "surrender": surrender,
    }


def main() -> int:
    result = classify(sys.stdin)
    for name, enabled in result.items():
        print(f"{name}={'true' if enabled else 'false'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
