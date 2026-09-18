"""Type ``this`` via Ghidra's native class namespace ↔ Structure binding.

Automatic ``this`` obtains its datatype from
``VariableUtilities.findOrCreateClassStruct(function)``. Enrichment should
establish that binding (correct ``GhidraClass`` parent + associated Structure)
and keep dynamic storage — not copy Structures into ``/wiz8/classes`` or enable
custom storage for ordinary methods.

Custom storage remains available only for exceptional ABI cases via
``--allow-custom-storage``; it is not the normal path for selecting a class type.
"""

from __future__ import annotations

from collections import Counter
from collections.abc import Mapping, Sequence
from pathlib import Path
from typing import Any

from .class_binding import (
    auto_this_structure,
    binding_agrees,
    ensure_function_class_namespace,
    ensure_ghidra_class,
    find_class_structure,
    resolve_class_binding,
)
from .config import Settings
from .paths import atomic_json, repo_relative
from .prototype_repair import _normalize_ghidra, classify_pair
from .source_index import source_functions

_SCHEMA = "wiz8.class-this-typing-v1"
# Soft conventions we may promote to thiscall for class-binding (stdcall is
# common PDB noise for methods). Explicit cdecl/fastcall are hard-disagree.
_THISCALL_SOFT = frozenset({"default", "unknown", "__stdcall", ""})


class StorageIdentityError(RuntimeError):
    """Raised inside a per-function transaction so the mutation is rolled back."""

    def __init__(self, row: Mapping[str, Any], before: Mapping[str, Any], after: Mapping[str, Any]):
        super().__init__("storage-changed")
        self.row = dict(row)
        self.before = dict(before)
        self.after = dict(after)


def _this_type_name(function: Any) -> str | None:
    params = list(function.getParameters())
    if not params:
        return None
    return str(params[0].getDataType())


def _normalize_storage(text: str | None) -> str | None:
    if text is None:
        return None
    return text.replace(" (auto)", "")


def _storage_snapshot(function: Any) -> dict[str, Any]:
    return_var = function.getReturn()
    return {
        "return_storage": _normalize_storage(
            str(return_var.getVariableStorage()) if return_var is not None else None
        ),
        "parameters": [
            {
                "ordinal": int(param.getOrdinal()),
                "name": str(param.getName()),
                "storage": _normalize_storage(str(param.getVariableStorage())),
            }
            for param in function.getParameters()
        ],
    }


def _storage_matches(before: Mapping[str, Any], after: Mapping[str, Any]) -> bool:
    if before.get("return_storage") != after.get("return_storage"):
        return False
    left = list(before.get("parameters") or [])
    right = list(after.get("parameters") or [])
    if len(left) != len(right):
        return False
    for a, b in zip(left, right, strict=True):
        if a.get("ordinal") != b.get("ordinal") or a.get("storage") != b.get("storage"):
            return False
    return True


def collect_this_typing_plan(
    repository: Path,
    program: Any,
    *,
    target: str = "WIZ8",
    addresses: Sequence[int] | None = None,
) -> dict[str, Any]:
    """Plan class-binding repairs for source ``__thiscall`` methods."""

    markers = {
        address: marker
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION"
        and marker.declaration is not None
        and marker.declaration.calling_convention == "__thiscall"
        and marker.declaration.owning_class
    }
    if addresses is not None:
        wanted = set(addresses)
        markers = {address: marker for address, marker in markers.items() if address in wanted}

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    rows: list[dict[str, Any]] = []
    counts: Counter[str] = Counter()
    missing_structures: Counter[str] = Counter()

    for address, marker in sorted(markers.items()):
        declaration = marker.declaration
        assert declaration is not None
        owning = str(declaration.owning_class)
        function = functions.getFunctionAt(space.getAddress(address))
        if function is None:
            action = "missing-function"
            binding: dict[str, Any] | None = None
            this_type = None
        else:
            binding = resolve_class_binding(program, owning)
            this_type = _this_type_name(function)
            ghidra_cc = _normalize_ghidra(function.getCallingConventionName())
            if function.hasCustomVariableStorage():
                action = "skip-custom-storage"
            elif (
                ghidra_cc not in _THISCALL_SOFT
                and ghidra_cc != "__thiscall"
                and classify_pair(ghidra_cc, "__thiscall") == "hard-disagree"
            ):
                action = "convention-hard-disagree"
            elif binding["status"] in {"missing-structure", "missing-class"}:
                action = "missing-structure"
                missing_structures[owning] += 1
            elif binding_agrees(binding, function) and function.getParentNamespace() is not None:
                # Parent must already be the GhidraClass; cheap name check.
                parent = function.getParentNamespace()
                if str(parent.getName(True)) == str(binding["ghidra_class"]):
                    action = "agree"
                else:
                    action = "bind-class-namespace"
            else:
                action = "bind-class-this"
        counts[action] += 1
        if action == "agree":
            continue
        row: dict[str, Any] = {
            "address": f"0x{address:08x}",
            "name": marker.name,
            "owning_class": owning,
            "ghidra_this": this_type,
            "structure_path": (binding or {}).get("structure_path"),
            "ghidra_class": (binding or {}).get("ghidra_class"),
            "action": action,
        }
        if action == "convention-hard-disagree" and function is not None:
            row["ghidra_convention"] = _normalize_ghidra(function.getCallingConventionName())
        rows.append(row)

    return {
        "schema": _SCHEMA,
        "target": target,
        "counts": dict(sorted(counts.items())),
        "actionable": counts["bind-class-this"] + counts["bind-class-namespace"],
        "missing_structures": [
            {"class": name, "methods": count} for name, count in missing_structures.most_common()
        ],
        "functions": rows,
    }


def apply_this_typing_row(
    program: Any,
    row: Mapping[str, Any],
    *,
    allow_custom_storage: bool = False,
) -> dict[str, Any]:
    """Apply one class-binding repair. Prefer dynamic auto ``this``."""

    space = program.getAddressFactory().getDefaultAddressSpace()
    functions = program.getFunctionManager()
    action = row.get("action")
    if action not in {"bind-class-this", "bind-class-namespace", "set-this-type"}:
        raise ValueError(f"unexpected this-typing action: {action}")
    address = int(row["address"], 0)
    owning = str(row["owning_class"])
    function = functions.getFunctionAt(space.getAddress(address))
    if function is None:
        return {**row, "error": "missing-function"}

    if function.hasCustomVariableStorage() and not allow_custom_storage:
        # Ordinary path must never silence custom-storage ABI by clearing it.
        return {
            **row,
            "error": "skip-custom-storage",
            "skipped": "skip-custom-storage",
        }

    current_cc = _normalize_ghidra(function.getCallingConventionName())
    if current_cc != "__thiscall":
        soft_ok = current_cc in _THISCALL_SOFT
        if not soft_ok and classify_pair(current_cc, "__thiscall") == "hard-disagree":
            return {
                **row,
                "error": "convention-hard-disagree",
                "ghidra_convention": current_cc,
                "skipped": "convention-hard-disagree",
            }

    try:
        ghidra_class = ensure_ghidra_class(program, owning)
    except RuntimeError as exc:
        message = str(exc)
        if "namespace collision" in message:
            return {**row, "error": "namespace-collision", "hint": message}
        raise
    structure = find_class_structure(program, ghidra_class)
    if structure is None:
        return {**row, "error": "missing-structure"}

    before = _storage_snapshot(function)
    namespace_changed = ensure_function_class_namespace(function, ghidra_class)

    # Ordinary path: dynamic storage + thiscall so auto this uses the bound Structure.
    if current_cc != "__thiscall":
        function.setCallingConvention("__thiscall")

    pointee = auto_this_structure(function)
    path = str(pointee.getPathName()) if pointee is not None else None
    expected = str(structure.getPathName())
    if path == expected:
        after = _storage_snapshot(function)
        # Ordinary path must not change parameter/return storage ABI.
        if not _storage_matches(before, after):
            raise StorageIdentityError(row, before, after)
        return {
            "address": row["address"],
            "name": row.get("name"),
            "owning_class": owning,
            "from": row.get("ghidra_this"),
            "to": path,
            "structure_path": expected,
            "namespace_changed": namespace_changed,
            "action": "bind-class-this",
        }

    if not allow_custom_storage:
        return {
            **row,
            "error": "auto-this-unbound",
            "structure_path": expected,
            "auto_this_path": path,
            "hint": "class Structure exists but auto this did not bind; check GhidraClass parent",
        }

    # Exceptional ABI path: explicit custom storage (opt-in only).
    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]
    from ghidra.program.model.symbol import SourceType  # type: ignore[import-not-found]

    function.setCustomVariableStorage(True)
    parameters = list(function.getParameters())
    if not parameters:
        raise StorageIdentityError(row, before, _storage_snapshot(function))
    pointer = PointerDataType(structure, program.getDataTypeManager())
    parameters[0].setDataType(pointer, SourceType.IMPORTED)
    if parameters[0].getName() != "this":
        parameters[0].setName("this", SourceType.IMPORTED)
    after = _storage_snapshot(function)
    if not _storage_matches(before, after):
        raise StorageIdentityError(row, before, after)
    return {
        "address": row["address"],
        "name": row.get("name"),
        "owning_class": owning,
        "from": row.get("ghidra_this"),
        "to": str(pointer),
        "structure_path": expected,
        "namespace_changed": namespace_changed,
        "action": "custom-storage-this",
    }


class _RowApplyError(Exception):
    def __init__(self, payload: Mapping[str, Any]):
        super().__init__(str(payload.get("error") or "apply-failed"))
        self.payload = dict(payload)


def apply_this_typing(
    program: Any,
    plan: Mapping[str, Any],
    *,
    allow_custom_storage: bool = False,
) -> dict[str, Any]:
    """Apply planned class bindings; each row is its own top-level transaction."""

    import pyghidra

    applied: list[dict[str, Any]] = []
    errors: list[dict[str, Any]] = []
    skipped: list[dict[str, Any]] = []
    for row in plan.get("functions", []):
        if row.get("action") not in {
            "bind-class-this",
            "bind-class-namespace",
            "set-this-type",
        }:
            continue
        try:
            with pyghidra.transaction(program, f"Bind class this at {row.get('address')}"):
                result = apply_this_typing_row(
                    program, row, allow_custom_storage=allow_custom_storage
                )
                if result.get("error"):
                    raise _RowApplyError(result)
            applied.append(result)
        except StorageIdentityError as exc:
            errors.append(
                {
                    **exc.row,
                    "error": "storage-changed",
                    "storage_before": exc.before,
                    "storage_after": exc.after,
                }
            )
        except _RowApplyError as exc:
            payload = exc.payload
            err = str(payload.get("error") or "")
            if err in {
                "auto-this-unbound",
                "requires-custom-storage",
                "skip-custom-storage",
                "convention-hard-disagree",
                "namespace-collision",
            }:
                skipped.append({**payload, "skipped": err})
            else:
                errors.append(payload)
        except Exception as exc:  # noqa: BLE001
            errors.append({**dict(row), "error": str(exc)})
    return {
        "applied": len(applied),
        "errors": errors,
        "skipped": skipped,
        "functions": applied,
    }


def run_class_this_typing(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    apply: bool = False,
    allow_custom_storage: bool = False,
    addresses: Sequence[int] | None = None,
) -> dict[str, Any]:
    """Report or apply class-namespace bindings for typed automatic ``this``."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_this_typing_plan(
            settings.repo_dir, program, target=target, addresses=addresses
        )
        out_dir = settings.build_dir / "class-this-typing"
        report_path = out_dir / "report.json"
        atomic_json(
            report_path,
            {
                **plan,
                "program": program_name,
                "apply": apply,
                "allow_custom_storage": allow_custom_storage,
            },
        )
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "allow_custom_storage": allow_custom_storage,
            "counts": plan["counts"],
            "actionable": plan["actionable"],
            "missing_structure_classes": len(plan["missing_structures"]),
            "report": repo_relative(report_path, settings.repo_dir),
            "sample": plan["functions"][:20],
            "missing_structures_sample": plan["missing_structures"][:20],
        }
        if not apply:
            return result

        applied = apply_this_typing(program, plan, allow_custom_storage=allow_custom_storage)
        dispose_sessions()
        program.save("Bind class namespaces for automatic this", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["skipped"] = len(applied.get("skipped") or [])
        result["sample"] = applied["functions"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = repo_relative(error_path, settings.repo_dir)
        if applied.get("skipped"):
            skip_path = out_dir / "skipped.json"
            atomic_json(skip_path, applied["skipped"])
            result["skipped_report"] = repo_relative(skip_path, settings.repo_dir)
        return result
