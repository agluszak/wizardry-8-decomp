"""Focused read-only Ghidra queries used by host-side reports."""

from __future__ import annotations

import json
from typing import Any

from ..config import Settings
from .recovery import _program_name, run_ghidra_script


def run_audit(
    settings: Settings,
    audit: str,
    arguments: list[str] | None = None,
    *,
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    """Run one focused Java audit and return its transient result."""

    program_name = _program_name(settings, program_selector)
    result = run_ghidra_script(
        settings,
        "Wiz8Audit.java",
        ["--audit", audit, *(arguments or [])],
        program_name=program_name,
    )
    try:
        return json.loads(result.read_text(encoding="utf-8"))
    finally:
        result.unlink(missing_ok=True)


def function_inventory(settings: Settings) -> list[dict[str, str]]:
    return list(run_audit(settings, "function-inventory")["functions"])


def class_fields(settings: Settings, names: set[str]) -> list[dict[str, Any]]:
    arguments = [value for name in sorted(names, key=str.casefold) for value in ("--class", name)]
    return list(run_audit(settings, "class-fields", arguments)["classes"])


def validate_function_entries(settings: Settings, entries: set[int]) -> dict[str, Any]:
    arguments = [value for entry in sorted(entries) for value in ("--entry", f"0x{entry:08x}")]
    return run_audit(settings, "function-exists", arguments)


def function_facts(settings: Settings, entries: set[int]) -> list[dict[str, Any]]:
    arguments = [value for entry in sorted(entries) for value in ("--entry", f"0x{entry:08x}")]
    return list(run_audit(settings, "function-facts", arguments)["functions"])


def class_facts(settings: Settings, names: set[str]) -> dict[str, Any]:
    arguments = [value for name in sorted(names, key=str.casefold) for value in ("--class", name)]
    return run_audit(settings, "class-facts", arguments)


def data_facts(settings: Settings, entries: set[int]) -> list[dict[str, Any]]:
    arguments = [value for entry in sorted(entries) for value in ("--entry", f"0x{entry:08x}")]
    return list(run_audit(settings, "data-facts", arguments)["data"])
