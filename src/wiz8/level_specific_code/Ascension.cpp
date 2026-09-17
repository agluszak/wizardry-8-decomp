#include "wiz8/level_specific_code/Ascension.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/PC_Item.h"

/* Level Specific Code\Ascension.cpp.

   Attribution evidence: 0x004DFB40 and 0x004DFB80 are bounded to this TU, and
   InitializeLevelMasterFunctions004D6C50 runs 0x004E0510 under case 0x24, the
   Footsteps level, which is part of the Ascension Peak block. */

/* Count how many of the three Ascension Peak relic items (0x242, 0x243,
   0x244) are on the party; the world-cursor "seen bodies" handler requires
   all three. */
// FUNCTION: WIZ8 0x004DF810
int CountAscensionPeakItems004DF810(void)
{
    int count = 0;

    if (FindItemOnParty(0x242, 0, 0, 2, 0)) {
        count = 1;
    }
    if (FindItemOnParty(0x243, 0, 0, 2, 0)) {
        ++count;
    }
    if (FindItemOnParty(0x244, 0, 0, 2, 0)) {
        ++count;
    }
    return count;
}

/* Spawn the Alfie "life" monster (0xaf) at the NP_AlfieLife entity when it is
   present in the level. */
// FUNCTION: WIZ8 0x004DFB40
unsigned char SpawnAlfieLife004DFB40(int unused)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_AlfieLife", &position, 0, 0) != 0) {
        SpawnMonsters(0xaf, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* Spawn the Alfie "know" monster (0xb0) at the NP_AlfieKnow entity when it is
   present in the level. */
// FUNCTION: WIZ8 0x004DFB80
unsigned char SpawnAlfieKnow004DFB80(int unused)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_AlfieKnow", &position, 0, 0) != 0) {
        SpawnMonsters(0xb0, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* The Footsteps puzzle's ButtonBlocker prop tracks fact 0x156. */
// FUNCTION: WIZ8 0x004E0510
void SyncButtonBlockerFromFact004E0510(void)
{
    W8Prop* prop = FindPropByName(g_world, "ButtonBlocker");
    if (prop != 0) {
        if (GetFact(0x156) != 0) {
            prop->SetSetting6C(1);
        } else {
            prop->SetSetting6C(0);
        }
    }
}
