from __future__ import annotations

import csv
import io
import json
import os
import re
from pathlib import Path
from typing import Any

import yaml

from ..config import Settings
from ..paths import atomic_json, atomic_write, json_hash, sha256_file
from ..provenance import derive_authority, origin_for_fid_source_kind
from .env import open_project, project_lock, start_pyghidra
from .project import resolve_program_name


def _config(settings: Settings) -> dict[str, Any]:
    config = yaml.safe_load(
        (settings.repo_dir / "config" / "ghidra.yml").read_text(encoding="utf-8")
    )
    return config["fid"]


def _database_path(settings: Settings, kind: str = "static") -> Path:
    key = {"static": "static_database", "srs": "srs_database"}.get(kind)
    if key is None:
        raise ValueError(f"unknown FID database kind: {kind}")
    return settings.repo_dir / _config(settings)[key]


def _find_fid_file(manager: Any, path: Path) -> Any | None:
    for fid_file in manager.getUserAddedFiles():
        if Path(str(fid_file.getPath())).resolve() == path.resolve():
            return fid_file
    return None


def _fid_file_path(fid_file: Any) -> Path:
    return Path(str(fid_file.getPath())).resolve()


def _seed_provenance(settings: Settings, kind: str) -> dict[str, dict[str, Any]]:
    manifest_path = _database_path(settings, kind).with_suffix(".json")
    if not manifest_path.is_file():
        return {}
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    groups = manifest.get("seed_groups", {})
    return {
        program: {**groups.get(record.get("seed_group"), {}), **record}
        for program, record in manifest.get("seed_provenance", {}).items()
    }


def _provenance_for_domain(
    provenance: dict[str, dict[str, Any]], domain_path: str
) -> dict[str, Any] | None:
    return provenance.get(domain_path.rstrip("/").rsplit("/", 1)[-1])


def _verified_existing_fid_summary(path: Path, input_sha256: str) -> dict[str, Any] | None:
    summary_path = path.with_suffix(".json")
    if not path.is_file() or not summary_path.is_file():
        return None
    summary = json.loads(summary_path.read_text(encoding="utf-8"))
    if summary.get("input_sha256") != input_sha256:
        return None
    if summary.get("sha256") != sha256_file(path):
        return None
    return summary


def _normalize_seed_provenance(
    records: list[dict[str, Any]],
) -> tuple[dict[str, dict[str, Any]], dict[str, dict[str, Any]]]:
    groups: dict[str, dict[str, Any]] = {}
    programs: dict[str, dict[str, Any]] = {}
    for record in records:
        group_key = "/".join((record["toolchain"], record["library"], record["variant"]))
        group = {
            key: record[key]
            for key in (
                "toolchain",
                "toolchain_commit",
                "library",
                "variant",
                "source_kind",
                "archive",
                "source",
            )
            if key in record
        }
        previous = groups.setdefault(group_key, group)
        if previous != group:
            raise RuntimeError(f"conflicting FID seed provenance group: {group_key}")
        programs[record["program"]] = {
            "seed_group": group_key,
            **{
                key: record[key]
                for key in (
                    "object_path",
                    "object_sha256",
                    "archive_member",
                    "archive_member_index",
                )
                if key in record
            },
        }
    return dict(sorted(groups.items())), dict(sorted(programs.items()))


def _is_authoritative_fid_name(name: str) -> bool:
    """Reject Ghidra defaults and compiler-local labels as symbol authority."""
    return (
        re.fullmatch(
            r"(?:\$L\d+|(?:FUN|LAB|SUB|DAT|PTR|OFF|UNK|EXT)_[0-9A-Fa-f]+|thunk_.+|"
            r"switchD_[0-9A-Fa-f]+(?:::[0-9A-Fa-f]+)?)",
            name,
        )
        is None
    )


def fid_status(settings: Settings) -> dict[str, Any]:
    databases = []
    for kind in ("static", "srs"):
        path = _database_path(settings, kind)
        manifest = path.with_suffix(".json")
        databases.append(
            {
                "kind": kind,
                "database": path.relative_to(settings.repo_dir).as_posix(),
                "exists": path.is_file(),
                "size": path.stat().st_size if path.is_file() else None,
                "sha256": sha256_file(path) if path.is_file() else None,
                "manifest": manifest.relative_to(settings.repo_dir).as_posix(),
                "manifest_exists": manifest.is_file(),
            }
        )
    return {"schema": "wiz8.fid-status", "databases": databases}


def build_srs_fid(settings: Settings) -> dict[str, Any]:
    start_pyghidra(settings)
    from ghidra.feature.fid.db import FidFileManager
    from ghidra.feature.fid.service import FidService
    from ghidra.program.model.lang import LanguageID
    from ghidra.util.task import TaskMonitor
    from java.io import File
    from java.util import ArrayList

    config = _config(settings)
    path = _database_path(settings, "srs")
    if path.exists():
        raise RuntimeError(f"refusing to replace existing FID database: {path}")
    path.parent.mkdir(parents=True, exist_ok=True)
    manager = FidFileManager.getInstance()
    manager.createNewFidDatabase(File(str(path)))
    fid_file = manager.addUserFidFile(File(str(path)))
    programs = ArrayList()
    with open_project(settings) as project:
        for name in config["seed_programs"]:
            domain_file = project.getProjectData().getFile("/" + name)
            if domain_file is None:
                raise RuntimeError(f"FID seed program is not imported: {name}")
            programs.add(domain_file)
        common_path = (
            settings.ghidra_install_dir
            / "Ghidra"
            / "Features"
            / "FunctionID"
            / "data"
            / "common_symbols_win32.txt"
        )
        common_symbols = ArrayList()
        if common_path.is_file():
            for line in common_path.read_text(encoding="utf-8", errors="replace").splitlines():
                value = line.strip()
                if value:
                    common_symbols.add(value)
        database = fid_file.getFidDB(True)
        try:
            result = FidService().createNewLibraryFromPrograms(
                database,
                "Sir-Tech SRS SDK",
                "1.42.2.9",
                "msvc6-release",
                programs,
                None,
                LanguageID(config["language"]),
                None,
                common_symbols,
                TaskMonitor.DUMMY,
            )
            database.saveDatabase("wizardry8 reproducible FID build", TaskMonitor.DUMMY)
            summary = {
                "schema": "wiz8.fid-database",
                "database": path.relative_to(settings.repo_dir).as_posix(),
                "sha256": sha256_file(path),
                "size": path.stat().st_size,
                "library": {
                    "family": str(result.getLibraryRecord().getLibraryFamilyName()),
                    "version": str(result.getLibraryRecord().getLibraryVersion()),
                    "variant": str(result.getLibraryRecord().getLibraryVariant()),
                    "language": config["language"],
                    "compiler": "windows",
                },
                "seed_programs": config["seed_programs"],
                "total_attempted": result.getTotalAttempted(),
                "total_added": result.getTotalAdded(),
                "total_excluded": result.getTotalExcluded(),
                "common_symbols_source": str(common_path.relative_to(settings.ghidra_install_dir))
                if common_path.is_file()
                else None,
                "unresolved_symbol_count": len(result.getUnresolvedSymbols()),
            }
        finally:
            database.close()
    atomic_json(path.with_suffix(".json"), summary)
    return summary


def _seed_program_name(toolchain: str, library: str, variant: str, object_path: Path) -> str:
    import re

    components = [toolchain, library, variant, object_path.stem]
    slug = "--".join(re.sub(r"[^a-z0-9]+", "-", item.casefold()).strip("-") for item in components)
    return f"fid--{slug}--{sha256_file(object_path)[:12]}"


def _cached_seed_manifest(settings: Settings) -> dict[str, Any] | None:
    from .fid_seeds import load_static_libraries, validate_seed_manifest

    path = settings.build_dir / "manifests" / "fid-seed-objects.json"
    if not path.is_file():
        return None
    manifest = json.loads(path.read_text(encoding="utf-8"))
    config = load_static_libraries(settings)
    configured = {(item.id, item.commit) for item in config.toolchains}
    recorded = {(item["id"], item["commit"]) for item in manifest.get("toolchains", [])}
    if configured != recorded:
        return None
    expected = {
        (toolchain.id, library.id, variant.id, tuple(variant.flags))
        for toolchain in config.toolchains
        if "compiler" in toolchain.capabilities
        for library in config.libraries
        for variant in library.seed_variants
        if library.source is not None
        and library.source.sha256 is not None
        and library.compilation_units
    }
    expected.update(
        (toolchain.id, archive.library, archive.variant, ())
        for toolchain in config.toolchains
        for archive in toolchain.precompiled_archives
        if archive.seed
    )
    actual = {
        (item["toolchain"], item["library"], item["variant"], tuple(item["flags"]))
        for item in manifest.get("libraries", [])
    }
    if expected != actual:
        return None
    try:
        validate_seed_manifest(settings, manifest, require_complete=True)
    except RuntimeError:
        return None
    return manifest


def import_static_seed_objects(
    settings: Settings, *, use_cached_objects: bool = False
) -> dict[str, Any]:
    from .fid_seeds import build_all_seed_objects
    from .import_programs import HASH_OPTION, PATH_OPTION

    start_pyghidra(settings, max_heap="16G")
    import pyghidra
    from java.lang import System

    object_manifest = _cached_seed_manifest(settings) if use_cached_objects else None
    if object_manifest is None:
        object_manifest = build_all_seed_objects(settings)
    records = []
    for library_record in object_manifest["libraries"]:
        toolchain = library_record["toolchain"]
        toolchain_commit = library_record["toolchain_commit"]
        library = library_record["library"]
        variant = library_record["variant"]
        root = settings.work_dir / "fid" / "objects" / toolchain / library / variant
        for object_record in library_record["objects"]:
            object_path = root / object_record["path"]
            name = _seed_program_name(toolchain, library, variant, object_path)
            provenance = {
                "toolchain_commit": toolchain_commit,
                "source_kind": library_record.get("source_kind", "compiled-source"),
                "object_path": object_record["path"],
                "object_sha256": object_record["sha256"],
            }
            for key in ("archive_member", "archive_member_index"):
                if key in object_record:
                    provenance[key] = object_record[key]
            if "archive" in library_record:
                provenance["archive"] = library_record["archive"]
            if "source" in library_record:
                provenance["source"] = library_record["source"]
            with open_project(settings) as project:
                domain_file = project.getProjectData().getFile("/" + name)
                if domain_file is not None:
                    with pyghidra.program_context(project, "/" + name) as program:
                        existing = program.getOptions("Program Information").getString(
                            HASH_OPTION, None
                        )
                    if existing != object_record["sha256"]:
                        raise RuntimeError(
                            f"refusing to replace FID seed {name}: byte hash changed"
                        )
                    records.append(
                        {
                            "program": name,
                            "status": "already-imported",
                            "toolchain": toolchain,
                            "library": library,
                            "variant": variant,
                            "sha256": existing,
                            **provenance,
                        }
                    )
                    if len(records) % 100 == 0:
                        System.gc()
                    continue
            with (
                project_lock(settings),
                pyghidra.open_program(
                    object_path,
                    project_location=settings.project_dir,
                    project_name=settings.project_name,
                    analyze=True,
                    language="x86:LE:32:default",
                    compiler="windows",
                    program_name=name,
                    nested_project_location=False,
                ) as flat_api,
            ):
                program = flat_api.getCurrentProgram()
                transaction = program.startTransaction("record reproducible FID seed identity")
                try:
                    options = program.getOptions("Program Information")
                    options.setString(HASH_OPTION, object_record["sha256"])
                    options.setString(
                        PATH_OPTION,
                        f"fid/objects/{toolchain}/{library}/{variant}/{object_record['path']}",
                    )
                    options.setString("WIZ8_FID_LIBRARY", library)
                    options.setString("WIZ8_FID_VARIANT", variant)
                    options.setString("WIZ8_FID_TOOLCHAIN", toolchain)
                    options.setString("WIZ8_FID_TOOLCHAIN_COMMIT", toolchain_commit)
                    options.setString(
                        "WIZ8_FID_SOURCE_KIND", library_record.get("source_kind", "compiled-source")
                    )
                    options.setString("WIZ8_FID_OBJECT_SHA256", object_record["sha256"])
                    if "archive_member" in object_record:
                        options.setString(
                            "WIZ8_FID_ARCHIVE_MEMBER", object_record["archive_member"]
                        )
                finally:
                    program.endTransaction(transaction, True)
                records.append(
                    {
                        "program": name,
                        "status": "imported",
                        "toolchain": toolchain,
                        "library": library,
                        "variant": variant,
                        "sha256": object_record["sha256"],
                        "function_count": program.getFunctionManager().getFunctionCount(),
                        **provenance,
                    }
                )
                if len(records) % 100 == 0:
                    System.gc()
    result = {"schema": "wiz8.fid-seed-import", "programs": records}
    atomic_json(settings.build_dir / "manifests" / "fid-seed-import.json", result)
    return result


def build_fid(settings: Settings) -> dict[str, Any]:
    """Build the primary FID database from named static-library object seeds."""
    from .fid_seeds import load_static_libraries

    imported = import_static_seed_objects(settings, use_cached_objects=True)
    start_pyghidra(settings)
    from ghidra.feature.fid.db import FidFileManager
    from ghidra.feature.fid.service import FidService
    from ghidra.program.model.lang import LanguageID
    from ghidra.util.task import TaskMonitor
    from java.io import File
    from java.util import ArrayList

    config = load_static_libraries(settings)
    ghidra_config = _config(settings)
    path = _database_path(settings, "static")
    path.parent.mkdir(parents=True, exist_ok=True)
    seed_groups, seed_provenance = _normalize_seed_provenance(imported["programs"])
    common_path = (
        settings.ghidra_install_dir
        / "Ghidra"
        / "Features"
        / "FunctionID"
        / "data"
        / "common_symbols_win32.txt"
    )
    input_identity = {
        "schema": "wiz8.fid-build-input",
        "builder_version": 2,
        "language": ghidra_config["language"],
        "toolchains": [toolchain.model_dump(mode="json") for toolchain in config.toolchains],
        "libraries": [
            {
                "id": library.id,
                "family": library.family,
                "version": library.version,
            }
            for library in config.libraries
        ],
        "seed_groups": seed_groups,
        "seed_provenance": seed_provenance,
        "common_symbols_sha256": sha256_file(common_path) if common_path.is_file() else None,
    }
    input_sha256 = json_hash(input_identity)
    summary_path = path.with_suffix(".json")
    previous = _verified_existing_fid_summary(path, input_sha256)
    if previous is not None:
        return previous
    # Ghidra embeds more packed-database identity than the basename.  Build at
    # the authoritative path, retaining the previous complete DB as a rollback
    # until the new database has saved successfully.
    backup_path = settings.work_dir / "fid" / "databases" / "backup" / path.name
    backup_path.parent.mkdir(parents=True, exist_ok=True)
    if backup_path.exists():
        backup_path.unlink()
    if path.exists():
        os.replace(path, backup_path)
    manager = FidFileManager.getInstance()
    manager.createNewFidDatabase(File(str(path)))
    fid_file = manager.addUserFidFile(File(str(path)))
    common_symbols = ArrayList()
    if common_path.is_file():
        for line in common_path.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.strip():
                common_symbols.add(line.strip())
    summaries = []
    database = fid_file.getFidDB(True)
    try:
        with open_project(settings) as project:
            groups: dict[tuple[str, str, str], list[str]] = {}
            for record in imported["programs"]:
                groups.setdefault(
                    (record["toolchain"], record["library"], record["variant"]), []
                ).append(record["program"])
            libraries_by_id = {library.id: library for library in config.libraries}
            for (toolchain_id, library_id, variant), names in sorted(groups.items()):
                programs = ArrayList()
                for name in sorted(names):
                    domain_file = project.getProjectData().getFile("/" + name)
                    if domain_file is None:
                        raise RuntimeError(f"FID seed program is not imported: {name}")
                    programs.add(domain_file)
                library = libraries_by_id[library_id]
                result = FidService().createNewLibraryFromPrograms(
                    database,
                    library.family,
                    library.version,
                    f"{toolchain_id}-{variant}",
                    programs,
                    None,
                    LanguageID(ghidra_config["language"]),
                    None,
                    common_symbols,
                    TaskMonitor.DUMMY,
                )
                summaries.append(
                    {
                        "family": library.family,
                        "version": library.version,
                        "variant": f"{toolchain_id}-{variant}",
                        "seed_programs": len(names),
                        "total_attempted": result.getTotalAttempted(),
                        "total_added": result.getTotalAdded(),
                        "total_excluded": result.getTotalExcluded(),
                        "unresolved_symbol_count": len(result.getUnresolvedSymbols()),
                    }
                )
            database.saveDatabase(
                "wizardry8 reproducible static-library FID build", TaskMonitor.DUMMY
            )
    except Exception:
        database.close()
        if path.exists():
            path.unlink()
        if backup_path.exists():
            os.replace(backup_path, path)
        raise
    else:
        database.close()
        if backup_path.exists():
            backup_path.unlink()
    summary = {
        "schema": "wiz8.fid-database",
        "kind": "static-libraries",
        "database": path.relative_to(settings.repo_dir).as_posix(),
        "sha256": sha256_file(path),
        "size": path.stat().st_size,
        "toolchains": [toolchain.model_dump(mode="json") for toolchain in config.toolchains],
        "toolchain_evidence": config.toolchain_evidence.model_dump(mode="json"),
        "libraries": summaries,
        "seed_groups": seed_groups,
        "seed_provenance": seed_provenance,
        "input_sha256": input_sha256,
        "common_symbols_source": str(common_path.relative_to(settings.ghidra_install_dir))
        if common_path.is_file()
        else None,
    }
    atomic_json(summary_path, summary)
    return summary


def match_fid(
    settings: Settings, selector: str, threshold: float | None = None, database_kind: str = "static"
) -> dict[str, Any]:
    start_pyghidra(settings)
    import pyghidra
    from ghidra.feature.fid.db import FidFileManager
    from ghidra.feature.fid.service import FidService
    from ghidra.util.task import TaskMonitor
    from java.io import File

    path = _database_path(settings, database_kind)
    if not path.is_file():
        raise RuntimeError("project-owned FID database does not exist; run 'wiz8 ghidra fid build'")
    manager = FidFileManager.getInstance()
    fid_file = _find_fid_file(manager, path) or manager.addUserFidFile(File(str(path)))
    previous = []
    active_databases = []
    target_path = path.resolve()
    for candidate in manager.getFidFiles():
        previous.append((candidate, bool(candidate.isActive())))
        candidate_path = _fid_file_path(candidate)
        selected = candidate_path == target_path
        candidate.setActive(selected)
        if selected:
            active_databases.append(candidate_path.as_posix())
    # getFidFiles() and getUserAddedFiles() may expose different Java wrapper
    # instances for the same configured database.  Path identity is stable.
    fid_file.setActive(True)
    if target_path.as_posix() not in active_databases:
        active_databases.append(target_path.as_posix())
    if active_databases != [target_path.as_posix()]:
        raise RuntimeError(f"failed to select exactly one FID database: {active_databases}")
    program_name = resolve_program_name(settings, selector)
    matches: list[dict[str, Any]] = []
    excluded_internal_matches = 0
    provenance_by_program = _seed_provenance(settings, database_kind)
    service = FidService()
    score_threshold = (
        threshold if threshold is not None else float(service.getDefaultScoreThreshold())
    )
    with (
        open_project(settings) as project,
        pyghidra.program_context(project, "/" + program_name) as program,
    ):
        query_service = manager.openFidQueryService(program.getLanguage(), False)
        try:
            results = service.processProgram(
                program, query_service, score_threshold, TaskMonitor.DUMMY
            )
            # FID match records resolve names through their owning database.
            # Materialize every value before closing the query service.
            for result in results:
                for match in result.matches:
                    function_record = match.getFunctionRecord()
                    library = match.getLibraryRecord()
                    fid_name = str(function_record.getName())
                    if not _is_authoritative_fid_name(fid_name):
                        excluded_internal_matches += 1
                        continue
                    seed_domain_path = str(function_record.getDomainPath())
                    provenance = _provenance_for_domain(provenance_by_program, seed_domain_path)
                    if database_kind == "static" and provenance is None:
                        raise RuntimeError(
                            f"FID match lacks seed provenance: {seed_domain_path} {fid_name}"
                        )
                    # A FID name is only as authoritative as the seed that carried
                    # it; see docs/wiz8-evidence-model.md.
                    name_origin = origin_for_fid_source_kind(
                        provenance.get("source_kind") if provenance else None
                    )
                    matches.append(
                        {
                            "target_address": str(result.function.getEntryPoint()),
                            "target_name": str(result.function.getName()),
                            "fid_name": fid_name,
                            "score": float(match.getOverallScore()),
                            "primary_score": float(match.getPrimaryFunctionCodeUnitScore()),
                            "child_score": float(match.getChildFunctionCodeUnitScore()),
                            "parent_score": float(match.getParentFunctionCodeUnitScore()),
                            "match_mode": str(match.getPrimaryFunctionMatchMode()),
                            "library_family": str(library.getLibraryFamilyName()),
                            "library_version": str(library.getLibraryVersion()),
                            "library_variant": str(library.getLibraryVariant()),
                            "seed_domain_path": seed_domain_path,
                            "seed_entry": f"0x{function_record.getEntryPoint():x}",
                            "seed_toolchain": provenance.get("toolchain") if provenance else None,
                            "seed_toolchain_commit": provenance.get("toolchain_commit")
                            if provenance
                            else None,
                            "seed_source_kind": provenance.get("source_kind")
                            if provenance
                            else None,
                            "seed_object_path": provenance.get("object_path")
                            if provenance
                            else None,
                            "seed_object_sha256": provenance.get("object_sha256")
                            if provenance
                            else None,
                            "seed_archive_member": provenance.get("archive_member")
                            if provenance
                            else None,
                            "seed_archive_member_index": provenance.get("archive_member_index")
                            if provenance
                            else None,
                            "seed_source_archive_sha256": provenance.get("source", {}).get(
                                "archive_sha256"
                            )
                            if provenance
                            else None,
                            "seed_source_tree_hash": provenance.get("source", {}).get(
                                "source_tree_hash"
                            )
                            if provenance
                            else None,
                            "name_origin": name_origin,
                            "authority": derive_authority((name_origin,)),
                        }
                    )
        finally:
            query_service.close()
    for candidate, was_active in previous:
        candidate.setActive(was_active)
    matches.sort(key=lambda item: (item["target_address"], -item["score"], item["fid_name"]))
    output = settings.build_dir / "evidence" / "fid" / f"{program_name}.csv"
    stream = io.StringIO(newline="")
    fields = [
        "target_address",
        "target_name",
        "fid_name",
        "score",
        "primary_score",
        "child_score",
        "parent_score",
        "match_mode",
        "library_family",
        "library_version",
        "library_variant",
        "seed_domain_path",
        "seed_entry",
        "seed_toolchain",
        "seed_toolchain_commit",
        "seed_source_kind",
        "seed_object_path",
        "seed_object_sha256",
        "seed_archive_member",
        "seed_archive_member_index",
        "seed_source_archive_sha256",
        "seed_source_tree_hash",
        "name_origin",
        "authority",
    ]
    writer = csv.DictWriter(stream, fieldnames=fields, lineterminator="\n")
    writer.writeheader()
    writer.writerows(matches)
    atomic_write(output, stream.getvalue())
    unique_targets = len({item["target_address"] for item in matches})
    summary = {
        "schema": "wiz8.fid-match",
        "program": program_name,
        "database_sha256": sha256_file(path),
        "database_kind": database_kind,
        "active_databases": active_databases,
        "threshold": score_threshold,
        "match_count": len(matches),
        "unique_target_count": unique_targets,
        "ambiguous_target_count": sum(
            1
            for address in {item["target_address"] for item in matches}
            if sum(item["target_address"] == address for item in matches) > 1
        ),
        "excluded_internal_match_count": excluded_internal_matches,
        "evidence_csv": output.relative_to(settings.repo_dir).as_posix(),
        "mutated_program": False,
    }
    atomic_json(settings.build_dir / "reports" / "fid" / f"{program_name}.json", summary)
    return summary
