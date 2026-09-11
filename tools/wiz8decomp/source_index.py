"""Project paths and toolchain configuration for reccmp's source index."""

from __future__ import annotations

import json
import re
import shlex
import shutil
from pathlib import Path
from typing import Any

from reccmp.source import SourceIndex, SourceIndexError, SourceMarker

from .config import Settings

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
    if document.get("schema") != "reccmp-source-index-v2":
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


def _cmake_configure_inputs(repository: Path) -> tuple[Path, ...]:
    """Fingerprint every CMake/toolchain input that shapes the lint compile DB.

    The previous short inventory missed CompileSettings.cmake, the clang-cl
    toolchain, and the extension source-model fragments, so `analyze
    source-index` could consume a stale database after a compile-setting edit.
    Collect the actual inputs instead of maintaining another partial list.
    """
    candidates: list[Path] = [repository / "CMakeLists.txt"]
    candidates.extend(sorted((repository / "cmake").glob("*.cmake")))
    candidates.extend(sorted(repository.glob("src/*/CMakeLists.txt")))
    candidates.append(repository / "src/wiz8/sources.cmake")
    seen: list[Path] = []
    for path in candidates:
        if path.is_file() and path not in seen:
            seen.append(path)
    return tuple(seen)


def _compile_db_files(database: Path) -> set[str]:
    try:
        entries = json.loads(database.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return set()
    files: set[str] = set()
    for entry in entries if isinstance(entries, list) else []:
        raw = str((entry or {}).get("file", ""))
        if raw.startswith("/repo/"):
            files.add(raw[len("/repo/") :])
        elif raw.startswith("/"):
            continue
        elif raw:
            files.add(raw)
    return files


def indexed_targets(repository: Path, database: Path | None = None) -> dict[str, tuple[str, ...]]:
    """Derive index targets from project source roots plus compile-DB coverage.

    Every reccmp target with a `source-root` is a candidate; when a compile
    database is available, keep only candidates with at least one entry whose
    file falls under one of their roots. This adds the first-party extension
    targets automatically once the lint projection emits their commands,
    without another hard-coded tuple.
    """
    targets = project_targets(repository)
    candidates = {
        target: _source_roots(config) for target, config in targets.items() if _source_roots(config)
    }
    if database is not None and database.is_file():
        covered = _compile_db_files(database)
        if covered:
            filtered = {}
            for target, roots in candidates.items():
                if any(
                    candidate == root or candidate.startswith(root.rstrip("/") + "/")
                    for candidate in covered
                    for root in roots
                ):
                    filtered[target] = roots
            if filtered:
                return filtered
    return candidates


def validate_source_index(repository: Path) -> dict[str, int]:
    validate_synthetic_marker_blocks(repository)
    index = SourceIndex.from_dict(load_source_index(repository))
    counts = {
        target: len(index.functions_by_address(target=target))
        for target in project_targets(repository)
    }
    if len({item.semantic_id for item in index.classes}) != len(index.classes):
        raise SourceIndexError("compiler-backed source index contains duplicate class definitions")
    result: dict[str, int] = {
        "functions": sum(counts.values()),
        "wiz8_functions": counts.get("WIZ8", 0),
        "surrender_functions": counts.get("SURRENDER", 0),
        "classes": len(index.classes),
        "vtable_classes": sum(item.vtable_address is not None for item in index.classes),
        "variables": len(index.variables),
        "conflicts": len(index.conflicts),
        # TODO(B): re-enable validate_cross_tu_declarations here once the
        # extern-array completion idiom (T[] vs T[N]) has an agreed rule. The
        # check itself stays tested below; it is parked, not removed.
    }
    for target in ("SREXT_JPEGIMPORTER", "SREXT_UNZIP"):
        if target in counts:
            result[f"{target.lower()}_functions"] = counts[target]
    return result


def validate_cross_tu_declarations(repository: Path) -> int:
    """Require one canonical type per external symbol in the Clang index.

    PARKED for B: `wiz8 analyze source-index` and `validate_source_index`
    do not call this yet. The remaining hits are the legal extern-array
    completion idiom (`extern T g[]` completed by `T g[N]`), which needs an
    array-aware compatibility rule before this can gate. Unit tests below
    keep the rest of the behavior pinned in the meantime.
    """
    document = load_source_index(repository)
    rendered: list[str] = []

    recorded: dict[tuple[str, str], dict[tuple[str, ...], list[str]]] = {}
    for conflict in document.get("conflicts") or ():
        key = (str(conflict.get("record_kind", "")), str(conflict.get("semantic_id", "")))
        variants = recorded.setdefault(key, {})
        for variant in conflict.get("variants") or ():
            signature = tuple(variant.get("signature") or ())
            locations = variants.setdefault(signature, [])
            for location in variant.get("locations") or ():
                if location not in locations:
                    locations.append(location)
    for (kind, semantic_id), variants in sorted(recorded.items()):
        if len(variants) < 2:
            continue
        # TU-local spellings never collide at link time: two `static`
        # definitions with different types are independent entities. Only a
        # disagreement involving an externally linked spelling can escape
        # separate compilation undetected.
        if not any(signature and signature[-1] == "external" for signature in variants):
            continue
        rendered.append(f"{semantic_id} ({kind})")
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    functions: dict[str, dict[tuple[Any, ...], list[str]]] = {}
    for item in document.get("declarations") or ():
        if item.get("linkage", "") != "external":
            continue
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
        functions.setdefault(name, {}).setdefault(signature, []).append(location)
    for name, variants in sorted(functions.items()):
        if len(variants) < 2:
            continue
        rendered.append(name)
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    variables: dict[str, dict[tuple[Any, ...], list[str]]] = {}
    for item in document.get("variables") or ():
        if item.get("linkage", "") != "external":
            continue
        semantic_id = str(item.get("semantic_id", ""))
        if not semantic_id:
            continue
        signature = (item.get("type", ""),)
        location = f"{item.get('source_file', '?')}:{item.get('line', '?')}"
        variables.setdefault(semantic_id, {}).setdefault(signature, []).append(location)
    for semantic_id, variants in sorted(variables.items()):
        if len(variants) < 2:
            continue
        rendered.append(semantic_id)
        for signature, locations in variants.items():
            rendered.append(f"  {' | '.join(str(part) for part in signature)}")
            rendered.extend(f"    {location}" for location in locations[:4])

    if rendered:
        raise SourceIndexError(
            "external symbols have divergent declarations:\n" + "\n".join(rendered)
        )
    return len(functions) + len(variables)


_INCLUDE_FLAGS_WITH_ARGUMENT = frozenset({"-I", "-isystem", "-iquote", "-idirafter"})


def _compile_command_include_dirs(entry: dict[str, Any]) -> list[str]:
    """The include search directories one compile command adds.

    The lint projection emits POSIX ``-I`` flags, so only the two spellings it
    can produce are recognized; everything else stays out of the dependency
    fingerprint.
    """
    arguments = entry.get("arguments")
    if isinstance(arguments, list):
        tokens = [str(token) for token in arguments]
    else:
        tokens = shlex.split(str(entry.get("command", "")))
    found: list[str] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token in _INCLUDE_FLAGS_WITH_ARGUMENT:
            if index + 1 < len(tokens):
                found.append(tokens[index + 1])
            index += 2
            continue
        for flag in ("-isystem", "-iquote", "-idirafter", "-I"):
            if token.startswith(flag) and len(token) > len(flag):
                found.append(token[len(flag) :])
                break
        index += 1
    return found


def _guest_to_host(guest: str, mounts: dict[Path, str]) -> Path | None:
    """Translate one compile-database path back to a host directory."""
    if not guest.startswith("/"):
        return None
    matches = sorted(
        ((mount_guest, host) for host, mount_guest in mounts.items()),
        key=lambda item: len(item[0]),
        reverse=True,
    )
    for mount_guest, host in matches:
        trimmed = mount_guest.rstrip("/")
        if guest == trimmed:
            return host
        if guest.startswith(trimmed + "/"):
            return host / guest[len(trimmed) + 1 :]
    return None


def _cache_inputs_by_target(
    repository: Path,
    targets: dict[str, tuple[Path, ...]],
    entries_by_target: dict[str, list[dict[str, Any]]],
    mounts: dict[Path, str],
) -> dict[str, tuple[Path, ...]]:
    """Derive each namespace's dependency fingerprint from its own compile
    commands.

    reccmp fingerprints a namespace from its marker targets plus ``cache_inputs``
    and reads every file below each entry. Handing every namespace the whole
    repository therefore re-hashes every tree four times and makes a WIZ8 source
    edit invalidate the extension namespaces. The include directories of the
    namespace's own compile commands are the actual header dependencies, so each
    namespace fingerprints only the headers it can include.
    """
    result: dict[str, tuple[Path, ...]] = {}
    for target, entries in entries_by_target.items():
        unique: list[Path] = []
        seen: set[Path] = set()
        for entry in entries:
            for guest in _compile_command_include_dirs(entry):
                host = _guest_to_host(guest, mounts)
                if host is None or host in seen or not host.is_dir():
                    continue
                seen.add(host)
                unique.append(host)
        result[target] = tuple(unique)
    for target in targets:
        result.setdefault(target, ())
    return result


def _seed_collector_binary(source: Path, destination: Path) -> None:
    """Reuse one compiled collector across the per-namespace projections.

    reccmp keeps the compiled ``indexer`` beside the projection in ``cache_dir``,
    so each namespace compiles the identical Clang collector on a cold cache.
    Copy the built executable and its digest into the next namespace's cache
    before its first lookup. The long-term fix belongs in reccmp (one batch with
    a namespace-aware identity); this keeps the Wizardry side from paying for it.
    """
    if source == destination:
        return
    destination.mkdir(parents=True, exist_ok=True)
    for name in ("indexer", "indexer.sha256"):
        origin = source / name
        target = destination / name
        if origin.is_file() and not target.is_file():
            shutil.copyfile(origin, target)


def _collect_per_namespace(
    repository: Path,
    database: Path,
    targets: dict[str, tuple[Path, ...]],
    roots: dict[str, tuple[str, ...]],
    *,
    clang: str | None,
    container_image: str | None,
    mounts: dict[Path, str],
    force: bool,
) -> SourceIndex:
    """Collect one source index per link namespace, then merge the markers.

    Each reccmp target is its own binary, so the same unmangled symbol may be
    legitimately defined in several of them (both extension DLLs define
    ``DllMain`` with the same ``_DllMain@12`` identity). The upstream
    collector merges records by ``semantic_id`` across every translation unit
    it sees, so one shared collection keeps only one of those definitions and
    the other target's marker binds to nothing. Partition the compile database
    by source root and collect each namespace with its own cache.

    Entries outside every source root are external/vendor translation units
    (``/zlib``, ``/infozip``); their headers are already parsed through the
    first-party units that include them, so running them standalone only
    multiplies work. An unowned ``/repo`` entry is a configuration error.
    """
    entries = json.loads(database.read_text(encoding="utf-8"))

    def owner(entry: dict[str, Any]) -> str | None:
        raw = str(entry.get("file", ""))
        candidate = raw.removeprefix("/repo/")
        for target, source_roots in roots.items():
            if any(
                candidate == root or candidate.startswith(root.rstrip("/") + "/")
                for root in source_roots
            ):
                return target
        return None

    by_target: dict[str, list[dict[str, Any]]] = {target: [] for target in targets}
    for entry in entries:
        target = owner(entry)
        if target is not None:
            by_target[target].append(entry)
            continue
        raw = str(entry.get("file", ""))
        if raw.startswith("/repo/"):
            raise SourceIndexError(
                f"compile database entry is outside every configured source-root: {raw}"
            )
    cache_inputs = _cache_inputs_by_target(repository, targets, by_target, mounts)
    indexes = []
    binary_source: Path | None = None
    for target, paths in targets.items():
        partitioned_db = (
            repository / "build" / "source-index-cache" / f"compile-commands-{target.lower()}.json"
        )
        partitioned_db.parent.mkdir(parents=True, exist_ok=True)
        content = json.dumps(by_target[target], indent=2) + "\n"
        if not partitioned_db.is_file() or partitioned_db.read_text(encoding="utf-8") != content:
            partitioned_db.write_text(content, encoding="utf-8")
        cache_dir = repository / "build" / "source-index-cache" / target.lower()
        if binary_source is not None:
            _seed_collector_binary(binary_source, cache_dir)
        indexes.append(
            SourceIndex.from_compile_database(
                repository,
                partitioned_db,
                {target: paths},
                clang=clang,
                container_image=container_image,
                compilation_root=Path("/repo"),
                mounts=mounts,
                cache_dir=cache_dir,
                cache_inputs=(*cache_inputs[target], database),
                force=force,
            )
        )
        if binary_source is None:
            binary_source = cache_dir
    # Each namespace keeps its own winner: the same unmangled spelling may be
    # legitimately defined in several binaries, and the consistency gate groups
    # every spelling by identity, so cross-namespace disagreements stay visible.
    return SourceIndex(
        declarations=(item for index in indexes for item in index.declarations),
        classes=(item for index in indexes for item in index.classes),
        markers=(item for index in indexes for item in index.markers),
        variables=(item for index in indexes for item in index.variables),
        conflicts=(item for index in indexes for item in index.conflicts),
    )


def write_source_index(settings: Settings, *, force: bool = False) -> dict[str, Any]:
    from .build import LINT_BUILD_DIR, VC6_IMAGE, configure_clang

    repository = settings.repo_dir.resolve()
    validate_synthetic_marker_blocks(repository)
    database = repository / LINT_BUILD_DIR / "compile_commands.json"
    inventories = _cmake_configure_inputs(repository)
    if not database.is_file() or any(
        path.is_file() and path.stat().st_mtime > database.stat().st_mtime for path in inventories
    ):
        configure_clang(settings)
    if not database.is_file():
        raise FileNotFoundError(f"clang configuration did not produce {database}")
    roots = indexed_targets(repository, database)
    targets = {
        target: tuple(
            sorted(
                path
                for source_root in source_roots
                for path in (repository / source_root).rglob("*")
                if path.suffix.lower() in _SOURCE_SUFFIXES
            )
        )
        for target, source_roots in roots.items()
    }
    mounts = {
        repository: "/repo",
        repository / LINT_BUILD_DIR: "/out",
        settings.work_dir / "fid/sources/unpacked/zlib-1.0.4/zlib-1.0.4": "/zlib",
        settings.work_dir / "fid/sources/unpacked/ijg-jpeg-6/jpeg-6": "/jpeg",
        settings.work_dir / "fid/sources/unpacked/infozip-unzip-5.4": "/infozip",
    }
    index = _collect_per_namespace(
        repository,
        database,
        targets,
        roots,
        clang="/usr/bin/clang-cl",
        container_image=VC6_IMAGE,
        mounts=mounts,
        force=force,
    )
    index.write(repository / "build/source-index.json")
    # TODO(B): gate on validate_cross_tu_declarations here once the
    # extern-array completion rule lands. Parked, not removed.
    return {
        "path": "build/source-index.json",
        "markers": len(index.markers),
        "declarations": len(index.declarations),
        "classes": len(index.classes),
        "variables": len(index.variables),
        "conflicts": len(index.conflicts),
    }
