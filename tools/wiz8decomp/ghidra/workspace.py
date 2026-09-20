from __future__ import annotations

import json
import tomllib
from pathlib import Path
from typing import Any

from ..config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
)
from ..paths import atomic_json, sha256_file
from .import_programs import HASH_OPTION

SEED_SCHEMA = "wiz8.ghidra-seeds"


def pinned_reccmp_revision(repository: Path) -> str | None:
    """Return the git revision pinned in ``pyproject.toml`` for reccmp."""

    path = repository / "pyproject.toml"
    if not path.is_file():
        return None
    payload = tomllib.loads(path.read_text(encoding="utf-8"))
    source = ((payload.get("tool") or {}).get("uv") or {}).get("sources") or {}
    reccmp = source.get("reccmp") or {}
    revision = reccmp.get("rev")
    return str(revision) if revision else None


def compiler_import_identity(settings: Settings, target: str) -> dict[str, Any]:
    """PDB and reccmp identities that a ``--import-source`` run projected."""

    from ..build import _PRODUCT_OUTPUTS, TARGET_ALIASES

    resolved = TARGET_ALIASES.get(target, target)
    stem = _PRODUCT_OUTPUTS.get(resolved, "Wiz8")
    build_dir = getattr(settings, "product_build_dir", Path(settings.repo_dir) / "build/decomp")
    pdb = Path(build_dir) / f"{stem}.pdb"
    return {
        "pdb_sha256": sha256_file(pdb) if pdb.is_file() else None,
        "reccmp_revision": pinned_reccmp_revision(settings.repo_dir),
        "pdb_path": str(pdb),
    }


def seed_manifest_path(settings: Settings) -> Path:
    return settings.repo_dir / "vendor" / "ghidra" / "exports" / "manifest.json"


def seed_records(settings: Settings) -> list[dict[str, Any]]:
    path = seed_manifest_path(settings)
    if not path.is_file():
        raise RuntimeError(f"Ghidra seed manifest is missing: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SEED_SCHEMA:
        raise RuntimeError(f"unsupported Ghidra seed manifest schema: {manifest.get('schema')!r}")
    records = manifest.get("seeds")
    if not isinstance(records, list) or not records:
        raise RuntimeError(f"Ghidra seed manifest contains no programs: {path}")
    return records


def resolve_seed_program(settings: Settings, selector: str | None = None) -> str:
    records = seed_records(settings)
    names = [str(record["program"]) for record in records]
    if selector is None or selector.casefold() == "wiz8":
        canonical = [name for name in names if "--gog-base--wiz8--" in name]
        if len(canonical) == 1:
            return canonical[0]
    if selector is not None and selector.casefold() in {"sr", "sr.dll"}:
        canonical = [name for name in names if "--gog-base--sr--" in name]
        if len(canonical) == 1:
            return canonical[0]
    if selector is not None:
        matches = [name for name in names if name == selector or name.startswith(selector)]
        if len(matches) == 1:
            return matches[0]
    raise ValueError(
        f"Ghidra selector {selector!r} did not identify one tracked seed: {', '.join(names)}"
    )


def seed_record(
    settings: Settings, selector: str | None = None, *, validate_archive: bool = True
) -> dict[str, Any]:
    program_name = resolve_seed_program(settings, selector)
    record = next(
        record for record in seed_records(settings) if record.get("program") == program_name
    )
    expected_runtime = (
        REQUIRED_GHIDRA_VERSION,
        REQUIRED_GHIDRA_RELEASE,
        REQUIRED_PYGHIDRA_VERSION,
    )
    actual_runtime = (
        record.get("ghidra_version"),
        record.get("ghidra_release"),
        record.get("pyghidra_version"),
    )
    if actual_runtime != expected_runtime:
        raise RuntimeError(
            "GZF seed runtime differs from the pinned runtime: "
            f"seed={actual_runtime}, required={expected_runtime}"
        )
    archive = settings.repo_dir / str(record["path"])
    resolved = {**record, "archive": archive}
    if validate_archive:
        _validate_seed_archive(resolved)
    return resolved


def _validate_seed_archive(seed: dict[str, Any]) -> None:
    archive = Path(seed["archive"])
    if not archive.is_file():
        raise RuntimeError(f"validated GZF seed is missing: {archive}")
    actual_hash = sha256_file(archive)
    if actual_hash != seed.get("sha256"):
        raise RuntimeError(
            f"GZF seed hash mismatch for {archive}: {actual_hash} != {seed.get('sha256')}"
        )


def _program_hash(project: Any, program_name: str) -> str | None:
    import pyghidra

    domain_file = project.getProjectData().getFile("/" + program_name)
    if domain_file is None:
        return None
    with pyghidra.program_context(project, "/" + program_name) as program:
        return program.getOptions("Program Information").getString(HASH_OPTION, None)


OWNER_MARKER = "checkout-owner.json"
OWNER_SCHEMA = "wiz8.ghidra-owner"
REVIEWED_SEEDS_KEY = "reviewed_seeds"
SOURCE_PROJECTION_KEY = "source_projection"


def _project_owner_record(settings: Settings) -> dict[str, Any]:
    marker = settings.project_dir / OWNER_MARKER
    if not marker.is_file():
        return {}
    record = json.loads(marker.read_text(encoding="utf-8"))
    if not isinstance(record, dict):
        raise TypeError(f"invalid Ghidra project owner marker: {marker}")
    schema = record.get("schema")
    if schema not in {None, OWNER_SCHEMA}:
        raise RuntimeError(f"unsupported Ghidra project owner marker schema: {schema!r}")
    return record


def check_project_owner(settings: Settings) -> None:
    """Refuse a project directory that another checkout restored.

    Ghidra writes the project in place, so two checkouts sharing one project
    silently corrupt each other's analysis state. The in-repo default makes
    ownership structural; this guard catches an override pointed at another
    checkout's live project.
    """

    record = _project_owner_record(settings)
    recorded = record.get("repo_dir")
    if recorded and Path(recorded).resolve() != settings.repo_dir.resolve():
        raise RuntimeError(
            f"{settings.project_dir} was restored by a different checkout ({recorded}). "
            "Every checkout needs its own live Ghidra project; point "
            "WIZ8_GHIDRA_PROJECT_DIR elsewhere or unset it to use this checkout's "
            "ghidra-project/ directory."
        )


def _write_project_owner(settings: Settings) -> None:
    marker = settings.project_dir / OWNER_MARKER
    if not marker.is_file():
        atomic_json(marker, {"schema": OWNER_SCHEMA, "repo_dir": str(settings.repo_dir)})


def record_project_seed(settings: Settings, seed: dict[str, Any]) -> None:
    """Record which reviewed GZF initialized one live project program."""

    check_project_owner(settings)
    _write_project_owner(settings)
    marker = settings.project_dir / OWNER_MARKER
    record = _project_owner_record(settings)
    reviewed = dict(record.get(REVIEWED_SEEDS_KEY) or {})
    reviewed[str(seed["program"])] = str(seed["sha256"])
    record.update(
        {
            "schema": OWNER_SCHEMA,
            "repo_dir": str(settings.repo_dir),
            REVIEWED_SEEDS_KEY: reviewed,
        }
    )
    atomic_json(marker, record)


def recorded_source_projection(settings: Settings, program_name: str) -> dict[str, Any]:
    """Return the last recorded projection payload for ``program_name``, if any."""

    record = _project_owner_record(settings)
    projections = record.get(SOURCE_PROJECTION_KEY)
    recorded = projections.get(program_name) if isinstance(projections, dict) else None
    return dict(recorded) if isinstance(recorded, dict) else {}


def record_source_projection(
    settings: Settings, program_name: str, payload: dict[str, Any]
) -> None:
    """Record the last successful source/evidence projection into ProgramDB."""

    check_project_owner(settings)
    _write_project_owner(settings)
    marker = settings.project_dir / OWNER_MARKER
    record = _project_owner_record(settings)
    projections = dict(record.get(SOURCE_PROJECTION_KEY) or {})
    projections[program_name] = payload
    record.update(
        {
            "schema": OWNER_SCHEMA,
            "repo_dir": str(settings.repo_dir),
            SOURCE_PROJECTION_KEY: projections,
        }
    )
    atomic_json(marker, record)


def source_projection_freshness(settings: Settings, program_name: str) -> dict[str, Any]:
    """Compare recorded sync provenance with current source inputs. Does not open Ghidra."""

    from ..source_index import source_index_freshness

    index = source_index_freshness(settings.repo_dir)
    recorded = recorded_source_projection(settings, program_name)
    if not recorded:
        return {
            "ok": True,
            "status": "never",
            "compiler_status": "never",
            "detail": (
                "live ProgramDB has not been synchronized from current source; "
                "reviewed-seed origin does not imply source projection"
            ),
            "source_index": index,
        }
    index_path = settings.repo_dir / "build/source-index.json"
    recorded_digest = recorded.get("source_index_sha256")
    current_digest = None
    if index_path.is_file():
        from ..paths import sha256_file

        current_digest = sha256_file(index_path)
    applied_ns = recorded.get("applied_ns")
    source_newer = False
    if isinstance(applied_ns, int) and index["state"] != "missing":
        source_newer = index["state"] == "stale" or (
            index_path.is_file() and index_path.stat().st_mtime_ns > applied_ns
        )
    source_status = "current"
    source_detail = "ProgramDB reflects the current source-index identity"
    if current_digest is None or recorded_digest != current_digest or source_newer:
        source_status = "stale"
        source_detail = (
            "source or source-index changed after the last complete `wiz8 ghidra sync`; "
            "reviewed-seed origin is independent"
        )
    compiler = compiler_import_identity(settings, str(recorded.get("target") or "WIZ8"))
    recorded_pdb = recorded.get("pdb_sha256")
    recorded_rev = recorded.get("reccmp_revision")
    if not recorded_pdb and not recorded_rev:
        compiler_status = "never"
        compiler_detail = (
            "compiler/PDB types have not been projected; run "
            "`uv run wiz8 ghidra sync --import-source` after `uv run wiz8 build WIZ8`"
        )
    elif recorded_pdb != compiler.get("pdb_sha256") or recorded_rev != compiler.get(
        "reccmp_revision"
    ):
        compiler_status = "stale"
        compiler_detail = (
            "PDB or pinned reccmp revision changed after the last `--import-source` sync"
        )
    else:
        compiler_status = "current"
        compiler_detail = "ProgramDB compiler/PDB projection matches the current PDB and reccmp pin"
    return {
        "ok": True,
        "status": source_status,
        "compiler_status": compiler_status,
        "detail": source_detail,
        "compiler_detail": compiler_detail,
        "recorded_source_index_sha256": recorded_digest,
        "current_source_index_sha256": current_digest,
        "recorded_pdb_sha256": recorded_pdb,
        "current_pdb_sha256": compiler.get("pdb_sha256"),
        "recorded_reccmp_revision": recorded_rev,
        "current_reccmp_revision": compiler.get("reccmp_revision"),
        "source_index": index,
    }


def project_seed_freshness(settings: Settings, seed: dict[str, Any]) -> dict[str, Any]:
    """Compare a live project's recorded reviewed seed with the current manifest.

    This is deliberately metadata-only: doctor can reject stale or unprovable
    analysis without opening Ghidra or mutating the live project.
    """

    project_file = settings.project_dir / f"{settings.project_name}.gpr"
    if not project_file.is_file():
        return {
            "ok": True,
            "status": "not-restored",
            "expected_seed_sha256": str(seed["sha256"]),
            "recorded_seed_sha256": None,
            "detail": (
                "no live Ghidra project exists; the canonical opener will restore the current seed"
            ),
        }

    marker = settings.project_dir / OWNER_MARKER
    if not marker.is_file():
        return {
            "ok": False,
            "status": "untracked",
            "expected_seed_sha256": str(seed["sha256"]),
            "recorded_seed_sha256": None,
            "detail": "live Ghidra project has no checkout owner/freshness marker",
        }

    check_project_owner(settings)
    record = _project_owner_record(settings)
    reviewed = record.get(REVIEWED_SEEDS_KEY)
    if not isinstance(reviewed, dict):
        return {
            "ok": False,
            "status": "unknown",
            "expected_seed_sha256": str(seed["sha256"]),
            "recorded_seed_sha256": None,
            "detail": "live Ghidra project predates reviewed-seed freshness tracking",
        }

    recorded = reviewed.get(str(seed["program"]))
    if not recorded:
        return {
            "ok": False,
            "status": "unknown",
            "expected_seed_sha256": str(seed["sha256"]),
            "recorded_seed_sha256": None,
            "detail": f"live Ghidra project has no reviewed seed provenance for {seed['program']}",
        }
    if recorded != seed["sha256"]:
        return {
            "ok": False,
            "status": "stale",
            "expected_seed_sha256": str(seed["sha256"]),
            "recorded_seed_sha256": str(recorded),
            "detail": "tracked reviewed GZF changed after this live project was restored",
        }
    return {
        "ok": True,
        "status": "current",
        "expected_seed_sha256": str(seed["sha256"]),
        "recorded_seed_sha256": str(recorded),
        "detail": "live Ghidra project was initialized from the current reviewed GZF",
    }


def restore_seed(settings: Settings, project: Any, selector: str | None = None) -> dict[str, Any]:
    """Restore the reviewed checkpoint once into the canonical local project.

    The project is operational analysis state. Evidence edits do not clone,
    replay, invalidate, or replace it.
    """

    seed = seed_record(settings, selector, validate_archive=False)
    program_name = str(seed["program"])
    check_project_owner(settings)
    settings.project_dir.mkdir(parents=True, exist_ok=True)
    _write_project_owner(settings)
    existing_hash = _program_hash(project, program_name)
    if existing_hash is not None:
        if existing_hash != seed["binary_sha256"]:
            raise RuntimeError(
                f"existing {program_name} hash {existing_hash} differs from the seed's "
                f"{seed['binary_sha256']}; use a different Ghidra project directory "
                "(WIZ8_GHIDRA_PROJECT_DIR)"
            )
        freshness = project_seed_freshness(settings, seed)
        if freshness["status"] == "stale":
            raise RuntimeError(
                f"existing {program_name} uses reviewed GZF {freshness['recorded_seed_sha256']}, "
                f"but this checkout requires {freshness['expected_seed_sha256']}; "
                "run `uv run wiz8 doctor` and reconcile or explicitly refresh the checkout-owned "
                "Ghidra project before using retail analysis"
            )
        status = "already-restored"
    else:
        _validate_seed_archive(seed)
        from ghidra.util.task import TaskMonitor
        from java.io import File

        project.getProjectData().getRootFolder().createFile(
            program_name, File(str(seed["archive"])), TaskMonitor.DUMMY
        )
        restored_hash = _program_hash(project, program_name)
        if restored_hash != seed["binary_sha256"]:
            raise RuntimeError(f"restored program hash metadata mismatch for {program_name}")
        record_project_seed(settings, seed)
        status = "restored"
    freshness = project_seed_freshness(settings, seed)
    return {
        "schema": "wiz8.ghidra-workspace",
        "program": program_name,
        "status": status,
        "project_dir": str(settings.project_dir),
        "seed": str(seed["archive"]),
        "binary_sha256": seed["binary_sha256"],
        "seed_freshness": freshness["status"],
    }


def ensure_seed(settings: Settings, project: Any, selector: str | None = None) -> str:
    result = restore_seed(settings, project, selector)
    return str(result["program"])
