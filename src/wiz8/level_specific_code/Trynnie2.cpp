#include "wiz8/level_specific_code/Trynnie2.h"
#include "wiz8/level_specific_code/Trynnie1.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/item_spawning.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/layouts/item_instance.h"
#include "wiz8/cursor.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/MonsterGenerator.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/string_database.h"

/* Level Specific Code\Trynnie2.cpp (level 0x1a).

   Attribution evidence: 0x004D9E60 passes this file's path string to
   MonsterGetIndexByLocationID. The level-0x1a block in
   InitializeLevelMasterFunctions registers the surrounding cluster
   (GoodaVine_A/B, Give_Zulu, Meat_Maker, Meat_Box, URN_Trigger_01-04) and runs
   the Trynnie2Killed location-variable sync. */

#define TRYNNIE2_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Trynnie2.cpp"

/* Fact 0x229 records that the Trynnie2 group was wiped out; mirror it into the
   Trynnie2Killed location variable the first time this level sees it, so the
   world-side check has a stable flag to read. */
// FUNCTION: WIZ8 0x004D9D30
void EnsureTrynnie2KilledVar(void)
{
    if (GetFact(0x229) == 1) {
        if (GetLocationVarIDByName("Trynnie2Killed") == -1) {
            KillTrynnieGroups();
            CreateLocationVar("Trynnie2Killed", 1);
        }
    }
}

/* "GoodaVine_A": place the vine item on the cursor, refusing while the cursor
   already holds an item. */
// FUNCTION: WIZ8 0x004D9D70
bool Trynnie2GoodaVineA(Trigger* pTrigger)
{
    if (g_status.item_in_cursor != 0) {
        return 0;
    }
    ReplaceOrCreateItem(&g_status.item_in_hand_235b, 0x16d, 1, 1, 0);
    SetItemCursor(0);
    return 1;
}

/* "GoodaVine_B": place the second vine item on the cursor, refusing while the
   cursor already holds an item. */
// FUNCTION: WIZ8 0x004D9DA0
bool Trynnie2GoodaVineB(Trigger* pTrigger)
{
    if (g_status.item_in_cursor != 0) {
        return 0;
    }
    ReplaceOrCreateItem(&g_status.item_in_hand_235b, 0x16e, 1, 1, 0);
    SetItemCursor(0);
    return 1;
}

/* "Give_Zulu": place item 0x1b3 on the cursor, refusing while the cursor
   already holds an item. */
// FUNCTION: WIZ8 0x004D9DD0
bool Trynnie2GiveZulu(Trigger* pTrigger)
{
    if (g_status.item_in_cursor != 0) {
        return 0;
    }
    ReplaceOrCreateItem(&g_status.item_in_hand_235b, 0x1b3, 1, 1, 0);
    SetItemCursor(0);
    return 1;
}

/* "Meat_Maker": spawn the meat item at the Meat_Position entity. */
// FUNCTION: WIZ8 0x004D9E00
bool Trynnie2MeatMaker(Trigger* pTrigger)
{
    srVector3T<float> entity_position;
    srVector3T<float> position;
    W8WorldItem* item;

    if (FindEntityByName("Meat_Position", &entity_position, 0, 0) != 0) {
        position = entity_position;
        item = SpawnItem(0x1b4, &position, 3, 1);
        if (item != 0) {
            ActivateItem(item);
        }
    }
    return 1;
}

/* "Meat_Box": feeding Hogar (monster 0x1d4) plain meat starts MoveHogar.msf;
   drugged meat starts MoveHogarDrugged.msf and pays the 500-point reward
   once. */
// FUNCTION: WIZ8 0x004D9E60
bool Trynnie2MeatBox(Trigger* pTrigger)
{
    W8MonsterGroup* group;
    W8MonsterInfo* info;
    unsigned int index;
    int item_id;

    group = FindFirstMonsterByID(0x1d4);
    if (group == 0 || g_status.item_in_cursor == 0) {
        ShowNotice(0xf, gppStringList[0x2590 / 4]);
        return 1;
    }
    item_id = GetItemInHand();
    if (item_id != 0x1b4 && item_id != 0x1c2) {
        ShowNotice(0xf, gppStringList[0x2590 / 4]);
        return 1;
    }
    index = MonsterGetIndexByLocationID(0x68, TRYNNIE2_CPP, group->leader_location_id, 1);
    info = MonsterGetScriptPartByLocationIndex(index);
    if (info->highest_condition >= 0xf) {
        return 1;
    }
    if (item_id == 0x1b4) {
        info->p3D->SetScript("MoveHogar.msf", 1);
    } else {
        info->p3D->SetScript("MoveHogarDrugged.msf", 1);
        if (GetLocationVarIDByName("HogarDruggedGivenExp") == -1) {
            AwardPartyExperience(500, 0);
            CreateLocationVar("HogarDruggedGivenExp", 1);
        }
    }
    ClearHeldItemDisplay();
    BeginScriptedWorldAction();
    return 1;
}

/* Use-item action for the Zulu (0x1b3) and item 0x1c3: hand the use to the
   type-7 world cursor node covering the camera position. An unhandled use
   reports the refusal string; the first handled use only latches the status
   flag, later uses show a second string and spawn the Mystical Shaman (0xec)
   at NP_MysticalShaman. Either way the used item is consumed into a scratch
   instance. The retail DispatchWorldCursorNodeCommand call passes only two
   arguments; arg is never dereferenced for command 8, so the callee reads an
   uninitialized stack slot there. */
// FUNCTION: WIZ8 0x004D9F60
bool Trynnie2UseItem(W8ItemInstance* item)
{
    srVector3T<float> position;
    W8ItemInstance destination;

    if (item->iItemNo != 0x1b3 && item->iItemNo != 0x1c3) {
        return 0;
    }
    if (DispatchWorldCursorNodeCommand(0, 8, 0) == 0) {
        ShowString(gppStringList[0x2598 / 4]);
        return 1;
    }
    if (item->iItemNo == 0x1b3) {
        ShowString(gppStringList[0x25a0 / 4]);
    } else {
        ShowString(gppStringList[0x259c / 4]);
    }
    if (g_status.use_item_latch_2445 == 0) {
        g_status.use_item_latch_2445 = true;
    } else {
        ShowString(gppStringList[0x25a4 / 4]);
        if (FindEntityByName("NP_MysticalShaman", &position, 0, 0) != 0) {
            SpawnMonsters(0xec, 1, &position, 0, 1, 0, 0);
        }
    }
    destination.iItemNo = -1;
    CopyItemInstance(&destination, item, 0, 1);
    return 1;
}

/* "URN_Trigger_01..04": once more than two of the four urn variables are set
   the pagoda generator is shut down; either way a level message reports the
   outcome. */
// FUNCTION: WIZ8 0x004DA060
bool Trynnie2UrnTrigger(Trigger* pTrigger)
{
    MonGen* generator;
    unsigned int count;

    count = 0;
    if (GetLocationVarValueByName("Urn1") > 0) {
        ++count;
    }
    if (GetLocationVarValueByName("Urn2") > 0) {
        ++count;
    }
    if (GetLocationVarValueByName("Urn3") > 0) {
        ++count;
    }
    if (GetLocationVarValueByName("Urn4") > 0) {
        ++count;
    }
    if (count >= 3) {
        generator = FindMonGenByName("PAGODA");
        if (generator != 0) {
            generator->generation_enabled = 0;
        }
        ShowLevelMessage(0x14);
        return 1;
    }
    ShowLevelMessage(0x13);
    return 1;
}
