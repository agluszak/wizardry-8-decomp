#ifndef WIZ8_FORMATS_GAMEPLAY_DATABASES_H
#define WIZ8_FORMATS_GAMEPLAY_DATABASES_H

#include <stdint.h>

#pragma pack(push, 1)

typedef uint16_t W8WideChar;

typedef struct W8RecordDatabaseHeader {
    uint32_t record_count;              /* 0x00 */
} W8RecordDatabaseHeader;               /* 0x04 */

typedef struct W8VersionedRecordDatabaseHeader {
    uint32_t record_count;              /* 0x00 */
    uint32_t version;                   /* 0x04 */
} W8VersionedRecordDatabaseHeader;      /* 0x08 */

typedef struct W8Dice {
    int16_t base;                       /* 0x00 */
    uint8_t count;                      /* 0x02 */
    uint8_t sides;                      /* 0x03 */
} W8Dice;                               /* 0x04 */

typedef struct W8ItemInstance {
    int32_t item_id;                    /* 0x00: -1 is empty */
    uint8_t stack_count;                /* 0x04: quantity-kind 1 */
    uint8_t uses_or_charges;            /* 0x05: quantity-kinds 2 through 4 */
    uint8_t identified;                 /* 0x06 */
    uint8_t unknown_07[4];              /* 0x07 */
    uint8_t unknown_0b;                 /* 0x0b */
} W8ItemInstance;                       /* 0x0c */

typedef struct W8ItemDatabaseRecord {
    W8WideChar display_name[30];         /* 0x000 */
    uint8_t unknown_03c[2];             /* 0x03c */
    uint8_t equip_class;                /* 0x03e: zero through twelve */
    uint16_t unidentified_name_index;   /* 0x03f */
    uint8_t flags_041;                  /* 0x041: bit zero starts identified */
    uint8_t category;                   /* 0x042: three is a spell source */
    uint8_t unknown_043[0x23];          /* 0x043 */
    uint8_t quantity_kind;              /* 0x066: zero none, one stack, two-four uses */
    W8Dice initial_quantity;             /* 0x067 */
    uint8_t unknown_06b[0x1f];          /* 0x06b */
    uint32_t value;                     /* 0x086: gold value, per bundle for class four */
    uint16_t weight;                    /* 0x08a: weight of one item */
    uint8_t unknown_08c[0x2d];          /* 0x08c */
    int32_t combine_ingredient_a;       /* 0x0b9 */
    int32_t combine_ingredient_b;       /* 0x0bd */
    uint8_t unknown_0c1[8];             /* 0x0c1 */
    uint8_t combine_skill;              /* 0x0c9: 0xff means no skill check */
    uint8_t combine_minimum_skill;      /* 0x0ca */
    uint8_t unknown_0cb[0x42];          /* 0x0cb */
} W8ItemDatabaseRecord;                 /* 0x10d */

typedef enum W8Faction {
    W8_FACTION_UNALIGNED = 0,
    W8_FACTION_PARTY = 1,
    W8_FACTION_DARK_SAVANT = 2,
    W8_FACTION_COSMIC_LORDS = 3,
    W8_FACTION_UMPANI = 4,
    W8_FACTION_TRANG = 5,
    W8_FACTION_MOOK = 6,
    W8_FACTION_RATTKIN_COMMON = 7,
    W8_FACTION_RATTKIN_MAFIA = 8,
    W8_FACTION_BROTHERHOOD = 9,
    W8_FACTION_HIGARDI_BANK = 10,
    W8_FACTION_HIGARDI_HLL = 11,
    W8_FACTION_HIGARDI_COMMON = 12,
    W8_FACTION_TRYNNIE = 13,
    W8_FACTION_MAD_MARTEN = 14,
    W8_FACTION_RAPAX_COMMON = 15,
    W8_FACTION_RAPAX_TEMPLAR = 16,
    W8_FACTION_RAPAX_ARMY = 17,
    W8_FACTION_KINGS_ASSASINS = 18,
    W8_FACTION_FILLER3 = 19,
    W8_FACTION_FILLER4 = 20,
    W8_FACTION_COUNT = 21
} W8Faction;

typedef enum W8FactionDisposition {
    W8_FACTION_HOSTILE = 0,
    W8_FACTION_NEUTRAL = 1,
    W8_FACTION_FRIENDLY = 2
} W8FactionDisposition;

enum {
    W8_CORE_ATTRIBUTE_COUNT = 7,
    W8_PROFESSION_COUNT = 15,
    W8_RACE_COUNT = 16,
    W8_PLAYABLE_RACE_COUNT = 11,
    W8_SKILL_COUNT = 41
};

typedef uint8_t W8SpellbookMask;

enum {
    W8_SPELLBOOK_NONE = 0,
    W8_SPELLBOOK_WIZARDRY = 1,
    W8_SPELLBOOK_DIVINITY = 2,
    W8_SPELLBOOK_ALCHEMY = 4,
    W8_SPELLBOOK_PSIONICS = 8
};

typedef struct W8AttributeMinimums {
    int32_t values[W8_CORE_ATTRIBUTE_COUNT];
} W8AttributeMinimums;                  /* 0x1c */

typedef struct W8ProfessionAbilities {
    int32_t ability_ids[3];             /* -1 terminates the displayed list */
} W8ProfessionAbilities;                /* 0x0c */

typedef struct W8RaceAbilities {
    int32_t ability_ids[5];             /* -1 denotes an unused slot */
} W8RaceAbilities;                      /* 0x14 */

typedef struct W8RaceResistanceAdjustment {
    int32_t resistance_index;           /* -1 terminates this race's list */
    int32_t adjustment_or_attribute;    /* values above 1000 select character data */
} W8RaceResistanceAdjustment;           /* 0x08 */

typedef struct W8RaceResistanceProfile {
    W8RaceResistanceAdjustment adjustments[6];
} W8RaceResistanceProfile;              /* 0x30 */

typedef struct W8SkillAttributes {
    int32_t category;                   /* 0..4; category four is expert skills */
    int32_t unknown_04;
    int32_t unknown_08;
    int32_t unknown_0c;
} W8SkillAttributes;                    /* 0x10 */

typedef struct W8ProfessionSkills {
    int32_t skill_ids[4];
} W8ProfessionSkills;                   /* 0x10 */

typedef struct W8StartingEquipment {
    int32_t item_ids[6];                /* -1 denotes an unused slot */
} W8StartingEquipment;                  /* 0x18 */

typedef struct W8MonsterCompanion {
    int16_t species_id;                 /* 0x00: less than one means absent */
    uint8_t spawn_chance_percent;       /* 0x02 */
} W8MonsterCompanion;                   /* 0x03 */

typedef struct W8MonsterDatabaseRecord {
    W8WideChar name_00[24];             /* 0x000: suffix after '#' removed at load */
    W8WideChar name_30[24];             /* 0x030: suffix after '#' removed at load */
    W8WideChar name_60[24];             /* 0x060: suffix after '#' removed at load */
    W8WideChar name_90[24];             /* 0x090: suffix after '#' removed at load */
    uint8_t unknown_0c0;                /* 0x0c0 */
    W8Dice group_size;                  /* 0x0c1 */
    W8MonsterCompanion companions[2];   /* 0x0c5 */
    uint8_t unknown_0cb[5];             /* 0x0cb */
    uint8_t flags_0d0;                  /* 0x0d0: bit zero uses NPC disposition */
    uint8_t attribute_values[5];        /* 0x0d1: indexed 0..4 by 0x004e5d00 */
    uint8_t unknown_0d6[0xab];          /* 0x0d6 */
    uint32_t combat_value;              /* 0x181: combat-strength/display value */
    uint8_t unknown_185[2];             /* 0x185 */
    int16_t record_id;                  /* 0x187: equals the database index */
    char cycle_name_189[0x31];          /* 0x189: case-insensitive GrCycle lookup key */
    float float_1ba;                    /* 0x1ba: scaled by 0x005ed4f0 */
    uint8_t unknown_1be[0x95];          /* 0x1be */
    int32_t value_253;                  /* 0x253: selected by 0x004e5b50 */
    int32_t value_257;                  /* 0x257: alternate selected value */
    int32_t hostility_range;            /* 0x25b: unaligned default/proximity hostility */
    int32_t faction_id;                 /* 0x25f: W8Faction value */
    uint8_t unknown_263[4];             /* 0x263 */
    uint8_t deleted;                    /* 0x267 */
    uint8_t unknown_268[2];             /* 0x268 */
    uint8_t flag_26a;                   /* 0x26a: selects alternate group configuration */
    uint32_t combat_value_override;     /* 0x26b: nonzero value overrides +0x181 */
    uint8_t unknown_26f[0x28];          /* 0x26f */
} W8MonsterDatabaseRecord;              /* 0x297 */

typedef struct W8LevelDatabaseRecord {
    W8WideChar display_name[30];         /* 0x00 */
    int32_t maximum_random_encounters;   /* 0x3c */
    int32_t minimum_random_encounters;   /* 0x40 */
    int32_t maximum_encounter_budget;    /* 0x44 */
    int32_t minimum_encounter_budget;    /* 0x48 */
    int32_t encounter_budget_period;     /* 0x4c: elapsed-time divisor */
    int32_t encounter_culling_seconds;   /* 0x50 */
    float unknown_054;                  /* 0x54 */
    int32_t level_id;                   /* 0x58: equals the database index */
    uint8_t reserved_05c[0x7c];         /* 0x5c: zero in the reviewed corpus */
} W8LevelDatabaseRecord;                /* 0xd8 */

typedef struct W8FactDatabaseRecord {
    uint32_t identifier;                /* 0x000: first record contains zero */
    char symbolic_name[256];            /* 0x004: FACT_* identifier */
    W8WideChar description[106];        /* 0x104: optional designer annotation */
} W8FactDatabaseRecord;                 /* 0x1d8 */

typedef enum W8SpellRealm {
    W8_SPELL_REALM_FIRE = 0,
    W8_SPELL_REALM_WATER = 1,
    W8_SPELL_REALM_AIR = 2,
    W8_SPELL_REALM_EARTH = 3,
    W8_SPELL_REALM_MENTAL = 4,
    W8_SPELL_REALM_DIVINE = 5,
    W8_SPELL_REALM_COUNT = 6
} W8SpellRealm;

typedef struct W8SpellRuntimeRecord {
    char database_name[64];             /* 0x000 */
    uint8_t unknown_040[8];             /* 0x040 */
    uint8_t alchemy_spell;              /* 0x048 */
    int32_t spell_point_cost;           /* 0x049: per power level */
    uint8_t unknown_04d[4];             /* 0x04d */
    W8Dice effect_dice;                 /* 0x051 */
    uint8_t unknown_055;                /* 0x055 */
    int32_t spell_level;                /* 0x056: zero through seven */
    uint8_t wizardry_spell;             /* 0x05a */
    char resource_name[64];             /* 0x05b: visual/MLS resource basename */
    W8WideChar display_name[64];         /* 0x09b */
    uint8_t unknown_11b[4];             /* 0x11b */
    uint8_t divinity_spell;             /* 0x11f */
    uint8_t psionics_spell;             /* 0x120 */
    float effect_radius;                /* 0x121 */
    uint8_t unknown_125;                /* 0x125 */
    uint8_t monster_castable;           /* 0x126: MonsterOKToCastSpell asserts on it */
    uint8_t unknown_127[8];             /* 0x127 */
    int32_t range_category;             /* 0x12f: range a monster casting it needs */
    W8SpellRealm realm;                 /* 0x133 */
    int32_t target_type;                /* 0x137: domain not yet enumerated */
    int32_t usable_when;                /* 0x13b: uiSpellUsableWhen, zero through four */
    uint8_t needs_aim_13f;              /* 0x13f: has to be aimed before it is cast */
    uint8_t unknown_140[0x0b];          /* 0x140 */
    char sound_name[0x74];              /* 0x14b: relative to Data\Spells\Sounds */
} W8SpellRuntimeRecord;                 /* 0x1bf */

typedef struct W8SpellDiskRecord {
    uint8_t ignored_prefix[0x101];       /* explicitly skipped by InitializeSpellDatabase */
    W8SpellRuntimeRecord runtime;        /* 0x101 */
} W8SpellDiskRecord;                    /* 0x2c0 */

#pragma pack(pop)

#endif
