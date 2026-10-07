"""Reject compiler-emission artifacts promoted to authored template source."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .source_text import RECOVERED_ROOTS, mask_cpp_noise, source_files

_EXPLICIT_SPECIALIZATION = re.compile(r"(?m)^[ \t]*template[ \t\r\n]*<[ \t\r\n]*>[ \t\r\n]*")
_EXPLICIT_INSTANTIATION = re.compile(r"(?m)^[ \t]*(?:extern[ \t]+)?template[ \t]+(?![ \t]*<)")


class TemplateModelError(RuntimeError):
    """Recovered source contains an unjustified explicit template construct."""


def _snippet(source: str, offset: int) -> str:
    return " ".join(source[offset : offset + 240].split())[:200]


def _violation(relative: str, source: str, offset: int, kind: str) -> dict[str, Any]:
    return {
        "file": relative,
        "line": source.count("\n", 0, offset) + 1,
        "kind": kind,
        "text": _snippet(source, offset),
    }


def _template_model_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for path in source_files(repository):
        relative = path.relative_to(repository).as_posix()
        source = path.read_text(encoding="utf-8", errors="ignore")
        masked = mask_cpp_noise(source)

        for match in _EXPLICIT_SPECIALIZATION.finditer(masked):
            violations.append(
                _violation(relative, source, match.start(), "explicit-specialization")
            )

        for match in _EXPLICIT_INSTANTIATION.finditer(masked):
            violations.append(_violation(relative, source, match.start(), "explicit-instantiation"))

    return violations


def validate_template_model(repository: Path) -> dict[str, Any]:
    violations = _template_model_violations(repository)
    if violations:
        rendered = "\n  ".join(
            f"{item['file']}:{item['line']}: {item['kind']}: {item['text']}" for item in violations
        )
        raise TemplateModelError(
            "template source-model gate failed: concrete compiler emissions do not justify "
            "explicit specialization/instantiation in recovered source. Move behavior to the "
            "primary template, or update the reviewed gate only when an accepted original-source "
            "oracle directly proves the exceptional construct:\n  " + rendered
        )

    return {
        "ok": True,
        "gate": "template-source-model",
        "roots": list(RECOVERED_ROOTS),
        "exceptions": [],
    }
