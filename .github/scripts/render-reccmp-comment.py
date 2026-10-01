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
print("#### Function recovery")
print()
print("| Target | Recovered | Retail | Recovery | Paired | Unpaired |")
print("| --- | ---: | ---: | ---: | ---: | ---: |")
for target, row in rows:
    project = row["project"]
    head, delta = project["head"], project["delta"]
    print(
        f"| `{target}` "
        f"| {count_with_delta(head['source_functions'], delta.get('source_functions'))} "
        f"| {count_with_delta(head.get('original_functions'), delta.get('original_functions'))} "
        f"| {ratio_with_delta(head.get('source_coverage'), delta.get('source_coverage'))} "
        f"| {count_with_delta(head['paired'], delta.get('paired'))} "
        f"| {count_with_delta(head['unpaired'], delta.get('unpaired'))} |"
    )

comparison_rows = [(target, row["comparison"]) for target, row in rows if row.get("comparison")]
if comparison_rows:
    print()
    print("#### Function quality")
    print()
    print("Function quality covers the compared selection; catalog counts cover the full targets.")
    print()
    print(
        "| Target | Compared | Avg similarity | Median | Exact/clean | Clean rate | "
        "Different | Code diffs | Referenced-data diffs | Fixed | Regressed | Analysis failed |"
    )
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
    for target, comparison in comparison_rows:
        head = comparison["head"]
        delta = comparison["delta"]
        transitions = comparison["transitions"]
        print(
            f"| `{target}` "
            f"| {count_with_delta(head['analyzed'], delta.get('analyzed'))} "
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
        "Exact code and data-only differences contribute 100% code similarity._"
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

blast_rows = [
    (target, comparison["header_blast_radius"])
    for target, comparison in comparison_rows
    if comparison.get("header_blast_radius")
]
if blast_rows:
    print()
    print("#### Regressions by changed header")
    print()
    print("| Target | Header | Newly different | Examples |")
    print("| --- | --- | ---: | --- |")
    for target, groups in blast_rows:
        for group in groups[:10]:
            examples = ", ".join(f"`{item['name']}`" for item in group["representatives"][:3])
            print(
                f"| `{target}` | `{group['header']}` | {group['newly_different']:,} | {examples} |"
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
