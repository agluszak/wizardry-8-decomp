#include "gameplay_boundaries.h"

extern unsigned int GetRandomNumber(unsigned int upper_bound);
extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);

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

// FUNCTION: WIZ8 0x00535AD0
W8FactionDisposition GetFactionDisposition(signed char faction)
{
    signed char disposition_score;

    if (faction < 0) {
        srAssertFail(
            "bFaction >= 0",
            "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp",
            0xaf,
            0);
    }
    if (faction >= 21) {
        srAssertFail(
            "bFaction < FACTION_COUNT",
            "C:\\Projects\\Wizardry 8\\Local Code\\Factions.cpp",
            0xb0,
            0);
    }

    disposition_score = g_factions[faction].disposition_score;
    if (disposition_score < 34) {
        return W8_FACTION_HOSTILE;
    }
    if (disposition_score < 67) {
        return W8_FACTION_NEUTRAL;
    }
    return W8_FACTION_FRIENDLY;
}
