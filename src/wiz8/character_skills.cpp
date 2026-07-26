#include "wiz8/gameplay_boundaries.h"

/* 0x00547940, not yet identified; asked here whether trait 0x1f applies. */
extern unsigned char Function547940(W8Character* character, int trait_id);

// FUNCTION: WIZ8 0x00553D90
/* Skill ids fall into three bands. Below 0x18 and at 0x1c..0x21 they are
   ordinary skills resolved against the profession; 0x18..0x1b are the magic
   realms, gated by the profession's magic-level offset; 0x22..0x28 index the
   attribute records instead, and count as available only once the attribute has
   reached its cap. */
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
