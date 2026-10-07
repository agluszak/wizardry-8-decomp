#ifndef WIZ8_LAYOUTS_CHARACTER_H
#define WIZ8_LAYOUTS_CHARACTER_H

#include "wiz8/attack_modes.h"
#include "wiz8/character_skills.h"
#include "wiz8/conditions.h"
#include "wiz8/load_category.h"
#include "surrender/srMath.h"
#include "wiz8/gameplay_modifiers.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/world.h"

#pragma pack(push, 1)

/* One enchantment slot. Both a character and a monster carry eight of them,
   and both clear a slot by zeroing all three dwords at once. */
struct W8Enchantment {
    /* 0x00: the enchantment's power; a slot is only overwritten by a higher
       (unsigned) power. Slot three also spends it as a charge count -
       Combat Attack decrements it per hit and clears the slot at zero. */
    unsigned int power;
    /* 0x04: a percentage. */
    unsigned short percent;
    /* 0x06: the percent-scaled magnitude: a dice roll times the power, raised
       by percent. The stamina path drains
       it as the absorb pool. */
    unsigned short magnitude;
    /* 0x08: turns left; the fatigue path consults it on slot five. */
    unsigned int turns;
}; /* 0x0c */

/* The game's wide text format: fixed-size UINT16 arrays stored inline in
   records and manipulated through the CRT wide-string functions. */

/* Base attributes and their effective values after equipment and effects. */
struct W8CharacterAttribute {
    unsigned int base; /* 0x00 */
    unsigned int effective;
    int change_counter;
    unsigned char unknown_0c[8];
}; /* 0x14 */

/* One skill record, indexed directly by skill id. The leading flag is set
   when a skill first becomes available. */
struct W8CharacterSkill {
    bool active; /* 0x00: skill slot in use */
    /* 0x01: set by ResetSkillContribution; never read. */
    bool reset_flag;
    /* 0x02: invested points. Spellbook selection and skill-increase notices
       read these; the spell-learning ceiling divides them by ten. */
    unsigned int points;
    /* 0x06: current level after profession/modifier adjustments. Spell pricing
       and resistance recalculation read this instead of the invested points. */
    unsigned int level;
    /* 0x0a: the attribute-derived base the level-up reset recomputes and the
       profession-skill assignment scales, before the spent points land on
       value_02. */
    unsigned int base_level;
    /* 0x0e: successful increase rolls; every eighth one raises value_02 by one
       and resets the count - PracticeCharacterSkill's practice tally. */
    unsigned int practice_count;
    /* 0x12: set when practice raises value_02; the flag the skill-increase
       notices key off. */
    bool improved;
    bool available;
    unsigned char unknown_14[0x12];
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

    /* A race adjustment at or below this is a flat amount; above it, it selects
       a character attribute by index biased this far. */
    W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS = 1000
};

/* One hand's derived attack block - the elements of W8Character::Hand. */
enum { HOLDS_NOTHING = 0 };
struct W8HandAttack {
    int uiHolds;                 /* 0x00 */
    bool in_play;                /* 0x04 */
    W8Skill weapon_skill;        /* 0x05, unaligned */
    W8Skill combat_skill;        /* 0x09 */
    unsigned int combined_skill; /* 0x0d */
    int attack_score;            /* 0x11 */
    unsigned int attacks;        /* 0x15 */
    int swings;                  /* 0x19 */
    int damage_bonus;            /* 0x1d */
    int hit_bonus;               /* 0x21 */
    int attack_bonus;            /* 0x25: attack-score term, paired with modifier attack_bonus */
    int damage_percent; /* 0x29: percent damage multiplier, paired with modifier damage_percent */
    W8Dice damage_dice; /* 0x2d */
    unsigned short attack_flags; /* 0x31 */
    unsigned char
        condition_chances[W8_ATTACK_EFFECT_COUNT]; /* 0x33: unarmed condition probabilities */
    unsigned char unknown_43[0x18];
}; /* 0x5b */

/* One condition record. GetConditionRecordFlag reads byte 8 of one of the
   four; the condition-0x13 fold reads record one's leading dwords as the level
   the binding was made on and the bound monster's id. */
struct W8CharacterConditionRecord {
    int level_acquired; /* 0x00: level the condition attached on */
    int source_monster; /* 0x04: monster id/location the dependence is bound to */
    bool active;
    unsigned char unknown_09[8];
}; /* 0x11 */
static_assert(sizeof(W8CharacterConditionRecord) == 0x11, "W8CharacterConditionRecord_size");

struct W8Character {
    /* 0x0000: SaveCharacter stamps 1 here before writing the record, so the
       leading dword is a saved-record version rather than runtime state. */
    unsigned int record_version;
    bool fInParty; /* 0x0004 */
    /* 0x0005: the character's name, also the stem of its "%ls.CHR" file. */
    wchar_t name[10];
    wchar_t name_part_2[6]; /* 0x0019: rendered as the parenthesized name */
    unsigned char unknown_0025[0x44];
    /* 0x0069 and 0x006d: the current profession and the one the character
       started in. The level band subtracts a base only while the two agree. */
    W8Profession iProfession;         /* 0x0069 */
    W8Profession original_profession; /* 0x006d */
    /* 0x0071: indexes the race resistance table. */
    W8Race iRace;
    /* 0x0075: zero is male and one is female. The quote lookup names the
       Data\Quotes\PCs files m_ or f_ from it, the item record's two-bit mask
       admits exactly one sex, and the female-only profession at index two
       forces the field to one. */
    W8Gender gender;
    /* 0x0079: the portrait/catalog index. Looked up by gender, race and
       profession together, drawn through
       DrawCatalogImage(-14, ...) and RenderPartyPortrait, and reassigned from
       g_portrait_groups[].portraits[] when the portrait changes. */
    int portrait_index;
    /* 0x007d: cleared by DeriveCharacterPersonality in both of its branches and set to -1
       by the character rebuild; all three accesses are four-byte stores. */
    int unknown_007d;
    int personality; /* indexes the state-5 descriptor text */
    int voice;       /* selected by the character voice control */
    /* 0x0089: the experience level; averaged across occupied slots. */
    unsigned int uiExpLevel;
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
    int attribute_point_deficit;
    W8CharacterSkill skills[0x29]; /* 0x019d, indexed by skill_id */
    unsigned char unknown_07b3[0x23a];
    /* 0x09ed..0x09f8: experience, the goal for the next level, and the goal
       the previous level had. A character is ready to advance once the first
       reaches the second. */
    unsigned int experience;
    unsigned int experience_goal;
    unsigned int experience_previous_goal;
    /* 0x09f9: this character's confirmed kills, shown on the camp stats
       page and imported from the Wiz7 record. */
    int kill_count;
    /* 0x09fd: how many times this character has died. */
    int death_count;
    /* 0x0a01: one entry per condition, holding how long it has left to run;
       W8_CONDITION_INDEFINITE means until something lifts it. The monster
       carries the identical array. */
    unsigned int uiCondition[W8_CONDITION_COUNT]; /* 0x0a01 */
    unsigned char unknown_0a51[0x14];
    W8Enchantment enchantments[8]; /* 0x0a65 */
    unsigned char unknown_0ac5[0x3c];
    /* 0x0b01: the highest currently-set condition index, rescanned from
       uiCondition[0x13] downward whenever a condition is
       lifted. Zero is either "none set" or condition zero, matching the
       monster copy at W8MonsterInfo::highest_condition. Thresholds are the
       condition ids themselves: below HOSTILE for rest/formation, below
       DEAD for ordinary party eligibility. */
    W8Condition highest_condition;
    /* 0x0b05: the highest enchantment slot still in use, recomputed by
       scanning down from the last one whenever a slot is cleared. */
    W8EnchantmentSlot enchantment_top;
    /* 0x0b09: the argument COND_POISONED carries (poison strength). */
    int condition_argument;
    /* 0x0b0d..0x0b20: the two pools with a ceiling each, plus the adjustment
       damage is booked against before hit points are recalculated. A character
       whose hp_current is zero is treated as out of the fight everywhere. */
    int uiHPMax;                  /* 0x0b0d */
    unsigned int hp_current;      /* 0x0b11 */
    int hp_adjustment;            /* 0x0b15 */
    int uiStaminaMax;             /* 0x0b19 */
    int stamina;                  /* 0x0b1d */
    unsigned int fatigue_penalty; /* 0x0b21: taken off the stamina ceiling */
    /* 0x0b25 and 0x0b45: the spell-point pools, one per spell realm. */
    int sp_max[W8_SPELL_REALM_COUNT]; /* 0x0b25 */
    unsigned char unknown_0b3d[8];
    int iSPLeft[W8_SPELL_REALM_COUNT]; /* 0x0b45 */
    unsigned char unknown_0b5d[8];
    /* 0x0b65: game minutes until the alchemist (trait MAKE_POTIONS) may brew
       again; the aging tick counts it down outside surprise, and a successful
       brew restarts it at 0x168. */
    unsigned int potion_brew_cooldown;
    /* 0x0b69, 0x0b71 and 0x0b79: the per-tick regeneration rates rebuilt from
       the pool ceilings, one each for hit points and stamina and one per spell
       realm at a two-float stride. */
    float health_regen_rate;
    /* 0x0b6d: fractional hit-point regeneration remainder carried between
       ticks; reset to zero when hit points reach the ceiling. */
    float health_regen_accumulator;
    float stamina_regen_rate;
    /* 0x0b75: fractional stamina-regeneration remainder carried between
       ticks; reset to zero when stamina reaches the ceiling. */
    float stamina_regen_accumulator;
    float spell_regen_rates[12];
    unsigned char unknown_0ba9[0x10];
    unsigned int inventory_weight;     /* 0x0bb9 */
    unsigned int party_weight_share;   /* 0x0bbd */
    unsigned int total_carried_weight; /* 0x0bc1 */
    unsigned int carrying_capacity;    /* 0x0bc5; displayed divided by 10 */
    /* 0x0bc9: the load category, zero through four, which scales what an
       action costs in fatigue. FatigueCharacter's error text calls it that. */
    W8LoadCategory load_category;
    /* 0x0bcd: one entry per spell. CanCharacterUseItem refuses a spell-source
       item whose spell already reads one here, so one is the learned state. */
    int spell_learned[136]; /* 0x0bcd */
    /* 0x0ded: indexed by skill_id. For the six realm skills it counts the
       spells known in that realm, which is what makes the skill available at
       all - LearnSpell bumps the entry and IsCharacterSkillAvailable reads it.
       LearnSpell also writes a recomputed figure into entry 36, which is a
       real skill id but not a count. */
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
    unsigned char flags0[0x20];
    /* 0x0f5d: the twelve worn/held slots, indexed by W8EquipSlot. Slots six
       and seven are the primary hands and eight and nine the alternate pair. */
    W8ItemInstance EquippedItem[12]; /* 0x0f5d */
    unsigned char flags1[0x3c];
    /* 0x1029: the eight per-character carried slots. GetOriginOfCharacterItem
       reports this array as origin zero and the equipment array as origin one. */
    W8ItemInstance backpack[8]; /* 0x1029 */
    unsigned char unknown_1089[0xc0];
    /* 0x1149: the two per-hand derived attack blocks. */
    W8HandAttack Hand[2];
    unsigned char unknown_11ff[0xb6];
    unsigned char dual_wielding; /* 0x12b5 */
    /* 0x12b6: per-monster-id detection score, one byte per record id. The
       wandering-group pass adds the group's member count while the party stays
       unaware, and the detection roll subtracts the record's effective level
       before scaling. */
    unsigned char monster_awareness[0x3e8];
    /* 0x169e: the fatigue band, zero through four, recomputed from the stamina
       fraction whenever it moves; a change re-runs the armour class pass. */
    int fatigue_band;
    /* 0x16a2: the modifier block rebuilt from the character's condition
       durations, enchantment slots and the bound-NPC penalty; folded into
       `bonus` alongside the equipment and party blocks. */
    W8GameplayModifierBlock condition_modifiers;
    /* 0x1709: the equipment bonus block accumulated from the worn items and
       folded into `bonus`. */
    W8GameplayModifierBlock equipment_bonus;
    /* 0x1770: the derived modifier block the rebuild clears and folds the
       equipment, persistent and party blocks into. */
    W8GameplayModifierBlock bonus;
    /* 0x17d7: the CamPos record GetWorldCameraState writes and recall restores.
       The cross-level path copies the whole 0x3c bytes onto
       W8GlobalStatus::pending_move_location. */
    W8WorldCameraState saved_location; /* 0x17d7 */
    /* 0x1813: which level that anchor belongs to. The recall compares it
       against g_status.current_level and takes a different path when they differ. */
    int saved_level;                          /* 0x1813 */
    W8CharacterConditionRecord conditions[4]; /* 0x1817 .. 0x185a */
    /* 0x185b: the deep-fatigue effect is already on this character, which is
       what stops FatigueCharacter re-applying it every turn. */
    bool deep_fatigue_applied;
    /* 0x185c: one cost per skill id 0x18..0x1b, read by the profession-change
       cost diff with the skill id biased down. */
    unsigned char skill_costs[4];
    unsigned char magic_bonus_pool;
    /* 0x1861: the anchor above has been set. Recall does nothing without it. */
    bool has_saved_location;
}; /* 0x1862 */

static_assert(sizeof(W8Character) == 0x1862, "W8Character_must_be_0x1862");

enum W8SkillImportPolicy {
    W8_SKILL_IMPORT_POLICY_0 = 0,
    W8_SKILL_IMPORT_POLICY_1 = 1,
    W8_SKILL_IMPORT_PROFESSION = 2,
    W8_SKILL_IMPORT_DISABLED = 3,
};

struct W8SkillAttributes {
    int category;
    W8SkillImportPolicy import_policy; /* 0x04: Wiz7 skill import eligibility */
    /* 0x08/0x0c: governing attributes averaged into base_level and
       listed in the stat-info dialog. */
    W8Attribute attribute_1;
    W8Attribute attribute_2;
};

#pragma pack(pop)

extern W8RaceResistanceProfile g_race_resistance_profiles[];
extern int g_profession_skill_availability[0x29][W8_PROFESSION_COUNT];
extern W8Skill g_profession_bonus_skills[W8_PROFESSION_COUNT];
extern W8SkillAttributes g_skill_attributes[0x29];

/* Profession and race trait sets consulted by CharacterHasTrait. Each entry is
   only its id list: three profession abilities, five race abilities. */
struct W8ProfessionAbilitySet {
    W8Trait ability_ids[3];
};

struct W8RaceAbilitySet {
    W8Trait ability_ids[5];
};

extern W8ProfessionAbilitySet g_profession_abilities[W8_PROFESSION_COUNT];
extern W8RaceAbilitySet g_race_abilities[W8_RACE_COUNT];
extern W8Skill g_profession_skills[W8_PROFESSION_COUNT][4];
extern int g_profession_magic_level_offsets[W8_PROFESSION_COUNT];
extern float g_profession_hit_point_factors[W8_PROFESSION_COUNT];

struct W8PortraitDescriptor {
    int group;
    int unknown_004;
    int unknown_008;
    int render_mode;
};

static_assert(sizeof(W8PortraitDescriptor) == 0x10, "W8PortraitDescriptor_size");

extern W8PortraitDescriptor g_portrait_descriptors[80];

#endif
