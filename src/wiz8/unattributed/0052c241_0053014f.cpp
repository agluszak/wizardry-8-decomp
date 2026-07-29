#include "wiz8/unattributed/quarantine_common.h"

/* Address quarantine 0052c241-0053014f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

// FUNCTION: WIZ8 0x0052E360
bool IsFlag6850FCSet(void)
{
    return g_flag_6850fc != 0xff;
}
