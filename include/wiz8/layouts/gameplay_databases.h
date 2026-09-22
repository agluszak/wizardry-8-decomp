#ifndef WIZ8_LAYOUTS_GAMEPLAY_DATABASES_H
#define WIZ8_LAYOUTS_GAMEPLAY_DATABASES_H

#include <stddef.h>

#include "wiz8/layouts/plist.h"
#include "wiz8/dice.h"
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
    unsigned char fHasAttack; /* 0x00 */
    /* 0x01: exact name from the Combat Attack.cpp assertion, which wants it
       other than MON_WEAPON_NAME_UNARMED; indexes the announcement verb table
       at 0x0061EB4C. */
    unsigned char ubWeaponNameIndex;
    /* 0x02: the weapon type; the announcement reports "weapon type is NONE"
       when it is zero and indexes the name-id table at 0x0061EB02. */
    unsigned char weapon_type_02;
    unsigned char range_category; /* 0x03 */
    /* 0x04: the attack's innate attack-score value; the monster score formula
       adds it alongside the modifier hit bonus and re-reads it inside the
       surprise repick penalty. */
    unsigned char attack_score_04;
    unsigned char missile_values_05[0x10]; /* 0x05 */
    unsigned short attack_modes;           /* 0x15 */
    /* 0x17: the attack's damage dice, packed; the missile path copies it
       into the attack block and the melee path rolls it. */
    W8Dice damage_dice;
    unsigned char missile_value_1b;
    /* 0x1c: the attack's weapon class, handed signed to BlockedForSpecialReason
       the way a character's weapon_sound_class_0c5 is. */
    signed char weapon_class_1c;
    signed char missile_type; /* 0x1d */
    unsigned char unknown_1e[4];
}; /* 0x22 */

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
   four admit exactly one of combat, the field, camping, or a lock/trap
   interaction. */
enum W8SpellUsage {
    W8_SPELL_USABLE_ANY_TIME = 0,
    W8_SPELL_USABLE_IN_COMBAT = 1,
    W8_SPELL_USABLE_OUT_OF_COMBAT = 2,
    W8_SPELL_USABLE_WHILE_CAMPED = 3,
    W8_SPELL_USABLE_ON_LOCK_OR_TRAP = 4,
    W8_SPELL_USAGE_COUNT = 5
};

/* SPELL_COUNT, named by the SpellUsableNow assertion that bounds its
   argument: the number of rows in the spell database. */
enum { W8_SPELL_COUNT = 0x96 };

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

/* Who or what a spell is cast on. The eleven values are the spell record's
   target_type domain; the interface names them through string ids 798-808,
   which give "Caster", "One Ally", "Party", "One Enemy", "Enemy Group",
   "Cone", "Radius", "All Enemies", "Point" and "Adventuring" twice over - the
   two adventuring kinds are the carried item (Identify Item) and the lock or
   trap (Knock Knock, Divine Trap). This is not the targeting-need domain
   GetTargetNeededForSpellFriendly maps these onto. */
enum W8SpellTargetType {
    W8_TARGET_TYPE_CASTER = 0,
    W8_TARGET_TYPE_ALLY = 1,
    W8_TARGET_TYPE_PARTY = 2,
    W8_TARGET_TYPE_ENEMY = 3,
    W8_TARGET_TYPE_ENEMY_GROUP = 4,
    W8_TARGET_TYPE_CONE = 5,
    W8_TARGET_TYPE_RADIUS = 6,
    W8_TARGET_TYPE_ALL_ENEMIES = 7,
    W8_TARGET_TYPE_POINT = 8,
    W8_TARGET_TYPE_ITEM = 9,
    W8_TARGET_TYPE_LOCK_OR_TRAP = 10,
    W8_TARGET_TYPE_COUNT = 11
};

/* One spell, as the database holds it at run time. */
struct W8SpellRuntimeRecord {
    char database_name[64]; /* 0x000 */
    unsigned char unknown_040[4];
    /* 0x044: base duration the spell-info dialog prints. */
    int duration_044;
    unsigned char alchemy_spell; /* 0x048 */
    int spell_point_cost;        /* 0x049: per power level */
    /* 0x04d: per-level duration added to duration_044 on the same line. */
    int duration_per_level_04d;
    W8Dice effect_dice; /* 0x051 */
    unsigned char unknown_055;
    int spell_level;              /* 0x056: zero through seven */
    unsigned char wizardry_spell; /* 0x05a */
    char resource_name[64];       /* 0x05b: visual/MLS resource basename */
    wchar_t display_name[64];     /* 0x09b */
    unsigned char unknown_11b[4];
    unsigned char divinity_spell; /* 0x11f */
    unsigned char psionics_spell; /* 0x120 */
    float effect_radius;          /* 0x121 */
    unsigned char blocks_auto_power_in_combat;
    /* 0x126: a monster may cast the spell at all. MonsterOKToCastSpell reports
       a spell without it by name and asserts. */
    unsigned char monster_castable;
    /* 0x127: radius added per power level; PopulateSpellTargetMarkers scales
       the pair as (radius_per_level_127 * power + effect_radius) * 500. */
    float radius_per_level_127;
    /* 0x12b: SpellInfoDialog selects the long-range caption when this is 3. */
    int field_12b;
    /* 0x12f: the range category a monster casting this spell needs. */
    W8RangeCategory range_category;
    W8SpellRealm realm;            /* 0x133 */
    W8SpellTargetType target_type; /* 0x137 */
    /* 0x13b: when the spell may be cast. SpellUsableNow switches on it and its
       assertion calls it uiSpellUsableWhen with a SPELL_USAGE_COUNT of five. */
    W8SpellUsage usable_when;
    /* 0x13f: the spell has to be aimed before it can be cast. */
    unsigned char needs_aim_13f;
    /* 0x140: the MissileTables.dbs row index the cast fires its effect
       through when missile_delivered is set. */
    int missile_index_140;
    /* 0x144: the spell's effect is delivered by a missile in flight, so its
       queued effect stays alive until the missile lands. Set for the ten
       projectile spells - Frost, Heal Wounds, Make Wounds, Sleep, Stamina,
       Terror, Noxious Fumes, Crush, Return to Portal and the monster Special
       Attack Cone. */
    unsigned char missile_delivered;
    unsigned char unknown_145[2];
    /* 0x147: SpellInfoDialog prints the effect-dice line when this is set. */
    int show_effect_dice;
    char sound_name[0x74]; /* 0x14b: relative to Data\Spells\Sounds */
}; /* 0x1bf */
static_assert(sizeof(W8SpellRuntimeRecord) == 0x1bf, "W8SpellRuntimeRecord_size");
static_assert(offsetof(W8SpellRuntimeRecord, duration_044) == 0x044,
              "W8SpellRuntimeRecord_duration_044");
static_assert(offsetof(W8SpellRuntimeRecord, duration_per_level_04d) == 0x04d,
              "W8SpellRuntimeRecord_duration_per_level");
static_assert(offsetof(W8SpellRuntimeRecord, show_effect_dice) == 0x147,
              "W8SpellRuntimeRecord_show_effect_dice");

struct W8FactDatabaseRecord {
    unsigned int identifier;  /* 0x000 */
    char symbolic_name[0x32]; /* 0x004 .. 0x035 */
    /* 0x036: the journal shades this fact's text when its value is true. */
    unsigned char highlight_when_true_036;
    /* 0x037: the visibility level the journal entry needs. */
    signed char visibility_037;
    /* 0x038 and 0x100: the alternate and normal journal descriptions, wide,
       selected by the fact's current value. */
    wchar_t alternate_description_038[0x64];
    wchar_t description_100[0x6c];
}; /* 0x1d8 */

static_assert(sizeof(W8FactDatabaseRecord) == 0x1d8, "W8FactDatabaseRecord_size_must_be_0x1d8");

/* One optional NPC stock-rule entry appended after its database record.
   DecayNpcInventory establishes the leading item id and the keep flag at 0x05.
   RestockNpcItems establishes 0x04 as the configured quantity: it restocks only
   while the NPC holds no more than half of it, and tops up by the shortfall. */
struct W8NpcItemStockRule {
    int item_id;              /* 0x00: Items.dbs index */
    unsigned char quantity;   /* 0x04: configured stock quantity */
    unsigned char persistent; /* 0x05: retain and replenish this item */
}; /* 0x06 */

/* The RPC-character block a record with has_group carries, from the record's
   0x0c4 up to the stock-rule list at 0x2ca. 0x0050AED0 expands it into a
   W8Character and is what establishes the field extents; its biased record
   base independently shows the block as one object. */
struct W8NpcCharacterTemplate {
    wchar_t name[10];       /* 0x000, record 0x0c4 */
    wchar_t name_part_2[6]; /* 0x014, record 0x0d8 */
    unsigned char unknown_01a[0x44];
    W8Profession
        profession;     /* 0x064, record 0x128: index into profession_levels[W8_PROFESSION_COUNT] */
    int race;           /* 0x068, record 0x12c */
    int table_value;    /* 0x06c, record 0x130: the value 0x004EF950 otherwise computes */
    unsigned int level; /* 0x070, record 0x134: starting profession level */
    int attributes[7];  /* 0x074, record 0x138: W8CharacterAttribute::value per attribute */
    int skills[0x29];   /* 0x090, record 0x154: W8CharacterSkill::value_02 per skill */
    unsigned char
        spells[0x72]; /* 0x134, record 0x1f8: one flag per learnable spell, indexed from one */
    unsigned short equipment_present[12]; /* 0x1a6, record 0x26a: zero leaves the slot empty */
    unsigned short equipment_ids[12];     /* 0x1be, record 0x282: Items.dbs index, 0xffff empty */
    unsigned short backpack_present[8];   /* 0x1d6, record 0x29a */
    unsigned short backpack_ids[8];       /* 0x1e6, record 0x2aa */
    signed char gender; /* 0x1f6, record 0x2ba: copied into the character's own sex field */
    short wanted_item_ids_1f7[3];
    unsigned char unknown_1fd[9];
}; /* 0x206, record 0x0c4..0x2c9 */

static_assert(sizeof(W8NpcCharacterTemplate) == 0x206, "W8NpcCharacterTemplate_size_must_be_0x206");

/* One Data\Databases\NPC.DBS record. Only source-consumed fields are modelled
   here; Ghidra owns the wider operational field inventory. */
struct W8NpcDatabaseRecord {
    unsigned short
        version; /* 0x000: two in the corpus; the rule tail loads only when this exceeds 1 */
    /* 0x002: trade-pool stock count; CreateNpcRuntimeNode copies it into the
       runtime state's trade_pool_ca. */
    short trade_pool_002;
    /* 0x004: the wide source name the level-entry rebinding prefixes with an
       underscore to build the NPC's trigger name. */
    wchar_t source_name_004[0x28];
    /* 0x054: monster-bound NPC: no standalone runtime node is created, its
       state comes through the monster binding, and releasing the binding
       marks it unavailable. */
    unsigned char monster_bound_054;
    /* 0x055: gates the owned item-list teardown at 0x0055A5D0, which only
       releases the NPC's stock while this is set. */
    unsigned char owns_stock_055;
    /* 0x056: merchant: still opens dialogue when the disposition band is
       hostile, and item drops go through the trade transcript layout. */
    unsigned char merchant_056;
    /* 0x057: the NPC carries an RPC character and can join a monster group,
       which is what makes the group index on its runtime state meaningful. */
    unsigned char has_group;
    /* 0x058: the NPC's kind. Twenty is the one value a recovered body singles
       out, refusing to trade with it. */
    int kind;
    /* 0x05c: the disposition byte CreateNpcRuntimeNode starts the state with. */
    unsigned char disposition;
    /* 0x05d/0x05e: the signed scales the charm and talk interactions fold into
       the disposition shift; the sub-one path negates the /5 quotient. */
    signed char charm_scale_5d;
    signed char talk_scale_5e;
    /* 0x05f: the faction the NPC belongs to; zero leaves GetNpcDisposition on
       the record's own disposition byte. */
    unsigned char faction_5f;
    /* 0x060: one bit per named-person alias the NPC answers to; read as one
       dword by FindNpcNameOrPlaceQuote. */
    unsigned int name_alias_mask_060;
    /* 0x064: one-based index into g_item_tables selecting the record's item
       table; 0x0050B9E0 copies that table into the runtime state. The zero and
       past-the-end tests compare it signed. */
    int item_table_id;
    /* 0x068: one bit per service the NPC offers, matched against the table at
       0x00619DF8 that pairs each service id with its bit. */
    unsigned int service_flags;
    unsigned char unknown_06c[2];
    /* 0x06e: an allied faction; while a front-rank party member's bound NPC
       belongs to it, GetNpcDisposition pins this NPC's answer at fifty. The
       compare sign-extends it. */
    signed char allied_faction_6e;
    /* 0x06f: the minimum average party level the notice predicate at
       0x0050C870 requires before this NPC's group can be invited. */
    unsigned char min_party_level_6f;
    /* 0x070/0x074: the level this NPC restores its binding at and the
       FindEntityByName key the restore moves its monster to - the default
       pair copied into the runtime state's pending restore. */
    char restore_level;
    unsigned char unknown_071[3];
    char restore_entity_name[0x29];
    unsigned char flag_9d; /* 0x09d: and only when this is clear */
    /* 0x09e: what the NPC is called, unless a fact substitutes another name.
       The wide RPC-character name at 0x0c4 bounds the string extent. */
    char display_name[0x26];
    W8NpcCharacterTemplate character; /* 0x0c4 */
    W8PList* item_stock_rules;        /* 0x2ca: W8NpcItemStockRule* elements */
    /* 0x2ce/0x2d2: the trade price factors. CalculateTradeStackPrice adds the
       bargained adjustment to buy_price_factor when the NPC buys from the
       party and subtracts it from sell_price_factor when the party buys. */
    float buy_price_factor;
    float sell_price_factor;
    /* 0x2d6: one accepted trade-item class per bit, used by 0x0055B290. */
    unsigned int trade_item_class_mask;
    unsigned char unknown_2da[0x10];
    /* 0x2ea: voice-script NPC: picks the VOC_ script/sound prefix over NPC_
       and gates the normal dialogue paths. */
    unsigned char voice_script_2ea;
    /* 0x2eb: the purse the NPC carries; 0x004F8CB0 hands it to AddPartyGold
       when the NPC's monster dies. */
    int gold;
    unsigned char unknown_2ef[0x1a];
}; /* 0x309 */

static_assert(sizeof(W8NpcDatabaseRecord) == 0x309, "W8NpcDatabaseRecord_size_must_be_0x309");

/* One Data\Databases\LEVELS.DBS record. Only the disk and runtime stride is
   established; the leading field is a display name. */
struct W8LevelDatabaseRecord {
    unsigned char unknown_000[0x3c];
    /* 0x3c..0x50: the per-level random-encounter budget parameters, all five
       read by UpdateRandomEncounterBudget and the sixth by the culling pass.
       The reviewed Ghidra type carries the wider operational inventory. */
    int maximum_random_encounters; /* 0x3c */
    int minimum_random_encounters; /* 0x40 */
    int maximum_encounter_budget;  /* 0x44 */
    int minimum_encounter_budget;  /* 0x48 */
    int encounter_budget_period;   /* 0x4c: elapsed-time divisor */
    int encounter_culling_seconds; /* 0x50 */
    /* 0x54: float divisor the surprise transition uses when scaling game time
       and monster-generator duration. */
    float gameplay_time_scale_054;
    unsigned char unknown_058[0x80];
}; /* 0xd8 */

static_assert(sizeof(W8LevelDatabaseRecord) == 0xd8, "W8LevelDatabaseRecord_size_must_be_0xd8");
static_assert(offsetof(W8LevelDatabaseRecord, gameplay_time_scale_054) == 0x54,
              "W8LevelDatabaseRecord_gameplay_time_scale_054_offset");

/* One runtime DATABASES\MONSTERS.DBS record. The size is the tracked disk and
   runtime record size; source-consumed fields are typed here and the reviewed
   Ghidra type owns the wider operational inventory. */
/* The one monster record whose alternate name is used in place of its own;
   both bodies that name a monster test for it, which is why it lives here
   rather than in either of them. */
enum { W8_MONSTER_RECORD_ALTERNATE_NAME = 397 };

/* One slot of W8MonsterRecord::treasure_1c3. type selects direct item (0) or
   item-table (1) drops; the entry only fires when count is nonzero and a
   Random(100) roll stays under chance. */
struct W8MonsterTreasureEntry {
    unsigned char type;     /* 0x00 */
    short count;            /* 0x01: nonzero gates the slot */
    unsigned short item_id; /* 0x03: item id, or table id when type is 1 */
    unsigned char chance;   /* 0x05 */
    W8Dice dice;            /* 0x06: rolled once for the drop count */
};
static_assert(sizeof(W8MonsterTreasureEntry) == 10, "W8MonsterTreasureEntry_must_be_10");

struct W8MonsterTreasureBlock {
    unsigned char unknown_00[0x30];
    W8MonsterTreasureEntry slots[8]; /* 0x30 */
    W8Dice gold_dice;                /* 0x80: rolled once into AddPartyGold */
};
static_assert(sizeof(W8MonsterTreasureBlock) == 0x84, "W8MonsterTreasureBlock_must_be_0x84");

#pragma pack(push, 1)
struct W8EncounterCompanionRecord {
    short species;
    unsigned char chance;
};
#pragma pack(pop)
static_assert(sizeof(W8EncounterCompanionRecord) == 3, "W8EncounterCompanionRecord_size");

struct W8MonsterRecord {
    wchar_t name_00[24]; /* 0x000: suffix after '#' removed at load */
    wchar_t name_30[24]; /* 0x030: suffix after '#' removed at load */
    wchar_t name_60[24]; /* 0x060: suffix after '#' removed at load */
    wchar_t name_90[24]; /* 0x090: suffix after '#' removed at load */
    /* Cosmic Forge's Monster Editor exposes this byte as 'Can open doors'. */
    unsigned char can_open_doors_0c0;
    /* 0x0c1: rolled by the group-attack summon to size the spawned group. */
    W8Dice group_size_dice_0c1;
    W8EncounterCompanionRecord companions_0c5[2];
    /* 0x0cb: the monster's kind. The alchemy-casting rule admits kinds four,
       five and thirteen and no others, which is the only body that reads it. */
    unsigned char kind_0cb;
    /* 0x0cc: selects this monster's row in the name-prefix table at 0x0061E436,
       the same table a character indexes by sex. */
    unsigned char name_group_0cc;
    /* 0x0cd: the NPC record index this monster is bound to, fed to
       GetNpcStateByKind and compared against W8NpcState::name_style; 0xfa
       marks a record with no bound NPC. */
    unsigned char npc_kind_0cd;
    /* 0x0ce: signed stamina regeneration rate (Monster Editor: ST Regen). */
    signed char stamina_regeneration_0ce;
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
    unsigned char unknown_0de[2];
    /* 0x0e0/0x0e1: the percentage chances the AI casts a spell or flees each
       round; a monster with a usable attack ignores them and always tries. */
    unsigned char spell_chance_0e0;
    unsigned char flee_chance_0e1;
    /* 0x0e2: the percentage chance the AI advances on the party each round;
       zero falls back to the behavior byte at +0x1c0. */
    unsigned char advance_chance_0e2;
    /* 0x0e3: Special attack selector; indexes the effect rows used by
       GroupAttacks.cpp and the corresponding display-name table. */
    unsigned char special_attack_kind_0e3;
    unsigned char initiative_0e4; /* 0x0e4: Monster Editor Initiative */
    /* 0x0e5/0x0e6: attacks and swings per round, named by the Combat Attack.cpp
       data-error messages "has 0 ATTACKS/round" and "has 0 SWINGS/round". */
    unsigned char attacks_per_round_0e5;
    unsigned char swings_per_round_0e6;
    /* 0x0e7: the monster's three attacks, named by the Combat Range.cpp
       assertions pMonsterDB->Attack[uiAttack].fHasAttack and uiAttack <
       MAX_MONSTER_ATTACKS, which is what bounds the array at three. */
    W8MonsterAttack attacks[W8_MAX_MONSTER_ATTACKS]; /* 0x0e7 */
    /* 0x14d: the ten spells the AI may cast, zero for none; ChooseMonsterSpell
       weights them by the fixed table at 0x0061CC14. */
    unsigned char spells_14d[10];
    unsigned char attack_body_part_chances_157[5];
    unsigned char special_attack_cooldown_15c;
    signed char evasion_ac_15d;
    /* 0x15e: Monster Editor Constitution selector; retail uses it to choose
       the body-specific hit-location label row. */
    unsigned char constitution_15e;
    /* 0x15f: the percentage of hits that land on each of the seven monster
       hit locations; the total is reported when it falls short of 100. */
    unsigned char hit_location_chances_15f[7];
    /* 0x166/0x16d: the monster's armour per hit location and per attack mode,
       the terms TargetArmorClassAtLocation subtracts on top of evasion_ac_15d. */
    signed char armor_class_by_location[7];
    signed char armor_class_by_attack_mode[9];
    /* 0x176: the monster's own resistance per realm, read alongside the
       gameplay-modifier bonus wherever a character would read
       W8CharacterResistance::total. */
    unsigned char resistances[6];
    /* 0x17c: hit points gained (or lost, when negative) per game minute. */
    signed char hp_regeneration_17c;
    unsigned char unknown_17d[4];
    unsigned int experience_181; /* 0x181: base experience value */
    unsigned char unknown_185[2];
    short record_id_187;       /* 0x187: equals the zero-based database index */
    char cycle_name_189[0x30]; /* 0x189: GrCycle lookup key */
    /* 0x1b9: prefer ranged actions instead of closing/backing off. */
    unsigned char prefer_ranged_actions_1b9;
    float combat_move_range_1ba; /* 0x1ba: designer-facing combat movement range */
    /* 0x1be: blocks the DEAD condition. */
    unsigned char instant_death_immune_1be;
    /* 0x1bf/0x1c0: the Monster Editor's Combat Behaviour and Combat Morale selectors. */
    unsigned char combat_behavior_1bf;
    unsigned char combat_morale_1c0;
    /* 0x1c1: the MIPE monster list only admits records carrying -1 here, and
       stores the value itself as the selected monster index. */
    short editor_index_1c1;
    /* 0x1c3: the monster's treasure table. DropMonsterLoot fires each slot
       whose count is nonzero once Random(100) stays under its chance, rolling
       the slot dice for the drop count; type 0 creates the item id directly,
       type 1 feeds the id to GenerateItemsFromTable. The gold dice lands in
       AddPartyGold. */
    W8MonsterTreasureBlock treasure_1c3;
    unsigned char attack_multiple_targets_247;
    /* 0x248: Monster Editor camouflage rating; retail sight code consumes it. */
    unsigned char camouflage_248;
    unsigned char camouflage_249;
    /* 0x24a: the monster cannot be targeted at all. Every sweep that gathers
       candidates drops it before any other test. */
    unsigned char untargetable_24a;
    unsigned char unknown_24b[4];
    /* 0x24f: effective/combat monster level used by attack, resistance,
       spell and effect formulas. Distinct from the displayed level at 0x251. */
    unsigned char effective_level_24f;
    unsigned char unknown_250;
    /* 0x251: monster level shown by MonsterInfo and related UI. */
    unsigned char display_level_251;
    unsigned char unknown_252;
    int model_index_253;           /* 0x253: selected by 0x004e5b50 */
    int alternate_model_index_257; /* 0x257: alternate selected value */
    int hostility_radius_25b;      /* 0x25b: Minimal Neutrality Distance / Hostility Radius */
    int faction_id_25f;            /* 0x25f: W8Faction value, domain 0..20 */
    int material_263;              /* 0x263: Monster Editor Material selector */
    /* Carved out because LoadMonsterGroup skips every live-group step for a
       record that has it set. */
    unsigned char deleted; /* 0x267 */
    unsigned char significant_kill_268;
    unsigned char unknown_269;
    /* 0x26a: unborn monsters enter the birth/encounter lists before acting. */
    unsigned char unborn_26a;
    unsigned int experience_override_26b; /* 0x26b: nonzero experience override */
    /* 0x26f: the spell-point budget the monster casts out of, before its own
       runtime bonus. Zero is a data error the power-level chooser reports by
       name. */
    int sp_budget;
    unsigned char unknown_273[0x24];
}; /* 0x297 */

static_assert(sizeof(W8MonsterRecord) == 0x297, "W8MonsterRecord_size_must_be_0x297");
static_assert(offsetof(W8MonsterRecord, attack_body_part_chances_157) == 0x157,
              "W8MonsterRecord_attack_body_part_chances_157");
static_assert(offsetof(W8MonsterRecord, effective_level_24f) == 0x24f,
              "W8MonsterRecord_effective_level_24f");
static_assert(offsetof(W8MonsterRecord, display_level_251) == 0x251,
              "W8MonsterRecord_display_level_251");

#pragma pack(pop)

/* Data\Databases race table rows the resistance recalculation reads. A
   resistance_index of -1 terminates a race's list; an adjustment above
   W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS names a character attribute instead of a
   flat amount. */
struct W8RaceResistanceAdjustment {
    int resistance_index;
    int adjustment_or_attribute;
}; /* 0x08 */

struct W8RaceResistanceProfile {
    W8RaceResistanceAdjustment adjustments[6];
}; /* 0x30 */

/* Runtime roots for the database records described above. GameplayDatabase.cpp
   owns their storage; consumers reach it through this one declaration surface
   instead of the gameplay-boundary quarantine. */
extern W8FactDatabaseRecord* g_fact_records;
extern W8LevelDatabaseRecord* g_level_records;
extern W8NpcDatabaseRecord* g_npc_records;
extern W8SpellRuntimeRecord* g_spell_records;
extern unsigned int g_spell_database_version;

#endif
