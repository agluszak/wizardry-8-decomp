"""Repair function attributes: varargs from source, noreturn for known fatals.

Also mark high-confidence thunks: pure single-``JMP`` bodies and classic MSVC
``ADD``/``SUB ECX,imm`` adjustors. Thunk apply is gated separately from
varargs/noreturn.

Leaf-name noreturn candidates (``srAssertFail``, etc. without a source
``FUNCTION`` marker) stay report-only (``report-noreturn``); ``--apply`` does
not set noreturn from the leaf name alone.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json
from .source_index import source_functions

_SCHEMA = "wiz8.function-attributes-v1"

# Local fatal helpers that never return; verified by body shape / source.
_NORETURN_NAMES = frozenset(
    {
        "FatalError",
        "ShutdownWithErrorBox",
        "assert_fail",
        "srAssertFail",
    }
)


def _jmp_target_function(program: Any, instruction: Any, self_fn: Any) -> Any | None:
    if instruction.getMnemonicString().upper() != "JMP":
        return None
    flows = list(instruction.getFlows())
    if len(flows) != 1:
        return None
    target = program.getFunctionManager().getFunctionAt(flows[0])
    if target is None or target == self_fn:
        return None
    return target


def _pure_jmp_thunk_target(program: Any, function: Any) -> Any | None:
    """Return the callee of a single-instruction ``JMP`` body, else None."""

    listing = program.getListing()
    instructions = list(listing.getInstructions(function.getBody(), True))
    if len(instructions) != 1:
        return None
    return _jmp_target_function(program, instructions[0], function)


def _is_scalar(obj: Any) -> bool:
    """True when ``obj`` is a Ghidra ``Scalar`` immediate."""

    from ghidra.program.model.scalar import Scalar  # type: ignore[import-not-found]

    return isinstance(obj, Scalar)


def _is_ecx_immediate_adjust(instruction: Any) -> bool:
    """True for ``ADD ECX,imm`` / ``SUB ECX,imm`` (classic MI adjustor prefix)."""

    mnemonic = instruction.getMnemonicString().upper()
    if mnemonic not in {"ADD", "SUB"}:
        return False
    dest = str(instruction.getDefaultOperandRepresentation(0)).upper()
    if dest != "ECX":
        return False
    # Immediate second operand (disp8/disp32); reject register/memory forms.
    try:
        src = instruction.getOpObjects(1)
    except Exception:  # noqa: BLE001
        return False
    if not src:
        return False
    return all(_is_scalar(obj) for obj in src)


def _ecx_adjustor_thunk_target(program: Any, function: Any) -> dict[str, Any] | None:
    """Classic adjustor: ``ADD``/``SUB ECX,imm`` then ``JMP`` to a function."""

    listing = program.getListing()
    instructions = list(listing.getInstructions(function.getBody(), True))
    if len(instructions) != 2:
        return None
    if not _is_ecx_immediate_adjust(instructions[0]):
        return None
    target = _jmp_target_function(program, instructions[1], function)
    if target is None:
        return None
    return {
        "target": target,
        "kind": "ecx-adjustor",
        "prefix": str(instructions[0]),
    }


def collect_function_attribute_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
) -> dict[str, Any]:
    """Plan varargs/noreturn/thunk repairs from source markers and body shape."""

    markers = {
        address: marker
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION" and marker.declaration is not None
    }
    if addresses is not None:
        wanted = set(addresses)
        markers = {address: marker for address, marker in markers.items() if address in wanted}

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    seen_addresses: set[int] = set()

    for address, marker in sorted(markers.items()):
        declaration = marker.declaration
        assert declaration is not None
        function = functions.getFunctionAt(space.getAddress(address))
        if function is None:
            counts["missing-function"] += 1
            continue

        actions: list[str] = []
        if declaration.is_variadic and not function.hasVarArgs():
            actions.append("set-varargs")
        elif declaration.is_variadic and function.hasVarArgs():
            counts["varargs-agree"] += 1

        noreturn_name = marker.name.split("::")[-1]
        wants_noreturn = noreturn_name in _NORETURN_NAMES
        if wants_noreturn and not function.hasNoReturn():
            actions.append("set-noreturn")
        elif wants_noreturn and function.hasNoReturn():
            counts["noreturn-agree"] += 1

        if not actions:
            continue
        action = "+".join(actions)
        counts[action] += 1
        seen_addresses.add(address)
        rows.append(
            {
                "address": f"0x{address:08x}",
                "name": marker.name,
                "action": action,
                "has_varargs": bool(function.hasVarArgs()),
                "has_noreturn": bool(function.hasNoReturn()),
                "source_variadic": bool(declaration.is_variadic),
            }
        )

    for function in functions.getFunctions(True):
        if function.isExternal():
            continue
        name = function.getName()
        address = int(function.getEntryPoint().getOffset())
        # Leaf-name fatals without a source FUNCTION marker stay report-only.
        # --apply must not set noreturn from the name alone.
        if name in _NORETURN_NAMES and not function.hasNoReturn() and address not in seen_addresses:
            counts["report-noreturn"] += 1
            seen_addresses.add(address)
            rows.append(
                {
                    "address": f"0x{address:08x}",
                    "name": name,
                    "action": "report-noreturn",
                    "has_varargs": bool(function.hasVarArgs()),
                    "has_noreturn": False,
                    "source_variadic": False,
                }
            )
        if function.isThunk():
            counts["thunk-agree"] += 1
            continue
        if function.getBody().getNumAddresses() > 16:
            continue

        thunk: dict[str, Any] | None = None
        pure = _pure_jmp_thunk_target(program, function)
        if pure is not None:
            thunk = {"target": pure, "kind": "pure-jmp", "prefix": None}
        if thunk is None:
            thunk = _ecx_adjustor_thunk_target(program, function)
        if thunk is None:
            continue

        target_fn = thunk["target"]
        counts[f"set-thunk:{thunk['kind']}"] += 1
        target_offset = int(target_fn.getEntryPoint().getOffset())
        rows.append(
            {
                "address": f"0x{address:08x}",
                "name": function.getName(True),
                "action": "set-thunk",
                "thunk_kind": thunk["kind"],
                "thunk_prefix": thunk.get("prefix"),
                "thunk_target": f"0x{target_offset:08x}",
                "thunk_name": target_fn.getName(True),
            }
        )

    actionable = sum(counts[key] for key in counts if key.startswith("set-") or "+" in key)
    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": actionable,
        "functions": rows,
    }


def apply_function_attributes(
    program: Any,
    plan: dict[str, Any],
    *,
    apply_attributes: bool = True,
    apply_thunks: bool = False,
) -> dict[str, Any]:
    """Apply planned attributes.

    ``apply_attributes`` covers varargs/noreturn. ``apply_thunks`` covers
    ``set-thunk`` rows with ``thunk_kind == "pure-jmp"`` only; ecx-adjustor
    candidates remain report-only.
    """

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    applied: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []

    for row in plan.get("functions", []):
        action = str(row.get("action") or "")
        if action == "report-noreturn":
            continue
        if action == "set-thunk":
            if not apply_thunks:
                continue
            # Collect still reports ecx-adjustor candidates; apply only pure JMP.
            if row.get("thunk_kind") != "pure-jmp":
                continue
        elif apply_attributes and (action.startswith("set-") or "+" in action):
            pass
        else:
            continue
        function = functions.getFunctionAt(space.getAddress(int(row["address"], 0)))
        if function is None:
            errors.append({**row, "error": "missing-function"})
            continue
        try:
            if action != "set-thunk":
                if "set-varargs" in action:
                    function.setVarArgs(True)
                if "set-noreturn" in action:
                    function.setNoReturn(True)
            else:
                target = functions.getFunctionAt(space.getAddress(int(row["thunk_target"], 0)))
                if target is None:
                    errors.append({**row, "error": "missing-thunk-target"})
                    continue
                function.setThunkedFunction(target)
            applied.append(
                {
                    "address": row["address"],
                    "name": row.get("name"),
                    "action": action,
                    "thunk_kind": row.get("thunk_kind"),
                }
            )
        except Exception as exc:  # noqa: BLE001
            errors.append({**row, "error": str(exc)})
    return {"applied": len(applied), "errors": errors, "functions": applied}


def run_function_attributes(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    apply_thunks: bool = False,
    addresses: Sequence[int] | None = None,
) -> dict[str, Any]:
    """Report or apply source-backed function attributes.

    ``apply`` writes varargs/noreturn only. ``apply_thunks`` writes planned
    pure-``JMP`` thunks (ecx-adjustors stay report-only). Either flag may be
    set independently.
    """

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_function_attribute_plan(
            settings.repo_dir, program, target=target, addresses=addresses
        )
        out_dir = settings.build_dir / "function-attributes"
        report_path = out_dir / "report.json"
        atomic_json(
            report_path,
            {
                **plan,
                "program": program_name,
                "apply": apply,
                "apply_thunks": apply_thunks,
            },
        )
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "apply_thunks": apply_thunks,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "report": str(report_path.relative_to(settings.repo_dir)),
            "sample": plan["functions"][:20],
        }
        if not apply and not apply_thunks:
            return result

        with pyghidra.transaction(program, "Source-backed function attributes"):
            applied = apply_function_attributes(
                program,
                plan,
                apply_attributes=apply,
                apply_thunks=apply_thunks,
            )
        dispose_sessions()
        program.save("Source-backed function attributes", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["sample"] = applied["functions"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = str(error_path.relative_to(settings.repo_dir))
        return result
