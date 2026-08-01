"""Recovery-compiler regression harness.

``wiz8 recover regress`` measures how much of a recovered function the
Ghidra exporter can regenerate with zero manual edits: for each selected
address it exports the function, splices the emitted block over the existing
recovered body in its owning translation unit, builds the product, runs the
relocation-masked reccmp comparison for that address, and always restores
the file afterwards. Nothing is written permanently; the deliverable is the
per-function report.

The harness needs the live Ghidra project and the pinned VC6 toolchain, so it
is a manual/milestone gate, not part of ``just check`` or ``just test``.
"""

from __future__ import annotations

import re
from typing import Any

from .config import Settings

MARKER_LINE = re.compile(r"^// (?:FUNCTION|SYNTHETIC): WIZ8 0x(?P<address>[0-9a-f]{8})$")


def split_export_blocks(text: str) -> dict[int, str]:
    """Split exporter output into per-address blocks keyed by entry address.

    A block runs from its marker line to the line before the next marker.
    Trailing blank lines are not part of the block.
    """

    blocks: dict[int, str] = {}
    current: int | None = None
    lines: list[str] = []

    def flush() -> None:
        if current is None:
            return
        while lines and not lines[-1].strip():
            lines.pop()
        blocks[current] = "\n".join(lines) + "\n"

    for line in text.splitlines():
        match = MARKER_LINE.match(line)
        if match:
            flush()
            current = int(match.group("address"), 16)
            lines = [line]
        elif current is not None:
            lines.append(line)
    flush()
    return blocks


def marker_span(marker: dict[str, Any]) -> tuple[str, int, int] | None:
    """The (source_file, first_line, last_line) 1-based inclusive span the
    exported block replaces: the marker line through the declaration's end."""

    declaration = marker.get("declaration") or {}
    end_line = declaration.get("end_line")
    line = marker.get("line")
    if not isinstance(line, int) or not isinstance(end_line, int) or end_line < line:
        return None
    return str(marker["source_file"]), line - 1, end_line


def splice_lines(original: str, first_line: int, last_line: int, block: str) -> str:
    """Replace the 1-based inclusive line span with the block text."""

    lines = original.splitlines(keepends=True)
    if not 1 <= first_line <= last_line <= len(lines):
        raise ValueError(f"splice span {first_line}..{last_line} outside file")
    if not block.endswith("\n"):
        block += "\n"
    return "".join(lines[: first_line - 1]) + block + "".join(lines[last_line:])


def compile_diagnostics(build_output: str, limit: int = 20) -> list[str]:
    """The VC6 diagnostics inside a failed build's output, deduplicated.

    The raw failure text is dominated by jom's unwinding; the compiler's own
    `error C…` lines are the evidence worth reporting.
    """

    seen: dict[str, None] = {}
    for line in build_output.splitlines():
        if "error C" in line or "fatal error" in line:
            seen.setdefault(line.strip()[-300:], None)
    if not seen:
        return [build_output[-1000:]]
    return list(seen)[:limit]


def _failed_build_output(settings: Settings, fallback: str) -> str:
    """The failed product build's full output from its result log."""

    import json

    log = settings.repo_dir / "build" / "logs" / "product-build.json"
    try:
        record = json.loads(log.read_text(encoding="utf-8"))
        return f"{record.get('stdout', '')}\n{record.get('stderr', '')}"
    except (OSError, json.JSONDecodeError):
        return fallback


def regress(
    settings: Settings,
    selections: list[str],
    *,
    target: str = "WIZ8",
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    from .build import build_target
    from .ghidra.export_cpp import export_cpp
    from .reccmp_workflows import compare_selected, parse_address, triage_selected
    from .source_model import load_source_index

    addresses = [parse_address(selection) for selection in selections]
    if not addresses:
        raise ValueError("pass one or more function addresses")

    markers = {
        marker["address"]: marker
        for marker in load_source_index(settings.repo_dir)["markers"]
        if marker["marker_kind"] == "FUNCTION"
    }

    exported = export_cpp(
        settings, [f"0x{a:08x}" for a in addresses], program_selector=program_selector
    )
    blocks = split_export_blocks(exported["text"])

    rows: list[dict[str, Any]] = []
    for address in addresses:
        row: dict[str, Any] = {"address": f"0x{address:08x}"}
        rows.append(row)
        block = blocks.get(address)
        if block is None:
            row["status"] = "not-exported"
            continue
        marker = markers.get(address)
        span = marker_span(marker) if marker is not None else None
        if span is None:
            row["status"] = "unplaced"
            continue
        source_file, first_line, last_line = span
        row["source_file"] = source_file
        path = settings.repo_dir / source_file
        original = path.read_text(encoding="utf-8")
        try:
            path.write_text(splice_lines(original, first_line, last_line, block), encoding="utf-8")
            try:
                build_target(settings, target)
            except RuntimeError as error:
                row["status"] = "compile-failed"
                row["diagnostics"] = compile_diagnostics(_failed_build_output(settings, str(error)))
                continue
            comparison = compare_selected(settings.repo_dir, target, [address])
            entity = comparison["functions"][0]
            row["status"] = entity["status"]
            row["raw_matching"] = entity.get("raw_matching")
            if entity["status"] not in {"exact", "effective"}:
                triage = triage_selected(settings.repo_dir, target, [address])
                findings = triage.get("functions") or []
                if findings:
                    row["first_divergence"] = findings[0]
        finally:
            path.write_text(original, encoding="utf-8")

    summary = {
        "selected": len(rows),
        "exact": sum(row.get("status") == "exact" for row in rows),
        "effective": sum(row.get("status") == "effective" for row in rows),
        "compile_failed": sum(row.get("status") == "compile-failed" for row in rows),
        "functions": rows,
        "note": "sources restored; build products still reflect the last splice",
    }
    return summary
