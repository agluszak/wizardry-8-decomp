from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from ..config import Settings
from .apply_function_map import ACCEPTED_CONFIDENCE, load_function_identities
from .environment import start_pyghidra
from .import_programs import HASH_OPTION
from .observation_evidence import audit_observation_evidence
from .project import module_for_program, resolve_program_name
from .query_daemon import stop_daemon
from .apply_class_candidates import (
    function_resolver,
    interval_lookup,
    load_candidate_inputs,
    writer_comment_bodies,
)
from .reviewed_class_model import ghidra_namespace_name, load_reviewed_class_model, parse_pointee
from .reviewed_signatures import load_reviewed_signatures


def expected_pointee_display(pointee: str) -> str:
    """Ghidra display name of a pointer field with a reviewed pointee."""

    base, depth = parse_pointee(pointee)
    return base + " *" * (depth + 1)


def _expected_type_name(spec: str) -> str:
    aliases = {
        "unsigned char": "uchar",
        "unsigned int": "uint",
        "unsigned short": "ushort",
    }
    pointer_depth = 0
    spec = spec.strip()
    while spec.endswith("*"):
        pointer_depth += 1
        spec = spec[:-1].strip()
    return aliases.get(spec, spec) + " *" * pointer_depth


def _candidate_names(path: Path) -> list[tuple[int, str]]:
    candidates: list[tuple[int, str]] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            name = row["provisional_name"].strip()
            if name and row["confidence"].strip() not in ACCEPTED_CONFIDENCE:
                candidates.append((int(row["address"], 16), name))
    return candidates


def validate_reviewed_replay(
    settings: Settings,
    selector: str,
    *,
    evidence_program: str,
) -> dict[str, Any]:
    """Check the materialized Ghidra view against canonical reviewed evidence."""

    program_name = resolve_program_name(settings, selector)
    module = module_for_program(settings, program_name)
    directory = settings.repo_dir / "evidence" / "reviewed" / evidence_program
    function_path = directory / "functions.csv"
    identities = load_function_identities(function_path)
    signatures = load_reviewed_signatures(settings.repo_dir, evidence_program)
    class_model = load_reviewed_class_model(settings.repo_dir, evidence_program)

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.program.model.listing import CodeUnit

    failures: list[dict[str, str]] = []
    checks = {
        "binary_identity": 0,
        "functions": 0,
        "function_extents": 0,
        "signatures": 0,
        "vtables": 0,
        "vtable_slots": 0,
        "structures": 0,
        "fields": 0,
        "pointee_fields": 0,
        "candidate_structures": 0,
        "candidate_writer_comments": 0,
        "globals": 0,
        "candidate_names": 0,
        "observation_evidence": 0,
    }
    observation_audit: dict[str, Any] = {}

    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            options = program.getOptions("Program Information")
            actual_hash = options.getString(HASH_OPTION, None)
            checks["binary_identity"] += 1
            if actual_hash != module["sha256"]:
                failures.append(
                    {
                        "kind": "binary_identity",
                        "key": program_name,
                        "expected": module["sha256"],
                        "actual": str(actual_hash),
                    }
                )

            function_manager = program.getFunctionManager()
            address_space = program.getAddressFactory().getDefaultAddressSpace()
            listing = program.getListing()
            for identity in identities:
                address = address_space.getAddress(identity.address)
                function = function_manager.getFunctionAt(address)
                checks["functions"] += 1
                if function is None:
                    failures.append(
                        {
                            "kind": "function",
                            "key": identity.identity_id,
                            "expected": identity.name,
                            "actual": "missing",
                        }
                    )
                    continue
                expected_namespace, _, expected_name = identity.name.rpartition("::")
                # The replay spells namespaces the way Ghidra accepts them, so
                # compare against that spelling rather than the reviewed one --
                # a template instantiation over a pointer differs only by the
                # space the demangler puts before the star.
                expected_namespace = ghidra_namespace_name(expected_namespace)
                expected_name = ghidra_namespace_name(expected_name)
                actual_namespace = function.getParentNamespace().getName(True)
                if function.getName() != expected_name or (
                    expected_namespace and not actual_namespace.endswith(expected_namespace)
                ):
                    failures.append(
                        {
                            "kind": "function",
                            "key": identity.identity_id,
                            "expected": identity.name,
                            "actual": f"{actual_namespace}::{function.getName()}",
                        }
                    )
                comment = listing.getComment(CodeUnit.PLATE_COMMENT, address) or ""
                if identity.identity_id not in comment or (
                    identity.source_unit and f"Source unit: {identity.source_unit}" not in comment
                ):
                    failures.append(
                        {
                            "kind": "provenance",
                            "key": identity.identity_id,
                            "expected": "stable identity and source-unit comment",
                            "actual": comment,
                        }
                    )
                expected_size = identity.size
                if expected_size is not None:
                    checks["function_extents"] += 1
                    actual_size = function.getBody().getNumAddresses()
                    if actual_size != expected_size:
                        failures.append(
                            {
                                "kind": "function_extent",
                                "key": identity.identity_id,
                                "expected": str(expected_size),
                                "actual": str(actual_size),
                            }
                        )

            for address_value, candidate_name in _candidate_names(function_path):
                checks["candidate_names"] += 1
                function = function_manager.getFunctionAt(address_space.getAddress(address_value))
                if function is not None and function.getName() == candidate_name:
                    failures.append(
                        {
                            "kind": "candidate_promotion",
                            "key": f"0x{address_value:08x}",
                            "expected": "candidate name not primary",
                            "actual": candidate_name,
                        }
                    )

            for reviewed in signatures:
                checks["signatures"] += 1
                function = function_manager.getFunctionAt(
                    address_space.getAddress(reviewed.address)
                )
                if function is None:
                    failures.append(
                        {
                            "kind": "signature",
                            "key": reviewed.evidence_id,
                            "expected": "function",
                            "actual": "missing",
                        }
                    )
                    continue
                parameters = tuple(function.getParameters())
                actual_parameters = tuple(
                    (parameter.getName(), parameter.getDataType().getDisplayName())
                    for parameter in parameters
                    if not parameter.isAutoParameter()
                )
                expected_parameters = tuple(
                    (name, _expected_type_name(data_type))
                    for name, data_type in reviewed.parameters
                )
                if reviewed.this_type is not None:
                    expected_parameters = (
                        ("this", _expected_type_name(reviewed.this_type)),
                        *expected_parameters,
                    )
                actual = (
                    function.getCallingConventionName(),
                    function.getReturnType().getDisplayName(),
                    actual_parameters,
                    bool(function.hasVarArgs()),
                )
                expected = (
                    reviewed.calling_convention,
                    _expected_type_name(reviewed.return_type),
                    expected_parameters,
                    reviewed.variadic,
                )
                if actual != expected:
                    failures.append(
                        {
                            "kind": "signature",
                            "key": reviewed.evidence_id,
                            "expected": repr(expected),
                            "actual": repr(actual),
                        }
                    )
                if reviewed.this_type is not None:
                    this_parameter = next(
                        (parameter for parameter in parameters if parameter.getName() == "this"),
                        None,
                    )
                    actual_this_type = (
                        None
                        if this_parameter is None
                        else this_parameter.getDataType().getDisplayName()
                    )
                    expected_this_type = _expected_type_name(reviewed.this_type)
                    if actual_this_type != expected_this_type:
                        failures.append(
                            {
                                "kind": "this_type",
                                "key": reviewed.evidence_id,
                                "expected": expected_this_type,
                                "actual": str(actual_this_type),
                            }
                        )

            memory = program.getMemory()
            dtm = program.getDataTypeManager()
            slots_by_vtable: dict[str, list[Any]] = {}
            for slot in class_model.slots:
                slots_by_vtable.setdefault(slot.vtable_id, []).append(slot)
            for vtable in class_model.vtables:
                checks["vtables"] += 1
                address = address_space.getAddress(vtable.address)
                if vtable.slot_count is not None:
                    data = listing.getDataContaining(address)
                    expected_length = vtable.slot_count * 4
                    typed_through = (
                        None if data is None else data.getMaxAddress().subtract(address) + 1
                    )
                    if typed_through is None or typed_through < expected_length:
                        failures.append(
                            {
                                "kind": "vtable",
                                "key": vtable.vtable_id,
                                "expected": f"typed {expected_length}-byte table",
                                "actual": "missing"
                                if data is None
                                else f"typed through {typed_through} bytes",
                            }
                        )
                for slot in slots_by_vtable.get(vtable.vtable_id, []):
                    checks["vtable_slots"] += 1
                    actual_target = memory.getInt(address.add(slot.index * 4)) & 0xFFFFFFFF
                    if actual_target != slot.target:
                        failures.append(
                            {
                                "kind": "vtable_slot",
                                "key": f"{vtable.vtable_id}[{slot.index}]",
                                "expected": f"0x{slot.target:08x}",
                                "actual": f"0x{actual_target:08x}",
                            }
                        )

            fields_by_class: dict[str, list[Any]] = {}
            for field in class_model.fields:
                fields_by_class.setdefault(field.class_name, []).append(field)
            classes_by_name = {item.name: item for item in class_model.classes}
            for class_name, fields in fields_by_class.items():
                checks["structures"] += 1
                data_type = dtm.getDataType(f"/{evidence_program}/classes/{class_name}")
                expected_size = classes_by_name[class_name].size
                if data_type is None or data_type.getLength() != expected_size:
                    failures.append(
                        {
                            "kind": "structure",
                            "key": class_name,
                            "expected": str(expected_size),
                            "actual": "missing"
                            if data_type is None
                            else str(data_type.getLength()),
                        }
                    )
                    continue
                for field in fields:
                    checks["fields"] += 1
                    component = data_type.getComponentAt(field.offset)
                    actual = (
                        None
                        if component is None
                        else (component.getFieldName(), component.getLength())
                    )
                    expected = (field.name, field.size)
                    if actual != expected:
                        failures.append(
                            {
                                "kind": "field",
                                "key": f"{class_name}+0x{field.offset:x}",
                                "expected": repr(expected),
                                "actual": repr(actual),
                            }
                        )
                    if field.pointee and component is not None:
                        checks["pointee_fields"] += 1
                        actual_display = component.getDataType().getDisplayName()
                        expected_display = expected_pointee_display(field.pointee)
                        if actual_display != expected_display:
                            failures.append(
                                {
                                    "kind": "pointee",
                                    "key": f"{class_name}+0x{field.offset:x}",
                                    "expected": expected_display,
                                    "actual": str(actual_display),
                                }
                            )

            candidate_inputs = load_candidate_inputs(
                settings.repo_dir, program_name, function_resolver(program)
            )
            for skeleton in candidate_inputs["skeletons"]:
                checks["candidate_structures"] += 1
                data_type = dtm.getDataType(f"/wiz8/candidates/{skeleton['name']}")
                if data_type is None or data_type.getLength() != skeleton["size"]:
                    failures.append(
                        {
                            "kind": "candidate_structure",
                            "key": skeleton["name"],
                            "expected": str(skeleton["size"]),
                            "actual": "missing"
                            if data_type is None
                            else str(data_type.getLength()),
                        }
                    )
            from .observation_evidence import _has_owned_comment

            for writer in sorted(writer_comment_bodies(candidate_inputs["candidates"])):
                address = address_space.getAddress(writer)
                if listing.getCodeUnitAt(address) is None:
                    continue
                checks["candidate_writer_comments"] += 1
                if not _has_owned_comment(listing, address, "candidate-class"):
                    failures.append(
                        {
                            "kind": "candidate_writer_comment",
                            "key": f"0x{writer:08x}",
                            "expected": "candidate-class pre-comment",
                            "actual": "missing",
                        }
                    )

            unit_of = interval_lookup(candidate_inputs["intervals"])
            uncommented_unit_functions = 0
            function_iterator = program.getFunctionManager().getFunctions(True)
            while function_iterator.hasNext():
                function = function_iterator.next()
                entry = int(str(function.getEntryPoint()), 16)
                if unit_of(entry) is not None and not _has_owned_comment(
                    listing, function.getEntryPoint(), "translation-unit"
                ):
                    uncommented_unit_functions += 1
            observation_audit = audit_observation_evidence(program)
            checks["observation_evidence"] = (
                observation_audit["assertions"]["call_sites"]
                + observation_audit["eh"]["setups"]
                + observation_audit["polymorphism"]["slot_targets"]
                + observation_audit["globals"]["strict_scalar_candidates"]
                + observation_audit["runtime_class_names"]["call_sites"]
            )
            residuals = {
                "assertion_comments": observation_audit["assertions"]["without_comments"],
                "eh_comments": observation_audit["eh"]["without_comments"],
                "runtime_class_comments": observation_audit["runtime_class_names"][
                    "without_comments"
                ],
                "uncovered_vtable_targets": observation_audit["polymorphism"][
                    "uncovered_slot_targets"
                ],
                "undefined_vtable_slots": observation_audit["polymorphism"]["missing_table_slots"],
                "undefined_strict_scalars": observation_audit["globals"]["undefined"],
                "unit_functions_without_comment": uncommented_unit_functions,
            }
            for kind, count in residuals.items():
                if count:
                    failures.append(
                        {
                            "kind": "observation_replay",
                            "key": kind,
                            "expected": "0",
                            "actual": str(count),
                        }
                    )
    finally:
        project.close()

    return {
        "schema": "wiz8.ghidra-replay-validation",
        "program": program_name,
        "binary_sha256": module["sha256"],
        "ok": not failures,
        "checks": checks,
        "observations": observation_audit,
        "failure_count": len(failures),
        "failures": failures,
    }
