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
                            ArrayDataType(byte, 0x9C, 1),
                            "fields_03c",
                            "not yet field-reconciled",
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
                            ArrayDataType(byte, 0xD4, 1),
                            "fields_104",
                            "not yet field-reconciled",
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
                dice_pointer = PointerDataType(dice, dtm)
                item_instance_pointer = PointerDataType(item_instance, dtm)
                monster_record_pointer = PointerDataType(monster_record, dtm)
                level_record_pointer = PointerDataType(level_record, dtm)
                spell_record_pointer = PointerDataType(spell_record, dtm)
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
                    (0x0065BE18, dword),
                    (0x0065BE1C, spell_record_pointer),
                    (0x006836A4, level_record_pointer),
                    (0x00683F78, dword),
                    (0x00683F84, dword),
                    (0x00683F90, dword),
                    (0x006840C7, ArrayDataType(monster_record_pointer, 1000, 4)),
                    (0x0068516C, item_record_pointer),
                    (0x00685191, ArrayDataType(item_instance, 500, 0x0C)),
                    (0x00686901, dword),
                    (0x006874CB, item_instance),
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
                    0x00535AD0: (faction_disposition, [("faction", byte)]),
                    0x0051B5C0: (
                        PointerDataType(word, dtm),
                        [
                            ("item", item_instance_pointer),
                            ("include_quantity", byte),
                        ],
                    ),
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
                    0x0054A8A0: (
                        byte,
                        [("monster_index", dword), ("record", monster_record_pointer)],
                    ),
                    0x0054AE20: (dword, []),
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
                    (0x0065BE18, "g_spell_database_version"),
                    (0x0065BE1C, "g_spell_records"),
                    (0x006836A4, "g_level_records"),
                    (0x00683F78, "g_item_record_count"),
                    (0x00683F84, "g_monster_record_count"),
                    (0x00683F90, "g_level_record_count"),
                    (0x006840C7, "g_monster_record_cache"),
                    (0x0068516C, "g_item_records"),
                    (0x00685191, "g_party_item_pool"),
                    (0x00686901, "g_party_item_count"),
                    (0x006874CB, "g_item_in_hand"),
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
                                live_entry,
                                archive_state,
                                configuration,
                                dice,
                                item_instance,
                                item_record,
                                faction,
                                faction_disposition,
                                monster_companion,
                                monster_record,
                                level_record,
                                fact_record,
                                spell_realm,
                                spell_record,
                            )
                        ],
                        "typed_functions": typed_functions,
                        "typed_globals": [
                            "0x006000c8",
                            "0x0065be18",
                            "0x0065be1c",
                            "0x006836a4",
                            "0x00683f78",
                            "0x00683f84",
                            "0x00683f90",
                            "0x006840c7",
                            "0x0068516c",
                            "0x00685191",
                            "0x00686901",
                            "0x006874cb",
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
