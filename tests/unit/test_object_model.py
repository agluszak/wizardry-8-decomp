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
