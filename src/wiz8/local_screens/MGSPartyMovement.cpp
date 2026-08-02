#include "wiz8/unattributed/quarantine_common.h"

// GLOBAL: WIZ8 0x0069BF40
Controls* g_panel_69bf40;

// FUNCTION: WIZ8 0x005A1DD0
void RedrawPanel69BF4C(void)
{
    g_panel_69bf4c->Invalidate(0);
}
// FUNCTION: WIZ8 0x005A1E90
void RedrawPanel69BF40(void)
{
    g_panel_69bf40->Invalidate(0);
}

// FUNCTION: WIZ8 0x005A1EA0
void EnablePanel69BF40005A1EA0(void)
{
    g_panel_69bf40->SetEnabled(1);
}
