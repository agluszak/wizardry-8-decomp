from __future__ import annotations

import hashlib
import json
import os
import shutil
from contextlib import contextmanager
from pathlib import Path
from time import perf_counter
from typing import Any

from ..config import (
    REQUIRED_GHIDRA_RELEASE,
    REQUIRED_GHIDRA_VERSION,
    REQUIRED_PYGHIDRA_VERSION,
    Settings,
    ghidra_agent_id,
)
from ..paths import atomic_json, sha256_file
from .environment import start_pyghidra
from .import_programs import HASH_OPTION
from .project import module_for_program, resolve_program_name
from .rebuild import observation_replay_actions, reviewed_replay_actions
from .validate_replay import validate_reviewed_replay

SEED_SCHEMA = "wiz8.ghidra-seeds"
MATERIALIZATION_SCHEMA = "wiz8.ghidra-materialization"


def seed_manifest_path(settings: Settings) -> Path:
    return settings.repo_dir / "vendor" / "ghidra" / "exports" / "manifest.json"


# Modules under ghidra/ that cannot change what the replay writes into a
# program: transports, caches, importers and exporters. Hashing them into the
# materialization key means editing the cache or the daemon invalidates every
# agent's project and forces a full re-import, which is pure cost - during one
# session the tooling edits alone rebuilt several 51MB projects.
#
# This is a deny-list rather than an allow-list on purpose. A module nobody has
# classified still feeds the key, so the mistake is a needless rebuild rather
# than a stale program, and only a file positively known to be inert is dropped.
REPLAY_INERT_MODULES = frozenset(
    {
        "__init__.py",
        "aggregate_overlay.py",
        "analyze.py",
        "cache.py",
        "candidate_facts.py",
        "dependency_graph.py",
        "environment.py",
        "export_evidence.py",
        "export_programs.py",
        "fid.py",
        "fid_seeds.py",
        "import_programs.py",
        "inference.py",
        "overlay.py",
        "query.py",
        "query_daemon.py",
        "reconstructed_transfer.py",
        "semantic.py",
    }
)


def _replay_input_paths(settings: Settings) -> list[Path]:
    roots = [
        settings.repo_dir / "tools" / "wiz8decomp" / "ghidra",
        settings.repo_dir / "evidence" / "reviewed" / "wiz8",
        settings.repo_dir / "evidence" / "reviewed" / "sgp",
        settings.repo_dir / "evidence" / "snapshots" / "call-sites",
        settings.repo_dir / "evidence" / "snapshots" / "eh-metadata",
        settings.repo_dir / "evidence" / "snapshots" / "globals",
        settings.repo_dir / "evidence" / "snapshots" / "polymorphism",
    ]
    paths = [
        settings.repo_dir / "config" / "ghidra.yml",
        settings.repo_dir / "config" / "modules.yml",
        settings.repo_dir / "config" / "sgp.yml",
    ]
    for root in roots:
        if root.is_dir():
            paths.extend(
                path
                for path in root.rglob("*")
                if path.is_file()
                and path.suffix.casefold() in {".csv", ".json", ".py", ".yaml", ".yml"}
                and path.name not in REPLAY_INERT_MODULES
            )
    return sorted(
        {path for path in paths if path.is_file()},
        key=lambda path: path.relative_to(settings.repo_dir).as_posix(),
    )


def replay_input_sha256(settings: Settings) -> str:
    """Hash every checked-in input that can change the reviewed replay result."""

    digest = hashlib.sha256()
    for path in _replay_input_paths(settings):
        relative = path.relative_to(settings.repo_dir).as_posix()
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(sha256_file(path).encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def _seed_record(settings: Settings, program_name: str) -> dict[str, Any]:
    path = seed_manifest_path(settings)
    if not path.is_file():
        raise RuntimeError(f"Ghidra seed manifest is missing: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != SEED_SCHEMA:
        raise RuntimeError(f"unsupported Ghidra seed manifest schema: {manifest.get('schema')!r}")
    matches = [
        record for record in manifest.get("seeds", []) if record.get("program") == program_name
    ]
    if len(matches) != 1:
        raise RuntimeError(
            f"expected one validated GZF seed for {program_name}; found {len(matches)}"
        )
    record = matches[0]
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
    archive = settings.repo_dir / record["path"]
    if not archive.is_file():
        raise RuntimeError(
            f"validated GZF seed is missing: {archive}; restore the tracked canonical seed"
        )
    actual_archive_hash = sha256_file(archive)
    if actual_archive_hash != record.get("sha256"):
        raise RuntimeError(
            f"GZF seed hash mismatch for {archive}: {actual_archive_hash} != {record.get('sha256')}"
        )
    module = module_for_program(settings, program_name)
    if record.get("binary_sha256") != module["sha256"]:
        raise RuntimeError(f"GZF seed binary hash differs from configured {program_name}")
    return {**record, "archive": archive}


def _materialization_identity(
    settings: Settings, program_name: str, seed: dict[str, Any]
) -> dict[str, str]:
    replay_hash = replay_input_sha256(settings)
    payload = "\0".join(
        [
            program_name,
            seed["sha256"],
            replay_hash,
            REQUIRED_GHIDRA_VERSION,
            REQUIRED_GHIDRA_RELEASE,
            REQUIRED_PYGHIDRA_VERSION,
        ]
    )
    return {
        "seed_sha256": seed["sha256"],
        "replay_input_sha256": replay_hash,
        "materialization_key": hashlib.sha256(payload.encode("utf-8")).hexdigest(),
    }


def materialization_keep_count() -> int:
    """How many of this agent's materializations to retain.

    The key covers the reviewed evidence, so recording an identity or a layout -
    an ordinary part of porting - invalidates the cache and builds another
    project. Without eviction they accumulate at 51MB each; one session left
    2.4GB across 45 of them.
    """

    raw = os.environ.get("WIZ8_GHIDRA_KEEP_MATERIALIZATIONS", "3")
    try:
        return max(1, int(raw))
    except ValueError:
        return 3


def prune_materializations(root: Path, keep: int, current: Path | None = None) -> list[str]:
    """Drop all but the newest `keep` projects under one agent's root.

    Only ever called with this agent's own root. Another agent's projects are
    not ours to remove, and one of them may be open.
    """

    projects_dir = root / "projects"
    if not projects_dir.is_dir():
        return []
    projects = [path for path in projects_dir.iterdir() if path.is_dir()]
    resolved_current = current.resolve() if current is not None else None
    ordered = sorted(projects, key=lambda path: path.stat().st_mtime, reverse=True)
    removed: list[str] = []
    for index, path in enumerate(ordered):
        if index < keep:
            continue
        if resolved_current is not None and path.resolve() == resolved_current:
            continue
        shutil.rmtree(path, ignore_errors=True)
        removed.append(path.name)
    return removed


def _agent_settings(settings: Settings, identity: dict[str, str]) -> tuple[Settings, Path, Path]:
    owner = ghidra_agent_id()
    root = settings.work_dir / "ghidra-agents" / owner
    project_dir = root / "projects" / identity["materialization_key"][:20]
    # Linux AF_UNIX socket paths are limited to roughly 108 bytes. Keep the
    # runtime path short even when WIZ8_WORK_DIR and CODEX_THREAD_ID are long.
    runtime_dir = settings.ghidra_runtime_dir
    effective = settings.model_copy(
        update={
            "ghidra_project_dir_override": project_dir,
            "ghidra_runtime_dir_override": runtime_dir,
        }
    )
    return effective, root, project_dir / "materialization.json"


@contextmanager
def _materialization_lock(root: Path) -> Any:
    import fcntl

    root.mkdir(parents=True, exist_ok=True)
    with (root / "materialize.lock").open("a+", encoding="utf-8") as stream:
        fcntl.flock(stream.fileno(), fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(stream.fileno(), fcntl.LOCK_UN)


def _cached_marker(
    marker_path: Path, identity: dict[str, str], program_name: str, project_name: str
) -> dict[str, Any] | None:
    if not marker_path.is_file() or not (marker_path.parent / f"{project_name}.gpr").is_file():
        return None
    try:
        marker = json.loads(marker_path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None
    expected = {
        "schema": MATERIALIZATION_SCHEMA,
        "program": program_name,
        **identity,
    }
    if any(marker.get(key) != value for key, value in expected.items()):
        return None
    return marker


def materialize_program(
    settings: Settings,
    selector: str | None = None,
) -> tuple[Settings, dict[str, Any]]:
    """Restore, replay, and validate one per-agent program from its reviewed GZF seed."""

    program_name = resolve_program_name(settings, selector)
    seed = _seed_record(settings, program_name)
    identity = _materialization_identity(settings, program_name, seed)
    effective, root, marker_path = _agent_settings(settings, identity)
    report_path = (
        settings.build_dir / "reports" / "ghidra-cache" / ghidra_agent_id() / f"{program_name}.json"
    )

    with _materialization_lock(root):
        marker = _cached_marker(marker_path, identity, program_name, effective.project_name)
        if marker is not None:
            report = {
                **marker,
                "status": "cached",
                "project_dir": str(effective.project_dir),
                "total_seconds": 0.0,
                "under_one_minute": True,
            }
            atomic_json(report_path, report)
            return effective, report

        from .query_daemon import stop_daemon

        stop_daemon(effective, quiet=True)
        started = perf_counter()
        phases: list[dict[str, Any]] = []

        def phase(name: str, action: Any) -> Any:
            phase_started = perf_counter()
            value = action()
            phases.append({"name": name, "seconds": round(perf_counter() - phase_started, 3)})
            return value

        start_pyghidra(effective)
        import pyghidra
        from ghidra.util.task import TaskMonitor
        from java.io import File

        effective.project_dir.mkdir(parents=True, exist_ok=True)
        project = pyghidra.open_project(effective.project_dir, effective.project_name, create=True)
        try:
            root_folder = project.getProjectData().getRootFolder()
            domain_file = project.getProjectData().getFile("/" + program_name)
            if domain_file is None:
                phase(
                    "restore_gzf",
                    lambda: root_folder.createFile(
                        program_name, File(str(seed["archive"])), TaskMonitor.DUMMY
                    ),
                )
            else:
                phases.append({"name": "restore_gzf", "seconds": 0.0})
            with pyghidra.program_context(project, "/" + program_name) as program:
                actual_hash = program.getOptions("Program Information").getString(HASH_OPTION, None)
                if actual_hash != seed["binary_sha256"]:
                    raise RuntimeError(
                        f"restored program hash metadata mismatch for {program_name}"
                    )
        finally:
            project.close()

        for name, action in reviewed_replay_actions(effective, program_name):
            phase(name, action)
        for name, action in observation_replay_actions(effective, program_name):
            phase(name, action)
        validation = phase(
            "validation",
            lambda: validate_reviewed_replay(effective, program_name, evidence_program="wiz8"),
        )
        if not validation["ok"]:
            # Name the differences. The count alone gives nothing to act on, and
            # re-running the validator by hand does not recover them: it reports a
            # different set because it sees a project the replay has not been
            # applied to, and the shared project is routinely locked by another
            # worktree anyway. This message is the only place the keys are visible.
            shown = validation.get("failures", [])[:10]
            detail = "\n  ".join(
                f"{item.get('kind')} {item.get('key')}\n"
                f"    expected {item.get('expected')}\n"
                f"    actual   {item.get('actual')}"
                for item in shown
            )
            more = validation["failure_count"] - len(shown)
            if more > 0:
                detail += f"\n  ... and {more} more"
            raise RuntimeError(
                f"GZF materialization validation failed with "
                f"{validation['failure_count']} differences:\n  {detail}"
            )
        total = perf_counter() - started
        marker = {
            "schema": MATERIALIZATION_SCHEMA,
            "program": program_name,
            **identity,
            "binary_sha256": seed["binary_sha256"],
            "validation": validation,
            "phases": phases,
        }
        atomic_json(marker_path, marker)
        evicted = prune_materializations(root, materialization_keep_count(), effective.project_dir)
        report = {
            "evicted": evicted,
            **marker,
            "status": "materialized",
            "project_dir": str(effective.project_dir),
            "total_seconds": round(total, 3),
            "under_one_minute": total <= 60,
        }
        atomic_json(report_path, report)
        if total > 60:
            raise RuntimeError(
                f"agent GZF materialization took {total:.3f}s, exceeding the 60s workflow limit"
            )
        return effective, report


def cache_status(settings: Settings, selector: str | None = None) -> dict[str, Any]:
    program_name = resolve_program_name(settings, selector)
    seed = _seed_record(settings, program_name)
    identity = _materialization_identity(settings, program_name, seed)
    effective, _, marker_path = _agent_settings(settings, identity)
    marker = _cached_marker(marker_path, identity, program_name, effective.project_name)
    return {
        "schema": "wiz8.ghidra-cache-status",
        "program": program_name,
        "agent": ghidra_agent_id(),
        "seed": str(seed["archive"]),
        **identity,
        "materialized": marker is not None,
        "project_dir": str(effective.project_dir),
    }


def open_for_mutation(
    settings: Settings,
    selector: str | None = None,
    *,
    materialize: bool = True,
) -> Settings:
    """Prepare a mutating Ghidra command and return the settings it must use.

    A mutation has to run against the caller's own materialized project. Opening
    `settings.project_dir` instead reaches the shared one, where it contends for
    a lock another agent may hold - or takes a lock that blocks them - which is
    exactly what per-agent materialization exists to prevent. Every mutating
    command goes through here so that isolation is not something each one has to
    remember.

    `materialize=False` is for the reviewed replay, which these same commands
    make up: it already runs inside `materialize_program` holding its lock, so
    materializing again would recurse into itself.
    """

    # Deferred for the same reason query_daemon defers its import of this
    # module: the two refer to each other.
    from .query_daemon import stop_daemon

    effective = settings
    if materialize:
        effective, _ = materialize_program(settings, selector)
    stop_daemon(effective, quiet=True)
    start_pyghidra(effective)
    return effective
