"""Small lexical helpers shared by source-text checks, not a C++ declaration parser."""

from __future__ import annotations

import re
from pathlib import Path

RECOVERED_ROOTS = ("src/wiz8", "include/wiz8", "src/surrender", "include/surrender")
CPP_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx", ".inl"})
# Match literals before interpreting comment delimiters inside them. Preserve
# offsets and newlines so diagnostics still refer to the original source.
CPP_NOISE = re.compile(
    r'R"(?P<delimiter>[^\s()\\]{0,16})\(.*?\)(?P=delimiter)"'
    r'|//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)


def mask_cpp_noise(source: str, *, strings: bool = True) -> str:
    def mask(match: re.Match[str]) -> str:
        text = match.group()
        if not strings and not text.startswith(("//", "/*")):
            return text
        return "".join("\n" if char == "\n" else " " for char in text)

    return CPP_NOISE.sub(mask, source)


def source_files(repository: Path, roots: tuple[str, ...] = RECOVERED_ROOTS) -> list[Path]:
    return sorted(
        {
            path
            for root in roots
            for path in (repository / root).rglob("*")
            if path.is_file() and path.suffix.casefold() in CPP_SUFFIXES
        }
    )
