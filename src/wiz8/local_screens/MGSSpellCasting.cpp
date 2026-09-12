#include "wiz8/local_screens/MGSSpellCasting.h"

#include "wiz8/regions.h"

// FUNCTION: WIZ8 0x005a1140
unsigned char IgnoreSpellCastingInput(const InputAtom* input)
{
    return 0;
}

// FUNCTION: WIZ8 0x005A19A0
void DisableRegionSet1C(void)
{
    RegionSetDisable(0x1c);
}
