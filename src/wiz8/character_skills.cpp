#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/character.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/magic.h"
#include "wiz8/screen_state.h"

// FUNCTION: WIZ8 0x00558610
void Function558610(W8Character* character)
{
    character->table_value_0079 = -1;
    character->unknown_007d = -1;
    character->personality_0081 = -1;
    Function4EFA30(character);
    CalcCharacterTableValue(character);
}

/* Profession and race trait-id sets. Values are the retail table contents at
   0x0061507C (fifteen triples) and 0x00615130 (eleven quintuples); -1 is no
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
W8RaceAbilitySet g_race_abilities[11] = {
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
};

// GLOBAL: WIZ8 0x00615270
W8RaceResistanceProfile g_race_resistance_profiles[11] = {
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
};

/* Whether the character has the trait through profession, race, or - for
   trait 0x1c - a set second enchantment slot. Profession id -1 and race id
   -1 both mean absent and skip their tables. Read-only, so callers agree
   on a const character. */
// FUNCTION: WIZ8 0x00547940
unsigned char Function547940(const W8Character* character, int trait)
{
    unsigned int index;

    if (character == 0) {
        return 0;
    }
    if (character->current_profession != -1) {
        const int* abilities =
            g_profession_abilities[character->current_profession].ability_ids;
        for (index = 0; index < 3; ++index) {
            if (abilities[index] == trait) {
                return 1;
            }
        }
    }
    if (character->race != -1) {
        const int* abilities = g_race_abilities[character->race].ability_ids;
        for (index = 0; index < 5; ++index) {
            if (abilities[index] == trait) {
                return 1;
            }
        }
    }
    if (trait == 0x1c && character->enchantments[1].value_08 != 0) {
        return 1;
    }
    return 0;
}

/* Skill ids fall into three bands. Below 0x18 and at 0x1c..0x21 they are
   ordinary skills resolved against the profession; 0x18..0x1b are the magic
   realms, gated by the profession's magic-level offset; 0x22..0x28 index the
   attribute records instead, and count as available only once the attribute has
   reached its cap. */
// FUNCTION: WIZ8 0x00553d90
unsigned char IsCharacterSkillAvailable(
    W8Character* character,
    unsigned int skill_id,
    const unsigned char* expert_realm_flags)
{
    int profession;
    unsigned int index;
    int magic_offset;

    if (g_profession_skill_availability[skill_id][character->current_profession] == 0) {
        return 0;
    }
    if (Function547940(character, 0x1f)) {
        if (skill_id >= 0x18 && skill_id <= 0x1b) {
            return 0;
        }
        if (skill_id >= 0x1c && skill_id <= 0x21) {
            return 0;
        }
    }
    if (skill_id >= 0x1c && skill_id <= 0x21) {
        for (index = 0x18; index <= 0x1b; ++index) {
            if (character->skills[index].flag_00) {
                break;
            }
        }
        if (index > 0x1b) {
            return 0;
        }
        if (character->skill_unlocks[skill_id] > 0) {
            return 1;
        }
        if (expert_realm_flags && expert_realm_flags[skill_id - 0x1c]) {
            return 1;
        }
        return 0;
    }

    profession = character->current_profession;
    if (skill_id != (unsigned int)g_profession_bonus_skills[profession]) {
        for (index = 0; index < 4; ++index) {
            if (skill_id == (unsigned int)g_profession_skills[profession][index]) {
                return 1;
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
    return 1;
}

/* Average the two attribute values g_skill_attributes names for every skill
   into the skill's 0x0a base level. 0x00557D80 and 0x00557B20 seed the
   profession skill levels from these. */
// FUNCTION: WIZ8 0x00553c90
void Function553C90(W8Character* character)
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
void Function553CD0(W8Character* character)
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
        unsigned char available =
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
unsigned int Function553EE0(W8Character* character, int skill_id)
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
#include "wiz8/character.h"
#include "wiz8/layouts/gameplay_databases.h"
extern "C" {
// GLOBAL: WIZ8 0x006161dc
int g_profession_bonus_skills[15];
// GLOBAL: WIZ8 0x00615570
float g_profession_hit_point_factors[15];
// GLOBAL: WIZ8 0x00616310
int g_profession_magic_level_offsets[15];
// GLOBAL: WIZ8 0x00615840
int g_profession_skill_availability[0x29][15];
// GLOBAL: WIZ8 0x00616218
int g_profession_skills[15][4];
// GLOBAL: WIZ8 0x006155b0
W8SkillAttributes g_skill_attributes[0x29];
}
