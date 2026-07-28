"""The vtable-artifact classifier, validated on the cases that motivated it."""

from __future__ import annotations

from pathlib import Path

from wiz8decomp.object_model import classify_program_vtables

REPOSITORY = Path(__file__).resolve().parents[2]
CANONICAL = "wiz8--gog-base--wiz8--18a74ff61c65"


def _verdicts() -> dict[str, dict]:
    return classify_program_vtables(REPOSITORY, CANONICAL)


def test_the_timer_table_is_an_import_copy_not_a_class() -> None:
    # The correction that cost a wrong candidate, a ported phantom subclass
    # and a reviewed note: every ordinary slot is an srTimer import thunk and
    # slot 0 is the locally generated deleting destructor.
    verdict = _verdicts()["005ec078"]

    assert verdict["kind"] == "local-import-vtable-copy"
    assert verdict["imported_class"] == "srTimer"
    assert verdict["local_deleting_destructor"] == "004397d0"


def test_a_subclass_inheriting_import_slots_is_not_an_import_copy() -> None:
    # stMaterial inherits srMaterial's exported slots by thunk but overrides
    # five locally; one wrong classification here recreates the phantom.
    verdict = _verdicts()["005ecb6c"]

    assert verdict["kind"] == "derived-from-import"
    assert verdict["imported_class"] == "srMaterial"
    assert verdict["local_slots"] == 5


def test_purely_local_tables_stay_first_party() -> None:
    verdict = _verdicts()["005ed5bc"]

    assert verdict["kind"] == "first-party"
    assert verdict["slots"] == 18


def test_import_copies_are_rare_and_every_one_names_its_class() -> None:
    verdicts = _verdicts()
    copies = {
        table: verdict
        for table, verdict in verdicts.items()
        if verdict["kind"] == "local-import-vtable-copy"
    }

    # A local copy exists only where a dllimport class is instantiated; a
    # sudden flood would mean the rule broadened into misclassification.
    assert "005ec078" in copies
    assert len(copies) < 25
    assert all(verdict["imported_class"] for verdict in copies.values())


def test_containment_corrections_are_reported_not_silently_applied() -> None:
    from wiz8decomp.object_model import attribute_writers

    writes = [
        {
            "site": "004395d4",
            "function_start": "004393e0",
            "vtable": "005ec078",
            "object_offset": "0x0",
        },
        {
            "site": "00439863",
            "function_start": "004397d0",
            "vtable": "005ec078",
            "object_offset": "0x0",
        },
    ]
    # Ghidra containment: the first site really sits in 0x00439550 - the
    # timer-wrapper constructor the census misattributed to the hash-grow.
    containment = {"0x004395d4": "439550", "0x00439863": "004397d0"}

    corrected, corrections = attribute_writers(writes, containment)

    assert corrected[0]["function_start"] == "00439550"
    assert corrections == [{"site": "004395d4", "census": "004393e0", "containment": "00439550"}]
    assert corrected[1]["function_start"] == "004397d0"


def test_the_widget_table_unifies_with_its_reviewed_identity() -> None:
    # The anti-duplicate rule: vtable 0x005ED5BC's writer is the reviewed
    # W8WidgetBase005ED5BC's own deleting destructor, so no new owner type may
    # be invented for it.
    from wiz8decomp.object_model import lifecycle_unifications, load_reviewed_lifecycles

    writes = [
        {
            "site": "004f3d99",
            "function_start": "004f3d90",
            "vtable": "005ed5bc",
            "object_offset": "0x0",
        },
    ]
    proposals = lifecycle_unifications(writes, load_reviewed_lifecycles(REPOSITORY))

    widget = [p for p in proposals if p["vtable"] == "005ed5bc"]
    assert widget and widget[0]["unifies_with"] == "W8WidgetBase005ED5BC"
    assert widget[0]["shared_lifecycle"] == ["004f3d90"]


def test_a_shared_base_destructor_builds_one_scored_family() -> None:
    from wiz8decomp.object_model import destructor_family

    calls = [
        {"caller": "005a7620", "callee": "004f3480"},
        {"caller": "005a76d0", "callee": "004f3480"},
        {"caller": "005be950", "callee": "004f3480"},
    ]
    writes = [
        {"site": "1", "function_start": "005a7620", "vtable": "005eed8c", "object_offset": "0x0"},
        {"site": "2", "function_start": "005a76d0", "vtable": "005eeddc", "object_offset": "0x0"},
        {"site": "3", "function_start": "005be950", "vtable": "005ef364", "object_offset": "0x0"},
        # A non-family writer must not join.
        {"site": "4", "function_start": "004397d0", "vtable": "005ec078", "object_offset": "0x0"},
    ]

    family = destructor_family("004f3480", writes, calls)

    assert set(family["member_tables"]) == {"005eed8c", "005eeddc", "005ef364"}
    assert family["caller_count"] == 3
    assert family["fan_out_score"] == 33
