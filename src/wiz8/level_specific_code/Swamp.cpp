#include "wiz8/spell_ids.h"
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
static W8Monster* g_swamp_spawned_monster;

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
    if (g_status.item_in_cursor && GetItemInHand() == 0x2d0) {
        ClearHeldItemDisplay();
        ReplaceOrCreateItem(&g_status.item_in_hand, 0x15e, false, true, false);
        SetItemCursor(0);
        g_trigger_feedback = true;
    }
    return false;
}

static bool SwampGasFireSpawn(Trigger* pTrigger); /* 0x004DAA70 */
static void SwampGasFireItemDrop(int command);    /* 0x004DACD0 */

/* "gas_trig_plane01-07": while fact 0x16d is unset the trigger defers to the
   one-shot spawner; once set, it casts spell 0x2a at power 4 on the camera
   position. */
// FUNCTION: WIZ8 0x004DA9B0
bool SwampGasPlane(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (GetFact(W8_FACT_SAVANT_PARTY_SWAMP_MEET) == 0) {
        SwampGasFireSpawn(pTrigger);
        return false;
    }
    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, W8_SPELL_NOXIOUS_FUMES, 4);
    return true;
}

/* "fire_trig_plane01-07": same fact gate as the gas planes, but casts spell
   0x24. */
// FUNCTION: WIZ8 0x004DAA10
bool SwampFirePlane(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (GetFact(W8_FACT_SAVANT_PARTY_SWAMP_MEET) == 0) {
        SwampGasFireSpawn(pTrigger);
        return false;
    }
    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, W8_SPELL_FIREBALL, 4);
    return true;
}

/* The one-shot behind the gas and fire trigger planes, armed while fact 0x16d
   is unset: spawn monster 0x1b5 at the triggering plane's center, fade it in
   under the camera, arm the item-drop master function, and hand the
   location-0xa3 NPC binding a script notice. */
/* 0x004DAA8A is a split entry Ghidra carved out of this body: it resumes at
   the SetFact call after the fact-0x16d early return, not a separate
   authored function. */
// FUNCTION: WIZ8 0x004DAA70
static bool SwampGasFireSpawn(Trigger* pTrigger)
{
    srVector3T<float> center;
    srVector3T<float> position;
    srVector3T<float> mapped;
    srVector3T<float> look_target;
    W8MonsterGroup* group;
    W8MonsterInfo* monster_info;
    W8NpcState* npc;
    int index;

    if (GetFact(W8_FACT_SAVANT_PARTY_SWAMP_MEET) != 0) {
        return false;
    }
    SetFact(W8_FACT_SAVANT_PARTY_SWAMP_MEET, 1, false);
    ResetInactiveLevelDataVectors();
    center.Set((pTrigger->representation_vectors[0].x + pTrigger->representation_vectors[1].x +
                pTrigger->representation_vectors[2].x + pTrigger->representation_vectors[3].x) *
                   g_double_quarter,
               (pTrigger->representation_vectors[0].y + pTrigger->representation_vectors[1].y +
                pTrigger->representation_vectors[2].y + pTrigger->representation_vectors[3].y) *
                   g_double_quarter,
               (pTrigger->representation_vectors[0].z + pTrigger->representation_vectors[1].z +
                pTrigger->representation_vectors[2].z + pTrigger->representation_vectors[3].z) *
                   g_double_quarter);
    position = center;
    group = SpawnMonsters(0x1b5, 1, &position, 0, true, false, false);
    index = IListGetAt(group->monsters, 0);
    if (index != 0) {
        monster_info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x8f, SWAMP_CPP, index, true));
        if (monster_info != 0) {
            g_swamp_spawned_monster = monster_info->p3D;
            monster_info->p3D->m_pRep->instance_scale = 1.0f;
            monster_info->p3D->m_pRep->apply_instance_scale = true;
            g_swamp_spawned_monster->BeginFadeIn(2.0f);
            g_swamp_spawned_monster->GetMappedPosition(&mapped);
            look_target = monster_info->p3D->movement.position;
            look_target.y += monster_info->p3D->movement.height_offset;
            g_gd_camera->LookAt(&look_target, false);
            g_npc_dialogue_closed = false;
            g_master_functions->Add(SwampGasFireItemDrop);
        }
    }
    npc = FindNpcBindingForMonster(MonsterGetIndexByLocationID(0xa3, SWAMP_CPP, index, true));
    QueueNpcScriptNotice(npc, 0, -1, false, 0);
    return true;
}

/* The master function the spawn arms. 0xEFFFFFFF re-registers it and clears
   the notice flag; each zero tick waits for g_npc_dialogue_closed - raised when the
   queued NPC script notice completes - then drops an unidentified item 0x264
   at the spawned monster and fades it out for removal. */
// FUNCTION: WIZ8 0x004DACD0
static void SwampGasFireItemDrop(int command)
{
    srVector3T<float> position;
    srVector3T<float> drop_position;
    W8WorldItem* item;

    if (command != 0) {
        if (command == static_cast<int>(0xEFFFFFFF)) {
            g_npc_dialogue_closed = false;
            g_master_functions->Add(SwampGasFireItemDrop);
        }
        return;
    }
    g_remove_current_master_function = false;
    if (g_npc_dialogue_closed) {
        g_remove_current_master_function = true;
        if (g_swamp_spawned_monster != 0) {
            position = g_swamp_spawned_monster->GetPosition();
            drop_position = position;
            item = SpawnItem(0x264, &drop_position, W8_ITEM_ENTITY_PULSE | W8_ITEM_ENTITY_ROTATE,
                             true);
            item->item.identified = false;
            ActivateItem(item);
            g_swamp_spawned_monster->BeginFadeOutAndRemove(W8_MONSTER_REMOVAL_NONE);
        }
    }
}

/* "onelid": once fact 0x33 is set and an npc of kind 0x1e exists, advance the
   script facts - set 0x39 and clear 0x33. */
// FUNCTION: WIZ8 0x004DADF0
bool SwampOnelid(Trigger* pTrigger)
{
    if (GetFact(W8_FACT_CROCK_KIDNAPPED_PLAYER) != 0 && FindNpcOfKind(0x1e) != 0) {
        SetFact(W8_FACT_CROCK_PLAYER_RETURNED_TO_PARTY, 1, false);
        SetFact(W8_FACT_CROCK_KIDNAPPED_PLAYER, 0, false);
    }
    return true;
}
