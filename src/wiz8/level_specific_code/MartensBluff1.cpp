#include "wiz8/level_specific_code/MartensBluff1.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/location_variables.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/string_database.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"
#include "wiz8/dice.h"
#include "wiz8/utility.h"
#include "surrender/srMath.h"
#include "surrender/srCamera.h"
#include "soundman.h"
#include "random.h"

#define MARTENSBLUFF1_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\MartensBluff1.cpp"

/* Level Specific Code\MartensBluff1.cpp (level 5, Marten's Bluff).

   Attribution evidence: the assertions in MartensBluff1TeleportState
   quote this file's path at lines 761/779/784/789/794, and the spawn lookup
   in MartensBluff1TransportSpawn passes the path to
   MonsterGetIndexByLocationID at line 509. The level-5 block of
   InitializeLevelMasterFunctions registers the teleporter cluster
   (MR109, ButtonGigas, ButtonTrang, ButtonRift, ButtonMaten). */

// GLOBAL: WIZ8 0x00683558
bool g_teleport_running_683558;
// GLOBAL: WIZ8 0x0068355C
Trigger* g_door_controller_68355c;
// GLOBAL: WIZ8 0x00683560
stSound3D* g_gas_sound_00_683560;
// GLOBAL: WIZ8 0x00683564
stSound3D* g_gas_sound_01_683564;
// GLOBAL: WIZ8 0x00683568
W8IntervalGate* g_transport_gate_683568;

/* The level-5 entry point called from InitializeLevelMasterFunctions:
   caches the J-Doorcontroller trigger, seeds DialState, kicks the Trang
   transporter, raises the MR101 lift flag when the party is already beside it
   and starts or stops the two GasSpray particles' air-escaping loops from the
   saved gas state. */
// FUNCTION: WIZ8 0x004DEB40
void MartensBluff1Setup(void)
{
    srVector3T<float> position;
    srVector3T<float> trigger_position;
    Trigger* pTrigger;
    stParticle* particle;
    int dial_state;

    FindTriggerByName("Gas-Switch");
    g_door_controller_68355c = FindTriggerByName("J-Doorcontroller");
    g_gas_sound_00_683560 = 0;
    g_gas_sound_01_683564 = 0;
    g_teleport_running_683558 = 0;
    if (g_door_controller_68355c != 0 && GetLocationVarIDByName("DialState") == -1) {
        Random(8);
        Random(8);
        Random(8);
        CreateLocationVar("DialState", 0x470111);
    }
    MartensBluff1Transporter(static_cast<int>(0xEFFFFFFF));
    position = GetWorld()->camera->getLocation();
    pTrigger = FindTriggerByName("MR101");
    if (pTrigger != 0 && (pTrigger->flags_0a0 & W8_TRIGGER_POSITIONED) != 0) {
        pTrigger->GetPosition(&trigger_position);
        if ((trigger_position - position).Length() < g_double_005ec150) {
            FindTriggerByName("Lift2Marten2");
            if (GetLocationVarIDByName("LiftArrived") == -1) {
                CreateLocationVar("LiftArrived", 1);
                ShowString(gppStringList[0x1c6c / 4]);
            }
        }
    }
    if (g_door_controller_68355c != 0) {
        dial_state = 0;
        if (GetLocationVarIDByName("DialState") != -1) {
            dial_state = GetLocationVarValueByName("DialState");
        }
        particle = FindRegisteredParticle("GasSpray-00");
        if (particle != 0) {
            if ((dial_state & 0xf0000000) == 0) {
                particle->SetActive(0);
            } else {
                particle->getLocation(position);
                g_gas_sound_00_683560 = CreateAndPlaySoundNode(
                    "Data\\Sound\\Ambients\\Air_Escaping_Loop.wav", position, 0.3f, 30.0f, 1);
            }
        }
        particle = FindRegisteredParticle("GasSpray-01");
        if (particle != 0) {
            if ((dial_state & 0xf0000000) == 0) {
                particle->SetActive(0);
            } else {
                particle->getLocation(position);
                g_gas_sound_01_683564 = CreateAndPlaySoundNode(
                    "Data\\Sound\\Ambients\\Air_Escaping_Loop.wav", position, 0.3f, 30.0f, 1);
            }
        }
    }
}

/* The "F-Handlock" trap callback: already-unlocked or the right hand item
   (0x26f) opens it; otherwise it shocks the whole party for 2d4+1. */
// FUNCTION: WIZ8 0x004DEDB0
bool MartensBluff1FHandlock(Trigger* pTrigger)
{
    W8Dice dice;

    if (pTrigger->running != 0) {
        return true;
    }
    if (g_status_685170.item_in_cursor != 0 && GetItemInHand() == 0x26f) {
        return true;
    }
    SetDice(&dice, 2, 4, 1);
    ApplyRolledHealthChangeToParty(&dice, 0, 1);
    SoundPlay("Data\\Sound\\Ambients\\Electricity 04.wav", 0);
    g_trigger_feedback_00606994 = 1;
    return false;
}

/* The "Dial-A" callback: advance the lowest DialState digit modulo 8. */
// FUNCTION: WIZ8 0x004DEE10
bool MartensBluff1DialA(Trigger* pTrigger)
{
    int state;

    if (g_door_controller_68355c == 0) {
        return false;
    }
    state = GetLocationVarValueByName("DialState");
    state = ((state & 0xf) + 1) % 8 | (state & 0xfffffff0);
    SetTriggerVariableByName("DialState", state);
    return true;
}

/* The "Dial-B" callback: advance the second DialState digit modulo 8. */
// FUNCTION: WIZ8 0x004DEE50
bool MartensBluff1DialB(Trigger* pTrigger)
{
    int state;

    if (g_door_controller_68355c == 0) {
        return false;
    }
    state = GetLocationVarValueByName("DialState");
    state = (((state >> 4) & 0xf) + 1) % 8 << 4 | (state & 0xffffff0f);
    SetTriggerVariableByName("DialState", state);
    return true;
}

/* The "Dial-C" callback: advance the third DialState digit modulo 8. */
// FUNCTION: WIZ8 0x004DEEA0
bool MartensBluff1DialC(Trigger* pTrigger)
{
    int state;

    if (g_door_controller_68355c == 0) {
        return false;
    }
    state = GetLocationVarValueByName("DialState");
    state = (((state >> 8) & 0xf) + 1) % 8 << 8 | (state & 0xfffff0ff);
    SetTriggerVariableByName("DialState", state);
    return true;
}

/* The "Gas-Switch" callback: toggles the 0x1000 gas-disable digit of
   DialState. Re-enabling only clears the digit; disabling also stops the two
   GasSpray particles and their looping sounds. */
// FUNCTION: WIZ8 0x004DEEF0
bool MartensBluff1GasSwitch(Trigger* pTrigger)
{
    stParticle* particle;
    int state;

    if (g_door_controller_68355c == 0) {
        return false;
    }
    state = GetLocationVarValueByName("DialState");
    if ((state & 0xf000) != 0) {
        state &= ~0xf000;
        SetTriggerVariableByName("DialState", state);
        return true;
    }
    state = (state & 0xfff0fff) | 0x1000;
    particle = FindRegisteredParticle("GasSpray-00");
    if (particle != 0) {
        particle->SetActive(0);
    }
    particle = FindRegisteredParticle("GasSpray-01");
    if (particle != 0) {
        particle->SetActive(0);
    }
    if (g_gas_sound_00_683560 != 0) {
        g_gas_sound_00_683560->Stop();
        g_gas_sound_00_683560 = 0;
    }
    if (g_gas_sound_01_683564 != 0) {
        g_gas_sound_01_683564->Stop();
        g_gas_sound_01_683564 = 0;
    }
    SetTriggerVariableByName("DialState", state);
    return true;
}

/* The "J-Doorcontroller" callback: opens while the three dialed digits match
   the combination stored in bits 16-27 of DialState. A wrong code sprays gas
   (unless the gas switch disabled it) and casts spell 0x25 at the party. */
// FUNCTION: WIZ8 0x004DEFB0
bool MartensBluff1JDoorController(Trigger* pTrigger)
{
    srVector3T<float> position;
    stParticle* particle;
    int state;

    if (g_door_controller_68355c == 0 || pTrigger->running != 0) {
        return false;
    }
    state = GetLocationVarValueByName("DialState");
    if ((state >> 0x10 & 0xfff) == (state & 0xfff)) {
        return true;
    }
    if ((state & 0xf000) != 0) {
        return false;
    }
    g_trigger_feedback_00606994 = 1;
    particle = FindRegisteredParticle("GasSpray-00");
    if (particle != 0) {
        particle->SetActive(1);
        particle->getLocation(position);
        g_gas_sound_00_683560 = CreateAndPlaySoundNode(
            "Data\\Sound\\Ambients\\Air_Escaping_Loop.wav", position, 0.3f, 30.0f, 1);
    }
    particle = FindRegisteredParticle("GasSpray-01");
    if (particle != 0) {
        particle->SetActive(1);
        particle->getLocation(position);
        g_gas_sound_01_683564 = CreateAndPlaySoundNode(
            "Data\\Sound\\Ambients\\Air_Escaping_Loop.wav", position, 0.3f, 30.0f, 1);
    }
    position = GetWorld()->camera->getLocation();
    PointCastSpell(position, 0x25, 5);
    state |= 0x10000000;
    SetTriggerVariableByName("DialState", state);
    return false;
}

/* The "Controller" callback: toggles fact 0x42. */
// FUNCTION: WIZ8 0x004DF120
bool MartensBluff1Controller(Trigger* pTrigger)
{
    if (GetFact(0x42) == 0) {
        SetFact(0x42, 1, 0);
    } else {
        SetFact(0x42, 0, 0);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* The Trang transporter spawn: while fact 0x43 is unset and combat is off it
   finds the two ANTRHACAX entities, plays the transporter sound at the first,
   spawns monster group 0x177 there and hands the spawned monster its move
   script. Answers false only when the fact was already set. */
// FUNCTION: WIZ8 0x004DF160
bool MartensBluff1TransportSpawn(void)
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
        if (info != 0 && info->p3D != 0) {
            info->p3D->SetScript("M1_Trang_trans.MSF", 1);
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
void MartensBluff1Transporter(int command)
{
    g_flag_006834dc = false;
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
            g_master_functions_006834d8->Add(MartensBluff1Transporter);
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
            SetTriggerVariableByName("TransportSpawn", progress);
        }
        return;
    }
    if (g_transport_gate_683568 == 0) {
        g_flag_006834dc = true;
        return;
    }
    if (!g_transport_gate_683568->IsFinished()) {
        g_transport_gate_683568->PollElapsedIntervals();
        if (!g_transport_gate_683568->IsFinished()) {
            return;
        }
    }
    if (MartensBluff1TransportSpawn()) {
        g_transport_gate_683568->Arm();
        return;
    }
    delete g_transport_gate_683568;
    g_transport_gate_683568 = 0;
    g_flag_006834dc = true;
}

/* "MR109": the teleporter pad. Sends the party to the destination indexed by
   the TeleporterState location variable, picking a random pad when none has
   been chosen yet. Always answers false so the trigger stays armed. */
// FUNCTION: WIZ8 0x004DF4A0
bool MartensBluff1Teleporter(Trigger* pTrigger)
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
bool MartensBluff1ButtonGigas(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState(1);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* "ButtonTrang": advances the teleporter to pad 2. */
// FUNCTION: WIZ8 0x004DF560
bool MartensBluff1ButtonTrang(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState(2);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* "ButtonRift": advances the teleporter to pad 3. */
// FUNCTION: WIZ8 0x004DF580
bool MartensBluff1ButtonRift(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState(3);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* "ButtonMaten": advances the teleporter to pad 4. */
// FUNCTION: WIZ8 0x004DF5A0
bool MartensBluff1ButtonMaten(Trigger* pTrigger)
{
    if (g_teleport_running_683558 == 0) {
        return MartensBluff1TeleportState(4);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* Shared teleporter state advance. Asserts the MR109 trigger exists, reads
   the current TeleporterState and, when it names a different live pad, runs
   that pad's button trigger before writing the new state. The re-entrancy
   flag keeps the button callbacks from recursing back in. */
// FUNCTION: WIZ8 0x004DF5C0
bool MartensBluff1TeleportState(int new_state)
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
    SetTriggerVariableByName("TeleporterState", new_state);
    g_teleport_running_683558 = 0;
    return true;
}

/* The "WireTrigger" callback: latches TeleporterState 1 (returning false when
   it is already nonzero) and replays the ButtonGigas sequence under the
   teleport-running guard. */
// FUNCTION: WIZ8 0x004DF710
bool MartensBluff1WireTrigger(Trigger* pTrigger)
{
    Trigger* pTelTrigger;

    g_trigger_feedback_00606994 = 1;
    pTelTrigger = FindTriggerByName("MR109");
    if (pTelTrigger == 0) {
        srAssertFail("pTelTrigger", MARTENSBLUFF1_CPP, 0x329,
                     "Missing trigger 'MR109'! It's not in the LVL file!");
    }
    if (GetLocationVarIDByName("TeleporterState") == -1) {
        CreateLocationVar("TeleporterState", 1);
    } else {
        if (GetLocationVarValueByName("TeleporterState") != 0) {
            return false;
        }
        SetTriggerVariableByName("TeleporterState", 1);
    }
    g_teleport_running_683558 = 1;
    pTelTrigger = FindTriggerByName("ButtonGigas");
    if (pTelTrigger == 0) {
        srAssertFail("pTelTrigger", MARTENSBLUFF1_CPP, 0x336,
                     "Missing trigger 'ButtonGigas'! It's not in the LVL file!");
    }
    pTelTrigger->Run(-1);
    g_teleport_running_683558 = 0;
    return true;
}

/* The "MartenBook" callback: latches fact 0x268. */
// FUNCTION: WIZ8 0x004DF7E0
bool MartensBluff1MartenBook(Trigger* pTrigger)
{
    SetFact(0x268, 1, 0);
    return true;
}
