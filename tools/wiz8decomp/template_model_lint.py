"""Reject compiler-emission artifacts promoted to authored template source.

A concrete retail template body proves an instantiation happened. It does not
prove that the original source contained an explicit specialization or explicit
instantiation. Those constructs are therefore forbidden in recovered
Wizardry/SurRender source unless this gate itself carries a narrow,
source-oracle-backed exception.

There is intentionally no source comment waiver. Adding an exception changes
the reviewed source-model policy instead of letting a local matching tweak
silently redefine the authored template.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

_SOURCE_ROOTS = ("src/wiz8", "include/wiz8", "src/surrender", "include/surrender")
_CPP_SUFFIXES = {".cc", ".cpp", ".cxx", ".h", ".hpp"}

_NOISE = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)
_EXPLICIT_SPECIALIZATION = re.compile(
    r"(?m)^[ \t]*template[ \t\r\n]*<[ \t\r\n]*>[ \t\r\n]*"
)
_EXPLICIT_INSTANTIATION = re.compile(
    r"(?m)^[ \t]*(?:extern[ \t]+)?template[ \t]+(?![ \t]*<)"
)
_SR_INSTANCE_LIFECYCLE = re.compile(
    r"template[ \t\r\n]*<[ \t\r\n]*>[ \t\r\n]*"
    r"struct[ \t\r\n]+srInstanceLifecycle[ \t\r\n]*"
    r"<[ \t\r\n]*false[ \t\r\n]*>"
)


class TemplateModelError(RuntimeError):
    """Recovered source contains an unjustified explicit template construct."""


def _mask_cpp_noise(source: str) -> str:
    def mask(match: re.Match[str]) -> str:
        return "".join("\n" if char == "\n" else " " for char in match.group())

    return _NOISE.sub(mask, source)


def _snippet(source: str, offset: int) -> str:
    return " ".join(source[offset : offset + 240].split())[:200]


def _violation(relative: str, source: str, offset: int, kind: str) -> dict[str, Any]:
    return {
        "file": relative,
        "line": source.count("\n", 0, offset) + 1,
        "kind": kind,
        "text": _snippet(source, offset),
    }


def _allowed_specialization(relative: str, masked: str, offset: int) -> bool:
    if relative != "include/surrender/srTypeRegistry.h":
        return False
    return _SR_INSTANCE_LIFECYCLE.match(masked, offset) is not None


def _template_model_violations(repository: Path) -> list[dict[str, Any]]:
    violations: list[dict[str, Any]] = []
    for root_name in _SOURCE_ROOTS:
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix.lower() not in _CPP_SUFFIXES:
                continue
            relative = path.relative_to(repository).as_posix()
            source = path.read_text(encoding="utf-8", errors="ignore")
            masked = _mask_cpp_noise(source)

            for match in _EXPLICIT_SPECIALIZATION.finditer(masked):
                if _allowed_specialization(relative, masked, match.start()):
                    continue
                violations.append(
                    _violation(relative, source, match.start(), "explicit-specialization")
                )

            for match in _EXPLICIT_INSTANTIATION.finditer(masked):
                violations.append(
                    _violation(relative, source, match.start(), "explicit-instantiation")
                )

    return violations


def validate_template_model(repository: Path) -> dict[str, Any]:
    violations = _template_model_violations(repository)
    if violations:
        rendered = "\n  ".join(
            f"{item['file']}:{item['line']}: {item['kind']}: {item['text']}"
            for item in violations
        )
        raise TemplateModelError(
            "template source-model gate failed: concrete compiler emissions do not justify "
            "explicit specialization/instantiation in recovered source. Move behavior to the "
            "primary template, or update the reviewed gate only when an accepted original-source "
            "oracle directly proves the exceptional construct:\n  "
            + rendered
        )

    return {
        "ok": True,
        "gate": "template-source-model",
        "roots": list(_SOURCE_ROOTS),
        "exceptions": ["include/surrender/srTypeRegistry.h: srInstanceLifecycle<false>"],
    }
