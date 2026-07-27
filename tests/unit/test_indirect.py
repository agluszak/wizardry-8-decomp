"""Finite target sets for virtual calls and the screen dispatcher."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.indirect import (
    resolve_handler_table,
    resolve_virtual_call,
    slot_override_sets,
)

REPOSITORY = Path(__file__).resolve().parents[2]
CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


def test_the_screen_dispatcher_becomes_reachable_targets() -> None:
    table = resolve_handler_table(REPOSITORY)

    # 62 states; the handlers are the application's real navigation graph,
    # invisible to a direct-edge call census.
    assert table["table_slots"] == 62
    assert table["distinct_handlers"] == 44
    assert all(slots for slots in table["handler_targets"].values())


def test_a_folded_stub_is_many_handlers_not_one_shared_handler() -> None:
    # The linker merged seventeen trivial `mov al,1; ret` handlers into one
    # address. Reporting that as a single target would invent a relationship
    # the binary does not have.
    table = resolve_handler_table(REPOSITORY)

    assert list(table["folded_stubs"]) == ["005b1740"]
    assert len(table["folded_stubs"]["005b1740"]) == 17
    assert "005b1740" not in table["handler_targets"]


def test_a_receiver_narrows_a_slot_from_the_whole_image_to_a_pair() -> None:
    unnarrowed = resolve_virtual_call(0, [], REPOSITORY, CANONICAL)
    narrowed = resolve_virtual_call(0, ["005ece78", "005ed5bc"], REPOSITORY, CANONICAL)

    # Slot 0 without a receiver is every destructor in the image; with the two
    # candidate tables it is exactly their two.
    assert len(unnarrowed["targets"]) > 300
    assert unnarrowed["narrowed_by_receiver"] is False
    assert narrowed["targets"] == ["004a5f00", "004f3d90"]
    assert narrowed["narrowed_by_receiver"] is True


def test_slot_override_sets_span_the_censused_tables() -> None:
    sets = slot_override_sets(REPOSITORY, CANONICAL)

    assert sets[0]["tables"] > 300
    # Deeper slots exist but on far fewer tables, which is what makes a
    # deep-slot virtual call cheap to resolve.
    assert sets[max(sets)]["tables"] < sets[0]["tables"]
