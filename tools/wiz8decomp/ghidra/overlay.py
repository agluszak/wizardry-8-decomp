"""Candidate overlays: disposable clones of the reviewed program.

A hypothesis is applied to a *clone* of the reviewed materialization, never to
the reviewed program itself. The clone lives beside the content-addressed
materializations under a `scratch-<hypothesis>` directory: applying a
hypothesis family, redecompiling its dependency cone and reading the
consequences all happen there, and discarding the clone is `rm -rf` - the
reviewed baseline cannot be affected because it was never opened for writing.

The first hypothesis kind this supports is the one the flat reviewed model
cannot express: a typed per-slot vtable. The reviewed replay models every
vtable as an array of pointers to one generic argumentless `virtual_function`,
which is why a virtual call decompiles as `(**(code **)(*this + 0x24))()`.
The overlay builds a `<Class>_vtable` structure with one named, `__thiscall`,
receiver-typed function-definition pointer per slot, retypes the class's vptr
to point at it, and types each slot target's `this` - after which every call
through the table renders with a slot name and a typed receiver.

Promotion never happens here: consequences worth keeping go through the
tracked evidence files and a reviewed-baseline rebuild.
"""

from __future__ import annotations

import csv
import re
import shutil
from pathlib import Path
from typing import Any

from ..config import Settings
from .project import resolve_program_name

_SLUG = re.compile(r"[^a-z0-9-]+")


def _slug(hypothesis: str) -> str:
    slug = _SLUG.sub("-", hypothesis.lower()).strip("-")
    if not slug:
        raise ValueError(f"hypothesis name has no usable characters: {hypothesis!r}")
    return slug


def _scratch_dir(effective: Settings, hypothesis: str) -> Path:
    return effective.project_dir.parent / f"scratch-{_slug(hypothesis)}"


def _overlay_settings(effective: Settings, hypothesis: str) -> Settings:
    return effective.model_copy(
        update={"ghidra_project_dir_override": _scratch_dir(effective, hypothesis)}
    )


def create_overlay(settings: Settings, selector: str, hypothesis: str) -> dict[str, Any]:
    """Clone the current reviewed materialization for one hypothesis."""

    from .cache import materialize_program

    effective, report = materialize_program(settings, selector)
    scratch = _scratch_dir(effective, hypothesis)
    if scratch.exists():
        shutil.rmtree(scratch)
    shutil.copytree(effective.project_dir, scratch)
    return {
        "hypothesis": _slug(hypothesis),
        "overlay_dir": str(scratch),
        "reviewed_dir": str(effective.project_dir),
        "reviewed_status": report.get("status"),
    }


def discard_overlay(settings: Settings, selector: str, hypothesis: str) -> dict[str, Any]:
    """Delete the clone; the reviewed baseline was never opened for writing."""

    from .cache import materialize_program

    effective, _ = materialize_program(settings, selector)
    scratch = _scratch_dir(effective, hypothesis)
    existed = scratch.exists()
    if existed:
        shutil.rmtree(scratch)
    return {"hypothesis": _slug(hypothesis), "discarded": existed}


def _reviewed_vtable(repo: Path, class_name: str) -> tuple[dict[str, Any], list[dict[str, str]]]:
    with (repo / "evidence" / "reviewed" / "wiz8" / "vtables.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        vtables = [
            row
            for row in csv.DictReader(stream)
            if row["class_name"] == class_name and row["kind"] == "primary"
        ]
    if not vtables:
        raise ValueError(f"no reviewed primary vtable for {class_name}")
    vtable = vtables[0]
    with (repo / "evidence" / "reviewed" / "wiz8" / "vtable-slots.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        slots = sorted(
            (row for row in csv.DictReader(stream) if row["vtable_id"] == vtable["vtable_id"]),
            key=lambda row: int(row["slot_index"]),
        )
    return vtable, slots


def _reviewed_vtables(
    repo: Path, class_name: str
) -> list[tuple[dict[str, Any], list[dict[str, str]]]]:
    """Every reviewed table of a class, primary and secondary."""

    reviewed = repo / "evidence" / "reviewed" / "wiz8"
    with (reviewed / "vtables.csv").open(newline="", encoding="utf-8") as stream:
        tables = [row for row in csv.DictReader(stream) if row["class_name"] == class_name]
    if not tables:
        raise ValueError(f"no reviewed vtable for {class_name}")
    with (reviewed / "vtable-slots.csv").open(newline="", encoding="utf-8") as stream:
        all_slots = list(csv.DictReader(stream))
    return [
        (
            table,
            sorted(
                (row for row in all_slots if row["vtable_id"] == table["vtable_id"]),
                key=lambda row: int(row["slot_index"]),
            ),
        )
        for table in sorted(tables, key=lambda row: int(row["subobject_offset"], 0))
    ]


def apply_typed_vtable(
    settings: Settings, selector: str, hypothesis: str, class_name: str
) -> dict[str, Any]:
    """Build the per-slot typed vtable for one reviewed class, in the clone."""

    from .cache import materialize_program
    from .environment import start_pyghidra

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, hypothesis)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay {_slug(hypothesis)} does not exist; create it first")
    tables = _reviewed_vtables(settings.repo_dir, class_name)

    start_pyghidra(settings)
    import pyghidra
    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
        StructureDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    stats = {
        "class": class_name,
        "tables": len(tables),
        "slots": sum(len(slots) for _table, slots in tables),
        "typed_targets": 0,
        "prototype_sources": {},
    }
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("typed vtable overlay")
            try:
                dtm = program.getDataTypeManager()
                category = CategoryPath("/wiz8/overlay")
                class_type = None
                for existing in dtm.getAllStructures():
                    if existing.getName() == class_name:
                        class_type = existing
                        break
                if class_type is None:
                    class_type = dtm.addDataType(
                        StructureDataType(category, class_name, 4),
                        DataTypeConflictHandler.KEEP_HANDLER,
                    )
                functions = program.getFunctionManager()
                space = program.getAddressFactory().getDefaultAddressSpace()
                targets: dict[str, dict[str, Any]] = {}
                primary_id = tables[0][0]["vtable_id"]
                for table_row, slots in tables:
                    subobject = int(table_row["subobject_offset"], 0)
                    suffix = "" if table_row["vtable_id"] == primary_id else f"_{subobject:x}"
                    receiver_type = class_type
                    if subobject:
                        receiver_type = dtm.addDataType(
                            StructureDataType(category, f"{class_name}_subobject_{subobject:x}", 4),
                            DataTypeConflictHandler.KEEP_HANDLER,
                        )
                    receiver = PointerDataType(receiver_type, dtm)
                    table = StructureDataType(category, f"{class_name}_vtable{suffix}", 0)
                    for row in slots:
                        index = int(row["slot_index"])
                        name = row["slot_name"] or f"slot{index}"
                        target = functions.getFunctionAt(space.getAddress(row["target"]))
                        definition, source = _slot_prototype(
                            program,
                            target,
                            receiver,
                            category,
                            f"{class_name}{suffix}_{name}",
                            ParameterDefinitionImpl,
                            FunctionDefinitionDataType,
                        )
                        stats["prototype_sources"][source] = (
                            stats["prototype_sources"].get(source, 0) + 1
                        )
                        added = dtm.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
                        table.add(PointerDataType(added, dtm), 4, name, None)
                        if target is not None:
                            target_key = str(target.getEntryPoint())
                            entry = targets.setdefault(
                                target_key, {"target": target, "receivers": {}}
                            )
                            entry["receivers"][receiver.getDisplayName()] = receiver
                    table_type = dtm.addDataType(table, DataTypeConflictHandler.REPLACE_HANDLER)
                    from .candidate_facts import stamp

                    stamp(
                        program,
                        space.getAddress(table_row["address"]),
                        hypothesis=hypothesis,
                        fact_id=f"typed-vtable:{table_row['vtable_id']}",
                        depends_on=[
                            f"vtable-slot:{row['slot_index']}:{row['target']}" for row in slots
                        ],
                        constraints={
                            "class": class_name,
                            "subobject_offset": table_row["subobject_offset"],
                            "slots": len(slots),
                            "prototype_policy": (
                                "reviewed, reconstructed, imported, compatible current, "
                                "receiver-only fallback"
                            ),
                        },
                    )
                    if subobject:
                        receiver_type.replaceAtOffset(
                            0,
                            PointerDataType(table_type, dtm),
                            4,
                            "vptr",
                            "typed secondary vtable overlay",
                        )
                    if subobject + 4 <= class_type.getLength():
                        component = class_type.getComponentAt(subobject)
                        field_name = (
                            component.getFieldName() if component is not None else None
                        ) or ("vptr" if subobject == 0 else f"base_{subobject:x}")
                        field_type = (
                            PointerDataType(table_type, dtm) if subobject == 0 else receiver_type
                        )
                        class_type.replaceAtOffset(
                            subobject,
                            field_type,
                            4,
                            field_name,
                            "typed vtable overlay; base-subobject receiver at this offset",
                        )

                # Each slot target's receiver is retyped in place. Replacing
                # the parameter list would discard the decompiler's recovered
                # arguments - measured: slot 0x004BF0F0 lost three of four - so
                # only the first parameter's type is set, and a function with
                # none gets one added rather than having its list rewritten.
                for entry in targets.values():
                    target = entry["target"]
                    receivers = list(entry["receivers"].values())
                    if len(receivers) != 1:
                        stats.setdefault("shared_slot_bodies", 0)
                        stats["shared_slot_bodies"] += 1
                        continue
                    receiver = receivers[0]
                    if target.getCallingConventionName() != "__thiscall":
                        target.setCallingConvention("__thiscall")
                    # __thiscall gives the function an auto-parameter for the
                    # receiver, which cannot be modified; the class's own type
                    # reaches the decompiler through the vtable structure and
                    # the slot's function definition instead. Explicit first
                    # parameters are retyped, auto ones are left alone.
                    explicit = [
                        parameter
                        for parameter in target.getParameters()
                        if not parameter.isAutoParameter()
                    ]
                    if explicit and explicit[0].getDataType().getLength() == 4:
                        explicit[0].setDataType(receiver, SourceType.ANALYSIS)
                        stats["typed_targets"] += 1
                    else:
                        stats.setdefault("auto_receiver", 0)
                        stats["auto_receiver"] += 1
                program.endTransaction(transaction, True)
            except Exception:
                program.endTransaction(transaction, False)
                raise
            program.save("typed vtable overlay", None)
    finally:
        from .semantic import dispose_sessions

        dispose_sessions()
        project.close()
    stats["vtable_ids"] = [table["vtable_id"] for table, _slots in tables]
    return stats


def _slot_prototype(
    program: Any,
    target: Any | None,
    receiver: Any,
    category: Any,
    name: str,
    parameter_definition: Any,
    function_definition: Any,
) -> tuple[Any, str]:
    """Copy the strongest current target prototype, replacing only ``this``.

    Reviewed signatures and reconstructed/imported signatures already live on
    the function and therefore win naturally.  Where the ProgramDB signature
    is still default, HighFunction contributes its recovered return and
    arguments.  The receiver is the only evidence-backed replacement; every
    other argument survives.
    """

    from ghidra.program.model.data import VoidDataType

    definition = function_definition(category, name)
    if target is None:
        definition.setReturnType(VoidDataType.dataType)
        definition.setArguments([parameter_definition("this", receiver, None)])
        return definition, "receiver-only-fallback"

    property_manager = program.getUsrPropertyManager()
    reconstructed = property_manager.getStringPropertyMap("wiz8.reconstructed")
    layer = property_manager.getStringPropertyMap("wiz8.layer")
    address = target.getEntryPoint()
    if target.isExternal():
        source = "imported"
    elif reconstructed is not None and reconstructed.hasProperty(address):
        source = "reconstructed"
    elif layer is not None and layer.hasProperty(address):
        source = "reviewed-or-current"
    else:
        source = "current-inferred"

    return_type = target.getReturnType()
    explicit = [
        parameter for parameter in target.getParameters() if not parameter.isAutoParameter()
    ]
    arguments = [parameter_definition("this", receiver, None)]
    if explicit and str(target.getSignatureSource()) != "DEFAULT":
        arguments.extend(
            parameter_definition(
                parameter.getName() or f"argument{index}", parameter.getDataType(), None
            )
            for index, parameter in enumerate(explicit, start=1)
        )
    else:
        try:
            from .semantic import _high_function

            prototype = _high_function(program, target).getFunctionPrototype()
            return_type = prototype.getReturnType() or return_type
            for index in range(prototype.getNumParams()):
                parameter = prototype.getParam(index)
                storage = str(parameter.getStorage())
                if "ECX" in storage.upper():
                    continue
                arguments.append(
                    parameter_definition(
                        parameter.getName() or f"argument{index + 1}",
                        parameter.getDataType(),
                        None,
                    )
                )
        except Exception:  # noqa: BLE001,S110 - the fallback remains receiver-only
            pass
    if return_type is None:
        return_type = VoidDataType.dataType
    definition.setReturnType(return_type)
    definition.setArguments(arguments)
    try:
        definition.setCallingConvention("__thiscall")
    except Exception:  # noqa: BLE001,S110 - older Ghidra accepts the prototype without it
        pass
    return definition, source


def decompile_in_overlay(
    settings: Settings, selector: str, hypothesis: str, address: str
) -> dict[str, Any]:
    """One function's C in the clone, for measuring a hypothesis's effect."""

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .query import execute_query

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, hypothesis)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay {_slug(hypothesis)} does not exist; create it first")

    start_pyghidra(settings)
    import pyghidra

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            return execute_query(program, "decompile", [address])
    finally:
        from .semantic import dispose_sessions

        dispose_sessions()
        project.close()


def facts_in_overlay(
    settings: Settings, selector: str, hypothesis: str, address: str
) -> dict[str, Any]:
    """Candidate and reviewed provenance at an anchor in the clone."""

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .query import execute_query

    effective, _ = materialize_program(settings, selector)
    overlay = _overlay_settings(effective, hypothesis)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay {_slug(hypothesis)} does not exist; create it first")
    start_pyghidra(settings)
    import pyghidra

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            return execute_query(program, "facts-at", [address])
    finally:
        project.close()


def dependency_cone(program: Any, repo: Path, class_name: str) -> dict[str, list[str]]:
    """ProgramDB-derived propagation cone for every table of one class."""

    from .dependency_graph import build_program_graph

    tables = _reviewed_vtables(repo, class_name)
    table_addresses = {int(table["address"], 16) for table, _slots in tables}
    graph = build_program_graph(program, table_addresses)
    for table, slots in tables:
        for slot in slots:
            if slot["target"]:
                graph.add(table["address"], slot["target"], "vtable-slot")
    return graph.cone([table["address"] for table, _slots in tables])


def measure_impact(
    settings: Settings, selector: str, hypothesis: str, class_name: str
) -> dict[str, Any]:
    """Redecompile the cone in baseline and overlay; report what changed.

    This is the propagation measurement the whole overlay exists for: one
    applied hypothesis, and the count of functions whose decompilation the
    program now renders differently. A hypothesis that changes nothing is a
    hypothesis worth discarding.
    """

    import contextlib
    import hashlib

    from .cache import materialize_program
    from .environment import start_pyghidra
    from .query import execute_query

    effective, _ = materialize_program(settings, selector)
    if not _scratch_dir(effective, hypothesis).exists():
        raise ValueError(f"overlay {_slug(hypothesis)} does not exist; create it first")
    overlay = _overlay_settings(effective, hypothesis)
    program_name = resolve_program_name(effective, selector)
    start_pyghidra(settings)
    import pyghidra

    # The overlay owns candidate computed references, so its ProgramDB is the
    # authoritative graph for the cone.  The baseline is only a semantic
    # comparison endpoint and never supplies stale snapshot ownership.
    graph_project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    try:
        with pyghidra.program_context(graph_project, "/" + program_name) as graph_program:
            cone = dependency_cone(graph_program, settings.repo_dir, class_name)
    finally:
        graph_project.close()
    addresses = sorted({address for group in cone.values() for address in group})

    def digests(project_dir: Path, project_name: str) -> dict[str, str]:
        project = pyghidra.open_project(project_dir, project_name, create=False)
        try:
            with pyghidra.program_context(project, "/" + program_name) as program:
                out = {}
                for address in addresses:
                    # A cone address that is not a function in this program -
                    # a thunk the census names, say - is simply not measured;
                    # both sides skip it identically, so the diff stays honest.
                    with contextlib.suppress(Exception):
                        result = execute_query(program, "decompile", ["0x" + address])
                        body = result.get("decompiled") or ""
                        out[address] = hashlib.sha256(body.encode()).hexdigest()
                return out
        finally:
            from .semantic import dispose_sessions

            dispose_sessions()
            project.close()

    before = digests(effective.project_dir, effective.project_name)
    after = digests(overlay.project_dir, overlay.project_name)
    changed = sorted(
        address for address in before if address in after and before[address] != after[address]
    )
    reasons = {
        address: [reason for reason, group in cone.items() if address in group]
        for address in changed
    }
    return {
        "class": class_name,
        "hypothesis": _slug(hypothesis),
        "cone": {reason: len(group) for reason, group in cone.items()},
        "cone_size": len(addresses),
        "decompiled": len(before),
        "changed": len(changed),
        "changed_functions": {address: reasons[address] for address in changed[:40]},
    }
