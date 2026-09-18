"""Compose a measured analysis-enrichment checkpoint.

Default mode is read-only measurement plus an actionable plan. Explicit flags
mutate a disposable restored project by default:

- ``--apply-conventions``: source-backed calling conventions
- ``--apply-enrichment``: source-safe typing stages (class structures, type-graph,
  this, vftables, globals, callbacks, Cosmic Forge, function attributes; thunks stay off)
- ``--cleanup-legacy-classes``: with ``--apply-enrichment``, also delete gated
  leftover ``/wiz8/classes`` types (dry-run inventory always runs)
- ``--import-source``: full ``reccmp-ghidra-import`` projection
- ``--live``: opt-in to mutate the canonical checkout Ghidra project instead

Promote an accepted disposable candidate with
``wiz8 analyze enrichment-promote`` (do not treat a second ``--live`` apply as
equivalent). Reviewed GZF refresh remains a separate
``wiz8 ghidra seed refresh`` step.

Trial artifacts live under ``work_dir/enrichment-checkpoint/run-<id>/``
(``before.json``, ``delta.json``, ``report.json``, ``candidate.gzf``,
``candidate.sha256``, ``input-manifest.json``, per-pass ``steps/``). A convenience
``build/enrichment-checkpoint/latest.json`` may mirror the report after the run.

Outcomes are split:

- ``safe_application``: no unexpected apply errors, import ok, inputs matched
- ``preserved_recovery``: quality/pain deltas report no new decompiler failures
- ``useful_improvement``: True only when measured debt/pain improved with evidence;
  ``None`` if unmeasured. Applied-row count alone is not useful improvement.
  Promotion may still be manually accepted when usefulness is inconclusive.

CLI exit uses ``ok == safe_application and preserved_recovery``.
"""

from __future__ import annotations

import json
import subprocess
import uuid
from pathlib import Path
from typing import Any

from .callback_typing import run_callback_typing
from .class_structure_projection import run_class_structure_projection
from .class_this_typing import run_class_this_typing
from .config import Settings
from .cosmic_forge_globals import run_cosmic_forge_globals
from .decompiler_quality import (
    build_quality_report,
    compute_quality_delta,
    evaluate_corpus,
    select_corpus,
    write_report,
)
from .function_attributes import run_function_attributes
from .global_typing import run_global_typing
from .legacy_classes_cleanup import run_legacy_classes_cleanup
from .paths import atomic_json, repo_relative, sha256_file
from .prototype_repair import run_prototype_repair
from .type_graph_projection import run_type_graph_projection
from .vftable_typing import run_vftable_typing

_SCHEMA = "wiz8.enrichment-checkpoint-v1"
_QUALITY_PROFILE = "analysis"


def _step_summary(result: dict[str, Any]) -> dict[str, Any]:
    """Bound nested step payloads kept in the checkpoint report."""

    summary: dict[str, Any] = {}
    for key in (
        "apply",
        "counts",
        "field_counts",
        "actionable",
        "applied",
        "apply_errors",
        "skipped",
        "report",
        "plan_hash",
        "allow_custom_storage",
        "apply_thunks",
        "skipped_fallback",
    ):
        if key in result:
            summary[key] = result[key]
    return summary


def _prepare_disposable_settings(
    settings: Settings, program_name: str, *, run_id: str, run_dir: Path
) -> tuple[Settings, dict[str, Any]]:
    """Restore a unique disposable Ghidra project under the run directory."""

    from .ghidra.env import open_project
    from .ghidra.workspace import restore_seed, seed_record

    seed = seed_record(settings, program_name, validate_archive=False)
    project_dir = run_dir / "ghidra-project"
    project_dir.mkdir(parents=True, exist_ok=False)
    derived = settings.model_copy(update={"ghidra_project_dir_override": project_dir})
    with open_project(derived, create=True) as project:
        restored = restore_seed(derived, project, program_name)
    seed_info = {
        "program": str(seed["program"]),
        "sha256": str(seed["sha256"]),
        "binary_sha256": str(seed["binary_sha256"]),
        "restore_status": restored.get("status"),
        "seed_freshness": restored.get("seed_freshness"),
    }
    return derived, seed_info


def _stage_apply_failed(step: dict[str, Any]) -> bool:
    result = step.get("result") or {}
    if not result.get("apply"):
        return False
    return int(result.get("apply_errors") or 0) > 0


def _applied_count(result: dict[str, Any]) -> int:
    total = 0
    for key in (
        "prototype_repair",
        "class_structures",
        "type_graph",
        "legacy_classes_cleanup",
        "class_this_typing",
        "vftable_typing",
        "global_typing",
        "callback_typing",
        "cosmic_forge_globals",
        "function_attributes",
    ):
        step = result.get(key)
        if isinstance(step, dict) and step.get("applied") is not None:
            total += int(step["applied"])
    return total


def _debt_improved(delta: dict[str, Any] | None) -> bool:
    if not isinstance(delta, dict):
        return False
    return int(delta.get("debt_total_delta") or 0) < 0


def compute_outcomes(
    result: dict[str, Any],
    *,
    mutated: bool,
    inputs_matched: bool,
) -> tuple[dict[str, bool | None], list[str], list[str]]:
    """Split validation into safe / preserved / useful outcomes."""

    safe_reasons: list[str] = []
    if not inputs_matched:
        safe_reasons.append("inputs did not match expected reviewed seed")
    if isinstance(result.get("reccmp_import"), dict) and result["reccmp_import"].get("ok") is False:
        safe_reasons.append("reccmp-ghidra-import failed")
    for step in result.get("steps") or []:
        if _stage_apply_failed(step):
            safe_reasons.append(
                f"{step.get('step')} reported apply_errors="
                f"{(step.get('result') or {}).get('apply_errors')}"
            )

    preserved_reasons: list[str] = []
    quality_delta = result.get("quality_delta")
    if isinstance(quality_delta, dict) and quality_delta.get("ok") is False:
        preserved_reasons.append("quality_delta.ok is false (decompiler regression)")
    pain_delta = result.get("pain_delta")
    if isinstance(pain_delta, dict) and pain_delta.get("ok") is False:
        preserved_reasons.append("pain_delta.ok is false (decompiler regression)")

    safe_application = not safe_reasons
    preserved_recovery = not preserved_reasons

    quality_measured = isinstance(quality_delta, dict) or isinstance(pain_delta, dict)
    measured = mutated and quality_measured
    useful: bool | None
    if not measured:
        useful = None
    elif not safe_application or not preserved_recovery:
        useful = False
    else:
        useful = _debt_improved(quality_delta if isinstance(quality_delta, dict) else None) or (
            _debt_improved(pain_delta if isinstance(pain_delta, dict) else None)
        )

    outcomes: dict[str, bool | None] = {
        "safe_application": safe_application,
        "preserved_recovery": preserved_recovery,
        "useful_improvement": useful,
    }
    return outcomes, safe_reasons, preserved_reasons


def _source_tree_identity(repo_dir: Path) -> dict[str, Any]:
    identity: dict[str, Any] = {}
    try:
        proc = subprocess.run(
            ["jj", "log", "-r", "@", "-n", "1", "--no-graph", "-T", "commit_id"],
            cwd=repo_dir,
            capture_output=True,
            text=True,
            check=False,
        )
        if proc.returncode == 0 and proc.stdout.strip():
            identity["jj_commit_id"] = proc.stdout.strip().splitlines()[0].strip()
    except OSError:
        pass
    if "jj_commit_id" not in identity:
        try:
            proc = subprocess.run(
                ["git", "rev-parse", "HEAD"],
                cwd=repo_dir,
                capture_output=True,
                text=True,
                check=False,
            )
            if proc.returncode == 0 and proc.stdout.strip():
                identity["git_commit_id"] = proc.stdout.strip()
        except OSError:
            pass
    return identity


def _collect_input_manifest(
    settings: Settings,
    seed_info: dict[str, Any],
    *,
    pdb_hash: str | None = None,
) -> dict[str, Any]:
    """Cheap provenance for promote verification."""

    manifest: dict[str, Any] = {
        "schema": "wiz8.enrichment-input-manifest-v1",
        "seed_sha256": seed_info.get("sha256"),
        "binary_sha256": seed_info.get("binary_sha256"),
        "seed_program": seed_info.get("program"),
        "source_tree": _source_tree_identity(settings.repo_dir),
    }
    source_index = settings.build_dir / "source-index.json"
    if not source_index.is_file():
        # Common alternate locations used by the repo.
        for candidate in (
            settings.build_dir / "reccmp" / "source-index.json",
            settings.repo_dir / "build" / "source-index.json",
        ):
            if candidate.is_file():
                source_index = candidate
                break
    if source_index.is_file():
        try:
            manifest["source_index_sha256"] = sha256_file(source_index)
            manifest["source_index_path"] = repo_relative(source_index, settings.repo_dir)
        except ValueError:
            manifest["source_index_sha256"] = sha256_file(source_index)
            manifest["source_index_path"] = str(source_index)
    if pdb_hash:
        manifest["pdb_sha256"] = pdb_hash
    try:
        from .ghidra.env import validate_environment

        runtime = validate_environment(settings)
        if isinstance(runtime, dict):
            for key in ("ghidra_version", "java_version", "pyghidra_version"):
                if runtime.get(key):
                    manifest[key] = runtime[key]
    except Exception as exc:  # noqa: BLE001
        manifest["ghidra_env_error"] = str(exc)
    try:
        import reccmp

        manifest["reccmp_version"] = getattr(reccmp, "__version__", None) or str(
            getattr(reccmp, "VERSION", "")
        )
    except Exception as exc:  # noqa: BLE001
        manifest["reccmp_version_error"] = str(exc)
    return manifest


def _freeze_candidate(
    settings: Settings,
    *,
    run_dir: Path,
    program_name: str,
    seed_info: dict[str, Any],
) -> dict[str, Any]:
    """Pack the disposable project into ``candidate.gzf`` + ``candidate.sha256``."""

    from .ghidra.export_programs import pack_program_archive

    gzf_path = run_dir / "candidate.gzf"
    packed = pack_program_archive(
        settings,
        program_name,
        gzf_path,
        expected_binary_sha256=str(seed_info.get("binary_sha256") or "") or None,
    )
    sha_path = run_dir / "candidate.sha256"
    sha_path.write_text(f"{packed['sha256']}  candidate.gzf\n", encoding="utf-8")
    return {
        "path": str(gzf_path),
        "sha256": packed["sha256"],
        "sha256_path": str(sha_path),
        "function_count": packed.get("function_count"),
        "pack_seconds": packed.get("pack_seconds"),
    }


def run_enrichment_checkpoint(
    settings: Settings,
    *,
    program_name: str = "wiz8",
    target: str = "WIZ8",
    quality_limit: int = 50,
    quality_seed: int = 1,
    apply_conventions: bool = False,
    apply_enrichment: bool = False,
    import_source: bool = False,
    cleanup_legacy_classes: bool = False,
    measure_quality: bool = True,
    measure_pain: bool = False,
    live: bool = False,
) -> dict[str, Any]:
    """Run the enrichment checkpoint; trial truth lives under ``run-<id>/``."""

    convenience_dir = settings.build_dir / "enrichment-checkpoint"
    steps: list[dict[str, Any]] = []
    run_id = uuid.uuid4().hex[:12]
    run_dir = settings.work_dir / "enrichment-checkpoint" / f"run-{run_id}"
    run_dir.mkdir(parents=True, exist_ok=True)
    steps_dir = run_dir / "steps"
    steps_dir.mkdir(parents=True, exist_ok=True)

    result: dict[str, Any] = {
        "schema": _SCHEMA,
        "program": program_name,
        "target": target,
        "run_id": run_id,
        "apply_conventions": apply_conventions,
        "apply_enrichment": apply_enrichment,
        "import_source": import_source,
        "cleanup_legacy_classes": cleanup_legacy_classes,
        "live": live,
        "steps": steps,
        "run_dir": str(run_dir),
    }

    corpus: dict[str, Any] | None = None
    before_report: dict[str, Any] | None = None
    pain_corpus: dict[str, Any] | None = None
    mutated = (
        apply_conventions
        or apply_enrichment
        or import_source
        or (cleanup_legacy_classes and apply_enrichment)
    )
    inputs_matched = True
    seed_info: dict[str, Any] | None = None

    run_settings = settings
    if mutated and not live:
        run_settings, seed_info = _prepare_disposable_settings(
            settings, program_name, run_id=run_id, run_dir=run_dir
        )
        result["ghidra_project"] = str(run_settings.project_dir)
        result["disposable"] = True
        result["expected_seed_sha256"] = seed_info["sha256"]
        result["seed_program"] = seed_info["program"]
        result["seed"] = seed_info
        inputs_matched = seed_info.get("restore_status") in {
            "restored",
            "already-restored",
        } and bool(seed_info.get("sha256"))
    else:
        from .ghidra.workspace import seed_record

        seed = seed_record(settings, program_name, validate_archive=False)
        seed_info = {
            "program": str(seed["program"]),
            "sha256": str(seed["sha256"]),
            "binary_sha256": str(seed["binary_sha256"]),
        }
        result["ghidra_project"] = str(run_settings.project_dir)
        result["disposable"] = False
        result["expected_seed_sha256"] = seed_info["sha256"]
        result["seed_program"] = seed_info["program"]
        result["seed"] = seed_info

    assert seed_info is not None
    input_manifest = _collect_input_manifest(settings, seed_info)
    atomic_json(run_dir / "input-manifest.json", input_manifest)
    result["input_manifest"] = input_manifest

    # Per-pass reports nest under the run directory when invoked from checkpoint.
    step_settings = run_settings.model_copy(update={"build_dir": steps_dir})

    if measure_quality:
        corpus = select_corpus(
            settings.repo_dir,
            target=target,
            limit=quality_limit,
            seed=quality_seed,
            corpus_kind="oracle",
            write_manifest_dir=None,
        )
        corpus_payload = {
            "schema": "wiz8.decompiler-quality-corpus-v1",
            "seed": corpus.get("seed"),
            "limit": corpus.get("limit"),
            "match_filter": corpus.get("match_filter"),
            "corpus_kind": corpus.get("corpus_kind"),
            "corpus_source": corpus.get("corpus_source"),
            "addresses": [f"0x{address:08x}" for address in corpus["addresses"]],
        }
        atomic_json(run_dir / "corpus.json", corpus_payload)
        result["corpus"] = {
            "size": len(corpus["addresses"]),
            "seed": corpus.get("seed"),
            "limit": corpus.get("limit"),
            "corpus_kind": corpus.get("corpus_kind"),
            "corpus_source": corpus.get("corpus_source"),
        }

        before_eval = evaluate_corpus(
            run_settings,
            corpus["addresses"],
            program_name=program_name,
            profile=_QUALITY_PROFILE,
        )
        before_report = build_quality_report(
            target=target,
            program_name=program_name,
            profile=_QUALITY_PROFILE,
            corpus=corpus,
            evaluation=before_eval,
        )
        write_report(settings, before_report, out_dir=run_dir, stem="before")
        steps.append(
            {
                "step": "decompiler-quality-before",
                "result": {"summary": before_eval["summary"]},
            }
        )
        result["quality_before"] = before_eval["summary"]

        if measure_pain:
            pain_corpus = select_corpus(
                settings.repo_dir,
                target=target,
                limit=quality_limit,
                seed=quality_seed,
                corpus_kind="pain",
                write_manifest_dir=None,
            )
            pain_payload = {
                "schema": "wiz8.decompiler-quality-corpus-v1",
                "seed": pain_corpus.get("seed"),
                "limit": pain_corpus.get("limit"),
                "match_filter": pain_corpus.get("match_filter"),
                "corpus_kind": pain_corpus.get("corpus_kind"),
                "corpus_source": pain_corpus.get("corpus_source"),
                "addresses": [f"0x{address:08x}" for address in pain_corpus["addresses"]],
            }
            atomic_json(run_dir / "pain-corpus.json", pain_payload)
            pain_eval = evaluate_corpus(
                run_settings,
                pain_corpus["addresses"],
                program_name=program_name,
                profile=_QUALITY_PROFILE,
            )
            pain_report = build_quality_report(
                target=target,
                program_name=program_name,
                profile=_QUALITY_PROFILE,
                corpus=pain_corpus,
                evaluation=pain_eval,
            )
            write_report(settings, pain_report, out_dir=run_dir, stem="pain-before")
            result["pain_before"] = pain_eval["summary"]
            result["pain_corpus"] = {
                "size": len(pain_corpus["addresses"]),
                "seed": pain_corpus.get("seed"),
                "match_filter": pain_corpus.get("match_filter"),
            }

    conventions = run_prototype_repair(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_conventions,
    )
    steps.append({"step": "prototype-repair", "result": _step_summary(conventions)})
    result["prototype_repair"] = {
        "apply": apply_conventions,
        "counts": conventions.get("counts"),
        "actionable": conventions.get("actionable"),
        "applied": conventions.get("applied"),
        "apply_errors": conventions.get("apply_errors"),
    }

    if import_source:
        from .ghidra.reccmp_import import import_reccmp_source

        imported = import_reccmp_source(run_settings, program_name)
        steps.append({"step": "reccmp-ghidra-import", "result": imported})
        result["reccmp_import"] = {
            "ok": imported.get("command", {}).get("returncode") == 0,
            "program": imported.get("program"),
        }
        pdb_hash = None
        if isinstance(imported, dict):
            pdb_hash = imported.get("pdb_sha256") or (imported.get("pdb") or {}).get("sha256")
        if pdb_hash:
            input_manifest["pdb_sha256"] = pdb_hash
            atomic_json(run_dir / "input-manifest.json", input_manifest)
            result["input_manifest"] = input_manifest
        if apply_conventions:
            follow = run_prototype_repair(
                step_settings,
                target=target,
                program_name=program_name,
                apply=True,
            )
            steps.append({"step": "prototype-repair-after-import", "result": _step_summary(follow)})

    class_structures = run_class_structure_projection(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
        type_this=False,
    )
    steps.append({"step": "class-structures", "result": _step_summary(class_structures)})
    result["class_structures"] = _step_summary(class_structures)

    type_graph = run_type_graph_projection(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "type-graph", "result": _step_summary(type_graph)})
    result["type_graph"] = _step_summary(type_graph)

    identity_map = None
    report_rel = type_graph.get("report")
    if report_rel:
        report_path = settings.repo_dir / str(report_rel)
        if not report_path.is_file():
            report_path = steps_dir / "type-graph-projection" / "report.json"
        if report_path.is_file():
            identity_map = json.loads(report_path.read_text(encoding="utf-8")).get("identity_map")
    legacy_cleanup = run_legacy_classes_cleanup(
        step_settings,
        program_name=program_name,
        apply=bool(apply_enrichment and cleanup_legacy_classes),
        identity_map_or_bindings=identity_map,
    )
    steps.append({"step": "legacy-classes-cleanup", "result": _step_summary(legacy_cleanup)})
    result["legacy_classes_cleanup"] = _step_summary(legacy_cleanup)

    this_typing = run_class_this_typing(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
        allow_custom_storage=False,
    )
    steps.append({"step": "class-this-typing", "result": _step_summary(this_typing)})
    result["class_this_typing"] = _step_summary(this_typing)

    vftables = run_vftable_typing(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "vftable-typing", "result": _step_summary(vftables)})
    result["vftable_typing"] = _step_summary(vftables)

    globals_typing = run_global_typing(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "global-typing", "result": _step_summary(globals_typing)})
    result["global_typing"] = _step_summary(globals_typing)

    callbacks = run_callback_typing(
        step_settings,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "callback-typing", "result": _step_summary(callbacks)})
    result["callback_typing"] = _step_summary(callbacks)

    cosmic = run_cosmic_forge_globals(
        step_settings,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "cosmic-forge-globals", "result": _step_summary(cosmic)})
    result["cosmic_forge_globals"] = _step_summary(cosmic)

    attributes = run_function_attributes(
        step_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
        apply_thunks=False,
    )
    steps.append({"step": "function-attributes", "result": _step_summary(attributes)})
    result["function_attributes"] = _step_summary(attributes)

    if measure_quality and corpus is not None and before_report is not None and mutated:
        after_eval = evaluate_corpus(
            run_settings,
            corpus["addresses"],
            program_name=program_name,
            profile=_QUALITY_PROFILE,
        )
        after_report = build_quality_report(
            target=target,
            program_name=program_name,
            profile=_QUALITY_PROFILE,
            corpus=corpus,
            evaluation=after_eval,
        )
        write_report(settings, after_report, out_dir=run_dir, stem="after")
        delta = compute_quality_delta(before_report, after_report)
        atomic_json(run_dir / "delta.json", delta)
        steps.append(
            {
                "step": "decompiler-quality-after",
                "result": {"summary": after_eval["summary"], "ok": delta.get("ok")},
            }
        )
        result["quality_after"] = after_eval["summary"]
        result["quality_delta"] = {
            "ok": delta["ok"],
            "totals_delta": delta["totals_delta"],
            "debt_total_delta": delta["debt_total_delta"],
            "failure_delta": delta["failure_delta"],
            "decompiler_regressions": delta["decompiler_regressions"],
        }

        if measure_pain and pain_corpus is not None:
            pain_eval = evaluate_corpus(
                run_settings,
                pain_corpus["addresses"],
                program_name=program_name,
                profile=_QUALITY_PROFILE,
            )
            pain_report = build_quality_report(
                target=target,
                program_name=program_name,
                profile=_QUALITY_PROFILE,
                corpus=pain_corpus,
                evaluation=pain_eval,
            )
            write_report(settings, pain_report, out_dir=run_dir, stem="pain-after")
            result["pain_after"] = pain_eval["summary"]
            pain_before_path = run_dir / "pain-before.json"
            if pain_before_path.is_file():
                pain_before_report = json.loads(pain_before_path.read_text(encoding="utf-8"))
                pain_delta = compute_quality_delta(pain_before_report, pain_report)
                atomic_json(run_dir / "pain-delta.json", pain_delta)
                result["pain_delta"] = {
                    "ok": pain_delta["ok"],
                    "totals_delta": pain_delta["totals_delta"],
                    "debt_total_delta": pain_delta["debt_total_delta"],
                    "failure_delta": pain_delta["failure_delta"],
                    "decompiler_regressions": pain_delta["decompiler_regressions"],
                }

    outcomes, safe_reasons, preserved_reasons = compute_outcomes(
        result, mutated=mutated, inputs_matched=inputs_matched
    )
    result["outcomes"] = outcomes
    result["inputs_matched"] = inputs_matched
    ok = bool(outcomes["safe_application"]) and bool(outcomes["preserved_recovery"])
    result["ok"] = ok
    failure_reasons = safe_reasons + preserved_reasons
    if failure_reasons:
        result["failure_reasons"] = failure_reasons

    # Freeze the exact disposable candidate artifact for promote (always when
    # disposable + mutated so promote never re-packs a mutable project later).
    if result.get("disposable") and mutated:
        try:
            frozen = _freeze_candidate(
                run_settings,
                run_dir=run_dir,
                program_name=str(seed_info.get("program") or program_name),
                seed_info=seed_info,
            )
            result["candidate"] = {
                "gzf": "candidate.gzf",
                "sha256": frozen["sha256"],
                "function_count": frozen.get("function_count"),
                "pack_seconds": frozen.get("pack_seconds"),
            }
        except Exception as exc:  # noqa: BLE001
            result["candidate_freeze_error"] = str(exc)
            if ok:
                result["ok"] = False
                result.setdefault("failure_reasons", []).append(f"candidate freeze failed: {exc}")
                outcomes = {
                    **outcomes,
                    "safe_application": False,
                }
                result["outcomes"] = outcomes
                ok = False

    result["next"] = [
        f"Review {run_dir}/{{before,after,delta,report,candidate.gzf}}.json artifacts.",
        "Spot-check affected identities with `uv run wiz8 report context ADDRESS...`.",
        (
            "When a disposable trial is accepted (outcomes.safe_application and "
            "preserved_recovery), promote that exact candidate with "
            "`uv run wiz8 analyze enrichment-promote --from-latest` "
            "(or pass the run directory). Do not rerun with `--live`."
        ),
    ]
    atomic_json(run_dir / "report.json", result)
    convenience_dir.mkdir(parents=True, exist_ok=True)
    latest_path = convenience_dir / "latest.json"
    atomic_json(latest_path, result)
    result["report"] = repo_relative(run_dir / "report.json", settings.repo_dir)
    result["latest_report"] = repo_relative(latest_path, settings.repo_dir)
    return {
        "schema": _SCHEMA,
        "ok": ok,
        "outcomes": outcomes,
        "run_id": run_id,
        "run_dir": result.get("run_dir"),
        "report": result["report"],
        "latest_report": result.get("latest_report"),
        "candidate": result.get("candidate"),
        "apply_conventions": apply_conventions,
        "apply_enrichment": apply_enrichment,
        "import_source": import_source,
        "cleanup_legacy_classes": cleanup_legacy_classes,
        "live": live,
        "disposable": result.get("disposable"),
        "ghidra_project": result.get("ghidra_project"),
        "expected_seed_sha256": result.get("expected_seed_sha256"),
        "seed_program": result.get("seed_program"),
        "inputs_matched": inputs_matched,
        "input_manifest": result.get("input_manifest"),
        "failure_reasons": result.get("failure_reasons"),
        "corpus": result.get("corpus"),
        "quality_before": result.get("quality_before"),
        "quality_after": result.get("quality_after"),
        "quality_delta": result.get("quality_delta"),
        "pain_corpus": result.get("pain_corpus"),
        "pain_before": result.get("pain_before"),
        "pain_after": result.get("pain_after"),
        "pain_delta": result.get("pain_delta"),
        "prototype_repair": result.get("prototype_repair"),
        "reccmp_import": result.get("reccmp_import"),
        "class_structures": result.get("class_structures"),
        "type_graph": result.get("type_graph"),
        "legacy_classes_cleanup": result.get("legacy_classes_cleanup"),
        "vftable_typing": result.get("vftable_typing"),
        "class_this_typing": result.get("class_this_typing"),
        "global_typing": result.get("global_typing"),
        "callback_typing": result.get("callback_typing"),
        "cosmic_forge_globals": result.get("cosmic_forge_globals"),
        "function_attributes": result.get("function_attributes"),
        "next": result["next"],
    }
