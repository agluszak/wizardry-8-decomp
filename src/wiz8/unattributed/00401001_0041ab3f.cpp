#include "wiz8/unattributed/quarantine_common.h"

#include "english.h"
#include "input.h"

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

// FUNCTION: WIZ8 0x00404C70
void DeleteFileByName(LPCSTR path)
{
    DeleteFileA(path);
}
// FUNCTION: WIZ8 0x00405720
int CompareSGPFileTimes(FILETIME* left, FILETIME* right)
{
    return CompareFileTime(left, right);
}
// FUNCTION: WIZ8 0x00407210
unsigned char SetValue5FF5F0(int value)
{
    g_dword_5ff5f0 = value;
    return 1;
}
// FUNCTION: WIZ8 0x0040A8A0
int GetMilesDigitalDriver0040A8A0(void)
{
    return g_value_6e4104;
}
// FUNCTION: WIZ8 0x0040C220
void ClearDisplayFlag650E90(void)
{
    g_display_flag_650e90 = 0;
}
extern "C" {
// FUNCTION: WIZ8 0x00411820
void GetClipRect(int* rect)
{
    rect[0] = g_clip_left_600078;
    rect[1] = g_clip_top_60007c;
    rect[2] = g_clip_right_600080;
    rect[3] = g_clip_bottom_600084;
}
}
