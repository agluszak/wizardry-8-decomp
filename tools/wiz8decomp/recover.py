"""Recovery-compiler regression harness.

``wiz8 recover regress`` measures how much of a recovered function the
Ghidra exporter can regenerate with zero manual edits: for each selected
address it exports the function, grafts the exported body under the
source-owned declaration (the declaration's compiler-owned spellings are
evidence the exporter must not overwrite), splices the result over the
existing recovered block in its owning translation unit, builds the product,
runs the relocation-masked reccmp comparison for that address, and always
restores the file afterwards. Nothing is written permanently; the
deliverable is the per-function report.

The harness needs the live Ghidra project and the pinned VC6 toolchain, so it
is a manual/milestone gate, not part of ``just check`` or ``just test``.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .config import Settings


def exported_blocks(result: dict[str, Any]) -> dict[int, str]:
    """The Java exporter's independently bounded per-entry results."""

    blocks: dict[int, str] = {}
    for item in result.get("exports", []):
        entry = item.get("entry")
        text = item.get("text")
        if isinstance(entry, str) and isinstance(text, str):
            blocks[int(entry, 0)] = text
    return blocks


def exported_bodies(result: dict[str, Any]) -> dict[int, str]:
    """The Java renderer's structurally bounded generated bodies."""

    bodies: dict[int, str] = {}
    for item in result.get("exports", []):
        entry = item.get("entry")
        body = item.get("body")
        if isinstance(entry, str) and isinstance(body, str):
            bodies[int(entry, 0)] = body
    return bodies


def marker_span(marker: dict[str, Any]) -> tuple[str, int, int] | None:
    """The (source_file, first_line, last_line) 1-based inclusive span the
    exported block replaces.

    The source index records the declaration's own line span; the marker
    policy (enforced by ``just check``) places ``// FUNCTION:`` on the line
    immediately above the declaration, so the block's span is exactly
    ``declaration line - 1`` through ``end_line``. ``verify_marker_adjacency``
    proves that assumption against the file before any splice trusts it.
    """

    declaration = marker.get("declaration") or {}
    end_line = declaration.get("end_line")
    line = marker.get("line")
    if not isinstance(line, int) or not isinstance(end_line, int) or end_line < line:
        return None
    return str(marker["source_file"]), line - 1, end_line


def verify_marker_adjacency(original: str, first_line: int, address: int) -> bool:
    """Whether the span's first line really is the address's marker line."""

    lines = original.splitlines()
    if not 1 <= first_line <= len(lines):
        return False
    return lines[first_line - 1].strip() == f"// FUNCTION: WIZ8 0x{address:08x}"


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


def graft_source_signature(marker: dict[str, Any], exported_body: str) -> str | None:
    """Combine compiler-indexed source spelling with Java-bounded body text."""

    declaration = marker.get("declaration") or {}
    signature = declaration.get("source_signature")
    address = marker.get("address")
    if not isinstance(signature, str) or not isinstance(address, int):
        return None
    body = exported_body.lstrip("\n")
    if body and not body.endswith("\n"):
        body += "\n"
    return f"// FUNCTION: WIZ8 0x{address:08x}\n{signature}\n{body}"


_UNDECLARED = re.compile(r"error C2065: '([A-Za-z_][A-Za-z0-9_]*)' : undeclared identifier")


def suggest_includes(repo_dir: Path, diagnostics: list[str]) -> dict[str, list[str]]:
    """Headers declaring the undeclared identifiers a failed build names.

    A deterministic suggestion list for the human (or a later milestone), not
    an automatic edit. The compiler-backed source index owns declaration
    identity; header text is never regex-parsed as a substitute.
    """

    names = sorted(
        {match.group(1) for line in diagnostics for match in [_UNDECLARED.search(line)] if match}
    )
    if not names:
        return {}
    try:
        import json

        index = json.loads((repo_dir / "build/source-index.json").read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    wanted = set(names)
    found: dict[str, list[str]] = {}
    for item in [*index.get("classes", []), *index.get("declarations", [])]:
        qualified = item.get("qualified_name")
        source_file = item.get("source_file")
        if not isinstance(qualified, str) or not isinstance(source_file, str):
            continue
        leaf = qualified.rsplit("::", 1)[-1]
        if leaf in wanted and source_file.endswith((".h", ".hpp", ".inc")):
            found.setdefault(leaf, []).append(source_file)
    for headers in found.values():
        headers[:] = sorted(set(headers))
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
    source_index = load_source_index(settings.repo_dir)
    markers = source_index["markers"]
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
    block = exported_blocks(exported).get(address)
    if block is None:
        return {"address": f"0x{address:08x}", "status": "not-exported"}

    placement = place_address(markers, address)
    if placement["status"] != "placed":
        return {"address": f"0x{address:08x}", **placement, "block": block}

    source_file = placement["source_file"]
    after_line = placement["after_line"]
    path = settings.repo_dir / source_file
    original = path.read_text(encoding="utf-8")

    # One verbatim candidate only. Source-shape alternatives that mutate the
    # rendered C++ with regexes were removed deliberately: shape hypotheses
    # belong in the exporter's recognizers, where Ghidra's structural model
    # backs them, not in textual rewrites of its output.
    candidates: list[tuple[str, str]] = [("as-exported", block)]

    rows: list[dict[str, Any]] = []
    product_dirty = False
    try:
        for name, candidate in candidates:
            row: dict[str, Any] = {"candidate": name}
            rows.append(row)
            path.write_text(insert_lines(original, after_line, candidate), encoding="utf-8")
            product_dirty = True
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
    # The last temporary build may reflect a candidate other than the final
    # tree (restored or applied); rebuild so products match the sources.
    result["product_restored"] = _restore_product(settings, target, product_dirty)
    return result


def _candidate_rank(row: dict[str, Any]) -> tuple[int, int, float]:
    """Rank candidates: exact beats effective beats any mismatch; among real
    mismatches a later structured first divergence wins, and raw similarity
    is only the final diagnostic tie-breaker."""

    order = {"exact": 4, "effective": 3, "mismatch": 2, "not-compared": 1}
    return (
        order.get(str(row.get("status")), 0),
        divergence_position(row.get("first_divergence")),
        float(row.get("raw_matching") or 0.0),
    )


def divergence_position(finding: Any) -> int:
    """The retail instruction index of a structured first divergence, or 0."""

    if not isinstance(finding, dict):
        return 0
    difference = finding.get("difference")
    if not isinstance(difference, dict):
        return 0
    orig = difference.get("orig")
    if not isinstance(orig, dict):
        return 0
    index = orig.get("instruction_index")
    return index if isinstance(index, int) and index >= 0 else 0


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
    blocks = exported_blocks(exported)
    bodies = exported_bodies(exported)

    defects: list[str] = []
    product_dirty = False
    rows: list[dict[str, Any]] = []
    for address in addresses:
        row: dict[str, Any] = {"address": f"0x{address:08x}"}
        rows.append(row)
        block = blocks.get(address)
        if block is None:
            row["status"] = "not-exported"
            continue
        block_defects = exporter_defects(block)
        if block_defects:
            # A defect is a bug in the exporter, not a property of the
            # function; measuring its verbatim fallback as exporter output
            # would silently launder the bug into a score.
            row["status"] = "exporter-defect"
            row["defects"] = block_defects
            defects.extend(f"0x{address:08x}: {defect}" for defect in block_defects)
            continue
        marker = markers.get(address)
        if marker is None:
            row["status"] = "unplaced"
            continue
        span = marker_span(marker)
        if span is None:
            row["status"] = "unplaced"
            continue
        source_file, first_line, last_line = span
        row["source_file"] = source_file
        path = settings.repo_dir / source_file
        original = path.read_text(encoding="utf-8")
        if not verify_marker_adjacency(original, first_line, address):
            row["status"] = "unplaced"
            row["reason"] = "index span does not start at the address's marker line"
            continue
        body = bodies.get(address)
        grafted = graft_source_signature(marker, body) if body is not None else None
        if grafted is None:
            row["status"] = "source-signature-unavailable"
            continue
        block = grafted
        try:
            path.write_text(splice_lines(original, first_line, last_line, block), encoding="utf-8")
            product_dirty = True
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
        "product_restored": _restore_product(settings, target, product_dirty),
    }
    if defects:
        raise RuntimeError(
            "exporter defects detected (sources and build restored):\n  " + "\n  ".join(defects)
        )
    return summary


def exporter_defects(block: str) -> list[str]:
    """The defect records the exporter flagged inside a block."""

    return [
        line.removeprefix("// exporter-defect: ").strip()
        for line in block.splitlines()
        if line.startswith("// exporter-defect:")
    ]


def _restore_product(settings: Settings, target: str, product_dirty: bool) -> bool:
    """Rebuild the restored tree so build products match the sources again.

    A temporary splice that reached the build leaves objects and the linked
    image reflecting text that no longer exists; a later comparison against
    them would be measuring a ghost. Returns True when products are known
    to match the restored tree.
    """

    if not product_dirty:
        return True
    from .build import build_target

    try:
        build_target(settings, target)
        return True
    except RuntimeError:
        return False


# ----------------------------------------------------------------------
# Sweep: repository-wide zero-edit regeneration measurement
# ----------------------------------------------------------------------

_DIAGNOSTIC_LINE = re.compile(r"([^\s(\\/]+\.cpp)\((\d+)\)\s*:")
_IDENTITY_GAP = re.compile(r"'(?:FUN_|DAT_|LAB_|PTR_|s_|u_|switchD)[A-Za-z0-9_.:\\]*'")
_INTRINSIC_GAP = re.compile(
    r"'(?:SBORROW\d|CARRY\d|SUB\d\d?|CONCAT\d\d?|ZEXT\d\d?|SEXT\d\d?|POPCOUNT|"
    r"ftol|NAN|ROUND|extraout_[A-Za-z0-9_]*|unaff_[A-Za-z0-9_]*|in_[A-Za-z0-9_]*|"
    r"undefined\d?)'"
)
_DECLARATION_GAP = re.compile(r"error C(?:2511|2440|2660|2661|2653|2039|2065)")


def categorize_failure(diagnostics: list[str]) -> str:
    """One failure category for a compile-failed function's diagnostics.

    Precedence is evidence strength: an unnamed Ghidra identity or an
    unlowered decompiler intrinsic explains the failure completely, while a
    declaration/type mismatch is the residual category for real ABI or
    spelling gaps.
    """

    text = "\n".join(diagnostics)
    if _INTRINSIC_GAP.search(text):
        return "unsupported-intrinsic"
    if _IDENTITY_GAP.search(text):
        return "unresolved-identity"
    if _DECLARATION_GAP.search(text):
        return "declaration-or-type"
    return "other-compile-failure"


_CORRUPTION_ERROR = re.compile(r"error C(?:2059|2143|2146|2015|2589|2654|1004|2001)")


def _earliest_corruption_line(diagnostics: list[str], source_file: str) -> int | None:
    """The first line of the file carrying a parse-corrupting diagnostic
    (syntax errors, malformed constants, member access outside any
    function). Corruption propagates forward across block boundaries, so
    diagnostics before this line are honest and diagnostics at or after it
    may be cascade noise."""

    earliest = None
    for diagnostic in diagnostics:
        if not _CORRUPTION_ERROR.search(diagnostic):
            continue
        match = _DIAGNOSTIC_LINE.search(diagnostic.replace("\\", "/"))
        if match is None or not source_file.endswith(match.group(1)):
            continue
        line = int(match.group(2))
        if earliest is None or line < earliest:
            earliest = line
    return earliest


def splice_unit(
    original: str, plan: list[tuple[int, int, int, str]]
) -> tuple[str, dict[int, tuple[int, int]]]:
    """Apply many block splices to one file in ascending line order.

    ``plan`` rows are ``(first_line, last_line, address, block)`` with
    non-overlapping 1-based inclusive spans. Returns the new text plus each
    address's line range inside it, which is what maps a compiler
    diagnostic back to the block that caused it.
    """

    lines = original.splitlines(keepends=True)
    ordered = sorted(plan)
    result: list[str] = []
    ranges: dict[int, tuple[int, int]] = {}
    cursor = 0  # 0-based index into lines, next unconsumed original line
    emitted = 0
    for first_line, last_line, address, block in ordered:
        if first_line <= cursor or last_line > len(lines):
            raise ValueError(f"overlapping or out-of-range span {first_line}..{last_line}")
        result.extend(lines[cursor : first_line - 1])
        emitted += first_line - 1 - cursor
        if not block.endswith("\n"):
            block += "\n"
        block_lines = block.splitlines(keepends=True)
        ranges[address] = (emitted + 1, emitted + len(block_lines))
        result.extend(block_lines)
        emitted += len(block_lines)
        cursor = last_line
    result.extend(lines[cursor:])
    return "".join(result), ranges


def attribute_diagnostics(
    diagnostics: list[str], ranges_by_file: dict[str, dict[int, tuple[int, int]]]
) -> tuple[dict[int, list[str]], list[str]]:
    """Map compiler diagnostics onto the spliced blocks that own their lines.

    Returns per-address diagnostics plus the diagnostics that hit no spliced
    block (cascades into untouched code, or lines outside any splice).
    """

    per_address: dict[int, list[str]] = {}
    unattributed: list[str] = []
    for diagnostic in diagnostics:
        match = _DIAGNOSTIC_LINE.search(diagnostic.replace("\\", "/"))
        owner = None
        if match is not None:
            file_name = match.group(1)
            line = int(match.group(2))
            for source_file, ranges in ranges_by_file.items():
                if not source_file.endswith(file_name):
                    continue
                for address, (first, last) in ranges.items():
                    if first <= line <= last:
                        owner = address
                        break
                break
        if owner is None:
            unattributed.append(diagnostic)
        else:
            per_address.setdefault(owner, []).append(diagnostic)
    return per_address, unattributed


def _sweep_selection(
    settings: Settings, source_file: str | None, class_name: str | None
) -> list[dict[str, Any]]:
    from .source_model import load_source_index

    markers = [
        marker
        for marker in load_source_index(settings.repo_dir)["markers"]
        if marker["marker_kind"] == "FUNCTION"
    ]
    if source_file is not None:
        markers = [marker for marker in markers if marker["source_file"] == source_file]
    if class_name is not None:
        import json

        index_path = settings.repo_dir / "build" / "ghidra-index" / "functions.json"
        if not index_path.is_file():
            raise RuntimeError(
                "class filtering needs build/ghidra-index/functions.json; "
                "run `uv run wiz8 ghidra index` first"
            )
        functions = json.loads(index_path.read_text(encoding="utf-8"))["functions"]
        wanted = {
            int(record["entry"], 16)
            for record in functions
            if record["qualified_name"].startswith(class_name + "::")
        }
        markers = [marker for marker in markers if marker["address"] in wanted]
    if not markers:
        raise ValueError("no recovered FUNCTION markers match the selection")
    return markers


def sweep(
    settings: Settings,
    *,
    source_file: str | None = None,
    class_name: str | None = None,
    target: str = "WIZ8",
    program_selector: str = "wiz8",
    rounds: int = 40,
) -> dict[str, Any]:
    """Classify zero-edit regeneration for every selected recovered function.

    All selected blocks are spliced at once and built together; compile
    diagnostics are attributed to the owning block, the failing blocks are
    restored, and the build repeats until it is clean (bounded rounds).
    Attribution is cascade-aware: parse corruption flows forward through a
    unit, so of the blocks carrying corruption-class diagnostics only the
    file's earliest is categorized and restored per round — the later ones
    stay spliced and are re-measured once the corrupting block is gone.
    The surviving splices are then compared in one batch. Sources are always
    restored and the product is rebuilt from the restored tree.
    """

    from .build import build_target
    from .ghidra.export_cpp import export_cpp
    from .reccmp_workflows import compare_selected, triage_selected

    markers = _sweep_selection(settings, source_file, class_name)
    addresses = [marker["address"] for marker in markers]
    exported = export_cpp(
        settings, [f"0x{a:08x}" for a in addresses], program_selector=program_selector
    )
    blocks = exported_blocks(exported)
    bodies = exported_bodies(exported)

    outcomes: dict[int, dict[str, Any]] = {}
    plan_by_file: dict[str, list[tuple[int, int, int, str]]] = {}
    originals: dict[str, str] = {}
    for marker in markers:
        address = marker["address"]
        outcome: dict[str, Any] = {
            "address": f"0x{address:08x}",
            "source_file": marker["source_file"],
        }
        outcomes[address] = outcome
        block = blocks.get(address)
        if block is None:
            outcome["status"] = "not-exported"
            continue
        if "/* Unable to decompile" in block or "/* No instruction at" in block:
            outcome["status"] = "decompiler-failure"
            continue
        defects = exporter_defects(block)
        if defects:
            outcome["status"] = "exporter-defect"
            outcome["defects"] = defects
            continue
        span = marker_span(marker)
        if span is None:
            outcome["status"] = "unplaced"
            continue
        path = settings.repo_dir / marker["source_file"]
        if marker["source_file"] not in originals:
            originals[marker["source_file"]] = path.read_text(encoding="utf-8")
        if not verify_marker_adjacency(originals[marker["source_file"]], span[1], address):
            outcome["status"] = "unplaced"
            outcome["reason"] = "index span does not start at the marker line"
            continue
        body = bodies.get(address)
        grafted = graft_source_signature(marker, body) if body is not None else None
        if grafted is None:
            outcome["status"] = "source-signature-unavailable"
            continue
        plan_by_file.setdefault(marker["source_file"], []).append(
            (span[1], span[2], address, grafted)
        )

    product_dirty = False
    try:
        live: dict[str, dict[int, tuple[int, int]]] = {}
        for source_file_name, plan in plan_by_file.items():
            text, ranges = splice_unit(originals[source_file_name], plan)
            (settings.repo_dir / source_file_name).write_text(text, encoding="utf-8")
            live[source_file_name] = ranges
            product_dirty = True

        for _ in range(rounds):
            if not any(live.values()):
                break
            try:
                build_target(settings, target)
                break
            except RuntimeError as error:
                diagnostics = compile_diagnostics(
                    _failed_build_output(settings, str(error)), limit=400
                )
                per_address, _ = attribute_diagnostics(diagnostics, live)
                if not per_address:
                    # Nothing attributable: restore every remaining splice and
                    # classify them as blocked rather than guessing.
                    for source_file_name, ranges in live.items():
                        for address in ranges:
                            outcomes[address]["status"] = "unit-blocked"
                        (settings.repo_dir / source_file_name).write_text(
                            originals[source_file_name], encoding="utf-8"
                        )
                    live = {}
                    break
                # Parse corruption flows forward through the unit, so the
                # earliest corruption-class diagnostic line splits each file:
                # blocks ending before it carry honest diagnostics, the
                # spliced block owning (or last preceding) it is the guilty
                # source, and every diagnosed block at or after it may be an
                # innocent victim — those stay spliced and are re-measured
                # once the corrupting block is gone.
                for source_file_name, ranges in live.items():
                    file_addresses = [a for a in per_address if a in ranges]
                    if not file_addresses:
                        continue
                    corruption = _earliest_corruption_line(diagnostics, source_file_name)
                    guilty = None
                    if corruption is not None:
                        preceding = [a for a in ranges if ranges[a][0] <= corruption]
                        if preceding:
                            guilty = max(preceding, key=lambda a: ranges[a][0])
                    for address in file_addresses:
                        past_corruption = (
                            corruption is not None and ranges[address][1] >= corruption
                        )
                        if past_corruption and address != guilty:
                            continue
                        outcome = outcomes[address]
                        outcome["status"] = categorize_failure(per_address[address])
                        outcome["diagnostics"] = per_address[address][:8]
                    if guilty is not None and "status" not in outcomes[guilty]:
                        fallback = (
                            f"parse corruption first surfaces at line {corruption}, "
                            "inside or directly after this block"
                        )
                        owned = per_address.get(guilty) or [fallback]
                        outcomes[guilty]["status"] = categorize_failure(owned)
                        outcomes[guilty]["diagnostics"] = owned[:8]
                for source_file_name, plan in plan_by_file.items():
                    remaining = [row for row in plan if "status" not in outcomes[row[2]]]
                    if len(remaining) == len(live.get(source_file_name, {})):
                        continue
                    text, ranges = splice_unit(originals[source_file_name], remaining)
                    (settings.repo_dir / source_file_name).write_text(text, encoding="utf-8")
                    live[source_file_name] = ranges
        else:
            # Rounds exhausted without a clean build: the survivors are
            # unmeasurable this run.
            for source_file_name, ranges in live.items():
                for address in ranges:
                    outcomes[address].setdefault("status", "unit-blocked")
                (settings.repo_dir / source_file_name).write_text(
                    originals[source_file_name], encoding="utf-8"
                )
            live = {}

        survivors = [address for ranges in live.values() for address in ranges]
        if survivors:
            comparison = compare_selected(settings.repo_dir, target, sorted(survivors))
            for entity in comparison.get("functions") or []:
                address = (
                    int(str(entity.get("address", "0x0")), 16) if "address" in entity else None
                )
                # reccmp reports entities in selection order; fall back to that
                # pairing when the record carries no address field.
                if address is None or address not in outcomes:
                    continue
                outcomes[address]["status"] = entity["status"]
                outcomes[address]["raw_matching"] = entity.get("raw_matching")
            paired = comparison.get("functions") or []
            if paired and all("address" not in entity for entity in paired):
                for address, entity in zip(sorted(survivors), paired):
                    outcomes[address]["status"] = entity["status"]
                    outcomes[address]["raw_matching"] = entity.get("raw_matching")
            mismatched = sorted(
                address for address in survivors if outcomes[address].get("status") == "mismatch"
            )
            if mismatched:
                triage = triage_selected(settings.repo_dir, target, mismatched)
                for finding in triage.get("functions") or []:
                    address = finding.get("address")
                    parsed = int(str(address), 16) if address else None
                    if parsed in outcomes:
                        outcomes[parsed]["first_divergence"] = finding
        for address in survivors:
            outcomes[address].setdefault("status", "not-compared")
    finally:
        for source_file_name, original in originals.items():
            (settings.repo_dir / source_file_name).write_text(original, encoding="utf-8")

    grouped: dict[str, int] = {}
    for outcome in outcomes.values():
        grouped[outcome.get("status", "unknown")] = (
            grouped.get(outcome.get("status", "unknown"), 0) + 1
        )
    return {
        "selected": len(outcomes),
        "summary": dict(sorted(grouped.items(), key=lambda kv: -kv[1])),
        "functions": [outcomes[address] for address in addresses],
        "product_restored": _restore_product(settings, target, product_dirty),
    }
