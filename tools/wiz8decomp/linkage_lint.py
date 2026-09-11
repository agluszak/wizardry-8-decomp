"""Ban first-party ``extern "C"`` outside the SGP bridge and marked boundaries.

Wizardry C++ code has C++ linkage by default. A fixed original address, a free
function, an unmangled-looking name, or how Ghidra spells a symbol are not C
boundaries. The only allowed homes are the designated bridge header and a line
that names the actual C consumer::

    extern "C" {  // C-LINKAGE: src/sgp/sgp.c calls MoveTimer

This gate is intentionally a line scan; it is a linkage reminder, not a
recovery subsystem.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

BRIDGE_HEADER = "include/wiz8/sgp_bridge.h"
SCOPE_DIRECTORIES = ("include/wiz8", "src/wiz8")
_EXTENSIONS = ("*.h", "*.hpp", "*.c", "*.cpp")
_EXTERN_C = re.compile(r'extern\s+"C"')
_MARKER = re.compile(r"C-LINKAGE:")
_LINE_COMMENT = re.compile(r"//.*")


class CLinkageGateError(RuntimeError):
    """A first-party extern "C" has no evidenced C boundary."""


def c_linkage_violations(repo_dir: Path) -> list[dict[str, Any]]:
    violations = []
    for directory in SCOPE_DIRECTORIES:
        root = repo_dir / directory
        if not root.is_dir():
            continue
        for pattern in _EXTENSIONS:
            for path in sorted(root.rglob(pattern)):
                relative = path.relative_to(repo_dir).as_posix()
                if relative == BRIDGE_HEADER:
                    continue
                for lineno, line in enumerate(
                    path.read_text(encoding="utf-8", errors="ignore").splitlines(), 1
                ):
                    if _EXTERN_C.search(_LINE_COMMENT.sub("", line)) and not _MARKER.search(line):
                        violations.append({"file": relative, "line": lineno})
    return violations


def validate_c_linkage(repo_dir: Path) -> dict[str, Any]:
    violations = c_linkage_violations(repo_dir)
    if violations:
        rendered = [f"{item['file']}:{item['line']}" for item in violations]
        raise CLinkageGateError(
            'extern "C" outside the SGP bridge or a C-LINKAGE marker:\n  ' + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "c-linkage"}
