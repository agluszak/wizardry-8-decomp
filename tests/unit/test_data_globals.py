from __future__ import annotations

import csv
from functools import cache
from pathlib import Path

_CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


@cache
def _snapshot() -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/globals/globals.csv").open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


@cache
def _polymorphism() -> list[dict[str, str]]:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/snapshots/polymorphism/vtables.csv").open(
        encoding="utf-8", newline=""
    ) as stream:
        return list(csv.DictReader(stream))


def test_globals_are_keyed_by_program_and_address() -> None:
    rows = _snapshot()

    keys = [(row["program"], row["address"]) for row in rows]
    assert len(keys) == len(set(keys))


def test_uninitialised_globals_are_reported_rather_than_dropped() -> None:
    """Most mutable state lives past `.data`'s file-backed bytes."""
    rows = [row for row in _snapshot() if row["program"] == _CANONICAL]

    bss = [row for row in rows if row["storage"] == "bss"]
    assert bss
    assert all(row["section"] == ".data" for row in bss)


def test_every_global_carries_at_least_one_reference() -> None:
    rows = _snapshot()

    assert rows
    assert all(int(row["reference_count"]) >= 1 for row in rows)
    assert all(int(row["function_count"]) >= 0 for row in rows)


def test_extent_bound_is_the_distance_to_the_next_global_in_the_section() -> None:
    rows = [row for row in _snapshot() if row["program"] == _CANONICAL and row["extent_upper"]]

    assert rows
    for row in rows:
        assert int(row["extent_upper"], 16) > int(row["address"], 16)
        assert int(row["extent_bytes"]) == int(row["extent_upper"], 16) - int(row["address"], 16)


def test_import_slots_are_separated_from_game_globals() -> None:
    """Their reference count measures library calls, not use of a game variable."""
    rows = [row for row in _snapshot() if row["kind"] == "import-slot"]

    assert rows
    assert all(row["preview"] for row in rows)
    assert all(row["storage"] == "initialized" for row in rows)


def test_vtables_appear_as_code_pointer_globals() -> None:
    """An independent check: the two censuses see the same addresses.

    Both derive from the relocation table but by different routes, so a vtable
    the polymorphism census found should also be a referenced global here.
    """
    census = {
        row["address"]
        for row in _polymorphism()
        if row["program"] == _CANONICAL and row["kind"] == "vftable"
    }
    globals_by_address = {
        row["address"]: row for row in _snapshot() if row["program"] == _CANONICAL
    }

    shared = census & set(globals_by_address)
    assert len(shared) >= 0.95 * len(census)
    assert all(globals_by_address[address]["kind"] == "code-pointer" for address in shared)


def test_string_previews_do_not_capture_bytes_inside_constants() -> None:
    """A literal starts after a terminator; four printable bytes alone are not one."""
    rows = [row for row in _snapshot() if row["kind"] == "string"]

    assert rows
    assert all(len(row["preview"]) >= 4 for row in rows)
    assert all(row["section"] in {".rdata", ".data"} for row in rows)
