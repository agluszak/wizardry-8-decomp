"""Plan-driven Ghidra-native candidate inference to a fixpoint."""

from __future__ import annotations

import csv
import hashlib
import json
from pathlib import Path
from typing import Any

from ..config import Settings
from ..indirect import resolve_handler_table
from ..typevars import derive_type_variables, load_knowledge, unify
from .candidate_facts import stamp
from .dependency_graph import add_pcode_dependencies, build_program_graph, canonical_address
from .project import resolve_program_name

CATEGORY = "/wiz8/overlay/candidates"
SCREEN_DISPATCH_FUNCTION = 0x004E3340
SCREEN_HANDLER_FIELD = 0x00647BD4


def load_plan(repo: Path, value: str) -> dict[str, Any]:
    """Load an explicit hypothesis plan; inference never guesses its scope."""

    path = Path(value)
    if not path.is_absolute():
        path = repo / path
    if not path.is_file():
        raise ValueError(
            f"hypothesis plan does not exist: {value}; pass a JSON plan with a hypothesis key"
        )
    plan = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(plan, dict) or not plan.get("hypothesis"):
        raise ValueError("hypothesis plan must be an object with a non-empty hypothesis")
    plan.setdefault("type_variables", [])
    plan.setdefault("vtables", [])
    plan.setdefault("screen_dispatch", False)
    plan.setdefault("aggregates", False)
    plan.setdefault("reconstructed", False)
    plan.setdefault("max_iterations", 8)
    plan.setdefault("semantic_limit", 120)
    return plan


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
    iterator = program.getDataTypeManager().getAllStructures()
    while iterator.hasNext():
        candidate = iterator.next()
        if candidate.getName() == name:
            return candidate
    return None


def _reviewed_owner(repo: Path, entry: str) -> str | None:
    path = repo / "evidence" / "reviewed" / "wiz8" / "functions.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["address"].lower().zfill(8) != entry.lower().zfill(8):
                continue
            current = row["current_name"]
            name = current if "::" in current else row["provisional_name"] or current
            return name.split("::", 1)[0] if "::" in name else None
    return None


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
    knowledge = load_knowledge(repo, program_name)
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
        if selected and applied:
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


def _reaches(node: Any, address: int, visited: set[Any] | None = None) -> bool:
    if node is None:
        return False
    visited = visited or set()
    marker = (str(node.getAddress()), str(node.getDef().getSeqnum()) if node.getDef() else None)
    if marker in visited:
        return False
    visited.add(marker)
    space = node.getAddress().getAddressSpace().getName()
    if (node.isConstant() or space == "ram") and int(node.getOffset()) == address:
        return True
    definition = node.getDef()
    return bool(
        definition is not None
        and any(
            _reaches(definition.getInput(index), address, visited)
            for index in range(definition.getNumInputs())
        )
    )


def add_screen_references(program: Any, repo: Path, hypothesis: str) -> dict[str, Any]:
    """Materialize the 44 real handler targets at the actual CALLIND site."""

    from ghidra.program.model.symbol import RefType, SourceType

    from .semantic import _high_function

    function = program.getFunctionManager().getFunctionAt(
        _address(program, SCREEN_DISPATCH_FUNCTION)
    )
    if function is None:
        return {"sites": [], "new_references": 0, "handlers": 0}
    high = _high_function(program, function, "normalize")
    sites = []
    iterator = high.getPcodeOps()
    while iterator.hasNext():
        op = iterator.next()
        if op.getMnemonic() == "CALLIND" and _reaches(op.getInput(0), SCREEN_HANDLER_FIELD):
            sites.append(op.getSeqnum().getTarget())
    table = resolve_handler_table(repo)
    targets = sorted(table["handler_targets"])
    references = program.getReferenceManager()
    added = 0
    for site in sites:
        existing = {
            str(reference.getToAddress()) for reference in references.getReferencesFrom(site)
        }
        for target in targets:
            destination = _address(program, target)
            if str(destination) not in existing:
                references.addMemoryReference(
                    site, destination, RefType.COMPUTED_CALL, SourceType.ANALYSIS, -1
                )
                added += 1
        stamp(
            program,
            site,
            hypothesis=hypothesis,
            fact_id="screen-handler-target-set",
            depends_on=[f"pcode:{site}", f"table:{SCREEN_HANDLER_FIELD:08x}"],
            target_set={
                "real_handlers": table["handler_targets"],
                "folded_logical_handlers": table["folded_stubs"],
            },
        )
    return {
        "sites": [str(site) for site in sites],
        "new_references": added,
        "handlers": len(targets),
        "folded_logical_handlers": sum(len(slots) for slots in table["folded_stubs"].values()),
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
            names.add(high.getDataType().getDisplayName())
        child = node.getDef()
        if child is not None:
            for index in range(child.getNumInputs()):
                collect(child.getInput(index), depth + 1)

    collect(target)
    return (slot_bytes // 4, names)


def add_virtual_references(
    program: Any, repo: Path, hypothesis: str, classes: list[str], candidates: list[Any]
) -> dict[str, Any]:
    """Resolve typed receiver -> vptr -> slot -> CALLIND expressions."""

    from ghidra.program.model.symbol import RefType, SourceType

    from .semantic import _high_function

    reviewed = repo / "evidence" / "reviewed" / "wiz8"
    table_ids: dict[str, set[str]] = {name: set() for name in classes}
    with (reviewed / "vtables.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["class_name"] in table_ids:
                table_ids[row["class_name"]].add(row["vtable_id"])
    slots: dict[tuple[str, int], set[str]] = {}
    with (reviewed / "vtable-slots.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            for class_name, ids in table_ids.items():
                if row["vtable_id"] in ids and row["target"]:
                    slots.setdefault((class_name, int(row["slot_index"])), set()).add(row["target"])
    references = program.getReferenceManager()
    added = 0
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
            owners = [
                name
                for name in classes
                if any(name.casefold() in type_name.casefold() for type_name in type_names)
            ]
            if len(owners) != 1:
                continue
            targets = sorted(slots.get((owners[0], slot), set()))
            if not targets:
                continue
            site = op.getSeqnum().getTarget()
            existing = {
                str(reference.getToAddress()) for reference in references.getReferencesFrom(site)
            }
            for target in targets:
                destination = _address(program, target)
                if str(destination) not in existing:
                    references.addMemoryReference(
                        site, destination, RefType.COMPUTED_CALL, SourceType.ANALYSIS, -1
                    )
                    added += 1
            stamp(
                program,
                site,
                hypothesis=hypothesis,
                fact_id=f"virtual-target-set:{owners[0]}:slot{slot}",
                depends_on=[f"pcode:{site}", f"receiver-type:{owners[0]}", f"vtable-slot:{slot}"],
                target_set={"receiver": owners[0], "slot": slot, "targets": targets},
            )
            resolved_sites.append(str(site))
    return {"new_references": added, "resolved_sites": sorted(set(resolved_sites))}


def _indirect_functions(program: Any, limit: int) -> list[Any]:
    functions = []
    iterator = program.getFunctionManager().getFunctions(True)
    listing = program.getListing()
    while iterator.hasNext() and len(functions) < limit:
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
    return functions


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


def analyze_overlay(settings: Settings, selector: str, plan_value: str) -> dict[str, Any]:
    """Apply a plan and iterate until no candidate type or edge changes."""

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .overlay import (
        _overlay_settings,
        _scratch_dir,
        apply_typed_vtable,
        create_overlay,
    )

    plan = load_plan(settings.repo_dir, plan_value)
    hypothesis = str(plan["hypothesis"])
    effective, _ = materialize_program(settings, selector)
    if not _scratch_dir(effective, hypothesis).exists():
        create_overlay(settings, selector, hypothesis)
    if plan["reconstructed"]:
        from .reconstructed_transfer import transfer_into_overlay

        transfer_into_overlay(settings, selector, hypothesis)
    for class_name in plan["vtables"]:
        apply_typed_vtable(settings, selector, hypothesis, class_name)
    if plan["aggregates"]:
        from .aggregate_overlay import apply_aggregates

        apply_aggregates(settings, selector, hypothesis)

    overlay = _overlay_settings(effective, hypothesis)
    start_pyghidra(settings)
    import pyghidra

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    iterations = []
    total_fields = total_targets = total_unifications = 0
    contradictions: list[Any] = []
    before: dict[str, dict[str, Any]] = {}
    after: dict[str, dict[str, Any]] = {}
    final_graph: dict[str, Any] = {}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            # Baseline semantic snapshot is taken from the untouched clone
            # state after explicit seed mutations; iteration-to-iteration
            # classification then reports consequences rather than seed setup.
            seed_functions = [
                canonical_address(item["function"]) for item in plan["type_variables"]
            ]
            seed_functions.append(f"{SCREEN_DISPATCH_FUNCTION:08x}") if plan[
                "screen_dispatch"
            ] else None
            planned_tables: set[int] = set()
            for class_name in plan["vtables"]:
                tables, _slots = _reviewed_tables(settings.repo_dir, class_name)
                planned_tables.update(int(table["address"], 16) for table in tables)
            graph = build_program_graph(program, planned_tables)
            candidates = _indirect_functions(program, int(plan.get("max_indirect_functions", 512)))
            measured = sorted(set(seed_functions))[: int(plan["semantic_limit"])]
            before = _semantic_snapshot(program, measured)

            for number in range(1, int(plan["max_iterations"]) + 1):
                transaction = program.startTransaction(f"candidate inference iteration {number}")
                iteration: dict[str, Any] = {"iteration": number, "type_variables": []}
                changed = 0
                try:
                    for request in plan["type_variables"]:
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
                    screen = (
                        add_screen_references(program, settings.repo_dir, hypothesis)
                        if plan["screen_dispatch"]
                        else {"new_references": 0}
                    )
                    virtual = (
                        add_virtual_references(
                            program,
                            settings.repo_dir,
                            hypothesis,
                            list(plan["vtables"]),
                            candidates,
                        )
                        if plan["vtables"]
                        else {"new_references": 0, "resolved_sites": []}
                    )
                    iteration["screen_dispatch"] = screen
                    iteration["virtual_calls"] = virtual
                    new_targets = int(screen.get("new_references", 0)) + int(
                        virtual.get("new_references", 0)
                    )
                    total_targets += new_targets
                    changed += new_targets

                    graph = build_program_graph(program, planned_tables)
                    for class_name in plan["vtables"]:
                        vtable, slots = _reviewed_tables(settings.repo_dir, class_name)
                        for table in vtable:
                            for slot in slots.get(table["vtable_id"], []):
                                graph.add(table["address"], slot["target"], "vtable-slot")
                    frontier = sorted(
                        {
                            address
                            for group in graph.cone(seed_functions, limit=4096).values()
                            for address in group
                            if _is_memory_address(address)
                            if program.getFunctionManager().getFunctionAt(
                                _address(program, address)
                            )
                            is not None
                        }
                    )
                    for address in frontier[: int(plan["semantic_limit"])]:
                        function = program.getFunctionManager().getFunctionAt(
                            _address(program, address)
                        )
                        if function is not None:
                            try:
                                add_pcode_dependencies(program, graph, function, planned_tables)
                            except Exception:  # noqa: BLE001,S110 - graph remains useful without one body
                                pass
                    measured = sorted(set(measured) | set(frontier[: int(plan["semantic_limit"])]))
                    iteration["dependency_cone"] = {
                        reason: len(addresses)
                        for reason, addresses in graph.cone(seed_functions).items()
                    }
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
            program.save("candidate inference fixpoint", None)
            after = _semantic_snapshot(program, measured)
            final_graph = graph.serialise()
    finally:
        from .semantic import dispose_sessions

        dispose_sessions()
        project.close()

    reached = bool(iterations) and iterations[-1]["new_candidate_changes"] == 0
    return {
        "hypothesis": hypothesis,
        "plan": plan_value,
        "iterations": iterations,
        "fixpoint": reached,
        "iteration_count": len(iterations),
        "semantic_changes": _classify_changes(
            before, after, total_fields, total_targets, total_unifications, contradictions
        ),
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
    }


def _reviewed_tables(
    repo: Path, class_name: str
) -> tuple[list[dict[str, str]], dict[str, list[dict[str, str]]]]:
    reviewed = repo / "evidence" / "reviewed" / "wiz8"
    with (reviewed / "vtables.csv").open(newline="", encoding="utf-8") as stream:
        tables = [row for row in csv.DictReader(stream) if row["class_name"] == class_name]
    ids = {row["vtable_id"] for row in tables}
    slots: dict[str, list[dict[str, str]]] = {identifier: [] for identifier in ids}
    with (reviewed / "vtable-slots.csv").open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["vtable_id"] in ids and row["target"]:
                slots[row["vtable_id"]].append(row)
    return tables, slots
