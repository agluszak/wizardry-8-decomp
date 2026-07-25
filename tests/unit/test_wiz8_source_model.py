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


def test_reviewed_cross_build_map_is_separate_and_explicit() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/cross-build-map.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        mappings = list(csv.DictReader(stream))
    with (repository / "config/analysis/cross-build-rejections.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rejections = list(csv.DictReader(stream))
    with (repository / "config/analysis/cross-build-oracles.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        oracles = list(csv.DictReader(stream))

    assert len(mappings) == 42
    assert len({(row["symbol"], row["variant"]) for row in mappings}) == 42
    assert {row["variant"] for row in mappings} == {"demo", "gog-1261", "gog-128"}
    assert {row["automated_classification"] for row in mappings} == {
        "candidate",
        "exact",
        "structurally-strong",
    }
    assert {row["review_decision"] for row in mappings} == {"manually-confirmed"}
    assert {row["review_decision"] for row in rejections} == {"rejected"}
    retail = next(row for row in oracles if row["variant"] == "retail-2001-12-23")
    assert retail["status"] == "protected-unavailable"
    assert retail["program"] == ""


def test_fan_patch_oracle_separates_original_targets_from_injected_hooks() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/functions/wiz8-cfagent-oracle.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        symbols = list(csv.DictReader(stream))
    with (repository / "config/analysis/fan-patch-128-hooks.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        hooks = list(csv.DictReader(stream))

    assert len(symbols) == 47
    assert len({row["address"] for row in symbols}) == 47
    assert {row["owner"] for row in symbols} == {"fan-patch-oracle"}
    assert {row["confidence"] for row in symbols} == {"strong"}
    by_name = {row["provisional_name"]: row["address"] for row in symbols}
    assert by_name["StartCombat"] == "004e7090"
    assert by_name["GetFact"] == "00506280"
    assert by_name["SetFact"] == "005061a0"

    assert len(hooks) == 26
    assert {row["ownership"] for row in hooks} == {"fan-patch-injected"}
    assert {row["kind"] for row in hooks} == {"hook", "inline-fix"}


def test_cfdat_override_evidence_separates_callsite_and_canonical_sizes() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/cfdat-overrides.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 15
    assert {row["status"] for row in rows} == {"typed", "partially-typed", "conflicting"}
    by_name = {row["filename"]: row for row in rows}
    assert by_name["racesattrs.cfdat"]["english_destination"] == "0x00614cf0"
    assert by_name["racesattrs.cfdat"]["canonical_size"] == "0x134"
    assert by_name["classesattrs.cfdat"]["status"] == "typed"
    assert by_name["classesskills.cfdat"]["callsite_size_argument"] == "0x7f8"
    assert by_name["classesskills.cfdat"]["canonical_size"] == "0x99c"
    assert by_name["classesexpgroup.cfdat"]["english_destination"] == "0x004ef1e0"


def test_gameplay_database_record_boundaries_match_the_corpus() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/database-records.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 5
    for row in rows:
        file_size = int(row["file_size"])
        header_size = int(row["header_size"], 0)
        record_count = int(row["record_count"])
        disk_record_size = int(row["disk_record_size"], 0)
        assert file_size == header_size + record_count * disk_record_size

    by_path = {row["archive_path"]: row for row in rows}
    assert by_path["DATABASES\\MONSTERS.DBS"]["runtime_record_size"] == "0x297"
    assert by_path["DATABASES\\SPELLTABLES.DBS"]["runtime_record_size"] == "0x1bf"


def test_level_format_inventory_preserves_typed_waypoint_boundary() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/level-formats.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    by_extension = {row["extension"]: row for row in rows}
    assert by_extension[".WPT"]["file_count"] == "35"
    assert by_extension[".WPT"]["header_size"] == "0x10"
    assert by_extension[".WPT"]["record_layout"] == "waypoint 0x10 then link 0x0e"
    assert by_extension[".WPT"]["loader"] == "0x00459650"
    assert by_extension[".OCT"]["loader"] == "0x0042bc10"


def test_variable_database_inventory_preserves_npc_rule_tail() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/variable-databases.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 3
    by_path = {row["archive_path"]: row for row in rows}
    npc = by_path["DATABASES\\NPC.DBS"]
    assert npc["archive_path"] == "DATABASES\\NPC.DBS"
    assert npc["file_size"] == "118674"
    assert npc["record_count"] == "146"
    assert npc["fixed_record_size"] == "0x309"
    assert npc["tail_layout"] == "uint32 count then count times 0x06 fact rules"
    assert npc["loader"] == "0x0054aac0"

    encounters = by_path["DATABASES\\ENCOUNTERTABLES.DBS"]
    assert encounters["file_size"] == "114919"
    assert encounters["record_count"] == "72"
    assert encounters["fixed_record_size"] == "0x108"
    assert encounters["loader"] == "0x0048a7a0"

    item_tables = by_path["DATABASES\\ITEMTABLES.DBS"]
    assert item_tables["file_size"] == "60506"
    assert item_tables["record_count"] == "114"
    assert item_tables["fixed_record_size"] == "0x1f1"
    assert item_tables["loader"] == "0x0054a510"


def test_save_game_section_vocabulary_is_unique_and_bidirectional() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/save-game-sections.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 28
    assert len({row["tag"] for row in rows}) == 28
    assert len({int(row["value"], 0) for row in rows}) == 28
    by_tag = {row["tag"]: row for row in rows}
    assert by_tag["GVER"]["consumer"] == "0x005123f0"
    assert by_tag["GSTA"]["consumer"] == "0x00512290"
    assert by_tag["STAT"]["evidence"] == "Fixed 0x314-byte W8SaveStatusHeader"
    assert by_tag["NPCF"]["consumer"] == "0x00506480/0x005064a0"
    assert {row["direction"] for row in rows} == {"save", "load", "both"}
