#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

extern "C" char* String(const char* format, ...);
extern W8RPCSlot g_rpc_slots[8];
extern W8RPCSlot g_rpc_slots_end[];
extern int g_string_table_count;
extern char** g_string_table;
extern int g_string_table_state;
extern char g_format_string_buffer[];
extern wchar_t g_wide_string_buffer[];

// FUNCTION: WIZ8 0x005179F0
void ClampUnsignedInteger(unsigned int* value, unsigned int minimum,
                          unsigned int maximum)
{
    if (*value > maximum) {
        *value = maximum;
    } else if (*value < minimum) {
        *value = minimum;
    }
}

// FUNCTION: WIZ8 0x00517A70
char* FormatString(const char* format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vsprintf(g_format_string_buffer, format, arguments);
    return g_format_string_buffer;
}

// FUNCTION: WIZ8 0x00517A90
wchar_t* FormatWideString(const wchar_t* format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    vswprintf(g_wide_string_buffer, format, arguments);
    return g_wide_string_buffer;
}

// FUNCTION: WIZ8 0x00517AB0
wchar_t* ConvertStringToWide(const char* string)
{
    swprintf(g_wide_string_buffer, L"%hs", string);
    return g_wide_string_buffer;
}

// FUNCTION: WIZ8 0x00517AD0
char* ConvertWideStringToString(const wchar_t* string)
{
    sprintf(reinterpret_cast<char*>(g_wide_string_buffer), "%ls", string);
    return reinterpret_cast<char*>(g_wide_string_buffer);
}

// FUNCTION: WIZ8 0x00517E20
void UnionScreenRects(const W8ScreenRect* first, const W8ScreenRect* second,
                      W8ScreenRect* result)
{
    result->left = first->left < second->left ? first->left : second->left;
    result->top = first->top < second->top ? first->top : second->top;
    result->right = first->right > second->right ? first->right : second->right;
    result->bottom = first->bottom > second->bottom ? first->bottom : second->bottom;
}

// FUNCTION: WIZ8 0x00517E70
unsigned char ScreenPointInRect(const W8ScreenRect* rect, const W8ScreenPoint* point)
{
    if (rect != 0 && point != 0 && point->x >= rect->left && point->x < rect->right
        && point->y >= rect->top && point->y < rect->bottom) {
        return 1;
    }
    return 0;
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
        String("PCPtrToPCSlot: ERROR - no match on ptr %d", character));
    return 0;
}

// FUNCTION: WIZ8 0x00517F30
unsigned char IsPartyCharacterPointer(const W8Character* character)
{
    W8Character* party_character = g_party_characters;
    unsigned int slot;

    for (slot = 0; slot < 8; ++slot, ++party_character) {
        if (character == party_character) {
            return 1;
        }
    }

    return 0;
}

// FUNCTION: WIZ8 0x00517F60
void AdjustByteByPercent(unsigned char* value, unsigned int percent)
{
    *value = (unsigned char)(((percent + 100) * *value + 50) / 100);
}

// FUNCTION: WIZ8 0x00517F90
void AdjustIntegerByPercent(unsigned int* value, unsigned int percent)
{
    *value += *value * percent / 100;
}

// FUNCTION: WIZ8 0x00518310
int RPCPtrToPCSlot(const W8RPCSlot* rpc)
{
    int slot = 0;

    for (W8RPCSlot* current = g_rpc_slots; current < g_rpc_slots_end; ++current) {
        if (rpc == current) {
            return slot;
        }
        ++slot;
    }
    char* message = String("RPCPtrToPCSlot: ERROR - no match on ptr %d", rpc);
    srAssertFail(
        "FALSE",
        "C:\\Projects\\Wizardry 8\\Local Code\\UtilityFunctions.cpp",
        0x385,
        message);
    return 0;
}

// FUNCTION: WIZ8 0x005184B0
void FreeStringTable(void)
{
    if (g_string_table != 0) {
        int index = 0;
        char** table = g_string_table;
        if (g_string_table_count > 0) {
            do {
                if (table[index] != 0) {
                    free(table[index]);
                    table = g_string_table;
                }
                ++index;
            } while (index < g_string_table_count);
        }
        free(table);
        g_string_table = 0;
        g_string_table_count = 0;
    }
}

// FUNCTION: WIZ8 0x00518B20
bool IsStringTableLoaded(void)
{
    bool loaded;

    loaded = g_string_table_state != 0;
    return loaded;
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
    skip = Random(8);
    scanned = 0;
    slot = 0;
    do {
        matched = 0;
        if (g_party_slot_rows[slot].flag_00 != 0 && (int)slot != excluded_slot) {
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
