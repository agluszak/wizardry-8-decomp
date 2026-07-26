#include "gameplay_boundaries.h"

#include <wchar.h>

extern __declspec(dllimport) void srAssertFail(
    const char* expression,
    const char* source_path,
    int line,
    const char* message);
extern char* FormatDiagnostic(const char* format, ...);

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

// FUNCTION: WIZ8 0x00518150
int GetRandomCharacter(int require_primary, int require_secondary, int excluded_slot,
                       signed char excluded_faction)
{
    int skip;
    unsigned int slot;
    unsigned int scanned;
    unsigned char matched;
    W8Character* character;

retry:
    skip = GetRandomNumber(8);
    scanned = 0;
    slot = 0;
    do {
        matched = 0;
        if (g_party_slot_rows[slot][0] != 0 && (int)slot != excluded_slot) {
            character = &g_party_characters[slot];
            if ((character->unknown_0b11 > 0 && character->unknown_0b01 < 0x12)
                || require_primary == 2) {
                if (excluded_faction == -1 || excluded_faction != character->faction) {
                    if (character->unknown_0b01 < 0xf || require_secondary == 2) {
                        matched = 1;
                        if (skip == 0) {
                            return slot;
                        }
                        skip--;
                        scanned = 0;
                    }
                }
            }
        }
        slot++;
        if (slot == 8) {
            slot = 0;
        }
        scanned++;
    } while (scanned <= 8);

    if (matched) {
        return slot;
    }
    if (require_secondary == 1) {
        require_secondary = 2;
        goto retry;
    }
    if (require_primary == 1) {
        require_primary = 2;
        goto retry;
    }
    return -1;
}
