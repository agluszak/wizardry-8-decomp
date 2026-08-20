#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 00526e91-0052a88f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00529560
void SetFlag68C4F4(void)
{
    g_flag_68c4f4 = 1;
}
// FUNCTION: WIZ8 0x00529BC0
void SetFlag68C4F7(void)
{
    g_flag_68c4f7 = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearFlag68C4F7(void)
{
    g_flag_68c4f7 = 0;
}
// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_flag_68c4fa;
}
// FUNCTION: WIZ8 0x0052A1A0
void SetFlag68C500(unsigned char value)
{
    g_flag_68c500 = value;
}

/* Rebuilds the hit-point ceiling from scratch every time it is called: each
   profession the character has levels in contributes its own per-level factor,
   scaled by a figure derived from the fourth attribute record's effective
   value, and the profession the character started in counts one level more
   than it has taken. The running total lives in the x87 stack across the whole
   loop, which is why the zero it starts from is loaded before the profession
   guard and discarded by an `fstp` on the early return.
   The level is unsigned - the emitted test is `jbe`, not `jle` - and the
   attribute is widened through a zeroed high dword, which is the unsigned
   conversion rather than the signed one.
   Losing the last hit point applies condition 0x12 with the ceiling duration
   the party notice uses, which is the one place this writes anything beyond
   the two pools. */
// FUNCTION: WIZ8 0x0052A2F0
void RecalculateCharacterHitPoints(W8Character* character)
{
    double total = 0.0;
    int profession;
    int hit_points;
    int remaining;

    if (character->current_profession == -1) {
        return;
    }

    for (profession = 0; profession < 15; profession++) {
        unsigned int levels = character->profession_levels[profession];
        if (profession == character->original_profession) {
            levels++;
        }
        if (levels > 0) {
            double vitality = character->attributes[3].effective * 0.4;
            total += (vitality * 0.02 + 0.6) *
                g_profession_hit_point_factors[profession] * levels;
        }
    }

    hit_points = (int)(total + 0.5) + character->hp_adjustment;
    if (hit_points < 1) {
        hit_points = 1;
    }
    if (hit_points != character->hp_max) {
        remaining = (hit_points - character->hp_max) + (int)character->hp_current;
        if (remaining < 0) {
            remaining = 0;
        }
        character->hp_max = hit_points;
        character->hp_current = remaining;
        if (remaining == 0) {
            SetCharacterCondition(
                CharacterPointerToPartySlot(character), 0x12, 9999, 0, 0, 1);
        }
    }
}

// FUNCTION: WIZ8 0x0052a760
int SumCharacterSpellPoints(const W8Character* character)
{
    int total = 0;
    for (int realm = 0; realm < W8_SPELL_REALM_COUNT; ++realm) {
        total += character->sp_max[realm];
    }
    return total;
}
