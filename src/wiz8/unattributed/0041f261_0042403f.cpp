#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 0041f261-0042403f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

/* Two whole-body reads through the pointer at 0x0065A0F8. The first hands back
   the twelve bytes at 0x8C as one block; the second converts the float at 0x04
   from radians to degrees and truncates it through the CRT's _ftol, which the
   original reaches as a tail jump because the conversion is the whole return
   value. The scale is one ULP above the float nearest 180/pi, so the original
   spelled it as a decimal literal rather than computing it from a pi constant;
   the literal here is the shortest decimal that reproduces the stored bytes. */
// FUNCTION: WIZ8 0x00421070
void GetPosition421070(W8Position* position)
{
    *position = g_object_65a0f8->position_8c;
}

// FUNCTION: WIZ8 0x00421550
int GetAngleInDegrees421550(void)
{
    return (int)(g_object_65a0f8->angle_04 * 57.295784f);
}

// FUNCTION: WIZ8 0x00421F30
IDirectDraw2* GetDirectDraw2(void)
{
    return g_direct_draw2_6596a0;
}
