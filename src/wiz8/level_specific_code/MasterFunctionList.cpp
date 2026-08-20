#include "wiz8/unattributed/quarantine_common.h"
#include "wiz8/fact_state.h"

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

/* Master-function values 0x10 and 0x26 select the same path once either of
   the two enabling facts has been set. */
// FUNCTION: WIZ8 0x004D9700
int NormalizeMasterFunctionValue004D9700(int value)
{
    if (value == 0x10 || value == 0x26) {
        if (GetFact(0x5b) == 0 && GetFact(0x1ce) == 0) {
            return 0x26;
        }
        value = 0x10;
    }
    return value;
}
