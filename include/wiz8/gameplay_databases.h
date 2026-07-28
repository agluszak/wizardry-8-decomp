#ifndef WIZ8_GAMEPLAY_DATABASES_H
#define WIZ8_GAMEPLAY_DATABASES_H

/*
 * The on-disk gameplay records, as the matching source compiles them.
 *
 * These are the types config/types/wiz8/gameplay_databases.h catalogues for
 * readers; this is the copy the build actually compiles against, and a test
 * holds the two to the same sizes. Everything here is a file format - read by
 * seeking to a record index and a fixed stride - so the sizes are part of the
 * format rather than incidental layout, and each carries an assertion.
 */

typedef unsigned short W8WideChar;

#pragma pack(push, 1)

typedef struct W8Dice {
    short base;
    unsigned char count;
    unsigned char sides;
} W8Dice;

typedef struct W8SpellRuntimeRecord {
    unsigned char unknown_000[0x48];
    unsigned char alchemy_spell;        /* 0x048 */
    int spell_point_cost;               /* 0x049 */
    unsigned char unknown_04d[9];
    int spell_level;                    /* 0x056 */
    unsigned char unknown_05a[0x41];
    W8WideChar display_name[64];        /* 0x09b */
    unsigned char unknown_11b[0xa];
    unsigned char unknown_125;
    /* 0x126: a monster may cast the spell at all. MonsterOKToCastSpell reports
       a spell without it by name and asserts. */
    unsigned char monster_castable;
    unsigned char unknown_127[0x10];
    int target_type;                    /* 0x137 */
    /* 0x13b: when the spell may be cast, zero through four. SpellUsableNow
       switches on it and its assertion calls it uiSpellUsableWhen with a
       SPELL_USAGE_COUNT of five. */
    int usable_when;
    unsigned char unknown_13f[0x80];
} W8SpellRuntimeRecord;                 /* 0x1bf */

typedef struct W8FactDatabaseRecord {
    unsigned int identifier;
    char symbolic_name[256];             /* 0x004 */
    W8WideChar description[106];         /* 0x104 */
} W8FactDatabaseRecord;                  /* 0x1d8 */

/* One Data\Databases\NPC.DBS record. Only the loader-touched fields are
   modelled here; config/types/wiz8/npc_database.h carries the full field
   model under the same name, and the spellings are kept convergent. */
typedef struct W8NpcDatabaseRecord {
    unsigned short version;              /* 0x000: two in the corpus; the rule tail loads only when this exceeds 1 */
    unsigned char unknown_002[0x9b];
    unsigned char flag_9d;               /* 0x09d: and only when this is clear */
    unsigned char unknown_09e[0x22c];
    void* fact_rules_runtime;            /* 0x2ca: runtime-owned rule PList */
    unsigned char unknown_2ce[0x3b];
} W8NpcDatabaseRecord;                   /* 0x309 */

/* One Data\Databases\LEVELS.DBS record. Only the disk and runtime stride is
   established; the leading field is a display name. */
typedef struct W8LevelDatabaseRecord {
    unsigned char unknown_000[0x3c];
    /* 0x3c..0x50: the per-level random-encounter budget parameters, all five
       read by UpdateRandomEncounterBudget and the sixth by the culling pass.
       Mirrors config/types/wiz8/gameplay_databases.h. */
    int maximum_random_encounters;       /* 0x3c */
    int minimum_random_encounters;       /* 0x40 */
    int maximum_encounter_budget;        /* 0x44 */
    int minimum_encounter_budget;        /* 0x48 */
    int encounter_budget_period;         /* 0x4c: elapsed-time divisor */
    int encounter_culling_seconds;       /* 0x50 */
    unsigned char unknown_054[0x84];
} W8LevelDatabaseRecord;                 /* 0xd8 */

/* One runtime DATABASES\MONSTERS.DBS record. The size is the tracked disk and
   runtime record size; the fields are typed by their consumers elsewhere.
   This is the same record config/types/wiz8/gameplay_databases.h models as
   W8MonsterDatabaseRecord and the Ghidra applied types use under that name;
   the porting name deliberately keeps the established W8MonsterRecord
   spelling of its byte-exact consumers, and the two field models are kept
   convergent rather than merged. */
typedef struct W8MonsterRecord {
    W8WideChar name_00[24];               /* 0x000: suffix after '#' removed at load */
    W8WideChar name_30[24];               /* 0x030: suffix after '#' removed at load */
    W8WideChar name_60[24];               /* 0x060: suffix after '#' removed at load */
    W8WideChar name_90[24];               /* 0x090: suffix after '#' removed at load */
    unsigned char unknown_0c0[0xb];
    /* 0x0cb: the monster's kind. The alchemy-casting rule admits kinds four,
       five and thirteen and no others, which is the only body that reads it. */
    unsigned char kind_0cb;
    /* 0x0cc: selects this monster's row in the name-prefix table at 0x0061E436,
       the same table a character indexes by faction. */
    unsigned char name_group_0cc;
    unsigned char unknown_0cd[3];
    /* 0x0d0: bit 0 routes disposition through the NPC record instead of the
       faction table, which is the only bit any recovered body reads. */
    unsigned char flags_0d0;
    /* 0x0d1: indexed 0..4 by ConvertMonsterAttribute at 0x004e5d00, which
       bounds-checks the index against five. The group update at 0x005113a0
       squares index one and scales it by fifteen for a cache duration, which is
       a use of an attribute rather than a separate field at 0x0d2. */
    unsigned char attribute_values_d1[5];
    W8Dice hit_points_d6;                 /* 0x0d6: rolled into hp_max/hp_current */
    W8Dice runtime_stat_da;               /* 0x0da: rolled into W8MonsterInfo +0x2f/+0x33 */
    unsigned char unknown_0de[0xa3];
    unsigned int combat_value_181;         /* 0x181: combat-strength/display value */
    unsigned char unknown_185[2];
    short record_id_187;                  /* 0x187: equals the zero-based database index */
    char cycle_name_189[0x31];             /* 0x189: GrCycle lookup key */
    float float_1ba;                      /* 0x1ba: scaled by 0x005ed4f0 */
    unsigned char unknown_1be[0x95];
    int value_253;                        /* 0x253: selected by 0x004e5b50 */
    int value_257;                        /* 0x257: alternate selected value */
    int hostility_range_25b;              /* 0x25b: unaligned; positive is a proximity threshold */
    int faction_id_25f;                   /* 0x25f: W8Faction value, domain 0..20 */
    unsigned char unknown_263[4];
    /* Carved out because LoadMonsterGroup skips every live-group step for a
       record that has it set. */
    unsigned char deleted;                /* 0x267 */
    unsigned char unknown_268[2];
    unsigned char flag_26a;               /* 0x26a: selects an alternate group configuration */
    unsigned int combat_value_override_26b; /* 0x26b: nonzero override */
    unsigned char unknown_26f[0x28];
} W8MonsterRecord;                       /* 0x297 */

#pragma pack(pop)



/* Data\Databases race table rows the resistance recalculation reads. A
   resistance_index of -1 terminates a race's list; an adjustment above
   W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS names a character attribute instead of a
   flat amount. Mirrors config/types/wiz8/gameplay_databases.h. */
typedef struct W8RaceResistanceAdjustment {
    int resistance_index;
    int adjustment_or_attribute;
} W8RaceResistanceAdjustment;           /* 0x08 */

typedef struct W8RaceResistanceProfile {
    W8RaceResistanceAdjustment adjustments[6];
} W8RaceResistanceProfile;              /* 0x30 */

#endif
