"""Create typed callback FunctionDefinitions and apply them to known fields.

Curated families only: region/UI/dialog/monster callbacks already named in
recovered headers. Speculative ``code *`` discovery is out of scope.
"""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

from .config import Settings
from .paths import atomic_json

_SCHEMA = "wiz8.callback-typing-v1"
_CATEGORY = "/wiz8/callbacks"

# Field sites are (class identity or absolute non-class path, field name).
# Wizardry classes resolve through GhidraClass → find_class_structure().
# Absolute SGP/GUI types such as /_GUI_BUTTON stay path lookups.
_CALLBACK_FAMILIES: tuple[dict[str, Any], ...] = (
    {
        "name": "W8RegionCallback",
        "return": "uchar",
        "convention": "__cdecl",
        "params": (("event", "InputAtom *"), ("region", "W8Region *")),
        "fields": (("W8Region", "callback"),),
    },
    {
        "name": "GUI_CALLBACK",
        "return": "void",
        "convention": "__cdecl",
        "params": (("button", "_GUI_BUTTON *"), ("reason", "int")),
        "fields": (
            ("/_GUI_BUTTON", "ClickCallback"),
            ("/_GUI_BUTTON", "MoveCallback"),
        ),
    },
    {
        "name": "W8DialogDestroyCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (("dialog", "W8DialogBase *"),),
        "fields": (("W8DialogBase", "m_destroy_callback"),),
    },
    {
        "name": "W8ControlCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (),
        "fields": (
            ("W8Widget", "m_primaryActivationCallback"),
            ("W8Widget", "m_leftButtonDownCallback"),
            ("W8Widget", "m_secondaryActivationCallback"),
            ("W8Widget", "m_rightButtonDownCallback"),
            ("W8Widget", "m_leftDoubleClickCallback"),
        ),
    },
    {
        "name": "CycleCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (("monster", "W8Monster *"),),
        "fields": (("W8Monster", "cycle_callback_230"),),
    },
    {
        "name": "ActivationCallback",
        "return": "bool",
        "convention": "__cdecl",
        "params": (("trigger", "Trigger *"),),
        "fields": (("Trigger", "activation_callback_360"),),
    },
    {
        "name": "MOUSE_CALLBACK",
        "return": "void",
        "convention": "__cdecl",
        "params": (("region", "_MOUSE_REGION *"), ("reason", "int")),
        "fields": (
            ("/_MOUSE_REGION", "MovementCallback"),
            ("/_MOUSE_REGION", "ButtonCallback"),
        ),
    },
    {
        "name": "MOUSEBLT_HOOK",
        "return": "void",
        "convention": "__cdecl",
        "params": (),
        "fields": (),  # applied via globals/parameters elsewhere; typedef only
    },
)


def _resolve_type(program: Any, spelling: str) -> Any | None:
    from .global_typing import resolve_data_type

    if spelling == "void":
        from ghidra.program.model.data import VoidDataType  # type: ignore[import-not-found]

        return VoidDataType()
    if spelling == "bool":
        from ghidra.program.model.data import BooleanDataType  # type: ignore[import-not-found]

        return BooleanDataType()
    if spelling == "uchar":
        from ghidra.program.model.data import UnsignedCharDataType  # type: ignore[import-not-found]

        return UnsignedCharDataType()
    if spelling == "int":
        from ghidra.program.model.data import IntegerDataType  # type: ignore[import-not-found]

        return IntegerDataType()
    return resolve_data_type(program, spelling)


def _resolve_field_owner(program: Any, owner: str) -> Any | None:
    """Bound Structure for a class identity, or absolute non-legacy path lookup."""

    from .class_binding import (
        find_class_structure,
        find_ghidra_class,
        is_legacy_enriched_path,
    )
    from .datatype_contracts import is_legacy_path

    if owner.startswith("/"):
        if is_legacy_path(owner):
            return None
        return program.getDataTypeManager().getDataType(owner)
    ghidra_class = find_ghidra_class(program, owner)
    if ghidra_class is None:
        return None
    structure = find_class_structure(program, ghidra_class)
    if structure is None:
        return None
    if is_legacy_enriched_path(str(structure.getPathName())):
        return None
    return structure


def _unwrap_function_definition(data_type: Any) -> Any | None:
    from ghidra.program.model.data import Pointer, TypeDef  # type: ignore[import-not-found]

    current = data_type
    while isinstance(current, TypeDef):
        current = current.getBaseDataType()
    if isinstance(current, Pointer):
        current = current.getDataType()
        while isinstance(current, TypeDef):
            current = current.getBaseDataType()
    if current is None:
        return None
    if hasattr(current, "getArguments") and hasattr(current, "getReturnType"):
        return current
    return None


def _definition_matches_family(existing: Any, family: dict[str, Any], program: Any) -> bool:
    """True when an existing FunctionDefinition matches the curated ABI contract.

    Parameter names are not ABI.
    """

    from .datatype_contracts import definition_contract_equals

    if existing is None or not hasattr(existing, "getArguments"):
        return False
    try:
        expected = _build_function_definition(program, family)
    except ValueError:
        return False
    return definition_contract_equals(existing, expected)


def _build_function_definition(program: Any, family: dict[str, Any]) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
    )

    definition = FunctionDefinitionDataType(CategoryPath(_CATEGORY), family["name"])
    return_type = _resolve_type(program, family["return"])
    if return_type is None:
        raise ValueError(f"unresolved return type for {family['name']}: {family['return']}")
    definition.setReturnType(return_type)
    params = []
    for name, spelling in family["params"]:
        data_type = _resolve_type(program, spelling)
        if data_type is None:
            raise ValueError(f"unresolved param type for {family['name']}.{name}: {spelling}")
        params.append(ParameterDefinitionImpl(name, data_type, None))
    if params:
        definition.setArguments(params)
    convention = family.get("convention")
    if convention:
        definition.setCallingConvention(convention)
    return definition


def _ensure_function_definition(program: Any, family: dict[str, Any]) -> Any:
    from ghidra.program.model.data import (  # type: ignore[import-not-found]
        CategoryPath,
        DataTypeConflictHandler,
    )

    manager = program.getDataTypeManager()
    path = f"{_CATEGORY}/{family['name']}"
    existing = manager.getDataType(path)
    if existing is not None and _definition_matches_family(existing, family, program):
        return existing

    manager.createCategory(CategoryPath(_CATEGORY))
    definition = _build_function_definition(program, family)
    return manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)


def _field_already_typed(component: Any, family: Mapping[str, Any], program: Any) -> bool:
    pointed = _unwrap_function_definition(component.getDataType())
    return _definition_matches_family(pointed, dict(family), program)


def collect_callback_typing_plan(program: Any) -> dict[str, Any]:
    """Plan FunctionDefinition creation and structure field replacements."""

    rows: list[dict[str, Any]] = []
    for family in _CALLBACK_FAMILIES:
        definition_path = f"{_CATEGORY}/{family['name']}"
        existing_def = program.getDataTypeManager().getDataType(definition_path)
        typedef_ok = existing_def is not None and _definition_matches_family(
            existing_def, family, program
        )
        typedef_disagrees = existing_def is not None and not typedef_ok

        if not family["fields"]:
            if typedef_ok:
                action = "agree"
            elif typedef_disagrees:
                action = "repair-typedef"
            else:
                action = "create-typedef"
            rows.append(
                {
                    "family": family["name"],
                    "structure": None,
                    "field": None,
                    "action": action,
                }
            )
            continue
        for owner, field_name in family["fields"]:
            structure = _resolve_field_owner(program, owner)
            if structure is None or not hasattr(structure, "getDefinedComponents"):
                rows.append(
                    {
                        "family": family["name"],
                        "owner": owner,
                        "structure": None,
                        "field": field_name,
                        "action": "missing-structure",
                    }
                )
                continue
            structure_path = str(structure.getPathName())
            component = None
            for candidate in structure.getDefinedComponents():
                if candidate.getFieldName() == field_name:
                    component = candidate
                    break
            if component is None:
                rows.append(
                    {
                        "family": family["name"],
                        "owner": owner,
                        "structure": structure_path,
                        "field": field_name,
                        "action": "missing-field",
                    }
                )
                continue
            already = _field_already_typed(component, family, program)
            compiler_backed = (
                _unwrap_function_definition(component.getDataType()) is not None and not already
            )
            current = component.getDataType()
            current_name = str(current.getName()) if hasattr(current, "getName") else str(current)
            if compiler_backed:
                action = "compiler-backed"
            elif typedef_disagrees and already:
                action = "repair-typedef"
            elif typedef_disagrees:
                action = "repair-and-set-field"
            elif already and typedef_ok:
                action = "agree"
            elif existing_def is None:
                action = "create-and-set-field"
            elif already:
                action = "agree"
            else:
                action = "set-field"
            rows.append(
                {
                    "family": family["name"],
                    "owner": owner,
                    "structure": structure_path,
                    "field": field_name,
                    "offset": component.getOffset(),
                    "current_type": current_name,
                    "action": action,
                }
            )
    actionable = sum(
        1
        for row in rows
        if row["action"]
        in {
            "create-and-set-field",
            "set-field",
            "create-typedef",
            "repair-typedef",
            "repair-and-set-field",
        }
    )
    return {
        "schema": _SCHEMA,
        "actionable": actionable,
        "fields": rows,
        "families": [family["name"] for family in _CALLBACK_FAMILIES],
    }


def apply_callback_typing(program: Any, plan: dict[str, Any]) -> dict[str, Any]:
    """Create callback typedefs and replace structure fields (one txn per row)."""

    from ghidra.program.model.data import PointerDataType  # type: ignore[import-not-found]

    from .ghidra.mutations import apply_rows

    by_family = {family["name"]: family for family in _CALLBACK_FAMILIES}
    created: set[str] = set()
    apply_actions = {
        "create-and-set-field",
        "set-field",
        "create-typedef",
        "repair-typedef",
        "repair-and-set-field",
    }

    def apply_one(program: Any, row: Mapping[str, Any]) -> dict[str, Any]:
        action = row.get("action")
        if action not in apply_actions:
            return {**dict(row), "error": f"unexpected-action:{action}"}
        family = by_family[str(row["family"])]
        path = f"{_CATEGORY}/{family['name']}"
        existed = program.getDataTypeManager().getDataType(path) is not None
        _ensure_function_definition(program, family)
        if not existed:
            created.add(family["name"])
        if action in {"create-typedef", "repair-typedef"}:
            return {
                "family": family["name"],
                "structure": None,
                "field": None,
                "type": family["name"],
                "action": action,
            }
        definition = program.getDataTypeManager().getDataType(path)
        pointer = PointerDataType(definition, program.getDataTypeManager())
        structure_path = row.get("structure")
        if not structure_path:
            return {**dict(row), "error": "missing-structure"}
        structure = program.getDataTypeManager().getDataType(str(structure_path))
        if structure is None:
            return {**dict(row), "error": "missing-structure"}
        structure.replaceAtOffset(
            int(row["offset"]),
            pointer,
            pointer.getLength(),
            str(row["field"]),
            None,
        )
        return {
            "family": family["name"],
            "owner": row.get("owner"),
            "structure": structure_path,
            "field": row["field"],
            "type": str(pointer.getName()),
            "action": action,
        }

    rows = [row for row in plan.get("fields", []) if row.get("action") in apply_actions]
    result = apply_rows(
        program,
        rows,
        apply_one,
        description="Type curated callback fields",
    )
    return {
        "applied": result["applied"],
        "errors": result["errors"],
        "fields": result["rows"],
        "created": sorted(created),
    }


def run_callback_typing(
    settings: Settings,
    *,
    program_name: str = "wiz8",
    apply: bool = False,
) -> dict[str, Any]:
    """Report or apply curated callback field typings."""

    import pyghidra

    from .ghidra.env import open_program
    from .ghidra.semantic import dispose_sessions

    with open_program(settings, program_name) as program:
        plan = collect_callback_typing_plan(program)
        out_dir = settings.build_dir / "callback-typing"
        report_path = out_dir / "report.json"
        atomic_json(report_path, {**plan, "program": program_name, "apply": apply})
        result: dict[str, Any] = {
            "schema": _SCHEMA,
            "program": program_name,
            "apply": apply,
            "actionable": plan["actionable"],
            "report": str(report_path.relative_to(settings.repo_dir)),
            "sample": plan["fields"][:20],
        }
        if not apply:
            return result

        applied = apply_callback_typing(program, plan)
        dispose_sessions()
        program.save("Type curated callback fields", pyghidra.task_monitor())
        result["applied"] = applied["applied"]
        result["apply_errors"] = len(applied["errors"])
        result["created"] = applied["created"]
        result["sample"] = applied["fields"][:20]
        if applied["errors"]:
            error_path = out_dir / "apply-errors.json"
            atomic_json(error_path, applied["errors"])
            result["apply_errors_report"] = str(error_path.relative_to(settings.repo_dir))
        return result
