"""PR comparison statistics against the merge-base build."""

from __future__ import annotations

import json
import re
import statistics
from collections import Counter
from pathlib import Path
from typing import Any

_ANALYZED = frozenset({"differences", "no-differences"})
_NON_EMITTED = ("internal-non-emission", "template-non-emission", "header-emission")

# Callee names Ghidriff records for each side, import thunks included
# ("SR.DLL::srHeap::free", "operator_new", "__3_YAXPAX_Z").
# Keep operation and family separate: a function may legitimately touch more
# than one allocator family, and a call census does not prove pointer ownership.
_SRHEAP_ALLOCATE = re.compile(r"srHeap(?:::|_+)allocate|_allocate_srHeap")
_SRHEAP_FREE = re.compile(r"srHeap(?:::|_+)free|_free_srHeap")
_CRT_ALLOCATE = re.compile(r"(?:^|::)operator_?new|^_+2_YA|\?\?2@|(?:^|::)_?(?:malloc|calloc)$")
_CRT_FREE = re.compile(r"(?:^|::)operator_?delete|^_+3_YA|\?\?3@|(?:^|::)_?free$")
_CRT_RESIZE = re.compile(r"(?:^|::)_?realloc$")


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _similarities(summary: dict[str, Any], ghidriff: dict[str, Any]) -> tuple[list[float], int]:
    """Read Ghidriff facts by exact pairs and count pairs without a recorded ratio."""
    ratios: dict[tuple[int, int], float] = {}
    for function in ghidriff["functions"]["modified"]:
        pair = (int(function["old"]["address"], 16), int(function["new"]["address"], 16))
        if pair in ratios:
            raise ValueError("Ghidriff report has duplicate function pairs")
        ratios[pair] = float(function["ratio"])

    similarities = []
    unscored = 0
    for row in summary.get("functions", ()):
        if row.get("outcome") not in _ANALYZED:
            continue
        if not row.get("code_diff"):
            similarities.append(1.0)
            continue
        pair = (int(row["orig"], 16), int(row["recomp"], 16))
        if pair not in ratios:
            unscored += 1
            continue
        similarities.append(ratios[pair])
    return similarities, unscored


def _allocator_operations(called: list[str]) -> dict[str, list[str]]:
    """Direct allocator call families grouped by operation."""
    operations: dict[str, set[str]] = {}
    for name in called:
        operation = None
        family = None
        if _SRHEAP_ALLOCATE.search(name):
            operation, family = "allocate", "srHeap"
        elif _SRHEAP_FREE.search(name):
            operation, family = "free", "srHeap"
        elif _CRT_ALLOCATE.search(name):
            operation, family = "allocate", "crt"
        elif _CRT_FREE.search(name):
            operation, family = "free", "crt"
        elif _CRT_RESIZE.search(name):
            operation, family = "resize", "crt"
        if operation is not None and family is not None:
            operations.setdefault(operation, set()).add(family)
    return {operation: sorted(families) for operation, families in operations.items()}


def _inline_expanded_calls(
    address: int,
    side: str,
    inline_callees: dict[int, set[int]],
    by_address: dict[int, dict[str, Any]],
) -> list[str]:
    """Callee names on one side, expanding calls to asymmetrically inlined callees.

    This is the call view the inline-normalized comparison sees: a call to a
    callee the other side inlined contributes that callee's own normalized
    calls.
    """
    names: list[str] = []
    pending = [(address, inline_callees.get(address, set()))]
    visited = {address}
    while pending:
        current, expandable = pending.pop()
        for call in by_address[current].get(side, {}).get("calls") or ():
            names.append(str(call.get("name") or ""))
            identity = str(call.get("identity") or "")
            if not identity.startswith("pair:"):
                continue
            callee = int(identity.removeprefix("pair:"), 16)
            if callee in expandable and callee not in visited and callee in by_address:
                visited.add(callee)
                pending.append((callee, expandable | inline_callees.get(callee, set())))
    return names


def allocator_call_disagreements(
    ghidriff: dict[str, Any],
    summary: dict[str, Any] | None = None,
    direct_calls: dict[str, Any] | None = None,
) -> dict[tuple[int, str], dict[str, Any]]:
    """Direct allocator operations whose observed families differ between sides.

    Compare only an operation present on both sides. If one side reaches an
    allocator through a helper while the other inlines it, the call census does
    not contain enough information to compare that operation. Likewise this is
    a call-graph triage signal, not pointer provenance: it does not claim that
    an allocation and free in the same function operate on the same storage.

    With the comparison summary and its direct-call census, a disagreement in a
    function that got an inline-normalization retry is dropped when the
    normalized call view, which expands asymmetrically inlined callees,
    agrees on that operation.
    """
    inline_callees: dict[int, set[int]] = {}
    by_address: dict[int, dict[str, Any]] = {}
    if summary is not None and direct_calls is not None:
        inline_callees = {
            int(row["orig"], 16): {int(callee, 16) for callee in row["inline_callees"]}
            for row in summary.get("functions", ())
            if row.get("inline_callees")
        }
        by_address = {int(row["address"], 16): row for row in direct_calls.get("functions", ())}

    result = {}
    for function in ghidriff["functions"]["modified"]:
        address = int(function["old"]["address"], 16)
        retail_called = list(function["old"].get("called") or [])
        rebuild_called = list(function["new"].get("called") or [])
        retail = _allocator_operations(retail_called)
        rebuild = _allocator_operations(rebuild_called)
        disagreements = [
            operation
            for operation in sorted(retail.keys() & rebuild.keys())
            if retail[operation] != rebuild[operation]
        ]
        if disagreements and address in inline_callees and address in by_address:
            normalized_retail = _allocator_operations(
                retail_called + _inline_expanded_calls(address, "orig", inline_callees, by_address)
            )
            normalized_rebuild = _allocator_operations(
                rebuild_called
                + _inline_expanded_calls(address, "recomp", inline_callees, by_address)
            )
            disagreements = [
                operation
                for operation in disagreements
                if normalized_retail.get(operation) != normalized_rebuild.get(operation)
            ]
        for operation in disagreements:
            result[(address, operation)] = {
                "orig": f"{address:#x}",
                "name": function["old"]["name"],
                "operation": operation,
                "retail": retail[operation],
                "rebuild": rebuild[operation],
            }
    return result


def header_regression_candidates(
    head: dict[str, Any], base: dict[str, Any], includers: dict[str, set[str]]
) -> dict[str, Any]:
    """Clean-to-differing transitions grouped by the changed headers their file includes.

    `includers` maps each changed header to the source files that include it
    directly (and the header itself). A regression whose marker file directly
    includes changed headers is a candidate for those headers; it is not proof
    that any of them caused it. Regressions whose file includes no changed
    header directly are only counted.
    """
    previous = {int(row["orig"], 16): row for row in base.get("functions", ())}
    regressed = [
        row
        for row in head.get("functions", ())
        if (before := previous.get(int(row["orig"], 16))) is not None
        and before.get("outcome") == "no-differences"
        and row.get("outcome") == "differences"
        and row.get("source")
    ]
    grouped: dict[tuple[str, ...], list[dict[str, Any]]] = {}
    for row in regressed:
        headers = tuple(
            sorted(header for header, files in includers.items() if row["source"]["path"] in files)
        )
        if headers:
            grouped.setdefault(headers, []).append(row)
    groups = [
        {
            "headers": list(headers),
            "newly_different": len(members),
            "representatives": [{"orig": row["orig"], "name": row["name"]} for row in members[:10]],
        }
        for headers, members in grouped.items()
    ]
    groups.sort(key=lambda group: (-group["newly_different"], group["headers"]))
    return {
        "newly_different": len(regressed),
        "without_direct_changed_header": len(regressed)
        - sum(group["newly_different"] for group in groups),
        "groups": groups,
    }


def comparison_metrics(summary: dict[str, Any], ghidriff: dict[str, Any]) -> dict[str, Any]:
    functions = list(summary.get("functions", ()))
    outcomes = Counter(str(row.get("outcome") or "") for row in functions)
    analyzed = outcomes["differences"] + outcomes["no-differences"]
    similarities, similarity_unscored = _similarities(summary, ghidriff)
    clean = outcomes["no-differences"]
    retried = [row for row in functions if row.get("inline_callees")]
    return {
        "requested": int(summary.get("requested") or len(functions)),
        "analyzed": analyzed,
        "average_similarity": statistics.fmean(similarities) if similarities else None,
        "median_similarity": statistics.median(similarities) if similarities else None,
        "similarity_scored": len(similarities),
        "similarity_unscored": similarity_unscored,
        "similarity_coverage": len(similarities) / analyzed if analyzed else None,
        "clean": clean,
        "clean_rate": clean / analyzed if analyzed else None,
        "differences": outcomes["differences"],
        "code_differences": sum(
            bool(row.get("code_diff")) for row in functions if row.get("outcome") in _ANALYZED
        ),
        "signature_differences": sum(bool(row.get("signature_diff")) for row in functions),
        "scalar_signedness_differences": sum(
            row.get("code_change_kind") == "scalar-signedness" for row in functions
        ),
        "data_differences": sum(
            bool(row.get("data")) for row in functions if row.get("outcome") in _ANALYZED
        ),
        "non_emitted": sum(outcomes[kind] for kind in _NON_EMITTED),
        "internal_non_emission": outcomes["internal-non-emission"],
        "template_non_emission": outcomes["template-non-emission"],
        "header_emission": outcomes["header-emission"],
        "missing": outcomes["missing"],
        "unpaired": outcomes["unpaired"],
        "analysis_failed": outcomes["analysis-failed"],
        "unidentified_references": sum(
            int(row.get("unidentified_references") or 0) for row in functions
        ),
        "inline_retries": len(retried),
        "inline_normalized_clean": sum(row.get("outcome") == "no-differences" for row in retried),
        "inline_still_different": sum(row.get("outcome") == "differences" for row in retried),
        "inline_retry_failures": sum(row.get("inline_normalized_diff") is None for row in retried),
    }


def datacmp_metrics(report: dict[str, Any]) -> dict[str, Any]:
    count = int(report.get("count") or 0)
    issues = list(report.get("issues") or ())
    issue_count = int(report.get("issue_count") or len(issues))
    matched = max(0, count - issue_count)
    return {
        "count": count,
        "matched": matched,
        "issue_count": issue_count,
        "match_rate": matched / count if count else None,
        "field_differences": sum(int(issue.get("difference_count") or 0) for issue in issues),
        "raw_only_issues": sum(bool(issue.get("raw_only")) for issue in issues),
    }


def _difference(head: Any, base: Any) -> Any:
    if head is None or base is None:
        return None
    return head - base


def _metric_delta(head: dict[str, Any], base: dict[str, Any]) -> dict[str, Any]:
    return {
        key: _difference(head.get(key), base.get(key))
        for key in head.keys() & base.keys()
        if isinstance(head.get(key), (int, float))
        and not isinstance(head.get(key), bool)
        and isinstance(base.get(key), (int, float))
        and not isinstance(base.get(key), bool)
    }


def _transitions(head: dict[str, Any], base: dict[str, Any]) -> dict[str, int]:
    current = {
        int(row["orig"], 16): str(row.get("outcome") or "") for row in head.get("functions", ())
    }
    previous = {
        int(row["orig"], 16): str(row.get("outcome") or "") for row in base.get("functions", ())
    }
    common = current.keys() & previous.keys()
    return {
        "resolved": sum(
            previous[address] == "differences" and current[address] == "no-differences"
            for address in common
        ),
        "newly_different": sum(
            previous[address] == "no-differences" and current[address] == "differences"
            for address in common
        ),
        "newly_paired": sum(
            previous[address] == "unpaired" and current[address] in _ANALYZED for address in common
        ),
        "newly_unpaired": sum(
            previous[address] in _ANALYZED and current[address] == "unpaired" for address in common
        ),
        "new_analysis_failures": sum(
            current[address] == "analysis-failed" and previous[address] != "analysis-failed"
            for address in common
        ),
        "head_only": len(current.keys() - previous.keys()),
        "base_only": len(previous.keys() - current.keys()),
    }


def non_emission_regressions(head: dict[str, Any], base: dict[str, Any]) -> list[dict[str, Any]]:
    """Existing PDB procedures may not disappear behind a debt classification."""
    previous = {int(row["orig"], 16): row for row in base.get("functions", ())}
    return [
        {"orig": row["orig"], "name": row.get("name"), "outcome": row["outcome"]}
        for row in head.get("functions", ())
        if row.get("outcome") == "internal-non-emission"
        and (before := previous.get(int(row["orig"], 16))) is not None
        and before.get("recomp") is not None
    ]


def export_delta(head: dict[str, Any], base: dict[str, Any]) -> dict[str, Any]:
    """Existing compiler exports are debt; additions, including replacements, fail."""
    current = set(head["compiler_exports_absent_from_retail"])
    previous = set(base["compiler_exports_absent_from_retail"])
    return {
        "head": sorted(current),
        "base": sorted(previous),
        "added": sorted(current - previous),
        "removed": sorted(previous - current),
        "delta": len(current) - len(previous),
    }


def pr_comparison_report(
    target: str,
    *,
    head_summary_path: Path | None = None,
    base_summary_path: Path | None = None,
    head_ghidriff_path: Path | None = None,
    base_ghidriff_path: Path | None = None,
    head_datacmp_path: Path | None = None,
    base_datacmp_path: Path | None = None,
    head_direct_calls_path: Path | None = None,
    base_direct_calls_path: Path | None = None,
    header_includers: dict[str, set[str]] | None = None,
    head_exports_path: Path | None = None,
    base_exports_path: Path | None = None,
) -> dict[str, Any]:
    report: dict[str, Any] = {
        "schema": "wiz8.pr-comparison-v1",
        "ok": True,
        "emission_regressions": [],
        "exports": None,
        "target": target,
        "comparison": None,
        "datacmp": None,
    }

    if (
        target == "SURRENDER"
        and (head_summary_path is not None or head_exports_path is not None)
        and (head_exports_path is None or base_exports_path is None)
    ):
        raise ValueError("SurRender fidelity requires both head and merge-base export reports")
    if head_exports_path is not None or base_exports_path is not None:
        if head_exports_path is None or base_exports_path is None:
            raise ValueError("export delta requires both head and base reports")
        report["exports"] = export_delta(
            _read_json(head_exports_path), _read_json(base_exports_path)
        )
        report["ok"] = not report["exports"]["added"]

    datacmp_paths = (head_datacmp_path, base_datacmp_path)
    if any(path is not None for path in datacmp_paths):
        if any(path is None for path in datacmp_paths):
            raise ValueError("datacmp delta requires both head and base reports")
        assert head_datacmp_path is not None
        assert base_datacmp_path is not None
        head_data = datacmp_metrics(_read_json(head_datacmp_path))
        base_data = datacmp_metrics(_read_json(base_datacmp_path))
        report["datacmp"] = {
            "head": head_data,
            "base": base_data,
            "delta": _metric_delta(head_data, base_data),
        }

    paths = (
        head_summary_path,
        base_summary_path,
        head_ghidriff_path,
        base_ghidriff_path,
    )
    if not any(path is not None for path in paths):
        return report
    if any(path is None for path in paths):
        raise ValueError("comparison delta requires both summaries and both Ghidriff reports")

    assert head_summary_path is not None
    assert base_summary_path is not None
    assert head_ghidriff_path is not None
    assert base_ghidriff_path is not None
    head_summary = _read_json(head_summary_path)
    base_summary = _read_json(base_summary_path)
    if head_summary.get("target") != target or base_summary.get("target") != target:
        raise ValueError("comparison summary target does not match requested target")

    head_ghidriff = _read_json(head_ghidriff_path)
    base_ghidriff = _read_json(base_ghidriff_path)
    report["emission_regressions"] = non_emission_regressions(head_summary, base_summary)
    report["ok"] = report["ok"] and not report["emission_regressions"]
    head_metrics = comparison_metrics(head_summary, head_ghidriff)
    base_metrics = comparison_metrics(base_summary, base_ghidriff)
    head_allocators = allocator_call_disagreements(
        head_ghidriff,
        head_summary,
        _read_json(head_direct_calls_path) if head_direct_calls_path is not None else None,
    )
    base_allocators = allocator_call_disagreements(
        base_ghidriff,
        base_summary,
        _read_json(base_direct_calls_path) if base_direct_calls_path is not None else None,
    )
    report["comparison"] = {
        "head": head_metrics,
        "base": base_metrics,
        "delta": _metric_delta(head_metrics, base_metrics),
        "transitions": _transitions(head_summary, base_summary),
        "allocator_calls": {
            "head": len(head_allocators),
            "base": len(base_allocators),
            "new": [
                head_allocators[key]
                for key in sorted(head_allocators.keys() - base_allocators.keys())
            ],
        },
        "header_candidates": header_regression_candidates(
            head_summary, base_summary, header_includers
        )
        if header_includers is not None
        else None,
    }
    return report
