#include "wiz8/level_specific_code/CosmicCircle.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/sr_api.h"

#define COSMIC_CIRCLE_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Cosmic Circle.cpp"

/* Level Specific Code\Cosmic Circle.cpp (level 4).

   Attribution evidence: 0x004D9AD0 and 0x004D9B40 carry this file's path in
   their assertion calls. The level-4 block in
   InitializeLevelMasterFunctions registers CC_TRIGGERPLANE1HEDRA and
   calls the setup. The leading always-false callback is installed on Arnika's
   "ARN11" trigger; it sits at the head of this unit between
   MasterFunctionList.cpp's template emissions and the CC callback. */

// FUNCTION: WIZ8 0x004D9AC0
bool CosmicCircleReturnFalse(Trigger* pTrigger)
{
    return false;
}

/* "CC_TRIGGERPLANE1HEDRA": stepping onto the arena trigger plane ends any
   running combat, posts the savant's script notice and starts CameraPath1. */
// FUNCTION: WIZ8 0x004D9AD0
bool CosmicCircleTriggerPlane1Hedra(Trigger* pTrigger)
{
    W8NpcState* pNPC;

    if (gXStatus.fCombatMode != 0) {
        EndCombat(1);
    }
    BeginScriptedWorldAction();
    pNPC = GetNpcStateByKind(0x85);
    if (pNPC == 0) {
        srAssertFail("pNPC", COSMIC_CIRCLE_CPP, 0x19, "Cannot find VOC_SAVANT_CC1");
    } else {
        QueueNpcScriptNotice(pNPC, 0, -1, 0, 0);
    }
    UpdateCameraPathStateByName(GetWorld(), "CameraPath1", 1);
    return true;
}

/* Spawn the Cosmic Circle arena: Altheides and the dark savant on their named
   positions, Bela and Phoonzang as hostile adds, the two principals aimed at
   each other's start points and the remaining trigger planes switched off. */
// FUNCTION: WIZ8 0x004D9B40
void CosmicCircleSetup(void)
{
    /* Retail dereferenced both monster infos unconditionally, leaving the
       pointer uninitialised on a missing named entity or failed id; null
       models that defect path deterministically. */
    W8MonsterInfo* pMonsterInfoDs = 0;
    W8MonsterInfo* pMonsterInfoAltheides = 0;
    srVector3T<float> positionAltheides;
    srVector3T<float> positionDs;
    srVector3T<float> positionBela;
    srVector3T<float> positionPhoonzang;
    W8MonsterGroup* group;
    Trigger* pTrigger;
    int uiMonsterID;
    unsigned int index;

    if (g_status.world_suspended_2390 == 0 && g_status.cc_arena_spawned_4972 == 0) {
        if (FindEntityByName("NP_ALTHEIDESARENA", &positionAltheides, 0, 0)) {
            group = SpawnMonsters(0x1b3, 1, &positionAltheides, 0, 1, 0, 0);
            uiMonsterID = IListGetAt(group->monsters, 0);
            if (uiMonsterID == 0) {
                srAssertFail("uiMonsterID", COSMIC_CIRCLE_CPP, 0x41,
                             "Invalid Monster ID for CC end");
            } else {
                index = MonsterGetIndexByLocationID(0x45, COSMIC_CIRCLE_CPP, uiMonsterID, 1);
                pMonsterInfoAltheides = MonsterGetScriptPartByLocationIndex(index);
            }
        }
        if (FindEntityByName("NP_DSARENA", &positionDs, 0, 0)) {
            group = SpawnMonsters(0x1b6, 1, &positionDs, 0, 1, 0, 0);
            uiMonsterID = IListGetAt(group->monsters, 0);
            if (uiMonsterID == 0) {
                srAssertFail("uiMonsterID", COSMIC_CIRCLE_CPP, 0x53,
                             "Invalid Monster ID for CC end");
            } else {
                index = MonsterGetIndexByLocationID(0x57, COSMIC_CIRCLE_CPP, uiMonsterID, 1);
                pMonsterInfoDs = MonsterGetScriptPartByLocationIndex(index);
            }
        }
        if (FindEntityByName("NP_BELASTART", &positionBela, 0, 0)) {
            SpawnMonsters(0x1b4, 1, &positionBela, 2, 1, 0, 0);
        }
        if (FindEntityByName("NP_PHOONZANG1", &positionPhoonzang, 0, 0)) {
            SpawnMonsters(0x197, 1, &positionPhoonzang, 2, 1, 0, 0);
        }
        g_status.cc_arena_spawned_4972 = true;
        pMonsterInfoDs->p3D->AimAtPosition(&positionAltheides);
        pMonsterInfoAltheides->p3D->AimAtPosition(&positionDs);
        pTrigger = FindTriggerByName("CC_TRIGGERPLANE2");
        if (pTrigger != 0) {
            pTrigger->flags_0a0 &= ~W8_TRIGGER_ON;
        }
        pTrigger = FindTriggerByName("CC_TRIGGERPLANE4");
        if (pTrigger != 0) {
            pTrigger->flags_0a0 &= ~W8_TRIGGER_ON;
        }
    }
}
