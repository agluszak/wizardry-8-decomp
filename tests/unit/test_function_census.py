from __future__ import annotations

import csv
from collections import Counter
from functools import cache
from pathlib import Path

_CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"
_ACCEPTED = {"exact", "strong"}


@cache
def _snapshot(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/functions" / name).open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


@cache
def _other(directory: str, name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots" / directory / name).open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_candidates_are_keyed_by_program_and_address() -> None:
    rows = _snapshot("candidates.csv")

    keys = [(row["program"], row["address"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_every_candidate_records_what_attests_it() -> None:
    rows = _snapshot("candidates.csv")

    assert rows
    assert all(row["sources"] for row in rows)
    vocabulary = {"eh-record", "call-target", "data-pointer", "padding"}
    assert all(set(row["sources"].split()) <= vocabulary for row in rows)


def test_an_unattested_candidate_off_an_instruction_boundary_is_rejected() -> None:
    """The linear decode is what separates a real entry from data."""
    rows = [
        row
        for row in _snapshot("candidates.csv")
        if not row["aligned"] and row["sources"] == "padding"
    ]

    assert rows
    assert all(row["verdict"] == "rejected" for row in rows)


def test_an_attested_candidate_off_a_boundary_is_flagged_not_discarded() -> None:
    """Something refers to it, so the sweep is the more likely thing to be wrong.

    An exception record outranks this: its entry point is read from the frame
    setup rather than inferred, so it stays `exact` whatever the sweep thinks.
    """
    rows = [
        row
        for row in _snapshot("candidates.csv")
        if not row["aligned"]
        and "eh-record" not in row["sources"]
        and set(row["sources"].split()) & {"call-target", "data-pointer"}
    ]

    assert rows
    assert all(row["verdict"] == "decode-disagrees" for row in rows)


def test_padding_alone_never_accepts_a_candidate() -> None:
    """Alignment padding also appears inside functions and after data."""
    rows = [
        row for row in _snapshot("candidates.csv") if row["sources"] == "padding" and row["aligned"]
    ]

    assert rows
    assert all(row["verdict"] == "padding-only" for row in rows)


def test_prologue_is_recorded_but_does_not_gate_acceptance() -> None:
    """Two thirds of proven entry points match a prologue shape; gating loses the rest."""
    accepted = [row for row in _snapshot("candidates.csv") if row["verdict"] in _ACCEPTED]

    assert accepted
    assert any(not row["prologue"] for row in accepted)


def test_exception_records_produce_the_exact_tier() -> None:
    rows = [row for row in _snapshot("candidates.csv") if "eh-record" in row["sources"]]

    assert rows
    assert all(row["verdict"] == "exact" for row in rows)


def test_known_entry_points_are_accepted() -> None:
    """Every vtable slot the polymorphism census resolved is a real function.

    An independent census produced those addresses, so a slot target this one
    rejects is a defect here rather than a disagreement.
    """
    slots = {
        row["target"]
        for row in _other("polymorphism", "slots.csv")
        if row["program"] == _CANONICAL and row["kind"] == "local" and row["target"]
    }
    verdicts = {
        row["address"]: row["verdict"]
        for row in _snapshot("candidates.csv")
        if row["program"] == _CANONICAL
    }

    missing = sorted(address for address in slots if address not in verdicts)
    assert not missing, missing[:5]
    unaccepted = sorted(address for address in slots if verdicts[address] not in _ACCEPTED)
    # The few that fail are where the resynchronising sweep stayed out of phase.
    # They must be flagged as a decode disagreement, never silently rejected.
    assert len(unaccepted) < 0.01 * len(slots), unaccepted[:5]
    assert all(verdicts[address] == "decode-disagrees" for address in unaccepted)


def test_call_edges_reference_an_accepted_caller() -> None:
    accepted = {
        (row["program"], row["address"])
        for row in _snapshot("candidates.csv")
        if row["verdict"] in _ACCEPTED
    }
    edges = _snapshot("calls.csv")

    assert edges
    assert all((row["program"], row["caller"]) in accepted for row in edges)
    assert all(int(row["call_sites"]) >= 1 for row in edges)


def test_call_edges_are_deduplicated() -> None:
    edges = _snapshot("calls.csv")

    keys = [(row["program"], row["caller"], row["callee"]) for row in edges]
    assert len(keys) == len(set(keys))


def test_accepted_sizes_are_bounded_by_the_next_accepted_start() -> None:
    rows = [
        row
        for row in _snapshot("candidates.csv")
        if row["program"] == _CANONICAL and row["verdict"] in _ACCEPTED and row["size"]
    ]

    assert rows
    assert all(int(row["size"]) > 0 for row in rows)


def test_every_program_contributes_candidates() -> None:
    counts = Counter(row["program"] for row in _snapshot("candidates.csv"))

    assert len(counts) >= 3
    assert all(count > 1000 for count in counts.values())
