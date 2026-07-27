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
            (
                row
                for row in csv.DictReader(stream)
                if row["vtable_id"] == vtable["vtable_id"]
            ),
            key=lambda row: int(row["slot_index"]),
        )
    return vtable, slots


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
    vtable, slots = _reviewed_vtable(settings.repo_dir, class_name)

    start_pyghidra(settings)
    import pyghidra
    from ghidra.program.model.data import (
        CategoryPath,
        DataTypeConflictHandler,
        FunctionDefinitionDataType,
        ParameterDefinitionImpl,
        PointerDataType,
        StructureDataType,
        VoidDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(overlay, selector)
    project = pyghidra.open_project(overlay.project_dir, overlay.project_name, create=False)
    stats = {"class": class_name, "slots": len(slots), "typed_targets": 0}
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
                receiver = PointerDataType(class_type, dtm)

                table = StructureDataType(category, f"{class_name}_vtable", 0)
                functions = program.getFunctionManager()
                space = program.getAddressFactory().getDefaultAddressSpace()
                targets = []
                for row in slots:
                    index = int(row["slot_index"])
                    name = row["slot_name"] or f"slot{index}"
                    definition = FunctionDefinitionDataType(category, f"{class_name}_{name}")
                    definition.setReturnType(VoidDataType.dataType)
                    definition.setArguments(
                        [ParameterDefinitionImpl("this", receiver, None)]
                    )
                    added = dtm.addDataType(definition, DataTypeConflictHandler.REPLACE_HANDLER)
                    table.add(PointerDataType(added, dtm), 4, name, None)
                    target = functions.getFunctionAt(space.getAddress(row["target"]))
                    if target is not None:
                        targets.append((target, name))
                table_type = dtm.addDataType(table, DataTypeConflictHandler.REPLACE_HANDLER)

                # The class's vptr now points at the typed table.
                if class_type.getNumComponents() > 0:
                    class_type.replaceAtOffset(
                        0, PointerDataType(table_type, dtm), 4, "vptr", "typed vtable overlay"
                    )

                # Each slot target's receiver is retyped in place. Replacing
                # the parameter list would discard the decompiler's recovered
                # arguments - measured: slot 0x004BF0F0 lost three of four - so
                # only the first parameter's type is set, and a function with
                # none gets one added rather than having its list rewritten.
                for target, _name in targets:
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
        project.close()
    stats["vtable_id"] = vtable["vtable_id"]
    return stats


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


def dependency_cone(repo: Path, program: str, class_name: str) -> dict[str, list[str]]:
    """The functions a change to one class can reach, by the reason each is in.

    Retyping a class touches more than the class: its vtable slot targets get
    a receiver, everything that calls one of those sees a changed signature,
    and every function that installs one of its vtables handles the object
    directly. Those three sets are the cone a fixpoint has to redecompile, and
    keeping the reason with each address is what makes the result reviewable.
    """

    import csv as _csv
    from collections import defaultdict as _defaultdict

    reviewed = repo / "evidence" / "reviewed" / "wiz8"
    with (reviewed / "vtables.csv").open(newline="", encoding="utf-8") as stream:
        tables = {
            row["vtable_id"]: row["address"]
            for row in _csv.DictReader(stream)
            if row["class_name"] == class_name
        }
    with (reviewed / "vtable-slots.csv").open(newline="", encoding="utf-8") as stream:
        targets = {
            row["target"]
            for row in _csv.DictReader(stream)
            if row["vtable_id"] in tables and row["target"]
        }
    with (
        repo / "evidence" / "snapshots" / "polymorphism" / "vptr-writes.csv"
    ).open(newline="", encoding="utf-8") as stream:
        writers = {
            row["function_start"]
            for row in _csv.DictReader(stream)
            if row["program"] == program and row["vtable"] in set(tables.values())
        }
    with (
        repo / "evidence" / "snapshots" / "functions" / "calls.csv"
    ).open(newline="", encoding="utf-8") as stream:
        callers = {
            row["caller"]
            for row in _csv.DictReader(stream)
            if row["program"] == program and row["callee"] in targets
        }

    cone: dict[str, list[str]] = _defaultdict(list)
    for address in sorted(targets):
        cone["slot-target"].append(address)
    for address in sorted(writers):
        cone["vptr-writer"].append(address)
    for address in sorted(callers - targets - writers):
        cone["calls-slot-target"].append(address)
    return dict(cone)


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
    cone = dependency_cone(settings.repo_dir, program_name, class_name)
    addresses = sorted({address for group in cone.values() for address in group})

    start_pyghidra(settings)
    import pyghidra

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
