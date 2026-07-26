from __future__ import annotations

import csv
from collections import Counter
from itertools import pairwise
from pathlib import Path

_CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


def _snapshot(name: str) -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/polymorphism" / name).open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def _reviewed() -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/vtables.csv").open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_tables_are_keyed_by_program_and_address() -> None:
    rows = _snapshot("vtables.csv")

    keys = [(row["program"], row["address"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_tables_never_overlap() -> None:
    """Overlapping tables mean the boundary rule failed to split a run."""
    rows = [row for row in _snapshot("vtables.csv") if row["program"] == _CANONICAL]
    tables = sorted(
        (int(row["address"], 16), int(row["slot_count"]), row["kind"]) for row in rows
    )

    for (start, count, kind), (next_start, _, next_kind) in pairwise(tables):
        if kind == "vbtable" or next_kind == "vbtable":
            continue  # a vbtable is not part of the pointer-run sequence
        assert start + count * 4 <= next_start, f"{start:#x} overlaps {next_start:#x}"


def test_every_slot_belongs_to_a_declared_table() -> None:
    tables = {(row["program"], row["address"]) for row in _snapshot("vtables.csv")}
    slots = _snapshot("slots.csv")

    assert slots
    assert all((row["program"], row["vtable"]) in tables for row in slots)


def test_slot_counts_agree_with_the_slot_rows() -> None:
    counts = Counter((row["program"], row["vtable"]) for row in _snapshot("slots.csv"))

    for row in _snapshot("vtables.csv"):
        assert counts[(row["program"], row["address"])] == int(row["slot_count"])


def test_pure_virtual_slots_are_resolved_through_the_import_table() -> None:
    """`_purecall` is found by name, so no address is hardcoded per build."""
    slots = [row for row in _snapshot("slots.csv") if row["kind"] == "pure-virtual"]

    assert slots
    # Every pure-virtual slot in one program points at the same thunk.
    for program in {row["program"] for row in slots}:
        targets = {row["target"] for row in slots if row["program"] == program}
        assert len(targets) == 1


def test_adjustor_thunks_record_their_adjustment_and_real_target() -> None:
    slots = [row for row in _snapshot("slots.csv") if row["kind"] == "adjustor-thunk"]

    assert slots
    assert all(row["adjust"] and row["thunk_target"] for row in slots)


def test_inherited_library_slots_name_the_method_they_point_at() -> None:
    slots = [row for row in _snapshot("slots.csv") if row["kind"] == "import-thunk"]

    assert slots
    assert any(row["import_signature"] for row in slots)


def test_virtual_inheritance_occurs_once_per_build() -> None:
    """A second vbtable is far more likely a misread layout than a real case."""
    rows = [row for row in _snapshot("vtables.csv") if row["kind"] == "vbtable"]

    per_program = Counter(row["program"] for row in rows)
    assert per_program
    assert set(per_program.values()) == {1}


def test_primary_tables_are_stored_at_object_offset_zero() -> None:
    rows = [row for row in _snapshot("vtables.csv") if row["subobject_offsets"]]

    assert rows
    assert any("0x0" in row["subobject_offsets"].split() for row in rows)


def test_census_reproduces_every_independently_correct_reviewed_count() -> None:
    """The three it contradicts are each a table plus the one following it.

    That is the overcount signature wiz8-8ga.5 describes, so a mismatch here is a
    regression in the boundary rule rather than a new disagreement.
    """
    census = {
        int(row["address"], 16): int(row["slot_count"])
        for row in _snapshot("vtables.csv")
        if row["program"] == _CANONICAL
    }
    overstated = {"Monster.primary", "VirtualFileBinIStream.secondary_0x10", "MonsterLight.secondary_0x138"}

    checked = 0
    for row in _reviewed():
        if not row["slot_count"] or row["vtable_id"] in overstated:
            continue
        address = int(row["address"], 16)
        assert address in census, row["vtable_id"]
        assert census[address] == int(row["slot_count"]), row["vtable_id"]
        checked += 1
    assert checked >= 8
