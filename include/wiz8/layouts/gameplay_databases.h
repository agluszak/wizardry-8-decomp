#ifndef WIZ8_LAYOUTS_GAMEPLAY_DATABASES_H
#define WIZ8_LAYOUTS_GAMEPLAY_DATABASES_H

#include <stddef.h>

#include "wiz8/3d_code/PList.h"
#include "wiz8/dice.h"
#include "wiz8/text_types.h"
#include <wchar.h>

/*
 * The on-disk gameplay records, as the matching source compiles them.
 *
 * This is the canonical field inventory consumed by matching source, review
 * tools, and the reviewed Ghidra project. Everything here is a file format read by seeking
 * to a record index and fixed stride, so the sizes are part of the format
 * rather than incidental layout.
 */

/* The fifteen professions, in the game's fixed class order. A character's
   current and original profession are one of these and index the
   per-profession tables; a generated character carries NONE until the player
   picks one. The NPC character block stores the same value. */
enum W8Profession {
    W8_PROFESSION_FIGHTER = 0,
    W8_PROFESSION_LORD = 1,
    W8_PROFESSION_VALKYRIE = 2,
    W8_PROFESSION_RANGER = 3,
    W8_PROFESSION_SAMURAI = 4,
    W8_PROFESSION_NINJA = 5,
    W8_PROFESSION_MONK = 6,
    W8_PROFESSION_ROGUE = 7,
    W8_PROFESSION_GADGETEER = 8,
    W8_PROFESSION_BARD = 9,
    W8_PROFESSION_PRIEST = 10,
    W8_PROFESSION_ALCHEMIST = 11,
    W8_PROFESSION_BISHOP = 12,
    W8_PROFESSION_PSIONIC = 13,
    W8_PROFESSION_MAGE = 14,
    W8_PROFESSION_COUNT = 15,
    W8_PROFESSION_NONE = -1
};

/* A character's sex. Zero is male and one is female, which is what the quote
   lookup, the item record's sex mask and the female-only profession all agree
   on. A character or template that has not been assigned one carries UNSET. */
enum W8Gender {
    W8_GENDER_MALE = 0,
    W8_GENDER_FEMALE = 1,
    W8_GENDER_COUNT = 2,
    W8_GENDER_UNSET = -1
};

#pragma pack(push, 1)

enum { W8_MAX_MONSTER_ATTACKS = 3 };

/* One of a monster's three attacks. Monster.cpp walks the array with a
   0x22-byte stride and copies the missile-launch fields below into its local
   attack block. */
struct W8MonsterAttack {
    unsigned char fHasAttack;           /* 0x00 */
    unsigned char unknown_01[2];
    unsigned char range_category;       /* 0x03 */
    unsigned char unknown_04;
    unsigned char missile_values_05[0x10]; /* 0x05 */
    unsigned short attack_modes;        /* 0x15 */
    int missile_value_17;               /* 0x17 */
    unsigned char missile_value_1b;
    unsigned char unknown_1c;
    signed char missile_type;            /* 0x1d */
    unsigned char unknown_1e[4];
};                                      /* 0x22 */

enum W8SpellRealm {
    W8_SPELL_REALM_FIRE = 0,
    W8_SPELL_REALM_WATER = 1,
    W8_SPELL_REALM_AIR = 2,
    W8_SPELL_REALM_EARTH = 3,
    W8_SPELL_REALM_MENTAL = 4,
    W8_SPELL_REALM_DIVINE = 5,
    W8_SPELL_REALM_COUNT = 6
};

/* The five situations a spell record can admit, numbered by the switch in
   SpellUsableNow. A spell usable at any time imposes no condition; the other
   four admit exactly one of combat, the field, camping or a shop. */
enum W8SpellUsage {
    W8_SPELL_USABLE_ANY_TIME = 0,
    W8_SPELL_USABLE_IN_COMBAT = 1,
    W8_SPELL_USABLE_OUT_OF_COMBAT = 2,
    W8_SPELL_USABLE_WHILE_CAMPED = 3,
    W8_SPELL_USABLE_WHILE_SHOPPING = 4,
    W8_SPELL_USAGE_COUNT = 5
};

/* The range bands an attack or spell works at, and the one value that means it
   has no range at all. CalcRangeDistance is the one place the band and a
   world distance are related. */
enum W8RangeCategory {
    W8_RANGE_NONE = -1,
    W8_RANGE_TOUCH = 0,
    W8_RANGE_SHORT = 1,
    W8_RANGE_LONG = 2,
    W8_RANGE_EXTREME = 3
};

/* Which spellbooks a profession may draw on, and which a spell belongs to. The
   four flags live apart in the record rather than as one mask, so the mask is
   built from them wherever the profession table is tested. */
enum {
    W8_SPELLBOOK_NONE = 0,
    W8_SPELLBOOK_WIZARDRY = 1,
    W8_SPELLBOOK_DIVINITY = 2,
    W8_SPELLBOOK_ALCHEMY = 4,
    W8_SPELLBOOK_PSIONICS = 8
};

/* No spell. The item database spells it with this in ubSpellNumber, which the
   learn-from-item assertion names. */
enum { W8_SPELL_NONE = 0 };

/* One spell, as the database holds it at run time. */
struct W8SpellRuntimeRecord {
    char database_name[64];             /* 0x000 */
    unsigned char unknown_040[8];
    unsigned char alchemy_spell;        /* 0x048 */
    int spell_point_cost;               /* 0x049: per power level */
    unsigned char unknown_04d[4];
    W8Dice effect_dice;                 /* 0x051 */
    unsigned char unknown_055;
    int spell_level;                    /* 0x056: zero through seven */
    unsigned char wizardry_spell;       /* 0x05a */
    char resource_name[64];             /* 0x05b: visual/MLS resource basename */
    W8WideChar display_name[64];        /* 0x09b */
    unsigned char unknown_11b[4];
    unsigned char divinity_spell;       /* 0x11f */
    unsigned char psionics_spell;       /* 0x120 */
    float effect_radius;                /* 0x121 */
    unsigned char unknown_125;
    /* 0x126: a monster may cast the spell at all. MonsterOKToCastSpell reports
       a spell without it by name and asserts. */
    unsigned char monster_castable;
    unsigned char unknown_127[8];
    /* 0x12f: the range category a monster casting this spell needs. */
    W8RangeCategory range_category;
    W8SpellRealm realm;                 /* 0x133 */
    int target_type;                    /* 0x137 */
    /* 0x13b: when the spell may be cast. SpellUsableNow switches on it and its
       assertion calls it uiSpellUsableWhen with a SPELL_USAGE_COUNT of five. */
    W8SpellUsage usable_when;
    /* 0x13f: the spell has to be aimed before it can be cast. */
    unsigned char needs_aim_13f;
    unsigned char unknown_140[4];
    /* 0x144: gates the effect's activation message in the per-frame spell
       update. */
    unsigned char field_144;
    unsigned char unknown_145[6];
    char sound_name[0x74];              /* 0x14b: relative to Data\Spells\Sounds */
};                                      /* 0x1bf */

struct W8FactDatabaseRecord {
    unsigned int identifier;             /* 0x000 */
    char symbolic_name[0x32];            /* 0x004 .. 0x035 */
    /* 0x036: the journal shades this fact's text when its value is true. */
    unsigned char highlight_when_true_036;
    /* 0x037: the visibility level the journal entry needs. */
    signed char visibility_037;
    /* 0x038 and 0x100: the alternate and normal journal descriptions, wide,
       selected by the fact's current value. */
    W8WideChar alternate_description_038[0x64];
    W8WideChar description_100[0x6c];
};                                       /* 0x1d8 */

/* One optional NPC stock-rule entry appended after its database record.
   DecayNpcInventory establishes the leading item id and the keep flag at 0x05.
   RestockNpcItems establishes 0x04 as the configured quantity: it restocks only
   while the NPC holds no more than half of it, and tops up by the shortfall. */
struct W8NpcItemStockRule {
    int item_id;                           /* 0x00: Items.dbs index */
    unsigned char quantity;               /* 0x04: configured stock quantity */
    unsigned char persistent;             /* 0x05: retain and replenish this item */
};                                        /* 0x06 */

/* The RPC-character block a record with has_group carries, from the record's
   0x0c4 up to the stock-rule list at 0x2ca. 0x0050AED0 expands it into a
   W8Character and is what establishes the field extents; its biased record
   base independently shows the block as one object. */
struct W8NpcCharacterTemplate {
    wchar_t name[10];                    /* 0x000, record 0x0c4 */
    wchar_t name_part_2[6];              /* 0x014, record 0x0d8 */
    unsigned char unknown_01a[0x44];
    W8Profession profession;             /* 0x064, record 0x128: index into profession_levels[W8_PROFESSION_COUNT] */
    int race;                            /* 0x068, record 0x12c */
    int table_value;                     /* 0x06c, record 0x130: the value 0x004EF950 otherwise computes */
    unsigned int level;                  /* 0x070, record 0x134: starting profession level */
    int attributes[7];                   /* 0x074, record 0x138: W8CharacterAttribute::value per attribute */
    int skills[0x29];                    /* 0x090, record 0x154: W8CharacterSkill::value_02 per skill */
    unsigned char spells[0x72];          /* 0x134, record 0x1f8: one flag per learnable spell, indexed from one */
    unsigned short equipment_present[12];/* 0x1a6, record 0x26a: zero leaves the slot empty */
    unsigned short equipment_ids[12];    /* 0x1be, record 0x282: Items.dbs index, 0xffff empty */
    unsigned short backpack_present[8];  /* 0x1d6, record 0x29a */
    unsigned short backpack_ids[8];      /* 0x1e6, record 0x2aa */
    signed char gender;                  /* 0x1f6, record 0x2ba: copied into the character's own sex field */
    unsigned char unknown_1f7[0xf];
};                                       /* 0x206, record 0x0c4..0x2c9 */

static_assert(sizeof(W8NpcCharacterTemplate) == 0x206,
              "W8NpcCharacterTemplate_size_must_be_0x206");

/* One Data\Databases\NPC.DBS record. Only source-consumed fields are modelled
   here; Ghidra owns the wider operational field inventory. */
struct W8NpcDatabaseRecord {
    unsigned short version;              /* 0x000: two in the corpus; the rule tail loads only when this exceeds 1 */
    /* 0x002: non-zero marks the record as carrying whatever the NPC manager's
       first predicate asks about. CreateNpcRuntimeNode copies it into the
       runtime state's word at 0x0ca. */
    short value_002;
    /* 0x004: the wide source name the level-entry rebinding prefixes with an
       underscore to build the NPC's trigger name. */
    wchar_t source_name_004[0x28];
    /* 0x054: when set, releasing the NPC binding marks it before handing the
       owned item-list handle to the teardown. */
    unsigned char unknown_054;
    /* 0x055: gates the owned item-list teardown at 0x0055A5D0, which only
       releases the NPC's stock while this is set. */
    unsigned char flag_055;
    unsigned char unknown_056;
    /* 0x057: the NPC carries an RPC character and can join a monster group,
       which is what makes the group index on its runtime state meaningful. */
    unsigned char has_group;
    /* 0x058: the NPC's kind. Twenty is the one value a recovered body singles
       out, refusing to trade with it. */
    int kind;
    /* 0x05c: the disposition byte CreateNpcRuntimeNode starts the state with. */
    unsigned char disposition;
    unsigned char unknown_05d[7];
    /* 0x064: one-based index into g_item_tables selecting the record's item
       table; 0x0050B9E0 copies that table into the runtime state. The zero and
       past-the-end tests compare it signed. */
    int item_table_id;
    /* 0x068: one bit per service the NPC offers, matched against the table at
       0x00619DFC that pairs each service id with its bit. */
    unsigned int service_flags;
    unsigned char unknown_06c[0x31];
    unsigned char flag_9d;               /* 0x09d: and only when this is clear */
    /* 0x09e: what the NPC is called, unless a fact substitutes another name.
       The wide RPC-character name at 0x0c4 bounds the string extent. */
    char display_name[0x26];
    W8NpcCharacterTemplate character;    /* 0x0c4 */
    W8PList* item_stock_rules;           /* 0x2ca: W8NpcItemStockRule* elements */
    unsigned char unknown_2ce[0x1c];
    /* 0x2ea: the second half of the rebinding gate, tested together with the
       record's 0x56 byte. */
    unsigned char flag_2ea;
    /* 0x2eb: the purse the NPC carries; 0x004F8CB0 hands it to AddPartyGold
       when the NPC's monster dies. */
    int gold;
    unsigned char unknown_2ef[0x1a];
};                                       /* 0x309 */

static_assert(sizeof(W8NpcDatabaseRecord) == 0x309, "W8NpcDatabaseRecord_size_must_be_0x309");

/* One Data\Databases\LEVELS.DBS record. Only the disk and runtime stride is
   established; the leading field is a display name. */
struct W8LevelDatabaseRecord {
    unsigned char unknown_000[0x3c];
    /* 0x3c..0x50: the per-level random-encounter budget parameters, all five
       read by UpdateRandomEncounterBudget and the sixth by the culling pass.
       The reviewed Ghidra type carries the wider operational inventory. */
    int maximum_random_encounters;       /* 0x3c */
    int minimum_random_encounters;       /* 0x40 */
    int maximum_encounter_budget;        /* 0x44 */
    int minimum_encounter_budget;        /* 0x48 */
    int encounter_budget_period;         /* 0x4c: elapsed-time divisor */
    int encounter_culling_seconds;       /* 0x50 */
    unsigned char unknown_054[0x84];
};                                       /* 0xd8 */

/* One runtime DATABASES\MONSTERS.DBS record. The size is the tracked disk and
   runtime record size; source-consumed fields are typed here and the reviewed
   Ghidra type owns the wider operational inventory. */
/* The one monster record whose alternate name is used in place of its own;
   both bodies that name a monster test for it, which is why it lives here
   rather than in either of them. */
enum { W8_MONSTER_RECORD_ALTERNATE_NAME = 397 };

struct W8MonsterRecord {
    W8WideChar name_00[24];               /* 0x000: suffix after '#' removed at load */
    W8WideChar name_30[24];               /* 0x030: suffix after '#' removed at load */
    W8WideChar name_60[24];               /* 0x060: suffix after '#' removed at load */
    W8WideChar name_90[24];               /* 0x090: suffix after '#' removed at load */
    unsigned char unknown_0c0[0xb];
    /* 0x0cb: the monster's kind. The alchemy-casting rule admits kinds four,
       five and thirteen and no others, which is the only body that reads it. */
    unsigned char kind_0cb;
    /* 0x0cc: selects this monster's row in the name-prefix table at 0x0061E436,
       the same table a character indexes by sex. */
    unsigned char name_group_0cc;
    unsigned char unknown_0cd[2];
    /* 0x0cf: the monster's own percentage reduction on incoming damage. */
    unsigned char damage_reduction;
    /* 0x0d0: bit 0 routes disposition through the NPC record instead of the
       faction table, which is the only bit any recovered body reads. */
    unsigned char flags_0d0;
    /* 0x0d1: indexed 0..4 by ConvertMonsterAttribute at 0x004e5d00, which
       bounds-checks the index against five. The group update at 0x005113a0
       squares index one and scales it by fifteen for a cache duration, which is
       a use of an attribute rather than a separate field at 0x0d2. */
    unsigned char attribute_values_d1[5]; /* 0x0d1 */
    W8Dice hit_points_d6;                 /* 0x0d6: rolled into hp_max/hp_current */
    W8Dice runtime_stat_da;               /* 0x0da: rolled into W8MonsterInfo +0x2f/+0x33 */
    unsigned char unknown_0de[5];
    /* 0x0e3: selects this monster's row in the two-byte AI table at
       0x0061EEFC; the row value six is the one the AI singles out. */
    unsigned char ai_kind;
    unsigned char unknown_0e4[3];
    /* 0x0e7: the monster's three attacks, named by the Combat Range.cpp
       assertions pMonsterDB->Attack[uiAttack].fHasAttack and uiAttack <
       MAX_MONSTER_ATTACKS, which is what bounds the array at three. */
    W8MonsterAttack attacks[W8_MAX_MONSTER_ATTACKS];   /* 0x0e7 */
    unsigned char unknown_14d[3];
    unsigned char unknown_150[0x31];
    unsigned int combat_value_181;         /* 0x181: combat-strength/display value */
    unsigned char unknown_185[2];
    short record_id_187;                  /* 0x187: equals the zero-based database index */
    char cycle_name_189[0x31];             /* 0x189: GrCycle lookup key */
    float float_1ba;                      /* 0x1ba: scaled by 0x005ed4f0 */
    unsigned char unknown_1be[0x8a];
    unsigned char flag_248;
    unsigned char unknown_249;
    /* 0x24a: the monster cannot be targeted at all. Every sweep that gathers
       candidates drops it before any other test. */
    unsigned char untargetable_24a;
    unsigned char unknown_24b[4];
    unsigned char missile_value_24f;
    unsigned char unknown_250[3];
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
    /* 0x26f: the spell-point budget the monster casts out of, before its own
       runtime bonus. Zero is a data error the power-level chooser reports by
       name. */
    int sp_budget;
    unsigned char unknown_273[0x24];
};                                       /* 0x297 */

static_assert(sizeof(W8MonsterRecord) == 0x297, "W8MonsterRecord_size_must_be_0x297");

#pragma pack(pop)



/* Data\Databases race table rows the resistance recalculation reads. A
   resistance_index of -1 terminates a race's list; an adjustment above
   W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS names a character attribute instead of a
   flat amount. */
struct W8RaceResistanceAdjustment {
    int resistance_index;
    int adjustment_or_attribute;
};                                      /* 0x08 */

struct W8RaceResistanceProfile {
    W8RaceResistanceAdjustment adjustments[6];
};                                      /* 0x30 */

/* Runtime roots for the database records described above. GameplayDatabase.cpp
   owns their storage; consumers reach it through this one declaration surface
   instead of the gameplay-boundary quarantine. */
extern W8FactDatabaseRecord* g_fact_records;
extern W8LevelDatabaseRecord* g_level_records;
extern W8NpcDatabaseRecord* g_npc_records;

#endif
