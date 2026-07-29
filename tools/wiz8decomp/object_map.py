"""Assign every function to an original object file, jointly rather than by interval.

The existing attribution takes each translation unit's assertion anchors and
calls the span between the outermost two that unit's interval. That is sound
where it speaks and silent everywhere else: a function in the gap between two
units' intervals gets no owner at all, and the gaps are where most of the image
lives. The gap is not an absence of evidence, though - it is a *boundary
placement problem*, and the call graph is evidence about where the boundary is.

The model here is one assignment over the `.text` order. A compiler emits a
translation unit's functions contiguously, so between an anchor owned by unit U
and the next anchor owned by unit V there is exactly one boundary, and the only
question is where in the gap it falls. The objective is that calls cluster
inside a unit: each placement costs every call a function exchanges with the
unit it is *not* being assigned to, counted across the whole image rather than
inside the gap, because a helper wedged between two units is typically called
from all over one of them and from nowhere in the other.

The cost is often flat across a stretch of the gap, and that stretch is the
answer rather than a problem to break. Functions before every optimal placement
belong to the left unit and functions after every one belong to the right, so
they are assigned; the functions the optimal placements disagree about are left
unassigned and the range is reported. Against the interval model this decides
1779 functions it left silent, and says of 1587 others that the evidence bounds
them without choosing.

Every assignment carries how it was reached: `anchor` for an assertion that
names its own file, `enclosed` between two anchors of the same unit, `cut` for a
placement every optimum agrees on. Nothing here is written to the ledger; the
map is a candidate to review, and the reconstructed build's object information
is an independent check on it rather than an input.
"""

from __future__ import annotations

import csv
from collections import defaultdict
from dataclasses import dataclass, field
from itertools import pairwise
from pathlib import Path
from typing import Any

from .cross_build import normalise_source_path

ANCHOR = "anchor"
ENCLOSED = "enclosed"
CUT = "cut"
AMBIGUOUS = "ambiguous"


@dataclass
class ObjectMap:
    """Candidate original-object ownership for one program's functions."""

    program: str
    owners: dict[str, str] = field(default_factory=dict)
    basis: dict[str, str] = field(default_factory=dict)
    boundaries: list[dict[str, Any]] = field(default_factory=list)
    unanchored_functions: int = 0

    def summary(self) -> dict[str, Any]:
        counts: dict[str, int] = defaultdict(int)
        for reason in self.basis.values():
            counts[reason] += 1
        return {
            "program": self.program,
            "assigned": len(self.owners),
            "units": len(set(self.owners.values())),
            "by_basis": dict(sorted(counts.items())),
            "boundaries": len(self.boundaries),
            "ambiguous_boundaries": len(
                [item for item in self.boundaries if item["basis"] == AMBIGUOUS]
            ),
            "outside_any_anchor_pair": self.unanchored_functions,
        }


def assertion_anchors(repo: Path, program: str) -> dict[int, str]:
    """Functions whose own assertions name exactly one source file.

    A function whose assertions name two files has had another unit's code
    inlined into it, and it anchors neither.
    """

    path = repo / "evidence" / "snapshots" / "call-sites" / "assertions.csv"
    units: dict[int, set[str]] = defaultdict(set)
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] != program or not row["function_start"].strip():
                continue
            source = row["source_path"].strip()
            if not source:
                continue
            units[int(row["function_start"], 16)].add(normalise_source_path(source))
    return {address: next(iter(owners)) for address, owners in units.items() if len(owners) == 1}


def call_graph(repo: Path, program: str) -> tuple[list[int], list[tuple[int, int]]]:
    """The program's censused functions in address order, and its call edges."""

    path = repo / "evidence" / "snapshots" / "functions" / "calls.csv"
    functions: set[int] = set()
    edges: list[tuple[int, int]] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            if row["program"] != program:
                continue
            caller = int(row["caller"], 16)
            callee = int(row["callee"], 16)
            functions.add(caller)
            functions.add(callee)
            edges.append((caller, callee))
    return sorted(functions), edges


def _boundary_costs(
    window: list[int],
    edges_by_function: dict[int, set[int]],
    owners: dict[int, str],
    left_unit: str,
    right_unit: str,
) -> list[int]:
    """The cost of each boundary placement inside one gap.

    Placement `p` puts the first `p` functions of the window on the left. A
    function pays for every call it exchanges with the unit it is *not*
    assigned to, counting edges anywhere in the image rather than only inside
    the gap - a helper wedged between two units is usually called from all over
    one of them and from nowhere in the other, and that is the whole signal.

    A function with no edges to either unit adds nothing to any placement,
    which is why plateaus exist and why they are reported instead of split.
    """

    pull_left: list[int] = []
    pull_right: list[int] = []
    for function in window:
        left = right = 0
        for other in edges_by_function.get(function, ()):
            owner = owners.get(other)
            if owner == left_unit:
                left += 1
            elif owner == right_unit:
                right += 1
        pull_left.append(left)
        pull_right.append(right)

    costs = []
    for placement in range(len(window) + 1):
        cost = sum(pull_right[:placement]) + sum(pull_left[placement:])
        costs.append(cost)
    return costs


def assign(repo: Path, program: str) -> ObjectMap:
    """One joint assignment of functions to units over the `.text` order."""

    anchors = assertion_anchors(repo, program)
    functions, edges = call_graph(repo, program)
    known = set(functions)
    for address in anchors:
        known.add(address)
    order = sorted(known)
    position = {function: index for index, function in enumerate(order)}

    edges_by_function: dict[int, set[int]] = defaultdict(set)
    for caller, callee in edges:
        edges_by_function[caller].add(callee)
        edges_by_function[callee].add(caller)

    result = ObjectMap(program=program)
    for address, unit in anchors.items():
        result.owners[f"{address:08x}"] = unit
        result.basis[f"{address:08x}"] = ANCHOR

    # Gaps between two anchors of one unit are settled before any boundary is
    # placed, because a boundary is decided by what its neighbours already own.
    anchored = sorted(anchors)
    owners: dict[int, str] = dict(anchors)
    for left, right in pairwise(anchored):
        if anchors[left] != anchors[right]:
            continue
        for function in order[position[left] + 1 : position[right]]:
            result.owners[f"{function:08x}"] = anchors[left]
            result.basis[f"{function:08x}"] = ENCLOSED
            owners[function] = anchors[left]

    for left, right in pairwise(anchored):
        between = order[position[left] + 1 : position[right]]
        if not between or anchors[left] == anchors[right]:
            continue

        window = [left, *between, right]
        costs = _boundary_costs(window, edges_by_function, owners, anchors[left], anchors[right])
        # The boundary lies inside the gap: the left anchor stays left and the
        # right anchor stays right.
        candidates = range(1, len(window))
        best = min(costs[placement] for placement in candidates)
        minima = [placement for placement in candidates if costs[placement] == best]
        lowest, highest = minima[0], minima[-1]
        basis = CUT if lowest == highest else AMBIGUOUS
        for offset, function in enumerate(window):
            if function in (left, right):
                continue
            if offset < lowest:
                owner = anchors[left]
            elif offset >= highest:
                owner = anchors[right]
            else:
                # Every optimal placement disagrees about this function, so the
                # evidence bounds the boundary without choosing inside it.
                continue
            result.owners[f"{function:08x}"] = owner
            result.basis[f"{function:08x}"] = CUT
        result.boundaries.append(
            {
                "left_unit": anchors[left],
                "right_unit": anchors[right],
                "gap": len(between),
                "cost": best,
                "basis": basis,
                "undetermined": max(0, highest - lowest),
                "first_right_function": f"{window[highest]:08x}",
            }
        )

    result.unanchored_functions = len(order) - len(result.owners)
    return result


def write_report(mapping: ObjectMap, destination: Path) -> dict[str, Any]:
    """Emit the candidate object map under `build/`."""

    destination.mkdir(parents=True, exist_ok=True)
    with (destination / "object-map.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=["address", "source_unit", "basis"])
        writer.writeheader()
        for address in sorted(mapping.owners):
            writer.writerow(
                {
                    "address": address,
                    "source_unit": mapping.owners[address],
                    "basis": mapping.basis[address],
                }
            )
    with (destination / "boundaries.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(
            stream,
            fieldnames=[
                "left_unit",
                "right_unit",
                "gap",
                "cost",
                "basis",
                "undetermined",
                "first_right_function",
            ],
        )
        writer.writeheader()
        writer.writerows(mapping.boundaries)
    return mapping.summary()
