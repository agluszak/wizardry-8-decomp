"""Align the same function across builds, so one recovery serves all of them.

The demo, the retail release and the patched release are three compilations of
one source tree at different moments. A fact recovered in one - an assertion's
file and line, a class identity, a signature - is a fact about a function that
exists in the other two at a different address, and without a mapping between
them every build is investigated from scratch.

The mapping is built from what survives relinking rather than from bytes.
Assertions are the anchor: a release build keeps the expression text and the
source path of every one it did not compile out, and an expression like
`lCollisions < 100` inside `Engine Code/GameData.cpp` names its function far
more precisely than any address does. Line numbers deliberately do *not* enter
the fingerprint - the same assertion sits at 591 in the demo and 595 in retail,
and that drift is a result worth reporting, not noise to match through.

Anchors alone reach only functions that assert. The call graph carries the rest:
once two functions are aligned, a callee that is the only unaligned callee on
both sides is the same callee, and repeating that to a fixpoint spreads the
alignment outward from every anchor. The rule refuses to choose whenever more
than one candidate fits, so the frontier stops at genuine ambiguity instead of
guessing through it.

Nothing here is an identity claim on its own. An alignment says *these two
addresses are the same function in two builds*; what that function is remains a
matter for the reviewed model, and the mapping is written under `build/` as a
generated index rather than into the ledger.
"""

from __future__ import annotations

import csv
import json
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

# Both builds record absolute build-machine paths - `E:\Wizardry 8\...` in the
# demo, `C:\Projects\Wizardry 8\...` in retail - so the shared tail is the
# project-relative path below the project directory.
PROJECT_MARKER = "wizardry 8/"

ANCHOR = "assertion fingerprint"


@dataclass
class Alignment:
    """A one-to-one mapping between two programs' functions, with reasons."""

    left: str
    right: str
    pairs: dict[str, str] = field(default_factory=dict)
    reasons: dict[str, str] = field(default_factory=dict)
    ambiguous: list[dict[str, Any]] = field(default_factory=list)

    def add(self, left: str, right: str, reason: str) -> bool:
        if left in self.pairs or right in set(self.pairs.values()):
            return False
        self.pairs[left] = right
        self.reasons[left] = reason
        return True

    def summary(self) -> dict[str, Any]:
        counts: dict[str, int] = defaultdict(int)
        for reason in self.reasons.values():
            counts[reason.split(" of ")[0]] += 1
        return {
            "left": self.left,
            "right": self.right,
            "aligned": len(self.pairs),
            "by_reason": dict(sorted(counts.items())),
            "ambiguous": len(self.ambiguous),
        }


def normalise_source_path(path: str) -> str:
    """The project-relative tail of a build-machine source path."""

    unified = path.replace("\\", "/").casefold()
    marker = unified.find(PROJECT_MARKER)
    if marker >= 0:
        return unified[marker + len(PROJECT_MARKER) :]
    return unified.rsplit("/", 1)[-1]


def _assertion_text(row: dict[str, str]) -> str:
    """What this site asserts: its expression, or its message when it has none.

    A handful of sites record neither, and a fingerprint element that is only
    a file name would tie together every silent site in that file.
    """

    return row["expression"].strip() or row["message"].strip()


def _assertion_rows(repo: Path) -> list[dict[str, str]]:
    path = repo / "evidence" / "snapshots" / "call-sites" / "assertions.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        return list(csv.DictReader(stream))


def assertion_fingerprints(repo: Path) -> dict[str, dict[str, frozenset[tuple[str, str]]]]:
    """Per program, each function's set of `(source file, expression)` pairs.

    The line number is left out on purpose: it is what moves between builds,
    and including it would make every edited file's functions unmatchable.
    """

    collected: dict[str, dict[str, set[tuple[str, str]]]] = defaultdict(lambda: defaultdict(set))
    for row in _assertion_rows(repo):
        function = row["function_start"].strip().lower()
        text = _assertion_text(row)
        if not function or not text:
            continue
        collected[row["program"]][function].add((normalise_source_path(row["source_path"]), text))
    return {
        program: {function: frozenset(pairs) for function, pairs in functions.items()}
        for program, functions in collected.items()
    }


def line_drift(repo: Path, left: str, right: str) -> list[dict[str, Any]]:
    """Where an assertion's line moved between two builds, by source file.

    An unchanged file keeps every assertion on its line; a file whose lines all
    shifted by the same amount had text inserted above them; a file with mixed
    drift was edited in several places. This is the cheapest available evidence
    of which translation units changed between two releases.
    """

    positions: dict[str, dict[tuple[str, str], int]] = {left: {}, right: {}}
    for row in _assertion_rows(repo):
        if row["program"] not in positions:
            continue
        text = _assertion_text(row)
        if not text or not row["line"].strip():
            continue
        positions[row["program"]][(normalise_source_path(row["source_path"]), text)] = int(
            row["line"]
        )

    drift: dict[str, set[int]] = defaultdict(set)
    for key, line in positions[left].items():
        other = positions[right].get(key)
        if other is not None:
            drift[key[0]].add(other - line)
    return [
        {
            "source_path": path,
            "shifts": sorted(shifts),
            "unchanged": shifts == {0},
        }
        for path, shifts in sorted(drift.items())
    ]


def _call_graph(repo: Path, programs: set[str]) -> dict[str, dict[str, dict[str, set[str]]]]:
    """`program -> {"callees": {caller: {callee}}, "callers": {callee: {caller}}}`."""

    graphs: dict[str, dict[str, dict[str, set[str]]]] = {
        program: {"callees": defaultdict(set), "callers": defaultdict(set)} for program in programs
    }
    path = repo / "evidence" / "snapshots" / "functions" / "calls.csv"
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] not in graphs:
                continue
            caller = row["caller"].strip().lower()
            callee = row["callee"].strip().lower()
            graphs[row["program"]]["callees"][caller].add(callee)
            graphs[row["program"]]["callers"][callee].add(caller)
    return graphs


def _anchor(alignment: Alignment, fingerprints: dict[str, dict[str, frozenset]]) -> None:
    """Align functions whose assertion fingerprints are unique and identical."""

    def by_fingerprint(program: str) -> dict[frozenset, list[str]]:
        grouped: dict[frozenset, list[str]] = defaultdict(list)
        for function, fingerprint in fingerprints.get(program, {}).items():
            if fingerprint:
                grouped[fingerprint].append(function)
        return grouped

    left = by_fingerprint(alignment.left)
    right = by_fingerprint(alignment.right)
    for fingerprint, functions in sorted(left.items(), key=lambda item: sorted(item[1])):
        others = right.get(fingerprint, [])
        if len(functions) == 1 and len(others) == 1:
            alignment.add(functions[0], others[0], ANCHOR)
        elif functions and others:
            alignment.ambiguous.append(
                {
                    "reason": "several functions share one assertion fingerprint",
                    "left": sorted(functions),
                    "right": sorted(others),
                }
            )


def _propagate(alignment: Alignment, graphs: dict[str, dict[str, dict[str, set[str]]]]) -> None:
    """Spread each anchor outward while exactly one candidate fits."""

    left_graph = graphs[alignment.left]
    right_graph = graphs[alignment.right]
    changed = True
    while changed:
        changed = False
        matched_right = set(alignment.pairs.values())
        for direction in ("callees", "callers"):
            for source, target in list(alignment.pairs.items()):
                open_left = [
                    item
                    for item in sorted(left_graph[direction].get(source, ()))
                    if item not in alignment.pairs
                ]
                open_right = [
                    item
                    for item in sorted(right_graph[direction].get(target, ()))
                    if item not in matched_right
                ]
                if len(open_left) == 1 and len(open_right) == 1:
                    reason = f"sole unaligned {direction[:-1]} of {source}"
                    if alignment.add(open_left[0], open_right[0], reason):
                        matched_right.add(open_right[0])
                        changed = True


def verify(
    alignment: Alignment, graphs: dict[str, dict[str, dict[str, set[str]]]]
) -> dict[str, Any]:
    """Check the alignment against edges it did not use to build itself.

    Every call between two aligned functions is a prediction: if the mapping is
    right, the same edge exists on the other side. The rate is the alignment's
    own quality measure, and it is reported rather than asserted - a build
    genuinely does inline a call the other keeps, so the honest expectation is
    a high rate, never a perfect one.
    """

    left_graph = graphs[alignment.left]["callees"]
    right_graph = graphs[alignment.right]["callees"]
    agreed = 0
    disagreed: list[dict[str, str]] = []
    for source, target in sorted(alignment.pairs.items()):
        for callee in sorted(left_graph.get(source, ())):
            mapped = alignment.pairs.get(callee)
            if mapped is None:
                continue
            if mapped in right_graph.get(target, ()):
                agreed += 1
            else:
                disagreed.append(
                    {"left": source, "right": target, "left_callee": callee, "right_callee": mapped}
                )
    total = agreed + len(disagreed)
    return {
        "predicted_edges": total,
        "agreed": agreed,
        "disagreed": len(disagreed),
        "agreement": round(agreed / total, 4) if total else None,
        "disagreements": disagreed,
    }


def align(repo: Path, left: str, right: str) -> Alignment:
    """Align two programs: anchor on assertions, then spread along the calls."""

    fingerprints = assertion_fingerprints(repo)
    for program in (left, right):
        if program not in fingerprints:
            raise ValueError(f"no assertion observations for program {program}")
    alignment = Alignment(left=left, right=right)
    _anchor(alignment, fingerprints)
    _propagate(alignment, _call_graph(repo, {left, right}))
    return alignment


def write_report(repo: Path, alignment: Alignment, destination: Path) -> dict[str, Any]:
    """Emit the alignment and the drift it measured as a generated index."""

    destination.mkdir(parents=True, exist_ok=True)
    with (destination / "alignment.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(
            stream, fieldnames=["left_program", "left", "right_program", "right", "reason"]
        )
        writer.writeheader()
        for source, target in sorted(alignment.pairs.items()):
            writer.writerow(
                {
                    "left_program": alignment.left,
                    "left": source,
                    "right_program": alignment.right,
                    "right": target,
                    "reason": alignment.reasons[source],
                }
            )

    drift = line_drift(repo, alignment.left, alignment.right)
    with (destination / "source-drift.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=["source_path", "shifts", "unchanged"])
        writer.writeheader()
        for row in drift:
            writer.writerow({**row, "shifts": " ".join(str(shift) for shift in row["shifts"])})

    verification = verify(alignment, _call_graph(repo, {alignment.left, alignment.right}))
    # A predicted edge that failed is where the alignment is most likely wrong,
    # so each one is written out; one callee failing under many callers is a
    # single bad pair rather than many, and that only shows in the whole list.
    disagreements = verification.pop("disagreements")
    with (destination / "disagreements.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=["left", "right", "left_callee", "right_callee"])
        writer.writeheader()
        writer.writerows(disagreements)

    summary = alignment.summary()
    summary["verification"] = verification
    summary["source_files_compared"] = len(drift)
    summary["source_files_unchanged"] = len([row for row in drift if row["unchanged"]])
    (destination / "summary.json").write_text(
        json.dumps(summary, indent=2) + "\n", encoding="utf-8"
    )
    return summary
