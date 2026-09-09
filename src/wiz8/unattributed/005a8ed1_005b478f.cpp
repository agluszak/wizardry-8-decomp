#include "wiz8/local_screens/MainGameScreen.h"

extern "C" {
extern int g_value_69c1cc;
// GLOBAL: WIZ8 0x0064d8ac
unsigned long g_value_64d8ac = 6;
// GLOBAL: WIZ8 0x0069c1cc
int g_value_69c1cc;
extern unsigned long g_value_64d8ac;
}

/* Address quarantine 005a8ed1-005b478f; bounds come from adjacent
   assertion-backed original translation-unit intervals. */

extern void Function4257F0(int value);

// FUNCTION: WIZ8 0x005A9E90
int* GetAddress69C1CC(void)
{
    return &g_value_69c1cc;
}
// FUNCTION: WIZ8 0x005AE9C0
void SetValue64D8AC(unsigned long value)
{
    g_value_64d8ac = value;
}

/* Release the three level-runtime dialogue owners through the shared
   teardown, then clear the slots. */
// FUNCTION: WIZ8 0x005B1C00
void Function5B1C00(void)
{
    if (g_level_block->unknown_2a0 != 0) {
        Function4257F0(g_level_block->unknown_2a0);
        g_level_block->unknown_2a0 = 0;
    }
    if (g_level_block->unknown_2a4 != 0) {
        Function4257F0(g_level_block->unknown_2a4);
        g_level_block->unknown_2a4 = 0;
    }
    if (g_level_block->unknown_2a8 != 0) {
        Function4257F0(g_level_block->unknown_2a8);
        g_level_block->unknown_2a8 = 0;
    }
}
