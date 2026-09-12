#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/character.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"

// FUNCTION: WIZ8 0x00558610
void InvalidateAndRecalculateCharacterClassData00558610(W8Character* character)
{
    character->table_value_0079 = -1;
    character->unknown_007d = -1;
    character->personality_0081 = -1;
    DeriveCharacterPersonality004EFA30(character);
    CalcCharacterTableValue(character);
}

/* Profession and race trait-id sets. Values are the retail table contents at
   0x0061507C (fifteen triples) and 0x00615130 (sixteen quintuples); -1 is no
   trait. */
// GLOBAL: WIZ8 0x0061507C
W8ProfessionAbilitySet g_profession_abilities[15] = {
    {{0, 19, 20}},
    {{1, -1, -1}},
    {{2, -1, -1}},
    {{4, 12, -1}},
    {{3, 16, -1}},
    {{5, 21, -1}},
    {{6, 7, -1}},
    {{9, -1, -1}},
    {{8, -1, -1}},
    {{10, -1, -1}},
    {{11, 17, -1}},
    {{18, -1, -1}},
    {{13, 17, -1}},
    {{3, 14, -1}},
    {{15, -1, -1}},
};

// GLOBAL: WIZ8 0x00615130
W8RaceAbilitySet g_race_abilities[16] = {
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{29, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{22, 26, 23, 25, 24}},
    {{27, -1, -1, -1, -1}},
    {{28, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{-1, -1, -1, -1, -1}},
    {{30, 31, -1, -1, -1}},
};

// GLOBAL: WIZ8 0x00615270
W8RaceResistanceProfile g_race_resistance_profiles[16] = {
    {{{-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 20}, {2, 10}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{0, 1003}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 1003}, {3, 10}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 1003}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{2, 15}, {3, 15}, {4, 15}, {5, 15}, {-1, 0}, {-1, 0}}},
    {{{1, 10}, {3, 10}, {0, 15}, {5, -10}, {4, -10}, {-1, 0}}},
    {{{1, 15}, {2, 5}, {5, -5}, {4, -5}, {-1, 0}, {-1, 0}}},
    {{{1, -15}, {3, 10}, {2, 10}, {4, 10}, {-1, 0}, {-1, 0}}},
    {{{1, 10}, {3, 5}, {5, 15}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{5, 10}, {1, 15}, {4, 15}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 20}, {2, 20}, {4, -10}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 15}, {1, 15}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{3, 15}, {2, 15}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{0, 45}, {4, -20}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
    {{{4, 75}, {5, -25}, {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0}}},
};

/* Whether the character has the trait through profession, race, or - for
   trait 0x1c - a set second enchantment slot. Profession id -1 and race id
   -1 both mean absent and skip their tables. Read-only, so callers agree
   on a const character. */
// FUNCTION: WIZ8 0x00547940
bool CharacterHasTrait00547940(const W8Character* character, int trait)
{
    unsigned int index;

    if (character == 0) {
        return false;
    }
    if (character->current_profession != -1) {
        const int* abilities =
            g_profession_abilities[character->current_profession].ability_ids;
        for (index = 0; index < 3; ++index) {
            if (abilities[index] == trait) {
                return true;
            }
        }
    }
    if (character->race != -1) {
        const int* abilities = g_race_abilities[character->race].ability_ids;
        for (index = 0; index < 5; ++index) {
            if (abilities[index] == trait) {
                return true;
            }
        }
    }
    if (trait == 0x1c && character->enchantments[1].value_08 != 0) {
        return true;
    }
    return false;
}

/* Scale a trait's flat value by the character's level in their current
   profession: full value above twenty levels, sixty percent at zero and two
   percent per level in between. The trait id is carried by the call but the
   body never reads it. */
// FUNCTION: WIZ8 0x005479b0
float ScaleValueByProfessionLevel005479B0(W8Character* character, int, float base)
{
    unsigned int level = character->profession_levels[character->current_profession];

    if (level > 0x14) {
        return base;
    }
    return (level + level + 60.0f) * base * 0.01f;
}

/* Skill ids fall into three bands. Below 0x18 and at 0x1c..0x21 they are
   ordinary skills resolved against the profession; 0x18..0x1b are the magic
   realms, gated by the profession's magic-level offset; 0x22..0x28 index the
   attribute records instead, and count as available only once the attribute has
   reached its cap. */
// FUNCTION: WIZ8 0x00553d90
bool IsCharacterSkillAvailable(
    W8Character* character,
    unsigned int skill_id,
    const unsigned char* expert_realm_flags)
{
    int profession;
    unsigned int index;
    int magic_offset;

    if (g_profession_skill_availability[skill_id][character->current_profession] == 0) {
        return false;
    }
    if (CharacterHasTrait00547940(character, 0x1f)) {
        if (skill_id >= 0x18 && skill_id <= 0x1b) {
            return false;
        }
        if (skill_id >= 0x1c && skill_id <= 0x21) {
            return false;
        }
    }
    if (skill_id >= 0x1c && skill_id <= 0x21) {
        for (index = 0x18; index <= 0x1b; ++index) {
            if (character->skills[index].flag_00) {
                break;
            }
        }
        if (index > 0x1b) {
            return false;
        }
        if (character->skill_unlocks[skill_id] > 0) {
            return true;
        }
        if (expert_realm_flags && expert_realm_flags[skill_id - 0x1c]) {
            return true;
        }
        return false;
    }

    profession = character->current_profession;
    if (skill_id != (unsigned int)g_profession_bonus_skills[profession]) {
        for (index = 0; index < 4; ++index) {
            if (skill_id == (unsigned int)g_profession_skills[profession][index]) {
                return true;
            }
        }
        if (skill_id >= 0x22 && skill_id <= 0x28) {
            return character->attributes[skill_id - 0x22].value >= 100;
        }
        /* The canonical emits a byte index table over 0x00..0x1b, placed after
           the body, with three groups: default for 0x00..0x09 and 0x12, a
           middle band of 0x0a..0x11 and 0x13..0x17, and the magic realms. Its
           first and third groups resolve to the same address, so the middle
           band is a real case group whose body merely returns 1 like the
           default. Neither an empty break nor an explicit return 1 reproduces
           the table: VC6 drops the empty group and range-tests the remainder,
           and returning 1 merges the two bands into one range compare that
           costs 41 bytes more. */
        switch (skill_id) {
        case 0x0a:
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x0e:
        case 0x0f:
        case 0x10:
        case 0x11:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            break;
        case 0x18:
        case 0x19:
        case 0x1a:
        case 0x1b:
            magic_offset = g_profession_magic_level_offsets[profession];
            if (magic_offset < 0 && magic_offset > -0xff) {
                return character->profession_levels[profession] + magic_offset > 0;
            }
            break;
        }
    }
    return true;
}

/* Rebuild each attribute's effective value from its base and the modifier
   block's seven adjustment bytes, clamped to one through 125, and then every
   skill's base level from the attribute pair g_skill_attributes names. The
   equipment refresh runs after each attribute so its use checks see the new
   value. */
// FUNCTION: WIZ8 0x005539e0
void ResetCharacterAttributes005539E0(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 7; ++index) {
        int value =
            static_cast<signed char>(character->bonus_1770.unknown_0c[index]) +
            character->attributes[index].value;
        if (value > 0x7d) {
            value = 0x7d;
        }
        else if (value < 1) {
            value = 1;
        }
        character->attributes[index].effective = value;
        Function51D960(character);
    }
    for (index = 0; index < 0x29; ++index) {
        int first = g_skill_attributes[index].unknown_04;
        int second = g_skill_attributes[index].unknown_08;
        character->skills[index].base_level_0a =
            (character->attributes[first].value +
             character->attributes[second].value) >> 1;
    }
}

/* Rebuild every skill level from the value already spent on it, the
   profession's bonus skill and the race and profession adjustment bytes the
   modifier block carries, clamped to zero through 125. The equipment refresh
   runs after each skill for the same reason. */
// FUNCTION: WIZ8 0x00553a60
void ResetCharacterSkills00553A60(W8Character* character)
{
    unsigned int index;

    for (index = 0; index < 0x29; ++index) {
        int value = character->skills[index].value_02;
        if (index == (unsigned int)g_profession_bonus_skills[character->current_profession]) {
            unsigned int bonus = (unsigned int)(value * 0x19) / 100;
            if (bonus == 0) {
                bonus = 1;
            }
            value += bonus;
        }
        value += static_cast<signed char>(character->bonus_1770.unknown_13[index]);
        if (value > 0x7d) {
            value = 0x7d;
        }
        else if (value < 0) {
            value = 0;
        }
        character->skills[index].level = value;
        Function51D960(character);
    }
}

/* Average the two attribute values g_skill_attributes names for every skill
   into the skill's 0x0a base level. 0x00557D80 and 0x00557B20 seed the
   profession skill levels from these. */
// FUNCTION: WIZ8 0x00553c90
void InitializeSkillBaseLevels00553C90(W8Character* character)
{
    for (int index = 0; index < 0x29; ++index) {
        int first = g_skill_attributes[index].unknown_04;
        int second = g_skill_attributes[index].unknown_08;
        character->skills[index].base_level_0a =
            (character->attributes[first].value +
             character->attributes[second].value) >> 1;
    }
}

/* Re-scan every skill's availability and mirror each flag change onto the
   open character screen's page 2. The realm flags array marks which expert
   skills gained a spell since the last scan. */
// FUNCTION: WIZ8 0x00553cd0
void RefreshCharacterSkillAvailability00553CD0(W8Character* character)
{
    unsigned char expert_realm_flags[8];
    int index;

    for (index = 0; index < 8; ++index) {
        expert_realm_flags[index] = 0;
    }
    for (index = 0; index < 0x72; ++index) {
        if (character->spell_learned[index + 1] == -1 ||
            character->spell_learned[index + 1] == 2) {
            expert_realm_flags[g_spell_records[index].realm] = 1;
        }
    }
    for (index = 0; index < 0x29; ++index) {
        bool available =
            IsCharacterSkillAvailable(character, index, expert_realm_flags);
        if (!available) {
            if (character->skills[index].flag_00) {
                character->skills[index].flag_00 = 0;
                if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                    Function5B1B30(index);
                }
            }
        }
        else if (!character->skills[index].flag_00) {
            character->skills[index].flag_00 = 1;
            if (g_current_screen_state.id == W8_SCREEN_CHARACTER) {
                Function5B1AF0(index);
            }
        }
    }
}

/* A quarter of the skill's current value, never below one. The bonus skill's
   level gets this added after the profession assignment. */
// FUNCTION: WIZ8 0x00553ee0
unsigned int GetSkillQuarterValue00553EE0(W8Character* character, int skill_id)
{
    unsigned int value =
        (character->skills[skill_id].value_02 * 0x19) / 100;
    if (value == 0) {
        value = 1;
    }
    return value;
}

/* Rebuilds all six resistance channels from scratch.
 
   The base of each starts at a flat 25 plus a tenth of the matching skill, then
   takes a flat bonus derived from skill 36 when the character has it, and five
   more for profession 14. The race table adds its own adjustments next: a value
   at or below 1000 is a flat amount, and anything above it names a character
   attribute whose fifth is added instead. Two attributes feed two specific
   channels directly, each contributing half of whatever it carries above 80.
 
   The total is then base plus the character's flat all-resistance bonus plus
   the per-channel one, and only the total is clamped - the base is left as
   computed, which is why a subsequent pass over the same character produces the
   same answer rather than compounding. */
/* The profession databases and per-skill attribute records. Contents are the
   retail tables at 0x00615570..0x0061634c; the five profession arrays share
   one contiguous block with the skill-attribute records. -1 in a skill slot
   means no skill, and -255 in a magic offset means the profession casts no
   spells at all. */
// GLOBAL: WIZ8 0x00615570
float g_profession_hit_point_factors[15] = {
    9.0f, 7.5f, 7.5f, 6.5f, 7.0f, 6.5f, 6.5f, 6.0f, 5.5f, 6.0f, 5.0f, 4.5f, 4.0f, 3.5f, 3.0f,
};
// GLOBAL: WIZ8 0x006155b0
W8SkillAttributes g_skill_attributes[0x29] = {
    {0, 1, 0, 4},
    {0, 1, 0, 4},
    {0, 1, 0, 4},
    {0, 1, 0, 4},
    {0, 1, 4, 5},
    {0, 0, 0, 4},
    {0, 1, 0, 4},
    {0, 1, 4, 5},
    {0, 1, 4, 0},
    {0, 0, 4, 0},
    {1, 1, 4, 1},
    {1, 1, 4, 1},
    {1, 2, 4, 1},
    {1, 1, 4, 5},
    {0, 1, 4, 5},
    {1, 2, 6, 1},
    {2, 0, 6, 1},
    {2, 0, 6, 1},
    {2, 1, 4, 6},
    {2, 1, 6, 5},
    {2, 0, 1, 6},
    {2, 0, 6, 1},
    {2, 0, 1, 6},
    {2, 2, 1, 4},
    {3, 1, 1, 1},
    {3, 1, 2, 2},
    {3, 1, 4, 1},
    {3, 1, 6, 1},
    {3, 1, 1, 2},
    {3, 1, 1, 2},
    {3, 1, 1, 2},
    {3, 1, 1, 2},
    {3, 1, 1, 2},
    {3, 1, 1, 2},
    {4, 3, 0, 0},
    {4, 3, 1, 1},
    {4, 3, 2, 2},
    {4, 3, 3, 3},
    {4, 3, 4, 4},
    {4, 3, 5, 5},
    {4, 3, 6, 6},
};
// GLOBAL: WIZ8 0x00615840
int g_profession_skill_availability[0x29][15] = {
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1},
    {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0},
    {0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
};
// GLOBAL: WIZ8 0x006161dc
int g_profession_bonus_skills[15] = {
    16, 18, 2, 17, 0, 19, 14, 10, 7, 22, 25, 26, 20, 27, 24,
};
// GLOBAL: WIZ8 0x00616218
int g_profession_skills[15][4] = {
    {17, 0, 1, 6},
    {16, 0, 4, -1},
    {16, 21, 1, -1},
    {15, 8, 21, -1},
    {16, 18, 19, -1},
    {16, 14, 9, 11},
    {16, 19, 5, 11},
    {4, 18, 13, 11},
    {17, 23, 10, -1},
    {12, 21, 20, -1},
    {3, 5, 22, -1},
    {21, 9, -1, -1},
    {26, 24, 25, 27},
    {22, 21, 32, -1},
    {28, 29, 30, 32},
};
// GLOBAL: WIZ8 0x00616310
int g_profession_magic_level_offsets[15] = {
    -255, -4, -4, -4, -4, -4, -4, -255, -255, -255, 0, 0, 0, 0, 0,
};
