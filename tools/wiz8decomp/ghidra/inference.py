"""Plan-driven Ghidra-native candidate inference to bounded stabilization."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Annotated, Any, Literal

from pydantic import BaseModel, ConfigDict, Field, field_validator

from ..config import Settings
from ..typevars import derive_type_variables, load_knowledge, unify
from .candidate_facts import facts, get_fact, iter_facts, stamp
from .dependency_graph import add_pcode_dependencies, build_program_graph, canonical_address
from .evidence_index import load_evidence_index, reviewed_owner
from .project import resolve_program_name

CATEGORY = "/wiz8/overlay/candidates"


class TypeVariableSeed(BaseModel):
    model_config = ConfigDict(extra="forbid")
    kind: Literal["type-variable"]
    function: str
    root: str = "this"

    @field_validator("function")
    @classmethod
    def address_is_explicit(cls, value: str) -> str:
        text = value.lower().removeprefix("0x")
        if len(text) != 8 or any(character not in "0123456789abcdef" for character in text):
            raise ValueError("function must be an eight-digit hexadecimal address")
        return text


class AggregateSeed(BaseModel):
    model_config = ConfigDict(extra="forbid")
    kind: Literal["aggregate"]
    name: str = Field(min_length=1)
    minimum_agreeing_sites: int = Field(default=2, ge=2, le=64)


class VtableSeed(BaseModel):
    model_config = ConfigDict(extra="forbid")
    kind: Literal["vtable"]
    name: str = Field(min_length=1)


class ReconstructedSeed(BaseModel):
    model_config = ConfigDict(extra="forbid")
    kind: Literal["reconstructed-transfer"]
    function: str

    @field_validator("function")
    @classmethod
    def address_is_explicit(cls, value: str) -> str:
        return TypeVariableSeed.address_is_explicit(value)


Seed = Annotated[
    TypeVariableSeed | AggregateSeed | VtableSeed | ReconstructedSeed,
    Field(discriminator="kind"),
]


class InferenceLimits(BaseModel):
    model_config = ConfigDict(extra="forbid")
    iterations: int = Field(default=8, ge=1, le=64)
    functions: int = Field(default=120, ge=1, le=100_000)
    indirect_functions: int = Field(default=512, ge=1, le=100_000)


class InferencePlan(BaseModel):
    model_config = ConfigDict(extra="forbid")
    hypothesis: str = Field(min_length=1)
    seeds: list[Seed] = Field(min_length=1)
    limits: InferenceLimits = Field(default_factory=InferenceLimits)


def load_plan(repo: Path, value: str) -> dict[str, Any]:
    """Load an explicit hypothesis plan; inference never guesses its scope."""

    path = Path(value)
    if not path.is_absolute():
        path = repo / path
    if not path.is_file():
        raise ValueError(
            f"hypothesis plan does not exist: {value}; pass a JSON plan with a hypothesis key"
        )
    decoded = json.loads(path.read_text(encoding="utf-8"))
    return InferencePlan.model_validate(decoded).model_dump()


def plan_sha256(plan: dict[str, Any]) -> str:
    canonical = json.dumps(plan, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def _address(program: Any, value: str | int) -> Any:
    if isinstance(value, str):
        integer = int(value, 0) if value.lower().startswith("0x") else int(value, 16)
    else:
        integer = value
    return program.getAddressFactory().getDefaultAddressSpace().getAddress(f"{integer:08x}")


def _is_memory_address(value: str) -> bool:
    text = value.lower().removeprefix("0x")
    return len(text) == 8 and all(character in "0123456789abcdef" for character in text)


def _structure(program: Any, name: str) -> Any | None:
    dtm = program.getDataTypeManager()
    paths = (
        f"{CATEGORY}/{name}",
        f"/wiz8/classes/{name}",
        f"/surrender/classes/{name}",
    )
    for path in paths:
        candidate = dtm.getDataType(path)
        if candidate is not None:
            return candidate
    return None


def _reviewed_owner(repo: Path, entry: str) -> str | None:
    return reviewed_owner(load_evidence_index(repo), int(entry, 16))


def _candidate_type_knowledge(program: Any) -> list[dict[str, Any]]:
    """Destructor-backed candidate structures already stored in this overlay."""

    knowledge = []
    for _address_value, fact_id, record in iter_facts(program):
        payload = record.get("payload", {})
        if (
            record.get("status") != "candidate"
            or not fact_id.startswith("CandidateType_")
            or not payload.get("complete_destructor")
        ):
            continue
        knowledge.append(
            {
                "type": fact_id,
                "tier": "candidate",
                "polymorphic": bool(payload.get("has_virtual_destructor")),
                "slot0": None,
                "destructors": {str(payload["complete_destructor"]).lower()},
            }
        )
    return knowledge


def materialize_type_variables(
    program: Any,
    repo: Path,
    program_name: str,
    hypothesis: str,
    entry: str,
    root: str,
) -> dict[str, Any]:
    """Derive, persist and apply the anonymous pointee types of one owner."""

    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
        StructureDataType,
        Undefined,
        UnsignedIntegerDataType,
        VoidDataType,
    )
    from ghidra.program.model.symbol import SourceType

    from .query import _operator_delete_entries
    from .semantic import field_accesses

    traced = field_accesses(program, entry, root)
    variables = derive_type_variables(
        traced["entry"],
        traced["root"],
        traced["accesses"],
        traced["calls"],
        _operator_delete_entries(program),
    )
    knowledge = load_knowledge(repo, program_name) + _candidate_type_knowledge(program)
    dtm = program.getDataTypeManager()
    category = CategoryPath(CATEGORY)
    owner_name = _reviewed_owner(repo, traced["entry"])
    owner = _structure(program, owner_name) if owner_name else None
    report: dict[str, Any] = {
        "entry": traced["entry"],
        "owner": owner_name,
        "variables": [],
        "new_facts": 0,
        "fields_applied": 0,
        "signature_applied": 0,
        "unifications": 0,
        "contradictions": [],
    }
    if owner is not None:
        function = program.getFunctionManager().getFunctionAt(_address(program, traced["entry"]))
        explicit = (
            [parameter for parameter in function.getParameters() if not parameter.isAutoParameter()]
            if function is not None
            else []
        )
        receiver = PointerDataType(owner, dtm)
        report["receiver_applied"] = False
        if explicit and explicit[0].getDataType().getLength() == 4:
            if explicit[0].getDataType().getDisplayName() != receiver.getDisplayName():
                explicit[0].setDataType(receiver, SourceType.ANALYSIS)
                report["receiver_applied"] = True
                report["signature_applied"] += 1
        elif function is not None:
            # No committed parameter list exists, so preserve every parameter
            # HighFunction recovered while materialising the implicit ECX
            # receiver. This is the safe case for applying a complete list:
            # there is no existing list to destroy.
            from ghidra.app.cmd.function import (
                ApplyFunctionSignatureCmd,
                FunctionRenameOption,
            )
            from ghidra.program.model.data import CategoryPath

            from .apply_unzip_model import _function_type
            from .semantic import _high_function

            prototype = _high_function(program, function).getFunctionPrototype()
            arguments = [("this", receiver)]
            for index in range(prototype.getNumParams()):
                parameter = prototype.getParam(index)
                if "ECX" in str(parameter.getStorage()).upper():
                    continue
                arguments.append(
                    (
                        parameter.getName() or f"argument{index + 1}",
                        parameter.getDataType(),
                    )
                )
            signature = _function_type(
                dtm,
                CategoryPath(CATEGORY),
                f"candidate_signature_{traced['entry']}",
                prototype.getReturnType(),
                arguments,
                "__thiscall",
            )
            command = ApplyFunctionSignatureCmd(
                function.getEntryPoint(),
                signature,
                SourceType.ANALYSIS,
                True,
                FunctionRenameOption.NO_CHANGE,
            )
            if command.applyTo(program):
                function.setCustomVariableStorage(True)
                parameters = function.getParameters()
                if parameters:
                    parameters[0].setDataType(receiver, SourceType.ANALYSIS)
                report["receiver_applied"] = True
                report["signature_applied"] += 1
    for variable in variables:
        constraints = dict(variable["constraints"])
        dependencies = [f"pcode:{site}" for site in variable["sources"]]
        destructor = constraints.get("complete_destructor")
        if destructor:
            dependencies.append(f"call:{destructor}")
            try:
                pointee = field_accesses(program, destructor, "0")
                members = sorted(
                    {
                        (int(access["offset"], 16), int(access.get("width") or 1), access["site"])
                        for access in pointee["accesses"]
                        if access["path"] == pointee["root"] and access["kind"] in {"load", "store"}
                    }
                )
                if members:
                    constraints["pointee_accesses"] = [
                        {"offset": f"0x{offset:x}", "width": width, "site": site}
                        for offset, width, site in members
                    ]
                    dependencies.extend(f"pcode:{site}" for _offset, _width, site in members)
            except Exception:  # noqa: BLE001,S110 - an unknown prototype may have no root yet
                pass

        matches = unify({**variable, "constraints": constraints}, knowledge)
        reviewed_match = any(match["tier"] == "reviewed" for match in matches)
        candidate_type = None
        if destructor and constraints.get("pointee_accesses") and not reviewed_match:
            candidate_name = f"CandidateType_{destructor}"
            candidate_type = _structure(program, candidate_name)
            if candidate_type is None:
                extent = max(
                    int(access["offset"], 16) + int(access["width"])
                    for access in constraints["pointee_accesses"]
                )
                candidate = StructureDataType(category, candidate_name, extent)
                for access in constraints["pointee_accesses"]:
                    member_offset = int(access["offset"], 16)
                    width = min(int(access["width"]), extent - member_offset)
                    candidate.replaceAtOffset(
                        member_offset,
                        Undefined.getUndefinedDataType(width),
                        width,
                        f"field_{member_offset:x}",
                        f"accessed at {access['site']}",
                    )
                candidate_type = dtm.addDataType(candidate, DataTypeConflictHandler.REPLACE_HANDLER)
            report["new_facts"] += int(
                stamp(
                    program,
                    _address(program, destructor),
                    hypothesis=hypothesis,
                    fact_id=candidate_name,
                    depends_on=sorted(set(dependencies)),
                    constraints=constraints,
                    type_variable=variable["name"],
                )
            )

        selected = matches[0]["type"] if len(matches) == 1 else None
        if selected:
            pointee_type = _structure(program, selected)
        else:
            pointee_type = None
        if pointee_type is None:
            extent = 4 if constraints.get("has_virtual_destructor") else 1
            for access in constraints.get("pointee_accesses", []):
                extent = max(extent, int(access["offset"], 16) + int(access["width"]))
            anonymous = StructureDataType(category, variable["name"], extent)
            if constraints.get("has_virtual_destructor"):
                vtable = StructureDataType(category, f"{variable['name']}_vtable", 0)
                definition = FunctionDefinitionDataType(
                    category, f"{variable['name']}_scalar_deleting_destructor"
                )
                definition.setReturnType(VoidDataType.dataType)
                definition.setArguments(
                    [
                        ParameterDefinitionImpl("this", PointerDataType(anonymous, dtm), None),
                        ParameterDefinitionImpl("flags", UnsignedIntegerDataType.dataType, None),
                    ]
                )
                definition.setCallingConvention("__thiscall")
                definition = dtm.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
                vtable.add(PointerDataType(definition, dtm), 4, "scalar_deleting_destructor", None)
                vtable = dtm.addDataType(vtable, DataTypeConflictHandler.REPLACE_HANDLER)
                anonymous.replaceAtOffset(
                    0, PointerDataType(vtable, dtm), 4, "vptr", "candidate virtual destructor"
                )
            for access in constraints.get("pointee_accesses", []):
                offset = int(access["offset"], 16)
                width = max(1, int(access["width"]))
                if offset == 0 and constraints.get("has_virtual_destructor"):
                    continue
                anonymous.replaceAtOffset(
                    offset,
                    Undefined.getUndefinedDataType(min(width, extent - offset)),
                    min(width, extent - offset),
                    f"field_{offset:x}",
                    f"accessed at {access['site']}",
                )
            pointee_type = dtm.addDataType(anonymous, DataTypeConflictHandler.REPLACE_HANDLER)

        offset = int(variable["root_offset"], 16)
        applied = False
        if owner is not None and offset + 4 <= owner.getLength():
            component = owner.getComponentAt(offset)
            field_name = component.getFieldName() if component is not None else None
            comment = component.getComment() if component is not None else None
            wanted = PointerDataType(pointee_type, dtm)
            if (
                component is None
                or component.getDataType().getDisplayName() != wanted.getDisplayName()
            ):
                owner.replaceAtOffset(
                    offset,
                    wanted,
                    4,
                    field_name or f"owned_{offset:x}",
                    comment or "candidate anonymous pointee",
                )
                report["fields_applied"] += 1
                applied = True

        fact_id = variable["name"]
        anchors = [traced["entry"], *variable["sources"]]
        previous = facts(program, _address(program, traced["entry"])).get(fact_id, {})
        was_unified = bool(
            selected and previous.get("payload", {}).get("unified_with") == selected
        )
        for anchor in anchors:
            report["new_facts"] += int(
                stamp(
                    program,
                    _address(program, anchor),
                    hypothesis=hypothesis,
                    fact_id=fact_id,
                    depends_on=sorted(set(dependencies)),
                    constraints=constraints,
                    type_variable=variable["name"],
                    unified_with=selected,
                )
            )
        if selected and not was_unified:
            report["unifications"] += 1
        report["variables"].append(
            {
                **variable,
                "constraints": constraints,
                "match_count": len(matches),
                "matches": matches if len(matches) <= 20 else [],
                "match_sample": matches[:10] if len(matches) > 20 else [],
                "materialized_type": pointee_type.getDisplayName(),
                "owner_field_applied": applied,
            }
        )
    return report


def _replace_owned_target_set(
    program: Any,
    site: Any,
    *,
    hypothesis: str,
    fact_id: str,
    depends_on: list[str],
    payload: dict[str, Any],
) -> dict[str, Any]:
    """Replace one fact's computed-call references and atomic payload."""

    from ghidra.program.model.symbol import RefType, SourceType

    references = program.getReferenceManager()
    previous = get_fact(program, site, fact_id) or {}
    old_targets = {str(value).lower() for value in previous.get("payload", {}).get("targets", [])}
    old_owned = {
        str(value).lower()
        for value in previous.get("payload", {}).get("owned_targets", [])
    }
    new_targets = {str(value).lower() for value in payload.get("targets", [])}
    removed_targets = old_targets - new_targets
    added_targets = new_targets - old_targets
    removable_targets = removed_targets & old_owned
    removed = 0
    for reference in list(references.getReferencesFrom(site)):
        if (
            str(reference.getToAddress()).lower() in removable_targets
            and reference.getReferenceType().isComputed()
            and str(reference.getSource()).upper() == "ANALYSIS"
        ):
            references.delete(reference)
            removed += 1
    existing = {
        str(reference.getToAddress()).lower()
        for reference in references.getReferencesFrom(site)
    }
    added = 0
    newly_owned: set[str] = set()
    for target in sorted(added_targets):
        destination = _address(program, target)
        if str(destination).lower() in existing:
            continue
        references.addMemoryReference(
            site, destination, RefType.COMPUTED_CALL, SourceType.ANALYSIS, -1
        )
        added += 1
        newly_owned.add(target)
    fact_payload = {
        **payload,
        "owned_targets": sorted((old_owned & new_targets) | newly_owned),
    }
    fact_changed = stamp(
        program,
        site,
        hypothesis=hypothesis,
        fact_id=fact_id,
        depends_on=depends_on,
        target_set=fact_payload,
    )
    return {
        "added": added,
        "removed": removed,
        "fact_changed": fact_changed,
        "narrowed": bool(old_targets and new_targets < old_targets),
    }


def _virtual_shape(op: Any) -> tuple[int, set[str]] | None:
    target = op.getInput(0)
    definition = target.getDef() if target is not None else None
    if definition is None or definition.getMnemonic() != "LOAD":
        return None
    pointer = definition.getInput(1)
    slot_bytes = 0
    pointer_def = pointer.getDef() if pointer is not None else None
    if pointer_def is not None and pointer_def.getMnemonic() in {"INT_ADD", "PTRSUB"}:
        constant = next(
            (
                pointer_def.getInput(index)
                for index in range(pointer_def.getNumInputs())
                if pointer_def.getInput(index).isConstant()
            ),
            None,
        )
        if constant is not None:
            slot_bytes = int(constant.getOffset())
    elif pointer_def is not None and pointer_def.getMnemonic() == "PTRADD":
        index, scale = pointer_def.getInput(1), pointer_def.getInput(2)
        if index.isConstant() and scale.isConstant():
            slot_bytes = int(index.getOffset()) * int(scale.getOffset())
    names: set[str] = set()

    def collect(node: Any, depth: int = 0) -> None:
        if node is None or depth > 6:
            return
        high = node.getHigh()
        if high is not None and high.getDataType() is not None:
            data_type = high.getDataType()
            names.add(str(data_type.getDisplayName()))
            try:
                names.add(str(data_type.getPathName()))
            except Exception:  # noqa: BLE001,S110 - not every datatype exposes a path
                pass
        child = node.getDef()
        if child is not None:
            for index in range(child.getNumInputs()):
                collect(child.getInput(index), depth + 1)

    collect(target)
    return (slot_bytes // 4, names)


def _exact_vtable_ids(
    type_names: set[str], identities: dict[str, str]
) -> set[str]:
    """Resolve only exact generated DataType identities, never name substrings."""

    return {
        identities[name.rstrip(" *")]
        for name in type_names
        if name.rstrip(" *") in identities
    }


def add_virtual_references(
    program: Any, repo: Path, hypothesis: str, classes: list[str], candidates: list[Any]
) -> dict[str, Any]:
    """Resolve typed receiver -> vptr -> slot -> CALLIND expressions."""

    from .semantic import _high_function

    index = load_evidence_index(repo)
    table_ids = {
        table.vtable_id
        for class_name in classes
        for table in index.vtables_by_class.get(class_name, ())
    }
    slots: dict[tuple[str, int], set[str]] = {}
    for vtable_id in table_ids:
        for slot in index.slots_by_vtable[vtable_id]:
            slots.setdefault((vtable_id, slot.index), set()).add(f"{slot.target:08x}")
    identities = {
        identity: vtable_id
        for vtable_id in table_ids
        for identity in {vtable_id, f"/wiz8/overlay/vtables/{vtable_id}"}
    }
    added = removed = narrowed = 0
    resolved_sites = []
    for function in candidates:
        try:
            high = _high_function(program, function)
        except Exception:  # noqa: BLE001,S112 - some tiny/thunk bodies do not decompile
            continue
        iterator = high.getPcodeOps()
        while iterator.hasNext():
            op = iterator.next()
            if op.getMnemonic() != "CALLIND":
                continue
            shape = _virtual_shape(op)
            if shape is None:
                continue
            slot, type_names = shape
            resolved_ids = _exact_vtable_ids(type_names, identities)
            if len(resolved_ids) != 1:
                continue
            vtable_id = next(iter(resolved_ids))
            targets = sorted(slots.get((vtable_id, slot), set()))
            if not targets:
                continue
            site = op.getSeqnum().getTarget()
            update = _replace_owned_target_set(
                program,
                site,
                hypothesis=hypothesis,
                fact_id=f"virtual-target-set:{site}",
                depends_on=[f"pcode:{site}", f"vtable:{vtable_id}", f"vtable-slot:{slot}"],
                payload={"vtable_id": vtable_id, "slot": slot, "targets": targets},
            )
            added += update["added"]
            removed += update["removed"]
            narrowed += int(update["narrowed"])
            resolved_sites.append(str(site))
    return {
        "new_references": added,
        "removed_references": removed,
        "narrowed_target_sets": narrowed,
        "resolved_sites": sorted(set(resolved_sites)),
    }


def _indirect_functions(program: Any, limit: int) -> tuple[list[Any], int]:
    functions = []
    iterator = program.getFunctionManager().getFunctions(True)
    listing = program.getListing()
    while iterator.hasNext():
        function = iterator.next()
        instructions = listing.getInstructions(function.getBody(), True)
        while instructions.hasNext():
            instruction = instructions.next()
            if instruction.getMnemonicString().upper() == "CALL" and not any(
                reference.getReferenceType().isCall()
                for reference in program.getReferenceManager().getReferencesFrom(
                    instruction.getAddress()
                )
            ):
                functions.append(function)
                break
    return functions[:limit], len(functions)


def _semantic_snapshot(program: Any, addresses: list[str]) -> dict[str, dict[str, Any]]:
    from .query import execute_query
    from .semantic import high_function

    snapshots = {}
    for address in addresses:
        function = program.getFunctionManager().getFunctionAt(_address(program, address))
        if function is None:
            continue
        try:
            body = execute_query(program, "decompile", [address]).get("decompiled") or ""
            high = high_function(program, address)
            snapshots[address] = {
                "presentation": hashlib.sha256(body.encode()).hexdigest(),
                "prototype": {
                    "return": high["return_type"],
                    "calling_convention": high["calling_convention"],
                    "parameters": [(item["type"], item["storage"]) for item in high["parameters"]],
                },
            }
        except Exception as error:  # noqa: BLE001 - one bad function does not erase the cone
            snapshots[address] = {"error": str(error)}
    return snapshots


def _classify_changes(
    before: dict[str, dict[str, Any]],
    after: dict[str, dict[str, Any]],
    new_fields: int,
    new_targets: int,
    unifications: int,
    contradictions: list[Any],
) -> dict[str, Any]:
    presentation = sorted(
        address
        for address in before.keys() & after.keys()
        if before[address].get("presentation") != after[address].get("presentation")
    )
    prototypes = sorted(
        address
        for address in before.keys() & after.keys()
        if before[address].get("prototype") != after[address].get("prototype")
    )
    return {
        "changed_presentation": presentation,
        "changed_inferred_prototype": prototypes,
        "newly_resolved_fields": new_fields,
        "newly_narrowed_indirect_targets": new_targets,
        "newly_unified_type_variables": unifications,
        "contradictions": contradictions,
        "no_semantic_change": not any(
            [presentation, prototypes, new_fields, new_targets, unifications, contradictions]
        ),
    }


def _reviewed_snapshot(
    settings: Settings,
    effective: Settings,
    selector: str,
    program_name: str,
    addresses: list[str],
) -> dict[str, dict[str, Any]]:
    """Read the reviewed ProgramDB only after its daemon has released it."""

    from .environment import start_pyghidra
    from .query_daemon import daemon_status, start_daemon, stop_daemon

    was_running = bool(daemon_status(effective).get("running"))
    stop_daemon(effective, quiet=True)
    try:
        start_pyghidra(settings)
        import pyghidra

        project = pyghidra.open_project(
            effective.project_dir, effective.project_name, create=False
        )
        try:
            with pyghidra.program_context(project, "/" + program_name) as program:
                return _semantic_snapshot(program, addresses)
        finally:
            from .semantic import dispose_sessions

            dispose_sessions()
            project.close()
    finally:
        if was_running:
            start_daemon(settings, selector)


def analyze_overlay(
    settings: Settings, selector: str, plan_value: str, *, resume: bool = False
) -> dict[str, Any]:
    """Apply a plan and iterate until no candidate type or edge changes."""

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .overlay import (
        _overlay_settings,
        apply_typed_vtable,
        create_overlay,
    )

    plan = load_plan(settings.repo_dir, plan_value)
    hypothesis = str(plan["hypothesis"])
    type_variables = [seed for seed in plan["seeds"] if seed["kind"] == "type-variable"]
    vtables = [seed["name"] for seed in plan["seeds"] if seed["kind"] == "vtable"]
    aggregates = [seed for seed in plan["seeds"] if seed["kind"] == "aggregate"]
    reconstructed = [
        seed for seed in plan["seeds"] if seed["kind"] == "reconstructed-transfer"
    ]
    limits = plan["limits"]
    effective, _ = materialize_program(settings, selector)
    creation = create_overlay(
        settings,
        selector,
        hypothesis,
        plan_sha256=plan_sha256(plan),
        resume=resume,
    )
    overlay_id = creation["overlay_id"]
    if reconstructed:
        from .reconstructed_transfer import transfer_into_overlay

        transfer_into_overlay(
            settings,
            selector,
            overlay_id,
            addresses={str(seed["function"]) for seed in reconstructed},
        )
    for class_name in vtables:
        apply_typed_vtable(
            settings, selector, overlay_id, class_name, hypothesis=hypothesis
        )
    aggregate_report: dict[str, Any] = {"placed": [], "types": [], "skipped": []}
    if aggregates:
        from .aggregate_overlay import apply_aggregates

        aggregate_report = apply_aggregates(
            settings,
            selector,
            overlay_id,
            aggregate_seeds=aggregates,
            hypothesis=hypothesis,
        )

    overlay = _overlay_settings(effective, overlay_id)
    start_pyghidra(settings)
    import pyghidra

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    iterations = []
    total_fields = total_targets = total_unifications = 0
    contradictions: list[Any] = []
    seeded: dict[str, dict[str, Any]] = {}
    after: dict[str, dict[str, Any]] = {}
    final_graph: dict[str, Any] = {}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            # Capture the seeded state before closure so its effects remain
            # distinct from consequences discovered by the worklist.
            seed_functions = [
                canonical_address(item["function"]) for item in type_variables
            ]
            seed_functions.extend(canonical_address(item["function"]) for item in reconstructed)
            seed_functions.extend(item["base"] for item in aggregate_report.get("placed", []))
            planned_tables: set[int] = set()
            for class_name in vtables:
                tables, _slots = _reviewed_tables(settings.repo_dir, class_name)
                planned_tables.update(int(table["address"], 16) for table in tables)
            seed_functions.extend(f"{table:08x}" for table in sorted(planned_tables))
            graph = build_program_graph(program, planned_tables)
            candidates, indirect_total = (
                _indirect_functions(program, int(limits["indirect_functions"]))
                if vtables
                else ([], 0)
            )
            initial_closure = graph.closure(
                seed_functions, limit=int(limits["functions"])
            )
            initial_frontier = {
                address
                for group in initial_closure["groups"].values()
                for address in group
                if _is_memory_address(address)
                if program.getFunctionManager().getFunctionAt(
                    _address(program, address)
                )
                is not None
            }
            measured = sorted(set(seed_functions) | initial_frontier)
            seeded = _semantic_snapshot(program, measured)

            for number in range(1, int(limits["iterations"]) + 1):
                transaction = program.startTransaction(f"candidate inference iteration {number}")
                iteration: dict[str, Any] = {"iteration": number, "type_variables": []}
                changed = 0
                try:
                    for request in type_variables:
                        result = materialize_type_variables(
                            program,
                            settings.repo_dir,
                            program_name,
                            hypothesis,
                            str(request["function"]),
                            str(request.get("root", "this")),
                        )
                        iteration["type_variables"].append(result)
                        changed += (
                            result["new_facts"]
                            + result["fields_applied"]
                            + result["signature_applied"]
                        )
                        total_fields += result["fields_applied"]
                        total_unifications += result["unifications"]
                        contradictions.extend(result["contradictions"])
                    virtual = (
                        add_virtual_references(
                            program,
                            settings.repo_dir,
                            hypothesis,
                            vtables,
                            candidates,
                        )
                        if vtables
                        else {"new_references": 0, "resolved_sites": []}
                    )
                    iteration["virtual_calls"] = virtual
                    new_targets = int(virtual.get("new_references", 0))
                    removed_targets = int(virtual.get("removed_references", 0))
                    narrowed_targets = int(virtual.get("narrowed_target_sets", 0))
                    total_targets += narrowed_targets
                    changed += new_targets + removed_targets

                    graph = build_program_graph(program, planned_tables)
                    for class_name in vtables:
                        vtable, slots = _reviewed_tables(settings.repo_dir, class_name)
                        for table in vtable:
                            for slot in slots.get(table["vtable_id"], []):
                                graph.add(table["address"], slot["target"], "vtable-slot")
                    closure = graph.closure(seed_functions, limit=int(limits["functions"]))
                    frontier = sorted(
                        {
                            address
                            for group in closure["groups"].values()
                            for address in group
                            if _is_memory_address(address)
                            if program.getFunctionManager().getFunctionAt(
                                _address(program, address)
                            )
                            is not None
                        }
                    )
                    for address in frontier:
                        function = program.getFunctionManager().getFunctionAt(
                            _address(program, address)
                        )
                        if function is not None:
                            try:
                                add_pcode_dependencies(program, graph, function, planned_tables)
                            except Exception:  # noqa: BLE001,S110 - graph remains useful without one body
                                pass
                    measured = sorted(set(measured) | set(frontier))
                    iteration["dependency_cone"] = {
                        reason: len(addresses)
                        for reason, addresses in closure["groups"].items()
                    }
                    iteration["scope_complete"] = closure["scope_complete"]
                    iteration["truncated_frontier"] = len(closure["truncated_frontier"])
                    iteration["new_candidate_changes"] = changed
                    program.endTransaction(transaction, True)
                except Exception:
                    program.endTransaction(transaction, False)
                    raise
                from .semantic import dispose_sessions

                dispose_sessions()
                iterations.append(iteration)
                if changed == 0:
                    break
            program.save("candidate inference stabilization", None)
            after = _semantic_snapshot(program, measured)
            serialised = graph.serialise()
            relevant_nodes = set(measured) | {
                canonical_address(seed) for seed in seed_functions
            }
            relevant_edges = [
                edge
                for edge in serialised["edges"]
                if edge["source"] in relevant_nodes and edge["target"] in relevant_nodes
            ]
            final_graph = {
                "nodes": len(
                    {edge["source"] for edge in relevant_edges}
                    | {edge["target"] for edge in relevant_edges}
                    | relevant_nodes
                ),
                "edges": relevant_edges,
            }
    finally:
        from .semantic import dispose_sessions

        dispose_sessions()
        project.close()

    reviewed = _reviewed_snapshot(
        settings, effective, selector, program_name, measured
    )

    stabilized = bool(iterations) and iterations[-1]["new_candidate_changes"] == 0
    semantic_frontier = (
        iterations[-1].get("truncated_frontier", 0) if iterations else 0
    )
    indirect_limit = int(limits["indirect_functions"])
    scope_complete = semantic_frontier == 0 and indirect_total <= indirect_limit
    report = {
        "hypothesis": hypothesis,
        "overlay_id": overlay_id,
        "overlay": creation,
        "plan": plan_value,
        "iterations": iterations,
        "stabilized": stabilized,
        "scope_complete": scope_complete,
        "truncated": {
            "semantic_frontier": semantic_frontier,
            "semantic_limit": int(limits["functions"]),
            "indirect_functions": indirect_total,
            "indirect_limit": indirect_limit,
        },
        "iteration_count": len(iterations),
        "semantic_deltas": {
            "reviewed_to_seeded": _classify_changes(
                reviewed, seeded, 0, 0, 0, []
            ),
            "seeded_to_closure": _classify_changes(
                seeded,
                after,
                total_fields,
                total_targets,
                total_unifications,
                contradictions,
            ),
            "reviewed_to_final": _classify_changes(
                reviewed,
                after,
                total_fields,
                total_targets,
                total_unifications,
                contradictions,
            ),
        },
        "dependency_graph": {
            "nodes": final_graph.get("nodes", 0),
            "edges": len(final_graph.get("edges", [])),
            "relations": sorted(
                {
                    reason.split("@", 1)[0]
                    for edge in final_graph.get("edges", [])
                    for reason in edge["reasons"]
                }
            ),
        },
        "dependency_graph_detail": final_graph,
    }
    report["semantic_changes"] = report["semantic_deltas"]["reviewed_to_final"]
    analysis_path = creation["overlay_dir"]
    (Path(analysis_path) / "analysis.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return {
        key: value for key, value in report.items() if key != "dependency_graph_detail"
    }


def _reviewed_tables(
    repo: Path, class_name: str
) -> tuple[list[dict[str, str]], dict[str, list[dict[str, str]]]]:
    index = load_evidence_index(repo)
    reviewed_tables = index.vtables_by_class.get(class_name, ())
    tables = [
        {
            "vtable_id": item.vtable_id,
            "class_name": item.class_name,
            "address": f"{item.address:08x}",
            "subobject_offset": f"0x{int(item.subobject_offset or 0):x}",
        }
        for item in reviewed_tables
    ]
    slots: dict[str, list[dict[str, str]]] = {}
    for table in reviewed_tables:
        slots[table.vtable_id] = [
            {
                "vtable_id": item.vtable_id,
                "slot_index": str(item.index),
                "target": f"{item.target:08x}",
                "slot_name": item.name,
            }
            for item in index.slots_by_vtable[table.vtable_id]
        ]
    return tables, slots
