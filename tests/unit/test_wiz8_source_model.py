import csv
from collections import Counter
from pathlib import Path


def test_wiz8_source_tree_preserves_raw_cpp_paths() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/source-tree.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 149
    assert Counter(row["subsystem"] for row in rows) == {
        "Engine Code": 50,
        "Local Code": 48,
        "Local Screens": 27,
        "Level Specific Code": 13,
        "Dialog Code": 9,
        "3D Code": 2,
    }
    assert sum(row["variants"] == "demo" for row in rows) == 13
    assert any(row["relative_path"] == "Dialog Code\\MonsterInfoDialog.cpp" for row in rows)
    assert not any(row["relative_path"] == "Dialog Code\\MonsterInfoDialog.c" for row in rows)


def test_reviewed_wiz8_classes_have_source_and_vtable_evidence() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/classes.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        classes = list(csv.DictReader(stream))

    by_name = {row["class_name"]: row for row in classes}
    assert set(by_name) == {"GrCycle", "Monster", "MonsterInfoDialog"}
    assert by_name["GrCycle"] == {
        "class_name": "GrCycle",
        "confidence": "strong",
        "vtable": "005ece78",
        "slots": "16",
        "constructor": "004a5e50",
        "destructor": "004a6610",
        "scalar_deleting_destructor": "004a5f00",
        "minimum_size": "0x1d8",
        "secondary_vtables": "005eceb8@0x18:13",
        "source_path": "Engine Code\\GrCycle.cpp",
        "evidence": (
            "Primary slots 4 and 11 directly reference the exact source path; "
            "constructor and destructor install primary and secondary vtables"
        ),
    }
    assert by_name["Monster"] == {
        "class_name": "Monster",
        "confidence": "strong",
        "vtable": "005ed200",
        "slots": "31",
        "constructor": "004bea20",
        "destructor": "004bee50",
        "scalar_deleting_destructor": "004beba0",
        "minimum_size": "0x628",
        "secondary_vtables": "",
        "source_path": "Engine Code\\Monster.cpp",
        "evidence": (
            "Slots 5 12 and 26 directly reference the exact source path; constructor "
            "writes this vtable after initializing fields through offset 0x624"
        ),
    }
    assert by_name["MonsterInfoDialog"] == {
        "class_name": "MonsterInfoDialog",
        "confidence": "strong",
        "vtable": "005ef910",
        "slots": "14",
        "constructor": "005d5e30",
        "destructor": "005d5f00",
        "scalar_deleting_destructor": "005d5ee0",
        "minimum_size": "0x130",
        "secondary_vtables": "",
        "source_path": "Dialog Code\\MonsterInfoDialog.cpp",
        "evidence": (
            "Slot 3 directly references the exact source path; constructor loads "
            "Data\\Dialogs\\popup_monsterinfo.sti and writes this vtable"
        ),
    }
