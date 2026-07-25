from __future__ import annotations

from typing import Any

from ..config import Settings
from .apply_unzip_model import _apply_data, _function_type, _structure
from .environment import start_pyghidra
from .project import resolve_program_name
from .query_daemon import stop_daemon

CATEGORY = "/wiz8/formats/slf"


def apply_wiz8_format_model(
    settings: Settings,
    selector: str = "wiz8--gog-base--wiz8--18a74ff61c65",
) -> dict[str, Any]:
    """Install the binary-grounded SLF archive types and parser signatures."""

    stop_daemon(settings, quiet=True)
    start_pyghidra(settings)
    import pyghidra
    from ghidra.app.cmd.function import ApplyFunctionSignatureCmd, FunctionRenameOption
    from ghidra.program.model.data import (
        ArrayDataType,
        ByteDataType,
        CategoryPath,
        CharDataType,
        DataTypeConflictHandler,
        DWordDataType,
        EnumDataType,
        FloatDataType,
        IntegerDataType,
        PointerDataType,
        QWordDataType,
        ShortDataType,
        VoidDataType,
        WordDataType,
    )
    from ghidra.program.model.symbol import SourceType

    program_name = resolve_program_name(settings, selector)
    project = pyghidra.open_project(settings.project_dir, settings.project_name, create=False)
    result: dict[str, Any] = {"program": program_name}
    try:
        with pyghidra.program_context(project, "/" + program_name) as program:
            transaction = program.startTransaction("apply Wizardry 8 SLF format model")
            commit = False
            try:
                dtm = program.getDataTypeManager()
                category = CategoryPath(CATEGORY)
                byte = ByteDataType.dataType
                char = CharDataType.dataType
                word = WordDataType.dataType
                dword = DWordDataType.dataType
                float_type = FloatDataType.dataType
                integer = IntegerDataType.dataType
                qword = QWordDataType.dataType
                short = ShortDataType.dataType
                void = VoidDataType.dataType
                generic_pointer = PointerDataType(dtm)
                char_pointer = PointerDataType(char, dtm)

                header = _structure(
                    dtm,
                    category,
                    "W8SlfHeader",
                    0x214,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "archive_name", "archive name"),
                        (0x100, ArrayDataType(char, 256, 1), "base_path", "payload base path"),
                        (0x200, dword, "file_count", "number of EOF directory records"),
                        (0x204, dword, "second_count", "meaning not yet established"),
                        (0x208, dword, "unknown_208", "unreviewed header field"),
                        (0x20C, dword, "unknown_20c", "unreviewed header field"),
                        (0x210, dword, "unknown_210", "unreviewed header field"),
                    ],
                )
                directory_entry = _structure(
                    dtm,
                    category,
                    "W8SlfDirectoryEntry",
                    0x118,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "path", "payload path"),
                        (0x100, dword, "data_offset", "payload offset from archive start"),
                        (0x104, dword, "data_size", "payload byte size"),
                        (0x108, dword, "status_108", "low byte zero means active"),
                        (0x10C, qword, "file_time", "Windows FILETIME representation"),
                        (0x114, dword, "unknown_114", "unreviewed directory field"),
                    ],
                )
                waypoint_header = _structure(
                    dtm,
                    category,
                    "W8WaypointFileHeader",
                    0x10,
                    [
                        (0x00, dword, "version", "canonical files use version two"),
                        (0x04, dword, "unknown_04", "usually zero"),
                        (0x08, dword, "waypoint_count", "includes index-zero sentinel"),
                        (0x0C, dword, "link_count", "includes index-zero sentinel"),
                    ],
                )
                waypoint = _structure(
                    dtm,
                    category,
                    "W8WaypointDisk",
                    0x10,
                    [
                        (0x00, word, "flags", "waypoint flags"),
                        (0x02, word, "first_link", "index into waypoint links"),
                        (0x04, ArrayDataType(float_type, 3, 4), "position", "XYZ position"),
                    ],
                )
                waypoint_link = _structure(
                    dtm,
                    category,
                    "W8WaypointLinkDisk",
                    0x0E,
                    [
                        (0x00, dword, "flags", "path link flags"),
                        (
                            0x04,
                            word,
                            "source_waypoint",
                            "absent from version-one files",
                        ),
                        (0x06, word, "destination_waypoint", "destination index"),
                        (0x08, float_type, "distance", "path distance"),
                        (0x0C, word, "next_link", "next edge for the same source"),
                    ],
                )
                encounter_time = EnumDataType(category, "W8EncounterTimeCondition", 1, dtm)
                encounter_time.add("W8_ENCOUNTER_DAY", 0)
                encounter_time.add("W8_ENCOUNTER_NIGHT", 1)
                encounter_time.add("W8_ENCOUNTER_ANY_TIME", 2)
                encounter_time = dtm.addDataType(
                    encounter_time, DataTypeConflictHandler.REPLACE_HANDLER
                )
                encounter_header = _structure(
                    dtm,
                    category,
                    "W8EncounterTableDiskHeader",
                    0x108,
                    [
                        (0x000, byte, "record_kind", "four in the reviewed corpus"),
                        (0x001, ArrayDataType(char, 256, 1), "name", "table name"),
                        (0x101, dword, "unknown_101", "unreviewed table field"),
                        (0x105, word, "version", "two in the reviewed corpus"),
                        (0x107, byte, "entry_count", "column count after the header"),
                    ],
                )
                encounter_script_name = _structure(
                    dtm,
                    category,
                    "W8EncounterScriptName",
                    0x40,
                    [(0x00, ArrayDataType(char, 64, 1), "value", "script name")],
                )
                encounter_byte_vector = _structure(
                    dtm,
                    category,
                    "W8EncounterByteVector",
                    0x10,
                    [
                        (0x00, dword, "unknown_00", "runtime container field"),
                        (0x04, integer, "count", "active values"),
                        (0x08, integer, "capacity", "allocated values"),
                        (0x0C, PointerDataType(byte, dtm), "values", "byte values"),
                    ],
                )
                encounter_table = _structure(
                    dtm,
                    category,
                    "W8EncounterTableRuntime",
                    0x158,
                    [
                        (0x000, generic_pointer, "vtable", "runtime table vtable"),
                        (0x004, integer, "species_count", "active species IDs"),
                        (0x008, integer, "species_capacity", "allocated species IDs"),
                        (
                            0x00C,
                            PointerDataType(word, dtm),
                            "species_ids",
                            "monster database IDs",
                        ),
                        (
                            0x010,
                            encounter_byte_vector,
                            "rarity_class",
                            "values 3 7 20 or 70",
                        ),
                        (
                            0x020,
                            encounter_byte_vector,
                            "time_condition",
                            "W8EncounterTimeCondition values",
                        ),
                        (
                            0x030,
                            encounter_byte_vector,
                            "challenge_level",
                            "values one through fifty",
                        ),
                        (
                            0x040,
                            ArrayDataType(byte, 0x10, 1),
                            "script_names_runtime",
                            "runtime string container",
                        ),
                        (0x050, ArrayDataType(char, 256, 1), "name", "table name"),
                        (0x150, dword, "unknown_150", "copied from disk offset 0x101"),
                        (0x154, byte, "version_two_flags", "version-two trailing byte"),
                        (0x155, ArrayDataType(byte, 3, 1), "padding_155", "alignment"),
                    ],
                )
                live_entry = _structure(
                    dtm,
                    category,
                    "W8SlfLiveEntry",
                    0x0C,
                    [
                        (0x00, char_pointer, "path", "separately allocated path"),
                        (0x04, dword, "data_size", "payload byte size"),
                        (0x08, dword, "data_offset", "payload offset"),
                    ],
                )
                live_entry_pointer = PointerDataType(live_entry, dtm)
                archive_state = _structure(
                    dtm,
                    category,
                    "W8SlfArchiveState",
                    0x28,
                    [
                        (0x00, char_pointer, "base_path", "copied from W8SlfHeader"),
                        (0x04, generic_pointer, "archive_file", "Win32 file handle"),
                        (0x08, word, "active_entry_count", "retained directory records"),
                        (0x0A, byte, "is_open", "nonzero after successful initialization"),
                        (0x0B, byte, "unknown_0b", "unreviewed state byte"),
                        (0x0C, dword, "unknown_0c", "unreviewed state field"),
                        (0x10, dword, "unknown_10", "initialized to zero"),
                        (0x14, dword, "lookup_bucket_count", "initialized to 0x14"),
                        (0x18, live_entry_pointer, "entries", "active entry array"),
                        (0x1C, generic_pointer, "lookup_buckets", "0x140-byte allocation"),
                        (0x20, generic_pointer, "mapping_handle", "optional mapping handle"),
                        (0x24, generic_pointer, "mapping_view", "optional read-only view"),
                    ],
                )
                archive_state_pointer = PointerDataType(archive_state, dtm)
                configuration = _structure(
                    dtm,
                    category,
                    "W8SlfConfiguration",
                    0x103,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "archive_path", "configured path"),
                        (0x100, byte, "enabled", "whether this slot is considered"),
                        (0x101, byte, "allow_fallback", "try the alternate base path"),
                        (0x102, byte, "map_file", "create a read-only file mapping"),
                    ],
                )
                dice = _structure(
                    dtm,
                    category,
                    "W8Dice",
                    0x04,
                    [
                        (0x00, short, "base", "signed additive base"),
                        (0x02, byte, "count", "number of independent rolls"),
                        (0x03, byte, "sides", "one through this value per roll"),
                    ],
                )
                item_instance = _structure(
                    dtm,
                    category,
                    "W8ItemInstance",
                    0x0C,
                    [
                        (0x00, integer, "item_id", "-1 is empty"),
                        (0x04, byte, "stack_count", "quantity-kind one"),
                        (0x05, byte, "uses_or_charges", "quantity-kinds two through four"),
                        (0x06, byte, "identified", "nonzero when identified"),
                        (0x07, ArrayDataType(byte, 4, 1), "unknown_07", "unreviewed state"),
                        (0x0B, byte, "unknown_0b", "optional initialization flag"),
                    ],
                )
                item_record = _structure(
                    dtm,
                    category,
                    "W8ItemDatabaseRecord",
                    0x10D,
                    [
                        (
                            0x000,
                            ArrayDataType(word, 30, 2),
                            "display_name",
                            "30 UTF-16 code units",
                        ),
                        (0x03C, ArrayDataType(byte, 3, 1), "unknown_03c", "unreviewed fields"),
                        (
                            0x03F,
                            word,
                            "unidentified_name_index",
                            "generic-name string-table index",
                        ),
                        (0x041, byte, "flags_041", "bit zero starts identified"),
                        (0x042, ArrayDataType(byte, 0x24, 1), "unknown_042", "unreviewed fields"),
                        (0x066, byte, "quantity_kind", "zero none; one stack; two-four uses"),
                        (0x067, dice, "initial_quantity", "initial stack/use dice"),
                        (0x06B, ArrayDataType(byte, 0x4E, 1), "unknown_06b", "unreviewed fields"),
                        (0x0B9, integer, "combine_ingredient_a", "first recipe item ID"),
                        (0x0BD, integer, "combine_ingredient_b", "second recipe item ID"),
                        (0x0C1, ArrayDataType(byte, 8, 1), "unknown_0c1", "unreviewed fields"),
                        (0x0C9, byte, "combine_skill", "0xff means no skill check"),
                        (0x0CA, byte, "combine_minimum_skill", "required skill value"),
                        (0x0CB, ArrayDataType(byte, 0x42, 1), "unknown_0cb", "unreviewed fields"),
                    ],
                )
                item_table_entry = _structure(
                    dtm,
                    category,
                    "W8ItemTableEntry",
                    0x05,
                    [
                        (0x00, short, "selector_00", "zero disables the slot"),
                        (0x02, short, "item_id", "index into Items.dbs"),
                        (0x04, byte, "weight", "weighted random selection"),
                    ],
                )
                item_table = _structure(
                    dtm,
                    category,
                    "W8ItemTableRecord",
                    0x1F1,
                    [
                        (0x000, ArrayDataType(char, 256, 1), "name", "lookup name"),
                        (0x100, dword, "category_id", "category-name index"),
                        (
                            0x104,
                            ArrayDataType(item_table_entry, 40, 5),
                            "entries",
                            "forty weighted item slots",
                        ),
                        (
                            0x1CC,
                            byte,
                            "level_scaled",
                            "filter candidates against current party level",
                        ),
                        (0x1CD, ArrayDataType(byte, 0x24, 1), "unknown_1cd", "unreviewed"),
                    ],
                )
                spellbook_mask = EnumDataType(category, "W8SpellbookMask", 1, dtm)
                spellbook_mask.add("W8_SPELLBOOK_NONE", 0)
                spellbook_mask.add("W8_SPELLBOOK_WIZARDRY", 1)
                spellbook_mask.add("W8_SPELLBOOK_DIVINITY", 2)
                spellbook_mask.add("W8_SPELLBOOK_ALCHEMY", 4)
                spellbook_mask.add("W8_SPELLBOOK_PSIONICS", 8)
                spellbook_mask = dtm.addDataType(
                    spellbook_mask, DataTypeConflictHandler.REPLACE_HANDLER
                )
                attribute_minimums = _structure(
                    dtm,
                    category,
                    "W8AttributeMinimums",
                    0x1C,
                    [(0x00, ArrayDataType(integer, 7, 4), "values", "seven core attributes")],
                )
                profession_abilities = _structure(
                    dtm,
                    category,
                    "W8ProfessionAbilities",
                    0x0C,
                    [(0x00, ArrayDataType(integer, 3, 4), "ability_ids", "-1 terminates the list")],
                )
                race_abilities = _structure(
                    dtm,
                    category,
                    "W8RaceAbilities",
                    0x14,
                    [(0x00, ArrayDataType(integer, 5, 4), "ability_ids", "-1 is unused")],
                )
                resistance_adjustment = _structure(
                    dtm,
                    category,
                    "W8RaceResistanceAdjustment",
                    0x08,
                    [
                        (0x00, integer, "resistance_index", "-1 terminates the list"),
                        (
                            0x04,
                            integer,
                            "adjustment_or_attribute",
                            "values above 1000 select character data",
                        ),
                    ],
                )
                resistance_profile = _structure(
                    dtm,
                    category,
                    "W8RaceResistanceProfile",
                    0x30,
                    [
                        (
                            0x00,
                            ArrayDataType(resistance_adjustment, 6, 0x08),
                            "adjustments",
                            "six resistance adjustment slots",
                        )
                    ],
                )
                skill_attributes = _structure(
                    dtm,
                    category,
                    "W8SkillAttributes",
                    0x10,
                    [
                        (0x00, integer, "category", "zero through four; four is expert"),
                        (0x04, integer, "unknown_04", "unreviewed skill attribute"),
                        (0x08, integer, "unknown_08", "unreviewed skill attribute"),
                        (0x0C, integer, "unknown_0c", "unreviewed skill attribute"),
                    ],
                )
                profession_skills = _structure(
                    dtm,
                    category,
                    "W8ProfessionSkills",
                    0x10,
                    [(0x00, ArrayDataType(integer, 4, 4), "skill_ids", "four professional skills")],
                )
                starting_equipment = _structure(
                    dtm,
                    category,
                    "W8StartingEquipment",
                    0x18,
                    [(0x00, ArrayDataType(integer, 6, 4), "item_ids", "-1 is unused")],
                )
                faction = EnumDataType(category, "W8Faction", 4, dtm)
                for name, value in (
                    ("W8_FACTION_UNALIGNED", 0),
                    ("W8_FACTION_PARTY", 1),
                    ("W8_FACTION_DARK_SAVANT", 2),
                    ("W8_FACTION_COSMIC_LORDS", 3),
                    ("W8_FACTION_UMPANI", 4),
                    ("W8_FACTION_TRANG", 5),
                    ("W8_FACTION_MOOK", 6),
                    ("W8_FACTION_RATTKIN_COMMON", 7),
                    ("W8_FACTION_RATTKIN_MAFIA", 8),
                    ("W8_FACTION_BROTHERHOOD", 9),
                    ("W8_FACTION_HIGARDI_BANK", 10),
                    ("W8_FACTION_HIGARDI_HLL", 11),
                    ("W8_FACTION_HIGARDI_COMMON", 12),
                    ("W8_FACTION_TRYNNIE", 13),
                    ("W8_FACTION_MAD_MARTEN", 14),
                    ("W8_FACTION_RAPAX_COMMON", 15),
                    ("W8_FACTION_RAPAX_TEMPLAR", 16),
                    ("W8_FACTION_RAPAX_ARMY", 17),
                    ("W8_FACTION_KINGS_ASSASINS", 18),
                    ("W8_FACTION_FILLER3", 19),
                    ("W8_FACTION_FILLER4", 20),
                ):
                    faction.add(name, value)
                faction = dtm.addDataType(faction, DataTypeConflictHandler.REPLACE_HANDLER)
                faction_disposition = EnumDataType(category, "W8FactionDisposition", 1, dtm)
                faction_disposition.add("W8_FACTION_HOSTILE", 0)
                faction_disposition.add("W8_FACTION_NEUTRAL", 1)
                faction_disposition.add("W8_FACTION_FRIENDLY", 2)
                faction_disposition = dtm.addDataType(
                    faction_disposition, DataTypeConflictHandler.REPLACE_HANDLER
                )
                monster_companion = _structure(
                    dtm,
                    category,
                    "W8MonsterCompanion",
                    0x03,
                    [
                        (0x00, short, "species_id", "less than one means absent"),
                        (0x02, byte, "spawn_chance_percent", "tested before spawning"),
                    ],
                )
                monster_record = _structure(
                    dtm,
                    category,
                    "W8MonsterDatabaseRecord",
                    0x297,
                    [
                        (
                            offset,
                            ArrayDataType(word, 24, 2),
                            f"name_{offset:02x}",
                            "UTF-16 name; suffix after '#' removed at load",
                        )
                        for offset in (0x00, 0x30, 0x60, 0x90)
                    ]
                    + [
                        (0x0C0, byte, "unknown_0c0", "unreviewed field"),
                        (0x0C1, dice, "group_size", "rolled when a group is spawned"),
                        (
                            0x0C5,
                            ArrayDataType(monster_companion, 2, 3),
                            "companions",
                            "two optional species and spawn-chance records",
                        ),
                        (0x0CB, ArrayDataType(byte, 5, 1), "unknown_0cb", "unreviewed fields"),
                        (0x0D0, byte, "flags_0d0", "bit zero uses NPC disposition"),
                        (0x0D1, byte, "unknown_0d1", "unreviewed field"),
                        (
                            0x0D2,
                            byte,
                            "disposition_cache_factor",
                            "squared then scaled for disposition cache duration",
                        ),
                        (0x0D3, ArrayDataType(byte, 0xB4, 1), "unknown_0d3", "unreviewed fields"),
                        (0x187, short, "record_id", "equals the zero-based database index"),
                        (0x189, ArrayDataType(byte, 0xD2, 1), "unknown_189", "unreviewed fields"),
                        (
                            0x25B,
                            integer,
                            "hostility_range",
                            "controls unaligned default and proximity-triggered hostility",
                        ),
                        (0x25F, faction, "faction_id", "W8Faction value"),
                        (0x263, ArrayDataType(byte, 4, 1), "unknown_263", "unreviewed fields"),
                        (0x267, byte, "deleted", "nonzero records are rejected by loaders"),
                        (0x268, ArrayDataType(byte, 2, 1), "unknown_268", "unreviewed fields"),
                        (
                            0x26A,
                            byte,
                            "flag_26a",
                            "selects an alternate monster-group configuration",
                        ),
                        (0x26B, ArrayDataType(byte, 0x2C, 1), "unknown_26b", "unreviewed fields"),
                    ],
                )
                level_record = _structure(
                    dtm,
                    category,
                    "W8LevelDatabaseRecord",
                    0xD8,
                    [
                        (
                            0x00,
                            ArrayDataType(word, 30, 2),
                            "display_name",
                            "30 UTF-16 code units",
                        ),
                        (
                            0x3C,
                            integer,
                            "maximum_random_encounters",
                            "upper bound for the runtime encounter limit",
                        ),
                        (
                            0x40,
                            integer,
                            "minimum_random_encounters",
                            "lower bound for the runtime encounter limit",
                        ),
                        (0x44, integer, "maximum_encounter_budget", "upper budget bound"),
                        (0x48, integer, "minimum_encounter_budget", "lower budget bound"),
                        (
                            0x4C,
                            integer,
                            "encounter_budget_period",
                            "elapsed-time divisor for budget replenishment",
                        ),
                        (
                            0x50,
                            integer,
                            "encounter_culling_seconds",
                            "developer-menu encounter culling time",
                        ),
                        (0x54, float_type, "unknown_054", "unreviewed level scale"),
                        (0x58, integer, "level_id", "equals the zero-based database index"),
                        (
                            0x5C,
                            ArrayDataType(byte, 0x7C, 1),
                            "reserved_05c",
                            "zero in the reviewed corpus",
                        ),
                    ],
                )
                fact_record = _structure(
                    dtm,
                    category,
                    "W8FactDatabaseRecord",
                    0x1D8,
                    [
                        (0x000, dword, "identifier", "32-bit fact identifier"),
                        (
                            0x004,
                            ArrayDataType(char, 256, 1),
                            "symbolic_name",
                            "FACT_* identifier",
                        ),
                        (
                            0x104,
                            ArrayDataType(word, 106, 2),
                            "description",
                            "optional designer annotation in UTF-16",
                        ),
                    ],
                )
                npc_fact_rule = _structure(
                    dtm,
                    category,
                    "W8NpcFactRule",
                    0x06,
                    [
                        (0x00, dword, "fact_id", "fact or derived-fact identifier"),
                        (0x04, word, "flags", "predicate and operator bits"),
                    ],
                )
                npc_record = _structure(
                    dtm,
                    category,
                    "W8NpcDatabaseRecord",
                    0x309,
                    [
                        (0x000, word, "version", "two in all reviewed records"),
                        (0x002, word, "unknown_002", "unreviewed field"),
                        (
                            0x004,
                            ArrayDataType(word, 40, 2),
                            "name_aliases",
                            "packed null-terminated UTF-16 aliases",
                        ),
                        (
                            0x054,
                            byte,
                            "spawn_mode",
                            "zero creates a persistent runtime node",
                        ),
                        (0x055, byte, "has_inventory", "initializes runtime inventory"),
                        (0x056, byte, "unknown_056", "unreviewed flag"),
                        (
                            0x057,
                            byte,
                            "has_rpc_character",
                            "allocates a full 0x1862-byte character state",
                        ),
                        (0x058, dword, "record_id", "equals the zero-based index"),
                        (0x05C, ArrayDataType(byte, 0x18, 1), "unknown_05c", "unreviewed"),
                        (
                            0x074,
                            ArrayDataType(char, 0x29, 1),
                            "entity_aliases",
                            "packed null-terminated aliases",
                        ),
                        (
                            0x09D,
                            byte,
                            "unknown_09d",
                            "zero enables the version-two rule tail",
                        ),
                        (
                            0x09E,
                            ArrayDataType(char, 0x26, 1),
                            "script_aliases",
                            "packed null-terminated aliases",
                        ),
                        (0x0C4, ArrayDataType(byte, 0x206, 1), "unknown_0c4", "unreviewed"),
                        (
                            0x2CA,
                            generic_pointer,
                            "fact_rules_runtime",
                            "runtime-owned rule container",
                        ),
                        (0x2CE, ArrayDataType(byte, 0x1D, 1), "unknown_2ce", "unreviewed"),
                        (0x2EB, dword, "unknown_2eb", "copied into the runtime node"),
                        (0x2EF, ArrayDataType(byte, 0x1A, 1), "unknown_2ef", "unreviewed"),
                    ],
                )
                save_section_tag = EnumDataType(category, "W8SaveSectionTag", 4, dtm)
                for name, value in (
                    ("W8_SAVE_GVER", 0x52455647),
                    ("W8_SAVE_GSTA", 0x41545347),
                    ("W8_SAVE_SHOT", 0x544F4853),
                    ("W8_SAVE_TEXT", 0x54584554),
                    ("W8_SAVE_TVAR", 0x52415654),
                    ("W8_SAVE_NPCI", 0x4943504E),
                    ("W8_SAVE_NPCT", 0x5443504E),
                    ("W8_SAVE_NPCF", 0x4643504E),
                    ("W8_SAVE_FATA", 0x41544146),
                    ("W8_SAVE_JRNL", 0x4C4E524A),
                    ("W8_SAVE_HYPN", 0x4E505948),
                    ("W8_SAVE_LVLS", 0x534C564C),
                    ("W8_SAVE_STAT", 0x54415453),
                    ("W8_SAVE_MONS", 0x534E4F4D),
                    ("W8_SAVE_ITEM", 0x4D455449),
                    ("W8_SAVE_CUBE", 0x45425543),
                    ("W8_SAVE_MONG", 0x474E4F4D),
                    ("W8_SAVE_LOCK", 0x4B434F4C),
                    ("W8_SAVE_TRES", 0x53455254),
                    ("W8_SAVE_AUTO", 0x4F545541),
                    ("W8_SAVE_TRIG", 0x47495254),
                    ("W8_SAVE_APST", 0x54535041),
                    ("W8_SAVE_CUBS", 0x53425543),
                    ("W8_SAVE_MGNS", 0x534E474D),
                    ("W8_SAVE_LCKS", 0x534B434C),
                    ("W8_SAVE_AMBS", 0x53424D41),
                    ("W8_SAVE_PART", 0x54524150),
                    ("W8_SAVE_LGHT", 0x5448474C),
                ):
                    save_section_tag.add(name, value)
                save_section_tag = dtm.addDataType(
                    save_section_tag, DataTypeConflictHandler.REPLACE_HANDLER
                )
                save_status_header = _structure(
                    dtm,
                    category,
                    "W8SaveStatusHeader",
                    0x314,
                    [
                        (0x000, float_type, "version", "2.0 in the canonical build"),
                        (0x004, integer, "unknown_004", "restored as one when zero"),
                        (0x008, integer, "unknown_008", "restored as one when zero"),
                        (0x00C, integer, "unknown_00c", "restored as one when zero"),
                        (0x010, integer, "unknown_010", "restored as one when zero"),
                        (
                            0x014,
                            ArrayDataType(byte, 0x100, 1),
                            "global_status",
                            "copied to and from the canonical global state",
                        ),
                        (
                            0x114,
                            ArrayDataType(byte, 0x200, 1),
                            "reserved_114",
                            "zeroed by SaveStatusHeader",
                        ),
                    ],
                )
                spell_realm = EnumDataType(category, "W8SpellRealm", 4, dtm)
                spell_realm.add("W8_SPELL_REALM_FIRE", 0)
                spell_realm.add("W8_SPELL_REALM_WATER", 1)
                spell_realm.add("W8_SPELL_REALM_AIR", 2)
                spell_realm.add("W8_SPELL_REALM_EARTH", 3)
                spell_realm.add("W8_SPELL_REALM_MENTAL", 4)
                spell_realm.add("W8_SPELL_REALM_DIVINE", 5)
                spell_realm = dtm.addDataType(spell_realm, DataTypeConflictHandler.REPLACE_HANDLER)
                spell_record = _structure(
                    dtm,
                    category,
                    "W8SpellRuntimeRecord",
                    0x1BF,
                    [
                        (0x000, ArrayDataType(char, 64, 1), "database_name", "database name"),
                        (0x040, ArrayDataType(byte, 8, 1), "unknown_040", "unreviewed fields"),
                        (0x048, byte, "alchemy_spell", "belongs to the Alchemy spellbook"),
                        (0x049, integer, "spell_point_cost", "cost per power level"),
                        (0x04D, ArrayDataType(byte, 4, 1), "unknown_04d", "unreviewed fields"),
                        (0x051, dice, "effect_dice", "effect magnitude dice"),
                        (0x055, byte, "unknown_055", "unreviewed flag"),
                        (0x056, integer, "spell_level", "zero through seven"),
                        (
                            0x05A,
                            byte,
                            "wizardry_spell",
                            "belongs to the Wizardry spellbook",
                        ),
                        (
                            0x05B,
                            ArrayDataType(char, 64, 1),
                            "resource_name",
                            "visual or MLS resource basename",
                        ),
                        (
                            0x09B,
                            ArrayDataType(word, 64, 2),
                            "display_name",
                            "64 UTF-16 code units",
                        ),
                        (0x11B, ArrayDataType(byte, 4, 1), "unknown_11b", "unreviewed fields"),
                        (0x11F, byte, "divinity_spell", "belongs to the Divinity spellbook"),
                        (0x120, byte, "psionics_spell", "belongs to the Psionics spellbook"),
                        (0x121, float_type, "effect_radius", "effect radius or range"),
                        (0x125, ArrayDataType(byte, 0x0E, 1), "unknown_125", "unreviewed fields"),
                        (0x133, spell_realm, "realm", "six-value spell realm"),
                        (
                            0x137,
                            integer,
                            "target_type",
                            "targeting domain not yet enumerated",
                        ),
                        (
                            0x13B,
                            ArrayDataType(byte, 0x10, 1),
                            "unknown_13b",
                            "effect fields not yet reconciled",
                        ),
                        (
                            0x14B,
                            ArrayDataType(char, 0x74, 1),
                            "sound_name",
                            "relative to Data\\Spells\\Sounds",
                        ),
                    ],
                )

                item_record_pointer = PointerDataType(item_record, dtm)
                item_table_pointer = PointerDataType(item_table, dtm)
                dice_pointer = PointerDataType(dice, dtm)
                item_instance_pointer = PointerDataType(item_instance, dtm)
                monster_record_pointer = PointerDataType(monster_record, dtm)
                level_record_pointer = PointerDataType(level_record, dtm)
                fact_record_pointer = PointerDataType(fact_record, dtm)
                npc_record_pointer = PointerDataType(npc_record, dtm)
                encounter_table_pointer = PointerDataType(encounter_table, dtm)
                spell_record_pointer = PointerDataType(spell_record, dtm)
                profession_skill_availability = ArrayDataType(integer, 15, 4)
                seek_origin = EnumDataType(category, "W8VirtualFileSeekOrigin", 1, dtm)
                seek_origin.add("W8_SEEK_BEGIN", 1)
                seek_origin.add("W8_SEEK_END", 2)
                seek_origin.add("W8_SEEK_CURRENT", 4)
                seek_origin = dtm.addDataType(seek_origin, DataTypeConflictHandler.REPLACE_HANDLER)

                address_space = program.getAddressFactory().getDefaultAddressSpace()
                _apply_data(
                    program,
                    address_space.getAddress(0x006000C8),
                    ArrayDataType(configuration, 6, 0x103),
                )
                _apply_data(
                    program,
                    address_space.getAddress(0x006EB724),
                    archive_state_pointer,
                )
                for raw_address, data_type in (
                    (0x00614CF0, ArrayDataType(attribute_minimums, 11, 0x1C)),
                    (0x00614E24, ArrayDataType(attribute_minimums, 15, 0x1C)),
                    (0x0061507C, ArrayDataType(profession_abilities, 15, 0x0C)),
                    (0x00615130, ArrayDataType(race_abilities, 16, 0x14)),
                    (0x00615270, ArrayDataType(resistance_profile, 16, 0x30)),
                    (0x00615570, ArrayDataType(float_type, 15, 4)),
                    (0x006155B0, ArrayDataType(skill_attributes, 41, 0x10)),
                    (
                        0x00615840,
                        ArrayDataType(profession_skill_availability, 41, 0x3C),
                    ),
                    (0x006161DC, ArrayDataType(integer, 15, 4)),
                    (0x00616218, ArrayDataType(profession_skills, 15, 0x10)),
                    (0x00616310, ArrayDataType(integer, 15, 4)),
                    (0x0061634C, ArrayDataType(spellbook_mask, 15, 1)),
                    (0x0061635C, ArrayDataType(starting_equipment, 15, 0x18)),
                    (0x006164C4, starting_equipment),
                    (0x0060A6C0, integer),
                    (0x0060A6C4, integer),
                    (0x0060A6C8, integer),
                    (0x0065BE18, dword),
                    (0x0065BE1C, spell_record_pointer),
                    (0x0065BA24, dword),
                    (0x0065BA28, dword),
                    (0x0065BA2C, PointerDataType(encounter_table_pointer, dtm)),
                    (0x0065BA3C, dword),
                    (0x0065BA40, dword),
                    (0x0065BA44, PointerDataType(char_pointer, dtm)),
                    (0x006836B0, PointerDataType(item_table_pointer, dtm)),
                    (0x006836B4, PointerDataType(char_pointer, dtm)),
                    (0x006836A0, npc_record_pointer),
                    (0x006836A4, level_record_pointer),
                    (0x006836AC, fact_record_pointer),
                    (0x00683F78, dword),
                    (0x00683F7C, dword),
                    (0x00683F80, dword),
                    (0x00683F84, dword),
                    (0x00683F88, dword),
                    (0x00683F8C, dword),
                    (0x00683F90, dword),
                    (0x006840C7, ArrayDataType(monster_record_pointer, 1000, 4)),
                    (0x0068516C, item_record_pointer),
                    (0x00685191, ArrayDataType(item_instance, 500, 0x0C)),
                    (0x00686901, dword),
                    (0x006874CB, item_instance),
                    (0x00688290, byte),
                    (0x00689B78, ArrayDataType(byte, 1000, 1)),
                ):
                    _apply_data(program, address_space.getAddress(raw_address), data_type)

                signatures: dict[int, tuple[Any, list[tuple[str, Any]]]] = {
                    0x00404C80: (
                        integer,
                        [
                            ("path", char_pointer),
                            ("open_flags", dword),
                            ("overlapped", byte),
                        ],
                    ),
                    0x00404E10: (void, [("handle", integer)]),
                    0x00404EA0: (
                        dword,
                        [
                            ("handle", integer),
                            ("buffer", generic_pointer),
                            ("size", dword),
                            ("bytes_read", PointerDataType(dword, dtm)),
                        ],
                    ),
                    0x00405030: (
                        dword,
                        [
                            ("handle", integer),
                            ("offset", integer),
                            ("origin", seek_origin),
                        ],
                    ),
                    0x004ACC10: (dword, []),
                    0x004AC9D0: (
                        integer,
                        [("spell_id", integer), ("normalize_single_target", byte)],
                    ),
                    0x004ACA60: (byte, [("spell_id", integer)]),
                    0x004ACB40: (integer, [("spell_level", integer)]),
                    0x004ACBA0: (integer, [("spell_id", integer)]),
                    0x004FF3B0: (
                        integer,
                        [("character", generic_pointer), ("profession_id", integer)],
                    ),
                    0x004F88A0: (integer, [("name", char_pointer)]),
                    0x004F88F0: (
                        dword,
                        [
                            ("output_items", generic_pointer),
                            ("table_id", dword),
                            ("maximum_items", dword),
                        ],
                    ),
                    0x004126F0: (dword, []),
                    0x00412A10: (dword, []),
                    0x00412BB0: (
                        dword,
                        [
                            ("archive_path", char_pointer),
                            ("state", archive_state_pointer),
                            ("allow_fallback", byte),
                        ],
                    ),
                    0x004E57C0: (monster_record_pointer, [("monster_id", dword)]),
                    0x00517970: (integer, [("dice", dice_pointer)]),
                    0x005179B0: (
                        integer,
                        [("base", integer), ("exponent", integer)],
                    ),
                    0x005179D0: (
                        void,
                        [
                            ("value", PointerDataType(integer, dtm)),
                            ("minimum", integer),
                            ("maximum", integer),
                        ],
                    ),
                    0x005061A0: (
                        void,
                        [
                            ("fact_id", integer),
                            ("value", byte),
                            ("suppress_side_effects", byte),
                        ],
                    ),
                    0x00506280: (byte, [("fact_id", integer)]),
                    0x00506310: (void, []),
                    0x00506480: (void, [("save_handle", integer)]),
                    0x005064A0: (void, [("save_handle", integer)]),
                    0x00506670: (
                        void,
                        [("fact_id", integer), ("value", byte)],
                    ),
                    0x005080F0: (byte, [("fact_id", integer)]),
                    0x00509AA0: (generic_pointer, [("npc_id", integer)]),
                    0x005123F0: (
                        dword,
                        [("slot_name", char_pointer), ("screenshot", generic_pointer)],
                    ),
                    0x00512920: (dword, [("slot_name", char_pointer)]),
                    0x00512D00: (dword, []),
                    0x00513090: (dword, []),
                    0x00513260: (dword, [("save_context", generic_pointer)]),
                    0x00535AD0: (faction_disposition, [("faction", byte)]),
                    0x0051B5C0: (
                        PointerDataType(word, dtm),
                        [
                            ("item", item_instance_pointer),
                            ("include_quantity", byte),
                        ],
                    ),
                    0x0052A2F0: (void, [("character", generic_pointer)]),
                    0x00551A60: (void, [("character", generic_pointer)]),
                    0x00553D90: (
                        byte,
                        [
                            ("character", generic_pointer),
                            ("skill_id", dword),
                            ("expert_realm_flags", generic_pointer),
                        ],
                    ),
                    0x00553F10: (
                        void,
                        [
                            ("character", generic_pointer),
                            ("skill_id", integer),
                            ("usage_points", integer),
                            ("suppress_notification", byte),
                        ],
                    ),
                    0x00557350: (
                        void,
                        [
                            ("character", generic_pointer),
                            ("creation_budget", generic_pointer),
                            ("eligibility", PointerDataType(byte, dtm)),
                        ],
                    ),
                    0x00557430: (void, [("character", generic_pointer)]),
                    0x0051C020: (
                        void,
                        [
                            ("item", item_instance_pointer),
                            ("item_id", dword),
                            ("maximum_quantity", byte),
                            ("force_identified", byte),
                            ("mark_special", byte),
                        ],
                    ),
                    0x00517EA0: (void, [("name", PointerDataType(word, dtm))]),
                    0x0054A400: (dword, []),
                    0x0054A510: (dword, []),
                    0x0054A8A0: (
                        byte,
                        [("monster_index", dword), ("record", monster_record_pointer)],
                    ),
                    0x0054AAC0: (dword, []),
                    0x0054AC90: (void, []),
                    0x0054AD00: (dword, []),
                    0x0054AE00: (void, []),
                    0x0054AE20: (dword, []),
                    0x0048C110: (byte, [("save_handle", integer)]),
                    0x0048A7A0: (dword, []),
                    0x0048B9A0: (
                        dword,
                        [
                            ("table", encounter_table_pointer),
                            ("candidate_indices", generic_pointer),
                        ],
                    ),
                    0x0048C810: (void, [("reset_budget", byte)]),
                    0x0048C8E0: (void, []),
                }
                typed_functions: list[str] = []
                for raw_address, (return_type, arguments) in signatures.items():
                    address = address_space.getAddress(raw_address)
                    function = program.getFunctionManager().getFunctionAt(address)
                    if function is None:
                        raise RuntimeError(f"no function at 0x{raw_address:08x}")
                    signature = _function_type(
                        dtm,
                        category,
                        f"signature_{function.getName()}",
                        return_type,
                        arguments,
                        "__cdecl",
                    )
                    command = ApplyFunctionSignatureCmd(
                        address,
                        signature,
                        SourceType.USER_DEFINED,
                        True,
                        FunctionRenameOption.NO_CHANGE,
                    )
                    if not command.applyTo(program):
                        raise RuntimeError(
                            f"failed to apply signature at 0x{raw_address:08x}: "
                            f"{command.getStatusMsg()}"
                        )
                    typed_functions.append(f"0x{raw_address:08x}")

                symbol_table = program.getSymbolTable()
                for raw_address, name in (
                    (0x006000C8, "g_slf_configurations"),
                    (0x0060A6C0, "g_random_encounter_limit"),
                    (0x0060A6C4, "g_random_encounter_budget"),
                    (0x0060A6C8, "g_encounter_culling_time_seconds"),
                    (0x00614CF0, "g_race_attribute_minimums"),
                    (0x00614E24, "g_profession_attribute_minimums"),
                    (0x0061507C, "g_profession_abilities"),
                    (0x00615130, "g_race_abilities"),
                    (0x00615270, "g_race_resistance_profiles"),
                    (0x00615570, "g_profession_hit_point_factors"),
                    (0x006155B0, "g_skill_attributes"),
                    (0x00615840, "g_profession_skill_availability"),
                    (0x006161DC, "g_profession_bonus_skills"),
                    (0x00616218, "g_profession_skills"),
                    (0x00616310, "g_profession_magic_level_offsets"),
                    (0x0061634C, "g_profession_spellbooks"),
                    (0x0061635C, "g_profession_starting_equipment"),
                    (0x006164C4, "g_faerie_starting_equipment"),
                    (0x0065BE18, "g_spell_database_version"),
                    (0x0065BE1C, "g_spell_records"),
                    (0x0065BA24, "g_encounter_table_count"),
                    (0x0065BA28, "g_encounter_table_capacity"),
                    (0x0065BA2C, "g_encounter_tables"),
                    (0x0065BA3C, "g_encounter_name_count"),
                    (0x0065BA40, "g_encounter_name_capacity"),
                    (0x0065BA44, "g_encounter_names"),
                    (0x006836B0, "g_item_tables"),
                    (0x006836B4, "g_item_table_category_names"),
                    (0x006836A0, "g_npc_records"),
                    (0x006836A4, "g_level_records"),
                    (0x006836AC, "g_fact_records"),
                    (0x00683F78, "g_item_record_count"),
                    (0x00683F7C, "g_item_table_count"),
                    (0x00683F80, "g_item_table_category_count"),
                    (0x00683F84, "g_monster_record_count"),
                    (0x00683F88, "g_npc_record_count"),
                    (0x00683F8C, "g_fact_record_count"),
                    (0x00683F90, "g_level_record_count"),
                    (0x006840C7, "g_monster_record_cache"),
                    (0x0068516C, "g_item_records"),
                    (0x00685191, "g_party_item_pool"),
                    (0x00686901, "g_party_item_count"),
                    (0x006874CB, "g_item_in_hand"),
                    (0x00688290, "g_log_fact_checks"),
                    (0x00689B78, "g_fact_values"),
                    (0x006EB724, "g_slf_archives"),
                ):
                    address = address_space.getAddress(raw_address)
                    symbol = symbol_table.getPrimarySymbol(address)
                    if symbol is None:
                        symbol = symbol_table.createLabel(address, name, SourceType.USER_DEFINED)
                    elif symbol.getName() != name:
                        symbol.setName(name, SourceType.USER_DEFINED)

                commit = True
                result.update(
                    {
                        "structures": [
                            str(data_type.getPathName())
                            for data_type in (
                                header,
                                directory_entry,
                                waypoint_header,
                                waypoint,
                                waypoint_link,
                                encounter_time,
                                encounter_header,
                                encounter_script_name,
                                encounter_byte_vector,
                                encounter_table,
                                live_entry,
                                archive_state,
                                configuration,
                                dice,
                                item_instance,
                                item_record,
                                item_table_entry,
                                item_table,
                                spellbook_mask,
                                attribute_minimums,
                                profession_abilities,
                                race_abilities,
                                resistance_adjustment,
                                resistance_profile,
                                skill_attributes,
                                profession_skills,
                                starting_equipment,
                                faction,
                                faction_disposition,
                                monster_companion,
                                monster_record,
                                level_record,
                                fact_record,
                                npc_fact_rule,
                                npc_record,
                                save_section_tag,
                                save_status_header,
                                spell_realm,
                                spell_record,
                            )
                        ],
                        "typed_functions": typed_functions,
                        "typed_globals": [
                            "0x006000c8",
                            "0x0060a6c0",
                            "0x0060a6c4",
                            "0x0060a6c8",
                            "0x00614cf0",
                            "0x00614e24",
                            "0x0061507c",
                            "0x00615130",
                            "0x00615270",
                            "0x00615570",
                            "0x006155b0",
                            "0x00615840",
                            "0x006161dc",
                            "0x00616218",
                            "0x00616310",
                            "0x0061634c",
                            "0x0061635c",
                            "0x006164c4",
                            "0x0065be18",
                            "0x0065be1c",
                            "0x0065ba24",
                            "0x0065ba28",
                            "0x0065ba2c",
                            "0x0065ba3c",
                            "0x0065ba40",
                            "0x0065ba44",
                            "0x006836b0",
                            "0x006836b4",
                            "0x006836a0",
                            "0x006836a4",
                            "0x006836ac",
                            "0x00683f78",
                            "0x00683f7c",
                            "0x00683f80",
                            "0x00683f84",
                            "0x00683f88",
                            "0x00683f8c",
                            "0x00683f90",
                            "0x006840c7",
                            "0x0068516c",
                            "0x00685191",
                            "0x00686901",
                            "0x006874cb",
                            "0x00688290",
                            "0x00689b78",
                            "0x006eb724",
                        ],
                    }
                )
            finally:
                program.endTransaction(transaction, commit)
            if commit:
                program.save("apply Wizardry 8 SLF format model", pyghidra.task_monitor())
    finally:
        project.close()
    return result
