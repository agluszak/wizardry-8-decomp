#include "wiz8/level_specific_code/Monastery2.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/sr_api.h"
#include "soundman.h"

/* Level Specific Code\Monastery2.cpp (level 9).

   Attribution evidence: only 0x004DC7A0 cites this file's path string, in its
   two pPropTrigger assertions. 0x004DC880 is attributed because
   MasterFunctionList registers it under level 9 (Monastery2) and it lies
   between the Monastery2 anchor and the level-8 (Monastery1) callback block.
   The wider gap 0x004DC390-0x004DCB10 holds contiguous per-level callback
   blocks for four level TUs - MtGigasOuter (0xe), MtGigasTop (0xf),
   Monastery2 (9) and Monastery1 (8) - which are not recovered here. */

#define MONASTERY2_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Monastery2.cpp"

// FUNCTION: WIZ8 0x004dc7a0
unsigned char Monastery2BellButton004DC7A0(Trigger* pTrigger)
{
    W8Prop* prop;
    Trigger* pPropTrigger;
    int slot;

    prop = FindPropByName(g_world, "dial1");
    if (prop != 0) {
        slot = prop->Rep()->FindCurrentAnimationSlot();
        if (slot == 0) {
            SoundPlay("Data\\Sound\\Ambients\\Mon2Bell1.wav", 0);
        } else if (slot == 1) {
            SoundPlay("Data\\Sound\\Ambients\\Mon2Bell2.wav", 0);
            return 1;
        } else if (slot == 2) {
            SoundPlay("Data\\Sound\\Ambients\\Mon2Bell3.wav", 0);
            pPropTrigger = FindTriggerByName("bellringswitch");
            if (pPropTrigger == 0) {
                srAssertFail("pPropTrigger", MONASTERY2_CPP, 0x21, 0);
            }
            pPropTrigger->flag_0a0_04 = 0;
            pPropTrigger = FindTriggerByName("bell_button");
            if (pPropTrigger == 0) {
                srAssertFail("pPropTrigger", MONASTERY2_CPP, 0x24, 0);
            }
            pPropTrigger->flag_0a0_04 = 0;
            return 1;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x004dc880
unsigned char Monastery2MicroDoor2004DC880(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8WorldItem* item;

    position.x = 13085.0f;
    position.y = -80.0f;
    position.z = -49440.0f;
    item = SpawnItem(0x2d7, &position, 3, 1);
    if (item != 0) {
        Function4F6CF0(item);
    }
    return 1;
}
