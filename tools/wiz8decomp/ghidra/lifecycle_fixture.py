"""Pinned-VC6 lifecycle recovery self-test in a disposable Ghidra project."""

from __future__ import annotations

import json
import re
import shutil
import tempfile
from pathlib import Path
from typing import Any

from .. import subprocesses
from ..config import Settings
from ..paths import atomic_json
from .fid_seeds import (
    _docker_cmake_build,
    load_static_libraries,
    select_toolchains,
)

_ROUND_TRIP_CLASSES = {"EmptyLifecycleA", "EmptyLifecycleB"}
_CLASSES = {
    "Base",
    "Secondary",
    "Derived",
    "VirtualDerived",
    "ClassDelete",
    "DeletesMember",
    *_ROUND_TRIP_CLASSES,
}

_GRAFT_SITES = {
    "EmptyLifecycleA::EmptyLifecycleA()": (
        "EmptyLifecycleA::EmptyLifecycleA() { lifecycle_sink += 10; }",
        "EmptyLifecycleA::EmptyLifecycleA()",
    ),
    "EmptyLifecycleA::~EmptyLifecycleA()": (
        "EmptyLifecycleA::~EmptyLifecycleA() { lifecycle_sink -= 10; }",
        "EmptyLifecycleA::~EmptyLifecycleA()",
    ),
    "EmptyLifecycleB::EmptyLifecycleB()": (
        "EmptyLifecycleB::EmptyLifecycleB() { lifecycle_sink += 20; }",
        "EmptyLifecycleB::EmptyLifecycleB()",
    ),
    "EmptyLifecycleB::~EmptyLifecycleB()": (
        "EmptyLifecycleB::~EmptyLifecycleB() { lifecycle_sink -= 20; }",
        "EmptyLifecycleB::~EmptyLifecycleB()",
    ),
}


def _build_fixture(
    settings: Settings,
    toolchain: Any,
    output: Path,
    *,
    source: Path | None = None,
    output_name: str = "lifecycle_probe",
) -> tuple[Path, Path]:
    if output.exists():
        shutil.rmtree(output)
    definitions = {"LIFECYCLE_OUTPUT_NAME": output_name}
    mounts = None
    if source is not None:
        definitions["LIFECYCLE_SOURCE"] = "Z:/sources/recovered/lifecycle.cpp"
        mounts = {"recovered": source.parent}
    _docker_cmake_build(
        settings,
        toolchain,
        output=output,
        source_dir="tools/recovery-fixture",
        target="recovery-lifecycle",
        definitions=definitions,
        source_mounts=mounts,
        log_name=f"cmake-recovery-lifecycle-{output_name}-vc6-sp5",
    )
    executable = output / f"{output_name}.exe"
    pdb = output / f"{output_name}.pdb"
    if not executable.is_file() or not pdb.is_file():
        raise RuntimeError("VC6 lifecycle self-test did not produce its executable and PDB")
    return executable, pdb


def _recover_image(
    settings: Settings, executable: Path, pdb: Path, *, label: str
) -> tuple[dict[str, Any], list[str]]:
    headless = settings.ghidra_install_dir / "support/analyzeHeadless"
    scripts = settings.repo_dir / "tools/ghidra-scripts"
    with tempfile.TemporaryDirectory(prefix=f"lifecycle-{label}-", dir=settings.build_dir) as raw:
        temporary = Path(raw)
        fixture = temporary / executable.name
        shutil.copy2(executable, fixture)
        shutil.copy2(pdb, temporary / pdb.name)
        source_index = temporary / "source-index.json"
        source_index.write_text('{"markers": []}\n', encoding="utf-8")
        result_path = temporary / "result.json"
        symbols_path = temporary / "symbols.json"
        (temporary / "project").mkdir()
        subprocesses.run(
            [
                headless,
                temporary / "project",
                "lifecycle-fixture",
                "-import",
                fixture,
                "-overwrite",
                "-processor",
                "x86:LE:32:default",
                "-cspec",
                "windows",
                "-scriptPath",
                scripts,
                "-postScript",
                "Wiz8Recover.java",
                "--source-index",
                source_index,
                "--output",
                result_path,
                "--all-functions",
            ],
            cwd=settings.repo_dir,
            log_path=settings.build_dir / "logs" / f"Wiz8RecoverSelfTest-{label}.json",
        )
        subprocesses.run(
            [
                headless,
                temporary / "project",
                "lifecycle-fixture",
                "-process",
                fixture.name,
                "-readOnly",
                "-noanalysis",
                "-scriptPath",
                scripts,
                "-postScript",
                "Wiz8Audit.java",
                "--audit",
                "lifecycle-symbols",
                *[value for name in sorted(_ROUND_TRIP_CLASSES) for value in ("--class", name)],
                "--output",
                symbols_path,
            ],
            cwd=settings.repo_dir,
            log_path=settings.build_dir / "logs" / f"Wiz8AuditLifecycle-{label}.json",
        )
        recovered = json.loads(result_path.read_text(encoding="utf-8"))
        symbols = json.loads(symbols_path.read_text(encoding="utf-8"))["vtables"]
    return recovered, sorted(symbols)


def _graft_round_trip_bodies(recovered: dict[str, Any], destination: Path) -> list[str]:
    bodies = {
        item["recovery"]["source_entity"]: item["body"]
        for item in recovered["exports"]
        if item["recovery"]["emission_kind"] == "authored_body" and item.get("body")
    }
    missing = sorted(set(_GRAFT_SITES) - set(bodies))
    if missing:
        raise RuntimeError("round-trip recovery missed authored entities: " + ", ".join(missing))
    source = (destination.parents[2] / "tests/recovery/lifecycle.cpp").read_text(encoding="utf-8")
    grafted: list[str] = []
    for entity, (original, prefix) in _GRAFT_SITES.items():
        body = str(bodies[entity])
        if (
            re.fullmatch(r"\s*\{(?:\s*lifecycle_sink\s*=\s*[^;]+;)?\s*return;\s*\}\s*", body)
            is None
        ):
            raise RuntimeError(f"recovered {entity} body is not directly compilable: {body!r}")
        if source.count(original) != 1:
            raise RuntimeError(f"round-trip graft site is not unique: {entity}")
        source = source.replace(original, prefix + body)
        grafted.append(entity)
    destination.write_text(source, encoding="utf-8")
    return grafted


def _lifecycle_family(recovered: dict[str, Any]) -> list[tuple[str, str, str]]:
    return sorted(
        (
            str(item.get("namespace", "")).rsplit("::", 1)[-1],
            str(item["recovery"]["source_entity"]),
            str(item["recovery"]["emission_kind"]),
        )
        for item in recovered["exports"]
        if str(item.get("namespace", "")).rsplit("::", 1)[-1] in _ROUND_TRIP_CLASSES
    )


def verify_lifecycle_fixture(settings: Settings) -> dict[str, Any]:
    """Build, import, and recover the lifecycle fixture without touching reviewed state."""

    toolchain = select_toolchains(
        load_static_libraries(settings), ["vc6-sp5"], capability="compiler"
    )[0]
    output = settings.build_dir / "recovery-fixture/vc6-sp5"
    executable, pdb = _build_fixture(settings, toolchain, output)
    recovered, original_vtables = _recover_image(settings, executable, pdb, label="original")
    atomic_json(settings.build_dir / "reports/recovery-lifecycle-functions.json", recovered)

    exports = recovered["exports"]
    namespaces = {str(item.get("namespace", "")).rsplit("::", 1)[-1] for item in exports}
    missing = sorted(_CLASSES - namespaces)
    if missing:
        raise RuntimeError(f"lifecycle PDB import missed classes: {', '.join(missing)}")

    wrappers: list[str] = []
    authored_by_entity: dict[str, int] = {}
    for item in exports:
        recovery = item["recovery"]
        emission = str(recovery["emission_kind"]).upper()
        entity = str(recovery["source_entity"])
        if item.get("body") and recovery["source_kind"] in {"constructor", "destructor"}:
            authored_by_entity[entity] = authored_by_entity.get(entity, 0) + 1
        if emission in {"SCALAR_DELETING_DESTRUCTOR", "VECTOR_DELETING_DESTRUCTOR"}:
            if "// SYNTHETIC:" not in item["text"] or item["body"]:
                raise RuntimeError(f"{item.get('name')} owns a deleting-wrapper body")
            wrappers.append(emission)
    duplicates = sorted(entity for entity, count in authored_by_entity.items() if count > 1)
    if duplicates:
        raise RuntimeError("multiple authored lifecycle bodies: " + ", ".join(duplicates))
    if "SCALAR_DELETING_DESTRUCTOR" not in wrappers:
        raise RuntimeError("fixture produced no classified scalar deleting destructor")

    destroy = [
        item
        for item in exports
        if str(item.get("name", "")).endswith("destroy_and_free")
        and item["recovery"]["emission_kind"] == "authored_body"
        and item["body"]
    ]
    if len(destroy) != 1:
        raise RuntimeError("destroy_and_free was not retained as one authored body")
    virtual_constructors = [
        item
        for item in exports
        if item["recovery"]["source_kind"] == "constructor"
        and "VirtualDerived::VirtualDerived" in item["recovery"]["source_entity"]
    ]
    if any(
        "VirtualDerived::VirtualDerived()" not in item["recovery"]["source_entity"]
        for item in virtual_constructors
    ):
        raise RuntimeError("virtual-base hidden constructor argument leaked into source syntax")

    with tempfile.TemporaryDirectory(prefix="lifecycle-source-", dir=settings.build_dir) as raw:
        recovered_source = Path(raw) / "lifecycle.cpp"
        grafted = _graft_round_trip_bodies(recovered, recovered_source)
        recovered_output = settings.build_dir / "recovery-fixture/vc6-sp5-recovered"
        recovered_executable, recovered_pdb = _build_fixture(
            settings,
            toolchain,
            recovered_output,
            source=recovered_source,
            output_name="lifecycle_recovered",
        )
        recompiled, recovered_vtables = _recover_image(
            settings, recovered_executable, recovered_pdb, label="recompiled"
        )
    original_family = _lifecycle_family(recovered)
    recompiled_family = _lifecycle_family(recompiled)
    if recompiled_family != original_family:
        raise RuntimeError(
            f"recompiled lifecycle family differs: {recompiled_family!r} != {original_family!r}"
        )
    if recovered_vtables != original_vtables or not original_vtables:
        raise RuntimeError(
            f"recompiled vtable family differs: {recovered_vtables!r} != {original_vtables!r}"
        )

    report = {
        "schema": "wiz8.recovery-lifecycle-self-test",
        "classes": len(_CLASSES),
        "functions": len(exports),
        "deleting_wrappers": sorted(wrappers),
        "destroy_and_free": "authored_body",
        "unique_lifecycle_definitions": True,
        "round_trip": {
            "grafted_entities": grafted,
            "lifecycle_emissions": len(original_family),
            "vtables": original_vtables,
            "recompiled": True,
        },
    }
    atomic_json(settings.build_dir / "reports/recovery-lifecycle-self-test.json", report)
    return report
