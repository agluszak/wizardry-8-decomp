#!/usr/bin/env python3
from __future__ import annotations

import json
import os


def load(name: str, target: str) -> tuple[str, dict] | None:
    raw = os.environ.get(name, "").strip()
    return (target, json.loads(raw)) if raw else None


def signed_count(value: float | None) -> str:
    if value is None:
        return "n/a"
    return f"{int(value):+d}"


def count_with_delta(value: int | None, delta: float | None) -> str:
    if value is None:
        return "n/a"
    return f"{value:,} ({signed_count(delta)})"


def ratio_with_delta(value: float | None, delta: float | None) -> str:
    if value is None:
        return "n/a"
    if delta is None:
        return f"{value * 100:.2f}%"
    return f"{value * 100:.2f}% ({float(delta) * 100:+.2f} pp)"


rows = [
    row
    for row in (
        load("WIZ8_STATUS", "WIZ8"),
        load("SURRENDER_STATUS", "SURRENDER"),
    )
    if row is not None
]
if not rows:
    raise SystemExit("no reccmp status rows")

print("<!-- wiz8-reccmp-status -->")
print("### reccmp status")
print()
print(
    "_Head values are followed by the change from the PR merge base. "
    "Function quality compares every currently recovered FUNCTION address "
    "against retail on both revisions._"
)
print()
comparison_rows = [(target, row["comparison"]) for target, row in rows if row.get("comparison")]
if comparison_rows:
    print()
    print("#### Comparison coverage and compiler debt")
    print()
    print(
        "| Target | Requested | Analyzed | Non-emitted | Internal | Template | Header | Unpaired | Analysis failed | Missing |"
    )
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
    for target, comparison in comparison_rows:
        head, delta = comparison["head"], comparison["delta"]
        values = [
            count_with_delta(head.get(key), delta.get(key))
            for key in (
                "requested",
                "analyzed",
                "non_emitted",
                "internal_non_emission",
                "template_non_emission",
                "header_emission",
                "unpaired",
                "analysis_failed",
                "missing",
            )
        ]
        print(f"| `{target}` | " + " | ".join(values) + " |")
    for target, row in rows:
        for regression in row.get("emission_regressions", []):
            print(
                f"\n**Emission regression ({target}):** `{regression['orig']}` {regression.get('name') or ''}"
            )

for target, row in rows:
    exports = row.get("exports")
    if exports is not None:
        print(
            f"\n**{target} extra compiler exports:** {len(exports['head'])} ({exports['delta']:+d})."
        )
        for symbol in exports["added"]:
            print(f"\n**New extra export:** `{symbol}`")

if comparison_rows:
    print()
    print("#### Function quality")
    print()
    print("Function quality covers the compared selection.")
    print()
    print(
        "| Target | Compared | Similarity scores | Avg similarity | Median | Exact/clean | Clean rate | "
        "Different | Code diffs | Referenced-data diffs | Fixed | Regressed | Analysis failed |"
    )
    print(
        "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"
    )
    for target, comparison in comparison_rows:
        head = comparison["head"]
        delta = comparison["delta"]
        transitions = comparison["transitions"]
        print(
            f"| `{target}` "
            f"| {count_with_delta(head['analyzed'], delta.get('analyzed'))} "
            f"| {head['similarity_scored']:,}/{head['analyzed']:,} "
            f"| {ratio_with_delta(head.get('average_similarity'), delta.get('average_similarity'))} "
            f"| {ratio_with_delta(head.get('median_similarity'), delta.get('median_similarity'))} "
            f"| {count_with_delta(head['clean'], delta.get('clean'))} "
            f"| {ratio_with_delta(head.get('clean_rate'), delta.get('clean_rate'))} "
            f"| {count_with_delta(head['differences'], delta.get('differences'))} "
            f"| {count_with_delta(head['code_differences'], delta.get('code_differences'))} "
            f"| {count_with_delta(head['data_differences'], delta.get('data_differences'))} "
            f"| {transitions['resolved']:,} "
            f"| {transitions['newly_different']:,} "
            f"| {count_with_delta(head['analysis_failed'], delta.get('analysis_failed'))} |"
        )
    print()
    print(
        "_Similarity is Ghidriff's normalized, signature-ignored code ratio. "
        "Average and median use scored functions only; missing ratios are excluded. "
        "Exact code and data-only differences contribute 100% code similarity._"
    )

if comparison_rows:
    print()
    print("#### Declaration and scalar signedness findings")
    print()
    print(
        "_Inferred declaration changes are separate from body quality. Scalar signedness "
        "findings remain code differences; they are exact int/uint spelling deltas, "
        "not equivalence claims or necessarily enum changes._"
    )
    print()
    print("| Target | Declaration differences | Scalar signedness differences |")
    print("| --- | ---: | ---: |")
    for target, comparison in comparison_rows:
        head, delta = comparison["head"], comparison["delta"]
        print(
            f"| `{target}` "
            f"| {count_with_delta(head.get('signature_differences'), delta.get('signature_differences'))} "
            f"| {count_with_delta(head.get('scalar_signedness_differences'), delta.get('scalar_signedness_differences'))} |"
        )

if comparison_rows:
    print()
    print("#### Inline normalization")
    print()
    print(
        "_Functions whose Ghidriff retry substituted asymmetrically inlined callees. "
        "Their reported outcome and diff are the retry's; retry failures fall back to "
        "analysis failure._"
    )
    print()
    print("| Target | Retries | Normalized clean | Still different | Retry failures |")
    print("| --- | ---: | ---: | ---: | ---: |")
    for target, comparison in comparison_rows:
        head = comparison["head"]
        delta = comparison["delta"]
        print(
            f"| `{target}` "
            f"| {count_with_delta(head.get('inline_retries'), delta.get('inline_retries'))} "
            f"| {count_with_delta(head.get('inline_normalized_clean'), delta.get('inline_normalized_clean'))} "
            f"| {count_with_delta(head.get('inline_still_different'), delta.get('inline_still_different'))} "
            f"| {count_with_delta(head.get('inline_retry_failures'), delta.get('inline_retry_failures'))} |"
        )

allocator_rows = [
    (target, comparison["allocator_calls"])
    for target, comparison in comparison_rows
    if comparison.get("allocator_calls") and comparison["allocator_calls"]["new"]
]
if allocator_rows:
    print()
    print("#### New allocator call-family disagreements")
    print()
    print(
        "_For the same direct allocator operation, both sides call an allocator but the "
        "observed families differ. This is a call-graph triage signal, not pointer-proven "
        "ownership or proof that storage crosses heaps._"
    )
    print()
    print("| Target | Function | Operation | Retail | Rebuild |")
    print("| --- | --- | --- | --- | --- |")
    for target, allocators in allocator_rows:
        for entry in allocators["new"][:20]:
            print(
                f"| `{target}` | `{entry['orig']}` {entry['name']} "
                f"| {entry['operation']} | {', '.join(entry['retail'])} "
                f"| {', '.join(entry['rebuild'])} |"
            )

candidate_rows = [
    (target, comparison["header_candidates"])
    for target, comparison in comparison_rows
    if comparison.get("header_candidates") and comparison["header_candidates"]["groups"]
]
if candidate_rows:
    print()
    print("#### Regressions in files directly including changed headers")
    print()
    print(
        "_Regressions grouped by the set of changed headers their marker file includes "
        "directly. These are review candidates, not causes._"
    )
    print()
    print("| Target | Changed headers | Newly different | Examples |")
    print("| --- | --- | ---: | --- |")
    for target, candidates in candidate_rows:
        for group in candidates["groups"][:10]:
            headers = ", ".join(f"`{header}`" for header in group["headers"])
            examples = ", ".join(f"`{item['name']}`" for item in group["representatives"][:3])
            print(f"| `{target}` | {headers} | {group['newly_different']:,} | {examples} |")
        if candidates["without_direct_changed_header"]:
            print(
                f"| `{target}` | _no direct changed include_ "
                f"| {candidates['without_direct_changed_header']:,} | |"
            )

data_rows = [(target, row["datacmp"]) for target, row in rows if row.get("datacmp")]
if data_rows:
    print()
    print("#### datacmp")
    print()
    print(
        "| Target | Data objects | Matched | Match rate | Issues | Field differences | Raw-only issues |"
    )
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")
    for target, data in data_rows:
        head = data["head"]
        delta = data["delta"]
        print(
            f"| `{target}` "
            f"| {count_with_delta(head['count'], delta.get('count'))} "
            f"| {count_with_delta(head['matched'], delta.get('matched'))} "
            f"| {ratio_with_delta(head.get('match_rate'), delta.get('match_rate'))} "
            f"| {count_with_delta(head['issue_count'], delta.get('issue_count'))} "
            f"| {count_with_delta(head['field_differences'], delta.get('field_differences'))} "
            f"| {count_with_delta(head['raw_only_issues'], delta.get('raw_only_issues'))} |"
        )

print()
print(f"_Updated from CI merge result `{os.environ.get('GITHUB_SHA', '')[:12]}`._")
