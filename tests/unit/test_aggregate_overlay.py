"""Planning the aggregate placement: what gets a structure and how wide."""

from __future__ import annotations

from wiz8decomp.ghidra.aggregate_overlay import (
    address_in_name,
    component_widths,
    plan_placements,
    plan_types,
)

_ROWS = [
    {
        "aggregate": "gXStatus",
        "member": "fCombatMode",
        "kind": "global",
        "storage": "DAT_00683f94",
        "agreed": "True",
    },
    {
        "aggregate": "gXStatus",
        "member": "plsItemList",
        "kind": "global",
        "storage": "DAT_00683fb5",
        "agreed": "True",
    },
    {
        "aggregate": "gXStatus",
        "member": "uiMonstersInDatabase",
        "kind": "global",
        "storage": "g_monster_record_count",
        "agreed": "True",
    },
    {
        "aggregate": "gLonely",
        "member": "only",
        "kind": "global",
        "storage": "DAT_00700000",
        "agreed": "True",
    },
    {
        "aggregate": "gXStatus",
        "member": "disputed",
        "kind": "global",
        "storage": "DAT_00683fff",
        "agreed": "False",
    },
    {
        "aggregate": "gpCombat",
        "member": "iActionChar",
        "kind": "offset",
        "storage": "0x7b4",
        "agreed": "True",
    },
    {
        "aggregate": "gpCombat",
        "member": "eCombatActionStatus",
        "kind": "offset",
        "storage": "0x7b0",
        "agreed": "True",
    },
]


def test_only_agreed_members_of_a_real_block_are_placed() -> None:
    placements = plan_placements(_ROWS)

    assert set(placements) == {"gXStatus"}
    assert ("DAT_00683fff", "disputed") not in placements["gXStatus"]
    # A single named address is already a named address; wrapping it in a
    # one-field structure would assert a block nothing has shown.
    assert "gLonely" not in placements


def test_a_reviewed_name_keeps_its_name_because_only_the_program_places_it() -> None:
    placements = plan_placements(_ROWS)

    assert ("g_monster_record_count", "uiMonstersInDatabase") in placements["gXStatus"]
    assert address_in_name("g_monster_record_count") is None
    assert address_in_name("DAT_00683f94") == 0x683F94


def test_pointer_reached_members_become_offsets_not_placements() -> None:
    types = plan_types(_ROWS)

    assert types["gpCombat"] == [(0x7B0, "eCombatActionStatus"), (0x7B4, "iActionChar")]
    assert "gpCombat" not in plan_placements(_ROWS)


def test_a_measured_width_is_capped_by_the_next_member() -> None:
    members = [(0x683F94, "fCombatMode"), (0x683F96, "fItemSelectMode")]

    # Ghidra records a four-byte access at the first address, but the second
    # member starts two bytes later: the access is wider than the field.
    sized = component_widths(members, {0x683F94: 4, 0x683F96: 2})

    assert sized == [(0x683F94, "fCombatMode", 2), (0x683F96, "fItemSelectMode", 2)]


def test_an_unmeasured_member_stays_one_byte_rather_than_swallowing_its_neighbour() -> None:
    members = [(0x10, "first"), (0x20, "second")]

    sized = component_widths(members, {})

    assert sized == [(0x10, "first", 1), (0x20, "second", 1)]
