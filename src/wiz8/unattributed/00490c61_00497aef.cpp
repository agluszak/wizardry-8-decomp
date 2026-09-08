#include "wiz8/world_cursor.h"

extern "C" float g_float_60ab48;
extern "C" {
// GLOBAL: WIZ8 0x0060ab48
float g_float_60ab48 = 4000.0f;
}

/* Address quarantine 00490c61-00497aef; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x00492530
void SetFloat60AB48(void)
{
    g_float_60ab48 = 4000.0f;
}

// FUNCTION: WIZ8 0x004914C0
unsigned char IsWorldCursorVisible(void)
{
    return g_world_cursor_0065ba8c != 0 &&
           g_world_cursor_0065ba8c->visible_40 != 0;
}
