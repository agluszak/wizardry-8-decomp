"""Create typed callback FunctionDefinitions and apply them to known fields.

Curated families only: region/UI/dialog/monster callbacks already named in
recovered headers. Speculative ``code *`` discovery is out of scope.

Class field sites are owning-class identities resolved through
``class_binding`` (bound/root Structure). Absolute ``/…`` paths remain for
non-class SGP/GUI Structures. Legacy ``/wiz8/classes`` is migration-only.
"""

from __future__ import annotations

from collections.abc import Mapping
from typing import Any

from .class_binding import (
    find_class_structure,
    find_ghidra_class,
    legacy_enriched_structure,
    resolve_class_binding,
)
from .config import Settings
from .paths import atomic_json

_SCHEMA = "wiz8.callback-typing-v1"
_CATEGORY = "/wiz8/callbacks"

# (typedef name, return type path-or-builtin, ((param_name, type path-or-builtin), ...),
#  calling convention, field sites as (owning class OR absolute /path, field name))
# Class sites resolve via class_binding; absolute paths are non-class Structures.
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


def type_identity(data_type: Any) -> str:
    """Stable identity for DataType comparison (path preferred over bare name)."""

    if data_type is None:
        return ""
    path = getattr(data_type, "getPathName", None)
    if callable(path):
        text = str(path())
        if text:
            return text
    return str(data_type.getName()) if hasattr(data_type, "getName") else str(data_type)


def _unwrap_typedefs(data_type: Any) -> Any:
    current = data_type
    while current is not None and "TypeDef" in type(current).__name__:
        if not hasattr(current, "getBaseDataType"):
            break
        current = current.getBaseDataType()
    return current


def _pointee_data_type(data_type: Any) -> Any | None:
    current = _unwrap_typedefs(data_type)
    if current is None:
        return None
    is_pointer = getattr(current, "isPointer", None)
    if callable(is_pointer):
        if not is_pointer():
            return None
    elif "Pointer" not in type(current).__name__:
        return None
    if not hasattr(current, "getDataType"):
        return None
    return _unwrap_typedefs(current.getDataType())


def definition_matches_family(existing: Any, family: Mapping[str, Any], program: Any) -> bool:
    """True when an existing FunctionDefinition matches the curated family."""

    if existing is None or not hasattr(existing, "getArguments"):
        return False
    expected_return = _resolve_type(program, str(family["return"]))
    if expected_return is None:
        return False
    if type_identity(existing.getReturnType()) != type_identity(expected_return):
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
        if type_identity(arg.getDataType()) != type_identity(expected_type):
            return False
    convention = family.get("convention")
    if convention:
        if not hasattr(existing, "getCallingConvention"):
            return False
        current = existing.getCallingConvention()
        if current is None or str(current) != str(convention):
            return False
    return True


# Back-compat alias for callers/tests that still import the private name.
_definition_matches_family = definition_matches_family


def definition_matches_function(definition: Any, function: Any) -> bool:
    """True when ``definition`` matches ``function``'s formal signature contract.

    Compares return type, argument types/names (including explicit ``this`` when
    present on the definition), and calling convention. Used by vftable typing
    to reject stale FunctionDefinitions that only share a name/slot count.
    """

    if definition is None or function is None:
        return False
    if not hasattr(definition, "getArguments") or not hasattr(function, "getReturnType"):
        return False
    if type_identity(definition.getReturnType()) != type_identity(function.getReturnType()):
        return False

    def_args = list(definition.getArguments())
    # Formal signature: prefer parameters as stored (auto this included when
    # FunctionDefinition was built with formal=False).
    fn_params = list(function.getParameters())
    if len(def_args) != len(fn_params):
        return False
    for arg, param in zip(def_args, fn_params, strict=True):
        if type_identity(arg.getDataType()) != type_identity(param.getDataType()):
            return False
        arg_name = str(arg.getName()) if hasattr(arg, "getName") else ""
        param_name = str(param.getName()) if hasattr(param, "getName") else ""
        if arg_name and param_name and arg_name != param_name:
            return False

    if hasattr(definition, "getCallingConvention") and hasattr(
        function, "getCallingConventionName"
    ):
        def_cc = definition.getCallingConvention()
        fn_cc = function.getCallingConventionName()
        if def_cc is not None and fn_cc is not None and str(def_cc) != str(fn_cc):
            return False
    return True


def _resolve_field_structure(program: Any, site: str) -> tuple[Any | None, str | None, str | None]:
    """Resolve a field site to (structure, report_path, resolution).

    ``site`` is either an absolute DataType path (``/…``) or a source owning-class
    identity. Class sites prefer the bound Structure; legacy ``/wiz8/classes`` is
    only a migration fallback.
    """

    if site.startswith("/"):
        structure = program.getDataTypeManager().getDataType(site)
        if structure is None or not hasattr(structure, "getDefinedComponents"):
            return None, site, "absolute-missing"
        return structure, site, "absolute"

    binding = resolve_class_binding(program, site)
    ghidra_class = find_ghidra_class(program, site)
    if ghidra_class is not None:
        structure = find_class_structure(program, ghidra_class)
        if structure is not None and hasattr(structure, "getDefinedComponents"):
            path = str(structure.getPathName())
            return structure, path, "bound"

    legacy = legacy_enriched_structure(program, site)
    if legacy is not None and hasattr(legacy, "getDefinedComponents"):
        path = str(legacy.getPathName())
        return legacy, path, "legacy-enriched"

    report = binding.get("structure_path") or binding.get("legacy_enriched_path") or site
    return None, str(report) if report else site, binding.get("status") or "missing-structure"


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
    if existing is not None and definition_matches_family(existing, family, program):
        return existing

    manager.createCategory(CategoryPath(_CATEGORY))
    definition = _build_function_definition(program, family)
    return manager.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)


def _field_already_typed(component: Any, family: Mapping[str, Any], program: Any) -> bool:
    """True when the field already points at a matching family FunctionDefinition."""

    pointed = _pointee_data_type(component.getDataType())
    if pointed is None:
        return False
    expected_path = f"{_CATEGORY}/{family['name']}"
    if type_identity(pointed) == expected_path:
        return definition_matches_family(pointed, family, program)
    # Path may differ after REPLACE; still accept a contract match on the pointee.
    return definition_matches_family(pointed, family, program)


def collect_callback_typing_plan(program: Any) -> dict[str, Any]:
    """Plan FunctionDefinition creation and structure field replacements."""

    rows: list[dict[str, Any]] = []
    for family in _CALLBACK_FAMILIES:
        definition_path = f"{_CATEGORY}/{family['name']}"
        existing_def = program.getDataTypeManager().getDataType(definition_path)
        typedef_ok = existing_def is not None and definition_matches_family(
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
        for site, field_name in family["fields"]:
            structure, structure_path, resolution = _resolve_field_structure(program, site)
            if structure is None:
                rows.append(
                    {
                        "family": family["name"],
                        "structure": structure_path,
                        "field": field_name,
                        "site": site,
                        "resolution": resolution,
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
                        "site": site,
                        "resolution": resolution,
                        "action": "missing-field",
                    }
                )
                continue
            already = _field_already_typed(component, family, program)
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
                    "site": site,
                    "resolution": resolution,
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
        site = str(row.get("site") or row["structure"])
        structure, structure_path, _resolution = _resolve_field_structure(program, site)
        if structure is None:
            structure = program.getDataTypeManager().getDataType(row["structure"])
            structure_path = row["structure"]
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
