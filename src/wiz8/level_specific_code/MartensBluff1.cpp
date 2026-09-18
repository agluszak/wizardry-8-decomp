#include "wiz8/level_specific_code/MartensBluff1.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/location_variables.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"
#include "surrender/srMath.h"
#include "random.h"

#define MARTENSBLUFF1_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MartensBluff1.cpp"

/* Level Specific Code\MartensBluff1.cpp (level 5, Marten's Bluff).

   Attribution evidence: the assertions in MartensBluff1TeleportState004DF5C0
   quote this file's path at lines 761/779/784/789/794, and the spawn lookup
   in MartensBluff1TransportSpawn004DF160 passes the path to
   MonsterGetIndexByLocationID at line 509. The level-5 block of
   InitializeLevelMasterFunctions004D6C50 registers the teleporter cluster
   (MR109, ButtonGigas, ButtonTrang, ButtonRift, ButtonMaten). */

// GLOBAL: WIZ8 0x00683558
bool g_teleport_running_683558;
// GLOBAL: WIZ8 0x00683568
W8IntervalGate* g_transport_gate_683568;

/* The Trang transporter spawn: while fact 0x43 is unset and combat is off it
   finds the two ANTRHACAX entities, plays the transporter sound at the first,
   spawns monster group 0x177 there and hands the spawned monster its move
   script. Answers false only when the fact was already set. */
// FUNCTION: WIZ8 0x004DF160
bool MartensBluff1TransportSpawn004DF160(void)
{
    srVector3T<float> position_a;
    srVector3T<float> position_b;
    W8MonsterGroup* group;
    W8MonsterInfo* info;
    int location_id;

    if (GetFact(0x43)) {
        return false;
    }
    if (gXStatus.fCombatMode != 0) {
        return true;
    }
    if (!FindEntityByName("ANTRHACAX1", &position_a, 0, 0)) {
        return true;
    }
    if (!FindEntityByName("ANTRHACAX2", &position_b, 0, 0)) {
        return true;
    }
    CreateAndPlaySoundNode("Data\\Sound\\Ambients\\TrangTransporter.wav", position_a, 0.7f, 30.0f,
                           0);
    group = SpawnMonsters(0x177, 1, &position_a, 0, 1, 0, 0);
    location_id = IListGetAt(group->monsters, 0);
    if (location_id != 0) {
        info = MonsterGetScriptPartByLocationIndex(
            MonsterGetIndexByLocationID(0x1fd, MARTENSBLUFF1_CPP, location_id, 1));
        if (info != 0 && info->monster != 0) {
            info->monster->SetScript004C7F10("M1_Trang_trans.MSF", 1);
        }
    }
    return true;
}

/* The TransportSpawn master function. A nonzero command services it: -1
   persists the gate progress into the TransportSpawn location variable and
   0xEFFFFFFF re-arms the minute gate and registers the run. The command-0 run
   fires the transporter spawn each time the gate elapses until the spawn
   reports the fact already set, then deletes the gate and unregisters. */
// FUNCTION: WIZ8 0x004DF260
void MartensBluff1Transporter004DF260(int command)
{
    g_flag_006834dc = 0;
    if (command != 0) {
        int progress = 0;

        if (command == static_cast<int>(0xEFFFFFFF)) {
            if (GetFact(0x43)) {
                return;
            }
            if (g_transport_gate_683568 != 0) {
                g_transport_gate_683568->Arm();
            } else {
                g_transport_gate_683568 = new W8IntervalGate(60.0f, 0, 1);
            }
            if (GetLocationVarIDByName("TransportSpawn") != -1) {
                g_transport_gate_683568->SetProgress(GetLocationVarValueByName("TransportSpawn") *
                                                     g_movement_speed_step_005ed490);
            }
            g_master_functions_006834d8->Add(MartensBluff1Transporter004DF260);
            return;
        }
        if (command != -1) {
            return;
        }
        if (g_transport_gate_683568 != 0) {
            progress = static_cast<int>(g_transport_gate_683568->GetProgress() *
                                        g_octree_cell_scale_005ebcd0);
        }
        if (GetLocationVarIDByName("TransportSpawn") == -1) {
            CreateLocationVar("TransportSpawn", progress);
        } else {
            SetTriggerVariableByName00444030("TransportSpawn", progress);
        }
        return;
    }
    if (g_transport_gate_683568 == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (!g_transport_gate_683568->IsFinished()) {
        g_transport_gate_683568->PollElapsedIntervals();
        if (!g_transport_gate_683568->IsFinished()) {
            return;
        }
    }
    if (MartensBluff1TransportSpawn004DF160()) {
        g_transport_gate_683568->Arm();
        return;
    }
    delete g_transport_gate_683568;
    g_transport_gate_683568 = 0;
    g_flag_006834dc = 1;
}

/* "MR109": the teleporter pad. Sends the party to the destination indexed by
   the TeleporterState location variable, picking a random pad when none has
   been chosen yet. Always answers false so the trigger stays armed. */
// FUNCTION: WIZ8 0x004DF4A0
bool MartensBluff1Teleporter004DF4A0(Trigger* pTrigger)
{
    int state;

    if (GetLocationVarIDByName("TeleporterState") == -1) {
        CreateLocationVar("TeleporterState", 0);
        state = Random(4) + 1;
    } else {
        state = GetLocationVarValueByName("TeleporterState");
        if (state == 0) {
            state = Random(4) + 1;
        }
    }
    switch (state - 1) {
    case 0:
        pTrigger->RunDestination00440DD0("CT611");
        return false;
    case 1:
        pTrigger->RunDestination00440DD0("CT107");
        return false;
    case 2:
        pTrigger->RunDestination00440DD0("RIF01");
        return false;
    case 3:
        pTrigger->RunDestination00440DD0("MR207");
        return false;
    }
    return false;
}

/* "ButtonGigas": advances the teleporter to pad 1. While the state helper is
   running it only re-arms the consumed-input flag. */
// FUNCTION: WIZ8 0x004DF540
bool MartensBluff1ButtonGigas004DF540(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState004DF5C0(1);
    }
    g_flag_00606994 = 1;
    return true;
}

/* "ButtonTrang": advances the teleporter to pad 2. */
// FUNCTION: WIZ8 0x004DF560
bool MartensBluff1ButtonTrang004DF560(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState004DF5C0(2);
    }
    g_flag_00606994 = 1;
    return true;
}

/* "ButtonRift": advances the teleporter to pad 3. */
// FUNCTION: WIZ8 0x004DF580
bool MartensBluff1ButtonRift004DF580(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState004DF5C0(3);
    }
    g_flag_00606994 = 1;
    return true;
}

/* "ButtonMaten": advances the teleporter to pad 4. */
// FUNCTION: WIZ8 0x004DF5A0
bool MartensBluff1ButtonMaten004DF5A0(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState004DF5C0(4);
    }
    g_flag_00606994 = 1;
    return true;
}

/* Shared teleporter state advance. Asserts the MR109 trigger exists, reads
   the current TeleporterState and, when it names a different live pad, runs
   that pad's button trigger before writing the new state. The re-entrancy
   flag keeps the button callbacks from recursing back in. */
// FUNCTION: WIZ8 0x004DF5C0
bool MartensBluff1TeleportState004DF5C0(int new_state)
{
    Trigger* pTelTrigger;
    Trigger* pOldButtonTrigger;
    int state;

    pTelTrigger = FindTriggerByName("MR109");
    if (pTelTrigger == 0) {
        srAssertFail("pTelTrigger", MARTENSBLUFF1_CPP, 0x2f9,
                     "Missing trigger 'MR109'! It's not in the LVL file!");
    }
    if (GetLocationVarIDByName("TeleporterState") == -1) {
        CreateLocationVar("TeleporterState", 0);
        return false;
    }
    state = GetLocationVarValueByName("TeleporterState");
    if (state == 0) {
        return false;
    }
    if (state == new_state) {
        return false;
    }
    g_teleport_running_683558 = 1;
    switch (state - 1) {
    case 0:
        pOldButtonTrigger = FindTriggerByName("ButtonGigas");
        if (pOldButtonTrigger == 0) {
            srAssertFail("pOldButtonTrigger", MARTENSBLUFF1_CPP, 0x30b,
                         "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
        }
        break;
    case 1:
        pOldButtonTrigger = FindTriggerByName("ButtonTrang");
        if (pOldButtonTrigger == 0) {
            srAssertFail("pOldButtonTrigger", MARTENSBLUFF1_CPP, 0x310,
                         "Missing trigger 'ButtonTrang'! It's not in the LVL file!");
        }
        break;
    case 2:
        pOldButtonTrigger = FindTriggerByName("ButtonRift");
        if (pOldButtonTrigger == 0) {
            srAssertFail("pOldButtonTrigger", MARTENSBLUFF1_CPP, 0x315,
                         "Missing trigger 'ButtonRift'! It's not in the LVL file!");
        }
        break;
    case 3:
        pOldButtonTrigger = FindTriggerByName("ButtonMaten");
        if (pOldButtonTrigger == 0) {
            srAssertFail("pOldButtonTrigger", MARTENSBLUFF1_CPP, 0x31a,
                         "Missing trigger 'ButtonMaten'! It's not in the LVL file!");
        }
        break;
    default:
        goto set_state;
    }
    pOldButtonTrigger->Run(-1);
set_state:
    SetTriggerVariableByName00444030("TeleporterState", new_state);
    g_teleport_running_683558 = 0;
    return true;
}
