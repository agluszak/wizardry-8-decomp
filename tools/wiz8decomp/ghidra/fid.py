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
from ..paths import atomic_json, atomic_write, sha256_file
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon


def _config(settings: Settings) -> dict[str, Any]:
    config = yaml.safe_load((settings.repo_dir / "config" / "ghidra.yml").read_text(encoding="utf-8"))
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


def _is_authoritative_fid_name(name: str) -> bool:
    """Reject compiler-local labels that are unstable between library builds."""
    return re.fullmatch(r"\$L\d+", name) is None


def fid_status(settings: Settings) -> dict[str, Any]:
    databases = []
    for kind in ("static", "srs"):
        path = _database_path(settings, kind)
        manifest = path.with_suffix(".json")
        databases.append({
            "kind": kind,
            "database": path.relative_to(settings.repo_dir).as_posix(),
            "exists": path.is_file(),
            "size": path.stat().st_size if path.is_file() else None,
            "sha256": sha256_file(path) if path.is_file() else None,
            "manifest": manifest.relative_to(settings.repo_dir).as_posix(),
            "manifest_exists": manifest.is_file(),
        })
    return {"schema": "wiz8.fid-status", "databases": databases}


def build_srs_fid(settings: Settings) -> dict[str, Any]:
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
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
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    programs = ArrayList()
    try:
        for name in config["seed_programs"]:
            domain_file = project.getProjectData().getFile("/" + name)
            if domain_file is None:
                raise RuntimeError(f"FID seed program is not imported: {name}")
            programs.add(domain_file)
        common_path = settings.ghidra_install_dir / "Ghidra" / "Features" / "FunctionID" / "data" / "common_symbols_win32.txt"
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
                "common_symbols_source": str(common_path.relative_to(settings.ghidra_install_dir)) if common_path.is_file() else None,
                "unresolved_symbol_count": len(result.getUnresolvedSymbols()),
            }
        finally:
            database.close()
    finally:
        project.close()
    atomic_json(path.with_suffix(".json"), summary)
    return summary


def _seed_program_name(toolchain: str, library: str, variant: str, object_path: Path) -> str:
    import re

    components = [toolchain, library, variant, object_path.stem]
    slug = "--".join(re.sub(r"[^a-z0-9]+", "-", item.casefold()).strip("-") for item in components)
    return f"fid--{slug}--{sha256_file(object_path)[:12]}"


def _cached_seed_manifest(settings: Settings) -> dict[str, Any] | None:
    from .fid_seeds import load_static_libraries

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
        if library.source is not None and library.source.sha256 is not None and library.compilation_units
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
    for item in manifest["libraries"]:
        root = settings.work_dir / "fid" / "objects" / item["toolchain"] / item["library"] / item["variant"]
        for record in item["objects"]:
            path = root / record["path"]
            if not path.is_file() or sha256_file(path) != record["sha256"]:
                return None
    return manifest


def import_static_seed_objects(settings: Settings, *, use_cached_objects: bool = False) -> dict[str, Any]:
    from .fid_seeds import build_seed_objects
    from .import_programs import HASH_OPTION, PATH_OPTION

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra

    object_manifest = _cached_seed_manifest(settings) if use_cached_objects else None
    if object_manifest is None:
        object_manifest = build_seed_objects(settings)
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
            project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
            try:
                domain_file = project.getProjectData().getFile("/" + name)
                if domain_file is not None:
                    with pyghidra.program_context(project, "/" + name) as program:
                        existing = program.getOptions("Program Information").getString(HASH_OPTION, None)
                    if existing != object_record["sha256"]:
                        raise RuntimeError(f"refusing to replace FID seed {name}: byte hash changed")
                    records.append({"program": name, "status": "already-imported", "toolchain": toolchain, "library": library, "variant": variant, "sha256": existing})
                    continue
            finally:
                project.close()
            with pyghidra.open_program(
                object_path,
                project_location=settings.project_dir,
                project_name=settings.project_name,
                analyze=True,
                language="x86:LE:32:default",
                compiler="windows",
                program_name=name,
                nested_project_location=False,
            ) as flat_api:
                program = flat_api.getCurrentProgram()
                transaction = program.startTransaction("record reproducible FID seed identity")
                try:
                    options = program.getOptions("Program Information")
                    options.setString(HASH_OPTION, object_record["sha256"])
                    options.setString(PATH_OPTION, f"fid/objects/{toolchain}/{library}/{variant}/{object_record['path']}")
                    options.setString("WIZ8_FID_LIBRARY", library)
                    options.setString("WIZ8_FID_VARIANT", variant)
                    options.setString("WIZ8_FID_TOOLCHAIN", toolchain)
                    options.setString("WIZ8_FID_TOOLCHAIN_COMMIT", toolchain_commit)
                    options.setString("WIZ8_FID_SOURCE_KIND", library_record.get("source_kind", "compiled-source"))
                finally:
                    program.endTransaction(transaction, True)
                records.append({
                    "program": name,
                    "status": "imported",
                    "toolchain": toolchain,
                    "library": library,
                    "variant": variant,
                    "sha256": object_record["sha256"],
                    "function_count": program.getFunctionManager().getFunctionCount(),
                })
    result = {"schema": "wiz8.fid-seed-import", "programs": records}
    atomic_json(settings.build_dir / "manifests" / "fid-seed-import.json", result)
    return result


def build_fid(settings: Settings) -> dict[str, Any]:
    """Build the primary FID database from named static-library object seeds."""
    from .fid_seeds import load_static_libraries

    imported = import_static_seed_objects(settings, use_cached_objects=True)
    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
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
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    common_path = settings.ghidra_install_dir / "Ghidra" / "Features" / "FunctionID" / "data" / "common_symbols_win32.txt"
    common_symbols = ArrayList()
    if common_path.is_file():
        for line in common_path.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.strip():
                common_symbols.add(line.strip())
    summaries = []
    database = fid_file.getFidDB(True)
    try:
        groups: dict[tuple[str, str, str], list[str]] = {}
        for record in imported["programs"]:
            groups.setdefault((record["toolchain"], record["library"], record["variant"]), []).append(record["program"])
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
            summaries.append({
                "family": library.family,
                "version": library.version,
                "variant": f"{toolchain_id}-{variant}",
                "seed_programs": len(names),
                "total_attempted": result.getTotalAttempted(),
                "total_added": result.getTotalAdded(),
                "total_excluded": result.getTotalExcluded(),
                "unresolved_symbol_count": len(result.getUnresolvedSymbols()),
            })
        database.saveDatabase("wizardry8 reproducible static-library FID build", TaskMonitor.DUMMY)
    except Exception:
        database.close()
        project.close()
        if path.exists():
            path.unlink()
        if backup_path.exists():
            os.replace(backup_path, path)
        raise
    else:
        database.close()
        project.close()
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
        "common_symbols_source": str(common_path.relative_to(settings.ghidra_install_dir)) if common_path.is_file() else None,
    }
    atomic_json(path.with_suffix(".json"), summary)
    return summary


def match_fid(settings: Settings, selector: str, threshold: float | None = None, database_kind: str = "static") -> dict[str, Any]:
    stop_daemon(settings, quiet=True)
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
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    matches: list[dict[str, Any]] = []
    excluded_internal_matches = 0
    service = FidService()
    score_threshold = threshold if threshold is not None else float(service.getDefaultScoreThreshold())
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            query_service = manager.openFidQueryService(program.getLanguage(), False)
            try:
                results = service.processProgram(program, query_service, score_threshold, TaskMonitor.DUMMY)
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
                        matches.append({
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
                            "seed_domain_path": str(function_record.getDomainPath()),
                            "seed_entry": f"0x{function_record.getEntryPoint():x}",
                        })
            finally:
                query_service.close()
    finally:
        project.close()
        for candidate, was_active in previous:
            candidate.setActive(was_active)
    matches.sort(key=lambda item: (item["target_address"], -item["score"], item["fid_name"]))
    output = settings.build_dir / "evidence" / "fid" / f"{program_name}.csv"
    stream = io.StringIO(newline="")
    fields = ["target_address", "target_name", "fid_name", "score", "primary_score", "child_score", "parent_score", "match_mode", "library_family", "library_version", "library_variant", "seed_domain_path", "seed_entry"]
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
        "ambiguous_target_count": sum(1 for address in {item["target_address"] for item in matches} if sum(item["target_address"] == address for item in matches) > 1),
        "excluded_internal_match_count": excluded_internal_matches,
        "evidence_csv": output.relative_to(settings.repo_dir).as_posix(),
        "mutated_program": False,
    }
    atomic_json(settings.build_dir / "reports" / "fid" / f"{program_name}.json", summary)
    return summary
