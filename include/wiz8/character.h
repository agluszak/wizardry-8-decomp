#ifndef WIZ8_CHARACTER_H
#define WIZ8_CHARACTER_H

#include "surrender/srMath.h"
#include "wiz8/gameplay_modifiers.h"
#include "wiz8/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/engine_code/World.h"

struct W8MonsterManagerEntry;

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
struct W8Enchantment {
    int value_00;
    int value_04;
    /* 0x08: the field the topmost-slot scan reads and the one the fatigue path
       consults on slot five. */
    int value_08;
}; /* 0x0c */

/* The game's wide text format: fixed-size UINT16 arrays stored inline in
   records and manipulated through the CRT wide-string functions. Under VC6
   wchar_t is unsigned short, so the two spellings are one type. */

/* One attribute record. The array is indexed by skill id biased by 0x22, so the
   seven attribute ids sit at the top of the skill numbering; only the leading
   value, which IsCharacterSkillAvailable tests against 100, is established. */
struct W8CharacterAttribute {
    unsigned int value; /* 0x00 */
    /* 0x04: the value after equipment and effects. Resistance recalculation
       reads this one, not the base, and only above a threshold of 0x50. */
    unsigned int effective;
    unsigned char unknown_08[0xc];
}; /* 0x14 */

/* One skill record, indexed directly by skill id. PracticeCharacterSkill
   establishes the stride and the leading flag it sets when a skill first
   becomes available; IsCharacterSkillAvailable reads the same flag. */
struct W8CharacterSkill {
    unsigned char flag_00; /* 0x00 */
    unsigned char unknown_01;
    /* 0x02: a second figure the spell-learning ceiling divides by ten, the
       same way the resistances divide `level`. The two are distinct fields of
       one skill, not one field read two ways. */
    unsigned int value_02;
    /* 0x06: the skill's current level. Resistance recalculation divides it by
       ten for skills 28..33 and by five for skill 36, which is what places it. */
    unsigned int level;
    /* 0x0a: the attribute-derived base the level-up reset recomputes and the
       profession-skill assignment scales, before the spent points land on
       value_02. */
    unsigned int base_level_0a;
    unsigned char unknown_0e[0x18];
}; /* 0x26 */

/* One resistance channel. Recalculation rebuilds `base` from scratch each time
   and then derives `total` from it, so the two are a computed pair rather than
   a stored value and a cache. */
struct W8CharacterResistance {
    unsigned int base;  /* 0x00 */
    unsigned int total; /* 0x04: clamped to 100 */
    unsigned char unknown_08[8];
}; /* 0x10 */

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

/* One hand's derived attack block. CalcAttacks walks two at this stride, while
   the equipment refresh helper writes the leading wield kind at 0x00. */
struct W8HandAttack {
    int wield_kind;              /* 0x00 */
    unsigned char in_play;       /* 0x04 */
    int weapon_skill;            /* 0x05, unaligned */
    int combat_skill;            /* 0x09 */
    unsigned int combined_skill; /* 0x0d */
    int attack_score;            /* 0x11 */
    unsigned int attacks;        /* 0x15 */
    int swings;                  /* 0x19 */
    int damage_bonus;            /* 0x1d */
    int hit_bonus;               /* 0x21 */
    int value_25;                /* 0x25 */
    int value_29;                /* 0x29 */
    W8Dice damage_dice;          /* 0x2d */
    unsigned short attack_flags; /* 0x31 */
    int value_33;
    unsigned char unknown_37[2];
    signed char strength_bonus_39;
    unsigned char unknown_3a;
    int value_3b;
    int value_3f;
    unsigned char unknown_43[0x18];
}; /* 0x5b */

/* One 0x11-byte condition record from 0x1817. Function5248A0 reads byte 8 of
   one of the four; the rest is not yet named. */
struct W8CharacterConditionRecord {
    unsigned char unknown_00[8];
    unsigned char value_08;
    unsigned char unknown_09[8];
}; /* 0x11 */
static_assert(sizeof(W8CharacterConditionRecord) == 0x11, "W8CharacterConditionRecord_size");

struct W8Character {
    /* 0x0000: SaveCharacter stamps 1 here before writing the record, so the
       leading dword is a saved-record version rather than runtime state. */
    unsigned int record_version;
    unsigned char in_party; /* 0x0004 */
    /* 0x0005: the character's name, wide, and the stem SaveCharacter formats
       "%ls.CHR" from. The extent below partitions the unknown run up to the
       profession at 0x0069; it is not proven, and only the fact that a wide
       string starts here is. */
    wchar_t name[10];
    wchar_t name_part_2[6]; /* 0x0019: rendered as the parenthesized name */
    unsigned char unknown_0025[0x44];
    /* 0x0069 and 0x006d: iProfession, named by the GameplayCode.cpp:399
       assertion that bounds it against PROF_COUNT, and the profession the
       character started in. The level band subtracts a base only while the two
       agree. */
    W8Profession current_profession;  /* 0x0069 */
    W8Profession original_profession; /* 0x006d */
    int race;                         /* 0x0071: indexes the race resistance table */
    /* 0x0075: zero is male and one is female. The quote lookup names the
       Data\Quotes\PCs files m_ or f_ from it, the item record's two-bit mask
       admits exactly one sex, and the female-only profession at index two
       forces the field to one. */
    W8Gender gender;
    /* 0x0079: looked up from the table at 0x00616604 by gender, race and
       profession together. */
    int table_value_0079;
    /* 0x007d: cleared by DeriveCharacterPersonality004EFA30 in both of its branches and set to -1
       by the character rebuild; all three accesses are four-byte stores. */
    int unknown_007d;
    int personality_0081;                       /* indexes the state-5 descriptor text */
    int voice_0085;                             /* selected by the character voice control */
    unsigned int level;                         /* 0x0089: averaged across occupied slots */
    int profession_levels[W8_PROFESSION_COUNT]; /* 0x008d */
    unsigned char unknown_00c9[0x14];
    /* 0x00dd: the eight-band ladder over the character's level in their
       current profession, and the base subtracted from it while they are still
       in the profession they started in. */
    int level_band;
    int level_band_base;
    W8CharacterAttribute attributes[7]; /* 0x00e5, indexed by skill_id - 0x22 */
    unsigned char unknown_0171[0x28];
    /* 0x199: the attribute points the level-up reset still owes against the
       profession's minimums. It accumulates as a negative debt and is drawn
       back down one point at a time once the pool is positive. */
    int attribute_point_deficit_0199;
    W8CharacterSkill skills[0x29]; /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x23a];
    /* 0x09ed..0x09f8: experience, the goal for the next level, and the goal
       the previous level had. A character is ready to advance once the first
       reaches the second. */
    unsigned int experience;
    unsigned int experience_goal;
    unsigned int experience_previous_goal;
    int value_09f9;
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
    int condition_turns[W8_CONDITION_COUNT]; /* 0x0a01 */
    unsigned char unknown_0a51[0x14];
    W8Enchantment enchantments[8]; /* 0x0a65 */
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
    int hp_max;                        /* 0x0b0d */
    unsigned int hp_current;           /* 0x0b11 */
    int hp_adjustment;                 /* 0x0b15 */
    int stamina_max;                   /* 0x0b19 */
    int stamina;                       /* 0x0b1d */
    unsigned int fatigue_penalty_0b21; /* 0x0b21: taken off the stamina ceiling */
    /* 0x0b25 and 0x0b45: iSPMax and iSPLeft, one per spell realm, named by the
       Health Stamina Mana.cpp:1067 assertion pPC->iSPLeft[uiRealm] and bounded
       at six realms by the total the party-wide restore accumulates. */
    int sp_max[W8_SPELL_REALM_COUNT]; /* 0x0b25 */
    unsigned char unknown_0b3d[8];
    int sp_left[W8_SPELL_REALM_COUNT]; /* 0x0b45 */
    unsigned char unknown_0b5d[0xc];
    /* 0x0b69, 0x0b71 and 0x0b79: the per-tick regeneration rates rebuilt from
       the pool ceilings, one each for hit points and stamina and one per spell
       realm at a two-float stride. */
    float health_regen_rate_0b69;
    unsigned char unknown_0b6d[4];
    float stamina_regen_rate_0b71;
    unsigned char unknown_0b75[4];
    float spell_regen_rates_0b79[12];
    unsigned char unknown_0ba9[0x10];
    int inventory_weight;     /* 0x0bb9 */
    int party_weight_share;   /* 0x0bbd */
    int total_carried_weight; /* 0x0bc1 */
    int carrying_capacity;    /* 0x0bc5; displayed divided by 10 */
    /* 0x0bc9: the load category, zero through four, which scales what an
       action costs in fatigue. FatigueCharacter's error text calls it that. */
    int load_category;
    /* 0x0bcd: one entry per spell. CanCharacterUseItem refuses a spell-source
       item whose spell already reads one here, so one is the learned state. */
    int spell_learned[136]; /* 0x0bcd */
    /* 0x0ded: indexed by skill_id. For the six realm skills it counts the
       spells known in that realm, which is what makes the skill available at
       all - LearnSpell bumps the entry and IsCharacterSkillAvailable reads it.
       LearnSpell also writes a recomputed figure into entry 36, which is a
       real skill id but not a count; that disagreement is recorded rather than
       resolved. */
    unsigned int skill_unlocks[0x25];
    /* 0x0e81: rebuilt by CalcInitiative from level, speed, senses, the
       initiative skill and the current load category. */
    int initiative;
    /* 0x0e85..0x0e8c: the two armor-class summaries rebuilt after the
       location values below. */
    int armor_class_total;
    int armor_class_average;
    /* 0x0e8d: thirteen armor-class components. CalcArmorClasses clears the
       whole run before applying equipment, traits, load and fatigue. */
    int armor_class_components[13];
    int armor_class_by_location[5]; /* 0x0ec1 */
    unsigned char unknown_0ed5[4];
    /* 0x0ed9: a percentage taken off incoming damage, the character's
       counterpart of the monster's own at 0x1e1. */
    int damage_reduction;
    W8CharacterResistance resistances[W8_RESISTANCE_COUNT]; /* 0x0edd */
    unsigned char unknown_0f3d[0x20];
    /* 0x0f5d: the twelve worn/held slots, indexed by the same slot numbering
       GetItemDefaultEquipSlot answers and GetItemEquipSlotMask sets bits for.
       Slots six and seven are the primary hands and eight and nine the
       alternate pair, which is what the two-handed and off-hand tests read. */
    W8ItemInstance equipment[12]; /* 0x0f5d */
    unsigned char unknown_0fed[0x3c];
    /* 0x1029: the eight per-character carried slots. GetOriginOfCharacterItem
       reports this array as origin zero and the equipment array as origin one. */
    W8ItemInstance backpack[8]; /* 0x1029 */
    unsigned char unknown_1089[0xc0];
    W8HandAttack hand_attacks[2]; /* 0x1149 */
    unsigned char unknown_11ff[0xb6];
    unsigned char dual_wielding; /* 0x12b5 */
    unsigned char unknown_12b6[0x3e8];
    /* 0x169e: the fatigue band, zero through four, recomputed from the stamina
       fraction whenever it moves; a change re-runs the armour class pass. */
    int fatigue_band;
    /* 0x16a2: a persistent 0x67-byte modifier source the party-effect rebuild
       folds into the derived block; what writes it is not yet recovered. */
    W8GameplayModifierBlock unknown_16a2;
    /* 0x1709: the equipment bonus block 0x0050E980 accumulates from the worn
       items and 0x0050F030 folds into the derived block at 0x1770. */
    W8GameplayModifierBlock equipment_bonus_1709;
    /* 0x1770: the derived modifier block the rebuild clears and folds the
       equipment, persistent and party blocks into. */
    W8GameplayModifierBlock bonus_1770;
    /* 0x17d7: the CamPos record GetWorldCameraState writes and recall restores.
       The cross-level path copies the whole 0x3c bytes onto
       W8GlobalStatus::pending_move_location. */
    W8WorldCameraState saved_location; /* 0x17d7 */
    /* 0x1813: which level that anchor belongs to. The recall compares it
       against g_status_685170.current_level and takes a different path when they differ. */
    int saved_level;                               /* 0x1813 */
    W8CharacterConditionRecord conditions_1817[4]; /* 0x1817 .. 0x185a */
    /* 0x185b: the deep-fatigue effect is already on this character, which is
       what stops FatigueCharacter re-applying it every turn. */
    unsigned char deep_fatigue_applied;
    /* 0x185c: one cost per skill id 0x18..0x1b, read by the profession-change
       cost diff with the skill id biased down. */
    unsigned char skill_costs_185c[4];
    unsigned char unknown_1860;
    /* 0x1861: the anchor above has been set. Recall does nothing without it. */
    unsigned char has_saved_location;
}; /* 0x1862 */

struct W8SkillAttributes {
    int category;
    int unknown_04;
    int unknown_08;
    int unknown_0c;
};

#pragma pack(pop)

extern W8RaceResistanceProfile g_race_resistance_profiles[];
extern int g_profession_skill_availability[0x29][W8_PROFESSION_COUNT];
extern int g_profession_bonus_skills[W8_PROFESSION_COUNT];
extern W8SkillAttributes g_skill_attributes[0x29];

/* Profession and race trait sets consulted by CharacterHasTrait00547940. Each entry is
   only its id list: three profession abilities, five race abilities. */
struct W8ProfessionAbilitySet {
    int ability_ids[3];
};

struct W8RaceAbilitySet {
    int ability_ids[5];
};

extern W8ProfessionAbilitySet g_profession_abilities[W8_PROFESSION_COUNT];
extern W8RaceAbilitySet g_race_abilities[16];
extern int g_profession_skills[W8_PROFESSION_COUNT][4];
extern int g_profession_magic_level_offsets[W8_PROFESSION_COUNT];
extern float g_profession_hit_point_factors[W8_PROFESSION_COUNT];

int GetNextCharacter(int require_primary, int require_secondary, int previous_slot);
int RPCPtrToPCSlot(const W8MonsterManagerEntry* rpc);
void StripMonsterNameSuffix(W8WideChar* name);
unsigned int CharacterPointerToPartySlot(const W8Character* character);
unsigned char SetCharacterCondition(int party_slot, int condition, int duration, int argument,
                                    char value_5, char value_6);
bool IsPartyCharacterPointer(const W8Character* character);
bool IsCharacterReadyToAdvance(int party_slot);
int GetProfessionCasterLevel(W8Character* character, int profession_id);
bool IsCharacterSkillAvailable(W8Character* character, unsigned int skill_id,
                               const unsigned char* expert_realm_flags);
void RecalculateCharacterResistances(W8Character* character);
int SumCharacterSpellPoints(const W8Character* character);

struct W8PortraitDescriptor {
    int group;
    int unknown_004;
    int unknown_008;
    int render_mode;
};

static_assert(sizeof(W8PortraitDescriptor) == 0x10, "W8PortraitDescriptor_size");

extern W8PortraitDescriptor g_portrait_descriptors_6483d0[80];

int GetCharacterRealmSpellPoints(const W8Character* character, int realm);
void GetCharacterHandDamageDice(const W8Character* character, int hand, W8Dice* dice);
int GetCharacterHandDamageBonus(const W8Character* character, int hand);

void InvalidateAndRecalculateCharacterClassData00558610(W8Character* character);
bool CharacterHasTrait00547940(const W8Character* character, int trait);

void DeriveCharacterPersonality004EFA30(W8Character* character);
int ComputeRealmSkillDebt(W8Character* original, W8Character* edited);
void Function4EF7E0(W8Character*, W8Character*, int);
int Function558640(W8Character*);
unsigned char Function5586B0(W8Character*);
void Function5218C0(W8Character*);
void Function52F2C0(W8Character* character);
void ApplyCharacterEffect(W8Character* character, int effect, int arg_3, int arg_4, int arg_5);
int CalcRangeCategoryToTarget(const W8Character* character, int hand);
int Function51C5A0(W8Character* character, int item_id);
bool RecalculateCarriedWeight(W8Character* character);
void CalcXPGoal(W8Character* character);
int GetSpellbookForSpell(const W8Character* character, int spell_id, int a, int b, int c);
int RebuildRealmSpellPointCeilings0052A540(W8Character* character);
unsigned char CharacterHasCondition(const W8Character* character, int condition);

/* Character generation and skill/encumbrance helpers whose bodies were split
   across CharGeneration.cpp, character_skills.cpp and the encumbrance unit. */
bool RecalculateCarryingCapacity004EDC10(W8Character* character);
/* 0x004ED9D0: the full derived-stat recompute, and the two equipment-bonus
   passes an NPC character's initialization runs. */
void Function4ED9D0(W8Character* character);
void AccumulateEquipmentModifiers(W8Character* character, W8GameplayModifierBlock* equipment_bonus);
void RebuildCharacterModifierBlock(W8Character* character);
void Function52A3E0(W8Character* character);
void Function52A500(W8Character* character);
void InitializeSkillBaseLevels00553C90(W8Character* character);
void RefreshCharacterSkillAvailability00553CD0(W8Character* character);
unsigned int GetSkillQuarterValue00553EE0(W8Character* character, int skill_id);

#endif
