"""Objective decompiler-quality scores over high-confidence recovered functions.

The corpus is recovered `FUNCTION` markers (optionally filtered to reccmp
exact/effective). Metrics are pattern counts over Ghidra's decompiled C text so
enrichment changes can be scored without arguing about aesthetics.
"""

from __future__ import annotations

import random
import re
from collections import Counter, defaultdict
from collections.abc import Iterable, Mapping, Sequence
from pathlib import Path
from typing import Any

from .config import Settings
from .paths import atomic_json, atomic_write
from .source_index import source_functions

_SCHEMA = "wiz8.decompiler-quality-v1"
_REVIEWED_CORPUS = Path("evidence/reviewed/wiz8/decompiler-quality-corpus.json")

# Ordered metric keys used in summaries and per-function rows.
METRIC_KEYS: tuple[str, ...] = (
    "undefined",
    "dat_",
    "fun_",
    "concat",
    "sub_extract",
    "code_ptr",
    "param_n",
    "anonymous_indirect_call",
    "untyped_ptr_arith",
    "c_style_cast",
)

_PATTERNS: dict[str, re.Pattern[str]] = {
    "undefined": re.compile(r"\bundefined(?:\d+|1|2|4|8)?\b"),
    "dat_": re.compile(r"\bDAT_[0-9A-Fa-f]+\b"),
    "fun_": re.compile(r"\bFUN_[0-9A-Fa-f]+\b"),
    "concat": re.compile(r"\bCONCAT(?:\d+)?\b"),
    # Ghidra extract/insert helpers: SUB41, SUB84, ...
    "sub_extract": re.compile(r"\bSUB\d+\b"),
    "code_ptr": re.compile(r"\bcode\s*\*"),
    "param_n": re.compile(r"\bparam_\d+\b"),
    "anonymous_indirect_call": re.compile(r"\(\s*\*\s*\*"),
    # *(T *)(expr + N) / *(T *)(expr + N)
    "untyped_ptr_arith": re.compile(
        r"\*\s*\([^;]*?\*\s*\)\s*\(\s*[^)]*?\+\s*(?:0x[0-9A-Fa-f]+|\d+)"
    ),
    # Deliberately coarse: counts parenthesized type-ish casts in C text.
    "c_style_cast": re.compile(
        r"\(\s*(?:const\s+|volatile\s+)*(?:unsigned\s+|signed\s+)?"
        r"(?:void|char|short|int|long|float|double|byte|uint|ulong|undefined\d*"
        r"|u?int\d+|float\d+|struct\s+\w+|class\s+\w+|\w+_t)"
        r"(?:\s*\*+|\s+)\s*\)"
    ),
}


def score_decompiled(text: str | None) -> dict[str, int]:
    """Count residual decompiler-debt tokens in one C rendering."""

    counts = {key: 0 for key in METRIC_KEYS}
    if not text:
        return counts
    for key, pattern in _PATTERNS.items():
        counts[key] = len(pattern.findall(text))
    return counts


def debt_total(counts: Mapping[str, int]) -> int:
    """Scalar used for ranking; every metric contributes equally for now."""

    return sum(int(counts.get(key, 0)) for key in METRIC_KEYS)


def _function_markers(repository: Path, target: str) -> dict[int, Any]:
    return {
        address: marker
        for address, marker in source_functions(repository, target).items()
        if marker.marker_kind == "FUNCTION"
    }


def _stratified_sample(
    markers: Mapping[int, Any],
    *,
    limit: int,
    seed: int,
) -> list[int]:
    """Stable sample spread across translation units rather than address order."""

    if limit <= 0:
        return []
    by_unit: dict[str, list[int]] = defaultdict(list)
    for address in sorted(markers):
        marker = markers[address]
        by_unit[str(marker.source_file)].append(address)
    units = sorted(by_unit)
    rng = random.Random(seed)
    for unit in units:
        addresses = by_unit[unit]
        addresses.sort()
        rng.shuffle(addresses)

    selected: list[int] = []
    # Round-robin so large TUs cannot monopolize a small corpus.
    while len(selected) < limit and by_unit:
        progress = False
        for unit in list(units):
            bucket = by_unit.get(unit)
            if not bucket:
                by_unit.pop(unit, None)
                continue
            selected.append(bucket.pop())
            progress = True
            if len(selected) >= limit:
                break
        if not progress:
            break
        units = sorted(by_unit)
    return sorted(selected)


def _load_reviewed_corpus(repository: Path) -> dict[str, Any] | None:
    path = repository / _REVIEWED_CORPUS
    if not path.is_file():
        return None
    import json

    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict):
        return None
    addresses = payload.get("addresses")
    if not isinstance(addresses, list):
        return None
    return payload


def _parse_address_list(values: Sequence[Any]) -> list[int]:
    selected: list[int] = []
    for value in values:
        if isinstance(value, int):
            selected.append(value)
        else:
            selected.append(int(str(value), 0))
    return selected


def select_corpus(
    repository: Path,
    *,
    target: str = "WIZ8",
    limit: int = 200,
    seed: int = 1,
    addresses: Sequence[int] | None = None,
    require_match: bool | None = None,
    corpus_kind: str = "oracle",
    write_manifest_dir: Path | None = None,
) -> dict[str, Any]:
    """Choose corpus addresses and optional reccmp match filter metadata.

    ``corpus_kind``:

    - ``oracle``: high-confidence recovered functions (``require_match=True`` by
      default) — exact/effective when compare is available. Measures whether
      enrichment damages known truth.
    - ``pain``: prefer ``mismatch|inconclusive`` compare statuses (exclude
      ``exact|effective``). Oversamples FUNCTION markers then filters via
      compare. Measures whether enrichment helps future recovery.

    When ``addresses`` is omitted and a reviewed freeze exists at
    ``evidence/reviewed/wiz8/decompiler-quality-corpus.json``, use those
    addresses (intersected with still-valid FUNCTION markers) for the oracle
    corpus. Otherwise sample as usual and optionally write an inspection copy
    under ``build/decompiler-quality/corpus-manifest.json``.
    """

    kind = corpus_kind.casefold().strip() or "oracle"
    if kind not in {"oracle", "pain"}:
        raise ValueError(f"unsupported corpus_kind: {corpus_kind!r} (expected oracle|pain)")
    if require_match is None:
        require_match = kind == "oracle"

    markers = _function_markers(repository, target)

    if addresses is not None:
        selected = sorted({address for address in addresses if address in markers})
        missing = sorted({address for address in addresses if address not in markers})
        match_rows: list[dict[str, Any]] = []
        if require_match and selected:
            match_rows, selected = _filter_matched(repository, target, selected, limit=limit)
        elif kind == "pain" and selected:
            match_rows, selected = _filter_pain(repository, target, selected, limit=limit)
        else:
            selected = selected[:limit]
        return {
            "addresses": selected,
            "candidates": len(addresses),
            "missing_from_source_index": [f"0x{address:08x}" for address in missing],
            "match_filter": (
                "exact|effective"
                if require_match
                else ("mismatch|inconclusive" if kind == "pain" else None)
            ),
            "matches": match_rows,
            "seed": seed,
            "limit": limit,
            "corpus_kind": kind,
            "corpus_source": "addresses-override",
        }

    # Reviewed freeze is an oracle pin; pain always samples live markers.
    if kind == "oracle":
        reviewed = _load_reviewed_corpus(repository)
        if reviewed is not None:
            frozen = _parse_address_list(reviewed.get("addresses") or [])
            selected = [address for address in frozen if address in markers]
            missing = [address for address in frozen if address not in markers]
            match_rows = []
            if require_match and selected:
                match_rows, selected = _filter_matched(
                    repository, target, selected, limit=limit or len(selected)
                )
            elif limit:
                selected = selected[:limit]
            return {
                "addresses": selected,
                "candidates": len(frozen),
                "missing_from_source_index": [f"0x{address:08x}" for address in missing],
                "match_filter": "exact|effective" if require_match else None,
                "matches": match_rows,
                "seed": reviewed.get("seed", seed),
                "limit": reviewed.get("limit", limit),
                "source_functions": len(markers),
                "corpus_kind": kind,
                "corpus_source": "reviewed-manifest",
                "reviewed_manifest": str(_REVIEWED_CORPUS),
            }

    oversample = min(len(markers), max(limit * 3, limit))
    candidates = _stratified_sample(markers, limit=oversample, seed=seed)
    match_rows = []
    selected = candidates
    if require_match:
        match_rows, selected = _filter_matched(repository, target, candidates, limit=limit)
    elif kind == "pain":
        match_rows, selected = _filter_pain(repository, target, candidates, limit=limit)
    else:
        selected = candidates[:limit]
    result = {
        "addresses": selected,
        "candidates": len(candidates),
        "missing_from_source_index": [],
        "match_filter": (
            "exact|effective"
            if require_match
            else ("mismatch|inconclusive" if kind == "pain" else None)
        ),
        "matches": match_rows,
        "seed": seed,
        "limit": limit,
        "source_functions": len(markers),
        "corpus_kind": kind,
        "corpus_source": "stratified-sample",
    }
    manifest_dir = write_manifest_dir
    if manifest_dir is not None:
        atomic_json(
            manifest_dir / "corpus-manifest.json",
            {
                "schema": "wiz8.decompiler-quality-corpus-v1",
                "seed": seed,
                "limit": limit,
                "corpus_kind": kind,
                "match_filter": result["match_filter"],
                "addresses": [f"0x{address:08x}" for address in selected],
            },
        )
    return result


def _filter_matched(
    repository: Path,
    target: str,
    candidates: Sequence[int],
    *,
    limit: int,
) -> tuple[list[dict[str, Any]], list[int]]:
    from .comparison import compare_selected

    report = compare_selected(repository, target, list(candidates))
    rows: list[dict[str, Any]] = []
    kept: list[int] = []
    for row in report.get("functions", []):
        status = str(row.get("status") or "")
        address = int(str(row["address"]), 0)
        if status not in {"exact", "effective"}:
            continue
        rows.append(
            {
                "address": row["address"],
                "name": row.get("name"),
                "status": status,
            }
        )
        kept.append(address)
        if len(kept) >= limit:
            break
    return rows, kept


def _filter_pain(
    repository: Path,
    target: str,
    candidates: Sequence[int],
    *,
    limit: int,
) -> tuple[list[dict[str, Any]], list[int]]:
    """Keep ``mismatch|inconclusive`` compare rows; exclude exact|effective."""

    from .comparison import compare_selected

    report = compare_selected(repository, target, list(candidates))
    rows: list[dict[str, Any]] = []
    kept: list[int] = []
    for row in report.get("functions", []):
        status = str(row.get("status") or "")
        address = int(str(row["address"]), 0)
        if status in {"exact", "effective"}:
            continue
        if status not in {"mismatch", "inconclusive"}:
            continue
        rows.append(
            {
                "address": row["address"],
                "name": row.get("name"),
                "status": status,
            }
        )
        kept.append(address)
        if len(kept) >= limit:
            break
    return rows, kept


def _resolve_function(program: Any, address: int) -> Any:
    space = program.getAddressFactory().getDefaultAddressSpace()
    entry = space.getAddress(address)
    return program.getFunctionManager().getFunctionAt(entry)


def evaluate_corpus(
    settings: Settings,
    addresses: Sequence[int],
    *,
    program_name: str = "wiz8",
    markers: Mapping[int, Any] | None = None,
    profile: str = "analysis",
) -> dict[str, Any]:
    """Decompile each address and aggregate metric counts."""

    from .ghidra.env import open_program
    from .ghidra.inspect import DecompileSession
    from .ghidra.semantic import decompile_c

    source = markers if markers is not None else _function_markers(settings.repo_dir, "WIZ8")
    functions: list[dict[str, Any]] = []
    totals: Counter[str] = Counter()
    failures = 0

    with open_program(settings, program_name) as program:
        session = DecompileSession(program, profile=profile)
        try:
            for address in addresses:
                marker = source.get(address)
                function = _resolve_function(program, address)
                row: dict[str, Any] = {
                    "address": f"0x{address:08x}",
                    "name": marker.name if marker is not None else None,
                    "source_file": marker.source_file if marker is not None else None,
                }
                if function is None:
                    row["status"] = "missing-function"
                    row["metrics"] = score_decompiled(None)
                    failures += 1
                    functions.append(row)
                    continue
                rendered = decompile_c(program, function, profile=profile, session=session)
                text = rendered.get("decompiled")
                completed = bool(rendered.get("completed")) and text is not None
                metrics = score_decompiled(text if completed else None)
                row["status"] = "ok" if completed else "decompiler-failure"
                row["metrics"] = metrics
                row["debt"] = debt_total(metrics)
                if not completed:
                    row["error"] = rendered.get("error")
                    failures += 1
                else:
                    totals.update(metrics)
                functions.append(row)
        finally:
            session.close()

    ok = [row for row in functions if row.get("status") == "ok"]
    return {
        "profile": profile,
        "functions": functions,
        "summary": {
            "requested": len(addresses),
            "ok": len(ok),
            "failures": failures,
            "totals": {key: int(totals.get(key, 0)) for key in METRIC_KEYS},
            "mean_debt": (sum(row["debt"] for row in ok) / len(ok)) if ok else 0.0,
            "max_debt": max((row["debt"] for row in ok), default=0),
        },
    }


def write_report(
    settings: Settings,
    report: Mapping[str, Any],
    *,
    out_dir: Path | None = None,
    stem: str = "report",
) -> Path:
    """Persist a quality report JSON (and markdown summary) under ``out_dir``."""

    destination = out_dir if out_dir is not None else settings.build_dir / "decompiler-quality"
    path = destination / f"{stem}.json"
    atomic_json(path, report)
    summary = report.get("summary") or {}
    totals = summary.get("totals") or {}
    lines = [
        f"# Decompiler quality ({stem})",
        "",
        f"- corpus: {summary.get('ok', 0)}/{summary.get('requested', 0)} decompiled",
        f"- failures: {summary.get('failures', 0)}",
        f"- mean debt: {summary.get('mean_debt', 0):.2f}",
        f"- max debt: {summary.get('max_debt', 0)}",
        "",
        "| metric | count |",
        "| --- | ---: |",
    ]
    for key in METRIC_KEYS:
        lines.append(f"| `{key}` | {totals.get(key, 0)} |")
    lines.append("")
    atomic_write(destination / f"{stem}.md", "\n".join(lines))
    return path


def compute_quality_delta(
    before: Mapping[str, Any],
    after: Mapping[str, Any],
    *,
    max_debt_total_delta: int = 0,
) -> dict[str, Any]:
    """Per-function metric deltas and per-metric totals delta.

    A status change from ``ok`` to ``decompiler-failure`` / missing is a hard
    regression: that row's debt fields are ``None`` and the trial is ``ok: false``.

    Aggregate ``debt_total_delta`` is part of the gate: an increase above
    ``max_debt_total_delta`` (default 0) fails ``ok``. Pass a reviewed positive
    allowance only when a measured debt increase is intentionally accepted.
    """

    before_rows = {
        str(row.get("address")): row for row in before.get("functions") or [] if row.get("address")
    }
    after_rows = {
        str(row.get("address")): row for row in after.get("functions") or [] if row.get("address")
    }
    addresses = sorted(set(before_rows) | set(after_rows))
    functions: list[dict[str, Any]] = []
    decompiler_regressions: list[dict[str, Any]] = []
    failure_statuses = {"decompiler-failure", "missing-function"}

    for address in addresses:
        left = before_rows.get(address) or {}
        right = after_rows.get(address) or {}
        status_before = left.get("status")
        status_after = right.get("status")
        left_ok = status_before == "ok"
        right_failed = status_after in failure_statuses or (
            status_before == "ok" and status_after is None
        )
        regression = bool(left_ok and right_failed)

        left_metrics = left.get("metrics") or score_decompiled(None)
        right_metrics = right.get("metrics") or score_decompiled(None)
        metrics_delta = {
            key: int(right_metrics.get(key, 0)) - int(left_metrics.get(key, 0))
            for key in METRIC_KEYS
        }
        row: dict[str, Any] = {
            "address": address,
            "name": right.get("name") or left.get("name"),
            "status_before": status_before,
            "status_after": status_after,
            "metrics_delta": metrics_delta,
        }
        if regression:
            row["debt_before"] = None
            row["debt_after"] = None
            row["debt_delta"] = None
            row["decompiler_regression"] = True
            decompiler_regressions.append(
                {
                    "address": address,
                    "name": row["name"],
                    "status_before": status_before,
                    "status_after": status_after or "missing",
                }
            )
        else:
            row["debt_before"] = debt_total(left_metrics)
            row["debt_after"] = debt_total(right_metrics)
            row["debt_delta"] = debt_total(right_metrics) - debt_total(left_metrics)
        functions.append(row)

    before_summary = before.get("summary") or {}
    after_summary = after.get("summary") or {}
    before_totals = before_summary.get("totals") or {}
    after_totals = after_summary.get("totals") or {}
    totals_delta = {
        key: int(after_totals.get(key, 0)) - int(before_totals.get(key, 0)) for key in METRIC_KEYS
    }
    before_debt = sum(int(before_totals.get(key, 0)) for key in METRIC_KEYS)
    after_debt = sum(int(after_totals.get(key, 0)) for key in METRIC_KEYS)
    before_failures = int(before_summary.get("failures") or 0)
    after_failures = int(after_summary.get("failures") or 0)
    failure_delta = after_failures - before_failures
    debt_total_delta = after_debt - before_debt
    ok = (
        failure_delta <= 0
        and not decompiler_regressions
        and debt_total_delta <= max_debt_total_delta
    )
    return {
        "schema": "wiz8.decompiler-quality-delta-v1",
        "ok": ok,
        "totals_delta": totals_delta,
        "debt_total_before": before_debt,
        "debt_total_after": after_debt,
        "debt_total_delta": debt_total_delta,
        "max_debt_total_delta": max_debt_total_delta,
        "failures_before": before_failures,
        "failures_after": after_failures,
        "failure_delta": failure_delta,
        "decompiler_regressions": decompiler_regressions,
        "functions": functions,
    }


def build_quality_report(
    *,
    target: str,
    program_name: str,
    profile: str,
    corpus: Mapping[str, Any],
    evaluation: Mapping[str, Any],
) -> dict[str, Any]:
    """Assemble a full decompiler-quality report payload."""

    return {
        "schema": _SCHEMA,
        "target": target,
        "program": program_name,
        "profile": profile,
        "corpus": {
            "seed": corpus.get("seed"),
            "limit": corpus.get("limit"),
            "candidates": corpus.get("candidates"),
            "match_filter": corpus.get("match_filter"),
            "corpus_kind": corpus.get("corpus_kind"),
            "source_functions": corpus.get("source_functions"),
            "corpus_source": corpus.get("corpus_source"),
            "matches": corpus.get("matches"),
            "missing_from_source_index": corpus.get("missing_from_source_index"),
            "addresses": [f"0x{address:08x}" for address in corpus["addresses"]],
        },
        "summary": evaluation["summary"],
        "functions": evaluation["functions"],
    }


def run_decompiler_quality(
    settings: Settings,
    *,
    target: str = "WIZ8",
    program_name: str = "wiz8",
    limit: int = 200,
    seed: int = 1,
    addresses: Iterable[int] | None = None,
    require_match: bool | None = None,
    corpus_kind: str = "oracle",
    profile: str = "analysis",
    out_dir: Path | None = None,
    report_stem: str = "report",
) -> dict[str, Any]:
    """Select corpus, decompile, score, and write build artifacts."""

    destination = out_dir if out_dir is not None else settings.build_dir / "decompiler-quality"
    address_list = list(addresses) if addresses is not None else None
    corpus = select_corpus(
        settings.repo_dir,
        target=target,
        limit=limit,
        seed=seed,
        addresses=address_list,
        require_match=require_match,
        corpus_kind=corpus_kind,
        write_manifest_dir=destination if address_list is None else None,
    )
    evaluation = evaluate_corpus(
        settings,
        corpus["addresses"],
        program_name=program_name,
        profile=profile,
    )
    report = build_quality_report(
        target=target,
        program_name=program_name,
        profile=profile,
        corpus=corpus,
        evaluation=evaluation,
    )
    path = write_report(settings, report, out_dir=destination, stem=report_stem)
    report["report"] = str(path.relative_to(settings.repo_dir))
    summary_md = destination / f"{report_stem}.md"
    report["summary_markdown"] = str(summary_md.relative_to(settings.repo_dir))
    # CLI emission stays bounded: drop the per-function dump from stdout payload.
    return {
        "schema": _SCHEMA,
        "report": report["report"],
        "summary_markdown": report["summary_markdown"],
        "profile": profile,
        "corpus": {
            "size": len(corpus["addresses"]),
            "seed": corpus.get("seed"),
            "limit": corpus.get("limit"),
            "match_filter": corpus.get("match_filter"),
            "corpus_kind": corpus.get("corpus_kind"),
            "candidates": corpus.get("candidates"),
            "corpus_source": corpus.get("corpus_source"),
        },
        "summary": evaluation["summary"],
    }
