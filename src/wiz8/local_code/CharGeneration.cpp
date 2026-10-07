#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/dialog_code/ProfRaceInfoDialog.h"
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

    if (GetProfessionCasterLevel(edited, W8_PROFESSION_NONE) != 0) {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (!original->skills[index].active) {
                continue;
            }
            if (edited->skills[index].active) {
                continue;
            }
            total += original->skill_costs[index - 0x18];
        }
        return total;
    }
    if (GetProfessionCasterLevel(original, W8_PROFESSION_NONE) < 1) {
        return 0;
    }
    {
        int index;

        for (index = 0x18; index <= 0x1b; ++index) {
            if (!original->skills[index].active) {
                continue;
            }
            total += original->skill_costs[index - 0x18];
        }
        total += original->magic_bonus_pool;
    }
    return total;
}

/* Character generation state. The four dwords below accumulate the pools a
   reset or level-up hands to the editing state; the byte at 0x34 records that
   a profession change forced the sex to one. */
// GLOBAL: WIZ8 0x0068de28
static int g_attribute_point_bonus;
// GLOBAL: WIZ8 0x0068de2c
static int g_skill_point_bonus;
// GLOBAL: WIZ8 0x0068de30
static int g_spell_point_bonus;
// GLOBAL: WIZ8 0x0068de34
static bool g_gender_locked;

static void InitializeCharacterPointPools(W8Character* character,
                                          W8CharacterCreationState* creation_state)
{
    int points = 0;
    if (character->iProfession != W8_PROFESSION_NONE && character->iRace != W8_RACE_NONE) {
        points = 0x3c + g_attribute_point_bonus;
        if (character->uiExpLevel != 1) {
            points -= 0x36;
        }
        if (points < 0) {
            points = 0;
        }
    }
    creation_state->attribute_points_total = points;
    creation_state->attribute_points_remaining = points;
    RecomputeAttributeLimits(character, creation_state);

    points = 0xf + g_skill_point_bonus;
    if (character->uiExpLevel != 1) {
        points -= 6;
    }
    if (points < 0) {
        points = 0;
    }
    creation_state->skill_points_total = points;
    creation_state->skill_points_remaining = points;
    RecomputeSkillLimits(character, creation_state);
}

/* A fresh creation: zero the character and the editing state, install the
   sentinel values, and hand out the level-one pools. */
// FUNCTION: WIZ8 0x00556dc0
void InitializeCharacterCreation(W8Character* character, W8CharacterCreationState* creation_state)
{
    g_attribute_point_bonus = 0;
    g_skill_point_bonus = 0;
    g_spell_point_bonus = 0;
    g_gender_locked = false;
    memset(character, 0, sizeof(*character));
    character->gender = W8_GENDER_UNSET;
    character->iProfession = W8_PROFESSION_NONE;
    character->iRace = W8_RACE_NONE;
    character->uiExpLevel = 1;
    character->level_band_base = 0;
    character->highest_condition = W8_CONDITION_NONE;
    character->enchantment_top = W8_ENCHANTMENT_NONE;
    character->portrait_index = -1;
    character->unknown_007d = -1;
    character->personality = -1;
    memset(creation_state, 0, sizeof(*creation_state));
    EmptyAllCarriedItems(character);

    InitializeCharacterPointPools(character, creation_state);
}

/* One level gained: raise the level and profession counter, reset the editing
   state, and re-derive every pool the character's new level entitles them
   to. */
// FUNCTION: WIZ8 0x00556cc0
void InitializeCharacterLevelUp(W8Character* character, W8CharacterCreationState* creation_state)
{
    g_attribute_point_bonus = 0;
    g_skill_point_bonus = 0;
    g_spell_point_bonus = 0;
    g_gender_locked = false;
    ++character->uiExpLevel;
    ++character->profession_levels[character->iProfession];
    memset(creation_state, 0, sizeof(*creation_state));

    InitializeCharacterPointPools(character, creation_state);

    if (character->attribute_point_deficit < 0) {
        PayDownAttributeDebt(character, creation_state);
    }
    CalcCharacterLevelBand(character);
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    FinalizeSpellPointPool(character, creation_state);
}

/* Reset one skill's contribution to the editing state: set its reset byte,
   refund its spent points, and let a realm skill that was just opened up reach its
   minimum five. */
// FUNCTION: WIZ8 0x00557c90
void ResetSkillContribution(W8Character* character, W8CharacterCreationState* creation_state,
                            W8Skill skill_id)
{
    /* Retail stores to byte 1 of the skill record (0x00557CAB), not to the
       active flag at byte 0. */
    character->skills[skill_id].reset_flag = true;
    creation_state->skill_points_spent[skill_id] = 0;

    if (skill_id >= W8_SKILL_SPELLBOOK_WIZARDRY && skill_id < W8_SKILL_FIRE_MAGIC) {
        int offset = g_profession_magic_level_offsets[character->iProfession];
        if (offset < 0 && offset > -0xff &&
            character->profession_levels[character->iProfession] + offset == 1) {
            int missing = 5 - character->skills[skill_id].points;
            if (missing > 0) {
                character->skills[skill_id].points += missing;
                character->skills[skill_id].level += missing;
                creation_state->skill_baselines[skill_id] += missing;
            }
        }
    }
}

/* Refund every point spent on one skill and leave its level at the base
   value the assignment gave it. */
// FUNCTION: WIZ8 0x00557d20
void RefundSkillAllocation(W8Character* character, W8CharacterCreationState* creation_state,
                           W8Skill skill_id)
{
    int spent = creation_state->skill_points_spent[skill_id];
    if (spent > 0) {
        creation_state->skill_points_remaining += spent;
        character->skills[skill_id].points -= spent;
        creation_state->skill_points_spent[skill_id] = 0;
    }
    character->skills[skill_id].level = character->skills[skill_id].points;
}

/* Every point the edited character has already committed is spent from the
   pool in one burst. */
// FUNCTION: WIZ8 0x00557ae0
void RefundAllocatedAttributes(W8Character* character, W8CharacterCreationState* creation_state)
{
    for (int index = 0; index < 7; ++index) {
        if (creation_state->attribute_values[index] > 0) {
            AdjustAllocatedAttribute(character, creation_state, static_cast<W8Attribute>(index),
                                     -creation_state->attribute_values[index]);
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
            points += creation_state->attribute_baselines[index];
        }
        step = (points + 2) / 3;
        if (step < 3) {
            step = points < 4 ? points : 3;
        }
    }
    creation_state->attribute_step_limit = step;

    for (index = 0; index < 7; ++index) {
        int limit = step - creation_state->attribute_baselines[index];
        int cap =
            (creation_state->attribute_values[index] - character->attributes[index].base) + 100;
        int value = limit;
        if (cap <= limit) {
            value = cap;
        }
        if (value < 0) {
            limit = 0;
        } else if (cap <= limit) {
            limit = cap;
        }
        creation_state->attribute_limits[index] = limit;
    }

    ClampAttributesToBudget(character, creation_state);
    if (creation_state->attribute_points_remaining > 0) {
        for (index = 0; index < 7; ++index) {
            if (creation_state->attribute_values[index] < creation_state->attribute_limits[index]) {
                creation_state->attributes_complete = false;
                return;
            }
        }
    }
    creation_state->attributes_complete = true;
}

/* Clamp the allocated attribute values to their limits, then take points back
   round-robin while the character is over budget. */
// FUNCTION: WIZ8 0x00557800
void ClampAttributesToBudget(W8Character* character, W8CharacterCreationState* creation_state)
{
    int index;

    for (index = 0; index < 7; ++index) {
        int value = creation_state->attribute_values[index];
        int limit = creation_state->attribute_limits[index];
        if (value > limit) {
            creation_state->attribute_values[index] = value - (value - limit);
        }
    }

    int total = 0;
    for (index = 0; index < 7; ++index) {
        total += creation_state->attribute_values[index];
    }
    if (total > 0 && creation_state->attribute_points_total > 0 &&
        creation_state->attribute_points_total < total) {
        index = 0;
        while (creation_state->attribute_points_total < total) {
            if (creation_state->attribute_values[index] > 0) {
                --creation_state->attribute_values[index];
                --character->attributes[index].base;
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

    character->attribute_point_deficit = 0;
    for (index = 0; index < 7; ++index) {
        int deficit = g_profession_attribute_minimums[character->iProfession].values[index] -
                      character->attributes[index].base;
        deficits[index] = deficit;
        if (deficit > 0) {
            character->attribute_point_deficit -= deficit;
        }
    }

    if (creation_state->attribute_points_total + character->attribute_point_deficit < 0) {
        PayDownAttributeDebt(character, creation_state);
        return;
    }

    character->attribute_point_deficit = 0;
    character->level_band_base = 0;
    for (index = 0; index < 7; ++index) {
        int deficit = deficits[index];
        if (deficit < 1) {
            if (character->uiExpLevel > 1) {
                int minimum = g_profession_attribute_minimums[character->iProfession].values[index];
                int value = creation_state->attribute_values[index];
                while (character->attributes[index].base - value <
                       static_cast<unsigned int>(minimum)) {
                    creation_state->attribute_values[index] = value - 1;
                    --creation_state->attribute_points_total;
                    ++creation_state->attribute_baselines[index];
                    value = creation_state->attribute_values[index];
                }
            }
        } else {
            character->attributes[index].base += deficit;
            character->attributes[index].effective += deficit;
            creation_state->attribute_points_total -= deficit;
            if (character->uiExpLevel > 1) {
                creation_state->attribute_baselines[index] = deficit;
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
    int deficits[7][2];
    int index;

    int total = 0;
    for (index = 0; index < 7; ++index) {
        unsigned int minimum =
            g_profession_attribute_minimums[character->iProfession].values[index];
        int deficit = 0;
        if (character->attributes[index].base < minimum) {
            deficit = minimum - character->attributes[index].base;
        }
        deficits[index][0] = deficit;
        deficits[index][1] = index;
        total -= deficit;
    }
    qsort(deficits, 7, 8, CompareSignedDescending);
    if (character->attribute_point_deficit != total) {
        character->attribute_point_deficit = total;
    }
    index = 0;
    while (character->attribute_point_deficit < 0 && creation_state->attribute_points_total > 0) {
        if (deficits[index][0] < 1 ||
            (index != 6 && deficits[index][0] <= deficits[index + 1][0])) {
            index = (index + 1) % 7;
        } else {
            int attribute = deficits[index][1];
            ++character->attributes[attribute].base;
            ++character->attributes[attribute].effective;
            total = deficits[index][0];
            ++character->attribute_point_deficit;
            int available = creation_state->attribute_points_total;
            deficits[index][0] = total - 1;
            creation_state->attribute_points_total = available - 1;
            index = 0;
        }
    }
    if (character->attribute_point_deficit >= 0) {
        if (creation_state->attribute_points_total < creation_state->attribute_points_remaining) {
            creation_state->attribute_points_remaining = creation_state->attribute_points_total;
        }
        return;
    }
    creation_state->attribute_points_total = character->attribute_point_deficit;
    character->level_band_base = character->uiExpLevel;
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

    /* 0x00557416 compares the loop counter against 15 with a signed jump. */
    for (int profession = 0; profession < W8_PROFESSION_COUNT; ++profession) {
        eligibility[profession] = 1;
        if (profession == character->iProfession) {
            eligibility[profession] = 1;
        } else if (profession == W8_PROFESSION_VALKYRIE && character->gender != W8_GENDER_FEMALE) {
            eligibility[W8_PROFESSION_VALKYRIE] = 0;
        } else {
            int deficit = 0;
            for (attribute = 0; attribute < 7; ++attribute) {
                int difference = g_profession_attribute_minimums[profession].values[attribute] -
                                 character->attributes[attribute].base;
                if (difference > 0) {
                    deficit -= difference;
                }
            }
            if (creation_state->attribute_points_total + deficit < 0) {
                eligibility[profession] = 0;
            }
            for (attribute = 0; attribute < 7; ++attribute) {
                if (static_cast<unsigned int>(creation_state->attribute_limits[attribute] -
                                              creation_state->attribute_values[attribute]) +
                        character->attributes[attribute].base <
                    static_cast<unsigned int>(
                        g_profession_attribute_minimums[profession].values[attribute])) {
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
                              W8Attribute attribute, int modifier)
{
    int value = creation_state->attribute_values[attribute];
    if (value + modifier < 0) {
        if (modifier >= 0) {
            srAssertFail("iModifier < 0", CHAR_GENERATION_CPP, 0x380, 0);
        }
        modifier = -value;
    }
    if (modifier > creation_state->attribute_points_remaining) {
        modifier = creation_state->attribute_points_remaining;
    }
    if (value + modifier > creation_state->attribute_limits[attribute]) {
        modifier = creation_state->attribute_limits[attribute] - value;
    }
    character->attributes[attribute].base += modifier;
    character->attributes[attribute].effective += modifier;
    creation_state->attribute_points_remaining -= modifier;
    creation_state->attribute_values[attribute] += modifier;

    if (creation_state->attribute_points_remaining > 0) {
        for (int index = 0; index < 7; ++index) {
            if (creation_state->attribute_values[index] < creation_state->attribute_limits[index]) {
                creation_state->attributes_complete = false;
                goto complete;
            }
        }
    }
    creation_state->attributes_complete = true;
complete:
    RecalculateCharacterHitPoints(character);
    RecalculateCharacterStamina(character);
    RecalculateCarryingCapacity(character);
    RecalculateCarriedWeight(character);
    RecalculateRealmSpellPoints(character);
    CalcArmorClasses(character);
    RecalculateCharacterResistances(character);
    if (character->uiExpLevel == 1) {
        RebuildSkillAllocations(character, creation_state);
    }
    CountRemainingSpellPoints(character, creation_state);
    RefreshCharacterSkillAvailability(character);
    RecomputeSkillLimits(character, creation_state);
}

/* Skill limits from the pool plus the base levels the attribute pairs give
   every skill. */
// FUNCTION: WIZ8 0x00557b20
void RecomputeSkillLimits(W8Character* character, W8CharacterCreationState* creation_state)
{
    InitializeSkillBaseLevels(character);
    int step = (creation_state->skill_points_total + 2) / 3;
    creation_state->skill_step_limit = step;

    for (int index = 0; index < 0x29; ++index) {
        if (!character->skills[index].active) {
            creation_state->skill_limits[index] = 0;
            continue;
        }
        int limit =
            (creation_state->skill_points_spent[index] - character->skills[index].points) + 0x4b;
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
            character->skills[index].points -= excess;
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
                --character->skills[index].points;
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
                creation_state->skills_complete = false;
                return;
            }
        }
    }
    creation_state->skills_complete = true;
}

/* Finalize the spell-point pool the level-up flow presents: settle the realm
   skills against the spent spell points, then trim any learned spell the
   character can no longer hold. */
// FUNCTION: WIZ8 0x00558070
void FinalizeSpellPointPool(W8Character* character, W8CharacterCreationState* creation_state)
{
    int spent = creation_state->spell_points_total - creation_state->spell_points_remaining;
    if (GetProfessionCasterLevel(character, W8_PROFESSION_NONE) < 1) {
        creation_state->spell_points_total = 0;
    } else {
        if (character->uiExpLevel == 1) {
            creation_state->magic_skill_bonus = 2;
        } else {
            creation_state->magic_skill_bonus = 1;
        }
        creation_state->magic_skill_bonus += g_spell_point_bonus;
        int total = 0;
        if (GetProfessionCasterLevel(character, W8_PROFESSION_NONE) > 0) {
            for (unsigned int realm = 0x18; realm < 0x1c; ++realm) {
                if (character->skills[realm].active) {
                    total += character->skill_costs[realm - 0x18];
                }
            }
            total += character->magic_bonus_pool;
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
        creation_state->spells_complete = false;
        return;
    }
    creation_state->spells_complete = true;
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
            character->skills[skill].points -= points;
            character->skills[skill].level -= points;
        }
        for (index = 0; index < 6; ++index) {
            int skill = 0x1c + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].points -= points;
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
            character->skills[skill].points += points;
            character->skills[skill].level += points;
        }
        for (index = 0; index < 6; ++index) {
            int skill = 0x1c + index;
            int points = creation_state->skill_points_spent[skill];
            character->skills[skill].points += points;
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
        creation_state->spells_complete = false;
        return available;
    }
    creation_state->spells_complete = true;
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
        character->skills[index].points = 0;
        character->skills[index].level = 0;
    }
    InitializeSkillBaseLevels(character);

    W8Profession profession = character->iProfession;
    int count = 1;
    for (index = 0; index < 4; ++index) {
        if (g_profession_skills[profession][index] != -1) {
            ++count;
        }
    }
    int step = 0x3c / count;
    if (creation_state->attribute_points_total < 0) {
        step -= static_cast<unsigned int>(creation_state->attribute_points_total * step * -2) / 100;
    }

    for (index = 0; index < 4; ++index) {
        W8Skill skill = g_profession_skills[profession][index];
        if (skill != W8_SKILL_NONE) {
            int value = (character->skills[skill].base_level * step) / 100;
            character->skills[skill].points = value;
            character->skills[skill].level = value;
        }
    }
    W8Skill bonus_skill = g_profession_bonus_skills[profession];
    int value = (character->skills[bonus_skill].base_level * step) / 100;
    character->skills[bonus_skill].points = value;
    character->skills[bonus_skill].level = value;
    for (index = 0; index < 0x29; ++index) {
        character->skills[index].points += creation_state->skill_points_spent[index];
        character->skills[index].level = character->skills[index].points;
    }
    character->skills[bonus_skill].level += GetSkillQuarterValue(character, bonus_skill);
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

    if (character->uiExpLevel <= 1) {
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

/* Spend one creation spell point on a pick: the spell goes to selected, and
   either end of the linked 0x49/0x4b pair drags its partner along unless the
   character already knows it. An emptied pool marks the page complete. */
// FUNCTION: WIZ8 0x005584e0
void SelectCreationSpell(W8Character* character, W8CharacterCreationState* creation_state,
                         unsigned int spell)
{
    if (creation_state->spell_points_remaining > 0) {
        character->spell_learned[spell] = 2;
        --creation_state->spell_points_remaining;
        if (spell == 0x4b) {
            if (character->spell_learned[0x49] != 1) {
                character->spell_learned[0x49] = 2;
            }
        } else if (spell == 0x49) {
            if (character->spell_learned[0x4b] != 1) {
                character->spell_learned[0x4b] = 2;
            }
        }
    }
    if (creation_state->spell_points_remaining == 0) {
        creation_state->spells_complete = true;
    }
    RecalculateRealmSpellPoints(character);
}

/* Return one creation spell pick: the spell goes to declined, an auto-added
   partner of the 0x49/0x4b pair follows only while it is still marked
   selected, and the point goes back to the pool. */
// FUNCTION: WIZ8 0x00558560
void DeselectCreationSpell(W8Character* character, W8CharacterCreationState* creation_state,
                           unsigned int spell)
{
    if (character->spell_learned[spell] != 0) {
        character->spell_learned[spell] = -1;
        if (spell == 0x4b) {
            if (character->spell_learned[0x49] == 2) {
                character->spell_learned[0x49] = -1;
            }
        } else if (spell == 0x49) {
            if (character->spell_learned[0x4b] == 2) {
                character->spell_learned[0x4b] = -1;
            }
        }
        ++creation_state->spell_points_remaining;
        creation_state->spells_complete = false;
        RecalculateRealmSpellPoints(character);
    }
}

/* Back out the in-progress spell picks before the pool is rebuilt: every
   tentative selection (state two) returns to unlearned, the point pool is
   topped back up and the page is marked incomplete. */
// FUNCTION: WIZ8 0x005585D0
void ResetSpellSelections(W8Character* character, W8CharacterCreationState* creation_state)
{
    for (int index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index] == 2) {
            character->spell_learned[index] = -1;
        }
    }
    creation_state->spell_points_remaining = creation_state->spell_points_total;
    creation_state->spells_complete = false;
}

/* Price the profession's starting gear: stage the equipment, sum every
   carried stack's value, then empty the character back out. */
// FUNCTION: WIZ8 0x00558640
int ComputeStartingEquipmentCost(W8Character* character)
{
    int total;
    int count;

    total = 0;
    AddCharacterStartingEquipment(character);
    for (count = 0; count < 0xc; ++count) {
        if (character->EquippedItem[count].iItemNo != -1) {
            total += GetItemStackValue(&character->EquippedItem[count]);
        }
    }
    for (count = 0; count < 8; ++count) {
        if (character->backpack[count].iItemNo != -1) {
            total += GetItemStackValue(&character->backpack[count]);
        }
    }
    EmptyAllCarriedItems(character);
    return total;
}

/* Whether the party's gold covers the new character's starting gear. */
// FUNCTION: WIZ8 0x005586b0
bool CanAffordStartingEquipment(W8Character* character)
{
    return static_cast<unsigned int>(ComputeStartingEquipmentCost(character)) <=
           g_status.party_gold;
}

/* Recompute the level-up pools after a profession change, refunding every
   baseline the previous profession's assignment had granted. */
// FUNCTION: WIZ8 0x00557060
void RebuildLevelUpPoolsForProfession(W8Character* character,
                                      W8CharacterCreationState* creation_state,
                                      W8Profession profession)
{
    int index;

    if (character->iProfession != W8_PROFESSION_NONE) {
        --character->profession_levels[character->iProfession];
    }
    ++character->profession_levels[profession];
    character->iProfession = profession;

    if (profession == W8_PROFESSION_VALKYRIE) {
        if (character->gender == W8_GENDER_MALE) {
            g_gender_locked = true;
            character->gender = W8_GENDER_FEMALE;
        }
    } else if (g_gender_locked) {
        g_gender_locked = false;
        character->gender = W8_GENDER_MALE;
    }

    if (character->uiExpLevel > 1) {
        for (index = 0; index < 7; ++index) {
            int baseline = creation_state->attribute_baselines[index];
            if (baseline > 0) {
                character->attributes[index].base -= baseline;
                character->attributes[index].effective -= baseline;
                creation_state->attribute_points_total += baseline;
                creation_state->attribute_baselines[index] = 0;
            }
        }
        for (index = 0; index < 0x29; ++index) {
            int baseline = creation_state->skill_baselines[index];
            if (baseline > 0) {
                character->skills[index].points -= baseline;
                character->skills[index].level -= baseline;
                creation_state->skill_baselines[index] = 0;
            }
        }
        W8Skill bonus_skill = g_profession_bonus_skills[character->iProfession];
        int base = character->skills[bonus_skill].points;
        int missing = (creation_state->skill_points_spent[bonus_skill] - base) + 5;
        if (missing > 0) {
            character->skills[bonus_skill].points = base + missing;
            character->skills[bonus_skill].level += missing;
            creation_state->skill_baselines[bonus_skill] += missing;
        }
        for (index = 0; index < 4; ++index) {
            W8Skill skill = g_profession_skills[character->iProfession][index];
            int value = character->skills[skill].points;
            int missing = (creation_state->skill_points_spent[skill] - value) + 5;
            if (missing > 0) {
                character->skills[skill].points = value + missing;
                character->skills[skill].level += missing;
                creation_state->skill_baselines[skill] += missing;
            }
        }
    }
    ApplyRaceProfessionTables(character, creation_state);
}

/* Assign the chosen race and rebuild the race/profession tables. */
// FUNCTION: WIZ8 0x005571c0
void SetCharacterRace(W8Character* character, W8CharacterCreationState* creation_state, W8Race race)
{
    character->iRace = race;
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
    if (character->iRace == W8_RACE_NONE) {
        if (character->iProfession != W8_PROFESSION_NONE) {
            for (index = 0; index < 7; ++index) {
                unsigned int minimum =
                    g_profession_attribute_minimums[character->iProfession].values[index];
                character->attributes[index].base = minimum;
                character->attributes[index].effective = minimum;
            }
        }
    } else {
        if (character->uiExpLevel == 1) {
            for (index = 0; index < 7; ++index) {
                unsigned int minimum = g_race_attribute_minimums[character->iRace].values[index];
                character->attributes[index].base = minimum;
                character->attributes[index].effective = minimum;
            }
        }
        if (character->iProfession != W8_PROFESSION_NONE) {
            if (character->uiExpLevel == 1) {
                int points = 0x3c + g_attribute_point_bonus;
                if (points < 0) {
                    points = 0;
                }
                creation_state->attribute_points_total = points;
                creation_state->attribute_points_remaining = points;
                RecomputeAttributeLimits(character, creation_state);
                character->original_profession = character->iProfession;
                ApplyProfessionMinimumAttributes(character, creation_state);
                CalcCharacterLevelBand(character);
                for (index = 0; index < 7; ++index) {
                    character->attributes[index].base += creation_state->attribute_values[index];
                }
            } else {
                ApplyProfessionMinimumAttributes(character, creation_state);
            }
            RefreshCharacterSkillAvailability(character);
            FinalizeSpellPointPool(character, creation_state);
            RecountLearnedSpellsByRealm(character);
            CountRemainingSpellPoints(character, creation_state);
            RefreshCharacterSkillAvailability(character);
            RecomputeSkillLimits(character, creation_state);
        }
        if (character->uiExpLevel == 1) {
            for (index = 0; index < 7; ++index) {
                character->attributes[index].effective = character->attributes[index].base;
            }
        }
        RecalculateCharacterHitPoints(character);
        RecalculateCharacterStamina(character);
        RecalculateCarryingCapacity(character);
        RecalculateCarriedWeight(character);
        RecalculateRealmSpellPoints(character);
        CalcArmorClasses(character);
    }
    RecalculateCharacterResistances(character);
    if (character->uiExpLevel == 1) {
        if (character->gender != W8_GENDER_UNSET && character->iProfession != W8_PROFESSION_NONE &&
            character->iRace != W8_RACE_NONE) {
            RebuildSkillAllocations(character, creation_state);
        }
        character->portrait_index = -1;
        character->unknown_007d = -1;
        character->personality = -1;
        character->voice = 0;
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
    character->skills[skill].points += modifier;
    character->skills[skill].level += modifier;
    creation_state->skill_points_remaining -= modifier;
    creation_state->skill_points_spent[skill] += modifier;

    if (creation_state->skill_points_remaining > 0) {
        for (int index = 0; index < 0x29; ++index) {
            if (creation_state->skill_points_spent[index] < creation_state->skill_limits[index]) {
                creation_state->skills_complete = false;
                goto complete;
            }
        }
    }
    creation_state->skills_complete = true;
complete:
    CountRemainingSpellPoints(character, creation_state);
    RefreshCharacterSkillAvailability(character);
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

    if (character->iProfession == W8_PROFESSION_BISHOP) {
        character->magic_bonus_pool += creation_state->magic_skill_bonus;
    } else {
        for (realm = 0x18; realm <= 0x1b; ++realm) {
            if (character->skills[realm].active) {
                character->skill_costs[realm - 0x18] += creation_state->magic_skill_bonus;
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
            if (character->skills[realm].active && character->skill_costs[realm - 0x18] > 0) {
                --character->skill_costs[realm - 0x18];
                goto spell_done;
            }
        }
        if (character->magic_bonus_pool > 0) {
            --character->magic_bonus_pool;
        } else if (character->iProfession == W8_PROFESSION_BISHOP) {
            for (realm = 0x18; realm <= 0x1b; ++realm) {
                if (character->skills[realm].active && character->skill_costs[realm - 0x18] > 0) {
                    --character->skill_costs[realm - 0x18];
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

    RebuildRealmSpellPointCeilings(character);
    character->experience_previous_goal = character->experience_goal;
    CalcXPGoal(character);
    CalcCharacterLevelBand(character);
}

/* The six starting item ids each profession hands out, with the faerie race's
   own row last; -1 is an empty slot. */
// GLOBAL: WIZ8 0x0061635c
int g_starting_equipment[0x10][6] = {
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

    set = character->iRace == W8_RACE_FAERIE ? 15 : character->iProfession;
    for (slot = 0; slot < 6; ++slot) {
        if (g_starting_equipment[set][slot] == -1) {
            continue;
        }
        ReplaceOrCreateItem(&item, g_starting_equipment[set][slot], true, true, true);
        AddItemToCharacter(character, &item, true, false, false);
    }

    switch (character->iProfession) {
    case W8_PROFESSION_PRIEST:
        if (character->skills[W8_SKILL_STAFF_WAND].level >
            character->skills[W8_SKILL_MACE_FLAIL].level) {
            ReplaceOrCreateItem(&item, 0x16, true, true, true);
        } else {
            ReplaceOrCreateItem(&item, 0x52, true, true, true);
        }
        break;
    case W8_PROFESSION_FIGHTER:
        if (character->skills[W8_SKILL_AXE].level > character->skills[W8_SKILL_SWORD].level) {
            ReplaceOrCreateItem(&item, 0x12, true, true, true);
        } else {
            ReplaceOrCreateItem(&item, 7, true, true, true);
        }
        break;
    case W8_PROFESSION_GADGETEER:
        ReplaceOrCreateItem(&item, 599, true, true, true);
        break;
    case W8_PROFESSION_BARD:
        ReplaceOrCreateItem(&item, 0x144, true, true, true);
        break;
    default:
        RebuildEquipmentAndDerivedStats(character);
        return;
    }
    AddItemToCharacter(character, &item, true, false, false);
    RebuildEquipmentAndDerivedStats(character);
}
