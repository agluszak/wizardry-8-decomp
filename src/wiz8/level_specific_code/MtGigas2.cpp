#include "wiz8/level_specific_code/MtGigas2.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/location_variables.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/fact_state.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/xstatus.h"
#include "wiz8/utility.h"
#include "wiz8/dice.h"
#include "wiz8/sr_api.h"
#include "surrender/srCamera.h"
#include "soundman.h"

/* Level Specific Code\MtGigas2.cpp (level 0x0d).

   Attribution evidence: InitializeLevelMasterFunctions004D6C50 runs
   0x004DB200 and registers 0x004DB380-0x004DBA90 under case 0x0d, and the
   range sits between the Trynnie1 TU and the MtGigasOuter TU which starts
   at 0x004DBE70. The UmpaniAlarm master at 0x004DB860 is the same shape as
   ControlCampAlarm (0x004DC3D0) in MtGigasOuter.cpp. */

// GLOBAL: WIZ8 0x006834e8
stSound3D* g_alarm_sound_6834e8;
// GLOBAL: WIZ8 0x006834ec
W8IntervalGate* g_alarm_gate_6834ec;

static void MtGigas2WireShock004DB530(void);
void MtGigas2UmpaniAlarm004DB860(int command);

/* Level-load restore. Recreates the wire-panel location variable (armed but
   untouched reads back -1), re-runs the three wire triggers when the panel
   was already solved, restarts a saved UmpaniAlarm countdown, and moves the
   Sergeant Rubble NPC to his covert when the rubble teleport was pending. */
// FUNCTION: WIZ8 0x004DB200
void MtGigas2Setup004DB200(void)
{
    Trigger* pTrigger;

    if (GetLocationVarIDByName("WirePanel") != -1) {
        if (GetLocationVarValueByName("WirePanel") == 3) {
            pTrigger = FindTriggerByName("redwire");
            if (pTrigger != 0) {
                pTrigger->Run(-1);
            }
            pTrigger = FindTriggerByName("bluewire");
            if (pTrigger != 0) {
                pTrigger->Run(-1);
            }
            pTrigger = FindTriggerByName("yellowwire");
            if (pTrigger != 0) {
                pTrigger->Run(-1);
            }
        }
    } else {
        CreateLocationVar("WirePanel", -1);
    }
    if (GetLocationVarIDByName("UmpaniAlarm") != -1) {
        int alarm = GetLocationVarValueByName("UmpaniAlarm");
        if (alarm != 0) {
            MtGigas2UmpaniAlarm004DB860(alarm);
        }
    }
    if (GetFact(0x89) == 0 && GetLocationVarIDByName("CODESgtRubbleTeleport") != -1 &&
        GetLocationVarValueByName("CODESgtRubbleTeleport") == 2) {
        W8NpcState* npc;
        W8MonsterInfo* info;
        srVector3T<float> position;

        SetTriggerVariableByName00444030("CODESgtRubbleTeleport", 1);
        pTrigger = FindTriggerByName("door08");
        if (pTrigger != 0) {
            pTrigger->Run(-1);
        }
        SetFact(0x21e, 0, 0);
        npc = GetNpcStateByKind(0x29);
        if (npc != 0) {
            info = GetNpcMonsterInfo(npc);
            if (info != 0 && FindEntityByName("RubbleCovert", &position, 0, 0)) {
                info->monster->SetPosition(&position);
            }
        }
    }
}

/* Activation callback on _VOC_EWAXXTRAIN: queues the wire-panel NPC notice
   with the held item, and accepts the 0x269/0x26c wire items to arm the
   panel (value 0) when it was still untouched (-1). */
// FUNCTION: WIZ8 0x004DB380
bool MtGigas2Train004DB380(Trigger* pTrigger)
{
    W8NpcState* npc;
    W8ItemInstance* item;
    int item_id;
    int value;

    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    npc = GetNpcStateByKind(0x5b);
    item = 0;
    item_id = 0;
    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
        item_id = GetItemInHand();
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    value = GetLocationVarValueByName("WirePanel");
    if (value == 3) {
        return false;
    }
    if (item_id == 0x269 || item_id == 0x26c) {
        SoundPlay("Data\\Sound\\Ambients\\Electricity 01.wav", 0);
        if (value == -1) {
            SetTriggerVariableByName00444030("WirePanel", 0);
            return true;
        }
    }
    return false;
}

/* Activation callback on redwire: the first wire moves the panel from 0 to
   1; any other order shocks the party. */
// FUNCTION: WIZ8 0x004DB420
bool MtGigas2RedWire004DB420(Trigger* pTrigger)
{
    int value = GetLocationVarValueByName("WirePanel");

    if (value == 3) {
        return true;
    }
    g_flag_00606994 = 1;
    if (value != 0) {
        MtGigas2WireShock004DB530();
        return false;
    }
    SetTriggerVariableByName00444030("WirePanel", 1);
    return true;
}

/* Activation callback on bluewire: valid only as the second wire (1 -> 2). */
// FUNCTION: WIZ8 0x004DB460
bool MtGigas2BlueWire004DB460(Trigger* pTrigger)
{
    int value = GetLocationVarValueByName("WirePanel");

    if (value == 3) {
        return true;
    }
    g_flag_00606994 = 1;
    if (value != 1) {
        MtGigas2WireShock004DB530();
        return false;
    }
    SetTriggerVariableByName00444030("WirePanel", 2);
    return true;
}

/* Activation callback on yellowwire: valid only as the last wire (2 -> 3);
   on success it latches fact 0xa7 and re-arms all three wire triggers. */
// FUNCTION: WIZ8 0x004DB4A0
bool MtGigas2YellowWire004DB4A0(Trigger* pTrigger)
{
    int value = GetLocationVarValueByName("WirePanel");
    Trigger* wire;

    if (value == 3) {
        return true;
    }
    g_flag_00606994 = 1;
    if (value != 2) {
        MtGigas2WireShock004DB530();
        return false;
    }
    SetFact(0xa7, 1, 0);
    SetTriggerVariableByName00444030("WirePanel", 3);
    pTrigger->flag_0a0_08 = 0;
    wire = FindTriggerByName("redwire");
    if (wire != 0) {
        wire->flag_0a0_08 = 0;
    }
    wire = FindTriggerByName("bluewire");
    if (wire != 0) {
        wire->flag_0a0_08 = 0;
    }
    return true;
}

/* Shared wrong-wire penalty: resets the panel and all three wire props, then
   rolls 2d4+1 electric damage into the party with the spark sound. The third
   wire prop is looked up under the misspelled name "yellowire". */
// FUNCTION: WIZ8 0x004DB530
static void MtGigas2WireShock004DB530(void)
{
    Trigger* pTrigger;
    W8Prop* prop;
    W8Dice dice;

    SetTriggerVariableByName00444030("WirePanel", 0);
    prop = 0;
    pTrigger = FindTriggerByName("redwire");
    if (pTrigger != 0) {
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = pTrigger->m_pProp;
    }
    if (prop != 0) {
        prop->SetSetting66(0);
    }
    pTrigger = FindTriggerByName("bluewire");
    if (pTrigger != 0) {
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = pTrigger->m_pProp;
    }
    if (prop != 0) {
        prop->SetSetting66(0);
    }
    pTrigger = FindTriggerByName("yellowire");
    if (pTrigger != 0) {
        if (pTrigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = pTrigger->m_pProp;
    }
    if (prop != 0) {
        prop->SetSetting66(0);
    }
    SetDice(&dice, 2, 4, 1);
    ApplyRolledHealthChangeToParty(&dice, 0, 1);
    SoundPlay("Data\\Sound\\Ambients\\Electricity 04.wav", 0);
}

/* Activation callback on _VOC_EWAXXLIFT3: queues the lift NPC notice with
   the held item, if any. */
// FUNCTION: WIZ8 0x004DB650
bool MtGigas2Lift3004DB650(Trigger* pTrigger)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    W8NpcState* npc = GetNpcStateByKind(0x5c);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    return false;
}

/* Activation callback on _VOC_EWAXXTOPDOOR1: queues the top-door NPC notice
   with the held item, if any. */
// FUNCTION: WIZ8 0x004DB690
bool MtGigas2TopDoor1004DB690(Trigger* pTrigger)
{
    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    W8NpcState* npc = GetNpcStateByKind(0x5f);
    W8ItemInstance* item = 0;

    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    return false;
}

/* Activation callback on _VOC_EWAXXOFFICER1: while ObstacleDoors bit 0 is
   set, runs Door19 and clears the bit; otherwise queues the officer NPC
   notice with the held item. */
// FUNCTION: WIZ8 0x004DB6D0
bool MtGigas2Officer1004DB6D0(Trigger* pTrigger)
{
    int doors;
    W8NpcState* npc;
    W8ItemInstance* item;

    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    doors = 0;
    if (GetLocationVarIDByName("ObstacleDoors") != -1) {
        doors = GetLocationVarValueByName("ObstacleDoors");
    }
    if ((doors & 1) != 0) {
        Trigger* door = FindTriggerByName("Door19");
        door->Run(-1);
        doors--;
        SetTriggerVariableByName00444030("ObstacleDoors", doors);
        return false;
    }
    npc = GetNpcStateByKind(0x7c);
    item = 0;
    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    return false;
}

/* Activation callback on _VOC_EWAXXOFFICER2: while ObstacleDoors bit 1 is
   set, runs Door18 and clears the bit; otherwise queues the officer NPC
   notice with the held item. */
// FUNCTION: WIZ8 0x004DB770
bool MtGigas2Officer2004DB770(Trigger* pTrigger)
{
    int doors;
    W8NpcState* npc;
    W8ItemInstance* item;

    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    doors = 0;
    if (GetLocationVarIDByName("ObstacleDoors") != -1) {
        doors = GetLocationVarValueByName("ObstacleDoors");
    }
    if ((doors & 2) != 0) {
        Trigger* door = FindTriggerByName("Door18");
        door->Run(-1);
        doors -= 2;
        SetTriggerVariableByName00444030("ObstacleDoors", doors);
        return false;
    }
    npc = GetNpcStateByKind(0x7d);
    item = 0;
    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    g_flag_00606994 = 1;
    return false;
}

/* Activation callback on triggerPlaneLaserAlarm/01: once per level (fact
   0xa5 and the UmpaniAlarm variable both unset) turns the Umpani faction
   hostile and starts the intruder alarm. */
// FUNCTION: WIZ8 0x004DB810
bool MtGigas2LaserAlarm004DB810(Trigger* pTrigger)
{
    g_flag_00606994 = 1;
    if (GetFact(0xa5) != 0) {
        return false;
    }
    if (GetLocationVarIDByName("UmpaniAlarm") != -1) {
        return false;
    }
    SetFactionDispositionBand(4, 0);
    MtGigas2UmpaniAlarm004DB860(0xEFFFFFFF);
    return true;
}

/* The Umpani alarm's master function, same shape as ControlCampAlarm in
   MtGigasOuter.cpp: command zero ticks the alarm gate while registered, -1
   writes the remaining seconds into UmpaniAlarm, and any other command plays
   the intruder warning at the camera and arms the gate (clamped to 30
   seconds). Unlike ControlCampAlarm the gate and sound are only created when
   both are absent, and finishing stops the sound but keeps its node. */
// FUNCTION: WIZ8 0x004DB860
void MtGigas2UmpaniAlarm004DB860(int command)
{
    srVector3T<float> position;

    g_flag_006834dc = 0;
    if (command != 0) {
        if (GetLocationVarIDByName("UmpaniAlarm") == -1) {
            CreateLocationVar("UmpaniAlarm", 0x1e);
        }
        if (command == -1) {
            SetTriggerVariableByName00444030(
                "UmpaniAlarm", g_alarm_gate_6834ec != 0
                                   ? static_cast<int>(g_alarm_gate_6834ec->GetElapsedSeconds())
                                   : 0);
            return;
        }
        if (g_alarm_gate_6834ec != 0 || g_alarm_sound_6834e8 != 0) {
            return;
        }
        position = GetWorld()->camera->getLocation();
        if (static_cast<unsigned int>(command) > 0x1e) {
            command = 0x1e;
        }
        g_alarm_gate_6834ec =
            new W8IntervalGate(static_cast<float>(static_cast<unsigned int>(command)), 0, 1);
        g_alarm_sound_6834e8 =
            CreateAndPlaySoundNode("Data\\Sound\\VOCs\\VOC_HLLIntruder\\VOC_HLLIntruder_003.wav",
                                   position, 1.0f, 75.0f, 1);
        if (g_alarm_sound_6834e8 != 0 && g_alarm_gate_6834ec != 0) {
            g_master_functions_006834d8->Add(MtGigas2UmpaniAlarm004DB860);
        }
    }
    if (!g_alarm_gate_6834ec->IsFinished()) {
        g_alarm_gate_6834ec->PollElapsedIntervals();
        if (!g_alarm_gate_6834ec->IsFinished()) {
            return;
        }
    }
    g_flag_006834dc = 1;
    g_alarm_sound_6834e8->Stop();
    if (g_alarm_gate_6834ec != 0) {
        delete g_alarm_gate_6834ec;
    }
    g_alarm_gate_6834ec = 0;
    SetTriggerVariableByName00444030("UmpaniAlarm", 0);
}

/* Activation callback on wiringMalfunction: plays the short spark sound. */
// FUNCTION: WIZ8 0x004DBA70
bool MtGigas2WiringMalfunction004DBA70(Trigger* pTrigger)
{
    SoundPlay("Data\\Sound\\Ambients\\Electricity 04.wav", 0);
    return true;
}

/* Activation callback on accessHatch: plays the falling-statue sound. */
// FUNCTION: WIZ8 0x004DBA90
bool MtGigas2AccessHatch004DBA90(Trigger* pTrigger)
{
    SoundPlay("Data\\Sound\\Ambients\\Sign Falling Statue.wav", 0);
    return true;
}
