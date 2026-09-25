#include "wiz8/level_specific_code/Swamp.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/vector.h"
#include "surrender/srCamera.h"

#define SWAMP_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Swamp.cpp"

// GLOBAL: WIZ8 0x006834e4
W8Monster* g_swamp_spawned_monster_6834e4;

/* Level Specific Code\Swamp.cpp (level 0x18).

   Attribution evidence: 0x004DAA70 passes this file's path string to
   MonsterGetIndexByLocationID. The level-0x18 block in
   InitializeLevelMasterFunctions registers the surrounding cluster
   (oil_pool, gas_trig_plane01-07, onelid, fire_trig_plane01-07). */

/* "oil_pool": item 0x2d0 held over the pool is converted to item 0x15e and the
   shared item-consumed flag is raised. Always returns 0, so the trigger stays
   armed for another try. */
// FUNCTION: WIZ8 0x004DA960
bool SwampOilPool(Trigger* pTrigger)
{
    if (g_status_685170.item_in_cursor != 0 && GetItemInHand() == 0x2d0) {
        ClearHeldItemDisplay();
        ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, 0x15e, 0, 1, 0);
        SetItemCursor(0);
        g_trigger_feedback_00606994 = 1;
    }
    return 0;
}

bool SwampGasFireSpawn(Trigger* pTrigger); /* 0x004DAA70 */
void SwampGasFireItemDrop(int command);    /* 0x004DACD0 */

/* "gas_trig_plane01-07": while fact 0x16d is unset the trigger defers to the
   one-shot spawner; once set, it casts spell 0x2a at power 4 on the camera
   position. */
// FUNCTION: WIZ8 0x004DA9B0
bool SwampGasPlane(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (GetFact(0x16d) == 0) {
        SwampGasFireSpawn(pTrigger);
        return 0;
    }
    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, 0x2a, 4);
    return 1;
}

/* "fire_trig_plane01-07": same fact gate as the gas planes, but casts spell
   0x24. */
// FUNCTION: WIZ8 0x004DAA10
bool SwampFirePlane(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (GetFact(0x16d) == 0) {
        SwampGasFireSpawn(pTrigger);
        return 0;
    }
    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, 0x24, 4);
    return 1;
}

/* The one-shot behind the gas and fire trigger planes, armed while fact 0x16d
   is unset: spawn monster 0x1b5 at the triggering plane's center, fade it in
   under the camera, arm the item-drop master function, and hand the
   location-0xa3 NPC binding a script notice. */
/* 0x004DAA8A is a split entry Ghidra carved out of this body: it resumes at
   the SetFact call after the fact-0x16d early return, not a separate
   authored function. */
// SYNTHETIC: WIZ8 0x004DAA8A
// SwampGasFireSpawn post-guard continuation
// FUNCTION: WIZ8 0x004DAA70
bool SwampGasFireSpawn(Trigger* pTrigger)
{
    srVector3T<float> center;
    srVector3T<float> position;
    srVector3T<float> mapped;
    srVector3T<float> look_target;
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    W8NpcState* npc;
    int index;

    if (GetFact(0x16d) != 0) {
        return 0;
    }
    SetFact(0x16d, 1, 0);
    ResetInactiveLevelDataVectors();
    center.Set(
        (pTrigger->representation_vectors_0cc[0].x + pTrigger->representation_vectors_0cc[1].x +
         pTrigger->representation_vectors_0cc[2].x + pTrigger->representation_vectors_0cc[3].x) *
            g_double_005ec980,
        (pTrigger->representation_vectors_0cc[0].y + pTrigger->representation_vectors_0cc[1].y +
         pTrigger->representation_vectors_0cc[2].y + pTrigger->representation_vectors_0cc[3].y) *
            g_double_005ec980,
        (pTrigger->representation_vectors_0cc[0].z + pTrigger->representation_vectors_0cc[1].z +
         pTrigger->representation_vectors_0cc[2].z + pTrigger->representation_vectors_0cc[3].z) *
            g_double_005ec980);
    position = center;
    group = SpawnMonsters(0x1b5, 1, &position, 0, 1, 0, 0);
    index = IListGetAt(group->monsters, 0);
    if (index != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x8f, SWAMP_CPP, index, 1));
        if (monster_info != 0) {
            g_swamp_spawned_monster_6834e4 = monster_info->p3D;
            monster_info->p3D->m_pRep->instance_scale_05c = 1.0f;
            monster_info->p3D->m_pRep->apply_instance_scale_061 = 1;
            g_swamp_spawned_monster_6834e4->BeginFadeIn(2.0f);
            g_swamp_spawned_monster_6834e4->GetMappedPosition(&mapped);
            look_target = monster_info->p3D->movement_0c0.position_040;
            look_target.y += monster_info->p3D->movement_0c0.height_offset_0b8;
            g_gd_camera_65a0f8->LookAt(&look_target, 0);
            g_flag_6109f0 = false;
            g_master_functions_006834d8->Add(SwampGasFireItemDrop);
        }
    }
    npc = FindNpcBindingForMonster(MonsterGetIndexByLocationID(0xa3, SWAMP_CPP, index, 1));
    QueueNpcScriptNotice(npc, 0, -1, 0, 0);
    return 1;
}

/* The master function the spawn arms. 0xEFFFFFFF re-registers it and clears
   the notice flag; each zero tick waits for g_flag_6109f0 - raised when the
   queued NPC script notice completes - then drops an unidentified item 0x264
   at the spawned monster and fades it out for removal. */
// FUNCTION: WIZ8 0x004DACD0
void SwampGasFireItemDrop(int command)
{
    srVector3T<float> position;
    srVector3T<float> drop_position;
    W8WorldItem* item;

    if (command != 0) {
        if (command == static_cast<int>(0xEFFFFFFF)) {
            g_flag_6109f0 = false;
            g_master_functions_006834d8->Add(SwampGasFireItemDrop);
        }
        return;
    }
    g_flag_006834dc = false;
    if (g_flag_6109f0 != 0) {
        g_flag_006834dc = true;
        if (g_swamp_spawned_monster_6834e4 != 0) {
            position = g_swamp_spawned_monster_6834e4->GetPosition();
            drop_position = position;
            item = SpawnItem(0x264, &drop_position, 3, 1);
            item->item.identified = 0;
            ActivateItem(item);
            g_swamp_spawned_monster_6834e4->BeginFadeOutAndRemove(0);
        }
    }
}

/* "onelid": once fact 0x33 is set and an npc of kind 0x1e exists, advance the
   script facts - set 0x39 and clear 0x33. */
// FUNCTION: WIZ8 0x004DADF0
bool SwampOnelid(Trigger* pTrigger)
{
    if (GetFact(0x33) != 0 && FindNpcOfKind(0x1e) != 0) {
        SetFact(0x39, 1, 0);
        SetFact(0x33, 0, 0);
    }
    return 1;
}
