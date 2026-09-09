#include "english.h"
#include "input.h"
#include "wiz8/utility.h"

#include <stdlib.h>
#include <string.h>

extern unsigned char g_flag_650dac;

/* Address quarantine 00401001-0041ab3f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// GLOBAL: WIZ8 0x00650f9c
int g_surface_pitch_00650f9c;
// GLOBAL: WIZ8 0x00650fa0
int g_surface_clip_left_00650fa0;
// GLOBAL: WIZ8 0x00650fa4
int g_surface_clip_right_00650fa4;
// GLOBAL: WIZ8 0x00650fa8
int g_surface_clip_top_00650fa8;
// GLOBAL: WIZ8 0x00650fac
int g_surface_clip_bottom_00650fac;

// FUNCTION: WIZ8 0x00413FD0
void SetSurfaceClipBounds00413FD0(
    int pitch, int left, int top, int width, int height)
{
    g_surface_pitch_00650f9c = pitch;
    g_surface_clip_left_00650fa0 = left;
    g_surface_clip_right_00650fa4 = left + width - 1;
    g_surface_clip_top_00650fa8 = top;
    g_surface_clip_bottom_00650fac = top + height - 1;
}

// GLOBAL: WIZ8 0x006505ac
char g_error_message_006505ac[0x800];

/* Record a fatal error message for the window procedure, then leave. The
   message is truncated into the shared buffer with its terminator forced. */
// FUNCTION: WIZ8 0x00401920
void ReportError00401920(const char* message)
{
    strncpy(g_error_message_006505ac, message, 0x7ff);
    g_error_message_006505ac[0x7ff] = 0;
    g_flag_650dac = 1;
    exit(0);
}

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
