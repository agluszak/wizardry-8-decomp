import collections
import csv
from collections import Counter
from pathlib import Path

from wiz8decomp.ghidra.reviewed_class_model import load_reviewed_class_model
from wiz8decomp.provenance import validate_provenance


def test_wiz8_source_tree_preserves_raw_cpp_paths() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/observations/wiz8/source-tree.csv").open(
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
    with (repository / "evidence/observations/wiz8/assertions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))
    with (repository / "evidence/observations/wiz8/source-tree.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        tree = {row["relative_path"] for row in csv.DictReader(stream)}

    assert len(rows) == 1038
    assert len({row["source_path"] for row in rows}) == 117
    assert all(row["expression"] and row["line"] for row in rows)

    # Members are named through -> or . -- but m_ is a per-class habit, not a
    # project-wide convention: most member accesses carry no m_ at all.
    members = [row for row in rows if "->" in row["expression"]]
    assert len(members) >= 150
    without_m = [row for row in members if "m_" not in row["expression"]]
    assert len(without_m) == 147
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


def test_ptr_vector_instantiations_are_inventoried() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/observations/wiz8/ptr-vector-instantiations.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    # One hand-rolled growable pointer array, instantiated once per element type.
    assert len(rows) == 75
    assert len({row["vtable"] for row in rows}) == 75

    # The virtual count is not uniform, so the inventory records a determination
    # per vtable instead of asserting that every instantiation has exactly one.
    verdicts = collections.Counter(row["single_virtual"] for row in rows)
    assert verdicts == {"yes": 35, "no": 31, "adjacent-vtable": 9}

    # The one type replaced two separately-named structs.
    header = (repository / "src/wiz8/gameplay_boundaries.h").read_text(encoding="utf-8")
    assert "W8PtrVector" in header
    assert "W8NPCItemListVector" not in header
    assert "W8MonsterGeneratorVector" not in header
    # The leading word is a vptr, not padding.
    assert "void* vptr;" in header


def test_allocator_layers_preserve_identity_provenance_and_ownership_signal() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/allocator-layers.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    by_address = {row["address"]: row for row in rows}
    assert len(rows) == len(by_address) == 8
    assert set(by_address) == {
        "005e1c10",
        "005e1ce0",
        "005eb1a0",
        "005eb1c4",
        "005eb1f8",
        "005eb224",
        "005ebab8",
        "005ebac0",
    }
    for row in rows:
        validate_provenance(row["name_origin"], row["authority"])

    assert by_address["005e1c10"]["address_kind"] == "function"
    assert by_address["005e1c10"]["authority"] == "descriptive"
    assert by_address["005e1ce0"]["address_kind"] == "import-thunk"
    assert by_address["005e1ce0"]["authority"] == "abi-backed"
    assert {
        row["ownership_signal"] for row in rows if row["allocator_family"] == "global-cpp"
    } == {"first-party-cpp-object"}
    assert {
        row["ownership_signal"]
        for row in rows
        if row["allocator_family"] == "surrender-heap"
    } == {"surrender-facing-object"}

    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        runtime = {row["address"]: row for row in csv.DictReader(stream)}
    assert runtime["005e1c10"]["provisional_name"] == "operator_delete"
    assert runtime["005e1ce0"]["provisional_name"] == "operator_new_import_thunk"

    with (repository / "evidence/observations/surrender/wiz8-sr-imports.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        sr_imports = {row["iat_address"]: row for row in csv.DictReader(stream)}
    assert sr_imports["005ebab8"]["decorated_name"] == "?free@srHeap@@QAEXPAX@Z"
    assert sr_imports["005ebac0"]["decorated_name"] == "?allocate@srHeap@@QAEPAXK@Z"


def test_surrender_math_templates_preserve_exported_names_and_layouts() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/surrender/math-types.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert {row["type_name"] for row in rows} == {
        "srVector2T",
        "srVector3T",
        "srVector4T",
        "srMatrix3T",
        "srMatrix4T",
        "srVector2i",
        "srVector3i",
    }
    assert {row["name_origin"] for row in rows} == {"original-export"}
    assert {row["authority"] for row in rows} == {"abi-backed"}
    by_name = {row["type_name"]: row for row in rows}
    assert by_name["srVector3T"]["scalar_instantiations"] == "float|double"
    assert by_name["srVector3T"]["scalar_codes"] == "M|N"
    assert by_name["srVector3T"]["size_bytes"] == "12|24"
    assert by_name["srMatrix4T"]["size_bytes"] == "64|128"

    header = (repository / "include/surrender/srMath.h").read_text(encoding="utf-8")
    gameplay_header = (repository / "src/wiz8/gameplay_boundaries.h").read_text(
        encoding="utf-8"
    )
    item_header = (repository / "src/wiz8/item_spawning.h").read_text(encoding="utf-8")
    assert "W8Vector3" not in header + gameplay_header + item_header
    assert "srVector3T<float> position" in item_header


def test_startup_spine_separates_library_from_first_party() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/startup-spine.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 33
    assert {row["ownership"] for row in rows} == {"library", "first-party"}

    # CRT startup is linked, never modelled: every library node must name its provider.
    library = [row for row in rows if row["ownership"] == "library"]
    assert len(library) == 4
    assert all(row["provided_by"] for row in library)
    assert all("MSVCRT" in row["provided_by"] for row in library)

    # The spine is anchored at the real PE entry point and reaches the frame tick.
    by_address = {row["node_address"]: row for row in rows if row["node_address"]}
    assert by_address["00401000"]["ownership"] == "library"
    assert by_address["00401670"]["role"] == "WinMain"
    assert by_address["004e3340"]["role"] == "per-frame tick"

    # Unresolved boundaries are recorded rather than guessed. The frame dispatch
    # table has since been enumerated, so it is resolved with partial attribution.
    unresolved = [row for row in rows if row["status"] == "unresolved"]
    assert not unresolved
    # One remaining small-gate group is intentionally partial.
    assert sum(1 for row in rows if row["status"] == "partial") == 1
    # One spine node is attributable to an original translation unit: the SGP
    # DirectDraw unit whose functions are already source-matched.
    assert by_address["0040f020"]["source_unit"] == "sgp/DirectDraw Calls.c"
    # atexit registers the shutdown handler before anything is created.
    assert by_address["004011ac"]["ownership"] == "library"
    assert by_address["004017f0"]["role"] == "shutdown handler"
    dispatch = by_address["00647bd4"]
    assert dispatch["status"] == "resolved"
    assert dispatch["source_unit"] == "partial"


def test_cpp_initializer_table_witnesses_link_order() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/observations/wiz8/cpp-initializers.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 266

    # The CRT concatenates each object's .CRT$XC* in link order, so a table that is
    # overwhelmingly ascending in address witnesses the object layout order.
    deltas = [int(row["delta"]) for row in rows if row["delta"]]
    assert len(deltas) == 265
    ascending = sum(1 for delta in deltas if delta > 0)
    assert ascending == 247
    # The descending steps are recorded, not smoothed away.
    assert sum(1 for delta in deltas if delta < 0) == 18

    attributed = {row["source_unit"] for row in rows if row["source_unit"]}
    assert attributed == {
        "Engine Code\\OctPath.cpp",
        "Engine Code\\3d.cpp",
        "Engine Code\\Monster.cpp",
        "Dialog Code\\AssayDialog.cpp",
    }


def test_frame_dispatch_table_is_enumerated_and_partly_attributed() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/observations/wiz8/frame-dispatch-table.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 62
    stubs = [row for row in rows if row["kind"] == "default-stub"]
    assert len(stubs) == 17
    # One shared stub address, not seventeen separate empty handlers.
    assert {row["handler_address"] for row in stubs} == {"005b1740"}

    # Every attributable handler is a screen, which is what identifies this table.
    attributed = {row["source_unit"] for row in rows if row["source_unit"]}
    assert attributed == {
        "Local Screens\\MainGameScreen.cpp",
        "Local Screens\\MainMenuScreen.cpp",
        "Local Screens\\ReviewCharacterScreen.cpp",
    }
    assert sum(1 for row in rows if row["source_unit"]) == 5


def test_surrender_abi_surface_is_complete_and_joins_the_jpeg_model() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/observations/surrender/wiz8-sr-imports.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        wiz8 = list(csv.DictReader(stream))
    with (repository / "evidence/observations/surrender/jpeg-sr-imports.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        jpeg = {row["decorated_name"]: row for row in csv.DictReader(stream)}

    assert len(wiz8) == 461
    # Every symbol demangles, and every one is original ABI evidence.
    assert all(row["demangled_signature"] for row in wiz8)
    assert {row["name_origin"] for row in wiz8} == {"original-export"}
    assert {row["authority"] for row in wiz8} == {"abi-backed"}

    # Class counts depend on the nesting rule, so pin both; the documentation
    # states which is which rather than quoting one number as if unambiguous.
    classes = {row["class_name"] for row in wiz8 if row["class_name"]}
    assert len(classes) == 51
    assert len({name.split("::")[0] for name in classes}) == 43
    assert "srHuffman::Decompressor" in classes
    assert sum(row["kind"] == "vftable" for row in wiz8) == 6

    # 9 of the 461 are plain C, so "461 decorated C++ symbols" would be wrong.
    plain_c = [row for row in wiz8 if not row["class_name"]]
    assert len(plain_c) == 9
    assert {row["kind"] for row in plain_c} == {"free-function", "global-object"}

    ctors = {row["class_name"] for row in wiz8 if row["kind"] == "constructor"}
    dtors = {row["class_name"] for row in wiz8 if row["kind"] == "destructor"}
    assert len(ctors & dtors) == 24
    assert len({c.split("::")[0] for c in ctors} & {d.split("::")[0] for d in dtors}) == 20

    # The two import models must agree wherever they overlap.
    by_name = {row["decorated_name"]: row for row in wiz8}
    shared = set(by_name) & set(jpeg)
    assert len(shared) == 62
    for name in shared:
        recorded = jpeg[name]["demangled_signature"].strip()
        if recorded:
            assert recorded == by_name[name]["demangled_signature"].strip(), name


def test_reviewed_wiz8_classes_have_source_and_vtable_evidence() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/classes.csv").open(
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
        "stLight",
        "Controls",
        "Item",
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
    # GrCycle's proof covers a vtable slot offset only, not any data member.
    assert "slot 9 only" in by_name["GrCycle"]["layout_proof"]
    assert by_name["MonsterInfoDialog"]["layout_proof"].startswith("005d6e60")
    # stLight is named by the runtime class registry and corroborated by a source path.
    assert by_name["stLight"]["source_path"] == "Engine Code\\Include\\stLight.hpp"
    assert "0x10006" in by_name["stLight"]["evidence"]
    assert by_name["VirtualFileBinIStream"]["layout_proof"].startswith("0047d5c0")
    assert not by_name["MonsterLight"]["layout_proof"]
    for name in ("VirtualFileBinIStream", "MonsterLight"):
        assert by_name[name]["source_path"] == ""
        assert by_name[name]["base_classes"]
        assert by_name[name]["base_name_origin"] == "original-export"
    grcycle = by_name["GrCycle"]
    assert grcycle["primary_vtable_id"] == "GrCycle.primary"
    assert grcycle["constructor"] == "004a5e50"
    assert grcycle["destructor"] == "004a6610"
    assert grcycle["scalar_deleting_destructor"] == "004a5f00"
    assert grcycle["minimum_size"] == "0x1d8"
    assert grcycle["source_path"] == "Engine Code\\GrCycle.cpp"
    assert "Primary slots 4 and 11 directly reference the exact source path" in grcycle["evidence"]
    monster = by_name["Monster"]
    assert monster["primary_vtable_id"] == "Monster.primary"
    assert monster["constructor"] == "004bea20"
    assert monster["destructor"] == "004bee50"
    assert monster["scalar_deleting_destructor"] == "004beba0"
    assert monster["minimum_size"] == "0x628"
    assert monster["source_path"] == "Engine Code\\Monster.cpp"
    assert "Slots 5 12 and 26 directly reference the exact source path" in monster["evidence"]
    dialog = by_name["MonsterInfoDialog"]
    assert dialog["primary_vtable_id"] == "MonsterInfoDialog.primary"
    assert dialog["constructor"] == "005d5e30"
    assert dialog["destructor"] == "005d5f00"
    assert dialog["scalar_deleting_destructor"] == "005d5ee0"
    assert dialog["minimum_size"] == "0x130"
    assert dialog["source_path"] == "Dialog Code\\MonsterInfoDialog.cpp"
    assert "Slot 3 directly references the exact source path" in dialog["evidence"]


def test_reviewed_wiz8_class_model_owns_layout_and_vtable_facts() -> None:
    repository = Path(__file__).resolve().parents[2]
    model = load_reviewed_class_model(repository, "wiz8")

    assert len(model.classes) == 9
    assert len(model.vtables) == 10
    assert len(model.slots) == 74
    assert len(model.fields) == 16

    vtables = {item.vtable_id: item for item in model.vtables}
    assert vtables["GrCycle.primary"].address == 0x005ECE78
    assert vtables["GrCycle.primary"].slot_count == 16
    assert vtables["GrCycle.secondary_0x18"].subobject_offset == 0x18
    assert vtables["Monster.primary"].slot_count == 31
    assert vtables["MonsterInfoDialog.primary"].slot_count == 14

    monster_fields = [field for field in model.fields if field.class_name == "Monster"]
    assert [(field.offset, field.size) for field in monster_fields] == [
        (0x0, 0x4),
        (0x4, 0xA8),
        (0xAC, 0x1B0),
        (0x25C, 0x1B0),
        (0x40C, 0x1B0),
        (0x5BC, 0x6C),
    ]

    apply_script = (
        repository / "tools/wiz8decomp/ghidra/apply_wiz8_class_model.py"
    ).read_text(encoding="utf-8")
    assert "0x005ECE78" not in apply_script.upper()
    assert "GR_CYCLE_TARGETS" not in apply_script
    assert "MONSTER_TARGETS" not in apply_script


def test_reviewed_cross_build_map_is_separate_and_explicit() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/cross-build/mappings.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        mappings = list(csv.DictReader(stream))
    with (repository / "evidence/reviewed/cross-build/rejections.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rejections = list(csv.DictReader(stream))
    with (repository / "evidence/reviewed/cross-build/oracles.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/functions.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        symbols = [
            row
            for row in csv.DictReader(stream)
            if "fan-patch-signature" in row["name_origin"].split("|")
        ]
    with (repository / "evidence/observations/cfagent-128/hooks.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        hooks = list(csv.DictReader(stream))

    assert len(symbols) == 45
    assert len({row["address"] for row in symbols}) == 45
    # The bodies these seeds point at are original Wizardry code. Ownership moves off
    # fan-patch-oracle only when another source proves which codebase the body is from;
    # so far that is only 0x0040EFA0, which the SGP Random.c compile placed in sgp-shared.
    assert {row["owner"] for row in symbols} == {"fan-patch-oracle", "sgp-shared"}
    assert sum(row["owner"] == "sgp-shared" for row in symbols) == 1
    assert {row["confidence"] for row in symbols if row["owner"] == "fan-patch-oracle"} == {
        "strong"
    }
    assert next(row for row in symbols if row["owner"] == "sgp-shared")["confidence"] == "exact"
    by_name = {row["provisional_name"]: row["address"] for row in symbols}
    assert by_name["StartCombat"] == "004e7090"
    assert by_name["GetFact"] == "00506280"
    assert by_name["SetFact"] == "005061a0"

    assert len(hooks) == 26
    assert {row["ownership"] for row in hooks} == {"fan-patch-injected"}
    assert {row["kind"] for row in hooks} == {"hook", "inline-fix"}


def test_cfdat_override_evidence_separates_callsite_and_canonical_sizes() -> None:
    repository = Path(__file__).resolve().parents[2]
    with (repository / "evidence/reviewed/wiz8/formats/cfdat-overrides.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/formats/database-records.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/formats/level-formats.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/formats/variable-databases.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/formats/save-game-sections.csv").open(
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
    with (repository / "evidence/reviewed/wiz8/formats/string-databases.csv").open(
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
    with (repository / "config/reccmp/wiz8-gameplay-boundaries.csv").open(
        newline="", encoding="utf-8"
    ) as stream:
        rows = list(csv.DictReader(stream))

    assert len(rows) == 71
    exact = [row for row in rows if row["confidence"] == "exact"]
    assert len(exact) == 66
    assert {int(row["size"]) for row in exact} == {
        6,
        7,
        12,
        13,
        19,
        20,
        24,
        25,
        26,
        29,
        33,
        40,
        43,
        47,
        53,
        55,
        56,
        57,
        61,
        66,
        70,
        76,
        81,
        82,
        83,
        85,
        98,
        100,
        103,
        105,
        106,
        109,
        110,
        115,
        117,
        118,
        130,
        132,
        145,
        174,
        179,
        199,
        205,
        206,
        218,
        219,
        231,
        236,
        239,
        245,
        452,
        455,
        456,
    }
    assert all(len(row["relocation_masked_sha256"]) == 64 for row in exact)
    backfire = next(row for row in rows if row["symbol"] == "CanSpellBackfire")
    assert backfire["confidence"] == "structurally-strong"
    assert backfire["relocation_masked_sha256"] == ""
    monster_lookup = next(row for row in rows if row["symbol"] == "FindMonGenByName")
    assert monster_lookup["confidence"] == "structurally-strong"
    assert monster_lookup["relocation_masked_sha256"] == ""
    first_monster = next(row for row in rows if row["symbol"] == "FindFirstMonsterByID")
    assert first_monster["confidence"] == "exact"
    assert first_monster["relocation_masked_sha256"]
    next_monster = next(row for row in rows if row["symbol"] == "FindNextExistingMonsterByID")
    assert next_monster["confidence"] == "exact"
    assert next_monster["relocation_masked_sha256"]
    item_origin = next(row for row in rows if row["symbol"] == "GetOriginOfCharacterItem")
    assert item_origin["confidence"] == "structurally-strong"
    assert item_origin["relocation_masked_sha256"] == ""
    script_part = next(row for row in rows if row["symbol"] == "MonsterGetScriptPartByLocationIndex")
    assert script_part["confidence"] == "structurally-strong"
    assert script_part["relocation_masked_sha256"] == ""
    index_by_location = next(row for row in rows if row["symbol"] == "MonsterGetIndexByLocationID")
    assert index_by_location["confidence"] == "exact"
    assert index_by_location["relocation_masked_sha256"]
    source = "\n".join(
        (repository / path).read_text(encoding="utf-8")
        for path in (
            "src/wiz8/character_items.c",
            "src/wiz8/character_skills.c",
            "src/wiz8/controls_regions.cpp",
            "src/wiz8/chunk_io.cpp",
            "src/wiz8/fact_state.c",
            "src/wiz8/game_databases.c",
            "src/wiz8/gameplay_boundaries.c",
            "src/wiz8/location_variables.c",
            "src/wiz8/local_code/UtilityFunctions.cpp",
            "src/wiz8/grcycle_behaviour.cpp",
            "src/wiz8/ilist.c",
            "src/wiz8/item_tables.c",
            "src/wiz8/item_mesh.cpp",
            "src/wiz8/item_spawning.cpp",
            "src/wiz8/message_box.cpp",
            "src/wiz8/monster_cycles.cpp",
            "src/wiz8/monster_info_dialog.cpp",
            "src/wiz8/monster_generators.cpp",
            "src/wiz8/monster_location.c",
            "src/wiz8/monster_lookup.c",
            "src/wiz8/npc_item_lists.c",
            "src/wiz8/spell_backfire.cpp",
            "src/wiz8/targeting.c",
            "src/wiz8/state_getters.c",
            "src/wiz8/vector_conversions.cpp",
            "src/wiz8/virtual_file_stream.cpp",
            "src/wiz8/octree_loading.cpp",
            "src/wiz8/plist.c",
            "src/wiz8/world_props.cpp",
        )
    )
    for row in rows:
        assert f"// FUNCTION: WIZ8 0x{row['address'].upper()}" in source
        assert row["symbol"] in source
