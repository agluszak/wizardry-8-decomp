#pragma once

#include "surrender/srMath.h"

struct W8Character;
struct W8MonsterInfo;

/* 0x00553F10: add usage points to one skill, roll its increase, and report
   the result; suppress_notification keeps silent practice calls silent. */
void PracticeCharacterSkill(W8Character* character, int skill_id, int usage_points,
                            unsigned char suppress_notification);

/* Ability trait ids granted through g_profession_abilities/g_race_abilities,
   the sets CharacterHasTrait00547940 consults; trait 0x1c additionally passes
   from the character's second enchantment slot. The names follow the attack
   sub-menu entry each one gates. */
enum {
    W8_TRAIT_PRAY = 0x0b,        /* priest */
    W8_TRAIT_TURN_UNDEAD = 0x11, /* priest and bishop */
    W8_TRAIT_BERSERK = 0x14,     /* fighter */
    W8_TRAIT_BREATHE = 0x1c      /* dracon racial or the breath enchantment */
};

void Function5477D0(W8MonsterInfo* monster_info, int flag);
void Function547A50(int party_slot);
/* 0x00547BF0: whether the slot has the priest/bishop turn-undead trait, its
   combat-state use flag is clear, and a live hostile undead monster (record
   kind 0x14) is in play. */
unsigned char CanPartySlotTurnUndead(int party_slot);
/* 0x00547F40: whether the slot has the priest's pray trait, its combat-state
   use flag is clear, and a live hostile monster is in play. */
unsigned char CanPartySlotPray(int party_slot);
float ScaleValueByProfessionLevel005479B0(W8Character* character, int trait, float base);
/* 0x005539E0: rebuild the effective attributes from the modifier block's
   seven adjustment bytes and every skill's base level from the attribute
   pair g_skill_attributes names. */
void ResetCharacterAttributes005539E0(W8Character* character);
/* 0x00553A60: rebuild every skill level from its base, the profession bonus
   and the race and profession skill adjustments. */
void ResetCharacterSkills00553A60(W8Character* character);
bool IsCharacterSkillAvailable(W8Character* character, unsigned int skill_id,
                               const unsigned char* expert_realm_flags);
void InvalidateAndRecalculateCharacterClassData00558610(W8Character* character);
bool CharacterHasTrait00547940(const W8Character* character, int trait);
/* 0x00553AD0: propagate a changed attribute base value - the at-maximum
   pseudo-skill flag, the effective value, equipment and derived state. */
void ApplyAttributeChange(W8Character* character, int attribute);
/* 0x00553C10: propagate a changed skill investment - availability rescan,
   the level rebuild, equipment and derived state. */
void ApplySkillChange(W8Character* character, int skill_id);
void InitializeSkillBaseLevels00553C90(W8Character* character);
void RefreshCharacterSkillAvailability00553CD0(W8Character* character);
unsigned int GetSkillQuarterValue00553EE0(W8Character* character, int skill_id);
