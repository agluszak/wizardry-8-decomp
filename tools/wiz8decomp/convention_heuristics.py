"""Binary calling-convention heuristics for unrecovered functions.

Source-backed ``prototype-repair`` already covers recovered ``FUNCTION``
markers. This pass scores remaining bodies and optionally applies only
high-confidence ``__thiscall`` conventions.

Evidence rules (asymmetric by design):

- Vtable membership is strong ``__thiscall`` evidence and is apply-eligible alone.
- Incoming ECX before definition is strong-ish ``__thiscall`` evidence; apply only
  when paired with another independent signal (e.g. callee stack purge).
- ``ret N`` / stack purge only means "callee cleans N bytes". It never proposes
  ``__stdcall`` by itself (MSVC ``__thiscall`` methods also purge explicit args).
- ``__cdecl`` is never inferred from "no purge + no ECX"; that needs caller-side
  evidence which this pass does not collect.

Everything else stays in the dry-run report as a candidate (``high_confidence``
false) and is never mutated.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json
from .source_index import source_functions

_SCHEMA = "wiz8.calling-convention-heuristics-v1"
_SOFT = frozenset({"unknown", "default"})
# Ghidra uses Integer.MAX_VALUE / MAX_VALUE-1 as invalid / unknown purge sentinels.
_INVALID_PURGE = frozenset({2147483647, 2147483646})


def _stack_purge(function: Any) -> int | None:
    """Return a real callee stack-purge size, or None when invalid/unknown."""

    try:
        if hasattr(function, "isStackPurgeSizeValid") and not function.isStackPurgeSizeValid():
            return None
        purge = int(function.getStackPurgeSize())
    except Exception:  # noqa: BLE001
        return None
    if purge in _INVALID_PURGE:
        return None
    return purge


def _register_name(obj: Any) -> str | None:
    try:
        from ghidra.program.model.lang import Register  # type: ignore[import-not-found]
    except ImportError:
        Register = None  # type: ignore[misc, assignment]

    if Register is not None and isinstance(obj, Register):
        return str(obj.getName()).upper()
    name = getattr(obj, "getName", None)
    if callable(name):
        try:
            return str(name()).upper()
        except Exception:  # noqa: BLE001
            return None
    type_name = type(obj).__name__
    if "Register" in type_name:
        text = str(obj).upper()
        return text if text else None
    return None


def _op_contains_ecx(instruction: Any, op_index: int) -> bool:
    try:
        objects = list(instruction.getOpObjects(op_index))
    except Exception:  # noqa: BLE001
        objects = []
    for obj in objects:
        reg = _register_name(obj)
        if reg == "ECX":
            return True
    try:
        text = str(instruction.getDefaultOperandRepresentation(op_index)).upper()
    except Exception:  # noqa: BLE001
        return False
    # Whole-register ECX only; avoid matching substrings in labels/symbols.
    tokens = {part.strip("[]*") for part in text.replace(",", " ").split()}
    return "ECX" in tokens


def _dest_is_ecx(instruction: Any) -> bool:
    try:
        if instruction.getNumOperands() < 1:
            return False
    except Exception:  # noqa: BLE001
        return False
    try:
        objects = list(instruction.getOpObjects(0))
    except Exception:  # noqa: BLE001
        objects = []
    if len(objects) == 1:
        reg = _register_name(objects[0])
        if reg == "ECX":
            return True
        if reg is not None:
            return False
    try:
        return str(instruction.getDefaultOperandRepresentation(0)).upper() == "ECX"
    except Exception:  # noqa: BLE001
        return False


def _is_xor_ecx_self(instruction: Any) -> bool:
    """True for ``XOR ECX,ECX`` (defines without consuming incoming this)."""

    if not _dest_is_ecx(instruction):
        return False
    try:
        if instruction.getNumOperands() < 2:
            return False
    except Exception:  # noqa: BLE001
        return False
    return _op_contains_ecx(instruction, 1)


def _uses_incoming_ecx(program: Any, function: Any) -> bool | None:
    """True when ECX is read before being defined in the prologue window."""

    listing = program.getListing()
    instructions = list(listing.getInstructions(function.getBody(), True))
    if not instructions:
        return None
    defined = False
    for index, instruction in enumerate(instructions[:24]):
        mnem = instruction.getMnemonicString().casefold()
        dest_ecx = _dest_is_ecx(instruction)

        if not defined:
            # Pure defines: write ECX without consuming the incoming value.
            if mnem in {"mov", "lea", "pop"} and dest_ecx:
                defined = True
                continue
            if mnem == "xor" and _is_xor_ecx_self(instruction):
                defined = True
                continue
            # Callee-save push is not thiscall evidence.
            if mnem == "push":
                continue
            # RMW (ADD/SUB/AND/OR/XOR ECX,…) or any other ECX input.
            try:
                num_ops = int(instruction.getNumOperands())
            except Exception:  # noqa: BLE001
                num_ops = 0
            for op_index in range(num_ops):
                if _op_contains_ecx(instruction, op_index):
                    return True
        if index > 12 and not defined:
            break
    return False


def _census_slot_targets(program: Any, repo_dir: Path, work_dir: Path) -> set[int]:
    """Vtable member entry points limited to confirmed census slot counts."""

    from .vftable_typing import census_vftable_slot_counts

    census = census_vftable_slot_counts(repo_dir, work_dir)
    memory = program.getMemory()
    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    targets: set[int] = set()
    for table, slot_count in census.items():
        count = int(slot_count)
        if count <= 0:
            continue
        for index in range(count):
            entry = space.getAddress(table + index * 4)
            if not memory.contains(entry):
                break
            try:
                value = memory.getInt(entry) & 0xFFFFFFFF
            except Exception:  # noqa: BLE001
                break
            if functions.getFunctionAt(space.getAddress(value)) is None:
                break
            targets.add(value)
    return targets


def _valid_purge(purge: int | None) -> int | None:
    if purge is None or purge in _INVALID_PURGE:
        return None
    return purge


def classify_convention_evidence(
    *,
    in_vtable: bool,
    incoming_ecx: bool | None,
    purge: int | None,
) -> dict[str, Any]:
    """Pure evidence classifier used by collect and unit tests.

    Returns ``proposed``, ``evidence``, and ``high_confidence`` for apply gating.
    Invalid Ghidra purge sentinels never contribute evidence or high confidence.
    """

    purge = _valid_purge(purge)
    evidence: list[str] = []
    if in_vtable:
        evidence.append("vtable-slot")
    if incoming_ecx is True:
        evidence.append("incoming-ecx")
    if purge is not None and 0 < purge < 0x7FFFFFFE:
        evidence.append(f"callee-cleans-{purge}")
    if purge == 0:
        evidence.append("callee-cleans-0")
    if incoming_ecx is False:
        evidence.append("no-incoming-ecx")

    proposed: str | None = None
    if "vtable-slot" in evidence or "incoming-ecx" in evidence:
        proposed = "__thiscall"

    # Apply only when method identity is independently corroborated (vtable slot).
    # Incoming ECX + callee cleanup also matches x86 __fastcall and must stay
    # report-only until stronger ABI evidence excludes competing conventions.
    high = proposed == "__thiscall" and "vtable-slot" in evidence

    return {
        "proposed": proposed,
        "evidence": evidence,
        "high_confidence": high,
    }


def collect_convention_heuristic_plan(
    repository: Path,
    program: Any,
    *,
    work_dir: Path,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Plan convention repairs for unrecovered functions."""

    recovered = {
        address
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION"
    }
    try:
        vtable_slots = _census_slot_targets(program, repository, work_dir)
    except Exception:  # noqa: BLE001 — missing binary / scan failure
        vtable_slots = set()
    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()

    iterable = (
        [functions.getFunctionAt(space.getAddress(address)) for address in addresses]
        if addresses is not None
        else list(functions.getFunctions(True))
    )

    for function in iterable:
        if function is None or function.isExternal() or function.isThunk():
            continue
        entry = int(function.getEntryPoint().getOffset())
        if entry in recovered:
            continue
        current = str(function.getCallingConventionName() or "unknown")
        if current not in _SOFT and current.startswith("__"):
            counts["already-explicit"] += 1
            continue

        purge = _stack_purge(function)
        ecx = _uses_incoming_ecx(program, function)
        classified = classify_convention_evidence(
            in_vtable=entry in vtable_slots,
            incoming_ecx=ecx,
            purge=purge,
        )
        proposed = classified["proposed"]
        evidence = classified["evidence"]
        high = bool(classified["high_confidence"])

        if proposed is None:
            counts["inconclusive"] += 1
            continue
        if current == proposed:
            counts["agree"] += 1
            continue

        action = "set-from-heuristic" if high else "candidate"
        counts[action] += 1
        rows.append(
            {
                "address": f"0x{entry:08x}",
                "name": function.getName(True),
                "ghidra": current,
                "proposed": proposed,
                "evidence": evidence,
                "action": action,
                "high_confidence": high,
            }
        )
        if limit is not None and len(rows) >= limit:
            break

    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": counts.get("set-from-heuristic", 0),
        "functions": rows,
    }


def apply_convention_heuristics(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Apply high-confidence heuristic conventions only."""

    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    applied: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []
    for row in plan.get("functions", []):
        if not row.get("high_confidence"):
            continue
        if row.get("action") != "set-from-heuristic":
            continue
        if row.get("proposed") != "__thiscall":
            # Never promote stdcall/cdecl from this pass.
            continue
        function = functions.getFunctionAt(space.getAddress(int(row["address"], 0)))
        if function is None:
            errors.append({**row, "error": "missing-function"})
            continue
        try:
            function.setCallingConvention("__thiscall")
            # Binary heuristics remain ANALYSIS provenance (not source/PDB).
            function.setSignatureSource(SourceType.ANALYSIS)
            applied.append(
                {
                    "address": row["address"],
                    "name": row.get("name"),
                    "from": row.get("ghidra"),
                    "to": "__thiscall",
                    "evidence": row.get("evidence"),
                }
            )
        except Exception as exc:  # noqa: BLE001
            errors.append({**row, "error": str(exc)})
    return {"applied": len(applied), "errors": errors, "functions": applied}


def run_convention_heuristics(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    addresses: Sequence[int] | None = None,
    limit: int | None = None,
) -> dict[str, Any]:
    """Report or apply unrecovered-body calling-convention heuristics."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_convention_heuristic_plan(
            settings.repo_dir,
            program,
            work_dir=settings.work_dir,
            target=target,
            addresses=addresses,
            limit=limit,
        )
        out_dir = settings.build_dir / "convention-heuristics"
        report_path = out_dir / "report.json"
        atomic_json(report_path, {**plan, "program": program_name, "apply": apply})
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": str(report_path.relative_to(settings.repo_dir)),
            "sample": [row for row in plan["functions"] if row.get("high_confidence")][:20],
            "candidates_sample": [
                row for row in plan["functions"] if not row.get("high_confidence")
            ][:20],
        }
        if not apply:
            return result
        with pyghidra.transaction(program, "Heuristic calling convention repair"):
            applied = apply_convention_heuristics(program, plan)
        dispose_sessions()
        program.save("Heuristic calling convention repair", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["sample"] = applied["functions"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = str(error_path.relative_to(settings.repo_dir))
        return result
