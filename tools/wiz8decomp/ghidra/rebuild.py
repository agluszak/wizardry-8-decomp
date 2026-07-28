from __future__ import annotations

from collections.abc import Callable
from time import perf_counter
from typing import Any

from ..config import Settings
from ..paths import atomic_json
from .apply_class_candidates import apply_class_candidates
from .apply_eh_frame_types import apply_eh_frame_types
from .apply_function_map import apply_function_map
from .apply_observation_evidence import apply_observation_evidence
from .apply_provenance import apply_provenance
from .apply_reviewed_vtables import apply_reviewed_vtables
from .apply_screen_dispatch import apply_screen_dispatch
from .apply_sgp_model import apply_sgp_model
from .apply_wiz8_class_model import apply_wiz8_class_model
from .apply_wiz8_format_model import apply_wiz8_format_model
from .apply_wiz8_signature_fixes import apply_wiz8_signature_fixes
from .apply_zlib_model import apply_zlib_model
from .import_programs import import_programs
from .project import resolve_program_name
from .validate_replay import validate_reviewed_replay


def reviewed_replay_actions(
    settings: Settings,
    program_name: str,
    *,
    evidence_program: str = "wiz8",
) -> list[tuple[str, Callable[[], Any]]]:
    """Return the single ordered reviewed replay used by rebuilds and GZF clones."""

    if evidence_program != "wiz8":
        raise ValueError(f"no replay profile exists for evidence program {evidence_program}")
    return [
        (
            "reviewed_function_catalog",
            lambda: apply_function_map(
                settings,
                program_name,
                settings.repo_dir / "evidence" / "reviewed" / evidence_program / "functions.csv",
                materialize=False,
            ),
        ),
        ("zlib_model", lambda: apply_zlib_model(settings, program_name, materialize=False)),
        ("sgp_model", lambda: apply_sgp_model(settings, program_name, materialize=False)),
        (
            "wiz8_format_model",
            lambda: apply_wiz8_format_model(settings, program_name, materialize=False),
        ),
        (
            "reviewed_class_model",
            lambda: apply_wiz8_class_model(settings, program_name, materialize=False),
        ),
        (
            "reviewed_signatures",
            lambda: apply_wiz8_signature_fixes(settings, program_name, materialize=False),
        ),
        (
            "reviewed_typed_vtables",
            lambda: apply_reviewed_vtables(settings, program_name, materialize=False),
        ),
        # Last: stamp each reviewed fact's ledger row at its address anchor, so
        # the program itself answers what is accepted here and why.
        (
            "reviewed_provenance",
            lambda: apply_provenance(settings, program_name, materialize=False),
        ),
    ]


def observation_replay_actions(
    settings: Settings, program_name: str
) -> list[tuple[str, Callable[[], Any]]]:
    """Return neutral machine observations, kept separate from reviewed semantics."""

    return [
        (
            "canonical_neutral_observations",
            lambda: apply_observation_evidence(settings, program_name, materialize=False),
        ),
        (
            "observed_screen_dispatch",
            lambda: apply_screen_dispatch(settings, program_name, materialize=False),
        ),
        # Typed EH frame slots join snapshot unwind facts with abi-backed import
        # demanglings and reviewed destructor identities; the plan layer skips
        # anything whose class is unproven, so this stays evidence-bound.
        (
            "eh_frame_types",
            lambda: apply_eh_frame_types(settings, program_name, materialize=False),
        ),
        # Candidate skeletons and bounded unit attribution, all candidate-marked
        # and derived from tracked inputs; last so nothing stronger runs after.
        (
            "candidate_class_observations",
            lambda: apply_class_candidates(settings, program_name, materialize=False),
        ),
    ]


def rebuild_program(
    settings: Settings,
    selector: str,
    *,
    evidence_program: str = "wiz8",
) -> dict[str, Any]:
    """Fresh-import and deterministically materialize one reviewed analysis view."""

    program_name = resolve_program_name(settings, selector)
    started = perf_counter()
    phases: list[dict[str, Any]] = []
    report: dict[str, Any] = {
        "schema": "wiz8.ghidra-rebuild",
        "program": program_name,
        "evidence_program": evidence_program,
        "phases": phases,
    }
    report_path = settings.build_dir / "reports" / "ghidra-replay" / f"{program_name}.json"

    def phase(name: str, action: Callable[[], Any]) -> Any:
        phase_started = perf_counter()
        try:
            value = action()
        except Exception as error:
            phases.append(
                {
                    "name": name,
                    "seconds": round(perf_counter() - phase_started, 3),
                    "ok": False,
                    "error": str(error),
                }
            )
            report.update(
                {
                    "ok": False,
                    "total_seconds": round(perf_counter() - started, 3),
                    "error": f"{name}: {error}",
                }
            )
            atomic_json(report_path, report)
            raise
        phases.append(
            {"name": name, "seconds": round(perf_counter() - phase_started, 3), "ok": True}
        )
        return value

    phase(
        "fresh_import_and_auto_analysis",
        lambda: import_programs(settings, requested_program=program_name, replace_existing=True),
    )
    for name, action in reviewed_replay_actions(
        settings, program_name, evidence_program=evidence_program
    ):
        phase(name, action)
    # Reviewed types and tables own semantic conclusions. Neutral observations
    # run last and fill only gaps, so they cannot be erased by or overwrite the
    # stronger replay layer.
    for name, action in observation_replay_actions(settings, program_name):
        phase(name, action)
    validation = phase(
        "validation",
        lambda: validate_reviewed_replay(settings, program_name, evidence_program=evidence_program),
    )
    report.update(
        {
            "ok": bool(validation["ok"]),
            "total_seconds": round(perf_counter() - started, 3),
            "under_one_minute": perf_counter() - started <= 60,
            "validation": validation,
        }
    )
    atomic_json(report_path, report)
    if not validation["ok"]:
        raise RuntimeError(
            f"Ghidra replay validation failed with {validation['failure_count']} differences; "
            f"see {report_path}"
        )
    return report
