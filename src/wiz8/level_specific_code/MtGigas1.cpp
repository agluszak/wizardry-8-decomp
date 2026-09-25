#include "wiz8/level_specific_code/MtGigas1.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/GDProp.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/xstatus.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "soundman.h"

/* Level Specific Code\MtGigas1.cpp (level 0x0c).

   Attribution evidence: InitializeLevelMasterFunctions runs
   0x004DBAB0 and registers 0x004DBB50-0x004DBE30 under case 0x0c, and the
   range sits between the MtGigas2 TU and the MtGigasOuter TU which starts
   at 0x004DBE70. */

// GLOBAL: WIZ8 0x006834f4
GDProp* g_plate_prop_6834f4;
// GLOBAL: WIZ8 0x006834f8
unsigned char g_plate_contact_6834f8;
// GLOBAL: WIZ8 0x006834f9
unsigned char g_plate_down_6834f9;

/* Level-load restore: binds the "plate" trigger's prop to the plate GDProp
   global and mirrors the saved PPlateDown variable into the latch flag,
   creating the variable when the level never wrote one. */
// FUNCTION: WIZ8 0x004DBAB0
void MtGigas1Setup(void)
{
    Trigger* pTrigger;
    W8Prop* prop;

    g_plate_prop_6834f4 = 0;
    g_plate_down_6834f9 = 0;
    pTrigger = FindTriggerByName("plate");
    if (pTrigger != 0) {
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = pTrigger->m_pProp;
        if (prop != 0) {
            g_plate_prop_6834f4 = prop->m_gd_prop;
        }
    }
    if (GetLocationVarIDByName("PPlateDown") == -1) {
        CreateLocationVar("PPlateDown", 0);
        return;
    }
    if (GetLocationVarValueByName("PPlateDown") != 0) {
        g_plate_down_6834f9 = 1;
    }
}

/* Activation callback on _VOC_EWAXXLIFT1: queues the lift NPC notice with
   the held item, if any. */
// FUNCTION: WIZ8 0x004DBB50
bool MtGigas1Lift1(Trigger* pTrigger)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    W8NpcState* npc = GetNpcStateByKind(0x5d);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_trigger_feedback_00606994 = 1;
    return false;
}

/* Activation callback on _VOC_EWAXXLIFT2: queues the lift NPC notice with
   the held item, if any. */
// FUNCTION: WIZ8 0x004DBB90
bool MtGigas1Lift2(Trigger* pTrigger)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    W8NpcState* npc = GetNpcStateByKind(0x5e);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_trigger_feedback_00606994 = 1;
    return false;
}

/* Activation callback on PRESSUREPLATE: ticks the secret door. The plate
   counts as held while the camera stands inside the prop's XZ bounds or the
   prop still has list entries. On the down edge it opens secretDoor-01 and
   drops the plate (setting 1, rumble + open sounds); on the release edge it
   reverses both (setting 3, close + rumble sounds) and clears PPlateDown. */
// FUNCTION: WIZ8 0x004DBBD0
bool MtGigas1PressurePlate(Trigger* pTrigger)
{
    srVector3T<float> minimum;
    srVector3T<float> maximum;
    Trigger* pPlateTrigger;
    srVector3T<float> position;

    if (g_plate_prop_6834f4 == 0) {
        return false;
    }
    srVector3T<double> camera_position = GetWorld()->camera->getLocation();
    position.x = static_cast<float>(camera_position.x);
    position.z = static_cast<float>(camera_position.z);
    g_plate_prop_6834f4->ComputeBounds(&minimum, &maximum);
    g_plate_contact_6834f8 = 0;
    if ((position.x >= minimum.x && position.x <= maximum.x && position.z >= minimum.z &&
         position.z <= maximum.z) ||
        g_plate_prop_6834f4->HasListEntries() != 0) {
        g_plate_contact_6834f8 = 1;
        if (g_plate_down_6834f9 != 0) {
            return false;
        }
        pPlateTrigger = FindTriggerByName("secretDoor-01");
        if (pPlateTrigger != 0) {
            pPlateTrigger->Run(-1);
            if (pPlateTrigger->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            if (pPlateTrigger->m_pProp != 0) {
                pPlateTrigger->m_pProp->SetSetting6E(1);
            }
        }
        pPlateTrigger = FindTriggerByName("plate");
        if (pPlateTrigger != 0) {
            pPlateTrigger->Run(-1);
            if (pPlateTrigger->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            if (pPlateTrigger->m_pProp != 0) {
                pPlateTrigger->m_pProp->SetSetting6E(1);
            }
        }
        g_plate_down_6834f9 = 1;
        SetTriggerVariableByName("PPlateDown", 1);
        SoundPlay("Data\\Sound\\Ambients\\Door Stone Open.wav", 0);
        SoundPlay("Data\\Sound\\Ambients\\Amb Rumble Very Low.wav", 0);
        return false;
    }
    if (g_plate_down_6834f9 == 0) {
        return false;
    }
    pPlateTrigger = FindTriggerByName("secretDoor-01");
    if (pPlateTrigger != 0) {
        pPlateTrigger->Run(-1);
        if (pPlateTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        if (pPlateTrigger->m_pProp != 0) {
            pPlateTrigger->m_pProp->SetSetting6E(3);
        }
    }
    pPlateTrigger = FindTriggerByName("plate");
    if (pPlateTrigger != 0) {
        pPlateTrigger->Run(-1);
        if (pPlateTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        if (pPlateTrigger->m_pProp != 0) {
            pPlateTrigger->m_pProp->SetSetting6E(3);
        }
    }
    g_plate_down_6834f9 = 0;
    SetTriggerVariableByName("PPlateDown", 0);
    SoundPlay("Data\\Sound\\Ambients\\Door Stone Close 01.wav", 0);
    SoundPlay("Data\\Sound\\Ambients\\Amb Rumble Very Low.wav", 0);
    return false;
}

/* Activation callback on mudWallTrigger: spawns the badass monster at the
   NP_BadassMonster entity. */
// FUNCTION: WIZ8 0x004DBE30
bool MtGigas1MudWall(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_BadassMonster", &position, 0, 0)) {
        SpawnMonsters(0x157, 1, &position, 1, 1, 0, 0);
    }
    return true;
}
