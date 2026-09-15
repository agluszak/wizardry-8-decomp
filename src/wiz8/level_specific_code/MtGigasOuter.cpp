#include "wiz8/level_specific_code/MtGigasOuter.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/cursor.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/float_constants.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "Debug.h"

#define MTGIGASOUTER_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MtGigasOuter.cpp"

/* Level Specific Code\MtGigasOuter.cpp (level 0x0e).

   Attribution evidence: 0x004DC080 is this TU's hard lower bound and
   InitializeLevelMasterFunctions004D6C50 runs 0x004DBE70 under case 0x0e,
   alongside the _VOC_EWAXXLIFT1 trigger wiring. The assert file string at
   0x00613640 names this TU for 0x004DBEC0 and 0x004DC080. */

// GLOBAL: WIZ8 0x006834fc
W8IntervalGate* g_lift_gate_6834fc;
// GLOBAL: WIZ8 0x00683500
W8Prop* g_lift_prop_683500;
// GLOBAL: WIZ8 0x00683504
stSound3D* g_alarm_sound_683504;
// GLOBAL: WIZ8 0x00683508
W8IntervalGate* g_alarm_gate_683508;

/* Lift prop Y travel: 5228.6 at the top of the ride, -2000.0 at the bottom,
   and -5000.0 parked out of the world once the ride is spent. */
// GLOBAL: WIZ8 0x00613600
const float LIFT_TOP_Y = 5228.6f;
// GLOBAL: WIZ8 0x00613604
const float LIFT_BOTTOM_Y = -2000.0f;
// GLOBAL: WIZ8 0x00613608
const float LIFT_PARKED_Y = -5000.0f;

/* When the FlagPosition location variable exists, feed its value to the lift
   gate; values at or above 1000 stop there. Otherwise the "flag" trigger
   runs. */
// FUNCTION: WIZ8 0x004DBE70
void ProcessFlagPosition004DBE70(void)
{
    if (GetLocationVarIDByName("FlagPosition") != -1) {
        int value = GetLocationVarValueByName("FlagPosition");
        ControlLiftGate(value);
        if (value >= 1000) {
            return;
        }
    }
    Trigger* trigger = FindTriggerByName("flag");
    trigger->Run(-1);
}

/* Activation callback installed on the "crank" trigger. A fresh game creates
   FlagPosition and queues the lift operator's greeting script; a zero value
   resets the lift and flags the trigger used; a saved mid-descent value
   (100-999) snaps the lift prop to its parked depth, marks the ride done and
   spawns the sentry at NP_catwalk. */
// FUNCTION: WIZ8 0x004DBEC0
bool OnCrankTriggerActivated(Trigger* trigger)
{
    srVector3T<float> position;
    W8NpcState* npc;
    Trigger* pFlagTrigger;
    W8Prop* prop;
    int value;

    if (g_status_685170.item_in_cursor != 0) {
        return false;
    }
    if (GetLocationVarIDByName("FlagPosition") == -1) {
        CreateLocationVar("FlagPosition", 0);
        npc = GetNpcStateByKind(0x2a);
        QueueNpcScriptNotice(npc, 0, 0x90, 1, 0);
        g_flag_00606994 = 1;
        return false;
    }
    value = GetLocationVarValueByName("FlagPosition");
    if (value == 0) {
        ControlLiftGate(0xEFFFFFFF);
        trigger->flag_0a0_08 = 0;
        npc = GetNpcStateByKind(0x2a);
        QueueNpcScriptNotice(npc, 0, 0x16, 1, 0);
        return true;
    }
    if (value >= 100 && value < 1000) {
        pFlagTrigger = FindTriggerByName("flag");
        if (pFlagTrigger == 0) {
            srAssertFail("pFlagTrigger", MTGIGASOUTER_CPP, 0x54,
                         "Missing trigger 'flag'! It's not in the LVL file!");
        }
        if (pFlagTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = pFlagTrigger->m_pProp;
        prop->GetPosition0044E2C0(&position);
        position.y = LIFT_PARKED_Y;
        prop->SetPosition0044E310(&position);
        SetTriggerVariableByName00444030("FlagPosition", 1000);
        ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, 0x290, 0, 0, 0);
        SetItemCursor(0);
        npc = GetNpcStateByKind(0x2a);
        QueueNpcScriptNotice(npc, 0, 0x18, 1, 0);
        ResumeNpc(npc, 1);
        if (FindEntityByName("NP_catwalk", &position, 0, 0) != 0) {
            SpawnMonsters(0x14a, 1, &position, 1, 1, 0, 0);
        }
        return true;
    }
    return false;
}

/* The lift's master function. Registered while the gate runs so the
   dispatcher ticks it every frame with command zero; other callers command
   it directly: a positive command moves the lift that many percent of the
   ride, 0xEFFFFFFF re-arms the ride from the top, and -1 persists the current
   progress into the FlagPosition location variable. The prop slides from
   5228.6 down to -2000.0 while the gate runs; -5000.0 parks it below. */
// FUNCTION: WIZ8 0x004DC080
void ControlLiftGate(int command)
{
    srVector3T<float> position;
    Trigger* pFlagTrigger;
    float progress;
    int value;

    g_flag_006834dc = 0;
    if (command != 0) {
        value = 0;
        if (command == -1) {
            if (g_lift_gate_6834fc != 0) {
                value = static_cast<int>(g_lift_gate_6834fc->GetProgress() * 100.0f);
            }
            if (GetLocationVarIDByName("FlagPosition") == -1) {
                CreateLocationVar("FlagPosition", value);
            } else {
                SetTriggerVariableByName00444030("FlagPosition", value);
            }
            return;
        }
        if (command != static_cast<int>(0xEFFFFFFF)) {
            value = command;
        }
        pFlagTrigger = FindTriggerByName("flag");
        if (pFlagTrigger == 0) {
            srAssertFail("pFlagTrigger", MTGIGASOUTER_CPP, 0x95,
                         "Missing trigger 'flag'! It's not in the LVL file!");
        }
        if (pFlagTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        g_lift_prop_683500 = pFlagTrigger->m_pProp;
        g_lift_prop_683500->GetPosition0044E2C0(&position);
        if (value < 100) {
            g_master_functions_006834d8->Add(ControlLiftGate);
            if (g_lift_gate_6834fc != 0) {
                g_lift_gate_6834fc->Arm();
            } else {
                g_lift_gate_6834fc = new W8IntervalGate(3.0f, 0, 1);
            }
            if (value > 0) {
                g_lift_gate_6834fc->SetProgress(static_cast<float>(value) *
                                                g_movement_speed_step_005ed490);
            }
            position.y = (LIFT_TOP_Y - LIFT_BOTTOM_Y) *
                             (g_float_005ebb38 -
                              static_cast<float>(value) * g_movement_speed_step_005ed490) +
                         LIFT_BOTTOM_Y;
        } else {
            if (value == 100) {
                position.y = LIFT_BOTTOM_Y;
            } else {
                position.y = LIFT_PARKED_Y;
            }
        }
        g_lift_prop_683500->SetPosition0044E310(&position);
        return;
    }
    if (g_lift_gate_6834fc != 0) {
        if (!g_lift_gate_6834fc->IsFinished()) {
            g_lift_gate_6834fc->PollElapsedIntervals();
        }
        if (!g_lift_gate_6834fc->IsFinished()) {
            progress = g_lift_gate_6834fc->GetProgress();
        } else {
            progress = 1.0f;
            SetTriggerVariableByName00444030("FlagPosition", 100);
            g_flag_006834dc = 1;
        }
        g_lift_prop_683500->GetPosition0044E2C0(&position);
        position.y = (g_float_005ebb38 - progress) * (LIFT_TOP_Y - LIFT_BOTTOM_Y) + LIFT_BOTTOM_Y;
        g_lift_prop_683500->SetPosition0044E310(&position);
    } else {
        g_flag_006834dc = 1;
    }
}

/* Activation callback installed on the "Security Button" trigger. The first
   press lowers the Umpani faction disposition and starts the camp alarm;
   while the UmpaniCampAlarm variable exists the button does nothing. */
// FUNCTION: WIZ8 0x004DC390
bool OnSecurityButtonActivated(Trigger* trigger)
{
    g_flag_00606994 = 1;
    if (GetLocationVarIDByName("UmpaniCampAlarm") != -1) {
        return false;
    }
    SetFactionDispositionBand(4, 0);
    ControlCampAlarm(0xEFFFFFFF);
    return true;
}

/* The camp alarm's master function, same shape as ControlLiftGate: command
   zero ticks the alarm gate while registered, -1 writes the remaining
   seconds into UmpaniCampAlarm, and any other command plays Alarm1.wav at
   the camera and re-arms the gate (clamped to 30 seconds). When the gate
   finishes the sound is released and the variable is reset to zero. */
// FUNCTION: WIZ8 0x004DC3D0
void ControlCampAlarm(int command)
{
    srVector3T<float> position;

    g_flag_006834dc = 0;
    if (command != 0) {
        if (GetLocationVarIDByName("UmpaniCampAlarm") == -1) {
            CreateLocationVar("UmpaniCampAlarm", 0x1e);
        }
        if (command == -1) {
            if (g_alarm_gate_683508 == 0) {
                SetTriggerVariableByName00444030("UmpaniCampAlarm", 0);
            } else {
                SetTriggerVariableByName00444030(
                    "UmpaniCampAlarm", static_cast<int>(g_alarm_gate_683508->GetElapsedSeconds()));
            }
            return;
        }
        position = GetWorld()->camera->getLocation();
        if (static_cast<unsigned int>(command) > 0x1e) {
            command = 0x1e;
        }
        if (g_alarm_sound_683504 != 0) {
            g_alarm_sound_683504->release();
            g_alarm_sound_683504 = 0;
        }
        g_alarm_sound_683504 =
            CreateAndPlaySoundNode("Data\\Sound\\Ambients\\Alarm1.wav", position, 1.0f, 75.0f, 1);
        if (g_alarm_gate_683508 != 0) {
            delete g_alarm_gate_683508;
            g_alarm_gate_683508 = 0;
        }
        g_alarm_gate_683508 =
            new W8IntervalGate(static_cast<float>(static_cast<unsigned int>(command)), 0, 1);
        if (g_alarm_gate_683508 != 0) {
            g_master_functions_006834d8->Add(ControlCampAlarm);
        }
    }
    if (!g_alarm_gate_683508->IsFinished()) {
        g_alarm_gate_683508->PollElapsedIntervals();
    }
    if (!g_alarm_gate_683508->IsFinished()) {
        return;
    }
    g_flag_006834dc = 1;
    if (g_alarm_sound_683504 != 0) {
        g_alarm_sound_683504->release();
        g_alarm_sound_683504 = 0;
    }
    if (g_alarm_gate_683508 != 0) {
        delete g_alarm_gate_683508;
    }
    g_alarm_gate_683508 = 0;
    SetTriggerVariableByName00444030("UmpaniCampAlarm", 0);
}

/* Activation callback on VOC_EWAXXSENTRYtrig: hands the sentry NPC (kind
   0x59) the item the player is holding, if any, as script argument -1. */
// FUNCTION: WIZ8 0x004DC600
bool OnSentryTriggerActivated(Trigger* trigger)
{
    W8NpcState* npc = GetNpcStateByKind(0x59);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    return true;
}

/* Activation callback on ewaxxdoortrigger03: the door only responds while an
   item in the 0x268-0x26e key range sits in the player's hand. */
// FUNCTION: WIZ8 0x004DC640
bool OnEwaxxDoor03Activated(Trigger* trigger)
{
    if (g_status_685170.item_in_cursor != 0) {
        int item_id = GetItemInHand();
        if (item_id >= 0x268 && item_id <= 0x26e) {
            return true;
        }
    }
    return false;
}

/* Activation callback on dummytrigger: every live member of monster kind
   0x180 gives birth once. */
// FUNCTION: WIZ8 0x004DC670
bool OnDummyTriggerActivated(Trigger* trigger)
{
    W8MonsterGroup* group = FindNextExistingMonsterByID(0x180, 0);

    while (group != 0) {
        GiveBirthToMonster(group);
        group = FindNextExistingMonsterByID(0x180, group);
    }
    return true;
}
