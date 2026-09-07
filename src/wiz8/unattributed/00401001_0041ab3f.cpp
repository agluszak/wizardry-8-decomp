#include "english.h"
#include "input.h"
#include "wiz8/utility.h"

/* Address quarantine 00401001-0041ab3f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00402780
unsigned short Function402780(unsigned short key, unsigned char modifiers)
{
    if ((modifiers & (CTRL_DOWN | ALT_DOWN)) != 0) return 0;
    if ((modifiers & SHIFT_DOWN) != 0)
        return gsKeyTranslationTable[key + 256];
    return gsKeyTranslationTable[key];
}

// FUNCTION: WIZ8 0x00402800
unsigned short Function402800(unsigned short character)
{
    return character > L'@' && character < L'[';
}

// FUNCTION: WIZ8 0x00402820
unsigned short Function402820(unsigned short character)
{
    return character > L'`' && character < L'{';
}

// FUNCTION: WIZ8 0x00402840
unsigned short Function402840(unsigned short character)
{
    return (character >= L'!' && character <= L'/') ||
           (character >= L':' && character <= L'@') ||
           (character >= L'[' && character <= L'_') ||
           (character >= L'{' && character <= L'}');
}

// FUNCTION: WIZ8 0x00402880
int Function402880(int character)
{
    if ((unsigned short)character > L'`' &&
        (unsigned short)character < L'{') {
        character -= L'a' - L'A';
    }
    return character;
}

// FUNCTION: WIZ8 0x004028A0
int Function4028A0(int character)
{
    if ((unsigned short)character > L'@' &&
        (unsigned short)character < L'[') {
        character += L'a' - L'A';
    }
    return character;
}

/* Unlike the pinned VC6 _wcsicmp, retail has no locale branch. The adjacent
   character helpers provide the same ASCII-only case conversion. */
// FUNCTION: WIZ8 0x00402920
int CompareWideTextIgnoreAsciiCase00402920(const wchar_t* first, const wchar_t* second)
{
    unsigned short left;
    unsigned short right;
    do {
        left = Function4028A0(*first++);
        right = Function4028A0(*second++);
    } while (left != 0 && left == right);
    return static_cast<unsigned int>(left) - static_cast<unsigned int>(right);
}
