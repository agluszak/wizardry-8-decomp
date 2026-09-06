"""Project paths and toolchain configuration for reccmp's source index."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex, SourceIndexError, SourceMarker

from .config import Settings

_TARGETS = {"WIZ8": "wiz8", "SURRENDER": "surrender"}


def load_source_index(repository: Path) -> dict[str, Any]:
    path = repository / "build/source-index.json"
    if not path.is_file():
        raise SourceIndexError(
            f"{path} is missing; run `just lint` then `wiz8 analyze source-index`"
        )
    document = json.loads(path.read_text(encoding="utf-8"))
    if document.get("schema") != "reccmp-source-index-v1":
        raise SourceIndexError(f"{path} has an unsupported source-index schema")
    return document


def target_for_program(program_name: str) -> str:
    normalized = program_name.casefold()
    return "SURRENDER" if "--sr--" in normalized or normalized == "surrender" else "WIZ8"


def source_functions(repository: Path, target: str = "WIZ8") -> dict[int, SourceMarker]:
    if target.upper() not in _TARGETS:
        raise SourceIndexError(f"unsupported source-index target: {target}")
    return SourceIndex.from_dict(load_source_index(repository)).functions_by_address(
        target=target.upper()
    )


def validate_source_index(repository: Path) -> dict[str, int]:
    index = SourceIndex.from_dict(load_source_index(repository))
    counts = {target: len(index.functions_by_address(target=target)) for target in _TARGETS}
    if len({item.semantic_id for item in index.classes}) != len(index.classes):
        raise SourceIndexError("compiler-backed source index contains duplicate class definitions")
    return {
        "functions": sum(counts.values()),
        "wiz8_functions": counts["WIZ8"],
        "surrender_functions": counts["SURRENDER"],
        "classes": len(index.classes),
        "vtable_classes": sum(item.vtable_address is not None for item in index.classes),
    }


def write_source_index(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    from .build import LINT_BUILD_DIR, VC6_IMAGE, configure_clang

    repository = settings.repo_dir.resolve()
    database = repository / LINT_BUILD_DIR / "compile_commands.json"
    inventories = tuple(
        repository / inventory
        for inventory in (
            "CMakeLists.txt",
            "src/wiz8/sources.cmake",
            "src/surrender/CMakeLists.txt",
        )
    )
    if not database.is_file() or any(
        path.is_file() and path.stat().st_mtime > database.stat().st_mtime for path in inventories
    ):
        configure_clang(settings)
    if not database.is_file():
        raise FileNotFoundError(f"clang configuration did not produce {database}")
    targets = {
        target: tuple(
            sorted(
                path
                for root in (repository / "src" / stem, repository / "include" / stem)
                for path in root.rglob("*")
                if path.suffix in {".c", ".cpp", ".h", ".hpp"}
            )
        )
        for target, stem in _TARGETS.items()
    }
    index = SourceIndex.from_compile_database(
        repository,
        database,
        targets,
        clang="/usr/bin/clang-cl",
        container_image=VC6_IMAGE,
        compilation_root=Path("/repo"),
        mounts={
            repository: "/repo",
            repository / LINT_BUILD_DIR: "/out",
            settings.work_dir / "fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4": "/zlib",
        },
        cache_dir=repository / "build/source-index-cache",
        cache_inputs=(
            *tuple(
                repository / path
                for path in (
                    "include",
                    "src",
                    "config",
                    "third_party/sfi-sgp/sgp",
                )
            ),
            settings.work_dir / "fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4",
        ),
        force=force,
    )
    index.write(repository / "build/source-index.json")
    return {
        "path": "build/source-index.json",
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
    }
