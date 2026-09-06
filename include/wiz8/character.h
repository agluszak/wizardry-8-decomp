#ifndef WIZ8_CHARACTER_H
#define WIZ8_CHARACTER_H

#include "surrender/srMath.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/saved_location.h"

#pragma pack(push, 1)

/* The eighteen conditions a character can be under, indexed directly into
   W8Character::condition_turns. Only the ones a recovered body names are
   spelled out; the rest keep their numbers. */
enum {
    /* Twenty entries: the per-character copier walks all twenty, while the
       sweep that lifts everything stops at eighteen because the last two are
       not the kind a rest clears. */
    W8_CONDITION_COUNT = 20,
    W8_CONDITION_CLEARABLE_COUNT = 18,
    W8_CONDITION_FATIGUE_DOUBLED = 2,
    W8_CONDITION_LOAD_EASED = 5,
    /* Seven is the one condition that carries a second value alongside its
       duration, which both copiers special-case. */
    W8_CONDITION_WITH_ARGUMENT = 7,
    W8_CONDITION_SPELLCASTING_BLOCKED = 8,
    W8_CONDITION_HOSTILE = 0xd,
    W8_CONDITION_EXHAUSTED = 0x11,
    W8_CONDITION_EQUIPMENT_UNLOCKED = 18,
    /* The duration that means "until lifted". */
    W8_CONDITION_INDEFINITE = 9999
};

/* One enchantment slot. Both a character and a monster carry eight of them,
   and both clear a slot by zeroing all three dwords at once. */
typedef struct W8Enchantment {
    int value_00;
    int value_04;
    /* 0x08: the field the topmost-slot scan reads and the one the fatigue path
       consults on slot five. */
    int value_08;
} W8Enchantment;                          /* 0x0c */

/* The game's wide text format: fixed-size UINT16 arrays stored inline in
   records and manipulated through the CRT wide-string functions. Under VC6
   wchar_t is unsigned short, so the two spellings are one type. */

/* One attribute record. The array is indexed by skill id biased by 0x22, so the
   seven attribute ids sit at the top of the skill numbering; only the leading
   value, which IsCharacterSkillAvailable tests against 100, is established. */
typedef struct W8CharacterAttribute {
    unsigned int value;                   /* 0x00 */
    /* 0x04: the value after equipment and effects. Resistance recalculation
       reads this one, not the base, and only above a threshold of 0x50. */
    unsigned int effective;
    unsigned char unknown_08[0xc];
} W8CharacterAttribute;                   /* 0x14 */

/* One skill record, indexed directly by skill id. PracticeCharacterSkill
   establishes the stride and the leading flag it sets when a skill first
   becomes available; IsCharacterSkillAvailable reads the same flag. */
typedef struct W8CharacterSkill {
    unsigned char flag_00;                /* 0x00 */
    unsigned char unknown_01;
    /* 0x02: a second figure the spell-learning ceiling divides by ten, the
       same way the resistances divide `level`. The two are distinct fields of
       one skill, not one field read two ways. */
    unsigned int value_02;
    /* 0x06: the skill's current level. Resistance recalculation divides it by
       ten for skills 28..33 and by five for skill 36, which is what places it. */
    unsigned int level;
    unsigned char unknown_0a[0x1c];
} W8CharacterSkill;                       /* 0x26 */

/* One resistance channel. Recalculation rebuilds `base` from scratch each time
   and then derives `total` from it, so the two are a computed pair rather than
   a stored value and a cache. */
typedef struct W8CharacterResistance {
    unsigned int base;                    /* 0x00 */
    unsigned int total;                   /* 0x04: clamped to 100 */
    unsigned char unknown_08[8];
} W8CharacterResistance;                  /* 0x10 */

/* The six realms a spell point pool is kept per are W8SpellRealm's, declared
   with the spell record in wiz8/layouts/gameplay_databases.h. */

enum {
    W8_RESISTANCE_COUNT = 6,
    /* The six skills whose level feeds the matching resistance, and the one
       whose presence adds a flat bonus to every one of them. */
    W8_FIRST_RESISTANCE_SKILL = 28,
    W8_RESISTANCE_BONUS_SKILL = 36,
    /* A race adjustment at or below this is a flat amount; above it, it selects
       a character attribute by index biased this far. */
    W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS = 1000
};

/* One hand's attack block. The extent is the stride between the two, and only
   the two leading fields are established. */
typedef struct W8HandAttack {
    unsigned char in_play;                /* 0x00 */
    int weapon_skill;                     /* 0x01, unaligned */
    unsigned char unknown_05[0xc];
    /* 0x11: what the hand's attack is worth, read only once the hand has a
       range to the target at all. */
    int attack_value;
    unsigned char unknown_15[0x46];
} W8HandAttack;                           /* 0x5b */

typedef struct W8Character {
    /* 0x0000: SaveCharacter stamps 1 here before writing the record, so the
       leading dword is a saved-record version rather than runtime state. */
    unsigned int record_version;
    unsigned char in_party;              /* 0x0004 */
    /* 0x0005: the character's name, wide, and the stem SaveCharacter formats
       "%ls.CHR" from. The extent below partitions the unknown run up to the
       profession at 0x0069; it is not proven, and only the fact that a wide
       string starts here is. */
    wchar_t name[10];
    wchar_t name_part_2[6];              /* 0x0019: rendered as the parenthesized name */
    unsigned char unknown_0025[0x44];
    /* 0x0069 and 0x006d: iProfession, named by the GameplayCode.cpp:399
       assertion that bounds it against PROF_COUNT, and the profession the
       character started in. The level band subtracts a base only while the two
       agree. */
    int current_profession;               /* 0x0069 */
    int original_profession;              /* 0x006d */
    int race;                             /* 0x0071: indexes the race resistance table */
    int faction;                          /* 0x0075: compared against the caller's faction */
    /* 0x0079: looked up from the table at 0x00616604 by faction, race and
       profession together. Note that the index multiplies 0x0075 by sixteen
       against an eleven-race domain, so either that field is not the faction
       the disposition code reads or the table is sparse; the disagreement is
       recorded rather than resolved. */
    int table_value_0079;
    unsigned char unknown_007d[4];
    int personality_0081;               /* indexes the state-5 descriptor text */
    unsigned char unknown_0085[4];
    unsigned int level;                   /* 0x0089: averaged across occupied slots */
    int profession_levels[15];            /* 0x008d */
    unsigned char unknown_00c9[0x14];
    /* 0x00dd: the eight-band ladder over the character's level in their
       current profession, and the base subtracted from it while they are still
       in the profession they started in. */
    int level_band;
    int level_band_base;
    W8CharacterAttribute attributes[7];   /* 0x00e5, indexed by skill_id - 0x22 */
    unsigned char unknown_0171[0x2c];
    W8CharacterSkill skills[0x29];        /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x23a];
    /* 0x09ed..0x09f8: experience, the goal for the next level, and the goal
       the previous level had. A character is ready to advance once the first
       reaches the second. */
    unsigned int experience;
    unsigned int experience_goal;
    unsigned int experience_previous_goal;
    unsigned char unknown_09f9[4];
    /* 0x09fd: how many times this character has died. */
    int death_count_09fd;
    /* 0x0a01: one entry per condition, holding how long it has left to run;
       W8_CONDITION_INDEFINITE means until something lifts it. The exhausted
       condition the fatigue path calls 0x11 lands exactly on element
       seventeen, which is what fixes base and stride, and the monster carries
       the identical array at its own 0x57 with the same indices meaning the
       same things. Several entries were read individually before this array
       explained them: two doubles the fatigue an action costs, eight blocks
       spellcasting, eighteen unlocks bound equipment. */
    int condition_turns[W8_CONDITION_COUNT];   /* 0x0a01 */
    unsigned char unknown_0a51[0x14];
    W8Enchantment enchantments[8];             /* 0x0a65 */
    unsigned char unknown_0ac5[0x3c];
    /* 0x0b01 gates party-member selection alongside hp_current: a slot is
       eligible when it still has hit points and this is under 0x12, and a
       second tier tests it against 0x0f. It is unsigned - the canonical
       compares are JB/JBE, not JL/JE - but its meaning is not established. */
    unsigned int unknown_0b01;
    /* 0x0b05: the highest enchantment slot still in use, recomputed by
       scanning down from the last one whenever a slot is cleared. */
    int enchantment_top;
    /* 0x0b09: the argument the seventh condition carries. */
    int condition_argument;
    /* 0x0b0d..0x0b20: the two pools with a ceiling each, plus the adjustment
       damage is booked against before hit points are recalculated. A character
       whose hp_current is zero is treated as out of the fight everywhere. */
    int hp_max;                           /* 0x0b0d */
    unsigned int hp_current;              /* 0x0b11 */
    int hp_adjustment;                    /* 0x0b15 */
    int stamina_max;                      /* 0x0b19 */
    int stamina;                          /* 0x0b1d */
    unsigned char unknown_0b21[4];
    /* 0x0b25 and 0x0b45: iSPMax and iSPLeft, one per spell realm, named by the
       Health Stamina Mana.cpp:1067 assertion pPC->iSPLeft[uiRealm] and bounded
       at six realms by the total the party-wide restore accumulates. */
    int sp_max[W8_SPELL_REALM_COUNT];     /* 0x0b25 */
    unsigned char unknown_0b3d[8];
    int sp_left[W8_SPELL_REALM_COUNT];    /* 0x0b45 */
    unsigned char unknown_0b5d[0x5c];
    int inventory_weight;               /* 0x0bb9 */
    int party_weight_share;              /* 0x0bbd */
    int total_carried_weight;            /* 0x0bc1 */
    int carrying_capacity;               /* 0x0bc5; displayed divided by 10 */
    /* 0x0bc9: the load category, zero through four, which scales what an
       action costs in fatigue. FatigueCharacter's error text calls it that. */
    int load_category;
    /* 0x0bcd: one entry per spell. CanCharacterUseItem refuses a spell-source
       item whose spell already reads one here, so one is the learned state. */
    int spell_learned[136];               /* 0x0bcd */
    /* 0x0ded: indexed by skill_id. For the six realm skills it counts the
       spells known in that realm, which is what makes the skill available at
       all - LearnSpell bumps the entry and IsCharacterSkillAvailable reads it.
       LearnSpell also writes a recomputed figure into entry 36, which is a
       real skill id but not a count; that disagreement is recorded rather than
       resolved. */
    unsigned int skill_unlocks[0x29];
    unsigned char unknown_0e91[0x48];
    /* 0x0ed9: a percentage taken off incoming damage, the character's
       counterpart of the monster's own at 0x1e1. */
    int damage_reduction;
    W8CharacterResistance resistances[W8_RESISTANCE_COUNT]; /* 0x0edd */
    unsigned char unknown_0f3d[0x20];
    /* 0x0f5d: the twelve worn/held slots, indexed by the same slot numbering
       GetItemDefaultEquipSlot answers and GetItemEquipSlotMask sets bits for.
       Slots six and seven are the primary hands and eight and nine the
       alternate pair, which is what the two-handed and off-hand tests read. */
    W8ItemInstance equipment[12];         /* 0x0f5d */
    unsigned char unknown_0fed[0x3c];
    /* 0x1029: the eight per-character carried slots. GetOriginOfCharacterItem
       reports this array as origin zero and the equipment array as origin one. */
    W8ItemInstance backpack[8];           /* 0x1029 */
    unsigned char unknown_1089[0xc4];
    /* 0x114d: one block per hand. Only the leading flag - the hand is in play -
       and the weapon skill the attack setup stamps into it are established. */
    W8HandAttack hand_attacks[2];         /* 0x114d */
    unsigned char unknown_1203[0x49b];
    /* 0x169e: the fatigue band, zero through four, recomputed from the stamina
       fraction whenever it moves; a change re-runs the armour class pass. */
    int fatigue_band;
    unsigned char unknown_16a2[0xd5];
    signed char resistance_bonus_all;     /* 0x1777: added to every resistance */
    unsigned char unknown_1778[0x34];
    signed char resistance_bonus[W8_RESISTANCE_COUNT];      /* 0x17ac */
    unsigned char unknown_17b2[3];
    /* 0x17b5: the character is out of the formation, which is what the
       front-rank counts skip. */
    unsigned char out_of_formation;
    unsigned char unknown_17b6[0x21];
    /* 0x17d7: where the character was last anchored, restored by the recall
       effect in Magic Effects.cpp. Its extent is proven rather than assumed:
       the cross-level path copies exactly 0x3c bytes from here with one
       rep movsd, which lands precisely on the level id below. */
    W8SavedLocation saved_location;      /* 0x17d7 */
    /* 0x1813: which level that anchor belongs to. The recall compares it
       against g_status_685170.current_level and takes a different path when they differ. */
    int saved_level;                     /* 0x1813 */
    unsigned char unknown_1817[0x44];
    /* 0x185b: the deep-fatigue effect is already on this character, which is
       what stops FatigueCharacter re-applying it every turn. */
    unsigned char deep_fatigue_applied;
    unsigned char unknown_185c[5];
    /* 0x1861: the anchor above has been set. Recall does nothing without it. */
    unsigned char has_saved_location;
} W8Character;                           /* 0x1862 */

typedef struct W8RPCSlot {
    unsigned char opaque[0x118];
} W8RPCSlot;

#pragma pack(pop)

extern "C" {

extern W8RaceResistanceProfile g_race_resistance_profiles[];
extern int g_profession_skill_availability[0x29][15];
extern int g_profession_bonus_skills[15];

/* Profession and race trait sets consulted by Function547940. Each entry is
   only its id list: three profession abilities, five race abilities. */
typedef struct W8ProfessionAbilitySet {
    int ability_ids[3];
} W8ProfessionAbilitySet;

typedef struct W8RaceAbilitySet {
    int ability_ids[5];
} W8RaceAbilitySet;

extern W8ProfessionAbilitySet g_profession_abilities[15];
extern W8RaceAbilitySet g_race_abilities[11];
extern int g_profession_skills[15][4];
extern int g_profession_magic_level_offsets[15];
extern float g_profession_hit_point_factors[15];

int GetNextCharacter(
    int require_primary, int require_secondary, int previous_slot);
int RPCPtrToPCSlot(const W8RPCSlot* rpc);
void StripMonsterNameSuffix(W8WideChar* name);
unsigned int CharacterPointerToPartySlot(const W8Character* character);
unsigned char SetCharacterCondition(
    int party_slot,
    int condition,
    int duration,
    int argument,
    char value_5,
    char value_6);
unsigned char IsPartyCharacterPointer(const W8Character* character);
bool IsCharacterReadyToAdvance(int party_slot);
int GetProfessionCasterLevel(W8Character* character, int profession_id);
unsigned char IsCharacterSkillAvailable(
    W8Character* character,
    unsigned int skill_id,
    const unsigned char* expert_realm_flags);
int SumCharacterSpellPoints(const W8Character* character);

}

#endif
