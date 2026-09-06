"""Project paths and toolchain configuration for reccmp's source index."""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex, SourceIndexError, SourceMarker

from .config import Settings

_INDEXED_TARGETS = ("WIZ8", "SURRENDER")
_SOURCE_SUFFIXES = frozenset({".c", ".cpp", ".h", ".hpp"})
_SYNTHETIC_MARKER = re.compile(r"^\s*//\s*SYNTHETIC:\s+")
_SOURCE_MARKER = re.compile(r"^\s*//\s*(?:FUNCTION|TEMPLATE|SYNTHETIC|LIBRARY|VTABLE|GLOBAL):\s+")


def validate_synthetic_marker_blocks(repository: Path) -> int:
    """Require marker-only synthetic identities with an explicit block end."""
    failures: list[str] = []
    count = 0
    for root_name in ("src", "include"):
        root = repository / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in _SOURCE_SUFFIXES:
                continue
            lines = path.read_text(encoding="utf-8").splitlines()
            for index, line in enumerate(lines):
                if not _SYNTHETIC_MARKER.match(line):
                    continue
                count += 1
                location = f"{path.relative_to(repository)}:{index + 1}"
                if index + 1 >= len(lines) or not lines[index + 1].lstrip().startswith("//"):
                    failures.append(f"{location}: SYNTHETIC lacks its identity comment")
                    continue
                if index + 2 >= len(lines):
                    continue
                following = lines[index + 2]
                if following.strip() and not _SOURCE_MARKER.match(following):
                    failures.append(
                        f"{location}: SYNTHETIC owns no declaration or body; "
                        "end the marker block before the next source entity"
                    )
    if failures:
        raise SourceIndexError("invalid SYNTHETIC marker blocks:\n" + "\n".join(failures))
    return count


def project_targets(repository: Path) -> dict[str, dict[str, Any]]:
    import yaml

    document = yaml.safe_load((repository / "reccmp-project.yml").read_text(encoding="utf-8"))
    return {str(name).upper(): value for name, value in (document.get("targets") or {}).items()}


def _source_roots(config: dict[str, Any]) -> tuple[str, ...]:
    value = config.get("source-root", ())
    return (value,) if isinstance(value, str) else tuple(value)


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


def target_for_program(repository: Path, program_name: str) -> str:
    """Resolve a target name, binary filename, or content-addressed Ghidra program."""

    targets = project_targets(repository)
    normalized = program_name.casefold()
    direct = [
        target
        for target, config in targets.items()
        if normalized in {target.casefold(), str(config["filename"]).casefold()}
    ]
    if len(direct) == 1:
        return direct[0]

    match = re.fullmatch(r"wiz8--.+--(.+)--[0-9a-f]{12}", normalized)
    if match:
        stem = match.group(1)
        associated = [
            target
            for target, config in targets.items()
            if re.sub(r"[^a-z0-9]+", "-", Path(config["filename"]).stem.casefold()).strip("-")
            == stem
        ]
        if len(associated) == 1:
            return associated[0]
    raise SourceIndexError(f"program has no configured reccmp target: {program_name}")


def source_functions(repository: Path, target: str = "WIZ8") -> dict[int, SourceMarker]:
    return SourceIndex.from_dict(load_source_index(repository)).functions_by_address(
        target=target.upper()
    )


def validate_source_index(repository: Path) -> dict[str, int]:
    validate_synthetic_marker_blocks(repository)
    index = SourceIndex.from_dict(load_source_index(repository))
    counts = {
        target: len(index.functions_by_address(target=target))
        for target in project_targets(repository)
    }
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
    validate_synthetic_marker_blocks(repository)
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
                for source_root in _source_roots(project_targets(repository)[target])
                for path in (repository / source_root).rglob("*")
                if path.suffix in _SOURCE_SUFFIXES
            )
        )
        for target in _INDEXED_TARGETS
        if target in project_targets(repository)
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
