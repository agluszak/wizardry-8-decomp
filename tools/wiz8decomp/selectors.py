"""Shared source-owned function selector resolution for recovery commands."""

from __future__ import annotations

from collections.abc import Iterable
from pathlib import Path

from .source_model import build_source_model


def _numeric_range(value: str) -> tuple[int, int] | None:
    start_text, separator, end_text = value.strip().partition(":")
    try:
        start = int(start_text, 0)
        end = int(end_text, 0) if separator else start
    except ValueError:
        return None
    if start < 0 or end < start:
        raise ValueError(f"invalid function selector range: {value}")
    return start, end


def resolve_function_selectors(repository: Path, values: Iterable[str]) -> list[int]:
    """Resolve addresses, inclusive ranges, and exact source-owned identities."""

    model = build_source_model(repository)
    selected: set[int] = set()
    by_name: dict[str, list[int]] = {}
    for address, function in model.functions.items():
        by_name.setdefault(function.name, []).append(address)

    for value in values:
        numeric = _numeric_range(value)
        if numeric is not None:
            start, end = numeric
            if start == end:
                selected.add(start)
            else:
                matches = [address for address in model.functions if start <= address <= end]
                if not matches:
                    raise ValueError(f"no source-owned functions in selector range {value}")
                selected.update(matches)
            continue
        matches = by_name.get(value, [])
        if not matches:
            # Ghidra's stable default names encode the reviewed entry directly.
            folded = value.casefold()
            for prefix in ("function", "fun_"):
                if folded.startswith(prefix):
                    suffix = value[len(prefix) :]
                    try:
                        selected.add(int(suffix, 16))
                        break
                    except ValueError:
                        pass
            else:
                raise ValueError(f"unknown function selector: {value}")
            continue
        if len(matches) > 1:
            candidates = ", ".join(f"0x{address:08x}" for address in matches[:8])
            raise ValueError(f"ambiguous function selector {value!r}; candidates: {candidates}")
        selected.add(matches[0])
    if not selected:
        raise ValueError("pass one or more function selectors")
    return sorted(selected)


def recovery_selections(repository: Path, values: Iterable[str]) -> list[str]:
    """Preserve numeric ranges for Ghidra and resolve names to entry addresses."""

    result: list[str] = []
    for value in values:
        numeric = _numeric_range(value)
        if numeric is not None:
            start, end = numeric
            result.append(f"0x{start:08x}" if start == end else f"0x{start:08x}:0x{end:08x}")
        else:
            result.extend(
                f"0x{address:08x}" for address in resolve_function_selectors(repository, [value])
            )
    if not result:
        raise ValueError("pass one or more function selectors")
    return result
