"""Ban first-party ``extern "C"`` outside marked ABI boundaries."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .source_text import mask_cpp_noise, source_files

SCOPE_DIRECTORIES = ("include/wiz8", "src/wiz8")
_EXTERN_C = re.compile(r'extern\s+"C"')
_MARKER = re.compile(r"C-LINKAGE:")


class CLinkageGateError(RuntimeError):
    """A first-party extern "C" has no evidenced C boundary."""


def c_linkage_violations(repo_dir: Path) -> list[dict[str, Any]]:
    violations = []
    for path in source_files(repo_dir, SCOPE_DIRECTORIES):
        relative = path.relative_to(repo_dir).as_posix()
        source = path.read_text(encoding="utf-8", errors="ignore")
        code = mask_cpp_noise(source, strings=False).splitlines()
        masked = mask_cpp_noise(source).splitlines()
        for lineno, (line, original) in enumerate(zip(code, source.splitlines()), 1):
            if any(
                masked[lineno - 1][m.start() : m.start() + 6] == "extern"
                for m in _EXTERN_C.finditer(line)
            ) and not _MARKER.search(original):
                violations.append({"file": relative, "line": lineno})
    return violations


def validate_c_linkage(repo_dir: Path) -> dict[str, Any]:
    violations = c_linkage_violations(repo_dir)
    if violations:
        rendered = [f"{item['file']}:{item['line']}" for item in violations]
        raise CLinkageGateError(
            'extern "C" without a C-LINKAGE marker:\n  ' + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "c-linkage"}
