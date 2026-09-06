#include "wiz8/character.h"
#include "wiz8/layouts/gameplay_databases.h"

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
// FUNCTION: WIZ8 0x00551a60
void RecalculateCharacterResistances(W8Character* character)
{
    unsigned int index;
    int channel;
    unsigned int adjustment;

    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        W8CharacterResistance* resistance = &character->resistances[index];

        resistance->base = 25;
        resistance->base =
            character->skills[W8_FIRST_RESISTANCE_SKILL + index].level / 10 + 25;
        if (character->skills[W8_RESISTANCE_BONUS_SKILL].flag_00 != 0) {
            resistance->base +=
                character->skills[W8_RESISTANCE_BONUS_SKILL].level / 5 + 5;
        }
        if (character->current_profession == 14) {
            resistance->base += 5;
        }
    }

    if (character->race != -1) {
        for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
            channel = g_race_resistance_profiles[character->race]
                          .adjustments[index].resistance_index;
            if (channel == -1) {
                break;
            }
            adjustment = g_race_resistance_profiles[character->race]
                             .adjustments[index].adjustment_or_attribute;
            if (static_cast<int>(adjustment) > W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS) {
                adjustment =
                    character->attributes[adjustment - W8_RACE_ADJUSTMENT_ATTRIBUTE_BIAS]
                        .value / 5;
            }
            character->resistances[channel].base += adjustment;
        }
    }

    if (character->attributes[1].effective > 0x50) {
        character->resistances[4].base +=
            (character->attributes[1].effective - 0x50) >> 1;
    }
    if (character->attributes[2].effective > 0x50) {
        character->resistances[5].base +=
            (character->attributes[2].effective - 0x50) >> 1;
    }

    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        W8CharacterResistance* resistance = &character->resistances[index];

        resistance->total = resistance->base;
        resistance->total = resistance->base + character->resistance_bonus_all;
        resistance->total += character->resistance_bonus[index];
    }
    for (index = 0; index < W8_RESISTANCE_COUNT; ++index) {
        if (character->resistances[index].total > 100) {
            character->resistances[index].total = 100;
        }
    }
}
#include "wiz8/character.h"
#include "wiz8/layouts/gameplay_databases.h"
