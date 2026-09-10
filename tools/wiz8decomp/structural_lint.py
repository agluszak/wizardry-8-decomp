"""Cheap structural invariants for recovered source.

Two checks that need no data flow:

- constant byte-offset arithmetic over ``reinterpret_cast<char*>(...)``: a
  complete object should be indexed or have its fields named, not walked by a
  hard-coded byte offset;
- constant indexing past the end of a fixed-size array declared in the
  recovered tree (``unsigned char block[0x100];`` then ``block[0x1ff]``).

The checks are deliberately shallow: they flag only literal constants and
only arrays whose declaration and use spell the same identifier.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")

_BYTE_CAST_OFFSET = re.compile(
    r"reinterpret_cast<\s*(?:const\s+)?(?:unsigned\s+)?char\s*\*>\s*\("
    r"[^;{}]*?\)\s*\+\s*(0[xX][0-9a-fA-F]+|\d+)"
)
_ARRAY_DECLARATION = re.compile(
    r"\b(?:unsigned\s+char|signed\s+char|char|short|unsigned\s+short|int|"
    r"unsigned\s+int|long|unsigned\s+long|float|double|wchar_t)\s+"
    r"([A-Za-z_]\w*)\s*\[\s*(0[xX][0-9a-fA-F]+|\d+)\s*\]"
)
# Only member accesses: a field name can be shadowed by a local, and a local
# array is not the declaration this gate knows about.
_ARRAY_INDEX = re.compile(r"(?:\.|->)\s*([A-Za-z_]\w*)\s*\[\s*(0[xX][0-9a-fA-F]+|\d+)\s*\]")


def _number(text: str) -> int:
    return int(text, 16) if text.lower().startswith("0x") else int(text)


def _scoped(path: Path, repo_dir: Path) -> bool:
    relative = str(path.relative_to(repo_dir))
    return relative.startswith(SCOPE_PREFIXES)


class StructuralGateError(RuntimeError):
    """A hard-coded byte offset or an out-of-bounds constant index."""


def validate_structures(repo_dir: Path) -> dict[str, Any]:
    violations = structural_violations(repo_dir)
    if violations:
        rendered = [
            f"{item['file']}:{item['line']} {item['kind']}: {item['detail']}" for item in violations
        ]
        raise StructuralGateError(
            "recovered source walks constant byte offsets or past array bounds:\n  "
            + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "structural-invariants"}


def structural_violations(repo_dir: Path) -> list[dict[str, Any]]:
    files = [
        path
        for path in list((repo_dir / "src/wiz8").rglob("*.cpp"))
        + list((repo_dir / "include/wiz8").rglob("*.h"))
    ]

    # Fixed-size array fields are declared in headers; collect every declared
    # size per name so an ambiguous name is skipped rather than guessed.
    fields: dict[str, set[int]] = {}
    for path in (repo_dir / "include/wiz8").rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for name, size in _ARRAY_DECLARATION.findall(text):
            fields.setdefault(name, set()).add(_number(size))

    violations: list[dict[str, Any]] = []
    for path in files:
        if not _scoped(path, repo_dir):
            continue
        relative = str(path.relative_to(repo_dir))
        for number, line in enumerate(
            path.read_text(encoding="utf-8", errors="ignore").splitlines(), 1
        ):
            for match in _BYTE_CAST_OFFSET.finditer(line):
                violations.append(
                    {
                        "kind": "constant-byte-offset",
                        "file": relative,
                        "line": number,
                        "detail": match.group(0)[:120],
                    }
                )
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
    return violations
