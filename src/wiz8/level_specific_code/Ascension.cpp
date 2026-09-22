#include "wiz8/level_specific_code/Ascension.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/IntervalGate.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/fact_state.h"
#include "wiz8/float_constants.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/location_variables.h"
#include "wiz8/message_box.h"
#include "wiz8/xstatus.h"
#include "soundman.h"
#include "surrender/srCamera.h"

/* Level Specific Code\Ascension.cpp.

   Attribution evidence: 0x004DF870, 0x004DFBC0 and 0x004E0430 cite this file's
   path string in their MonsterGetIndexByLocationID assertions (lines 0x17f,
   0x2cd, 0x2da, 0x2e7). 0x004DF810, 0x004DFB40, 0x004DFB80 and 0x004E0510 are
   bounded to this TU, and InitializeLevelMasterFunctions004D6C50 registers the
   0x004DFE60-0x004E04F0 callbacks plus the 0x004DF870 init call under case 1,
   the Ascension Peak level. 0x004E0560 and 0x004E05A0 are registered under
   case 0x24, the Footsteps level, which is part of the Ascension Peak block. */

#define ASCENSION_CPP "C:\\Projects\\Wizardry 8\\Level Specific Code\\Ascension.cpp"

// GLOBAL: WIZ8 0x0068356C
W8IntervalGate* g_avalanche_gate_68356c;

/* Count how many of the three Ascension Peak relic items (0x242, 0x243,
   0x244) are on the party; the world-cursor "seen bodies" handler requires
   all three. */
// FUNCTION: WIZ8 0x004DF810
int CountAscensionPeakItems004DF810(void)
{
    int count = 0;

    if (FindItemOnParty(0x242, 0, 0, 2, 0)) {
        count = 1;
    }
    if (FindItemOnParty(0x243, 0, 0, 2, 0)) {
        ++count;
    }
    if (FindItemOnParty(0x244, 0, 0, 2, 0)) {
        ++count;
    }
    return count;
}

/* Ascension Peak level init (level-1 master-function case): the relic-item
   count selects the staged encounters. Two or more relics run the avalanche
   sequence and the Rapax ambush once; all three additionally spawn the Savants
   and fire the "Bodies" trigger once. */
// FUNCTION: WIZ8 0x004DF870
unsigned char AscensionPeakInit004DF870(void)
{
    srVector3T<float> position;
    int count = CountAscensionPeakItems004DF810();

    if (count > 1) {
        if (GetLocationVarIDByName("AP_AtLeast2of3Items") == -1) {
            W8MonsterGroup* group = 0;
            W8MonsterInfo* info;

            AscensionAvalanche004DFBC0(1);
            if (FindEntityByName("NP_Rapax01", &position, 0, 0)) {
                group = SpawnMonsters(0xb1, 6, &position, 1, 1, 0, 0);
            }
            if (FindEntityByName("NP_Rapax04", &position, 0, 0)) {
                SpawnMonsters(0xf, 4, &position, 1, 1, 0, 0);
            }
            info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x17f, ASCENSION_CPP, group->value_9f, 1));
            info->monster->SetScript004C7F10("proximitylandslide.msf", 1);
            SpawnAscensionAmbush004DFEA0();
            CreateLocationVar("AP_AtLeast2of3Items", 1);
        }
        if (GetLocationVarIDByName("AP_SpawnDaughter") == -1) {
            W8NpcState* npc = GetNpcStateByKind(0x31);

            if (npc != 0 && npc->greeting_pending != 0 && npc->unknown_04 == 0 &&
                npc->unknown_2d == 0 && GetFact(0x15f) != 0) {
                if (FindEntityByName("NP_Daughter", &position, 0, 0)) {
                    SpawnMonsters(0x18d, 1, &position, 0, 1, 0, 0);
                    SetFact(0x327, 1, 0);
                }
                CreateLocationVar("AP_SpawnDaughter", 1);
            }
        }
    }
    if (count == 3) {
        if (GetLocationVarIDByName("AP_All3Items") == -1) {
            if (FindEntityByName("NP_Savants01", &position, 0, 0)) {
                SpawnMonsters(0x1a0, 6, &position, 1, 1, 0, 0);
            }
            if (GetFact(0x5c) != 0 || GetFact(0x97) != 0) {
                Trigger* bodies = FindTriggerByName("Bodies");

                if (bodies != 0) {
                    bodies->Run(-1);
                }
            }
            CreateLocationVar("AP_All3Items", 1);
        }
    }
    return 1;
}

/* Spawn the Alfie "chaos" monster (0xae) at the NP_AlfieChaos entity once the
   party carries the Astral Dominae (0x244). */
// FUNCTION: WIZ8 0x004DFAE0
unsigned char SpawnAlfieChaos004DFAE0(int unused)
{
    srVector3T<float> position;

    if (FindItemOnParty(0x244, 0, 0, 2, 0) == 0) {
        return 0;
    }
    if (FindEntityByName("NP_AlfieChaos", &position, 0, 0)) {
        SpawnMonsters(0xae, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* Spawn the Alfie "life" monster (0xaf) at the NP_AlfieLife entity when it is
   present in the level. */
// FUNCTION: WIZ8 0x004DFB40
unsigned char SpawnAlfieLife004DFB40(int unused)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_AlfieLife", &position, 0, 0) != 0) {
        SpawnMonsters(0xaf, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* Spawn the Alfie "know" monster (0xb0) at the NP_AlfieKnow entity when it is
   present in the level. */
// FUNCTION: WIZ8 0x004DFB80
unsigned char SpawnAlfieKnow004DFB80(int unused)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_AlfieKnow", &position, 0, 0) != 0) {
        SpawnMonsters(0xb0, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* Fire the "Avalanche" trigger and, when asked, update the 0x15d/0x15e
   proximity facts by measuring the camera against the ASC40 and ASC30
   positions, then arm the periodic land-shaker master. */
// FUNCTION: WIZ8 0x004DFBC0
void AscensionAvalanche004DFBC0(unsigned char command)
{
    Trigger* avalanche = FindTriggerByName("Avalanche");

    if (avalanche != 0) {
        avalanche->Run(-1);
    }
    if (command != 0) {
        srVector3T<float> position;
        srVector3T<float> trigger_position;

        position = GetWorld()->camera->getLocation();
        FindTriggerByName("ASC40")->GetPosition(&trigger_position);
        if ((position - trigger_position).Length() < g_double_005ec030) {
            SetFact(0x15d, 1, 0);
            SetFact(0x15e, 0, 0);
        } else {
            FindTriggerByName("ASC30")->GetPosition(&trigger_position);
            if ((position - trigger_position).Length() < g_double_005ec030) {
                SetFact(0x15d, 0, 0);
                SetFact(0x15e, 1, 0);
            }
        }
        g_master_functions_006834d8->Add(AscensionLandShaker004DFD70);
    }
}

/* The land-shaker master function, registered by AscensionAvalanche. The
   lazily created one-second gate runs the LandShaker trigger once it elapses;
   -1 returns early without touching the scripted-sequence flag. */
// FUNCTION: WIZ8 0x004DFD70
void AscensionLandShaker004DFD70(int command)
{
    Trigger* shaker;

    if (command == -1) {
        return;
    }
    g_flag_006834dc = 0;
    if (g_avalanche_gate_68356c == 0) {
        g_avalanche_gate_68356c = new W8IntervalGate(1.0f, 0, 1);
        return;
    }
    if (g_avalanche_gate_68356c->IsFinished() == 0) {
        g_avalanche_gate_68356c->PollElapsedIntervals();
        if (g_avalanche_gate_68356c->IsFinished() == 0) {
            return;
        }
    }
    delete g_avalanche_gate_68356c;
    g_avalanche_gate_68356c = 0;
    shaker = FindTriggerByName("LandShaker");
    if (shaker != 0) {
        shaker->Run(-1);
    }
    g_flag_006834dc = 1;
}

/* The RampUp activation callback: spawn the daughter monster (0x18d) at the
   NP_DarkSavant entity. */
// FUNCTION: WIZ8 0x004DFE60
bool AscensionRampUp004DFE60(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_DarkSavant", &position, 0, 0)) {
        SpawnMonsters(0x18d, 1, &position, 0, 1, 0, 0);
    }
    return true;
}

/* Spawn the Rapax ambush and the prince once two relics are brought to the
   peak. */
// FUNCTION: WIZ8 0x004DFEA0
unsigned char SpawnAscensionAmbush004DFEA0(void)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_Rapax02", &position, 0, 0)) {
        SpawnMonsters(0xc3, 3, &position, 0, 1, 0, 0);
    }
    if (FindEntityByName("NP_Rapax03", &position, 0, 0)) {
        SpawnMonsters(0xc4, 6, &position, 0, 1, 0, 0);
    }
    if (FindEntityByName("NP_Rapax05", &position, 0, 0)) {
        SpawnMonsters(0xc5, 6, &position, 0, 1, 0, 0);
    }
    if (FindEntityByName("NP_Prince", &position, 0, 0)) {
        SpawnMonsters(0xb2, 1, &position, 0, 1, 0, 0);
    }
    return 1;
}

/* The ChaosBTrigger activation callback: arm ChaosATrigger, and once
   AlethidiesChaosActive is set dismiss the bound Aletheides monster and clear
   the variable. */
// FUNCTION: WIZ8 0x004DFF90
bool AscensionChaosBTrigger004DFF90(Trigger* pTrigger)
{
    Trigger* chaos_a = FindTriggerByName("ChaosATrigger");

    if (chaos_a != 0) {
        chaos_a->flags_0a0 |= W8_TRIGGER_ENABLED;
    }
    if (GetLocationVarIDByName("AlethidiesChaosActive") != -1 &&
        GetLocationVarValueByName("AlethidiesChaosActive") != 0) {
        RemoveAletheides();
        SetTriggerVariableByName00444030("AlethidiesChaosActive", 0);
    }
    return true;
}

/* The ChaosATrigger activation callback: while AlethidiesChaosActive is set
   the trigger is inert. Otherwise Aletheides answers, and the party carrying
   the Astral Dominae (0x244) gets the relic exchange and the chaos Alfie
   spawn. */
// FUNCTION: WIZ8 0x004DFFF0
bool AscensionChaosATrigger004DFFF0(Trigger* pTrigger)
{
    if (GetLocationVarIDByName("AlethidiesChaosActive") == -1) {
        CreateLocationVar("AlethidiesChaosActive", 0);
    }
    if (GetLocationVarValueByName("AlethidiesChaosActive") != 0) {
        return false;
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 1);
    QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71e);
    if (FindItemOnParty(0x244, 0, 0, 2, 0) == 0) {
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71f);
    } else {
        SetTriggerVariableByName00444030("AlethidiesChaosActive", 1);
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x720);
        QueueNpcMessageLine(W8_NPC_MSG_CALL_4DFAE0, 0);
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 0);
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    return true;
}

/* The LifeBTrigger activation callback: arm LifeATrigger, and once
   AlethidiesLifeActive is set dismiss the bound Aletheides monster and clear
   the variable. */
// FUNCTION: WIZ8 0x004E00B0
bool AscensionLifeBTrigger004E00B0(Trigger* pTrigger)
{
    Trigger* life_a = FindTriggerByName("LifeATrigger");

    if (life_a != 0) {
        life_a->flags_0a0 |= W8_TRIGGER_ENABLED;
    }
    if (GetLocationVarIDByName("AlethidiesLifeActive") != -1 &&
        GetLocationVarValueByName("AlethidiesLifeActive") != 0) {
        RemoveAletheides();
        SetTriggerVariableByName00444030("AlethidiesLifeActive", 0);
    }
    return true;
}

/* The LifeATrigger activation callback: while AlethidiesLifeActive is set the
   trigger is inert. Otherwise Aletheides answers, and the party carrying the
   relic (0x242) gets the relic exchange and the life Alfie spawn. */
// FUNCTION: WIZ8 0x004E0110
bool AscensionLifeATrigger004E0110(Trigger* pTrigger)
{
    if (GetLocationVarIDByName("AlethidiesLifeActive") == -1) {
        CreateLocationVar("AlethidiesLifeActive", 0);
    }
    if (GetLocationVarValueByName("AlethidiesLifeActive") != 0) {
        return false;
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 1);
    QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71e);
    if (FindItemOnParty(0x242, 0, 0, 2, 0) == 0) {
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71f);
    } else {
        SetTriggerVariableByName00444030("AlethidiesLifeActive", 1);
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x720);
        QueueNpcMessageLine(W8_NPC_MSG_CALL_4DFB40, 0);
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 0);
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    return true;
}

/* The KnowBTrigger activation callback: arm KnowATrigger, and once
   AlethidiesKnowActive is set dismiss the bound Aletheides monster and clear
   the variable. */
// FUNCTION: WIZ8 0x004E01D0
bool AscensionKnowBTrigger004E01D0(Trigger* pTrigger)
{
    Trigger* know_a = FindTriggerByName("KnowATrigger");

    if (know_a != 0) {
        know_a->flags_0a0 |= W8_TRIGGER_ENABLED;
    }
    if (GetLocationVarIDByName("AlethidiesKnowActive") != -1 &&
        GetLocationVarValueByName("AlethidiesKnowActive") != 0) {
        RemoveAletheides();
        SetTriggerVariableByName00444030("AlethidiesKnowActive", 0);
    }
    return true;
}

/* The KnowATrigger activation callback: while AlethidiesKnowActive is set the
   trigger is inert. Otherwise Aletheides answers, and the party carrying the
   relic (0x243) gets the relic exchange and the know Alfie spawn. */
// FUNCTION: WIZ8 0x004E0230
bool AscensionKnowATrigger004E0230(Trigger* pTrigger)
{
    if (GetLocationVarIDByName("AlethidiesKnowActive") == -1) {
        CreateLocationVar("AlethidiesKnowActive", 0);
    }
    if (GetLocationVarValueByName("AlethidiesKnowActive") != 0) {
        return false;
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 1);
    QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71e);
    if (FindItemOnParty(0x243, 0, 0, 2, 0) == 0) {
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x71f);
    } else {
        SetTriggerVariableByName00444030("AlethidiesKnowActive", 1);
        QueueNpcMessageLine(W8_NPC_MSG_PORTRAIT_STRING, 0x720);
        QueueNpcMessageLine(W8_NPC_MSG_CALL_4DFB80, 0);
    }
    QueueNpcMessageLine(W8_NPC_MSG_FINISH_ACTION, 0);
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
    return true;
}

/* The second RampUp activation callback: spawn the Dark Savant (0xc2) and
   Bela (0x18c) and latch AP_DSSpawned so Path1Camera knows they arrived. */
// FUNCTION: WIZ8 0x004E02F0
bool AscensionDarkSavantSpawn004E02F0(Trigger* pTrigger)
{
    srVector3T<float> position;

    if (FindEntityByName("NP_DSavant", &position, 0, 0)) {
        SpawnMonsters(0xc2, 1, &position, 0, 1, 0, 0);
    }
    if (FindEntityByName("NP_Bela", &position, 0, 0)) {
        SpawnMonsters(0x18c, 1, &position, 0, 1, 0, 0);
    }
    if (GetLocationVarIDByName("AP_DSSpawned") == -1) {
        CreateLocationVar("AP_DSSpawned", 1);
    }
    return true;
}

/* The Path1Camera activation callback: once the Dark Savant is spawned, kill
   every live hostile monster, end combat and hand control to the scripted
   world action. */
// FUNCTION: WIZ8 0x004E0390
bool AscensionPath1Camera004E0390(Trigger* pTrigger)
{
    unsigned int index;

    if (GetLocationVarIDByName("AP_DSSpawned") == -1) {
        return false;
    }
    // c-style-cast-ok: ILLength consumes the W8PList monster list; the same historical C-style spelling appears in Health Stamina Mana.cpp.
    for (index = 0; index < PLLength(gXStatus.plsMonsterList); ++index) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(index);

        if (info->fActive != 0 && info->fInCombat != 0 && info->ubDisposition == DISP_HOSTILE &&
            info->monster->IsDying() == 0) {
            MonsterStartsDying(info, 1);
        }
    }
    if (gXStatus.fCombatMode != 0) {
        EndCombat004EA310(1);
    }
    BeginScriptedWorldAction();
    pTrigger->flags_0a0 &= ~W8_TRIGGER_ON;
    return true;
}

/* Remove the monster bound to whichever Aletheides NPC kind (0x2d, 0x2e or
   0x2f) is present, first match wins. */
// FUNCTION: WIZ8 0x004E0430
void RemoveAletheides(void)
{
    W8NpcState* npc = GetNpcStateByKind(0x2d);
    W8MonsterInfo* info;

    if (npc != 0) {
        info = GetNpcMonsterInfo(npc);
        if (info != 0) {
            RemoveMonster(MonsterGetIndexByLocationID(0x2cd, ASCENSION_CPP, info->location_id, 1),
                          1);
            return;
        }
    }
    npc = GetNpcStateByKind(0x2e);
    if (npc != 0) {
        info = GetNpcMonsterInfo(npc);
        if (info != 0) {
            RemoveMonster(MonsterGetIndexByLocationID(0x2da, ASCENSION_CPP, info->location_id, 1),
                          1);
            return;
        }
    }
    npc = GetNpcStateByKind(0x2f);
    if (npc != 0) {
        info = GetNpcMonsterInfo(npc);
        if (info != 0) {
            RemoveMonster(MonsterGetIndexByLocationID(0x2e7, ASCENSION_CPP, info->location_id, 1),
                          1);
        }
    }
}

/* The Shaker activation callback: play the avalanche rumble. */
// FUNCTION: WIZ8 0x004E04F0
bool AscensionShaker004E04F0(Trigger* pTrigger)
{
    SoundPlay("Data\\Sound\\Ambients\\Rumble_01.wav", 0);
    return true;
}
