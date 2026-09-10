#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/fact_state.h"

extern "C" int g_value_6834d4;
extern "C" {
// GLOBAL: WIZ8 0x006834d4
int g_value_6834d4;
}

// GLOBAL: WIZ8 0x006834d8
W8GrowableVector<W8MasterFunction>* g_master_functions_006834d8;
// GLOBAL: WIZ8 0x006834dc
unsigned char g_flag_006834dc;

/* Run every registered master function once with argument zero, dropping the
   ones that set the removal flag while it runs. */
// FUNCTION: WIZ8 0x004D8E40
void RunMasterFunctions004D8E40(void)
{
    int count = g_master_functions_006834d8->GetCount();

    for (int index = 0; index < count; ++index) {
        (*g_master_functions_006834d8->GetAt(index))(0);
        if (g_flag_006834dc != 0) {
            g_master_functions_006834d8->RemoveAt(index);
            --count;
            --index;
        }
    }
}

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
