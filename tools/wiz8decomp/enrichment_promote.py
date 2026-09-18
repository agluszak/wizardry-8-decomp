"""Promote a measured disposable enrichment-checkpoint candidate into live.

Packs the candidate project's program to a temporary GZF and replaces the
checkout-owned live Ghidra project only when seed freshness would accept
replacement (or with ``--force``). Does not rewrite reviewed vendor seeds.
"""

from __future__ import annotations

import json
import shutil
import time
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json

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
    if not report.get("ghidra_project"):
        return False
    outcomes = report.get("outcomes")
    if isinstance(outcomes, dict):
        return bool(outcomes.get("safe_application")) and bool(outcomes.get("preserved_recovery"))
    return bool(report.get("ok"))


def find_latest_promotable_run(work_dir: Path) -> Path:
    """Newest ``run-*`` under ``work_dir/enrichment-checkpoint`` with a good report."""

    root = work_dir / "enrichment-checkpoint"
    if not root.is_dir():
        raise FileNotFoundError(f"no enrichment-checkpoint runs under {root}")
    candidates: list[tuple[float, Path]] = []
    for path in root.glob("run-*"):
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
        candidates.append((report_path.stat().st_mtime, path))
    if not candidates:
        raise FileNotFoundError(
            f"no promotable enrichment-checkpoint run under {root} "
            "(need report.json with safe_application and preserved_recovery)"
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
        return find_latest_promotable_run(settings.work_dir)
    if run_dir is None:
        raise ValueError("pass a disposable run directory or --from-latest")
    path = Path(run_dir).expanduser().resolve()
    if not path.is_dir():
        raise FileNotFoundError(f"enrichment-checkpoint run directory missing: {path}")
    return path


def _live_replacement_allowed(
    settings: Settings, seed: dict[str, Any], *, force: bool
) -> dict[str, Any]:
    """Return freshness details; raise when replacement is refused."""

    from .ghidra.workspace import project_seed_freshness

    freshness = project_seed_freshness(settings, seed)
    status = str(freshness.get("status"))
    if force or status in {"not-restored", "current"}:
        return freshness
    raise RuntimeError(
        f"live Ghidra project freshness is {status!r} ({freshness.get('detail')}); "
        "refuse to overwrite without --force"
    )


def _move_aside_live_project(settings: Settings) -> Path | None:
    live = settings.project_dir
    project_file = live / f"{settings.project_name}.gpr"
    if not project_file.is_file() and not live.exists():
        return None
    if not live.exists():
        return None
    stamp = time.strftime("%Y%m%dT%H%M%S")
    aside = settings.work_dir / f"ghidra-project-aside-{stamp}"
    if aside.exists():
        aside = settings.work_dir / f"ghidra-project-aside-{stamp}-{int(time.time())}"
    shutil.move(str(live), str(aside))
    return aside


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
    from .ghidra.export_programs import pack_program_archive
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

    candidate_project = Path(str(report.get("ghidra_project") or "")).expanduser()
    if not candidate_project.is_dir():
        raise FileNotFoundError(f"candidate ghidra_project missing: {candidate_project}")

    expected_seed_sha256 = report.get("expected_seed_sha256") or (report.get("seed") or {}).get(
        "sha256"
    )
    seed_program = report.get("seed_program") or (report.get("seed") or {}).get("program")
    current_seed = seed_record(settings, program_name, validate_archive=False)
    if seed_program and seed_program != current_seed["program"]:
        raise RuntimeError(
            f"candidate seed program {seed_program!r} != current reviewed "
            f"{current_seed['program']!r}"
        )
    if expected_seed_sha256 and expected_seed_sha256 != current_seed["sha256"]:
        raise RuntimeError(
            f"candidate expected_seed_sha256 {expected_seed_sha256} != current reviewed "
            f"{current_seed['sha256']}; refuse to promote onto a different seed revision"
        )

    # Live target is this checkout's configured project_dir (not the candidate).
    freshness = _live_replacement_allowed(settings, current_seed, force=force)

    out_dir = settings.build_dir / "enrichment-promote"
    out_dir.mkdir(parents=True, exist_ok=True)
    gzf_path = out_dir / f"candidate-{resolved.name}.gzf"

    candidate_settings = settings.model_copy(
        update={"ghidra_project_dir_override": candidate_project}
    )
    packed = pack_program_archive(
        candidate_settings,
        str(current_seed["program"]),
        gzf_path,
        expected_binary_sha256=str(current_seed["binary_sha256"]),
    )

    aside = _move_aside_live_project(settings)
    settings.project_dir.mkdir(parents=True, exist_ok=True)

    from ghidra.util.task import TaskMonitor  # type: ignore[import-not-found]
    from java.io import File  # type: ignore[import-not-found]

    with open_project(settings, create=True) as project:
        project.getProjectData().getRootFolder().createFile(
            str(current_seed["program"]), File(str(gzf_path)), TaskMonitor.DUMMY
        )
        record_project_seed(settings, current_seed)

    result = {
        "schema": _SCHEMA,
        "ok": True,
        "run_dir": str(resolved),
        "run_id": report.get("run_id"),
        "candidate_ghidra_project": str(candidate_project),
        "live_ghidra_project": str(settings.project_dir),
        "aside_ghidra_project": str(aside) if aside is not None else None,
        "packed_gzf": str(gzf_path.relative_to(settings.repo_dir)),
        "packed": {
            "sha256": packed["sha256"],
            "function_count": packed.get("function_count"),
            "pack_seconds": packed.get("pack_seconds"),
        },
        "seed_program": current_seed["program"],
        "expected_seed_sha256": current_seed["sha256"],
        "live_freshness_before": freshness,
        "force": force,
        "candidate_outcomes": report.get("outcomes"),
    }
    report_path = out_dir / "report.json"
    atomic_json(report_path, result)
    result["report"] = str(report_path.relative_to(settings.repo_dir))
    return result
