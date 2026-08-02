"""Transient VC6 lifecycle fixture recovery gate.

The fixture is compiler input, not repository evidence.  Every run rebuilds it
with the pinned toolchain, imports the executable/PDB into a disposable Ghidra
project, executes the real Java role/exporter code, and validates source-entity
ownership.  No live reviewed project or serialized side inventory is involved.
"""

from __future__ import annotations

import re
import shutil
import tempfile
from pathlib import Path
from typing import Any

from ..config import Settings
from ..paths import atomic_json, atomic_write
from .environment import start_pyghidra
from .export_cpp import _attach_jar, ensure_exporter_jar
from .fid_seeds import probe_toolchains

_CLASSES = ("Base", "Secondary", "Derived", "VirtualDerived", "ClassDelete", "DeletesMember")


def verify_lifecycle_fixture(settings: Settings) -> dict[str, Any]:
    """Run the pinned build -> Ghidra -> exporter source-ownership gate."""

    probe_toolchains(settings, ["vc6-sp5"])
    probe_dir = settings.work_dir / "fid/toolchain-probes/vc6-sp5"
    executable = probe_dir / "lifecycle_probe.exe"
    pdb = probe_dir / "lifecycle_probe.pdb"
    if not executable.is_file() or not pdb.is_file():
        raise RuntimeError("VC6 lifecycle fixture did not produce its executable and PDB")

    jar = ensure_exporter_jar(settings)
    start_pyghidra(settings, max_heap="4G")
    _attach_jar(jar)

    import jpype
    import pyghidra
    from ghidra.program.model.listing import GhidraClass
    from ghidra.util.task import TaskMonitor

    exporter = jpype.JClass("wiz8.exporter.Wiz8RecoveryExporter")
    role_resolver = jpype.JClass("wiz8.exporter.FunctionRoleResolver")

    settings.build_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="lifecycle-ghidra-", dir=settings.build_dir) as raw:
        temporary = Path(raw)
        fixture_exe = temporary / executable.name
        shutil.copy2(executable, fixture_exe)
        shutil.copy2(pdb, temporary / pdb.name)
        project_dir = temporary / "project"
        with pyghidra.open_program(
            fixture_exe,
            project_location=project_dir,
            project_name="lifecycle-fixture",
            analyze=True,
            language="x86:LE:32:default",
            compiler="windows",
            program_name="lifecycle_probe",
            nested_project_location=False,
        ) as flat_api:
            program = flat_api.getCurrentProgram()
            functions = list(program.getFunctionManager().getFunctions(True))
            atomic_json(
                settings.build_dir / "reports/lifecycle-fixture/functions.json",
                [
                    {
                        "address": function.getEntryPoint().getOffset(),
                        "name": str(function.getName(True)),
                        "role": str(role_resolver.resolve(function).emissionKind()),
                        "source": str(role_resolver.resolve(function).sourceKind()),
                        "body": bool(role_resolver.resolve(function).hasAuthoredBody()),
                    }
                    for function in functions
                ],
            )
            classes = {
                str(function.getParentNamespace().getName()): function.getParentNamespace()
                for function in functions
                if isinstance(function.getParentNamespace(), GhidraClass)
            }
            missing = sorted(set(_CLASSES) - classes.keys())
            if missing:
                raise RuntimeError(f"lifecycle PDB import missed classes: {', '.join(missing)}")

            exports: dict[str, str] = {}
            for name in _CLASSES:
                text = str(exporter.exportClass(program, name, TaskMonitor.DUMMY))
                report = settings.build_dir / "reports/lifecycle-fixture" / f"{name}.cpp"
                report.parent.mkdir(parents=True, exist_ok=True)
                atomic_write(report, text)
                _assert_unique_lifecycle_definitions(name, text)
                if re.search(
                    r"// FUNCTION:[^\n]*\n[^\n]*(?:scalar|vector) deleting destructor",
                    text,
                ):
                    raise RuntimeError(f"{name} promoted a deleting wrapper to FUNCTION")
                exports[name] = text

            destroy = [
                function for function in functions if function.getName() == "destroy_and_free"
            ]
            authored_destroy = [
                function
                for function in destroy
                if str(role_resolver.resolve(function).emissionKind()) == "AUTHORED_BODY"
                and role_resolver.resolve(function).hasAuthoredBody()
            ]
            if len(authored_destroy) != 1:
                raise RuntimeError(
                    f"expected one authored destroy_and_free, found {len(authored_destroy)}"
                )
            destroy_function = authored_destroy[0]
            destroy_role = role_resolver.resolve(destroy_function)
            if str(destroy_role.emissionKind()) != "AUTHORED_BODY":
                raise RuntimeError(
                    "destroy_and_free was incorrectly suppressed as a deleting wrapper"
                )
            block = str(
                exporter.exportFunctions(
                    program,
                    jpype.JArray(jpype.JLong)([destroy_function.getEntryPoint().getOffset()]),
                    TaskMonitor.DUMMY,
                )[0]
            )
            if "// FUNCTION:" not in block:
                raise RuntimeError("destroy_and_free did not retain an authored FUNCTION block")

            wrappers = []
            for function in functions:
                role = role_resolver.resolve(function)
                emission = str(role.emissionKind())
                if emission not in {"SCALAR_DELETING_DESTRUCTOR", "VECTOR_DELETING_DESTRUCTOR"}:
                    continue
                wrapper = str(
                    exporter.exportFunctions(
                        program,
                        jpype.JArray(jpype.JLong)([function.getEntryPoint().getOffset()]),
                        TaskMonitor.DUMMY,
                    )[0]
                )
                if "// SYNTHETIC:" not in wrapper:
                    raise RuntimeError(f"{function.getName(True)} owns a deleting-wrapper body")
                wrappers.append(emission)
            if "SCALAR_DELETING_DESTRUCTOR" not in wrappers:
                raise RuntimeError("fixture produced no classified scalar deleting destructor")

            virtual_constructor = exports["VirtualDerived"]
            generated_virtual_constructors = re.findall(
                r"VirtualDerived::VirtualDerived\(([^)]*)\)", virtual_constructor
            )
            if any(parameters.strip() for parameters in generated_virtual_constructors):
                raise RuntimeError(
                    "virtual-base hidden constructor argument leaked into source syntax"
                )

            return {
                "schema": "wiz8.lifecycle-fixture-roundtrip",
                "classes": len(exports),
                "functions": len(functions),
                "deleting_wrappers": sorted(wrappers),
                "destroy_and_free": "authored_body",
                "unique_lifecycle_definitions": True,
            }


def _assert_unique_lifecycle_definitions(class_name: str, text: str) -> None:
    """Final-output assertion only; role identity was established in Ghidra."""

    constructor = re.compile(rf"^(?:[^/\n].*\s)?{re.escape(class_name)}::{re.escape(class_name)}\(")
    destructor = re.compile(rf"^(?:[^/\n].*\s)?{re.escape(class_name)}::~{re.escape(class_name)}\(")
    constructor_lines = [line for line in text.splitlines() if constructor.search(line)]
    destructor_lines = [line for line in text.splitlines() if destructor.search(line)]
    if len(constructor_lines) != len(set(constructor_lines)):
        raise RuntimeError(f"{class_name} exported duplicate constructor source signatures")
    if len(destructor_lines) != 1:
        raise RuntimeError(
            f"{class_name} exported {len(destructor_lines)} destructor definitions, expected one"
        )
