#pragma once

#include "wiz8/layouts/gameplay_databases.h"

struct W8Character;

/* CharGeneration.cpp's transient editing state, embedded by the Character
   screen and passed through its four pages and commit helpers. */
struct W8CharacterCreationState {
    int attribute_points_remaining; /* 0x000 */
    int attribute_points_total;     /* 0x004 */
    int attribute_values_008[7];    /* 0x008 */
    int attribute_step_limit;       /* 0x024 */
    int attribute_limits_028[7];    /* 0x028 */
    bool attributes_complete;       /* 0x044: set once every attribute choice validates */
    unsigned char unknown_045[3];
    int attribute_baselines_048[7]; /* 0x048 */
    int skill_points_remaining;     /* 0x064 */
    int skill_points_total;         /* 0x068 */
    int skill_points_spent[0x29];   /* 0x06c */
    int skill_step_limit;           /* 0x110 */
    int skill_limits[0x29];         /* 0x114 */
    bool skills_complete;           /* 0x1b8: set once every skill choice validates */
    unsigned char unknown_1b9[3];
    /* 0x1bc: the per-skill counterpart of attribute_baselines_048. The
       level-up reset zeroes it, and a profession change refunds whatever each
       entry holds before the new profession's skill points are assigned. */
    int skill_baselines_1bc[0x29];
    int spell_points_remaining;    /* 0x260 */
    int spell_points_total;        /* 0x264 */
    int magic_skill_bonus;         /* 0x268 */
    unsigned char spells_complete; /* 0x26c */
    unsigned char unknown_26d[3];
};
static_assert(sizeof(W8CharacterCreationState) == 0x270, "W8CharacterCreationState_size");

void InitializeCharacterCreation(W8Character*, W8CharacterCreationState*);
void InitializeCharacterLevelUp(W8Character*, W8CharacterCreationState*);
void ApplyRaceProfessionTables(W8Character*, W8CharacterCreationState*);
void RebuildLevelUpPoolsForProfession(W8Character*, W8CharacterCreationState*, W8Profession);
void Function5571C0(W8Character*, W8CharacterCreationState*, int);
void Function5571E0(W8Character*, W8CharacterCreationState*, W8Gender);
void PayDownAttributeDebt(W8Character*, W8CharacterCreationState*);
void DetermineEligibleProfessions(W8Character*, W8CharacterCreationState*, unsigned char*);
void Function557580(W8Character*, W8CharacterCreationState*, bool);
/* 0x00557430: hand out the starting equipment table the race or profession
   selects, then the profession's own extra item. */
void AddCharacterStartingEquipment(W8Character*);
void RecomputeAttributeLimits(W8Character*, W8CharacterCreationState*);
void ClampAttributesToBudget(W8Character*, W8CharacterCreationState*);
void ApplyProfessionMinimumAttributes(W8Character*, W8CharacterCreationState*);
void AdjustAllocatedAttribute(W8Character*, W8CharacterCreationState*, int, int);
void Function557AE0(W8Character*, W8CharacterCreationState*);
void RecomputeSkillLimits(W8Character*, W8CharacterCreationState*);
void InitializeLevelUpAttributePool(W8Character*, W8CharacterCreationState*, unsigned int, int);
void ResetSkillContribution(W8Character*, W8CharacterCreationState*, int);
void RefundSkillAllocation(W8Character*, W8CharacterCreationState*, int);
void RebuildSkillAllocations(W8Character*, W8CharacterCreationState*);
void ClampSkillsToBudget(W8Character*, W8CharacterCreationState*);
void RefundAllSkillPoints(W8Character*, W8CharacterCreationState*);
void FinalizeSpellPointPool(W8Character*, W8CharacterCreationState*);
int CountRemainingSpellPoints(W8Character*, W8CharacterCreationState*);
int ComputeLevelUpSpellPointAward(W8Character*, W8CharacterCreationState*);
void Function5584E0(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function558560(W8Character*, W8CharacterCreationState*, unsigned int spell);
void Function5585D0(W8Character*, W8CharacterCreationState*);
