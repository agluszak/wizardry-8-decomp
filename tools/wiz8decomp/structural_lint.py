"""Exact structural invariants for recovered source."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .cast_lint import added_lines_without_marker, baseline_diff
from .global_model import global_violations
from .source_text import mask_cpp_noise, source_files

SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")

_ARRAY_DECLARATION = re.compile(
    r"\b(?:unsigned\s+char|signed\s+char|char|short|unsigned\s+short|int|"
    r"unsigned\s+int|long|unsigned\s+long|float|double|wchar_t)\s+"
    r"([A-Za-z_]\w*)\s*\[\s*(0[xX][0-9a-fA-F]+|\d+)\s*\]"
)
# Only member accesses: a field name can be shadowed by a local, and a local
# array is not the declaration this gate knows about.
_ARRAY_INDEX = re.compile(r"(?:\.|->)\s*([A-Za-z_]\w*)\s*\[\s*(0[xX][0-9a-fA-F]+|\d+)\s*\]")
_VOID_ELEMENT = re.compile(r"W8GrowableVector\s*<\s*void\s*\*>")
_VOID_MARKER = re.compile(r"vector-void-ok:\s*\S", re.IGNORECASE)
_IFNDEF = re.compile(r"^\s*#ifndef\s+(\w+)\s*$")


def _number(text: str) -> int:
    return int(text, 16) if text.lower().startswith("0x") else int(text)


def _include_guard_violations(path: Path, relative: str) -> list[dict[str, Any]]:
    """Flag ifndef-guarded Wizardry headers that leak declarations past #endif."""
    if not relative.startswith("include/wiz8/") or path.suffix.lower() not in {".h", ".hpp"}:
        return []
    lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    guard: str | None = None
    includes_before: list[int] = []
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("#pragma once"):
            return []
        if stripped.startswith("#include") and guard is None:
            includes_before.append(index + 1)
            continue
        match = _IFNDEF.match(stripped)
        if match and guard is None:
            name = match.group(1)
            if name.endswith(("_H", "_HPP")):
                guard = name
    if guard is None:
        return []
    violations: list[dict[str, Any]] = []
    for number in includes_before:
        violations.append(
            {
                "kind": "include-before-guard",
                "file": relative,
                "line": number,
                "detail": f"{guard} include appears before the include guard",
            }
        )
    last_code = 0
    last_text = ""
    for index, line in enumerate(lines):
        if line.strip():
            last_code = index + 1
            last_text = line.strip()
    if last_text and not last_text.startswith("#endif"):
        violations.append(
            {
                "kind": "include-guard-closed-early",
                "file": relative,
                "line": last_code,
                "detail": f"{guard} has declarations after its include guard",
            }
        )
    return violations


def _scoped(path: Path, repo_dir: Path) -> bool:
    relative = str(path.relative_to(repo_dir))
    return relative.startswith(SCOPE_PREFIXES)


class StructuralGateError(RuntimeError):
    """A recovered source-model invariant failed."""


def validate_structures(repo_dir: Path) -> dict[str, Any]:
    violations = structural_violations(repo_dir)
    if violations:
        rendered = [
            f"{item['file']}:{item['line']} {item['kind']}: {item['detail']}" for item in violations
        ]
        raise StructuralGateError(
            "recovered source breaks a structural invariant:\n  " + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "structural-invariants"}


def structural_violations(repo_dir: Path) -> list[dict[str, Any]]:
    files = source_files(repo_dir, ("src/wiz8", "include/wiz8"))
    # Fixed-size member arrays: ambiguous names are deliberately not guessed.
    fields: dict[str, set[int]] = {}
    for path in source_files(repo_dir, ("include/wiz8",)):
        text = mask_cpp_noise(path.read_text(encoding="utf-8", errors="ignore"))
        for name, size in _ARRAY_DECLARATION.findall(text):
            fields.setdefault(name, set()).add(_number(size))

    violations: list[dict[str, Any]] = []
    for path in files:
        if not _scoped(path, repo_dir):
            continue
        relative = str(path.relative_to(repo_dir))
        for number, line in enumerate(
            mask_cpp_noise(path.read_text(encoding="utf-8", errors="ignore")).splitlines(), 1
        ):
            for match in _ARRAY_INDEX.finditer(line):
                name, index_text = match.group(1), match.group(2)
                sizes = fields.get(name)
                if not sizes:
                    continue
                index = _number(index_text)
                if all(index >= size for size in sizes):
                    violations.append(
                        {
                            "kind": "array-index-out-of-bounds",
                            "file": relative,
                            "line": number,
                            "detail": (
                                f"{name}[{index_text}] but {name} is {sorted(sizes)} elements"
                            ),
                        }
                    )
        violations.extend(_include_guard_violations(path, relative))
    for item in added_lines_without_marker(baseline_diff(repo_dir)[1], _VOID_ELEMENT, _VOID_MARKER):
        violations.append(
            {
                "kind": "unresolved-void-vector",
                "file": item["file"],
                "line": item["line"],
                "detail": (
                    "new W8GrowableVector<void*> needs a 'vector-void-ok: reason' "
                    f"comment: {item['text']}"
                ),
            }
        )
    violations.extend(global_violations(repo_dir))
    return violations
