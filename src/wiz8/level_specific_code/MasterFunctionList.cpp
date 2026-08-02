#include "wiz8/unattributed/quarantine_common.h"

/* Predicate installed in the master-function callback table. */
// FUNCTION: WIZ8 0x004D95F0
unsigned char IsMasterFunctionTypeEight004D95F0(int type)
{
    return type == 8;
}

// FUNCTION: WIZ8 0x004D96F0
void ClearValue6834D4(void)
{
    g_value_6834d4 = 0;
}
