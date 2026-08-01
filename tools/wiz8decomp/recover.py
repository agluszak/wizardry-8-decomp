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
from pathlib import Path
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


def block_end_line(marker: dict[str, Any]) -> int:
    """The last 1-based line of a marker's block in its source file.

    A FUNCTION marker owns its declaration span; marker kinds without a
    declaration (SYNTHETIC, TEMPLATE, LIBRARY) own the marker line plus the
    symbol comment that must follow SYNTHETIC/TEMPLATE markers.
    """

    declaration = marker.get("declaration") or {}
    end_line = declaration.get("end_line")
    if isinstance(end_line, int):
        return end_line
    line = int(marker["line"])
    return line + 1 if marker["marker_kind"] in {"SYNTHETIC", "TEMPLATE"} else line


def place_address(markers: list[dict[str, Any]], address: int) -> dict[str, Any]:
    """Choose the owning file and insertion line for a new function.

    The bracketing recovered neighbours decide: when both live in one file,
    the new block belongs after the earlier neighbour's block. Different
    files mean the address sits between translation units, and an address
    outside every recovered range has no proven owner; both refuse rather
    than guess.
    """

    ordered = sorted(markers, key=lambda marker: marker["address"])
    previous = None
    following = None
    for marker in ordered:
        if marker["address"] < address:
            previous = marker
        elif marker["address"] > address:
            following = marker
            break
    if previous is None or following is None:
        return {"status": "unplaced", "reason": "outside every recovered address range"}
    if previous["source_file"] != following["source_file"]:
        return {
            "status": "unplaced",
            "reason": "between translation units "
            f"{previous['source_file']} and {following['source_file']}",
        }
    return {
        "status": "placed",
        "source_file": previous["source_file"],
        "after_line": block_end_line(previous),
    }


def insert_lines(original: str, after_line: int, block: str) -> str:
    """Insert the block after the 1-based line, separated by a blank line."""

    lines = original.splitlines(keepends=True)
    if not 0 <= after_line <= len(lines):
        raise ValueError(f"insertion line {after_line} outside file")
    if not block.endswith("\n"):
        block += "\n"
    return "".join(lines[:after_line]) + "\n" + block + "".join(lines[after_line:])


_INITIALIZER_HEAD = re.compile(r"^\s{4}: ")
_MEMBER_INITIALIZER = re.compile(r"^([a-z_][A-Za-z0-9_]*)\((.*)\)$")


def constructor_store_alternative(block: str) -> str | None:
    """The bounded initializer-vs-body-store alternative for a constructor.

    Member initializers the exporter lifted move back into leading body
    assignments; base initializers cannot move and stay. Only single-argument
    member initializers convert (a multi-argument one is a real constructor
    call, not an assignment). No convertible member initializer means no
    alternative.
    """

    lines = block.split("\n")
    start = next((i for i, line in enumerate(lines) if _INITIALIZER_HEAD.match(line)), None)
    if start is None:
        return None
    end = start
    while end + 1 < len(lines) and lines[end + 1].startswith("      "):
        end += 1
    text = " ".join(line.strip() for line in lines[start : end + 1])[2:]
    initializers: list[str] = []
    depth = 0
    current = ""
    for char in text:
        if char == "," and depth == 0:
            initializers.append(current.strip())
            current = ""
            continue
        depth += char in "(<"
        depth -= char in ")>"
        current += char
    initializers.append(current.strip())

    kept: list[str] = []
    stores: list[str] = []
    for initializer in initializers:
        match = _MEMBER_INITIALIZER.match(initializer)
        if match is not None and match.group(2) and not _splits_at_top_level(match.group(2)):
            stores.append(f"  {match.group(1)} = {match.group(2)};")
        else:
            kept.append(initializer)
    if not stores:
        return None

    brace = next((i for i in range(end + 1, len(lines)) if lines[i].strip() == "{"), None)
    if brace is None:
        return None
    rebuilt = lines[:start]
    if kept:
        rebuilt.append("    : " + ", ".join(kept))
    rebuilt.extend(lines[end + 1 : brace + 1])
    rebuilt.extend(stores)
    rebuilt.extend(lines[brace + 1 :])
    return "\n".join(rebuilt)


def _splits_at_top_level(text: str) -> bool:
    depth = 0
    for char in text:
        if char == "," and depth == 0:
            return True
        depth += char in "(<"
        depth -= char in ")>"
    return False


_UNDECLARED = re.compile(r"error C2065: '([A-Za-z_][A-Za-z0-9_]*)' : undeclared identifier")


def suggest_includes(repo_dir: Path, diagnostics: list[str]) -> dict[str, list[str]]:
    """Headers declaring the undeclared identifiers a failed build names.

    A deterministic suggestion list for the human (or a later milestone), not
    an automatic edit: the scan finds headers whose own text declares the
    class or struct.
    """

    names = sorted(
        {match.group(1) for line in diagnostics for match in [_UNDECLARED.search(line)] if match}
    )
    if not names:
        return {}
    wanted = set(names)
    found: dict[str, list[str]] = {}
    for header in sorted((repo_dir / "include").rglob("*.h")):
        try:
            text = header.read_text(encoding="utf-8")
        except (OSError, UnicodeDecodeError):
            continue
        for name in wanted:
            if re.search(rf"\b(?:class|struct)\s+{name}\b", text):
                found.setdefault(name, []).append(header.relative_to(repo_dir).as_posix())
    return found


def recover_function(
    settings: Settings,
    selection: str,
    *,
    apply: bool = False,
    target: str = "WIZ8",
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    """Recover one function into its owning translation unit.

    Export the function, place it in address order between its recovered
    neighbours, then build/compare/triage a bounded candidate set and keep
    the best non-regressing shape. The default previews and restores the
    tree; ``--apply`` leaves the chosen candidate in place.
    """

    from .build import build_target
    from .ghidra.export_cpp import export_cpp
    from .reccmp_workflows import compare_selected, parse_address, triage_selected
    from .source_model import load_source_index

    address = parse_address(selection)
    markers = load_source_index(settings.repo_dir)["markers"]
    owned = next((marker for marker in markers if marker["address"] == address), None)
    if owned is not None:
        return {
            "address": f"0x{address:08x}",
            "status": "already-recovered",
            "source_file": owned["source_file"],
            "line": owned["line"],
            "note": "the address already has a marker; use `wiz8 recover regress` "
            "to measure regeneration of a recovered body",
        }

    exported = export_cpp(settings, [f"0x{address:08x}"], program_selector=program_selector)
    block = split_export_blocks(exported["text"]).get(address)
    if block is None:
        return {"address": f"0x{address:08x}", "status": "not-exported"}

    placement = place_address(markers, address)
    if placement["status"] != "placed":
        return {"address": f"0x{address:08x}", **placement, "block": block}

    source_file = placement["source_file"]
    after_line = placement["after_line"]
    path = settings.repo_dir / source_file
    original = path.read_text(encoding="utf-8")

    candidates: list[tuple[str, str]] = [("as-exported", block)]
    alternative = constructor_store_alternative(block)
    if alternative is not None:
        candidates.append(("member-stores", alternative))

    rows: list[dict[str, Any]] = []
    try:
        for name, candidate in candidates:
            row: dict[str, Any] = {"candidate": name}
            rows.append(row)
            path.write_text(insert_lines(original, after_line, candidate), encoding="utf-8")
            try:
                build_target(settings, target)
            except RuntimeError as error:
                row["status"] = "compile-failed"
                row["diagnostics"] = compile_diagnostics(_failed_build_output(settings, str(error)))
                row["suggested_includes"] = suggest_includes(settings.repo_dir, row["diagnostics"])
                continue
            comparison = compare_selected(settings.repo_dir, target, [address])
            functions = comparison.get("functions") or []
            if not functions:
                row["status"] = "not-compared"
                continue
            entity = functions[0]
            row["status"] = entity["status"]
            row["raw_matching"] = entity.get("raw_matching")
            if entity["status"] not in {"exact", "effective"}:
                triage = triage_selected(settings.repo_dir, target, [address])
                findings = triage.get("functions") or []
                if findings:
                    row["first_divergence"] = findings[0]
    finally:
        path.write_text(original, encoding="utf-8")

    ranked = sorted(rows, key=_candidate_rank, reverse=True)
    chosen = ranked[0] if ranked and _candidate_rank(ranked[0])[0] > 0 else None
    result: dict[str, Any] = {
        "address": f"0x{address:08x}",
        "status": "previewed",
        "source_file": source_file,
        "after_line": after_line,
        "candidates": rows,
        "chosen": chosen["candidate"] if chosen else None,
    }
    if apply and chosen is not None:
        chosen_block = dict(candidates)[chosen["candidate"]]
        path.write_text(insert_lines(original, after_line, chosen_block), encoding="utf-8")
        result["status"] = "applied"
        result["note"] = "source updated; rerun the applicable validation lane"
    elif apply:
        result["status"] = "previewed"
        result["note"] = "no candidate compiled; nothing applied"
    return result


def _candidate_rank(row: dict[str, Any]) -> tuple[int, float]:
    order = {"exact": 4, "effective": 3, "mismatch": 2, "not-compared": 1}
    return (
        order.get(str(row.get("status")), 0),
        float(row.get("raw_matching") or 0.0),
    )


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
