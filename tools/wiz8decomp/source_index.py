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
            if path.suffix.lower() not in _SOURCE_SUFFIXES:
                continue
            lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
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
            f"{path} is missing; run `wiz8 lint` then `wiz8 analyze source-index`"
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
        "c_linkage_symbols": validate_cross_tu_declarations(repository),
    }


def validate_cross_tu_declarations(repository: Path) -> int:
    """Require one canonical type per external symbol in the Clang index.

    C++ mangling already encodes the complete type, so two declarations that
    disagree about an overloaded C++ function cannot share a ``semantic_id``.
    Unmangled/C-linkage symbols carry no type in the symbol, and that is where
    a writer/reader disagreement survives separate compilation undetected.
    Group those by their undecorated source name and require one signature.
    """
    document = load_source_index(repository)
    signatures: dict[str, dict[tuple[Any, ...], list[str]]] = {}
    for item in document.get("declarations") or ():
        semantic_id = str(item.get("semantic_id", ""))
        if not semantic_id or semantic_id.startswith("?"):
            continue
        name = re.sub(r"@\d+$", "", semantic_id.lstrip("_@"))
        if not name:
            continue
        signature = (
            item.get("semantic_kind", ""),
            item.get("calling_convention", ""),
            item.get("return_type", ""),
            tuple(item.get("parameter_types") or ()),
        )
        location = f"{item.get('source_file', '?')}:{item.get('line', '?')}"
        signatures.setdefault(name, {}).setdefault(signature, []).append(location)
    conflicts = {name: variants for name, variants in signatures.items() if len(variants) > 1}
    if conflicts:
        rendered = []
        for name, variants in sorted(conflicts.items()):
            rendered.append(name)
            for signature, locations in variants.items():
                rendered.append(f"  {' | '.join(str(part) for part in signature)}")
                rendered.extend(f"    {location}" for location in locations[:4])
        raise SourceIndexError(
            "external symbols have divergent declarations:\n" + "\n".join(rendered)
        )
    return len(signatures)


def write_source_index(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    from .build import LINT_BUILD_DIR, VC6_IMAGE, configure_clang

    repository = settings.repo_dir.resolve()
    validate_synthetic_marker_blocks(repository)
    database = repository / LINT_BUILD_DIR / "compile_commands.json"
    inventories = tuple(
        repository / inventory
        for inventory in (
            "CMakeLists.txt",
            "cmake/clang-cl-i686.cmake",
            "cmake/CompileSettings.cmake",
            "cmake/Lint.cmake",
            "src/wiz8/sources.cmake",
            "src/sgp/CMakeLists.txt",
            "src/surrender/CMakeLists.txt",
            "src/srext_jpegimporter/CMakeLists.txt",
            "src/srext_unzip/CMakeLists.txt",
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
                if path.suffix.lower() in _SOURCE_SUFFIXES
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
            settings.work_dir / "fid/sources/unpacked/ijg-jpeg-6/jpeg-6": "/jpeg",
            settings.work_dir / "fid/sources/unpacked/infozip-unzip-5.4": "/infozip",
        },
        cache_dir=repository / "build/source-index-cache",
        cache_inputs=(
            *tuple(
                repository / path
                for path in (
                    "include",
                    "src",
                    "config",
                )
            ),
            settings.work_dir / "fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4",
            settings.work_dir / "fid/sources/unpacked/ijg-jpeg-6/jpeg-6",
            settings.work_dir / "fid/sources/unpacked/infozip-unzip-5.4",
        ),
        force=force,
    )
    index.write(repository / "build/source-index.json")
    c_linkage_symbols = validate_cross_tu_declarations(repository)
    return {
        "path": "build/source-index.json",
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
        "c_linkage_symbols": c_linkage_symbols,
    }
