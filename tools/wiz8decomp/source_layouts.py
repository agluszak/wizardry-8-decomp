"""Run and compare focused source-layout audits against live Ghidra types."""

from __future__ import annotations

import csv
import hashlib
import io
import json
from pathlib import Path
from typing import Any

from .paths import atomic_json, atomic_write

REVIEW_STATE_FAILURES = frozenset({"missing-ghidra-class", "missing-field"})
BASELINE_COLUMNS = ("kind", "class", "field", "expected", "actual")
DEFAULT_BASELINE = Path("config/verification/source-layout-baseline.csv")


def verify_source_layouts(settings: Any, pdb: Path | None = None) -> dict[str, Any]:
    """Run the audit in a cached derived project without touching reviewed state."""

    from .ghidra.env import open_project
    from .ghidra.reccmp_import import import_reccmp_source
    from .ghidra.recovery import _program_name, run_headless_script
    from .ghidra.workspace import restore_seed, seed_record
    from .paths import sha256_file

    path = pdb or (settings.repo_dir / "build/decomp/Wiz8.pdb")
    if not path.is_file():
        raise ValueError(f"compiled VC6 PDB does not exist: {path}; build WIZ8 first")
    source_index = settings.build_dir / "source-index.json"
    if not source_index.is_file():
        raise ValueError(f"source index does not exist: {source_index}; run `just test` first")
    seed = seed_record(settings, "wiz8")
    digest = hashlib.sha256()
    audit_script = settings.repo_dir / "tools/ghidra-scripts/Wiz8Audit.java"
    for value in (
        sha256_file(path),
        sha256_file(source_index),
        sha256_file(audit_script),
        str(seed["sha256"]),
    ):
        digest.update(value.encode("ascii"))
        digest.update(b"\0")
    cache = settings.build_dir / "ghidra-verify" / digest.hexdigest()[:20]
    cached_report = cache / "source-layouts.json"
    if cached_report.is_file():
        report = json.loads(cached_report.read_text(encoding="utf-8"))
        destination = settings.build_dir / "reports/source-layouts/report.json"
        atomic_json(destination, report)
        report["report"] = str(destination)
        return report

    derived = settings.model_copy(update={"ghidra_project_dir_override": cache / "project"})
    derived.project_dir.mkdir(parents=True, exist_ok=True)
    with open_project(derived, create=True) as project:
        restore_seed(derived, project, "wiz8")
    import_reccmp_source(derived, "wiz8")
    program_name = _program_name(derived, "wiz8")
    transient = run_headless_script(
        derived,
        "Wiz8Audit.java",
        [
            "--audit",
            "source-layouts",
            "--source-index",
            str(source_index),
        ],
        program_name=program_name,
    )
    try:
        report = json.loads(transient.read_text(encoding="utf-8"))
    finally:
        transient.unlink(missing_ok=True)
    report["pdb"] = str(path)
    atomic_json(cached_report, report)
    destination = settings.build_dir / "reports/source-layouts/report.json"
    atomic_json(destination, report)
    report["report"] = str(destination)
    return report


def require_source_layouts(report: dict[str, Any]) -> dict[str, Any]:
    if not report["ok"]:
        raise ValueError(
            f"compiled source layout differs at {report['failure_count']} checks; "
            f"see {report['report']}"
        )
    return report


def _stable_value(value: Any) -> str:
    if value is None:
        return ""
    if isinstance(value, (dict, list)):
        return json.dumps(value, sort_keys=True, separators=(",", ":"))
    return str(value)


def normalize_layout_failure(failure: dict[str, Any]) -> dict[str, str]:
    """Project every audit failure onto a stable semantic comparison key."""

    expected = failure.get("expected", failure.get("expected_pointer_depth"))
    actual = failure.get("actual", failure.get("actual_types"))
    return {
        "kind": str(failure["kind"]),
        "class": str(failure.get("class") or ""),
        "field": str(failure.get("field") or ""),
        "expected": _stable_value(expected),
        "actual": _stable_value(actual),
    }


def layout_failure_key(failure: dict[str, Any]) -> tuple[str, str, str, str, str]:
    normalized = normalize_layout_failure(failure)
    return (
        normalized["kind"],
        normalized["class"],
        normalized["field"],
        normalized["expected"],
        normalized["actual"],
    )


def compare_source_layout_reports(
    current: dict[str, Any],
    baseline: dict[str, Any],
    *,
    baseline_name: str,
) -> dict[str, Any]:
    """Compare two red-or-green reports without blessing either failure set."""

    current_by_key = {
        layout_failure_key(item): normalize_layout_failure(item) for item in current["failures"]
    }
    baseline_by_key = {
        layout_failure_key(item): normalize_layout_failure(item) for item in baseline["failures"]
    }
    introduced_keys = sorted(current_by_key.keys() - baseline_by_key.keys())
    fixed_keys = sorted(baseline_by_key.keys() - current_by_key.keys())
    unchanged_keys = sorted(current_by_key.keys() & baseline_by_key.keys())
    introduced = [current_by_key[key] for key in introduced_keys]
    fixed = [baseline_by_key[key] for key in fixed_keys]

    def signature(item: dict[str, str]) -> tuple[str, str, str]:
        return item["kind"], item["expected"], item["actual"]

    fixed_by_signature: dict[tuple[str, str, str], list[dict[str, str]]] = {}
    for item in fixed:
        fixed_by_signature.setdefault(signature(item), []).append(item)
    spelling_changes = [
        {"baseline": old, "current": item}
        for item in introduced
        for old in fixed_by_signature.get(signature(item), [])
        if (old["class"], old["field"]) != (item["class"], item["field"])
    ]
    new_review_state = [item for item in introduced if item["kind"] in REVIEW_STATE_FAILURES]
    new_contradictions = [item for item in introduced if item["kind"] not in REVIEW_STATE_FAILURES]
    return {
        "schema": "wiz8.source-layout-delta",
        "ok": not introduced,
        "baseline": baseline_name,
        "baseline_failure_count": len(baseline_by_key),
        "current_failure_count": len(current_by_key),
        "introduced_count": len(introduced),
        "fixed_count": len(fixed),
        "unchanged_count": len(unchanged_keys),
        "introduced": introduced,
        "new_contradictions": new_contradictions,
        "new_review_state": new_review_state,
        "fixed": fixed,
        "spelling_changes": spelling_changes,
    }


def load_source_layout_baseline(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise ValueError(
            f"source-layout baseline does not exist: {path}; "
            "run wiz8 analyze source-layouts --write-baseline once on reviewed main"
        )
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    return {
        "schema": "wiz8.source-layout-baseline",
        "failure_count": len(rows),
        "failures": rows,
    }


def write_source_layout_baseline(path: Path, report: dict[str, Any]) -> dict[str, Any]:
    """Initialize a baseline or ratchet an existing one strictly downward."""

    normalized = sorted(
        (normalize_layout_failure(item) for item in report["failures"]),
        key=layout_failure_key,
    )
    if path.is_file():
        previous = load_source_layout_baseline(path)
        previous_keys = {layout_failure_key(item) for item in previous["failures"]}
        additions = [item for item in normalized if layout_failure_key(item) not in previous_keys]
        if additions:
            raise ValueError(
                f"refusing to add {len(additions)} failures to the source-layout baseline"
            )
    output = io.StringIO(newline="")
    writer = csv.DictWriter(output, fieldnames=BASELINE_COLUMNS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(normalized)  # pyright: ignore[reportArgumentType]
    atomic_write(path, output.getvalue())
    return {"baseline": str(path), "failure_count": len(normalized)}


def verify_source_layout_delta(
    settings: Any,
    current: dict[str, Any],
    against: Path | None = None,
) -> dict[str, Any]:
    baseline_path = against or (settings.repo_dir / DEFAULT_BASELINE)
    if not baseline_path.is_absolute():
        baseline_path = settings.repo_dir / baseline_path
    baseline = load_source_layout_baseline(baseline_path)
    delta = compare_source_layout_reports(
        current,
        baseline,
        baseline_name=str(baseline_path),
    )
    destination = settings.build_dir / "reports/source-layouts/delta.json"
    atomic_json(destination, delta)
    delta["report"] = str(destination)
    return delta


def require_source_layout_delta(report: dict[str, Any]) -> dict[str, Any]:
    if not report["ok"]:
        raise ValueError(
            f"source-layout verification introduced {report['introduced_count']} failures "
            f"against {report['baseline']}; see {report['report']}"
        )
    return report
