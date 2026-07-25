#include "gameplay_boundaries.h"

#include <wchar.h>
#include <stdio.h>

extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);
extern char* FormatDiagnostic(const char* format, ...);
extern unsigned char EvaluateFact(int fact_id);
extern void WriteGameLog(int channel, const wchar_t* format, ...);
/* Provisional semantic name for the journal/notification path at 0x005588f0. */
extern void RecordFactChangeForJournal(int fact_id);
extern void HandleFactChange(int fact_id, unsigned char value);

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

// FUNCTION: WIZ8 0x00517EA0
void StripMonsterNameSuffix(unsigned short* name)
{
    wchar_t* suffix = wcschr((wchar_t*)name, L'#');

    if (suffix != 0) {
        *suffix = L'\0';
    }
}

// FUNCTION: WIZ8 0x00517EC0
unsigned int CharacterPointerToPartySlot(W8Character* character)
{
    unsigned int slot;
    W8Character* party_character;

    if (!character->in_party) {
        srAssertFail(
            "pPC->fInParty",
            "C:\\Projects\\Wizardry 8\\Local Code\\UtilityFunctions.cpp",
            0x1c8,
            "PCPtrToPCSlot: ERROR - called for non-party character");
    }

    party_character = g_party_characters;
    for (slot = 0; slot < 8; ++slot, ++party_character) {
        if (character == party_character) {
            return slot;
        }
    }

    srAssertFail(
        "FALSE",
        "C:\\Projects\\Wizardry 8\\Local Code\\UtilityFunctions.cpp",
        0x1d1,
        FormatDiagnostic("PCPtrToPCSlot: ERROR - no match on ptr %d", character));
    return 0;
}

// FUNCTION: WIZ8 0x004FF3B0
int GetProfessionCasterLevel(W8Character* character, int profession_id)
{
    int magic_level_offset;

    if (profession_id == -1) {
        profession_id = character->current_profession;
        if (profession_id == -1) {
            srAssertFail(
                "iProfession != -1",
                "C:\\Projects\\Wizardry 8\\Local Code\\Magic.cpp",
                0xe13,
                0);
        }
    }

    magic_level_offset = g_profession_magic_level_offsets[profession_id];
    if (magic_level_offset == -255) {
        return -1;
    }
    return character->profession_levels[profession_id] + magic_level_offset;
}

// FUNCTION: WIZ8 0x00506280
unsigned char GetFact(int fact_id)
{
    unsigned char value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return 0;
    }

    value = EvaluateFact(fact_id);
    if (g_log_fact_checks) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        WriteGameLog(
            5,
            L"Checking fact %S which is %s",
            g_fact_records[fact_id].symbolic_name,
            display_value);
    }
    return value;
}

// FUNCTION: WIZ8 0x005061A0
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects)
{
    unsigned char previous_value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return;
    }

    previous_value = g_fact_values[fact_id];
    g_fact_values[fact_id] = (unsigned char)value;

    if (fact_id < g_fact_record_count) {
        if (value) {
            sprintf((char*)display_value, "TRUE");
        } else {
            sprintf((char*)display_value, "FALSE");
        }
    }

    if (!suppress_side_effects) {
        if (g_fact_values[fact_id] != previous_value) {
            RecordFactChangeForJournal(fact_id);
        }
        HandleFactChange(fact_id, value);

        if (g_log_fact_checks) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            WriteGameLog(
                5,
                L"%S set to %s",
                g_fact_records[fact_id].symbolic_name,
                display_value);
        }
    }
}
