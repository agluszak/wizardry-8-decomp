#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 0047a791-0047b4ff; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0047AE70
unsigned char GetFlag6850F6(void)
{
    return g_flag_6850f6;
}
// FUNCTION: WIZ8 0x0047AE80
bool IsFlag6850FASet(void)
{
    return g_flag_6850fa != 0xff;
}
