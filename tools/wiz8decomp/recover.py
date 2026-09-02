"""Recovery-compiler regression harness.

``wiz8 recover regress`` measures how much of a recovered function the
Ghidra engine can regenerate with zero manual edits: for each selected
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

import csv
import re
from pathlib import Path
from typing import Any

from .config import Settings


def exported_blocks(result: dict[str, Any]) -> dict[int, str]:
    """The recovery engine's independently bounded per-entry results."""

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


def exported_defects(result: dict[str, Any]) -> dict[int, list[str]]:
    """Structured recovery-engine defects by entry address."""

    defects: dict[int, list[str]] = {}
    for item in result.get("exports", []):
        entry = item.get("entry")
        recovery = item.get("recovery")
        values = recovery.get("defects") if isinstance(recovery, dict) else None
        if isinstance(entry, str) and isinstance(values, list):
            defects[int(entry, 0)] = [str(value) for value in values]
    return defects


def exported_declines(result: dict[str, Any]) -> dict[int, list[dict[str, str]]]:
    """Source-entity blockers which make an unattended write unsafe."""

    declines: dict[int, list[dict[str, str]]] = {}
    for item in result.get("exports", []):
        entry = item.get("entry")
        recovery = item.get("recovery")
        passes = recovery.get("passes") if isinstance(recovery, dict) else None
        if not isinstance(entry, str) or not isinstance(passes, list):
            continue
        blockers = [
            {
                "pass": str(value.get("pass", "")),
                "detail": str(value.get("detail", "")),
            }
            for value in passes
            if isinstance(value, dict)
            and value.get("status") == "declined"
            and value.get("pass") == "signature.prototype"
        ]
        if blockers:
            declines[int(entry, 0)] = blockers
    return declines


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
    return lines[first_line - 1].strip().casefold() == (
        f"// FUNCTION: WIZ8 0x{address:08x}".casefold()
    )


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

    line = int(marker["line"])
    kind = marker["marker_kind"]
    if kind in {"SYNTHETIC", "TEMPLATE"}:
        return line + 1
    declaration = marker.get("declaration") or {}
    end_line = declaration.get("end_line")
    return end_line if isinstance(end_line, int) else line


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


_UNIT_DIRECTORIES = {
    "3d code": "3d_code",
    "dialog code": "dialog_code",
    "engine code": "engine_code",
    "level specific code": "level_specific_code",
    "local code": "local_code",
    "local screens": "local_screens",
}


def repository_source_file(repo_dir: Path, unit: str, markers: list[dict[str, Any]]) -> str | None:
    """Map one reviewed original unit to its unique recovered physical file."""

    normalized = unit.replace("\\", "/")
    basename = normalized.rsplit("/", 1)[-1]
    candidates = sorted(
        {
            str(marker["source_file"])
            for marker in markers
            if Path(str(marker["source_file"])).name.casefold() == basename.casefold()
            and (repo_dir / str(marker["source_file"])).is_file()
        }
    )
    if len(candidates) == 1:
        return candidates[0]
    directory, separator, name = normalized.partition("/")
    if not separator:
        return None
    mapped = _UNIT_DIRECTORIES.get(directory.casefold())
    if mapped is None:
        return None
    candidate = Path("src/wiz8") / mapped / name
    return candidate.as_posix() if (repo_dir / candidate).is_file() else None


def resolve_source_placement(
    repo_dir: Path, markers: list[dict[str, Any]], address: int
) -> dict[str, Any]:
    """Resolve ownership with the same assertion-backed authority as context."""

    from .ghidra.unit_intervals import TranslationUnitResolver

    assertion_path = repo_dir / "evidence/observations/wiz8/assertions.csv"
    with assertion_path.open(newline="", encoding="utf-8") as stream:
        assertions = list(csv.DictReader(stream))
    ownership = TranslationUnitResolver(assertions).resolve(address)
    unit = str(ownership.get("source_path") or "")
    if not unit:
        return {
            "status": "unplaced",
            **ownership,
            "reason": "translation-unit resolver did not prove a source owner",
        }
    source_file = repository_source_file(repo_dir, unit, markers)
    if source_file is None:
        return {
            "status": "unplaced",
            **ownership,
            "reason": f"translation unit {unit} has no recovered physical source file",
        }
    owned = sorted(
        (marker for marker in markers if marker["source_file"] == source_file),
        key=lambda marker: marker["address"],
    )
    previous = next((marker for marker in reversed(owned) if marker["address"] < address), None)
    following = next((marker for marker in owned if marker["address"] > address), None)
    if previous is None and following is None:
        return {
            "status": "unplaced",
            **ownership,
            "reason": f"translation unit {unit} has no recovered insertion anchor",
        }
    if previous is not None:
        after_line = block_end_line(previous)
    else:
        assert following is not None
        after_line = max(int(following["line"]) - 1, 0)
    return {
        "status": "placed",
        **ownership,
        "source_file": source_file,
        "after_line": after_line,
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


_GENERATED_ADDRESS_NAME = re.compile(r"\b(?:_?DAT|PTR|UNK|LAB)_([0-9A-Fa-f]{8})\b")
_SOURCE_ADDRESS_NAME = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*_([0-9A-Fa-f]{8}))\b")
_FTOL_ASSIGNMENT = re.compile(
    r"(?m)^(?P<indent>[ \t]*)(?P<temp>[A-Za-z_][A-Za-z0-9_]*) = ftol\(\);\n"
    r"(?P=indent)(?P<lhs>[A-Za-z_][A-Za-z0-9_]*(?:\[[^\n;]+\])?) = "
    r"(?P=temp)(?P<tail>[^;]*);"
)


def project_source_forms(
    generated: str, source_file: str, source_block: str = ""
) -> tuple[str, list[dict[str, str]], list[str]]:
    """Project only unique source-owned spellings into generated C++."""

    by_address: dict[str, set[str]] = {}
    for match in _SOURCE_ADDRESS_NAME.finditer(source_file):
        by_address.setdefault(match.group(2).casefold(), set()).add(match.group(1))
    projections: list[dict[str, str]] = []

    def replace_address(match: re.Match[str]) -> str:
        candidates = by_address.get(match.group(1).casefold(), set())
        if len(candidates) != 1:
            return match.group(0)
        replacement = next(iter(candidates))
        projections.append({"kind": "identifier", "from": match.group(0), "to": replacement})
        return replacement

    projected = _GENERATED_ADDRESS_NAME.sub(replace_address, generated)
    for match in list(_FTOL_ASSIGNMENT.finditer(projected)):
        lhs = match.group("lhs")
        statements = re.findall(
            rf"(?m)^[ \t]*{re.escape(lhs)}[ \t]*=[ \t]*([^;\n]+);", source_block
        )
        cast_statements = [value for value in statements if "(int)" in value]
        if len(cast_statements) != 1:
            continue
        replacement = f"{match.group('indent')}{lhs} = {cast_statements[0]};"
        temp = match.group("temp")
        projected = projected[: match.start()] + replacement + projected[match.end() :]
        if len(re.findall(rf"\b{re.escape(temp)}\b", projected)) == 1:
            projected = re.sub(
                rf"(?m)^[ \t]*(?:short|int|long) {re.escape(temp)};[ \t]*\n", "", projected
            )
        projections.append({"kind": "source-cast", "from": "ftol", "to": lhs})
        break
    blockers = sorted(set(_GENERATED_ADDRESS_NAME.findall(projected)))
    if "ftol()" in projected:
        blockers.append("ftol")
    return projected, projections, blockers


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
    neighbours, then build/compare/triage the recovered definition. The
    default previews and restores the tree; ``--apply`` leaves a compiling
    definition in place.
    """

    from .build import build_target
    from .ghidra.recovery import recover_functions
    from .reccmp_workflows import compare_selected
    from .selectors import resolve_function_selectors
    from .source_model import load_source_index

    resolved = resolve_function_selectors(settings.repo_dir, [selection])
    if len(resolved) != 1:
        raise ValueError("recover function takes exactly one function selector")
    address = resolved[0]
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

    placement = resolve_source_placement(settings.repo_dir, markers, address)
    exported = recover_functions(settings, [f"0x{address:08x}"], program_selector=program_selector)
    declines = exported_declines(exported).get(address, [])
    if declines:
        decline_result: dict[str, Any] = {
            "address": f"0x{address:08x}",
            "status": "declined",
            "reason": "source entity has an unresolved formal prototype",
            "blockers": declines,
            "placement": placement["status"],
        }
        for key in ("source_path", "attribution", "source_file", "after_line"):
            if placement.get(key) is not None:
                decline_result[key] = placement[key]
        if placement["status"] != "placed" and placement.get("reason"):
            decline_result["placement_reason"] = placement["reason"]
        return decline_result
    block = exported_blocks(exported).get(address)
    if block is None:
        return {"address": f"0x{address:08x}", "status": "not-exported"}

    if placement["status"] != "placed":
        return {"address": f"0x{address:08x}", **placement, "block": block}

    source_file = placement["source_file"]
    after_line = placement["after_line"]
    path = settings.repo_dir / source_file
    original = path.read_text(encoding="utf-8")
    block, projections, blockers = project_source_forms(block, original)
    if blockers:
        return {
            "address": f"0x{address:08x}",
            "status": "declined",
            "reason": "generated source contains unresolved source forms",
            "source_file": source_file,
            "projections": projections,
            "blockers": blockers,
        }

    outcome: dict[str, Any] = {}
    product_dirty = False
    try:
        path.write_text(insert_lines(original, after_line, block), encoding="utf-8")
        product_dirty = True
        try:
            build_target(settings, target)
        except RuntimeError as error:
            outcome["status"] = "compile-failed"
            outcome["diagnostics"] = compile_diagnostics(_failed_build_output(settings, str(error)))
            outcome["suggested_includes"] = suggest_includes(
                settings.repo_dir, outcome["diagnostics"]
            )
        else:
            comparison = compare_selected(settings.repo_dir, target, [address])
            functions = comparison.get("functions") or []
            if not functions:
                outcome["status"] = "not-compared"
            else:
                entity = functions[0]
                outcome["status"] = entity["status"]
                outcome["raw_matching"] = entity.get("raw_matching")
                if entity["status"] not in {"exact", "effective"}:
                    outcome["first_divergence"] = entity
    finally:
        path.write_text(original, encoding="utf-8")

    compiling = outcome.get("status") not in {"compile-failed", "not-compared", None}
    result: dict[str, Any] = {
        "address": f"0x{address:08x}",
        "status": "previewed",
        "source_file": source_file,
        "after_line": after_line,
        "translation_unit": placement.get("source_path"),
        "attribution": placement.get("attribution"),
        "projections": projections,
        "recovery": outcome,
    }
    if apply and compiling:
        path.write_text(insert_lines(original, after_line, block), encoding="utf-8")
        result["status"] = "applied"
        result["note"] = "source updated; rerun the applicable validation lane"
    elif apply:
        result["status"] = "previewed"
        result["note"] = "no candidate compiled; nothing applied"
    # The temporary build may reflect a restored tree; rebuild so products
    # match the final sources.
    result["product_restored"] = _restore_product(settings, target, product_dirty)
    return result


def regress(
    settings: Settings,
    selections: list[str],
    *,
    target: str = "WIZ8",
    program_selector: str = "wiz8",
) -> dict[str, Any]:
    from .build import build_target
    from .ghidra.recovery import recover_functions
    from .reccmp_workflows import compare_selected
    from .selectors import resolve_function_selectors
    from .source_model import load_source_index

    addresses = resolve_function_selectors(settings.repo_dir, selections)
    if not addresses:
        raise ValueError("pass one or more function addresses")

    markers = {
        marker["address"]: marker
        for marker in load_source_index(settings.repo_dir)["markers"]
        if marker["marker_kind"] == "FUNCTION"
    }

    exported = recover_functions(
        settings, [f"0x{a:08x}" for a in addresses], program_selector=program_selector
    )
    blocks = exported_blocks(exported)
    bodies = exported_bodies(exported)
    packet_defects = exported_defects(exported)

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
        block_defects = packet_defects.get(address, [])
        if block_defects:
            # A defect is a bug in recovery, not a property of the
            # function; measuring its verbatim fallback as exporter output
            # would silently launder the bug into a score.
            row["status"] = "recovery-defect"
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
        source_lines = original.splitlines(keepends=True)
        source_block = "".join(source_lines[first_line - 1 : last_line])
        block, projections, blockers = project_source_forms(grafted, original, source_block)
        if projections:
            row["projections"] = projections
        if blockers:
            row["status"] = "source-form-unavailable"
            row["blockers"] = blockers
            continue
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
                row["first_divergence"] = entity
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
            "recovery defects detected (sources and build restored):\n  " + "\n  ".join(defects)
        )
    return summary


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
        markers = [
            marker
            for marker in markers
            if (marker.get("declaration") or {}).get("owning_class") == class_name
        ]
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
    from .ghidra.recovery import recover_functions
    from .reccmp_workflows import compare_selected

    markers = _sweep_selection(settings, source_file, class_name)
    addresses = [marker["address"] for marker in markers]
    exported = recover_functions(
        settings, [f"0x{a:08x}" for a in addresses], program_selector=program_selector
    )
    blocks = exported_blocks(exported)
    bodies = exported_bodies(exported)
    packet_defects = exported_defects(exported)

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
        defects = packet_defects.get(address, [])
        if defects:
            outcome["status"] = "recovery-defect"
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
                if entity["status"] == "mismatch":
                    outcomes[address]["first_divergence"] = entity
            paired = comparison.get("functions") or []
            if paired and all("address" not in entity for entity in paired):
                for address, entity in zip(sorted(survivors), paired):
                    outcomes[address]["status"] = entity["status"]
                    outcomes[address]["raw_matching"] = entity.get("raw_matching")
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
