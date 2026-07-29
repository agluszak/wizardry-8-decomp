#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 00401001-0041ab3f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00404C70
void DeleteFileByName(LPCSTR path)
{
    DeleteFileA(path);
}
// FUNCTION: WIZ8 0x00405720
void CompareFileTimes(const FILETIME* left, const FILETIME* right)
{
    CompareFileTime(left, right);
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
