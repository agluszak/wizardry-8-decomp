"""Exact structural invariants for recovered source.

The gate keeps the one check that a regular expression can decide exactly:
a constant index past the end of a fixed-size array field declared in the
recovered headers.

The earlier constant-byte-offset regex was removed deliberately. It inspected
one source line at a time, recognized only ``reinterpret_cast<char*>(x) + N``,
and therefore could not see the equivalent escapes it claimed to protect
against (a typed pointer plus one, a cast split across lines, a byte field
reached through a member name). A partial regex invites false confidence at the
wrong level.

The replacement invariant is a review criterion rather than a rewrite of that
regex: a cast from a complete recovered object type to a byte pointer is only
allowed at an explicitly designated serialization/ABI boundary, and a byte
pointer must not walk an object by constant offsets outside such a boundary.
New casts must carry their ``reinterpret-ok: reason`` marker (the cast gate
enforces that on the change diff), and substantial new bodies are reviewed
against the typed-object-escape rule before acceptance.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

SCOPE_PREFIXES = ("src/wiz8/", "include/wiz8/")

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
    """A constant index past a fixed-size array field."""


def validate_structures(repo_dir: Path) -> dict[str, Any]:
    violations = structural_violations(repo_dir)
    if violations:
        rendered = [
            f"{item['file']}:{item['line']} {item['kind']}: {item['detail']}" for item in violations
        ]
        raise StructuralGateError(
            "recovered source indexes a fixed-size array past its end:\n  " + "\n  ".join(rendered)
        )
    return {"ok": True, "gate": "structural-invariants"}


def structural_violations(repo_dir: Path) -> list[dict[str, Any]]:
    files = [
        path
        for path in list((repo_dir / "src/wiz8").rglob("*.cpp"))
        + list((repo_dir / "include/wiz8").rglob("*.h"))
        + list((repo_dir / "include/wiz8").rglob("*.hpp"))
    ]

    # Fixed-size array fields are declared in headers; collect every declared
    # size per name so an ambiguous name is skipped rather than guessed.
    fields: dict[str, set[int]] = {}
    for path in (repo_dir / "include/wiz8").rglob("*.h"):
        text = path.read_text(encoding="utf-8", errors="ignore")
        for name, size in _ARRAY_DECLARATION.findall(text):
            fields.setdefault(name, set()).add(_number(size))
    for path in (repo_dir / "include/wiz8").rglob("*.hpp"):
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
