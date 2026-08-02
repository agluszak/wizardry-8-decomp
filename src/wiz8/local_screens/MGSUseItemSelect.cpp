#include "wiz8/unattributed/quarantine_common.h"

// GLOBAL: WIZ8 0x0069B9A0
int g_value_69b9a0;
// GLOBAL: WIZ8 0x0069B9A4
int g_value_69b9a4;

// FUNCTION: WIZ8 0x0059CF30
void SetValue69B988(int value)
{
    g_value_69b988 = value;
}
// FUNCTION: WIZ8 0x0059CF40
void RedrawPanel69B998(void)
{
    g_panel_69b998->Invalidate(0);
}

// FUNCTION: WIZ8 0x0059E0D0
int GetSelectedOrFallbackValue0059E0D0(void)
{
    int value = g_value_69b9a4;
    if (value == 0) {
        value = g_value_69b9a0;
    }
    return value;
}
