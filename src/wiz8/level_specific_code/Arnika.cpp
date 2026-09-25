#include "wiz8/level_specific_code/Arnika.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/cursor.h"
#include "wiz8/fact_state.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/location_variables.h"
#include "wiz8/sr_api.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/game_status.h"
#include "surrender/srMath.h"
#include "soundman.h"

#define ARNIKA_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Arnika.cpp"

/* Level Specific Code\Arnika.cpp (level 0, Arnika).

   Attribution evidence: the missing-trigger assertions quote this file's
   path at lines 752-764, 1026-1038, 1469, 1512, 1520 and 1543, and the
   assert expressions name the gEl01/gEl02 members. The level-0 block of
   InitializeLevelMasterFunctions004D6C50 registers the trigger cluster
   (ChaosMolori, Maddmook, CMbox, AstralDominae, Mookholo, MookFrontDoor,
   YellowButton, Vaultalarmdoor, Exitbutton, GenVault-2-door, ARN11,
   RedButton, El1-TopButtons, El1-BottomButtons, GreenButton, Elevator-02,
   LazerScanner, ScannerDoor). */

struct W8Elevator {
    int state;                   /* 0x00: persisted as El0XState */
    int moving;                  /* 0x04: persisted as El0XMoving */
    int button_down;             /* 0x08: persisted as Red/GreenButtonDown */
    Trigger* pElevator;          /* 0x0c */
    Trigger* pLift;              /* 0x10 */
    Trigger* pTopDoor;           /* 0x14 */
    Trigger* pBottomDoor;        /* 0x18 */
    Trigger* pTopDoorCollide;    /* 0x1c */
    Trigger* pBottomDoorCollide; /* 0x20 */
    Trigger* pButton;            /* 0x24 */
    W8Prop* pProp;               /* 0x28: rep the moving master waits on */
};

// GLOBAL: WIZ8 0x00683570
W8Elevator gEl01;
// GLOBAL: WIZ8 0x006835A0
W8Elevator gEl02;

// GLOBAL: WIZ8 0x00613DCC
bool g_red_button_armed_613dcc = true;
// GLOBAL: WIZ8 0x006835EC
W8Prop* g_el01_button_prop_6835ec;
// GLOBAL: WIZ8 0x006835F0
W8Prop* g_el02_button_prop_6835f0;

// GLOBAL: WIZ8 0x006835CC
W8Prop* g_lazer_prop_6835cc;
// GLOBAL: WIZ8 0x006835D0
W8Prop* g_exit_door_prop_6835d0;
// GLOBAL: WIZ8 0x006835D4
Trigger* g_exit_door_trigger_6835d4;
// GLOBAL: WIZ8 0x006835D8
int g_laser_scanning_6835d8;
// GLOBAL: WIZ8 0x006835DC
stSound3D* g_warning_loop_6835dc;
// GLOBAL: WIZ8 0x006835E0
stSound3D* g_warning_oneshot_6835e0;
// GLOBAL: WIZ8 0x006835E4
W8IntervalGate* g_warning_gate_6835e4;
// GLOBAL: WIZ8 0x006835E8
W8Monster* g_mookholo_monster_6835e8;

/* Level init: clears the laser-scanner and exit-door caches, runs both
   elevator setups, then restores the laser scan, the warning sound, the
   Screg flag and the inside-the-inn NPC teleport from the persisted
   location variables and fact 0xc1. */
// FUNCTION: WIZ8 0x004E06D0
void ArnikaLevelSetup004E06D0(void)
{
    srVector3T<float> position;
    W8NpcState* npc;
    W8MonsterInfo* monster_info;
    Trigger* pTrigger;

    g_lazer_prop_6835cc = 0;
    g_exit_door_prop_6835d0 = 0;
    g_exit_door_trigger_6835d4 = 0;
    ArnikaElevator1Setup004E13B0();
    ArnikaElevator2Setup004E1A10();
    g_flag_006834dd = 0;
    if (GetLocationVarIDByName("LaserScanning") != -1 &&
        GetLocationVarValueByName("LaserScanning") != 0) {
        pTrigger = FindTriggerByName("LazerScanner");
        if (pTrigger != 0) {
            if (pTrigger->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            g_lazer_prop_6835cc = pTrigger->m_pProp;
            g_laser_scanning_6835d8 = 1;
            BeginScriptedWorldAction();
            g_master_functions_006834d8->Add(ArnikaLaserScanMaster004E0960);
        }
    }
    if (GetLocationVarIDByName("WarningSound") != -1) {
        int value = GetLocationVarValueByName("WarningSound");
        if (value != 0) {
            ArnikaWarningSound004E0AC0(value);
        }
    }
    if (GetLocationVarIDByName("ScregActive") != -1 &&
        GetLocationVarValueByName("ScregActive") == 1) {
        npc = GetNpcStateByKind(0x17);
        if (npc == 0 || (monster_info = GetNpcMonsterInfo(npc)) == 0 || monster_info->p3D == 0) {
            SetTriggerVariableByName00444030("ScregActive", 0);
        }
    }
    if (GetFact(0xc1) != 0) {
        npc = GetNpcStateByKind(0x18);
        if (npc != 0) {
            monster_info = GetNpcMonsterInfo(npc);
            if (monster_info != 0 && monster_info->p3D != 0 &&
                FindEntityByName("Inside_Inn", &position, 0, 0) != 0) {
                monster_info->p3D->SetPosition(&position);
            }
        }
    }
}

/* LazerScanner trigger: caches the scanner prop, refuses while its rep is
   still animating or the HLL door is already open, then arms the scan. */
// FUNCTION: WIZ8 0x004E0880
bool ArnikaLazerScanner004E0880(Trigger* pTrigger)
{
    if (pTrigger->m_bRepType != 2) {
        srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                     0x3ed, 0);
    }
    g_lazer_prop_6835cc = pTrigger->m_pProp;
    if (g_lazer_prop_6835cc->Rep()->animation_playing_06d != 0) {
        return 0;
    }
    if (GetLocationVarIDByName("HLLDoorOpen") != -1) {
        return 0;
    }
    g_laser_scanning_6835d8 = 1;
    BeginScriptedWorldAction();
    g_master_functions_006834d8->Add(ArnikaLaserScanMaster004E0960);
    return 1;
}

/* Laser-scan master: a positive command re-arms the scan, -1 persists the
   state into LaserScanning, and a zero tick drops the scan once the prop's
   rep stops animating. */
// FUNCTION: WIZ8 0x004E0960
void ArnikaLaserScanMaster004E0960(int command)
{
    if (command != 0) {
        if (command == -1) {
            if (GetLocationVarIDByName("LaserScanning") == -1) {
                CreateLocationVar("LaserScanning", g_laser_scanning_6835d8);
            } else {
                SetTriggerVariableByName00444030("LaserScanning", g_laser_scanning_6835d8);
            }
            return;
        }
        g_laser_scanning_6835d8 = 1;
        BeginScriptedWorldAction();
        g_master_functions_006834d8->Add(ArnikaLaserScanMaster004E0960);
        return;
    }
    g_flag_006834dc = 0;
    if (g_lazer_prop_6835cc == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (g_lazer_prop_6835cc->Rep()->animation_playing_06d != 0) {
        return;
    }
    g_flag_006834dc = 1;
    ClearMainGameTargetState();
    g_laser_scanning_6835d8 = 0;
}

/* ScannerDoor trigger: the 0x27b key item opens the HLL door once. */
// FUNCTION: WIZ8 0x004E0A80
bool ArnikaScannerDoor004E0A80(Trigger* pTrigger)
{
    if (FindItemOnParty(0x27b, 0, 0, 2, 0) == 0) {
        return 0;
    }
    if (GetLocationVarIDByName("HLLDoorOpen") == -1) {
        CreateLocationVar("HLLDoorOpen", 1);
    }
    return 1;
}

/* Warning-sound master: a positive command starts the ULLspawn alarm for
   that many seconds (or just the one-shot above 0x1e), -1 persists the
   remaining time into WarningSound, and a zero tick runs the countdown and
   stops the looped VOC when it expires. */
// FUNCTION: WIZ8 0x004E0AC0
void ArnikaWarningSound004E0AC0(int command)
{
    srVector3T<float> position;

    g_flag_006834dc = 0;
    if (command != 0) {
        if (GetLocationVarIDByName("WarningSound") == -1) {
            CreateLocationVar("WarningSound", 0x1e);
        }
        if (command == -1) {
            if (g_warning_gate_6835e4 != 0) {
                SetTriggerVariableByName00444030(
                    "WarningSound", static_cast<int>(g_warning_gate_6835e4->GetElapsedSeconds()));
            } else {
                SetTriggerVariableByName00444030("WarningSound", 0x3e8);
            }
            return;
        }
        if (FindEntityByName("ULLspawn", &position, 0, 0) == 0) {
            return;
        }
        g_warning_gate_6835e4 = 0;
        if (command <= 0x1e) {
            g_warning_gate_6835e4 = new W8IntervalGate(static_cast<float>(command), 0, 1);
        } else {
            g_warning_oneshot_6835e0 = CreateAndPlaySoundNode(
                "Data\\Sound\\VOCs\\VOC_HLLIntruder\\VOC_HLLIntruder_002.wav", position, 1.0f,
                100.0f, 0);
        }
        g_warning_loop_6835dc =
            CreateAndPlaySoundNode("Data\\Sound\\VOCs\\VOC_HLLIntruder\\VOC_HLLIntruder_003.wav",
                                   position, 1.0f, 75.0f, 1);
        if (g_warning_loop_6835dc != 0 || g_warning_oneshot_6835e0 != 0) {
            g_master_functions_006834d8->Add(ArnikaWarningSound004E0AC0);
        }
    }
    if (g_warning_oneshot_6835e0 != 0) {
        if (g_warning_oneshot_6835e0->IsPlaying()) {
            return;
        }
        g_warning_oneshot_6835e0 = 0;
    }
    if (g_warning_gate_6835e4 == 0) {
        g_warning_gate_6835e4 = new W8IntervalGate(30.0f, 0, 1);
        return;
    }
    if (!g_warning_gate_6835e4->IsFinished()) {
        g_warning_gate_6835e4->PollElapsedIntervals();
        if (!g_warning_gate_6835e4->IsFinished()) {
            return;
        }
    }
    g_flag_006834dc = 1;
    g_warning_loop_6835dc->Stop();
    delete g_warning_gate_6835e4;
    g_warning_gate_6835e4 = 0;
    SetTriggerVariableByName00444030("WarningSound", 0);
}

/* Mookholo trigger: spawns the Screg ambush once while neither ScregActive
   nor MookDoorOpen is set, then hands the spawned monster to the fade-out
   watch and queues the NPC notice. */
// FUNCTION: WIZ8 0x004E0DC0
bool ArnikaMookholo004E0DC0(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8MonsterGroup* group;
    W8MonsterInfo* info;
    int location_id;

    if (gXStatus.fCombatMode != 0) {
        return 0;
    }
    if (GetLocationVarIDByName("ScregActive") != -1 &&
        GetLocationVarValueByName("ScregActive") != 0) {
        return 0;
    }
    if (GetLocationVarIDByName("MookDoorOpen") != -1 &&
        GetLocationVarValueByName("MookDoorOpen") != 0) {
        return 0;
    }
    if (FindEntityByName("Mookholo", &position, 0, 0) != 0) {
        group = SpawnMonsters(0xb, 1, &position, 0, 1, 0, 0);
        if (GetLocationVarIDByName("ScregActive") != -1) {
            SetTriggerVariableByName00444030("ScregActive", 1);
        } else {
            CreateLocationVar("ScregActive", 1);
        }
        location_id = IListGetAt(group->monsters, 0);
        if (location_id != 0) {
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x193, ARNIKA_CPP, location_id, 1));
            if (info != 0) {
                g_mookholo_monster_6835e8 = info->p3D;
                g_flag_6109f0 = 0;
                g_master_functions_006834d8->Add(ArnikaMookholoWatch004E0F70);
            }
            QueueNpcScriptNotice(FindNpcBindingForMonster(MonsterGetIndexByLocationID(
                                     0x199, ARNIKA_CPP, location_id, 1)),
                                 0, -1, 0, 0);
        }
    }
    return 1;
}

/* Mookholo watch: the 0xEFFFFFFF toggle re-arms the wait, and a zero tick
   fades the spawned monster out once g_flag_6109f0 signals the NPC notice
   finished. */
// FUNCTION: WIZ8 0x004E0F70
void ArnikaMookholoWatch004E0F70(int command)
{
    if (command != 0) {
        if (command == static_cast<int>(0xEFFFFFFF)) {
            g_flag_6109f0 = 0;
            g_master_functions_006834d8->Add(ArnikaMookholoWatch004E0F70);
        }
        return;
    }
    g_flag_006834dc = 0;
    if (g_flag_6109f0 != 0) {
        g_flag_006834dc = 1;
        if (g_mookholo_monster_6835e8 != 0) {
            g_mookholo_monster_6835e8->BeginFadeOutAndRemove004C5040(3);
            g_mookholo_monster_6835e8 = 0;
        }
    }
}

/* MookFrontDoor trigger: asserts the Mookholo trigger exists and opens the
   Mook door by latching MookDoorOpen. */
// FUNCTION: WIZ8 0x004E1040
bool ArnikaMookFrontDoor004E1040(Trigger* pTrigger)
{
    Trigger* pMookHolo = FindTriggerByName("Mookholo");

    if (pMookHolo == 0) {
        srAssertFail("pMookHolo", ARNIKA_CPP, 0x1ee,
                     "Missing trigger 'Mookholo'! It's not in the LVL file!");
    }
    if (GetLocationVarIDByName("MookDoorOpen") != -1) {
        SetTriggerVariableByName00444030("MookDoorOpen", 1);
    } else {
        CreateLocationVar("MookDoorOpen", 1);
    }
    return 1;
}

/* YellowButton trigger: raises the "nothing happened" flag, spawns the
   bank guards at Bguards and puts the first on the guard script. */
// FUNCTION: WIZ8 0x004E10A0
bool ArnikaYellowButton004E10A0(Trigger* pTrigger)
{
    srVector3T<float> position;
    W8MonsterGroup* group;
    W8MonsterInfo* info;

    g_trigger_feedback_00606994 = 1;
    if (FindEntityByName("Bguards", &position, 0, 0) != 0) {
        group = SpawnMonsters(0xc, 6, &position, 1, 1, 0, 0);
        if (group != 0) {
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x215, ARNIKA_CPP, group->leader_id_9f, 1));
            info->p3D->SetScript004C7F10("guard.msf", 1);
        }
    }
    return 1;
}

/* Vaultalarmdoor trigger: raises the "nothing happened" flag, plays the
   vault alarm, turns every guard group hostile and records fact 0xe0. */
// FUNCTION: WIZ8 0x004E1120
bool ArnikaVaultAlarmDoor004E1120(Trigger* pTrigger)
{
    W8MonsterGroup* group;

    g_trigger_feedback_00606994 = 1;
    SoundPlay("Data\\Sound\\Ambients\\VaultAlarm.wav", 0);
    group = FindNextExistingMonsterByID(0xc, 0);
    while (group != 0) {
        SetMonsterGroupHostility(group, 1, 0);
        group = FindNextExistingMonsterByID(0xc, group);
    }
    SetFact(0xe0, 1, 0);
    return 1;
}

/* Exitbutton trigger: caches the exit-door trigger and prop, toggles the
   Teleporting location variable, arms the teleport watch when a teleport
   starts and records facts 0xcd and 0xe0. */
// FUNCTION: WIZ8 0x004E1180
bool ArnikaExitButton004E1180(Trigger* pTrigger)
{
    g_exit_door_trigger_6835d4 = pTrigger;
    if (pTrigger->m_bRepType != 2) {
        srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                     0x3ed, 0);
    }
    g_exit_door_prop_6835d0 = pTrigger->m_pProp;
    if (GetLocationVarIDByName("Teleporting") == -1) {
        CreateLocationVar("Teleporting", 1);
        g_master_functions_006834d8->Add(ArnikaTeleportWatch004E1300);
    } else if (GetLocationVarValueByName("Teleporting") == 0) {
        SetTriggerVariableByName00444030("Teleporting", 1);
        g_master_functions_006834d8->Add(ArnikaTeleportWatch004E1300);
    } else {
        SetTriggerVariableByName00444030("Teleporting", 0);
    }
    g_trigger_feedback_00606994 = 1;
    SetFact(0xcd, 1, 0);
    SetFact(0xe0, 1, 0);
    return 1;
}

/* Teleport watch: once the exit-door prop's rep finishes animating, runs
   the cached trigger and teleports the party to ARN11. */
// FUNCTION: WIZ8 0x004E1300
void ArnikaTeleportWatch004E1300(int command)
{
    g_flag_006834dc = 0;
    if (g_exit_door_prop_6835d0->Rep()->animation_playing_06d != 0) {
        return;
    }
    g_flag_006834dc = 1;
    g_exit_door_trigger_6835d4->Run(-1);
    g_exit_door_trigger_6835d4->RunDestination00440DD0("ARN11");
}

/* GenVault-2-door trigger: spawns the vault golem once at the Golem
   entity. */
// FUNCTION: WIZ8 0x004E1340
bool ArnikaGenVaultDoor004E1340(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("Golem", &position, 0, 0) != 0) {
        if (GetLocationVarIDByName("GolemSpawned") != -1) {
            return 0;
        }
        CreateLocationVar("GolemSpawned", 1);
        SpawnMonsters(0x31, 1, &position, 1, 1, 0, 0);
        SoundPlay("Data\\Sound\\Ambients\\Temp Transporting.wav", 0);
    }
    return 1;
}

/* Elevator-1 setup: resolves the seven triggers, restores El01State,
   El01Moving and RedButtonDown from the location variables, re-arms the
   moving master against the door or lift prop that was mid-flight and
   re-arms the button master when the red button was down. */
// FUNCTION: WIZ8 0x004E13B0
void ArnikaElevator1Setup004E13B0(void)
{
    gEl01.pElevator = FindTriggerByName("Elevator-1");
    if (gEl01.pElevator == 0) {
        srAssertFail("gEl01.pElevator", ARNIKA_CPP, 0x2f0,
                     "Missing trigger 'Elevator-1'! It's not in the LVL file!");
    }
    gEl01.pLift = FindTriggerByName("El1-Lift");
    if (gEl01.pLift == 0) {
        srAssertFail("gEl01.pLift", ARNIKA_CPP, 0x2f2,
                     "Missing trigger 'El1-Lift'! It's not in the LVL file!");
    }
    gEl01.pTopDoor = FindTriggerByName("El1-TopGate");
    if (gEl01.pTopDoor == 0) {
        srAssertFail("gEl01.pTopDoor", ARNIKA_CPP, 0x2f4,
                     "Missing trigger 'El1-TopGate'! It's not in the LVL file!");
    }
    gEl01.pBottomDoor = FindTriggerByName("El1-BottomGate");
    if (gEl01.pBottomDoor == 0) {
        srAssertFail("gEl01.pBottomDoor", ARNIKA_CPP, 0x2f6,
                     "Missing trigger 'El1-BottomGate'! It's not in the LVL file!");
    }
    gEl01.pTopDoorCollide = FindTriggerByName("El1-TopGateC");
    if (gEl01.pTopDoorCollide == 0) {
        srAssertFail("gEl01.pTopDoorCollide", ARNIKA_CPP, 0x2f8,
                     "Missing trigger 'El1-TopGateC'! It's not in the LVL file!");
    }
    gEl01.pBottomDoorCollide = FindTriggerByName("El1-BottomGateC");
    if (gEl01.pBottomDoorCollide == 0) {
        srAssertFail("gEl01.pBottomDoorCollide", ARNIKA_CPP, 0x2fa,
                     "Missing trigger 'El1-BottomGateC'! It's not in the LVL file!");
    }
    gEl01.pButton = FindTriggerByName("RedButton");
    if (gEl01.pButton == 0) {
        srAssertFail("gEl01.pButton", ARNIKA_CPP, 0x2fc,
                     "Missing trigger 'RedButton'! It's not in the LVL file!");
    }
    gEl01.state = 1;
    gEl01.moving = 0;
    gEl01.button_down = 0;
    if (GetLocationVarIDByName("El01State") == -1) {
        CreateLocationVar("El01State", gEl01.state);
    } else {
        gEl01.state = GetLocationVarValueByName("El01State");
    }
    if (GetLocationVarIDByName("El01Moving") == -1) {
        CreateLocationVar("El01Moving", gEl01.moving);
    } else {
        gEl01.moving = GetLocationVarValueByName("El01Moving");
    }
    SetTriggerVariableByName00444030("El01Moving", 0);
    if (GetLocationVarIDByName("RedButtonDown") == -1) {
        CreateLocationVar("RedButtonDown", gEl01.button_down);
    } else {
        gEl01.button_down = GetLocationVarValueByName("RedButtonDown");
    }
    if (gEl01.moving != 0) {
        switch (gEl01.state) {
        case 1:
        case 5:
            if (gEl01.pLift->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl01.pProp = gEl01.pLift->m_pProp;
            break;
        case 2:
        case 4:
            if (gEl01.pTopDoor->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl01.pProp = gEl01.pTopDoor->m_pProp;
            break;
        case 6:
        case 8:
            if (gEl01.pBottomDoor->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl01.pProp = gEl01.pBottomDoor->m_pProp;
            break;
        }
        g_master_functions_006834d8->Add(ArnikaEl1Moving004E19B0);
    }
    if (gEl01.button_down != 0) {
        ArnikaEl1Button004E17C0(gEl01.button_down);
    }
}

/* "RedButton": while the button press is armed it stamps the pressed fact,
   then either starts the lift (states 1/7) or just re-arms the button master
   and always reports handled. */
// FUNCTION: WIZ8 0x004E1740
bool ArnikaRedButton004E1740(Trigger* pTrigger)
{
    if (g_red_button_armed_613dcc == 0) {
        return false;
    }
    if (gEl01.button_down == 2) {
        return true;
    }
    if (gEl01.button_down != 0) {
        return false;
    }
    g_red_button_armed_613dcc = 0;
    SetFact(0xc7, 1, 0);
    g_red_button_armed_613dcc = 1;
    g_trigger_feedback_00606994 = 1;
    if (gEl01.state != 3) {
        if (gEl01.button_down == 2) {
            return false;
        }
        if (gEl01.state != 1 && gEl01.state != 7) {
            return false;
        }
        ArnikaElevatorAdvance004E2010(1);
    }
    ArnikaEl1Button004E17C0(static_cast<int>(0xEFFFFFFF));
    return true;
}

/* The elevator-1 button master function. A nonzero command re-arms it: -1
   persists the button state, 0xEFFFFFFF presses the button, and any other
   value is the pending press. The command-0 run waits for the button prop's
   rep to finish, then runs the button trigger once and clears the press. */
// FUNCTION: WIZ8 0x004E17C0
void ArnikaEl1Button004E17C0(int command)
{
    if (command != 0) {
        if (command == -1) {
            SetTriggerVariableByName00444030("RedButtonDown", gEl01.button_down);
            return;
        }
        if (gEl01.pButton->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        g_el01_button_prop_6835ec = gEl01.pButton->m_pProp;
        if (g_el01_button_prop_6835ec == 0) {
            return;
        }
        g_master_functions_006834d8->Add(ArnikaEl1Button004E17C0);
        if (command != static_cast<int>(0xEFFFFFFF)) {
            gEl01.button_down = command;
        } else {
            gEl01.button_down = 1;
        }
        return;
    }
    g_flag_006834dc = 0;
    if (g_el01_button_prop_6835ec == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (g_el01_button_prop_6835ec->Rep()->animation_playing_06d != 0) {
        return;
    }
    if (gEl01.button_down == 1) {
        gEl01.button_down = 2;
        gEl01.pButton->Run(-1);
        gEl01.button_down = 3;
        return;
    }
    gEl01.button_down = 0;
    SetTriggerVariableByName00444030("RedButtonDown", 0);
    g_flag_006834dc = 1;
}

/* "El1-TopButtons": advances the lift while no button run is pending and the
   elevator is in a callable state; always reports handled. */
// FUNCTION: WIZ8 0x004E1930
bool ArnikaEl1TopButtons004E1930(Trigger* pTrigger)
{
    g_trigger_feedback_00606994 = 1;
    if (gEl01.button_down != 2 && (gEl01.state == 1 || gEl01.state == 3 || gEl01.state == 7)) {
        ArnikaElevatorAdvance004E2010(1);
    }
    return true;
}

/* "El1-BottomButtons": same advance as the top call buttons. */
// FUNCTION: WIZ8 0x004E1970
bool ArnikaEl1BottomButtons004E1970(Trigger* pTrigger)
{
    g_trigger_feedback_00606994 = 1;
    if (gEl01.button_down != 2 && (gEl01.state == 1 || gEl01.state == 3 || gEl01.state == 7)) {
        ArnikaElevatorAdvance004E2010(1);
    }
    return true;
}

/* The El01Moving master function: -1 stamps the location variable, and the
   command-0 run advances the elevator once the watched door or lift prop's
   rep has finished playing. */
// FUNCTION: WIZ8 0x004E19B0
void ArnikaEl1Moving004E19B0(int command)
{
    if (command != 0) {
        if (command == -1) {
            SetTriggerVariableByName00444030("El01Moving", 1);
            return;
        }
    }
    g_flag_006834dc = 0;
    if (gEl01.pProp == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (gEl01.pProp->Rep()->animation_playing_06d != 0) {
        return;
    }
    g_flag_006834dc = 1;
    ArnikaElevatorAdvance004E2010(1);
}

/* Elevator-02 setup: resolves its triggers, restores El02State, El02Moving
   and GreenButtonDown, opens the bottom doors on a fresh level and re-arms
   the moving and button masters. */
// FUNCTION: WIZ8 0x004E1A10
void ArnikaElevator2Setup004E1A10(void)
{
    gEl02.pElevator = FindTriggerByName("Elevator-02");
    if (gEl02.pElevator == 0) {
        srAssertFail("gEl02.pElevator", ARNIKA_CPP, 0x402,
                     "Missing trigger 'Elevator-02'! It's not in the LVL file!");
    }
    gEl02.pLift = FindTriggerByName("El-02Lift");
    if (gEl02.pLift == 0) {
        srAssertFail("gEl02.pLift", ARNIKA_CPP, 0x404,
                     "Missing trigger 'El-02Lift'! It's not in the LVL file!");
    }
    gEl02.pTopDoor = FindTriggerByName("El-02TopDoor");
    if (gEl02.pTopDoor == 0) {
        srAssertFail("gEl02.pTopDoor", ARNIKA_CPP, 0x406,
                     "Missing trigger 'El-02TopDoor'! It's not in the LVL file!");
    }
    gEl02.pBottomDoor = FindTriggerByName("El-02BottomDoor");
    if (gEl02.pBottomDoor == 0) {
        srAssertFail("gEl02.pBottomDoor", ARNIKA_CPP, 0x408,
                     "Missing trigger 'El-02BottomDoor'! It's not in the LVL file!");
    }
    gEl02.pTopDoorCollide = FindTriggerByName("El-02TopDoorCollide");
    if (gEl02.pTopDoorCollide == 0) {
        srAssertFail("gEl02.pTopDoorCollide", ARNIKA_CPP, 0x40a,
                     "Missing trigger 'El-02TopDoorCollide'! It's not in the LVL file!");
    }
    gEl02.pBottomDoorCollide = FindTriggerByName("El-02BottomDoorCollide");
    if (gEl02.pBottomDoorCollide == 0) {
        srAssertFail("gEl02.pBottomDoorCollide", ARNIKA_CPP, 0x40c,
                     "Missing trigger 'El-02BottomDoorCollide'! It's not in the LVL file!");
    }
    gEl02.pButton = FindTriggerByName("GreenButton");
    if (gEl02.pButton == 0) {
        srAssertFail("gEl02.pButton", ARNIKA_CPP, 0x40e,
                     "Missing trigger 'GreenButton'! It's not in the LVL file!");
    }
    gEl02.pProp = 0;
    gEl02.state = 7;
    gEl02.moving = 0;
    gEl02.button_down = 0;
    if (GetLocationVarIDByName("El02State") == -1) {
        CreateLocationVar("El02State", gEl02.state);
        gEl02.pBottomDoor->Run(-1);
        gEl02.pBottomDoorCollide->Run(-1);
    } else {
        gEl02.state = GetLocationVarValueByName("El02State");
    }
    if (GetLocationVarIDByName("El02Moving") == -1) {
        CreateLocationVar("El02Moving", gEl02.moving);
    } else {
        gEl02.moving = GetLocationVarValueByName("El02Moving");
    }
    SetTriggerVariableByName00444030("El02Moving", 0);
    if (GetLocationVarIDByName("GreenButtonDown") == -1) {
        CreateLocationVar("GreenButtonDown", gEl02.button_down);
    } else {
        gEl02.button_down = GetLocationVarValueByName("GreenButtonDown");
    }
    if (gEl02.moving != 0) {
        switch (gEl02.state) {
        case 1:
        case 5:
            if (gEl02.pLift->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl02.pProp = gEl02.pLift->m_pProp;
            break;
        case 2:
        case 4:
            if (gEl02.pTopDoor->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl02.pProp = gEl02.pTopDoor->m_pProp;
            break;
        case 6:
        case 8:
            if (gEl02.pBottomDoor->m_bRepType != 2) {
                srAssertFail("m_bRepType == TRIGGER_REP_PROP",
                             "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
            }
            gEl02.pProp = gEl02.pBottomDoor->m_pProp;
            break;
        }
        g_master_functions_006834d8->Add(ArnikaEl2Moving004E1FB0);
    }
    if (gEl02.button_down != 0) {
        ArnikaEl2Button004E1E10(gEl02.button_down);
    }
}

/* "GreenButton": while no press is pending it plays the acknowledgement,
   runs the elevator unless it is already at the top, and re-arms the button
   master. */
// FUNCTION: WIZ8 0x004E1DC0
bool ArnikaGreenButton004E1DC0(Trigger* pTrigger)
{
    if (gEl02.button_down == 2) {
        return true;
    }
    if (gEl02.button_down != 0) {
        return false;
    }
    SetFactionDispositionBand(0xa, W8_FACTION_HOSTILE);
    g_trigger_feedback_00606994 = 1;
    if (gEl02.state != 3) {
        gEl02.pElevator->Run(-1);
    }
    ArnikaEl2Button004E1E10(static_cast<int>(0xEFFFFFFF));
    return true;
}

/* The elevator-2 button master function, mirroring ArnikaEl1Button004E17C0
   against GreenButtonDown. */
// FUNCTION: WIZ8 0x004E1E10
void ArnikaEl2Button004E1E10(int command)
{
    if (command != 0) {
        if (command == -1) {
            SetTriggerVariableByName00444030("GreenButtonDown", gEl02.button_down);
            return;
        }
        if (gEl02.pButton->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        g_el02_button_prop_6835f0 = gEl02.pButton->m_pProp;
        if (g_el02_button_prop_6835f0 == 0) {
            return;
        }
        g_master_functions_006834d8->Add(ArnikaEl2Button004E1E10);
        if (command != static_cast<int>(0xEFFFFFFF)) {
            gEl02.button_down = command;
        } else {
            gEl02.button_down = 1;
        }
        return;
    }
    g_flag_006834dc = 0;
    if (g_el02_button_prop_6835f0 == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (g_el02_button_prop_6835f0->Rep()->animation_playing_06d != 0) {
        return;
    }
    if (gEl02.button_down == 1) {
        gEl02.button_down = 2;
        gEl02.pButton->Run(-1);
        gEl02.button_down = 3;
        return;
    }
    gEl02.button_down = 0;
    SetTriggerVariableByName00444030("GreenButtonDown", 0);
    g_flag_006834dc = 1;
}

/* "Elevator-02": steps the lift while no button run is pending and the
   elevator is in a callable state; answers false otherwise. */
// FUNCTION: WIZ8 0x004E1F80
bool ArnikaElevator02Trigger004E1F80(Trigger* pTrigger)
{
    if (gEl02.button_down == 2) {
        return false;
    }
    if (gEl02.state != 1 && gEl02.state != 3 && gEl02.state != 7) {
        return false;
    }
    ArnikaElevatorAdvance004E2010(2);
    return true;
}

/* The El02Moving master function, mirroring ArnikaEl1Moving004E19B0. */
// FUNCTION: WIZ8 0x004E1FB0
void ArnikaEl2Moving004E1FB0(int command)
{
    if (command != 0) {
        if (command == -1) {
            SetTriggerVariableByName00444030("El02Moving", 1);
            return;
        }
    }
    g_flag_006834dc = 0;
    if (gEl02.pProp == 0) {
        g_flag_006834dc = 1;
        return;
    }
    if (gEl02.pProp->Rep()->animation_playing_06d != 0) {
        return;
    }
    g_flag_006834dc = 1;
    ArnikaElevatorAdvance004E2010(2);
}

/* Shared elevator step: advances the selected elevator one state, running
   the door/lift triggers for the departing state and caching the rep the
   moving master will wait on. States 3 and 7 are the door-closed pauses that
   produce no prop. */
// FUNCTION: WIZ8 0x004E2010
void ArnikaElevatorAdvance004E2010(int which)
{
    W8Elevator* el;
    W8Prop* prop;

    if (which == 1) {
        el = &gEl01;
    } else {
        el = &gEl02;
    }
    /* Invalid state leaves `prop` unset; retail reused the `which` register
       for the out-prop slot, which is not authored source. */
    switch (el->state) {
    case 1:
        if (el->pTopDoor->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pTopDoor->m_pProp;
        el->pTopDoor->Run(-1);
        el->pTopDoorCollide->Run(-1);
        el->state = 2;
        break;
    case 2:
        prop = 0;
        el->state = 3;
        break;
    case 3:
        if (el->pTopDoor->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pTopDoor->m_pProp;
        el->pTopDoor->Run(-1);
        el->pTopDoorCollide->Run(-1);
        el->state = 4;
        break;
    case 4:
        if (el->pLift->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pLift->m_pProp;
        el->pLift->Run(-1);
        el->state = 5;
        break;
    case 5:
        if (el->pBottomDoor->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pBottomDoor->m_pProp;
        el->pBottomDoor->Run(-1);
        el->pBottomDoorCollide->Run(-1);
        el->state = 6;
        break;
    case 6:
        prop = 0;
        el->state = 7;
        break;
    case 7:
        if (el->pBottomDoor->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pBottomDoor->m_pProp;
        el->pBottomDoor->Run(-1);
        el->pBottomDoorCollide->Run(-1);
        el->state = 8;
        break;
    case 8:
        if (el->pLift->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        prop = el->pLift->m_pProp;
        el->pLift->Run(-1);
        el->state = 1;
        break;
    }
    if (which == 1) {
        SetTriggerVariableByName00444030("El01State", el->state);
        gEl01.pProp = prop;
        if (prop != 0) {
            g_master_functions_006834d8->Add(ArnikaEl1Moving004E19B0);
        }
    } else {
        SetTriggerVariableByName00444030("El02State", el->state);
        gEl02.pProp = prop;
        if (prop != 0) {
            g_master_functions_006834d8->Add(ArnikaEl2Moving004E1FB0);
        }
    }
}

/* "ChaosMolori": rejects the activation when an item is on the cursor and
   the shared callback flag is clear; accepts when the cursor is empty or the
   flag is raised. */
// FUNCTION: WIZ8 0x004E2340
bool ArnikaChaosMolori004E2340(Trigger* pTrigger)
{
    if (g_flag_006834dd == 0 && g_status_685170.item_in_cursor != 0) {
        return false;
    }
    return true;
}

/* "Maddmook": the first time the party has been warned, posts the mook's
   script notice and drops the faction band. */
// FUNCTION: WIZ8 0x004E2360
bool ArnikaMaddmook004E2360(Trigger* pTrigger)
{
    if (GetLocationVarIDByName("WarnedAboutEntry") != -1 &&
        GetLocationVarValueByName("WarnedAboutEntry") == 1 &&
        GetFactionDisposition(W8_FACTION_MOOK) != W8_DISPOSITION_NEUTRAL) {
        W8NpcState* npc = GetNpcStateByKind(0x69);
        if (npc != 0) {
            QueueNpcScriptNotice(npc, 0, 7, 0, 0);
            SetFactionDispositionBand(W8_FACTION_MOOK, W8_FACTION_HOSTILE);
        }
    }
    return true;
}

/* "AstralDominae": same cursor/flag gate as ChaosMolori, then forwards the
   activation to the CMBox trigger. */
// FUNCTION: WIZ8 0x004E23C0
bool ArnikaAstralDominae004E23C0(Trigger* pTrigger)
{
    if (g_flag_006834dd == 0 && g_status_685170.item_in_cursor != 0) {
        return false;
    }
    pTrigger = FindTriggerByName("CMBox");
    if (pTrigger == 0) {
        srAssertFail("pTrigger", ARNIKA_CPP, 0x5bd,
                     "Missing trigger 'CMBox'! It's not in the LVL file!");
    }
    pTrigger->Run(-1);
    return true;
}

/* "CMbox": places the held relic on the pedestal through
   ArnikaPedestalItem004E24E0, then runs the matching trigger inside the
   shared callback flag so the pedestal callbacks stay quiet. */
// FUNCTION: WIZ8 0x004E2420
bool ArnikaCMbox004E2420(Trigger* pTrigger)
{
    int previous = -1;
    int item;

    if (g_flag_006834dd != 0) {
        return false;
    }
    item = ArnikaPedestalItem004E24E0(&previous);
    if (item < -1) {
        return false;
    }
    if (item == 0x244) {
        pTrigger = FindTriggerByName("ChaosMolori");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", ARNIKA_CPP, 0x5e8,
                         "Missing trigger 'ChaosMolori'! It's not in the LVL file!");
        }
    } else if (item == 0x242 || item == 0x264) {
        pTrigger = FindTriggerByName("AstralDominae");
        if (pTrigger == 0) {
            srAssertFail("pTrigger", ARNIKA_CPP, 0x5f0,
                         "Missing trigger 'AstralDominae'! It's not in the LVL file!");
        }
    } else {
        return previous != -1;
    }
    g_flag_006834dd = 1;
    pTrigger->Run(-1);
    g_flag_006834dd = 0;
    return true;
}

/* Pedestal item exchange: when a relic (0x242/0x244/0x264) is on the cursor
   it is consumed into the PedestalItem location variable and the stored item
   comes back to the cursor; on the first exchange it also aims the camera at
   the mook and posts the entry notice. Answers the item placed, -1 when the
   pedestal already held one, or -2 when the cursor item was not a relic. */
// FUNCTION: WIZ8 0x004E24E0
int ArnikaPedestalItem004E24E0(int* previous_item)
{
    Trigger* pTrigger;
    W8NpcState* npc;
    W8MonsterInfo* info;
    srVector3T<float> position;
    int item = -1;
    int previous;

    if (g_status_685170.item_in_cursor != 0) {
        item = GetItemInHand();
        if (item != 0x244 && item != 0x242 && item != 0x264) {
            return -2;
        }
    }
    pTrigger = FindTriggerByName("ChaosMolori");
    if (pTrigger == 0) {
        srAssertFail("pTrigger", ARNIKA_CPP, 0x607,
                     "Missing trigger 'ChaosMolori'! It's not in the LVL file!");
    }
    if (item > -1) {
        ClearHeldItemDisplay();
    }
    if (GetLocationVarIDByName("PedestalItem") != -1 &&
        GetLocationVarIDByName("WarnedAboutEntry") != -1 &&
        GetLocationVarValueByName("WarnedAboutEntry") == 1) {
        previous = GetLocationVarValueByName("PedestalItem");
        if (item == -1) {
            return previous | item;
        }
        SetTriggerVariableByName00444030("PedestalItem", item);
        if (GetLocationVarIDByName("WarnedAboutEntry") != -1) {
            SetTriggerVariableByName00444030("WarnedAboutEntry", 0);
        }
        *previous_item = previous;
        return item;
    }
    if (GetLocationVarIDByName("PedestalItem") == -1) {
        CreateLocationVar("PedestalItem", -1);
        previous = 0x244;
        if (item == -1) {
            ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, previous, 0, 0, 0);
        }
    } else {
        previous = GetLocationVarValueByName("PedestalItem");
        if (previous == -1) {
            if (item == previous) {
                return -2;
            }
        } else if (item == -1) {
            ReplaceOrCreateItem(&g_status_685170.item_in_hand_235b, previous, 0, 0, 0);
        }
    }
    SetItemCursor(0);
    npc = GetNpcStateByKind(0x69);
    if (npc != 0) {
        info = GetNpcMonsterInfo(npc);
        if (info != 0) {
            position = info->p3D->movement_0c0.position_040;
            position.y += info->p3D->movement_0c0.height_offset_0b8;
            g_gd_camera_65a0f8->LookAt(&position, 0);
        }
        QueueNpcScriptNotice(npc, 0, 8, 0, 0);
    }
    if (GetLocationVarIDByName("WarnedAboutEntry") == -1) {
        CreateLocationVar("WarnedAboutEntry", 1);
    } else {
        SetTriggerVariableByName00444030("WarnedAboutEntry", 1);
    }
    *previous_item = previous;
    return item;
}

/* The "BallSlot" activation callback: on the first ball insert it creates
   item 0x240 and hands it to the kind-0x16 NPC's script notice queue. */
// FUNCTION: WIZ8 0x004E26F0
bool ArnikaBallSlot004E26F0(Trigger* pTrigger)
{
    W8ItemInstance item;
    W8NpcState* npc;

    if (GetLocationVarIDByName("BallInserted") == -1) {
        CreateLocationVar("BallInserted", 1);
        npc = GetNpcStateByKind(0x16);
        ReplaceOrCreateItem(&item, 0x240, 1, 1, 0);
        QueueNpcScriptNotice(npc, &item, -1, 0, 0);
    }
    g_trigger_feedback_00606994 = 1;
    return true;
}

/* The "Flightrecordertrigger" activation callback: outside NPC dialogue it
   queues the kind-0x14 NPC's script notice, passing the held item when the
   cursor holds one. */
// FUNCTION: WIZ8 0x004E2760
bool ArnikaFlightRecorder004E2760(Trigger* pTrigger)
{
    W8ItemInstance* item;
    W8NpcState* npc;

    if (gXStatus.fNpcDialogueMode != 0) {
        return false;
    }
    g_trigger_feedback_00606994 = 1;
    npc = GetNpcStateByKind(0x14);
    item = 0;
    if (g_status_685170.item_in_cursor != 0) {
        item = &g_status_685170.item_in_hand_235b;
    }
    QueueNpcScriptNotice(npc, item, -1, 0, 0);
    return false;
}
