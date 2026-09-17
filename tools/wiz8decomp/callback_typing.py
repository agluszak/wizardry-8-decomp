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

# (typedef name, return type path-or-builtin, ((param_name, type path-or-builtin), ...),
#  calling convention, field sites as (structure path, field name))
# Field sites mutate only /wiz8/classes projections (or root SGP/GUI types that
# are not class-projection targets). Root/PDB copies of Wizardry classes stay
# untouched so projection comparisons remain meaningful.
_CALLBACK_FAMILIES: tuple[dict[str, Any], ...] = (
    {
        "name": "W8RegionCallback",
        "return": "uchar",
        "convention": "__cdecl",
        "params": (("event", "InputAtom *"), ("region", "W8Region *")),
        "fields": (("/wiz8/classes/W8Region", "callback"),),
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
        # Prefer projected class; missing-structure when not yet promoted.
        "fields": (("/wiz8/classes/W8DialogBase", "m_destroy_callback"),),
    },
    {
        "name": "W8ControlCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (),
        "fields": (
            ("/wiz8/classes/W8Widget", "m_primaryActivationCallback"),
            ("/wiz8/classes/W8Widget", "m_leftButtonDownCallback"),
            ("/wiz8/classes/W8Widget", "m_secondaryActivationCallback"),
            ("/wiz8/classes/W8Widget", "m_rightButtonDownCallback"),
            ("/wiz8/classes/W8Widget", "m_leftDoubleClickCallback"),
        ),
    },
    {
        "name": "CycleCallback",
        "return": "void",
        "convention": "__cdecl",
        "params": (("monster", "W8Monster *"),),
        "fields": (("/wiz8/classes/W8Monster", "cycle_callback_230"),),
    },
    {
        "name": "ActivationCallback",
        "return": "bool",
        "convention": "__cdecl",
        "params": (("trigger", "Trigger *"),),
        "fields": (("/wiz8/classes/Trigger", "activation_callback_360"),),
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


def _type_identity(data_type: Any) -> str:
    if data_type is None:
        return ""
    path = getattr(data_type, "getPathName", None)
    if callable(path):
        text = str(path())
        if text:
            return text
    return str(data_type.getName()) if hasattr(data_type, "getName") else str(data_type)


def _definition_matches_family(existing: Any, family: dict[str, Any], program: Any) -> bool:
    """True when an existing FunctionDefinition matches the curated family."""

    if existing is None or not hasattr(existing, "getArguments"):
        return False
    expected_return = _resolve_type(program, family["return"])
    if expected_return is None:
        return False
    if _type_identity(existing.getReturnType()) != _type_identity(expected_return):
        return False
    existing_args = list(existing.getArguments())
    expected_params = list(family["params"])
    if len(existing_args) != len(expected_params):
        return False
    for arg, (name, spelling) in zip(existing_args, expected_params, strict=True):
        if str(arg.getName()) != name:
            return False
        expected_type = _resolve_type(program, spelling)
        if expected_type is None:
            return False
        if _type_identity(arg.getDataType()) != _type_identity(expected_type):
            return False
    convention = family.get("convention")
    if convention:
        if not hasattr(existing, "getCallingConvention"):
            return False
        current = existing.getCallingConvention()
        if current is None or str(current) != str(convention):
            return False
    return True


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


def _field_already_typed(component: Any, family_name: str) -> bool:
    from ghidra.program.model.data import Pointer, TypeDef  # type: ignore[import-not-found]

    current = component.getDataType()
    while isinstance(current, TypeDef):
        current = current.getBaseDataType()
    if isinstance(current, Pointer):
        pointed = current.getDataType()
        if pointed is not None and family_name in str(pointed.getName()):
            return True
    return False


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
        for structure_path, field_name in family["fields"]:
            structure = program.getDataTypeManager().getDataType(structure_path)
            if structure is None or not hasattr(structure, "getDefinedComponents"):
                rows.append(
                    {
                        "family": family["name"],
                        "structure": structure_path,
                        "field": field_name,
                        "action": "missing-structure",
                    }
                )
                continue
            component = None
            for candidate in structure.getDefinedComponents():
                if candidate.getFieldName() == field_name:
                    component = candidate
                    break
            if component is None:
                rows.append(
                    {
                        "family": family["name"],
                        "structure": structure_path,
                        "field": field_name,
                        "action": "missing-field",
                    }
                )
                continue
            already = _field_already_typed(component, family["name"])
            current = component.getDataType()
            current_name = str(current.getName()) if hasattr(current, "getName") else str(current)
            if typedef_disagrees and already:
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
        structure = program.getDataTypeManager().getDataType(row["structure"])
        structure.replaceAtOffset(
            int(row["offset"]),
            pointer,
            pointer.getLength(),
            str(row["field"]),
            None,
        )
        return {
            "family": family["name"],
            "structure": row["structure"],
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
