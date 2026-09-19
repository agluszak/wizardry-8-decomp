"""The one established-source/evidence → ProgramDB synchronization path."""

from __future__ import annotations

import time
from typing import Any

from ..config import Settings
from ..paths import sha256_file
from ..source_index import address_bound_identities, target_for_program, write_source_index
from .mutations import auto_parameters
from .resolve import hex_address, resolve_program_selector


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


def _thunked_local_target(function: Any) -> Any | None:
    """The local function a thunk forwards to, or None for ordinary bodies.

    Ghidra propagates a signature written to a thunk onto its target, so a
    thunk's own ABI is derived rather than authored: projecting it would fight
    the target's own row.
    """

    is_thunk = getattr(function, "isThunk", None)
    if not callable(is_thunk) or not bool(is_thunk()):
        return None
    thunked = function.getThunkedFunction(True)
    if thunked is None or bool(thunked.isExternal()):
        return None
    return thunked


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
    if _thunked_local_target(function) is not None:
        # Only the name is projected; the target's own row owns the ABI.
        return {
            "address": hex_address(identity.address),
            "action": "updated" if changed else "agree",
            "changed": changed,
            "name": function.getName(True),
            "skipped": "thunk-derived-signature",
        }
    convention = identity.calling_convention
    if convention in {"__cdecl", "__stdcall", "__fastcall", "__thiscall"}:
        current = str(function.getCallingConventionName() or "unknown")
        if current != convention:
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
    """Source-declared explicit parameters. Implicit this is not inferred from type spelling."""

    return tuple(identity.parameter_types or ())


def _is_placeholder_parameter_name(name: str) -> bool:
    return name.startswith("param_") and name[6:].isdigit()


def _parameter_names_agree(current: str, desired: str) -> bool:
    """Names are not ABI; a recovered placeholder defers to any stored spelling."""

    return current == desired or _is_placeholder_parameter_name(desired)


def _signature_source(function: Any) -> str:
    source = function.getSignatureSource()
    name = getattr(source, "name", None)
    if callable(name):
        try:
            name = name()
        except TypeError:  # pragma: no cover - defensive for Java enum proxies
            name = None
    return str(name or source)


def _stored_signature_matches(function: Any, resolved: dict[str, Any], identity: Any) -> bool:
    """Normalized ABI equality between ProgramDB and the recovered prototype.

    Agreement is exact for everything the source prototype declares: return
    type, explicit parameter count, parameter types, variadic shape and calling
    convention. The single enumerated exception is Ghidra's synthetic
    ``__thiscall`` auto-``this``, which is not a source parameter and is
    ignored. Agreement is never inferred from a stored prototype that carries
    *more* parameters than the source, and ``T *`` is not agreement for ``T``.
    """

    # Ghidra lowers an aggregate return to a pointer plus an automatic
    # return-storage parameter. Its formal signature retains the C++ value
    # return that source declarations express.
    if _type_key(function.getSignature().getReturnType()) != _type_key(resolved["return_type"]):
        return False
    existing = [
        parameter for parameter in function.getParameters() if not bool(parameter.isAutoParameter())
    ]
    desired = resolved["parameters"]
    if len(existing) != len(desired):
        return False
    if bool(function.hasVarArgs()) != bool(getattr(identity, "is_variadic", False)):
        return False
    wanted_cc = identity.calling_convention or ("__thiscall" if identity.has_this else None)
    if wanted_cc and str(function.getCallingConventionName() or "unknown") != wanted_cc:
        return False
    return all(
        _parameter_names_agree(current.getName(), wanted.getName())
        and _type_key(current.getDataType()) == _type_key(wanted.getDataType())
        for current, wanted in zip(existing, desired, strict=True)
    )


def _apply_signature(program: Any, function: Any, identity: Any) -> dict[str, Any]:
    """Project the recovered prototype, structured data types first.

    Comparing the ProgramDB-resolved structured form with normalized ABI
    equality is what makes a repeated ``ghidra sync`` a real no-op instead of a
    source-text parser round-trip. The parser stays the fallback for spellings
    the structured resolver cannot place, and an existing IMPORTED prototype is
    left untouched because ``--import-source`` owns that projection.
    """

    structured = _resolved_structured_signature(program, identity)
    if not structured.get("error"):
        if _stored_signature_matches(function, structured, identity):
            return {"applied": False}
        applied = _apply_structured_signature(program, function, structured, identity)
        if not applied.get("error"):
            # A write Ghidra refused to keep is a projection failure, not an
            # update: report it so it blocks provenance instead of churning.
            if not _stored_signature_matches(function, structured, identity):
                return {"error": "structured-apply-not-stored"}
            return applied
        structured = applied
    if _signature_source(function) == "IMPORTED":
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


def _resolved_structured_signature(program: Any, identity: Any) -> dict[str, Any]:
    """Resolve the recovered prototype into ProgramDB data types.

    A returned ``error`` is a hard resolve failure (an unknown spelling); the
    caller then falls back to the source-text parser.
    """

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


def _apply_structured_signature(
    program: Any, function: Any, resolved: dict[str, Any], identity: Any
) -> dict[str, Any]:
    from ghidra.program.model.listing import Function
    from ghidra.program.model.symbol import SourceType

    wanted_cc = identity.calling_convention or ("__thiscall" if identity.has_this else None)
    # Ghidra silently ignores a parameter replacement that omits the function's
    # existing auto parameters (thiscall ``this``, structure-return pointers),
    # so carry them over and append the recovered explicit parameters.
    try:
        function.setReturnType(resolved["return_type"], SourceType.IMPORTED)
        function.replaceParameters(
            Function.FunctionUpdateType.DYNAMIC_STORAGE_ALL_PARAMS,
            True,
            SourceType.IMPORTED,
            *auto_parameters(function),
            *resolved["parameters"],
        )
        if wanted_cc:
            function.setCallingConvention(wanted_cc)
        function.setVarArgs(bool(getattr(identity, "is_variadic", False)))
        function.setSignatureSource(SourceType.IMPORTED)
    except Exception as exc:  # noqa: BLE001
        return {"error": f"apply-failed:{exc}"}
    return {"applied": True}


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
        errors = [f"{result.get('schema', 'step')} apply_errors={result['apply_errors']}"]
    if result.get("error"):
        errors = [result["error"], *errors]
    return errors


def _projection_complete(
    conflicts: list[Any],
    unresolved_signatures: list[Any],
    steps: list[dict[str, Any]],
) -> bool:
    """True when every supported established fact was applied or explicitly skipped."""

    if conflicts or unresolved_signatures:
        return False
    return not any(_hard_errors(step.get("result") or {}) for step in steps)


def _record_step(
    steps: list[dict[str, Any]],
    conflicts: list[dict[str, Any]],
    name: str,
    result: dict[str, Any],
) -> None:
    steps.append({"step": name, "result": _step_summary(result)})
    for row in _hard_errors(result):
        if isinstance(row, dict):
            conflicts.append(row)
        else:
            conflicts.append({"step": name, "error": str(row)})


def synchronize(
    settings: Settings,
    *,
    program_selector: str = "wiz8",
    import_source: bool = False,
) -> dict[str, Any]:
    """Project every supported established source/evidence fact into ProgramDB."""

    from .env import open_program, save_program
    from .mutations import RowApplyError, program_transaction
    from .reccmp_import import import_reccmp_source
    from .workspace import (
        compiler_import_identity,
        record_source_projection,
        recorded_source_projection,
    )

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

    if import_source:
        imported = import_reccmp_source(settings, program_selector)
        steps.append({"step": "reccmp-import", "result": imported})

    saved = False
    with open_program(settings, program_selector) as program:
        domain = program.getDomainFile()
        if domain is not None and bool(domain.isReadOnly()):
            raise RuntimeError(
                f"Ghidra program {program_name} is read-only; cannot synchronize ProgramDB"
            )

        from ..callback_typing import apply_callback_typing, collect_callback_typing_plan
        from ..class_structure_projection import (
            apply_structure_projection,
            collect_structure_projection_plan,
        )
        from ..class_this_typing import apply_this_typing, collect_this_typing_plan
        from ..cosmic_forge_globals import apply_cosmic_forge_globals, collect_cosmic_forge_plan
        from ..function_attributes import (
            apply_function_attributes,
            collect_function_attribute_plan,
        )
        from ..global_typing import apply_global_typing, collect_global_typing_plan
        from ..prototype_repair import apply_source_conventions, collect_source_convention_plan
        from ..surrender_iat_typing import apply_surrender_iat_typing, collect_surrender_iat_plan
        from ..type_graph_projection import apply_type_graph_projection, collect_type_graph_plan
        from ..vbtable_typing import apply_vbtable_typing, collect_vbtable_typing_plan
        from ..vftable_typing import apply_vftable_typing, collect_vftable_typing_plan

        for primary in primaries:
            try:
                with program_transaction(program, f"Materialize {hex_address(primary.address)}"):
                    materialized = _materialize_function(program, primary.address, primary.name)
                    if materialized.get("action") == "conflict":
                        raise RowApplyError(materialized)
                if materialized.get("action") == "created":
                    created.append(materialized)
            except RowApplyError as exc:
                conflicts.append(exc.payload)

        convention_plan = collect_source_convention_plan(settings.repo_dir, program, target=target)
        _record_step(
            steps, conflicts, "conventions", apply_source_conventions(program, convention_plan)
        )

        structure_plan = collect_structure_projection_plan(
            settings.repo_dir, program, target=target
        )
        _record_step(
            steps,
            conflicts,
            "class-structures",
            apply_structure_projection(program, structure_plan),
        )

        type_plan = collect_type_graph_plan(settings.repo_dir, program, target=target)
        _record_step(
            steps, conflicts, "type-graph", apply_type_graph_projection(program, type_plan)
        )

        def project_prototypes() -> None:
            """Project each source-bound name, convention and prototype.

            Runs after the structure/vftable/vbtable passes: those retype
            ProgramDB data that Ghidra links back to a live function, so the
            author-documented prototype has to be the last word.
            """

            for primary in primaries:
                try:
                    with program_transaction(program, f"Prototype {hex_address(primary.address)}"):
                        applied = _apply_name_and_prototype(program, primary)
                        if applied.get("action") in {"conflict", "unresolved-signature"}:
                            raise RowApplyError(applied)
                    signatures.append(applied)
                except RowApplyError as exc:
                    signatures.append(exc.payload)
                    conflicts.append(exc.payload)

        this_plan = collect_this_typing_plan(settings.repo_dir, program, target=target)
        _record_step(steps, conflicts, "class-this", apply_this_typing(program, this_plan))

        vftable_plan = collect_vftable_typing_plan(settings.repo_dir, program, target=target)
        _record_step(steps, conflicts, "vftables", apply_vftable_typing(program, vftable_plan))

        try:
            vb_plan = collect_vbtable_typing_plan(settings.repo_dir, program, target=target)
            _record_step(steps, conflicts, "vbtables", apply_vbtable_typing(program, vb_plan))
        except Exception as exc:  # noqa: BLE001 — still a hard apply failure
            failure = {"step": "vbtables", "error": str(exc)}
            steps.append({"step": "vbtables", "result": {"error": str(exc)}})
            conflicts.append(failure)

        iat_plan = collect_surrender_iat_plan(settings.repo_dir, program)
        _record_step(
            steps, conflicts, "surrender-iat", apply_surrender_iat_typing(program, iat_plan)
        )

        global_plan = collect_global_typing_plan(settings.repo_dir, program, target=target)
        _record_step(steps, conflicts, "globals", apply_global_typing(program, global_plan))

        callback_plan = collect_callback_typing_plan(program)
        _record_step(steps, conflicts, "callbacks", apply_callback_typing(program, callback_plan))

        cosmic_plan = collect_cosmic_forge_plan(settings.repo_dir, program)
        _record_step(
            steps, conflicts, "cosmic-forge", apply_cosmic_forge_globals(program, cosmic_plan)
        )

        attribute_plan = collect_function_attribute_plan(settings.repo_dir, program, target=target)
        _record_step(
            steps,
            conflicts,
            "attributes",
            apply_function_attributes(program, attribute_plan, apply_thunks=False),
        )

        project_prototypes()

        save_program(program, "Synchronize established source facts")
        saved = True

    unresolved = [row for row in signatures if row.get("action") == "unresolved-signature"]
    complete = saved and _projection_complete(conflicts, unresolved, steps)
    if complete:
        index_path = settings.repo_dir / "build/source-index.json"
        previous = recorded_source_projection(settings, program_name)
        payload: dict[str, Any] = {
            "source_index_sha256": sha256_file(index_path) if index_path.is_file() else None,
            "applied_ns": time.time_ns(),
            "target": target,
            "complete": True,
        }
        if import_source:
            payload.update(compiler_import_identity(settings, target))
        else:
            for key in ("pdb_sha256", "reccmp_revision"):
                if previous.get(key):
                    payload[key] = previous[key]
        record_source_projection(settings, program_name, payload)

    return {
        "schema": "wiz8.ghidra-sync",
        "program": program_name,
        "source_index": index,
        "created": created,
        "signature_updates": sum(1 for row in signatures if row.get("action") == "updated"),
        "signature_agreements": sum(1 for row in signatures if row.get("action") == "agree"),
        "unresolved_signatures": unresolved[:20],
        "conflicts": conflicts[:50],
        "steps": steps,
        "ok": complete,
        "provenance_advanced": complete,
    }
