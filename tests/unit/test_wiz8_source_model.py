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


def test_assertion_harvest_yields_identifiers_and_extends_the_tree() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/assertions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))
    with (repository / "config/analysis/wiz8/source-tree.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        tree = {row["relative_path"] for row in csv.DictReader(stream)}

    assert len(rows) == 1038
    assert len({row["source_path"] for row in rows}) == 117
    assert all(row["expression"] and row["line"] for row in rows)

    # Members are named through -> or . and carry the original's m_ prefix.
    members = [row for row in rows if "->" in row["expression"]]
    assert len(members) >= 150
    assert any("m_pacRecipients" in row["expression"] for row in rows)
    assert any("pWorld->plsProps" in row["expression"] for row in rows)

    # The .cpp paths cross-validate the tracked census; the .hpp paths extend it.
    absolute = {row["source_path"] for row in rows if not row["source_path"].startswith("..")}
    assert {path.split("Wizardry 8\\", 1)[-1] for path in absolute} <= tree
    headers = sorted({row["source_path"] for row in rows if row["source_path"].startswith("..")})
    assert headers == [
        "..\\Engine Code\\Include\\AnimRep.hpp",
        "..\\Engine Code\\Include\\Trigger.hpp",
        "..\\Engine Code\\Include\\stHeap.hpp",
        "..\\Engine Code\\Include\\stLight.hpp",
    ]


def test_reviewed_wiz8_classes_have_source_and_vtable_evidence() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/classes.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        classes = list(csv.DictReader(stream))

    by_name = {row["class_name"]: row for row in classes}
    assert set(by_name) == {
        "GrCycle",
        "Monster",
        "MonsterInfoDialog",
        "VirtualFileBinIStream",
        "MonsterLight",
        "Octree",
    }
    # Classes recovered from source paths carry one; classes recovered from an
    # imported SurRender base carry a named base instead.
    for name in ("GrCycle", "Monster", "MonsterInfoDialog"):
        assert by_name[name]["source_path"]
        assert by_name[name]["base_classes"] == ""
    assert all("layout_proof" in row for row in classes)
    # Octree is the first class whose layout is byte-proven rather than inferred.
    assert by_name["Octree"]["layout_proof"].startswith("0042e440")
    assert by_name["Monster"]["layout_proof"].startswith("004bfab0")
    assert not any(by_name[n]["layout_proof"] for n in ("GrCycle", "MonsterInfoDialog"))
    for name in ("VirtualFileBinIStream", "MonsterLight"):
        assert by_name[name]["source_path"] == ""
        assert by_name[name]["base_classes"]
        assert by_name[name]["base_name_origin"] == "original-export"
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
        "base_classes": "",
        "base_name_origin": "",
        "source_path": "Engine Code\\GrCycle.cpp",
        "layout_proof": "",
        "evidence": (
            "Primary slots 4 and 11 directly reference the exact source path; "
            "constructor and destructor install primary and secondary vtables"
        ),
    }
    monster = by_name["Monster"]
    assert monster["vtable"] == "005ed200"
    assert monster["slots"] == "31"
    assert monster["constructor"] == "004bea20"
    assert monster["destructor"] == "004bee50"
    assert monster["scalar_deleting_destructor"] == "004beba0"
    assert monster["minimum_size"] == "0x628"
    assert monster["source_path"] == "Engine Code\\Monster.cpp"
    assert "Slots 5 12 and 26 directly reference the exact source path" in monster["evidence"]
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
        "base_classes": "",
        "base_name_origin": "",
        "source_path": "Dialog Code\\MonsterInfoDialog.cpp",
        "layout_proof": "",
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
    # The bodies these seeds point at are original Wizardry code. Ownership moves off
    # fan-patch-oracle only when another source proves which codebase the body is from;
    # so far that is only 0x0040EFA0, which the SGP Random.c compile placed in sgp-shared.
    assert {row["owner"] for row in symbols} == {"fan-patch-oracle", "sgp-shared"}
    assert sum(row["owner"] == "sgp-shared" for row in symbols) == 1
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


def test_string_database_inventory_preserves_footer_index_boundaries() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/wiz8/string-databases.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 3
    by_path = {row["archive_path"]: row for row in rows}
    assert by_path["DATABASES\\ITEMDESC.DBS"]["record_count"] == "1000"
    assert by_path["DATABASES\\ITEMDESC.DBS"]["maximum_code_units"] == "474"
    assert by_path["DATABASES\\SPELLDESC.DBS"]["record_count"] == "160"
    assert by_path["DATABASES\\SPELLEFFECT.DBS"]["maximum_code_units"] == "0"
    assert {row["encoded"] for row in rows} == {"false"}
    assert {row["consumer"] for row in rows} == {"0x0052ff80"}


def test_initial_owned_wiz8_boundaries_are_exact() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "config/analysis/reccmp/wiz8-gameplay-boundaries.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 41
    exact = [row for row in rows if row["confidence"] == "exact"]
    assert len(exact) == 34
    assert {int(row["size"]) for row in exact} == {
        6,
        19,
        24,
        25,
        26,
        33,
        47,
        50,
        53,
        61,
        70,
        76,
        82,
        85,
        100,
        103,
        105,
        109,
        110,
        115,
        117,
        118,
        132,
        179,
        199,
        218,
        219,
        231,
    }
    assert all(len(row["relocation_masked_sha256"]) == 64 for row in exact)
    backfire = next(row for row in rows if row["symbol"] == "CanSpellBackfire")
    assert backfire["confidence"] == "structurally-strong"
    assert backfire["relocation_masked_sha256"] == ""
    monster_lookup = next(row for row in rows if row["symbol"] == "FindMonGenByName")
    assert monster_lookup["confidence"] == "structurally-strong"
    assert monster_lookup["relocation_masked_sha256"] == ""
    first_monster = next(row for row in rows if row["symbol"] == "FindFirstMonsterByID")
    assert first_monster["confidence"] == "structurally-strong"
    assert first_monster["relocation_masked_sha256"] == ""
    next_monster = next(row for row in rows if row["symbol"] == "FindNextExistingMonsterByID")
    assert next_monster["confidence"] == "structurally-strong"
    assert next_monster["relocation_masked_sha256"] == ""
    item_origin = next(row for row in rows if row["symbol"] == "GetOriginOfCharacterItem")
    assert item_origin["confidence"] == "structurally-strong"
    assert item_origin["relocation_masked_sha256"] == ""
    script_part = next(row for row in rows if row["symbol"] == "MonsterGetScriptPartByLocationIndex")
    assert script_part["confidence"] == "structurally-strong"
    assert script_part["relocation_masked_sha256"] == ""
    index_by_location = next(row for row in rows if row["symbol"] == "MonsterGetIndexByLocationID")
    assert index_by_location["confidence"] == "structurally-strong"
    assert index_by_location["relocation_masked_sha256"] == ""
    source = "\n".join(
        (repository / path).read_text(encoding="utf-8")
        for path in (
            "src/wiz8/character_items.c",
            "src/wiz8/gameplay_boundaries.c",
            "src/wiz8/location_variables.c",
            "src/wiz8/item_spawning.cpp",
            "src/wiz8/message_box.cpp",
            "src/wiz8/monster_cycles.cpp",
            "src/wiz8/monster_generators.cpp",
            "src/wiz8/monster_location.c",
            "src/wiz8/monster_lookup.c",
            "src/wiz8/npc_item_lists.c",
            "src/wiz8/random_number.c",
            "src/wiz8/spell_backfire.cpp",
            "src/wiz8/targeting.c",
            "src/wiz8/state_getters.c",
            "src/wiz8/vector_conversions.cpp",
            "src/wiz8/octree_loading.cpp",
            "src/wiz8/world_props.cpp",
        )
    )
    for row in rows:
        assert f"// FUNCTION: WIZ8 0x{row['address'].upper()}" in source
        assert row["symbol"] in source
