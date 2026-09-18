"""The one established-source/evidence → ProgramDB synchronization path."""

from __future__ import annotations

import time
from typing import Any

from ..config import Settings
from ..paths import sha256_file
from ..source_index import address_bound_identities, target_for_program, write_source_index
from .resolve import hex_address, resolve_program_selector


class _HardSyncConflict(Exception):
    """Abort the native transaction without advertising a successful projection."""


def _has_function_overlap(conflicts: list[Any]) -> bool:
    return any(
        isinstance(row, dict) and row.get("error") == "function-overlap" for row in conflicts
    )


def _materialize_function(program: Any, address: int, name: str | None) -> dict[str, Any]:
    from ghidra.app.cmd.disassemble import DisassembleCommand
    from ghidra.app.cmd.function import CreateFunctionCmd

    space = program.getAddressFactory().getDefaultAddressSpace()
    entry = space.getAddress(address)
    manager = program.getFunctionManager()
    existing = manager.getFunctionAt(entry)
    if existing is not None:
        return {"address": hex_address(address), "action": "exists", "name": existing.getName(True)}
    containing = manager.getFunctionContaining(entry)
    if containing is not None:
        owner = int(containing.getEntryPoint().getOffset())
        if owner != address:
            return {
                "address": hex_address(address),
                "action": "conflict",
                "error": "function-overlap",
                "owner": hex_address(owner),
                "owner_name": containing.getName(True),
            }
    block = program.getMemory().getBlock(entry)
    if block is None or not block.isExecute():
        return {
            "address": hex_address(address),
            "action": "conflict",
            "error": "not-executable",
        }
    data = program.getListing().getDefinedDataContaining(entry)
    if data is not None and not str(data.getDataType().getName()).startswith("undefined"):
        return {
            "address": hex_address(address),
            "action": "conflict",
            "error": "data-overlap",
            "data_type": data.getDataType().getDisplayName(),
        }
    instruction = program.getListing().getInstructionAt(entry)
    if instruction is None:
        if not DisassembleCommand(entry, None, True).applyTo(program):
            return {
                "address": hex_address(address),
                "action": "conflict",
                "error": "disassemble-failed",
            }
        instruction = program.getListing().getInstructionAt(entry)
        if instruction is None:
            return {
                "address": hex_address(address),
                "action": "conflict",
                "error": "no-instruction",
            }
    if not CreateFunctionCmd(entry).applyTo(program):
        return {
            "address": hex_address(address),
            "action": "conflict",
            "error": "create-function-failed",
        }
    created = manager.getFunctionAt(entry)
    if created is None:
        return {
            "address": hex_address(address),
            "action": "conflict",
            "error": "create-function-missing",
        }
    if name and created.getName() in {"FUN_" + f"{address:08X}", f"FUN_{address:08x}"}:
        from ghidra.program.model.symbol import SourceType

        created.setName(name, SourceType.IMPORTED)
    return {"address": hex_address(address), "action": "created", "name": created.getName(True)}


def _apply_name_and_prototype(program: Any, identity: Any) -> dict[str, Any]:
    from ghidra.program.model.symbol import SourceType

    space = program.getAddressFactory().getDefaultAddressSpace()
    function = program.getFunctionManager().getFunctionAt(space.getAddress(identity.address))
    if function is None:
        return {"address": hex_address(identity.address), "action": "missing-function"}
    if identity.kind in {"library", "synthetic", "template", "global", "vtable"}:
        return {"address": hex_address(identity.address), "action": "skip-non-function"}
    changed = []
    desired = identity.name or identity.qualified_name.rsplit("::", 1)[-1]
    if desired and function.getName() != desired and not identity.folded:
        try:
            function.setName(desired, SourceType.IMPORTED)
            changed.append("name")
        except Exception as exc:  # noqa: BLE001
            return {
                "address": hex_address(identity.address),
                "action": "conflict",
                "error": f"rename-failed:{exc}",
            }
    convention = identity.calling_convention
    if convention in {"__cdecl", "__stdcall", "__fastcall", "__thiscall"}:
        current = str(function.getCallingConventionName() or "unknown")
        has_auto_this = any(
            bool(parameter.isAutoParameter()) for parameter in function.getParameters()
        )
        if current != convention and not (
            convention == "__thiscall" and current == "__cdecl" and not has_auto_this
        ):
            try:
                function.setCallingConvention(convention)
                function.setSignatureSource(SourceType.ANALYSIS)
                changed.append("convention")
            except Exception as exc:  # noqa: BLE001
                return {
                    "address": hex_address(identity.address),
                    "action": "conflict",
                    "error": f"convention-failed:{exc}",
                }
    signature = identity.source_signature
    if signature or identity.parameter_types or identity.return_type:
        applied = _apply_signature(program, function, identity)
        if applied.get("error"):
            return {
                "address": hex_address(identity.address),
                "action": "unresolved-signature",
                "error": applied["error"],
                "signature": signature,
            }
        if applied.get("applied"):
            changed.append("signature")
    return {
        "address": hex_address(identity.address),
        "action": "updated" if changed else "agree",
        "changed": changed,
        "name": function.getName(True),
    }


def _type_key(data_type: Any) -> str:
    if data_type is None:
        return ""
    getter = getattr(data_type, "getPathName", None)
    if callable(getter):
        return str(getter())
    return str(data_type)


def _parameter_names_from_signature(signature: str | None, count: int) -> list[str]:
    names = [f"param_{index}" for index in range(count)]
    if not signature or "(" not in signature or ")" not in signature or count <= 0:
        return names
    inner = signature[signature.find("(") + 1 : signature.rfind(")")]
    if not inner.strip() or inner.strip() == "void":
        return names
    parts: list[str] = []
    depth = 0
    current: list[str] = []
    for char in inner:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        if char == "," and depth == 0:
            parts.append("".join(current).strip())
            current = []
            continue
        current.append(char)
    if "".join(current).strip():
        parts.append("".join(current).strip())
    reserved = {"int", "void", "char", "short", "long", "float", "double", "bool", "unsigned"}
    for index, part in enumerate(parts[:count]):
        token = part.replace("*", " ").split()[-1]
        if token.isidentifier() and token not in reserved:
            names[index] = token
    return names


def _explicit_parameter_types(identity: Any) -> tuple[str, ...]:
    types = list(identity.parameter_types or ())
    if not identity.has_this or not types:
        return tuple(types)
    owner = str(identity.owning_class or "").replace(" ", "")
    first = types[0].replace(" ", "")
    if owner and (first == f"{owner}*" or first.endswith(f"::{owner}*")):
        types = types[1:]
    return tuple(types)


def _resolved_structured_signature(program: Any, identity: Any) -> dict[str, Any]:
    from ghidra.program.model.listing import ParameterImpl

    from ..global_typing import resolve_data_type

    return_spelling = identity.return_type or "void"
    if return_spelling == "void":
        from ghidra.program.model.data import VoidDataType

        return_type = VoidDataType()
    else:
        return_type = resolve_data_type(program, return_spelling)
    if return_type is None:
        return {"error": f"unresolved-return:{return_spelling}"}
    parameters = []
    spellings = _explicit_parameter_types(identity)
    names = _parameter_names_from_signature(identity.source_signature, len(spellings))
    for index, spelling in enumerate(spellings):
        data_type = resolve_data_type(program, spelling)
        if data_type is None:
            return {"error": f"unresolved-parameter:{spelling}"}
        parameters.append(ParameterImpl(names[index], data_type, program))
    return {"return_type": return_type, "parameters": parameters}


def _is_placeholder_parameter_name(name: str) -> bool:
    return name.startswith("param_") and name[6:].isdigit()


def _parameter_names_agree(current: str, desired: str) -> bool:
    if current == desired:
        return True
    return _is_placeholder_parameter_name(desired)


def _return_types_agree(current: Any, desired: Any) -> bool:
    if _type_key(current) == _type_key(desired):
        return True
    if current is None or desired is None:
        return False
    if "Pointer" in type(current).__name__ or (
        callable(getattr(current, "isPointer", None)) and current.isPointer()
    ):
        pointee = current.getDataType() if hasattr(current, "getDataType") else None
        return _type_key(pointee) == _type_key(desired)
    return False


def _unused_source_parameters_omitted(existing: list[Any], parameters: list[Any]) -> bool:
    """True when Ghidra kept a prefix or suffix of the source parameters.

    Unused thiscall/event integers are often dropped from ProgramDB even after
    an IMPORTED apply; rewriting them every sync does not stick.
    """

    if len(existing) >= len(parameters):
        return False
    if all(
        _parameter_names_agree(current.getName(), desired.getName())
        and _type_key(current.getDataType()) == _type_key(desired.getDataType())
        for current, desired in zip(existing, parameters, strict=False)
    ) and all(
        _is_placeholder_parameter_name(parameter.getName())
        for parameter in parameters[len(existing) :]
    ):
        return True
    omitted = len(parameters) - len(existing)
    leading = parameters[:omitted]
    trailing = parameters[omitted:]
    if not all(_is_placeholder_parameter_name(parameter.getName()) for parameter in leading):
        return False
    return all(
        _parameter_names_agree(current.getName(), desired.getName())
        and _type_key(current.getDataType()) == _type_key(desired.getDataType())
        for current, desired in zip(existing, trailing, strict=True)
    )


def _stored_signature_matches(function: Any, return_type: Any, parameters: list[Any]) -> bool:
    existing = [
        parameter for parameter in function.getParameters() if not bool(parameter.isAutoParameter())
    ]
    if not _return_types_agree(function.getReturnType(), return_type):
        return False
    if len(existing) > len(parameters):
        # Source declared fewer parameters than ProgramDB already stores. Do not
        # strip recovered parameters every sync when Ghidra will restore them.
        return True
    if len(existing) == len(parameters) and all(
        _parameter_names_agree(current.getName(), desired.getName())
        and _type_key(current.getDataType()) == _type_key(desired.getDataType())
        for current, desired in zip(existing, parameters, strict=True)
    ):
        return True
    return _unused_source_parameters_omitted(existing, parameters)


def _apply_signature(program: Any, function: Any, identity: Any) -> dict[str, Any]:
    structured = _resolved_structured_signature(program, identity)
    if not structured.get("error"):
        if _stored_signature_matches(function, structured["return_type"], structured["parameters"]):
            return {"applied": False}
        applied = _apply_structured_signature(program, function, structured)
        if not applied.get("error"):
            return applied
        structured = applied
    source = function.getSignatureSource()
    source_name = getattr(source, "name", None)
    if callable(source_name):
        try:
            source_name = source_name()
        except TypeError:
            source_name = None
    if str(source_name or source) == "IMPORTED":
        return {"applied": False}
    parsed = _apply_parsed_signature(program, function, identity.source_signature)
    if not parsed.get("error"):
        return parsed
    return {
        "error": "; ".join(part for part in (structured.get("error"), parsed.get("error")) if part)
    }


def _apply_parsed_signature(program: Any, function: Any, signature: str | None) -> dict[str, Any]:
    if not signature:
        return {"error": "no-source-signature"}
    try:
        from ghidra.app.util.parser import FunctionSignatureParser
        from ghidra.program.model.listing import Function, ParameterImpl
        from ghidra.program.model.symbol import SourceType
    except Exception as exc:  # noqa: BLE001
        return {"error": f"parser-unavailable:{exc}"}
    try:
        parser = FunctionSignatureParser(program.getDataTypeManager(), None)
        parsed = parser.parse(function.getSignature(), signature)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"parse-failed:{exc}"}
    if parsed is None:
        return {"error": "parse-failed"}
    current = function.getPrototypeString(False, False)
    desired = parsed.getPrototypeString() if hasattr(parsed, "getPrototypeString") else ""
    if desired and current.replace(" ", "") == desired.replace(" ", ""):
        return {"applied": False}
    try:
        function.setReturnType(parsed.getReturnType(), SourceType.IMPORTED)
        parameters = [
            ParameterImpl(
                argument.getName() or f"param_{index}",
                argument.getDataType(),
                program,
            )
            for index, argument in enumerate(parsed.getArguments())
        ]
        function.replaceParameters(
            Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
            True,
            SourceType.IMPORTED,
            *parameters,
        )
        function.setVarArgs(bool(parsed.hasVarArgs()))
        function.setSignatureSource(SourceType.IMPORTED)
        return {"applied": True}
    except Exception as exc:  # noqa: BLE001
        return {"error": f"apply-failed:{exc}"}


def _apply_structured_signature(
    program: Any, function: Any, resolved: dict[str, Any]
) -> dict[str, Any]:
    from ghidra.program.model.listing import Function
    from ghidra.program.model.symbol import SourceType

    return_type = resolved["return_type"]
    parameters = resolved["parameters"]
    current = function.getPrototypeString(False, False).replace(" ", "")
    try:
        function.setReturnType(return_type, SourceType.IMPORTED)
        function.replaceParameters(
            Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
            True,
            SourceType.IMPORTED,
            *parameters,
        )
        function.setSignatureSource(SourceType.IMPORTED)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"apply-failed:{exc}"}
    updated = function.getPrototypeString(False, False).replace(" ", "")
    return {"applied": updated != current}


def _step_summary(result: dict[str, Any]) -> dict[str, Any]:
    return {
        key: result.get(key)
        for key in (
            "applied",
            "actionable",
            "apply_errors",
            "errors",
            "counts",
            "ok",
        )
        if key in result
    }


def _hard_errors(result: dict[str, Any]) -> list[Any]:
    errors = list(result.get("errors") or result.get("apply_errors") or [])
    if isinstance(result.get("apply_errors"), int) and result["apply_errors"] and not errors:
        return [f"{result.get('schema', 'step')} apply_errors={result['apply_errors']}"]
    return errors


def synchronize(
    settings: Settings,
    *,
    program_selector: str = "wiz8",
    import_source: bool = False,
) -> dict[str, Any]:
    """Project every supported established source/evidence fact into ProgramDB."""

    import pyghidra

    from .env import open_program
    from .workspace import record_source_projection

    program_name = resolve_program_selector(settings, program_selector)
    target = target_for_program(settings.repo_dir, program_name)
    index = write_source_index(settings)
    identities = address_bound_identities(settings.repo_dir, target)
    steps: list[dict[str, Any]] = []
    conflicts: list[dict[str, Any]] = []
    created: list[dict[str, Any]] = []
    signatures: list[dict[str, Any]] = []

    primaries: list[Any] = []
    for bound in identities.values():
        primary = next(
            (
                item
                for item in bound
                if item.kind in {"definition", "declaration"} and not item.identity_alias
            ),
            bound[0],
        )
        if primary.kind in {"definition", "declaration"}:
            primaries.append(primary)

    saved = False
    aborted = False
    with open_program(settings, program_selector) as program:
        domain = program.getDomainFile()
        if domain is not None and bool(domain.isReadOnly()):
            raise RuntimeError(
                f"Ghidra program {program_name} is read-only; cannot synchronize ProgramDB"
            )
        try:
            with pyghidra.transaction(program, "Synchronize established source facts"):
                for primary in primaries:
                    materialized = _materialize_function(program, primary.address, primary.name)
                    if materialized.get("action") == "created":
                        created.append(materialized)
                    elif materialized.get("action") == "conflict":
                        conflicts.append(materialized)

                from ..callback_typing import apply_callback_typing, collect_callback_typing_plan
                from ..class_structure_projection import (
                    apply_structure_projection,
                    collect_structure_projection_plan,
                )
                from ..class_this_typing import apply_this_typing, collect_this_typing_plan
                from ..cosmic_forge_globals import (
                    apply_cosmic_forge_globals,
                    collect_cosmic_forge_plan,
                )
                from ..function_attributes import (
                    apply_function_attributes,
                    collect_function_attribute_plan,
                )
                from ..global_typing import apply_global_typing, collect_global_typing_plan
                from ..surrender_iat_typing import (
                    apply_surrender_iat_typing,
                    collect_surrender_iat_plan,
                )
                from ..type_graph_projection import (
                    apply_type_graph_projection,
                    collect_type_graph_plan,
                )
                from ..vbtable_typing import apply_vbtable_typing, collect_vbtable_typing_plan
                from ..vftable_typing import apply_vftable_typing, collect_vftable_typing_plan

                structure_plan = collect_structure_projection_plan(
                    settings.repo_dir, program, target=target
                )
                structures = apply_structure_projection(program, structure_plan)
                steps.append({"step": "class-structures", "result": _step_summary(structures)})
                conflicts.extend(_hard_errors(structures))

                type_plan = collect_type_graph_plan(settings.repo_dir, program, target=target)
                types = apply_type_graph_projection(program, type_plan)
                steps.append({"step": "type-graph", "result": _step_summary(types)})
                conflicts.extend(_hard_errors(types))

                for primary in primaries:
                    applied = _apply_name_and_prototype(program, primary)
                    signatures.append(applied)
                    if applied.get("action") == "conflict":
                        conflicts.append(applied)

                this_plan = collect_this_typing_plan(settings.repo_dir, program, target=target)
                this_typing = apply_this_typing(program, this_plan)
                steps.append({"step": "class-this", "result": _step_summary(this_typing)})
                conflicts.extend(this_typing.get("errors") or [])

                vftable_plan = collect_vftable_typing_plan(
                    settings.repo_dir, program, target=target
                )
                vftables = apply_vftable_typing(program, vftable_plan)
                steps.append({"step": "vftables", "result": _step_summary(vftables)})
                conflicts.extend(_hard_errors(vftables))

                try:
                    vb_plan = collect_vbtable_typing_plan(settings.repo_dir, program, target=target)
                    vbtables = apply_vbtable_typing(program, vb_plan)
                    steps.append({"step": "vbtables", "result": _step_summary(vbtables)})
                    conflicts.extend(_hard_errors(vbtables))
                except Exception as exc:  # noqa: BLE001
                    steps.append({"step": "vbtables", "result": {"error": str(exc)}})

                iat_plan = collect_surrender_iat_plan(settings.repo_dir, program)
                iat = apply_surrender_iat_typing(program, iat_plan)
                steps.append({"step": "surrender-iat", "result": _step_summary(iat)})
                conflicts.extend(_hard_errors(iat))

                global_plan = collect_global_typing_plan(settings.repo_dir, program, target=target)
                globals_ = apply_global_typing(program, global_plan)
                steps.append({"step": "globals", "result": _step_summary(globals_)})
                conflicts.extend(_hard_errors(globals_))

                callback_plan = collect_callback_typing_plan(program)
                callbacks = apply_callback_typing(program, callback_plan)
                steps.append({"step": "callbacks", "result": _step_summary(callbacks)})
                conflicts.extend(_hard_errors(callbacks))

                cosmic_plan = collect_cosmic_forge_plan(settings.repo_dir, program)
                cosmic = apply_cosmic_forge_globals(program, cosmic_plan)
                steps.append({"step": "cosmic-forge", "result": _step_summary(cosmic)})
                conflicts.extend(_hard_errors(cosmic))

                attribute_plan = collect_function_attribute_plan(
                    settings.repo_dir, program, target=target
                )
                attributes = apply_function_attributes(program, attribute_plan, apply_thunks=False)
                steps.append({"step": "attributes", "result": _step_summary(attributes)})
                conflicts.extend(_hard_errors(attributes))

                if import_source:
                    from .reccmp_import import import_reccmp_source

                    imported = import_reccmp_source(settings, program_selector)
                    steps.append({"step": "reccmp-import", "result": imported})
                if _has_function_overlap(conflicts):
                    raise _HardSyncConflict()
        except _HardSyncConflict:
            saved = False
        else:
            from .env import save_program

            reported_mutations = bool(created) or any(
                row.get("action") == "updated" for row in signatures
            )
            if reported_mutations and not program.isChanged():
                aborted = True
            else:
                save_program(program, "Synchronize established source facts")
                index_path = settings.repo_dir / "build/source-index.json"
                record_source_projection(
                    settings,
                    program_name,
                    {
                        "source_index_sha256": sha256_file(index_path)
                        if index_path.is_file()
                        else None,
                        "applied_ns": time.time_ns(),
                        "target": target,
                    },
                )
                saved = True

    return {
        "schema": "wiz8.ghidra-sync",
        "program": program_name,
        "source_index": index,
        "created": created,
        "signature_updates": sum(1 for row in signatures if row.get("action") == "updated"),
        "signature_agreements": sum(1 for row in signatures if row.get("action") == "agree"),
        "unresolved_signatures": [
            row for row in signatures if row.get("action") == "unresolved-signature"
        ][:20],
        "conflicts": conflicts[:50],
        "steps": steps,
        "ok": saved and not aborted and not _has_function_overlap(conflicts),
        "provenance_advanced": saved,
        "transaction_aborted": aborted,
    }
