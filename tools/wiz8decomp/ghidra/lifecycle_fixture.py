"""Pinned-VC6 lifecycle recovery self-test in a disposable Ghidra project."""

from __future__ import annotations

import json
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

_CLASSES = {"Base", "Secondary", "Derived", "VirtualDerived", "ClassDelete", "DeletesMember"}


def verify_lifecycle_fixture(settings: Settings) -> dict[str, Any]:
    """Build, import, and recover the lifecycle fixture without touching reviewed state."""

    toolchain = select_toolchains(
        load_static_libraries(settings), ["vc6-sp5"], capability="compiler"
    )[0]
    output = settings.build_dir / "recovery-fixture/vc6-sp5"
    if output.exists():
        shutil.rmtree(output)
    _docker_cmake_build(
        settings,
        toolchain,
        output=output,
        source_dir="tools/recovery-fixture",
        target="recovery-lifecycle",
        definitions={},
        log_name="cmake-recovery-lifecycle-vc6-sp5",
    )
    executable = output / "lifecycle_probe.exe"
    pdb = output / "lifecycle_probe.pdb"
    if not executable.is_file() or not pdb.is_file():
        raise RuntimeError("VC6 lifecycle self-test did not produce its executable and PDB")

    headless = settings.ghidra_install_dir / "support/analyzeHeadless"
    scripts = settings.repo_dir / "tools/ghidra-scripts"
    with tempfile.TemporaryDirectory(prefix="lifecycle-ghidra-", dir=settings.build_dir) as raw:
        temporary = Path(raw)
        fixture = temporary / executable.name
        shutil.copy2(executable, fixture)
        shutil.copy2(pdb, temporary / pdb.name)
        source_index = temporary / "source-index.json"
        source_index.write_text('{"markers": []}\n', encoding="utf-8")
        result_path = temporary / "result.json"
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
            log_path=settings.build_dir / "logs/Wiz8RecoverSelfTest.json",
        )
        recovered = json.loads(result_path.read_text(encoding="utf-8"))
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

    report = {
        "schema": "wiz8.recovery-lifecycle-self-test",
        "classes": len(_CLASSES),
        "functions": len(exports),
        "deleting_wrappers": sorted(wrappers),
        "destroy_and_free": "authored_body",
        "unique_lifecycle_definitions": True,
    }
    atomic_json(settings.build_dir / "reports/recovery-lifecycle-self-test.json", report)
    return report
