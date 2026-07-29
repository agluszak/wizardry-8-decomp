"""Thin Wizardry adapter for reccmp's compiler-backed source index."""

from __future__ import annotations

from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex

from .build import LINT_BUILD_DIR, VC6_IMAGE
from .config import Settings
from .subprocesses import resolve_executable

_SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})


def _source_paths(repository: Path, stem: str) -> tuple[Path, ...]:
    roots = (repository / f"src/{stem}", repository / f"include/{stem}")
    return tuple(
        sorted(
            path for root in roots for path in root.rglob("*") if path.suffix in _SOURCE_SUFFIXES
        )
    )


def build_source_index(settings: Settings) -> SourceIndex:
    """Replay clang-cl compile commands and join their AST with reccmp markers."""

    repository = settings.repo_dir.resolve()
    clang_build = repository / LINT_BUILD_DIR
    compilation_database = clang_build / "compile_commands.json"
    if not compilation_database.is_file():
        raise FileNotFoundError(
            f"{compilation_database} is missing; run `just lint` to generate it"
        )
    database_mtime = compilation_database.stat().st_mtime
    inventories = (
        repository / "src/wiz8/sources.cmake",
        repository / "src/surrender/CMakeLists.txt",
        repository / "CMakeLists.txt",
    )
    stale = [
        path for path in inventories if path.is_file() and path.stat().st_mtime > database_mtime
    ]
    if stale:
        raise RuntimeError(
            "the compile database predates "
            + ", ".join(path.relative_to(repository).as_posix() for path in stale)
            + "; run `just lint` to reconfigure it before indexing"
        )
    docker = resolve_executable("docker") or "docker"
    command_prefix = (
        docker,
        "run",
        "--rm",
        "--init",
        "--network",
        "none",
        "--volume",
        f"{repository}:/repo:ro",
        "--volume",
        f"{clang_build}:/out:ro",
        "--volume",
        f"{settings.work_dir / 'fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4'}:/zlib:ro",
        "--workdir",
        "/out",
        "--entrypoint",
        "/usr/bin/clang-cl",
        VC6_IMAGE,
    )
    return SourceIndex.from_compilation_database_targets(
        repository,
        compilation_database,
        {
            "WIZ8": _source_paths(repository, "wiz8"),
            "SURRENDER": _source_paths(repository, "surrender"),
        },
        command_prefix=command_prefix,
        compilation_root=Path("/repo"),
        execution_cwd=repository,
    )


def write_source_index(settings: Settings) -> dict[str, Any]:
    """Write the disposable canonical source projection."""

    index = build_source_index(settings)
    destination = settings.repo_dir / "build/source-index.json"
    index.write(destination)
    return {
        "path": destination.relative_to(settings.repo_dir).as_posix(),
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
    }
