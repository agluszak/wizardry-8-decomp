"""Audit source-owned layouts against the VC6 PDB and live Ghidra index."""

from __future__ import annotations

import csv
import io
import json
from pathlib import Path
from typing import Any

from .paths import atomic_json, atomic_write
from .source_model import load_source_index

REVIEW_STATE_FAILURES = frozenset({"missing-ghidra-class", "missing-field"})
BASELINE_COLUMNS = ("kind", "class", "field", "expected", "actual")
DEFAULT_BASELINE = Path("config/verification/source-layout-baseline.csv")


def _field_name(name: str) -> str:
    return name.removeprefix("m_")


def _pointer_depth(spelling: str) -> int:
    depth = 0
    value = spelling.rstrip()
    while value.endswith("*"):
        depth += 1
        value = value[:-1].rstrip()
    return depth


def canonical_type_spelling(text: str) -> str:
    """One comparable spelling for a template type name.

    Ghidra cannot store ``<>`` in type names, so the project and the reccmp
    PDB import both use the bracket encoding (``srClassSupport[stLevel,
    srNode,0,65543]``) while source declarations spell real C++
    (``srClassSupport<stLevel, srNode, false, 65543>``). Both sides
    canonicalize to angle brackets, no spaces, ``_#`` as ``*``, and numeric
    booleans, so identical types compare equal regardless of origin.
    """

    spelling = text.replace(" ", "")
    if spelling.endswith("_#"):
        return canonical_type_spelling(spelling[:-2]) + "*"
    for open_char, close_char in (("[", "]"), ("<", ">")):
        start = spelling.find(open_char)
        if start > 0 and spelling.endswith(close_char):
            arguments = _split_template_arguments(spelling[start + 1 : -1])
            return (
                spelling[:start]
                + "<"
                + ",".join(canonical_type_spelling(argument) for argument in arguments)
                + ">"
            )
    return {"false": "0", "true": "1"}.get(spelling, spelling)


def _split_template_arguments(text: str) -> list[str]:
    parts: list[str] = []
    depth = 0
    start = 0
    for index, char in enumerate(text):
        if char in "[<":
            depth += 1
        elif char in "]>":
            depth -= 1
        elif char == "," and depth == 0:
            parts.append(text[start:index])
            start = index + 1
    parts.append(text[start:])
    return parts


def _is_base_component(component: dict[str, Any]) -> bool:
    field = str(component.get("field") or "")
    return field == "base" or field.startswith("base_")


def compare_source_layouts(
    source_classes: list[dict[str, Any]],
    ghidra_types: dict[str, dict[str, Any]],
) -> dict[str, Any]:
    """Check every class made layout-owned by a compiler-enforced size assertion."""

    failures: list[dict[str, Any]] = []
    checks = {"classes": 0, "source_fields": 0, "fields": 0, "bases": 0}
    layout_owned = [item for item in source_classes if item.get("asserted_size") is not None]
    for source_class in layout_owned:
        name = source_class["qualified_name"]
        expected_size = int(source_class["asserted_size"])
        # The reccmp importer materializes the rebuilt PDB class at the root
        # category.  Original reviewed Ghidra layouts remain under
        # /wiz8/classes.  Comparing those two projections keeps PDB decoding in
        # reccmp instead of growing another Wizardry ABI frontend here.
        rebuilt = ghidra_types.get(f"/{name}")
        if rebuilt is None:
            failures.append({"kind": "missing-pdb-class", "class": name})
            continue
        checks["classes"] += 1
        if int(rebuilt["length"]) != expected_size:
            failures.append(
                {
                    "kind": "size",
                    "class": name,
                    "expected": expected_size,
                    "actual": int(rebuilt["length"]),
                }
            )

        actual_bases = {
            canonical_type_spelling(str(component["type"]))
            for component in rebuilt.get("components", [])
            if _is_base_component(component)
        }
        for expected_base in source_class.get("bases", []):
            checks["bases"] += 1
            if canonical_type_spelling(expected_base) not in actual_bases:
                failures.append(
                    {
                        "kind": "base",
                        "class": name,
                        "expected": expected_base,
                        "actual": sorted(actual_bases),
                    }
                )

        rebuilt_fields = {
            str(component.get("field")): component
            for component in rebuilt.get("components", [])
            if component.get("field") not in {None, "vftable"} and not _is_base_component(component)
        }
        for source_field in source_class.get("fields", []):
            checks["source_fields"] += 1
            field_name = str(source_field["name"])
            actual_field = rebuilt_fields.get(field_name)
            if actual_field is None:
                failures.append({"kind": "missing-pdb-field", "class": name, "field": field_name})
                continue
            expected_depth = _pointer_depth(str(source_field["type"]))
            actual_depth = _pointer_depth(str(actual_field["type"]))
            if expected_depth and expected_depth != actual_depth:
                failures.append(
                    {
                        "kind": "source-field-pointer-depth",
                        "class": name,
                        "field": field_name,
                        "expected": expected_depth,
                        "actual": actual_depth,
                    }
                )

        original = ghidra_types.get(f"/wiz8/classes/{name}")
        if original is None:
            failures.append({"kind": "missing-ghidra-class", "class": name})
            continue

        def flattened(data_type: dict[str, Any], origin: int = 0) -> list[dict[str, Any]]:
            values: list[dict[str, Any]] = []
            for component in data_type.get("components", []):
                offset = origin + int(component["offset"])
                base = ghidra_types.get(f"/{component.get('type', '')}")
                if _is_base_component(component) and base is not None:
                    values.extend(flattened(base, offset))
                else:
                    values.append({**component, "offset": offset})
            return values

        source_fields = flattened(rebuilt)
        for component in original.get("components", []):
            field = component.get("field")
            if not field:
                continue
            component_offset = int(component["offset"])
            component_end = component_offset + int(component["length"])
            covering = [
                item
                for item in source_fields
                if int(item["offset"]) < component_end
                and int(item["offset"]) + int(item["length"]) > component_offset
            ]
            covered = {
                byte
                for item in covering
                for byte in range(
                    max(component_offset, int(item["offset"])),
                    min(component_end, int(item["offset"]) + int(item["length"])),
                )
            }
            if len(covered) != int(component["length"]):
                failures.append({"kind": "missing-field", "class": name, "field": field})
                continue
            checks["fields"] += 1
            if "vptr" in field:
                continue
            exact = [
                item
                for item in covering
                if int(item["offset"]) == component_offset
                and int(item["length"]) == int(component["length"])
            ]
            expected_depth = _pointer_depth(str(component["type"]))
            actual_depth = _pointer_depth(str(exact[0]["type"])) if exact else 0
            if expected_depth and actual_depth and actual_depth != expected_depth:
                failures.append(
                    {
                        "kind": "field",
                        "class": name,
                        "field": field,
                        "expected_pointer_depth": expected_depth,
                        "actual_types": [str(item["type"]) for item in exact],
                    }
                )
    return {
        "schema": "wiz8.source-layout-audit",
        "ok": not failures,
        "layout_owned_classes": len(layout_owned),
        "checks": checks,
        "failure_count": len(failures),
        "failures": failures,
    }


def verify_source_layouts(settings: Any, pdb: Path | None = None) -> dict[str, Any]:
    path = pdb or (settings.repo_dir / "build/decomp/Wiz8.pdb")
    if not path.is_file():
        raise ValueError(f"compiled VC6 PDB does not exist: {path}; build WIZ8 first")
    types_path = settings.repo_dir / "build/ghidra-index/types.json"
    if not types_path.is_file():
        raise ValueError(f"Ghidra type index does not exist: {types_path}; run wiz8 ghidra index")
    ghidra_document = json.loads(types_path.read_text(encoding="utf-8"))
    ghidra_types = {item["path"]: item for item in ghidra_document["types"]}
    source_classes = [
        item
        for item in load_source_index(settings.repo_dir)["classes"]
        if item["source_file"].startswith(("src/wiz8/", "include/wiz8/"))
    ]
    report = compare_source_layouts(source_classes, ghidra_types)
    report["pdb"] = str(path)
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
