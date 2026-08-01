"""Source-shape rules for compiler-owned C++ lifecycle artifacts."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".h", ".hpp"})
DELETING_DESTRUCTOR_DECORATED = (
    "`scalar deleting destructor'",
    "`vector deleting destructor'",
)
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


class SourcePolicyError(RuntimeError):
    """First-party source manually models a compiler-owned artifact."""


def _without_comments_and_literals(source: str) -> str:
    """Replace comments and literals with spaces while preserving line numbers."""

    output = list(source)
    index = 0
    state = "code"
    quote = ""
    while index < len(source):
        char = source[index]
        following = source[index + 1] if index + 1 < len(source) else ""
        if state == "code":
            if char == "/" and following == "/":
                output[index] = output[index + 1] = " "
                index += 2
                state = "line-comment"
                continue
            if char == "/" and following == "*":
                output[index] = output[index + 1] = " "
                index += 2
                state = "block-comment"
                continue
            if char in {'"', "'"}:
                output[index] = " "
                quote = char
                state = "literal"
        elif state == "line-comment":
            if char == "\n":
                state = "code"
            else:
                output[index] = " "
        elif state == "block-comment":
            if char == "*" and following == "/":
                output[index] = output[index + 1] = " "
                index += 2
                state = "code"
                continue
            if char != "\n":
                output[index] = " "
        else:
            if char == "\\" and following:
                output[index] = " "
                if following != "\n":
                    output[index + 1] = " "
                index += 2
                continue
            if char == quote:
                state = "code"
            if char != "\n":
                output[index] = " "
        index += 1
    return "".join(output)


def _source_files(repository: Path) -> list[Path]:
    return sorted(
        path
        for root_name in ("include", "src")
        if (root := repository / root_name).is_dir()
        for path in root.rglob("*")
        if path.is_file() and path.suffix.casefold() in SOURCE_SUFFIXES
    )


def validate_source_policy(repository: Path) -> dict[str, Any]:
    problems: list[str] = []
    sources = _source_files(repository)
    for path in sources:
        relative = path.relative_to(repository).as_posix()
        source = path.read_text(encoding="utf-8")
        code = _without_comments_and_literals(source)
        for match in IDENTIFIER.finditer(code):
            identifier = match.group(0)
            normalized = identifier.replace("_", "").casefold()
            line = code.count("\n", 0, match.start()) + 1
            if normalized == "installvtable":
                problems.append(
                    f"{relative}:{line}: InstallVtable methods manually model compiler vptr writes"
                )
            elif normalized.endswith(("scalardeletingdestructor", "vectordeletingdestructor")):
                problems.append(
                    f"{relative}:{line}: deleting destructors must be compiler-generated"
                )
            elif identifier.casefold().startswith("g_vtable_"):
                problems.append(
                    f"{relative}:{line}: raw vtable globals must not model C++ object lifecycle"
                )

        lines = source.splitlines()
        for index, line in enumerate(lines):
            if not any(spelling in line for spelling in DELETING_DESTRUCTOR_DECORATED):
                continue
            previous = lines[index - 1].strip() if index else ""
            if not previous.startswith("// SYNTHETIC:"):
                problems.append(
                    f"{relative}:{index + 1}: deleting-wrapper evidence requires "
                    "an immediately preceding // SYNTHETIC: marker"
                )

    if problems:
        raise SourcePolicyError("source policy failed:\n  " + "\n  ".join(sorted(problems)))
    return {"ok": True, "source_files": len(sources)}
