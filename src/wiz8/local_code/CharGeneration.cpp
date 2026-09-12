#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/character.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/magic.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"

#include <stdlib.h>
#include <string.h>

#define CHAR_GENERATION_CPP "C:\\Projects\\Wizardry 8\\Local Code\\CharGeneration.cpp"

/* Refund every spent skill point through the per-skill commit helper. */
// FUNCTION: WIZ8 0x00557F90
void RefundAllSkillPoints(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;
    int spent;

    for (index = 0; index < 0x29; ++index) {
        spent = creation_state->skill_points_spent[index];
        if (spent > 0) {
            InitializeLevelUpAttributePool(character, creation_state, index, -spent);
        }
    }
}

/* Sums the realm skill costs the edited character still owes: skills the
   original holds but the edited build lacks, or the original's own set when
   it has no caster level to compare against. */
// FUNCTION: WIZ8 0x00557FD0
int ComputeRealmSkillDebt(W8Character* original, W8Character* edited)
{
    int total = 0;

    if (GetProfessionCasterLevel(edited, -1) != 0) {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (original->skills[index].flag_00 == 0) {
                continue;
            }
            if (edited->skills[index].flag_00 != 0) {
                continue;
            }
            total += original->skill_costs_185c[index - 0x18];
        }
        return total;
    }
    if (GetProfessionCasterLevel(original, -1) < 1) {
        return 0;
    }
    {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (original->skills[index].flag_00 == 0) {
                continue;
            }
            total += original->skill_costs_185c[index - 0x18];
        }
        total += original->unknown_1860;
    }
    return total;
}

/* Character generation state. The four dwords below accumulate the pools a
   reset or level-up hands to the editing state; the byte at 0x34 records that
   a profession change forced the sex to one. */
// GLOBAL: WIZ8 0x0068de28
int g_attribute_point_bonus_0068de28;
// GLOBAL: WIZ8 0x0068de2c
int g_skill_point_bonus_0068de2c;
// GLOBAL: WIZ8 0x0068de30
int g_spell_point_bonus_0068de30;
// GLOBAL: WIZ8 0x0068de34
unsigned char g_gender_locked_0068de34;

/* Empty every item record the character carries. 0x00520070 expands the
   per-slot helper at both loops, which is why the body lives in PC Item.cpp
   and duplicates that helper instead of calling it. */

/* A fresh creation: zero the character and the editing state, install the
   sentinel values, and hand out the level-one pools. */
// FUNCTION: WIZ8 0x00556dc0
void InitializeCharacterCreation(W8Character* character, W8CharacterCreationState* creation_state)
{
    g_attribute_point_bonus_0068de28 = 0;
    g_skill_point_bonus_0068de2c = 0;
    g_spell_point_bonus_0068de30 = 0;
    g_gender_locked_0068de34 = 0;
    memset(character, 0, sizeof(*character));
    character->gender = W8_GENDER_UNSET;
    character->current_profession = W8_PROFESSION_NONE;
    character->race = -1;
    character->level = 1;
    character->level_band_base = 0;
    character->highest_condition = 0;
    character->enchantment_top = 0;
    character->table_value_0079 = -1;
    character->unknown_007d = -1;
    character->personality_0081 = -1;
    memset(creation_state, 0, sizeof(*creation_state));
    EmptyAllCarriedItems(character);

    int points = 0;
    if (character->current_profession != -1 && character->race != -1) {
        points = 0x3c + g_attribute_point_bonus_0068de28;
        if (character->level != 1) {
            points -= 0x36;
        }
        if (points < 0) {
            points = 0;
        }
    }
    creation_state->attribute_points_total = points;
    creation_state->attribute_points_remaining = points;
    RecomputeAttributeLimits(character, creation_state);

    points = 0xf + g_skill_point_bonus_0068de2c;
    if (character->level != 1) {
        points -= 6;
    }
    if (points < 0) {
        points = 0;
    }
    creation_state->skill_points_total = points;
    creation_state->skill_points_remaining = points;
    RecomputeSkillLimits(character, creation_state);
}

/* One level gained: raise the level and profession counter, reset the editing
   state, and re-derive every pool the character's new level entitles them
   to. */
// FUNCTION: WIZ8 0x00556cc0
void InitializeCharacterLevelUp(W8Character* character, W8CharacterCreationState* creation_state)
{
    g_attribute_point_bonus_0068de28 = 0;
    g_skill_point_bonus_0068de2c = 0;
    g_spell_point_bonus_0068de30 = 0;
    g_gender_locked_0068de34 = 0;
    ++character->level;
    ++character->profession_levels[character->current_profession];
    memset(creation_state, 0, sizeof(*creation_state));

    int points = 0;
    if (character->current_profession != -1 && character->race != -1) {
        points = 0x3c + g_attribute_point_bonus_0068de28;
        if (character->level != 1) {
            points -= 0x36;
        }
        if (points < 0) {
            points = 0;
        }
    }
    creation_state->attribute_points_total = points;
    creation_state->attribute_points_remaining = points;
    RecomputeAttributeLimits(character, creation_state);

    points = 0xf + g_skill_point_bonus_0068de2c;
    if (character->level != 1) {
        points -= 6;
    }
    if (points < 0) {
        points = 0;
    }
    creation_state->skill_points_total = points;
    creation_state->skill_points_remaining = points;
    RecomputeSkillLimits(character, creation_state);

    if (character->attribute_point_deficit_0199 < 0) {
        PayDownAttributeDebt(character, creation_state);
    }
    CalcCharacterLevelBand(character);
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    FinalizeSpellPointPool(character, creation_state);
}

/* Reset one skill's contribution to the editing state: drop its flag, refund
   its spent points, and let a realm skill that was just opened up reach its
   minimum five. */
// FUNCTION: WIZ8 0x00557c90
void ResetSkillContribution(W8Character* character, W8CharacterCreationState* creation_state,
                            int skill_id)
{
    character->skills[skill_id].flag_00 = 1;
    creation_state->skill_points_spent[skill_id] = 0;

    if (skill_id >= 0x18 && skill_id < 0x1c) {
        int offset = g_profession_magic_level_offsets[character->current_profession];
        if (offset < 0 && offset > -0xff &&
            character->profession_levels[character->current_profession] + offset == 1) {
            int missing = 5 - character->skills[skill_id].value_02;
            if (missing > 0) {
                character->skills[skill_id].value_02 += missing;
                character->skills[skill_id].level += missing;
                creation_state->skill_baselines_1bc[skill_id] += missing;
            }
        }
    }
}

/* Refund every point spent on one skill and leave its level at the base
   value the assignment gave it. */
// FUNCTION: WIZ8 0x00557d20
void RefundSkillAllocation(W8Character* character, W8CharacterCreationState* creation_state,
                           int skill_id)
{
    int spent = creation_state->skill_points_spent[skill_id];
    if (spent > 0) {
        creation_state->skill_points_remaining += spent;
        character->skills[skill_id].value_02 -= spent;
        creation_state->skill_points_spent[skill_id] = 0;
    }
    character->skills[skill_id].level = character->skills[skill_id].value_02;
}

/* Every point the edited character has already committed is spent from the
   pool in one burst. */
// FUNCTION: WIZ8 0x00557ae0
void RefundAllocatedAttributes(W8Character* character, W8CharacterCreationState* creation_state)
{
    for (int index = 0; index < 7; ++index) {
        if (creation_state->attribute_values_008[index] > 0) {
            AdjustAllocatedAttribute(character, creation_state, index,
                                     -creation_state->attribute_values_008[index]);
        }
    }
}

/* Attribute limits and the step ceiling for the current pool, then the
   attribute-point reconciliations. */
// FUNCTION: WIZ8 0x00557730
void RecomputeAttributeLimits(W8Character* character, W8CharacterCreationState* creation_state)
{
    int points = creation_state->attribute_points_total;
    int step;
    int index;

    if (points < 0) {
        step = 0;
    } else {
        for (index = 0; index < 7; ++index) {
            points += creation_state->attribute_baselines_048[index];
        }
        step = (points + 2) / 3;
        if (step < 3) {
            step = points < 4 ? points : 3;
        }
    }
    creation_state->attribute_step_limit = step;

    for (index = 0; index < 7; ++index) {
        int limit = step - creation_state->attribute_baselines_048[index];
        int cap =
            (creation_state->attribute_values_008[index] - character->attributes[index].value) +
            100;
        int value = limit;
        if (cap <= limit) {
            value = cap;
        }
        if (value < 0) {
            limit = 0;
        } else if (cap <= limit) {
            limit = cap;
        }
        creation_state->attribute_limits_028[index] = limit;
    }

    ClampAttributesToBudget(character, creation_state);
    if (creation_state->attribute_points_total > 0) {
        for (index = 0; index < 7; ++index) {
            if (creation_state->attribute_values_008[index] <
                creation_state->attribute_limits_028[index]) {
                creation_state->attributes_complete = 0;
                return;
            }
        }
    }
    creation_state->attributes_complete = 1;
}

/* Clamp the allocated attribute values to their limits, then take points back
   round-robin while the character is over budget. */
// FUNCTION: WIZ8 0x00557800
void ClampAttributesToBudget(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;

    for (index = 0; index < 7; ++index) {
        int value = creation_state->attribute_values_008[index];
        int limit = creation_state->attribute_limits_028[index];
        if (value > limit) {
            creation_state->attribute_values_008[index] = value - (value - limit);
        }
    }

    int total = 0;
    for (index = 0; index < 7; ++index) {
        total += creation_state->attribute_values_008[index];
    }
    if (total > 0 && creation_state->attribute_points_total > 0 &&
        creation_state->attribute_points_total < total) {
        index = 0;
        while (creation_state->attribute_points_total < total) {
            if (creation_state->attribute_values_008[index] > 0) {
                --creation_state->attribute_values_008[index];
                --character->attributes[index].value;
                --total;
            }
            if (++index == 7) {
                index = 0;
            }
        }
    }
    creation_state->attribute_points_remaining = creation_state->attribute_points_total - total;
}

/* Top the character's attributes up to the profession's minimums, or hand the
   shortfall to 0x00557200 when the pool cannot cover it. A level-one character
   also gets the value/baseline bookkeeping the creation flow expects. */
// FUNCTION: WIZ8 0x00557890
void ApplyProfessionMinimumAttributes(W8Character* character,
                                      W8CharacterCreationState* creation_state)
{
    int deficits[7];
    int index;

    character->attribute_point_deficit_0199 = 0;
    for (index = 0; index < 7; ++index) {
        int deficit = g_profession_attribute_minimums[character->current_profession].values[index] -
                      character->attributes[index].value;
        deficits[index] = deficit;
        if (deficit > 0) {
            character->attribute_point_deficit_0199 -= deficit;
        }
    }

    if (creation_state->attribute_points_total + character->attribute_point_deficit_0199 < 0) {
        PayDownAttributeDebt(character, creation_state);
        return;
    }

    character->attribute_point_deficit_0199 = 0;
    character->level_band_base = 0;
    for (index = 0; index < 7; ++index) {
        int deficit = deficits[index];
        if (deficit < 1) {
            if (character->level > 1) {
                int minimum =
                    g_profession_attribute_minimums[character->current_profession].values[index];
                int value = creation_state->attribute_values_008[index];
                while (character->attributes[index].value - value < (unsigned int)minimum) {
                    creation_state->attribute_values_008[index] = value - 1;
                    --creation_state->attribute_points_total;
                    ++creation_state->attribute_baselines_048[index];
                    value = creation_state->attribute_values_008[index];
                }
            }
        } else {
            character->attributes[index].value += deficit;
            character->attributes[index].effective += deficit;
            creation_state->attribute_points_total -= deficit;
            if (character->level > 1) {
                creation_state->attribute_baselines_048[index] = deficit;
            }
        }
    }
    creation_state->attribute_points_remaining = creation_state->attribute_points_total;
    RecomputeAttributeLimits(character, creation_state);
}

/* Draw the level-up attribute debt back down one point at a time, preferring
   the largest remaining deficit, until the pool or the debt runs out. */
// FUNCTION: WIZ8 0x00557200
void PayDownAttributeDebt(W8Character* character, W8CharacterCreationState* creation_state)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int deficits[7][2];
    int index;

    int total = 0;
    for (index = 0; index < 7; ++index) {
        int minimum = g_profession_attribute_minimums[character->current_profession].values[index];
        int deficit = 0;
        if (character->attributes[index].value < minimum) {
            deficit = minimum - character->attributes[index].value;
        }
        deficits[index][0] = deficit;
        deficits[index][1] = index;
        total -= deficit;
    }
    qsort(deficits, 7, 8, CompareSignedDescending);
    if (character->attribute_point_deficit_0199 != total) {
        character->attribute_point_deficit_0199 = total;
    }
    index = 0;
    while (character->attribute_point_deficit_0199 < 0 &&
           creation_state->attribute_points_total > 0) {
        if (deficits[index][0] < 1 ||
            (index != 6 && deficits[index][0] <= deficits[index + 1][0])) {
            index = (index + 1) % 7;
        } else {
            int attribute = deficits[index][1];
            ++character->attributes[attribute].value;
            ++character->attributes[attribute].effective;
            total = deficits[index][0];
            ++character->attribute_point_deficit_0199;
            int available = creation_state->attribute_points_total;
            deficits[index][0] = total - 1;
            creation_state->attribute_points_total = available - 1;
            index = 0;
        }
    }
    if (character->attribute_point_deficit_0199 >= 0) {
        if (creation_state->attribute_points_total < creation_state->attribute_points_remaining) {
            creation_state->attribute_points_remaining = creation_state->attribute_points_total;
        }
        return;
    }
    creation_state->attribute_points_total = character->attribute_point_deficit_0199;
    character->level_band_base = character->level;
#pragma clang diagnostic pop
}

/* Which of the fifteen professions the current attribute budget can still
   reach: the character's own profession is always eligible, profession two
   needs the matching sex, and every other one has to clear the
   profession's minimums within the remaining pool and limits. */
// FUNCTION: WIZ8 0x00557350
void DetermineEligibleProfessions(W8Character* character, W8CharacterCreationState* creation_state,
                                  unsigned char* eligibility)
{
    int attribute;

    for (unsigned int profession = 0; profession < W8_PROFESSION_COUNT; ++profession) {
        eligibility[profession] = 1;
        if (profession == (unsigned int)character->current_profession) {
            eligibility[profession] = 1;
        } else if (profession == W8_PROFESSION_VALKYRIE && character->gender != W8_GENDER_FEMALE) {
            eligibility[W8_PROFESSION_VALKYRIE] = 0;
        } else {
            int deficit = 0;
            for (attribute = 0; attribute < 7; ++attribute) {
                int difference = g_profession_attribute_minimums[profession].values[attribute] -
                                 character->attributes[attribute].value;
                if (difference > 0) {
                    deficit -= difference;
                }
            }
            if (creation_state->attribute_points_total + deficit < 0) {
                eligibility[profession] = 0;
            }
            for (attribute = 0; attribute < 7; ++attribute) {
                if ((unsigned int)(creation_state->attribute_limits_028[attribute] -
                                   creation_state->attribute_values_008[attribute]) +
                        character->attributes[attribute].value <
                    (unsigned int)g_profession_attribute_minimums[profession].values[attribute]) {
                    eligibility[profession] = 0;
                    break;
                }
            }
        }
    }
}

/* Apply one attribute adjustment through the same clamps 0x005579E0 uses for
   the creation flow: never below zero, never above the pool or the limit. */
// FUNCTION: WIZ8 0x005579e0
void AdjustAllocatedAttribute(W8Character* character, W8CharacterCreationState* creation_state,
                              int attribute, int modifier)
{
    int value = creation_state->attribute_values_008[attribute];
    if (value + modifier < 0) {
        if (modifier >= 0) {
            srAssertFail("iModifier < 0", CHAR_GENERATION_CPP, 0x380, 0);
        }
        modifier = -value;
    }
    if (modifier > creation_state->attribute_points_remaining) {
        modifier = creation_state->attribute_points_remaining;
    }
    if (value + modifier > creation_state->attribute_limits_028[attribute]) {
        modifier = creation_state->attribute_limits_028[attribute] - value;
    }
    character->attributes[attribute].value += modifier;
    character->attributes[attribute].effective += modifier;
    creation_state->attribute_points_remaining -= modifier;
    creation_state->attribute_values_008[attribute] += modifier;

    if (creation_state->attribute_points_remaining > 0) {
        for (int index = 0; index < 7; ++index) {
            if (creation_state->attribute_values_008[index] <
                creation_state->attribute_limits_028[index]) {
                creation_state->attributes_complete = 0;
                goto complete;
            }
        }
    }
    creation_state->attributes_complete = 1;
complete:
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateCarryingCapacity004EDC10(character);
    RecalculateCarriedWeight(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    RecalculateCharacterResistances(character);
    if (character->level == 1) {
        RebuildSkillAllocations(character, creation_state);
    }
    CountRemainingSpellPoints(character, creation_state);
    RefreshCharacterSkillAvailability00553CD0(character);
    RecomputeSkillLimits(character, creation_state);
}

/* Skill limits from the pool plus the base levels the attribute pairs give
   every skill. */
// FUNCTION: WIZ8 0x00557b20
void RecomputeSkillLimits(W8Character* character, W8CharacterCreationState* creation_state)
{
    InitializeSkillBaseLevels00553C90(character);
    int step = (creation_state->skill_points_total + 2) / 3;
    creation_state->skill_step_limit = step;

    for (int index = 0; index < 0x29; ++index) {
        if (character->skills[index].flag_00 == 0) {
            creation_state->skill_limits[index] = 0;
            continue;
        }
        int limit =
            (creation_state->skill_points_spent[index] - character->skills[index].value_02) + 0x4b;
        int value = step;
        if (limit <= step) {
            value = limit;
        }
        if (value < 0) {
            creation_state->skill_limits[index] = 0;
        } else {
            if (step < limit) {
                limit = step;
            }
            creation_state->skill_limits[index] = limit;
        }
    }
    ClampSkillsToBudget(character, creation_state);
}

/* Clamp the committed skill points to their limits, refund round-robin while
   over budget, and report whether every skill still has room. */
// FUNCTION: WIZ8 0x00557eb0
void ClampSkillsToBudget(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;

    for (index = 0; index < 0x29; ++index) {
        int spent = creation_state->skill_points_spent[index];
        int excess = spent - creation_state->skill_limits[index];
        if (excess > 0) {
            creation_state->skill_points_spent[index] = spent - excess;
            character->skills[index].value_02 -= excess;
            character->skills[index].level -= excess;
        }
    }
    int total = 0;
    for (index = 0; index < 0x29; ++index) {
        total += creation_state->skill_points_spent[index];
    }
    if (total > 0 && creation_state->skill_points_total > 0 &&
        creation_state->skill_points_total < total) {
        index = 0;
        while (creation_state->skill_points_total < total) {
            if (creation_state->skill_points_spent[index] > 0) {
                --creation_state->skill_points_spent[index];
                --character->skills[index].value_02;
                --character->skills[index].level;
                --total;
            }
            if (++index == 0x29) {
                index = 0;
            }
        }
    }
    creation_state->skill_points_remaining = creation_state->skill_points_total - total;
    if (creation_state->skill_points_remaining > 0) {
        for (index = 0; index < 0x29; ++index) {
            if (creation_state->skill_points_spent[index] < creation_state->skill_limits[index]) {
                creation_state->skills_complete = 0;
                return;
            }
        }
    }
    creation_state->skills_complete = 1;
}

/* Finalize the spell-point pool the level-up flow presents: settle the realm
   skills against the spent spell points, then trim any learned spell the
   character can no longer hold. */
// FUNCTION: WIZ8 0x00558070
void FinalizeSpellPointPool(W8Character* character, W8CharacterCreationState* creation_state)
{
    int spent = creation_state->spell_points_total - creation_state->spell_points_remaining;
    if (GetProfessionCasterLevel(character, -1) < 1) {
        creation_state->spell_points_total = 0;
    } else {
        if (character->level == 1) {
            creation_state->magic_skill_bonus = 2;
        } else {
            creation_state->magic_skill_bonus = 1;
        }
        creation_state->magic_skill_bonus += g_spell_point_bonus_0068de30;
        int total = 0;
        if (GetProfessionCasterLevel(character, -1) > 0) {
            for (unsigned int realm = 0x18; realm < 0x1c; ++realm) {
                if (character->skills[realm].flag_00 != 0) {
                    total += character->skill_costs_185c[realm - 0x18];
                }
            }
            total += character->unknown_1860;
        }
        creation_state->spell_points_total = total + creation_state->magic_skill_bonus;
    }

    if (creation_state->spell_points_total < spent) {
        for (unsigned int index = 0; index < 0x72; ++index) {
            if (character->spell_learned[index] == 2) {
                character->spell_learned[index] = 0;
                --spent;
                if (spent == creation_state->spell_points_total) {
                    break;
                }
            }
        }
    }
    creation_state->spell_points_remaining = creation_state->spell_points_total - spent;
    if (creation_state->spell_points_remaining > 0) {
        creation_state->spells_complete = 0;
        return;
    }
    creation_state->spells_complete = 1;
}

/* Re-checks every spell against the character's current realm skills and
   reports how many points still remain to be allocated. */
// FUNCTION: WIZ8 0x00558180
int CountRemainingSpellPoints(W8Character* character, W8CharacterCreationState* creation_state)
{
    int available = 0;
    int index;
    int spent = creation_state->spell_points_total - creation_state->spell_points_remaining;

    if (creation_state->spell_points_total < 1) {
        for (index = 0; index < 0x72; ++index) {
            if (character->spell_learned[index] == -1 || character->spell_learned[index] == 2) {
                character->spell_learned[index] = 0;
            }
        }
    } else {
        for (index = 0; index < 4; ++index) {
            int skill = 0x18 + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].value_02 -= points;
            character->skills[skill].level -= points;
        }
        for (index = 0; index < 6; ++index) {
            int skill = 0x1c + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].value_02 -= points;
            character->skills[skill].level -= points;
        }
        for (index = 0; index < 0x72; ++index) {
            if (character->spell_learned[index] != 1) {
                if (!CanCharacterLearnSpell(character, index)) {
                    if (character->spell_learned[index] == 2) {
                        --spent;
                        ++creation_state->spell_points_remaining;
                    }
                    character->spell_learned[index] = 0;
                } else {
                    if (character->spell_learned[index] != 2) {
                        character->spell_learned[index] = -1;
                    }
                    ++available;
                }
            }
        }
        for (index = 0; index < 4; ++index) {
            int skill = 0x18 + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].value_02 += points;
            character->skills[skill].level += points;
        }
        for (index = 0; index < 6; ++index) {
            int skill = 0x1c + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].value_02 += points;
            character->skills[skill].level += points;
        }
    }

    if (available < creation_state->spell_points_total) {
        creation_state->spell_points_total = available;
        int remaining = available - spent;
        if (remaining < 0) {
            remaining = 0;
        }
        creation_state->spell_points_remaining = remaining;
    } else {
        creation_state->spell_points_remaining = creation_state->spell_points_total - spent;
    }
    if (creation_state->spell_points_total != 0 && creation_state->spell_points_remaining != 0) {
        creation_state->spells_complete = 0;
        return available;
    }
    creation_state->spells_complete = 1;
    return available;
}

/* Reset every skill to the attribute-derived base, hand the profession's
   skills and bonus skill their share of the step pool, then add the points
   already committed. */
// FUNCTION: WIZ8 0x00557d80
void RebuildSkillAllocations(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;

    for (index = 0; index < 0x29; ++index) {
        character->skills[index].value_02 = 0;
        character->skills[index].level = 0;
    }
    InitializeSkillBaseLevels00553C90(character);

    int profession = character->current_profession;
    int count = 1;
    for (index = 0; index < 4; ++index) {
        if (g_profession_skills[profession][index] != -1) {
            ++count;
        }
    }
    int step = 0x3c / count;
    if (creation_state->attribute_points_total < 0) {
        step -= (unsigned int)(creation_state->attribute_points_total * step * -2) / 100;
    }

    for (index = 0; index < 4; ++index) {
        int skill = g_profession_skills[profession][index];
        if (skill != -1) {
            int value = (character->skills[skill].base_level_0a * step) / 100;
            character->skills[skill].value_02 = value;
            character->skills[skill].level = value;
        }
    }
    int bonus_skill = g_profession_bonus_skills[profession];
    int value = (character->skills[bonus_skill].base_level_0a * step) / 100;
    character->skills[bonus_skill].value_02 = value;
    character->skills[bonus_skill].level = value;
    for (index = 0; index < 0x29; ++index) {
        character->skills[index].value_02 += creation_state->skill_points_spent[index];
        character->skills[index].level = character->skills[index].value_02;
    }
    character->skills[bonus_skill].level += GetSkillQuarterValue00553EE0(character, bonus_skill);
}

/* Compute how many spell points the level-up summary can award from the
   character's six realm pools, saving and restoring the learned-spell flags
   around the trial assignment. */
// FUNCTION: WIZ8 0x00558330
int ComputeLevelUpSpellPointAward(W8Character* character, W8CharacterCreationState* creation_state)
{
    int total = 0;
    int index;
    int saved[0x72];

    if (character->level < 2) {
        if (creation_state->spell_points_total > 0) {
            if (creation_state->spell_points_remaining > 0) {
                for (index = 0; index < 0x72; ++index) {
                    saved[index] = character->spell_learned[index];
                }
                for (index = 0; index < 0x72; ++index) {
                    character->spell_learned[index] = 0;
                }
                static const int trial_spells[6] = {0xc, 6, 7, 5, 3, 0xb};
                for (index = 0; index < 6; ++index) {
                    if (CanCharacterLearnSpell(character, trial_spells[index])) {
                        character->spell_learned[trial_spells[index]] = 1;
                    }
                }
            }
            RecalculateRealmSpellPoints(character);
            int pools[6][2];
            for (index = 0; index < 6; ++index) {
                pools[index][0] = character->sp_max[index];
                pools[index][1] = index;
            }
            qsort(pools, 6, 8, CompareSignedDescending);
            int count =
                creation_state->spell_points_total < 7 ? creation_state->spell_points_total : 6;
            for (index = 0; index < count; ++index) {
                total += character->sp_max[pools[index][1]];
            }
            if (creation_state->spell_points_remaining > 0) {
                for (index = 0; index < 0x72; ++index) {
                    character->spell_learned[index] = saved[index];
                }
                RecalculateRealmSpellPoints(character);
            }
        }
        return total;
    }
    for (index = 0; index < 6; ++index) {
        total += character->sp_max[index];
    }
    return total;
}

/* Recompute the level-up pools after a profession change, refunding every
   baseline the previous profession's assignment had granted. */
// FUNCTION: WIZ8 0x00557060
void RebuildLevelUpPoolsForProfession(W8Character* character,
                                      W8CharacterCreationState* creation_state,
                                      W8Profession profession)
{
    int index;

    if (character->current_profession != W8_PROFESSION_NONE) {
        --character->profession_levels[character->current_profession];
    }
    ++character->profession_levels[profession];
    character->current_profession = profession;

    if (profession == W8_PROFESSION_VALKYRIE) {
        if (character->gender == W8_GENDER_MALE) {
            g_gender_locked_0068de34 = 1;
            character->gender = W8_GENDER_FEMALE;
        }
    } else if (g_gender_locked_0068de34) {
        g_gender_locked_0068de34 = 0;
        character->gender = W8_GENDER_MALE;
    }

    if (character->level > 1) {
        for (index = 0; index < 7; ++index) {
            int baseline = creation_state->attribute_baselines_048[index];
            if (baseline > 0) {
                character->attributes[index].value -= baseline;
                character->attributes[index].effective -= baseline;
                creation_state->attribute_points_total += baseline;
                creation_state->attribute_baselines_048[index] = 0;
            }
        }
        for (index = 0; index < 0x29; ++index) {
            int baseline = creation_state->skill_baselines_1bc[index];
            if (baseline > 0) {
                character->skills[index].value_02 -= baseline;
                character->skills[index].level -= baseline;
                creation_state->skill_baselines_1bc[index] = 0;
            }
        }
        int bonus_skill = g_profession_bonus_skills[character->current_profession];
        int base = character->skills[bonus_skill].value_02;
        int missing = (creation_state->skill_points_spent[bonus_skill] - base) + 5;
        if (missing > 0) {
            character->skills[bonus_skill].value_02 = base + missing;
            character->skills[bonus_skill].level += missing;
            creation_state->skill_baselines_1bc[bonus_skill] += missing;
        }
        for (index = 0; index < 4; ++index) {
            int skill = g_profession_skills[character->current_profession][index];
            int value = character->skills[skill].value_02;
            int missing = (creation_state->skill_points_spent[skill] - value) + 5;
            if (missing > 0) {
                character->skills[skill].value_02 = value + missing;
                character->skills[skill].level += missing;
                creation_state->skill_baselines_1bc[skill] += missing;
            }
        }
    }
    ApplyRaceProfessionTables(character, creation_state);
}

/* Assign the chosen race and rebuild the race/profession tables. */
// FUNCTION: WIZ8 0x005571c0
void SetCharacterRace(W8Character* character, W8CharacterCreationState* creation_state, int race)
{
    character->race = race;
    ApplyRaceProfessionTables(character, creation_state);
}

/* Assign the chosen sex and rebuild the race/profession tables. */
// FUNCTION: WIZ8 0x005571e0
void SetCharacterGender(W8Character* character, W8CharacterCreationState* creation_state,
                        W8Gender gender)
{
    character->gender = gender;
    ApplyRaceProfessionTables(character, creation_state);
}

/* Apply the race and profession tables once race, profession and sex are
   all set, rebuilding every derived pool on the way. */
// FUNCTION: WIZ8 0x00556eb0
void ApplyRaceProfessionTables(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;
    if (character->race == -1) {
        if (character->current_profession != -1) {
            for (index = 0; index < 7; ++index) {
                unsigned int minimum =
                    g_profession_attribute_minimums[character->current_profession].values[index];
                character->attributes[index].value = minimum;
                character->attributes[index].effective = minimum;
            }
        }
    } else {
        if (character->level == 1) {
            for (index = 0; index < 7; ++index) {
                unsigned int minimum = g_race_attribute_minimums[character->race].values[index];
                character->attributes[index].value = minimum;
                character->attributes[index].effective = minimum;
            }
        }
        if (character->current_profession != -1) {
            if (character->level == 1) {
                int points = 0x3c + g_attribute_point_bonus_0068de28;
                if (points < 0) {
                    points = 0;
                }
                creation_state->attribute_points_total = points;
                creation_state->attribute_points_remaining = points;
                RecomputeAttributeLimits(character, creation_state);
                character->original_profession = character->current_profession;
                ApplyProfessionMinimumAttributes(character, creation_state);
                CalcCharacterLevelBand(character);
                for (index = 0; index < 7; ++index) {
                    character->attributes[index].value +=
                        creation_state->attribute_values_008[index];
                }
            } else {
                ApplyProfessionMinimumAttributes(character, creation_state);
            }
            RefreshCharacterSkillAvailability00553CD0(character);
            FinalizeSpellPointPool(character, creation_state);
            RecountLearnedSpellsByRealm004F96A0(character);
            CountRemainingSpellPoints(character, creation_state);
            RefreshCharacterSkillAvailability00553CD0(character);
            RecomputeSkillLimits(character, creation_state);
        }
        if (character->level == 1) {
            for (index = 0; index < 7; ++index) {
                character->attributes[index].effective = character->attributes[index].value;
            }
        }
        RecalculateCharacterHitPoints(character);
        RecalculateCharacterStamina(character);
        RecalculateCarryingCapacity004EDC10(character);
        RecalculateCarriedWeight(character);
        RecalculateRealmSpellPoints(character);
        CalcArmorClasses(character);
    }
    RecalculateCharacterResistances(character);
    if (character->level == 1) {
        if (character->gender != -1 && character->current_profession != -1 &&
            character->race != -1) {
            RebuildSkillAllocations(character, creation_state);
        }
        character->table_value_0079 = -1;
        character->unknown_007d = -1;
        character->personality_0081 = -1;
        character->voice_0085 = 0;
    }
}

/* The level-up attribute editor's pool is fixed by the profession minimums,
   so the deficit sweep runs before the skill pass. */
// FUNCTION: WIZ8 0x00557bc0
void InitializeLevelUpAttributePool(W8Character* character,
                                    W8CharacterCreationState* creation_state, unsigned int skill,
                                    int modifier)
{
    int spent = creation_state->skill_points_spent[skill];
    if (spent + modifier < 0) {
        if (modifier >= 0) {
            srAssertFail("iModifier < 0", CHAR_GENERATION_CPP, 0x448, 0);
        }
        modifier = -spent;
    }
    if (creation_state->skill_points_remaining < modifier) {
        modifier = creation_state->skill_points_remaining;
    }
    if (creation_state->skill_limits[skill] < spent + modifier) {
        modifier = creation_state->skill_limits[skill] - spent;
    }
    character->skills[skill].value_02 += modifier;
    character->skills[skill].level += modifier;
    creation_state->skill_points_remaining -= modifier;
    creation_state->skill_points_spent[skill] += modifier;

    if (creation_state->skill_points_remaining > 0) {
        for (int index = 0; index < 0x29; ++index) {
            if (creation_state->skill_points_spent[index] < creation_state->skill_limits[index]) {
                creation_state->skills_complete = 0;
                goto complete;
            }
        }
    }
    creation_state->skills_complete = 1;
complete:
    CountRemainingSpellPoints(character, creation_state);
    RefreshCharacterSkillAvailability00553CD0(character);
    RecomputeSkillLimits(character, creation_state);
    RecalculateCharacterResistances(character);
}

/* Finalize a created character: recompute the derived stats, hand out the
   starting equipment when the caller asked for it, settle the attribute magic
   skill bonus against the skill costs of the chosen spells, turn every chosen
   spell into a learned one, reset the ones left over, rebuild the spell-point
   ceilings and re-derive the experience goal and level band. The spell the
   creation flow marks as chosen but does not charge for is skipped by its
   retail index, not by anything a record says. */
// FUNCTION: WIZ8 0x00557580
void FinalizeCreatedCharacter(W8Character* character, W8CharacterCreationState* creation_state,
                              bool give_starting_equipment)
{
    unsigned int realm;
    unsigned int spell;

    RecalculateCharacterDerivedStats(character);
    if (give_starting_equipment) {
        AddCharacterStartingEquipment(character);
    }

    if (character->current_profession == 0xc) {
        character->unknown_1860 += creation_state->magic_skill_bonus;
    } else {
        for (realm = 0x18; realm <= 0x1b; ++realm) {
            if (character->skills[realm].flag_00 != 0) {
                character->skill_costs_185c[realm - 0x18] += creation_state->magic_skill_bonus;
                break;
            }
        }
    }

    for (spell = 0; spell < 0x72; ++spell) {
        if (character->spell_learned[spell] != 2) {
            continue;
        }
        character->spell_learned[spell] = 1;
        if (spell == 0x49) {
            continue;
        }
        for (realm = 0x18; realm <= 0x1b; ++realm) {
            switch (realm) {
            case 0x18:
                if (g_spell_records[spell].wizardry_spell == 0)
                    continue;
                break;
            case 0x19:
                if (g_spell_records[spell].divinity_spell == 0)
                    continue;
                break;
            case 0x1a:
                if (g_spell_records[spell].alchemy_spell == 0)
                    continue;
                break;
            case 0x1b:
                if (g_spell_records[spell].psionics_spell == 0)
                    continue;
                break;
            }
            if (character->skills[realm].flag_00 != 0 &&
                character->skill_costs_185c[realm - 0x18] > 0) {
                --character->skill_costs_185c[realm - 0x18];
                goto spell_done;
            }
        }
        if (character->unknown_1860 > 0) {
            --character->unknown_1860;
        } else if (character->current_profession == 0xc) {
            for (realm = 0x18; realm <= 0x1b; ++realm) {
                if (character->skills[realm].flag_00 != 0 &&
                    character->skill_costs_185c[realm - 0x18] > 0) {
                    --character->skill_costs_185c[realm - 0x18];
                    break;
                }
            }
        }
    spell_done:;
    }

    for (spell = 0; spell < 0x72; ++spell) {
        if (character->spell_learned[spell] == 2) {
            character->spell_learned[spell] = 1;
        }
    }

    RebuildRealmSpellPointCeilings0052A540(character);
    character->experience_previous_goal = character->experience_goal;
    CalcXPGoal(character);
    CalcCharacterLevelBand(character);
}

/* The six starting item ids each profession hands out, with the faerie race's
   own row last; -1 is an empty slot. */
// GLOBAL: WIZ8 0x0061635c
int g_starting_equipment_61635c[0x10][6] = {
    {143, 153, 203, 246, -1, -1},  {35, 1, 158, 204, 247, 210},  {75, 170, 197, 246, 215, -1},
    {110, 125, 157, 188, 247, -1}, {38, 49, 165, 192, 246, -1},  {105, 230, 180, 209, 253, -1},
    {79, 105, 165, 192, 246, -1},  {0, 0, 99, 156, 187, 247},    {132, 156, 187, 246, -1, -1},
    {769, 116, 132, 156, 187, -1}, {165, 192, 246, 335, -1, -1}, {28, 165, 192, 246, 350, -1},
    {82, 165, 192, 246, 223, -1},  {0, 165, 192, 246, 347, -1},  {28, 165, 192, 246, 385, -1},
    {63, 169, 196, 335, 358, 0},
};

/* Hand out the race or profession's six-item starting set and then the extra
   item the profession's own skill pair selects, and settle the character's
   modifiers once the equipment is on. */
// FUNCTION: WIZ8 0x00557430
void AddCharacterStartingEquipment(W8Character* character)
{
    W8ItemInstance item;
    unsigned int slot;
    unsigned int set;

    EmptyAllCarriedItems(character);

    set = character->race == 5 ? 15 : character->current_profession;
    for (slot = 0; slot < 6; ++slot) {
        if (g_starting_equipment_61635c[set][slot] == -1) {
            continue;
        }
        ReplaceOrCreateItem(&item, g_starting_equipment_61635c[set][slot], 1, 1, 1);
        AddItemToCharacter(character, &item, 1, 0, 0);
    }

    switch (character->current_profession) {
    case 0:
        if (character->skills[1].level > character->skills[0].level) {
            ReplaceOrCreateItem(&item, 0x12, 1, 1, 1);
        } else {
            ReplaceOrCreateItem(&item, 7, 1, 1, 1);
        }
        break;
    case 8:
        ReplaceOrCreateItem(&item, 599, 1, 1, 1);
        break;
    case 9:
        ReplaceOrCreateItem(&item, 0x144, 1, 1, 1);
        break;
    case 10:
        if (character->skills[5].level > character->skills[3].level) {
            ReplaceOrCreateItem(&item, 0x16, 1, 1, 1);
        } else {
            ReplaceOrCreateItem(&item, 0x52, 1, 1, 1);
        }
        break;
    default:
        RebuildEquipmentAndDerivedStats(character);
        return;
    }
    AddItemToCharacter(character, &item, 1, 0, 0);
    RebuildEquipmentAndDerivedStats(character);
}
