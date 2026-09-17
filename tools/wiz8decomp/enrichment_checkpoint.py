"""Compose a measured analysis-enrichment checkpoint.

Default mode is read-only measurement plus an actionable plan. Explicit flags
mutate a disposable restored project by default:

- ``--apply-conventions``: source-backed calling conventions
- ``--apply-enrichment``: source-safe typing stages (class structures, this,
  vftables, globals, callbacks, Cosmic Forge, function attributes; thunks stay off)
- ``--import-source``: full ``reccmp-ghidra-import`` projection
- ``--live``: opt-in to mutate the canonical checkout Ghidra project instead

Reviewed GZF refresh remains a separate ``wiz8 ghidra seed refresh`` step.

Quality before/after reuse one pinned oracle corpus under
``build/enrichment-checkpoint/`` (``corpus.json``, ``before.json``,
``after.json``, ``delta.json``, ``report.json``). Optional pain scoring pins
``pain-corpus.json`` once and reuses those addresses for after/delta. Any new
decompiler failure or unexpected apply error hard-gates the trial
(``ok == false`` → nonzero CLI exit).
"""

from __future__ import annotations

import json
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
from .paths import atomic_json
from .prototype_repair import run_prototype_repair
from .vftable_typing import run_vftable_typing

_SCHEMA = "wiz8.enrichment-checkpoint-v1"
_QUALITY_PROFILE = "analysis"


def _step_summary(result: dict[str, Any]) -> dict[str, Any]:
    """Bound nested step payloads kept in the checkpoint report."""

    summary: dict[str, Any] = {}
    for key in (
        "apply",
        "counts",
        "actionable",
        "applied",
        "apply_errors",
        "skipped",
        "report",
        "allow_custom_storage",
        "apply_thunks",
        "skipped_fallback",
    ):
        if key in result:
            summary[key] = result[key]
    return summary


def _prepare_disposable_settings(settings: Settings, program_name: str) -> Settings:
    """Restore a unique disposable Ghidra project under the work directory.

    Each run gets its own directory so concurrent checkouts cannot delete one
    another's candidates. Old runs are left in place for inspection.
    """

    import uuid

    from .ghidra.env import open_project
    from .ghidra.workspace import restore_seed

    run_id = uuid.uuid4().hex[:12]
    project_dir = settings.work_dir / "enrichment-checkpoint" / f"run-{run_id}" / "ghidra-project"
    project_dir.mkdir(parents=True, exist_ok=False)
    derived = settings.model_copy(update={"ghidra_project_dir_override": project_dir})
    with open_project(derived, create=True) as project:
        restore_seed(derived, project, program_name)
    return derived


def _stage_apply_failed(step: dict[str, Any]) -> bool:
    result = step.get("result") or {}
    if not result.get("apply"):
        return False
    return int(result.get("apply_errors") or 0) > 0


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
    measure_quality: bool = True,
    measure_pain: bool = False,
    live: bool = False,
) -> dict[str, Any]:
    """Run the enrichment checkpoint and write ``build/enrichment-checkpoint/``."""

    out_dir = settings.build_dir / "enrichment-checkpoint"
    steps: list[dict[str, Any]] = []
    result: dict[str, Any] = {
        "schema": _SCHEMA,
        "program": program_name,
        "target": target,
        "apply_conventions": apply_conventions,
        "apply_enrichment": apply_enrichment,
        "import_source": import_source,
        "live": live,
        "steps": steps,
    }

    corpus: dict[str, Any] | None = None
    before_report: dict[str, Any] | None = None
    pain_corpus: dict[str, Any] | None = None
    mutated = apply_conventions or apply_enrichment or import_source

    run_settings = settings
    if mutated and not live:
        run_settings = _prepare_disposable_settings(settings, program_name)
        result["ghidra_project"] = str(run_settings.project_dir)
        result["disposable"] = True
    else:
        result["ghidra_project"] = str(run_settings.project_dir)
        result["disposable"] = False

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
        atomic_json(out_dir / "corpus.json", corpus_payload)
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
        write_report(settings, before_report, out_dir=out_dir, stem="before")
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
            atomic_json(out_dir / "pain-corpus.json", pain_payload)
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
            write_report(settings, pain_report, out_dir=out_dir, stem="pain-before")
            result["pain_before"] = pain_eval["summary"]
            result["pain_corpus"] = {
                "size": len(pain_corpus["addresses"]),
                "seed": pain_corpus.get("seed"),
                "match_filter": pain_corpus.get("match_filter"),
            }

    conventions = run_prototype_repair(
        run_settings,
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
        # Conventions may drift after a bulk import; re-apply source CCs.
        if apply_conventions:
            follow = run_prototype_repair(
                run_settings,
                target=target,
                program_name=program_name,
                apply=True,
            )
            steps.append({"step": "prototype-repair-after-import", "result": _step_summary(follow)})

    # Source-safe enrichment stages (dry-run unless --apply-enrichment).
    # Order: promote class Structures, type this, then vftables (this before
    # vftable so Method* this is canonical before FunctionDefinition capture).
    class_structures = run_class_structure_projection(
        run_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
        type_this=False,
    )
    steps.append({"step": "class-structures", "result": _step_summary(class_structures)})
    result["class_structures"] = _step_summary(class_structures)

    this_typing = run_class_this_typing(
        run_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
        allow_custom_storage=False,
    )
    steps.append({"step": "class-this-typing", "result": _step_summary(this_typing)})
    result["class_this_typing"] = _step_summary(this_typing)

    vftables = run_vftable_typing(
        run_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "vftable-typing", "result": _step_summary(vftables)})
    result["vftable_typing"] = _step_summary(vftables)

    globals_typing = run_global_typing(
        run_settings,
        target=target,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "global-typing", "result": _step_summary(globals_typing)})
    result["global_typing"] = _step_summary(globals_typing)

    callbacks = run_callback_typing(
        run_settings,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "callback-typing", "result": _step_summary(callbacks)})
    result["callback_typing"] = _step_summary(callbacks)

    cosmic = run_cosmic_forge_globals(
        run_settings,
        program_name=program_name,
        apply=apply_enrichment,
    )
    steps.append({"step": "cosmic-forge-globals", "result": _step_summary(cosmic)})
    result["cosmic_forge_globals"] = _step_summary(cosmic)

    attributes = run_function_attributes(
        run_settings,
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
        write_report(settings, after_report, out_dir=out_dir, stem="after")
        delta = compute_quality_delta(before_report, after_report)
        atomic_json(out_dir / "delta.json", delta)
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
            write_report(settings, pain_report, out_dir=out_dir, stem="pain-after")
            result["pain_after"] = pain_eval["summary"]
            pain_before_path = out_dir / "pain-before.json"
            if pain_before_path.is_file():
                pain_before_report = json.loads(pain_before_path.read_text(encoding="utf-8"))
                pain_delta = compute_quality_delta(pain_before_report, pain_report)
                atomic_json(out_dir / "pain-delta.json", pain_delta)
                result["pain_delta"] = {
                    "ok": pain_delta["ok"],
                    "totals_delta": pain_delta["totals_delta"],
                    "debt_total_delta": pain_delta["debt_total_delta"],
                    "failure_delta": pain_delta["failure_delta"],
                    "decompiler_regressions": pain_delta["decompiler_regressions"],
                }

    failure_reasons: list[str] = []
    quality_delta = result.get("quality_delta")
    if isinstance(quality_delta, dict) and quality_delta.get("ok") is False:
        failure_reasons.append("quality_delta.ok is false (decompiler regression)")
    pain_delta = result.get("pain_delta")
    if isinstance(pain_delta, dict) and pain_delta.get("ok") is False:
        failure_reasons.append("pain_delta.ok is false (decompiler regression)")
    if isinstance(result.get("reccmp_import"), dict) and result["reccmp_import"].get("ok") is False:
        failure_reasons.append("reccmp-ghidra-import failed")
    for step in steps:
        if _stage_apply_failed(step):
            failure_reasons.append(
                f"{step.get('step')} reported apply_errors="
                f"{(step.get('result') or {}).get('apply_errors')}"
            )

    ok = not failure_reasons
    result["ok"] = ok
    if failure_reasons:
        result["failure_reasons"] = failure_reasons

    result["next"] = [
        "Review build/enrichment-checkpoint/{corpus,before,after,delta,report}.json.",
        "Spot-check affected identities with `uv run wiz8 report context ADDRESS...`.",
        (
            "When a disposable trial is accepted, reproduce with `--live` only if "
            "intentionally mutating the checkout project, then run "
            "`uv run wiz8 ghidra seed refresh wiz8`."
        ),
    ]
    path = out_dir / "report.json"
    atomic_json(path, result)
    result["report"] = str(path.relative_to(settings.repo_dir))
    # Bound stdout: drop nested step payloads.
    return {
        "schema": _SCHEMA,
        "ok": ok,
        "report": result["report"],
        "apply_conventions": apply_conventions,
        "apply_enrichment": apply_enrichment,
        "import_source": import_source,
        "live": live,
        "disposable": result.get("disposable"),
        "ghidra_project": result.get("ghidra_project"),
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
        "vftable_typing": result.get("vftable_typing"),
        "class_this_typing": result.get("class_this_typing"),
        "global_typing": result.get("global_typing"),
        "callback_typing": result.get("callback_typing"),
        "cosmic_forge_globals": result.get("cosmic_forge_globals"),
        "function_attributes": result.get("function_attributes"),
        "next": result["next"],
    }
