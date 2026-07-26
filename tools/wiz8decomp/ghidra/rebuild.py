from __future__ import annotations

from collections.abc import Callable
from time import perf_counter
from typing import Any

from ..config import Settings
from ..paths import atomic_json
from .apply_function_map import apply_function_map
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
        ("zlib_model", lambda: apply_zlib_model(settings, program_name)),
        ("sgp_model", lambda: apply_sgp_model(settings, program_name)),
        ("wiz8_format_model", lambda: apply_wiz8_format_model(settings, program_name)),
        ("reviewed_class_model", lambda: apply_wiz8_class_model(settings, program_name)),
        (
            "reviewed_signatures",
            lambda: apply_wiz8_signature_fixes(settings, program_name),
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
