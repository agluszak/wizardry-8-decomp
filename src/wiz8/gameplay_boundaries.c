#include "gameplay_boundaries.h"

extern unsigned int GetRandomNumber(unsigned int upper_bound);

static __inline int MinimumCasterLevel(int spell_level)
{
    switch (spell_level) {
    case 2:
        return 3;
    case 3:
        return 5;
    case 4:
        return 8;
    case 5:
        return 11;
    case 6:
        return 14;
    case 7:
        return 18;
    default:
        return 1;
    }
}

// FUNCTION: WIZ8 0x00517970
int RollDice(const W8Dice* dice)
{
    unsigned int roll;
    int result = dice->base;

    for (roll = 0; roll < dice->count; ++roll) {
        result += GetRandomNumber(dice->sides) + 1;
    }
    return result;
}

// FUNCTION: WIZ8 0x005179B0
int IntegerPower(int base, unsigned int exponent)
{
    int result;

    for (result = 1; exponent > 0; --exponent) {
        result *= base;
    }
    return result;
}

// FUNCTION: WIZ8 0x005179D0
void ClampInteger(int* value, int minimum, int maximum)
{
    int current = *value;

    if (current > maximum) {
        *value = maximum;
    } else if (current < minimum) {
        *value = minimum;
    }
}

// FUNCTION: WIZ8 0x004AC9D0
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target)
{
    int target_type = g_spell_records[spell_id].target_type;

    if (target_type == 1 && normalize_single_target) {
        target_type = 0;
    }
    return target_type;
}

// FUNCTION: WIZ8 0x004ACB40
int MinimumCasterLevelForSpellLevel(int spell_level)
{
    return MinimumCasterLevel(spell_level);
}

// FUNCTION: WIZ8 0x004ACBA0
int GetMinimumCasterLevelForSpell(int spell_id)
{
    return MinimumCasterLevel(g_spell_records[spell_id].spell_level);
}
