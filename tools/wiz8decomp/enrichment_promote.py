"""Promote a measured disposable enrichment-checkpoint candidate into live.

Consumes the frozen ``candidate.gzf`` / ``candidate.sha256`` written by the
checkpoint run. Does not re-pack a mutable project as the authoritative
artifact. Does not rewrite reviewed vendor seeds.
"""

from __future__ import annotations

import json
import shutil
import time
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json, repo_relative, sha256_file

_SCHEMA = "wiz8.enrichment-promote-v1"


def _load_report(run_dir: Path) -> dict[str, Any]:
    path = run_dir / "report.json"
    if not path.is_file():
        raise FileNotFoundError(f"enrichment-checkpoint report missing: {path}")
    report = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(report, dict):
        raise TypeError(f"invalid enrichment-checkpoint report: {path}")
    return report


def _outcomes_ok(report: dict[str, Any]) -> bool:
    if report.get("disposable") is not True:
        return False
    outcomes = report.get("outcomes")
    if isinstance(outcomes, dict):
        return bool(outcomes.get("safe_application")) and bool(outcomes.get("preserved_recovery"))
    return bool(report.get("ok"))


def find_latest_promotable_run(root: Path) -> Path:
    """Newest ``run-*`` under ``root/enrichment-checkpoint`` with a good report."""

    checkpoint_root = root / "enrichment-checkpoint"
    if not checkpoint_root.is_dir():
        raise FileNotFoundError(f"no enrichment-checkpoint runs under {checkpoint_root}")
    candidates: list[tuple[float, Path]] = []
    for path in checkpoint_root.glob("run-*"):
        if not path.is_dir():
            continue
        report_path = path / "report.json"
        if not report_path.is_file():
            continue
        try:
            report = _load_report(path)
        except (OSError, TypeError, json.JSONDecodeError):
            continue
        if not _outcomes_ok(report):
            continue
        if not (path / "candidate.gzf").is_file():
            continue
        candidates.append((report_path.stat().st_mtime, path))
    if not candidates:
        raise FileNotFoundError(
            f"no promotable enrichment-checkpoint run under {checkpoint_root} "
            "(need report.json + candidate.gzf with safe_application and preserved_recovery)"
        )
    candidates.sort(key=lambda item: item[0], reverse=True)
    return candidates[0][1]


def resolve_run_dir(
    settings: Settings,
    *,
    run_dir: Path | None = None,
    from_latest: bool = False,
) -> Path:
    if from_latest and run_dir is not None:
        raise ValueError("pass either a run directory or --from-latest, not both")
    if from_latest:
        return find_latest_promotable_run(settings.build_dir)
    if run_dir is None:
        raise ValueError("pass a disposable run directory or --from-latest")
    path = Path(run_dir).expanduser().resolve()
    if not path.is_dir():
        raise FileNotFoundError(f"enrichment-checkpoint run directory missing: {path}")
    return path


def _read_candidate_sha256(run_dir: Path) -> str:
    sha_path = run_dir / "candidate.sha256"
    if not sha_path.is_file():
        raise FileNotFoundError(
            f"candidate.sha256 missing under {run_dir}; refuse to promote without "
            "a frozen checkpoint artifact"
        )
    text = sha_path.read_text(encoding="utf-8").strip().split()
    if not text:
        raise ValueError(f"empty candidate.sha256: {sha_path}")
    return text[0]


def _resolve_frozen_candidate(run_dir: Path, report: dict[str, Any]) -> tuple[Path, str]:
    gzf = run_dir / "candidate.gzf"
    if not gzf.is_file():
        raise FileNotFoundError(
            f"candidate.gzf missing under {run_dir}; refuse to re-pack a mutable "
            "project as the authoritative promote artifact"
        )
    expected = _read_candidate_sha256(run_dir)
    actual = sha256_file(gzf)
    if actual != expected:
        raise RuntimeError(f"candidate.gzf sha256 mismatch under {run_dir}: {actual} != {expected}")
    reported = (report.get("candidate") or {}).get("sha256")
    if reported and reported != actual:
        raise RuntimeError(f"candidate sha256 disagrees with report.json ({reported} != {actual})")
    return gzf, actual


def _live_replacement_allowed(
    settings: Settings, seed: dict[str, Any], *, force: bool
) -> dict[str, Any]:
    """Return freshness details; raise when a live project exists without ``--force``.

    Seed freshness ``current`` only proves which reviewed GZF initialized the
    project. It does not prove the live analysis is untouched. If a live
    project already exists, require an explicit replacement flag. The previous
    project is moved aside rather than deleted.
    """

    from .ghidra.workspace import project_seed_freshness

    freshness = project_seed_freshness(settings, seed)
    status = str(freshness.get("status"))
    project_file = settings.project_dir / f"{settings.project_name}.gpr"
    live_exists = project_file.is_file()
    if force:
        return freshness
    if not live_exists or status == "not-restored":
        return freshness
    raise RuntimeError(
        f"live Ghidra project exists with freshness {status!r} ({freshness.get('detail')}); "
        "refuse to overwrite without --force"
    )


def _verify_provenance(
    settings: Settings,
    report: dict[str, Any],
    current_seed: dict[str, Any],
    *,
    force: bool,
) -> list[str]:
    """Refuse promotion when candidate inputs disagree with the current checkout."""

    from .enrichment_checkpoint import _collect_input_manifest

    warnings: list[str] = []
    expected_seed = report.get("expected_seed_sha256") or (report.get("seed") or {}).get("sha256")
    seed_program = report.get("seed_program") or (report.get("seed") or {}).get("program")
    if seed_program and seed_program != current_seed["program"]:
        raise RuntimeError(
            f"candidate seed program {seed_program!r} != current reviewed "
            f"{current_seed['program']!r}"
        )
    if expected_seed and expected_seed != current_seed["sha256"]:
        raise RuntimeError(
            f"candidate expected_seed_sha256 {expected_seed} != current reviewed "
            f"{current_seed['sha256']}; refuse to promote onto a different seed revision"
        )

    manifest = report.get("input_manifest")
    if not isinstance(manifest, dict):
        manifest = {}
    current = _collect_input_manifest(
        settings,
        {
            "sha256": current_seed["sha256"],
            "binary_sha256": current_seed.get("binary_sha256"),
            "program": current_seed["program"],
        },
    )
    checks: list[tuple[str, Any, Any]] = [
        ("seed_sha256", manifest.get("seed_sha256"), current.get("seed_sha256")),
        ("binary_sha256", manifest.get("binary_sha256"), current.get("binary_sha256")),
        (
            "source_index_sha256",
            manifest.get("source_index_sha256"),
            current.get("source_index_sha256"),
        ),
        ("ghidra_version", manifest.get("ghidra_version"), current.get("ghidra_version")),
        ("pyghidra_version", manifest.get("pyghidra_version"), current.get("pyghidra_version")),
        ("reccmp_version", manifest.get("reccmp_version"), current.get("reccmp_version")),
    ]
    candidate_tree = (
        (manifest.get("source_tree") or {}) if isinstance(manifest.get("source_tree"), dict) else {}
    )
    current_tree = (
        (current.get("source_tree") or {}) if isinstance(current.get("source_tree"), dict) else {}
    )
    checks.append(
        (
            "source_tree",
            candidate_tree.get("jj_commit_id") or candidate_tree.get("git_commit_id"),
            current_tree.get("jj_commit_id") or current_tree.get("git_commit_id"),
        )
    )
    if report.get("import_source") or manifest.get("pdb_sha256"):
        checks.append(("pdb_sha256", manifest.get("pdb_sha256"), current.get("pdb_sha256")))

    for label, left, right in checks:
        if left in {None, ""} or right in {None, ""}:
            continue
        if left == right:
            continue
        msg = f"input-manifest {label} {left} != current {right}"
        if force:
            warnings.append(msg)
        else:
            raise RuntimeError(msg + "; refuse without --force")
    return warnings


def run_enrichment_promote(
    settings: Settings,
    *,
    run_dir: Path | None = None,
    from_latest: bool = False,
    force: bool = False,
    program_name: str = "wiz8",
) -> dict[str, Any]:
    """Promote one disposable enrichment candidate into the checkout live project."""

    from .ghidra.env import open_project
    from .ghidra.workspace import record_project_seed, seed_record

    resolved = resolve_run_dir(settings, run_dir=run_dir, from_latest=from_latest)
    report = _load_report(resolved)
    if report.get("disposable") is False or report.get("live"):
        raise RuntimeError(
            f"candidate {resolved} is not a disposable enrichment trial; refuse to promote"
        )
    if not _outcomes_ok(report):
        raise RuntimeError(
            f"candidate {resolved} is not promotable: outcomes/ok do not allow promotion"
        )

    gzf_path, candidate_sha256 = _resolve_frozen_candidate(resolved, report)
    current_seed = seed_record(settings, program_name, validate_archive=False)
    provenance_warnings = _verify_provenance(settings, report, current_seed, force=force)

    freshness = _live_replacement_allowed(settings, current_seed, force=force)

    out_dir = settings.build_dir / "enrichment-promote"
    out_dir.mkdir(parents=True, exist_ok=True)

    stamp = time.strftime("%Y%m%dT%H%M%S")
    staging = settings.work_dir / f"ghidra-project-staging-{stamp}"
    if staging.exists():
        staging = settings.work_dir / f"ghidra-project-staging-{stamp}-{int(time.time())}"
    staging.mkdir(parents=True, exist_ok=False)

    staging_settings = settings.model_copy(update={"ghidra_project_dir_override": staging})

    from ghidra.util.task import TaskMonitor  # type: ignore[import-not-found]
    from java.io import File  # type: ignore[import-not-found]

    # Restore into staging and verify before touching live.
    with open_project(staging_settings, create=True) as project:
        project.getProjectData().getRootFolder().createFile(
            str(current_seed["program"]), File(str(gzf_path)), TaskMonitor.DUMMY
        )
        record_project_seed(staging_settings, current_seed)

    staging_gpr = staging / f"{settings.project_name}.gpr"
    if not staging_gpr.is_file():
        shutil.rmtree(staging, ignore_errors=True)
        raise RuntimeError(f"staging restore failed; missing {staging_gpr}")

    aside: Path | None = None
    live = settings.project_dir
    live_gpr = live / f"{settings.project_name}.gpr"
    if live_gpr.is_file() or live.exists():
        aside = settings.work_dir / f"ghidra-project-aside-{stamp}"
        if aside.exists():
            aside = settings.work_dir / f"ghidra-project-aside-{stamp}-{int(time.time())}"
        shutil.move(str(live), str(aside))

    try:
        shutil.move(str(staging), str(live))
    except Exception:
        # Best-effort rollback: put aside back if swap failed mid-way.
        if aside is not None and aside.exists() and not live.exists():
            shutil.move(str(aside), str(live))
        if staging.exists():
            shutil.rmtree(staging, ignore_errors=True)
        raise

    result = {
        "schema": _SCHEMA,
        "ok": True,
        "run_dir": str(resolved),
        "run_id": report.get("run_id"),
        "candidate_gzf": str(gzf_path),
        "candidate_sha256": candidate_sha256,
        "live_ghidra_project": str(settings.project_dir),
        "aside_ghidra_project": str(aside) if aside is not None else None,
        "seed_program": current_seed["program"],
        "expected_seed_sha256": current_seed["sha256"],
        "live_freshness_before": freshness,
        "force": force,
        "provenance_warnings": provenance_warnings,
        "candidate_outcomes": report.get("outcomes"),
    }
    report_path = out_dir / "report.json"
    atomic_json(report_path, result)
    result["report"] = repo_relative(report_path, settings.repo_dir)
    return result
