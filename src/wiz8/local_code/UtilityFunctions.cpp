#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdarg.h>
#include <ctype.h>
#include <float.h>
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
extern wchar_t g_empty_wide_string[];

static __inline int UtilityIntegerPower(int base, unsigned int exponent)
{
    int result;

    for (result = 1; exponent > 0; --exponent) {
        result *= base;
    }
    return result;
}

// FUNCTION: WIZ8 0x00517950
void SetDice(W8Dice* dice, unsigned char count, unsigned char sides, short base)
{
    dice->count = count;
    dice->sides = sides;
    dice->base = base;
}

// FUNCTION: WIZ8 0x00517970
int RollDice(const W8Dice* dice)
{
    unsigned int roll;
    int result = dice->base;

    for (roll = 0; roll < dice->count; ++roll) {
        result += Random(dice->sides) + 1;
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

// FUNCTION: WIZ8 0x00517A10
int CompareUnsignedDescending(const unsigned int* first, const unsigned int* second)
{
    unsigned int left = *first;
    unsigned int right = *second;

    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00517A30
int CompareSignedAscending(const int* first, const int* second)
{
    int left = *first;
    int right = *second;

    if (left < right) {
        return -1;
    }
    if (left > right) {
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00517A50
int CompareSignedDescending(const int* first, const int* second)
{
    int left = *first;
    int right = *second;

    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
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

// FUNCTION: WIZ8 0x00517AF0
wchar_t* FormatUnsignedIntegerWithCommas(wchar_t* output, unsigned int value)
{
    bool first_group = true;
    wchar_t group[10];
    unsigned int divisor;
    int exponent;

    wcscpy(output, g_empty_wide_string);
    exponent = 9;
    do {
        unsigned int threshold;

        divisor = UtilityIntegerPower(10, exponent);
        threshold = exponent > 0 ? divisor : 0;

        if (value >= threshold) {
            unsigned int group_value;

            if (!first_group) {
                wcscat(output, L",");
            }
            group_value = value / divisor;
            swprintf(group, first_group ? L"%d" : L"%03d", group_value);
            wcscat(output, group);
            value -= divisor * group_value;
            first_group = false;
        } else if (!first_group) {
            wcscat(output, L",000");
        }
        exponent -= 3;
    } while (exponent >= 0);
    return output;
}

// FUNCTION: WIZ8 0x00517BD0
char* TitleCaseString(char* string)
{
    bool capitalize = true;
    char* cursor = string;

    while (*cursor != '\0') {
        if (capitalize) {
            *cursor = static_cast<char>(toupper(*cursor));
        } else {
            *cursor = static_cast<char>(tolower(*cursor));
        }

        switch (*cursor) {
        case ' ':
        case '&':
        case '\'':
        case '(':
        case '*':
        case '-':
        case '.':
        case '2':
        case '?':
            capitalize = true;
            break;
        default:
            capitalize = false;
            break;
        }
        ++cursor;
    }
    return string;
}

static __forceinline float NormalizeAngleInline(float angle)
{
    if (!_finite(angle)) {
        srAssertFail(
            "_finite(flAngle)",
            "C:\\Projects\\Wizardry 8\\Local Code\\UtilityFunctions.cpp",
            0x13b,
            0);
    }

    angle += 6.2831852;
    while (angle < 0.0f) {
        angle += 6.2831852f;
    }
    while (angle >= 6.2831852f) {
        angle -= 6.2831852f;
    }
    return angle;
}

// FUNCTION: WIZ8 0x00517C60
float NormalizeAngle(float angle)
{
    return NormalizeAngleInline(angle);
}

// FUNCTION: WIZ8 0x00517CE0
float ShortestAngleDistance(float first, float second)
{
    float forward = NormalizeAngleInline(first - second);
    float backward = NormalizeAngleInline(second - first);

    return forward < backward ? forward : backward;
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
void StripMonsterNameSuffix(W8WideChar* name)
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
            if ((character->hp_current > 0 && character->unknown_0b01 < 0x12)
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

// FUNCTION: WIZ8 0x00518230
int GetNextCharacter(int require_primary, int require_secondary, int previous_slot)
{
    int start_slot = (previous_slot + 1) % 8;
    W8Character* characters = g_party_characters;
    W8PartySlotRow* rows = g_party_slot_rows;
    int slot;
    unsigned int scanned;

retry:
    slot = start_slot;
    scanned = 0;

    do {
        if (rows[slot].flag_00 != 0) {
            W8Character* character = &characters[slot];

            if ((character->hp_current > 0 && character->unknown_0b01 < 0x12)
                || require_primary == 2) {
                if (character->unknown_0b01 < 0xf || require_secondary == 2) {
                    return slot;
                }
            }
        }
        ++slot;
        if (slot == 8) {
            slot = 0;
        }
        ++scanned;
    } while (scanned <= 8);

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

// FUNCTION: WIZ8 0x005182E0
void FormatDebugMessage(int channel, const char* format, ...)
{
    char message[200];
    va_list arguments;

    (void)channel;
    va_start(arguments, format);
    vsprintf(message, format, arguments);
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
