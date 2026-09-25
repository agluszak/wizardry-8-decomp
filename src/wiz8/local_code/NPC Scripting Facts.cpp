#include "wiz8/fact_state.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_code/Targeting.h"

#include "soundman.h"
#include "wiz8/layouts/character.h"
#include "wiz8/character_skills.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayMods.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/local_code/party_encumbrance.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatAttack.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/NPCInteractionSubscreen.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/location_variables.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/npc_state.h"
#include "wiz8/npc_items.h"
#include "wiz8/npc_script_file.h"
#include "wiz8/sr_api.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/utility.h"

#include <string.h>
#include <wchar.h>
#include "wiz8/local_code/ItemManager.h"
#include "wiz8/level_specific_code/MasterFunctionList.h"
#include "wiz8/local_code/Factions.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/virtual_file.h"
#include <stdio.h>

#define NPC_SCRIPTING_FACTS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting Facts.cpp"

/* Local Code\NPC Scripting Facts.cpp. Direct assertion ownership. */

/* SOUNDPARMS EOS callback adapter: retail stores this JMP thunk rather than
   ClearScriptedSceneActive's void() entry. */
// FUNCTION: WIZ8 0x005092c0
static void ClearPotionExplosionSoundFlag(void*)
{
    ClearScriptedSceneActive();
}

/* Camera-shake completion callback stored on W8CameraShakeEffect::completion_callback_48. */
// FUNCTION: WIZ8 0x005092d0
static void ReplayEarthquakeShake(void)
{
    CreateCameraShakeEffect(3.0f, 0, 1.0f, 0, 0);
    BeginEndgameSequence();
}

// FUNCTION: WIZ8 0x00506670
void HandleFactChange(int fact_id, unsigned char value)
{
    W8NpcState* npc;
    W8MonsterGroup* group;
    W8WorldItem* world_item;
    W8Character* character;
    Trigger* trigger;
    stParticle* particle;
    W8MonsterInfo* monster_info;
    W8CameraShakeEffect* shake;
    SOUNDPARMS sound;
    srVector3T<float> position;
    wchar_t display_value[10];
    unsigned char fact_value;
    unsigned int added;
    int location_id;
    int slot;
    const char* trigger_name;

    switch (fact_id) {
    case 0x17:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28a, 1, 0x15180);
        return;
    case 0x18:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28b, 1, 0x15180);
        return;
    case 0x19:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28c, 1, 0x15180);
        return;
    case 0x1a:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28d, 1, 0x15180);
        return;
    case 0x1b:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28e, 1, 0x15180);
        return;
    case 0x1f:
        if (value == 0) {
            return;
        }
        if (NpcLeadHasNameStyle(7)) {
            return;
        }
        npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        ReleaseNpcMonsterBinding(npc, 0);
        RestoreNamedNpcAtLevel(npc->name_style, 0x11, "NP_MylesCell");
        return;
    case 0x22:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x48);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x614, 0);
        }
        ClearNpcScheduledItem(npc, 0x242, 0);
        return;
    case 0x28:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_TRIGGER_FIX, 0);
        return;
    case 0x33:
        if (value != 0 && FindEntityByName("NP_BlueFlowers", &position, 0, 0) != 0) {
            world_item = SpawnItem(0x2eb, &position, 3, 1);
            if (world_item != 0) {
                ActivateItem(world_item);
            }
            world_item = SpawnItem(0x2eb, &position, 3, 1);
            if (world_item != 0) {
                ActivateItem(world_item);
            }
        }
        return;
    case 0x37:
        fact_value = EvaluateFact(0x36);
        if (g_status.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x36].symbolic_name,
                        display_value);
        }
        if (fact_value == 0 && FindEntityByName("Brekek", &position, 0, 0) != 0) {
            SpawnMonsters(0x131, 1, &position, 1, 1, 1, 0);
        }
        return;
    case 0x39:
        if (value != 0 && g_status.buffers.Char[g_status.party_slot_249c].uiCondition[0x13] != 0) {
            RemoveCharacterCondition(g_status.party_slot_249c, 0x13, 1);
        }
        return;
    case 0x3a:
        trigger = FindTriggerByName("ZaHealthDoor");
        if (trigger == 0) {
            srAssertFail("pTrigger", NPC_SCRIPTING_FACTS_CPP, 0x640, 0);
        }
        trigger->Run(-1);
        return;
    case 0x44:
        if (value != 0) {
            npc = GetNpcStateByKind(0x20);
            if (npc != 0) {
                ReleaseNpcScriptFile(npc->script_file);
                ReloadNpcScriptResources(npc);
                if (!npc->is_grouped) {
                    CancelNpcDialogue();
                    QueueNpcScriptLine(0, 0, 0, 0);
                    return;
                }
                character = GetNpcGroupCharacter(npc);
                QueueCharacterEvent(character, 0, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
        }
        return;
    case 0x64:
        if (value != 0) {
            MarkNpcOfKind(0x29);
        }
        return;
    case 0x72:
        ReleaseNpcMonsterByKind(0x2c);
        return;
    case 0x86:
        if (value != 0) {
            for (slot = 0; slot < 8; ++slot) {
                character = &g_status.buffers.Char[slot];
                if (g_status.buffers.XChar[slot].fOccupied != 0 &&
                    character->highest_condition < 0x12 &&
                    g_profession_skill_availability[7][character->iProfession] != 0 &&
                    character->skills[7].points_02 < 10) {
                    character->skills[7].points_02 = 10;
                    ApplySkillChange(character, 7);
                }
            }
        }
        return;
    case 0x89:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x29);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x636, 0);
        }
        MarkNpcOfKind(0x29);
        return;
    case 0x91:
        if (value != 0) {
            trigger = FindTriggerByName("Door08");
            if (trigger != 0 && (trigger->flags_0a0 >> 0x13 & 1) == 0) {
                trigger->Run(-1);
            }
        }
        return;
    case 0xa5:
        if (value != 0) {
            trigger = FindTriggerByName("SecurityLasers");
            if (trigger != 0) {
                trigger->Run(-1);
            }
            CreateLocationVar("LasersOff", 1);
        }
        return;
    case 0xa6:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("ewaxxdoortrigger01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0xa8:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door07");
        if (trigger != 0 && (trigger->flags_0a0 >> 0x13 & 1) == 0) {
            trigger->Run(-1);
        }
        return;
    case 0xa9:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door06");
        if (trigger != 0 && (trigger->flags_0a0 >> 0x13 & 1) == 0) {
            trigger->Run(-1);
        }
        return;
    case 0xac:
        trigger = FindTriggerByName("scannerdoor");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("Scanner_Trigger_Plane");
        if (trigger == 0) {
            return;
        }
        trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        return;
    case 0xb6:
        if (value != 0) {
            SetFact(0x308, 1, 0);
        }
        return;
    case 0xb7:
        if (value != 0) {
            QueueNpcMessageLine(W8_NPC_MSG_MOOK_COMMENT, 0);
        }
        return;
    case 0xba:
        trigger = FindTriggerByName("MookFrontDoor");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("Mookoff-01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0xc4:
        trigger = FindTriggerByName("blackboxrecorder");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0xc6:
        group = FindFirstMonsterByID(0x13);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, 0);
        }
        return;
    case 0xc7:
        trigger = FindTriggerByName("redbutton");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0xc9:
        if (value == 0) {
            SetFactionDispositionBand(0xb, 0);
            return;
        }
        SetFactionDispositionBand(0xb, 2);
        return;
    case 0xca:
        if (value == 0) {
            SetFactionDispositionBand(0xc, 0);
            return;
        }
        SetFactionDispositionBand(0xc, 2);
        return;
    case 0xcb:
        if (value == 0) {
            SetFactionDispositionBand(10, 0);
            return;
        }
        SetFactionDispositionBand(10, 2);
        return;
    case 0xcc:
        if (value == 0) {
            SetFactionDispositionBand(9, 0);
            return;
        }
        SetFactionDispositionBand(9, 2);
        return;
    case 0xcd:
    case 0xe0:
        if (value == 0) {
            return;
        }
        fact_value = EvaluateFact(0xd1);
        if (g_status.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0xd1].symbolic_name,
                        display_value);
        }
        if (fact_value != 0) {
            SetFact(0xe2, 1, 0);
        }
        SetFact(0x24d, 0, 0);
        return;
    case 0xdb:
        npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info == 0) {
            return;
        }
        monster_info->p3D->SetScript("Guard.msf", 1);
        return;
    case 0xe8:
        if (value != 0) {
            trigger = FindTriggerByName("MartenMural");
            if (trigger != 0) {
                trigger->Run(-1);
            }
            trigger = FindTriggerByName("Muraltrigger");
            if (trigger == 0) {
                return;
            }
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
            return;
        }
        trigger = FindTriggerByName("Muraltrigger");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x11e:
        if (value == 0) {
            SetFactionDispositionBand(5, 0);
            return;
        }
        SetFactionDispositionBand(5, 2);
        return;
    case 0x14f:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_GOLEM, 0);
        return;
    case 0x151:
        trigger = FindTriggerByName("ChaosBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("ChaosDoor01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_AD, 0);
        return;
    case 0x153:
        trigger = FindTriggerByName("KnowBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("DoorKnow03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_DD, 0);
        return;
    case 0x155:
        trigger = FindTriggerByName("LifeBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("DoorLife03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_CM, 0);
        return;
    case 0x157:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_SAVANT, 0);
        return;
    case 0x15f:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3b);
        SetFactionFlag(0x10, 1);
        return;
    case 0x16c:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x48), 0x242, 1, 0);
        return;
    case 0x16f:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_JANETTE, 0);
        return;
    case 0x173:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_MARTEN, 0);
        return;
    case 0x179:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x2b);
        return;
    case 0x183:
        if (value == 0) {
            SetFactionDispositionBand(4, 0);
            return;
        }
        SetFactionDispositionBand(4, 2);
        return;
    case 0x188:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLight23MeansOpen");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLight23MeansOpen01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x195:
        if (value == 0) {
            return;
        }
        for (slot = 0; slot < 8; ++slot) {
            character = &g_status.buffers.Char[slot];
            if (g_status.buffers.XChar[slot].fOccupied != 0 && character->hp_current != 0 &&
                character->highest_condition < 0x12) {
                added = 100 - character->attributes[1].value;
                if (added > 5) {
                    added = 5;
                }
                character->attributes[1].value += added;
                ApplyAttributeChange(character, 1);
            }
        }
        ShowString(gppStringList[0x770 / 4]);
        trigger = FindTriggerByName("FOUNT_RIDDLE");
        if (trigger == 0) {
            return;
        }
        trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        return;
    case 0x197:
        QueueNpcMessageLine(W8_NPC_MSG_MILANO_RAT_DOOR, 0);
        return;
    case 0x19b:
        if (value == 0) {
            return;
        }
        SetFact(0x273, 1, 0);
        return;
    case 0x19c:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_GARI, 0);
        return;
    case 0x19e:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x49), 0x1b0, 1, 0x15180);
        return;
    case 0x1a0:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_BELA, 0);
        return;
    case 0x1a5:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SHAMAN, 0);
        return;
    case 0x1a8:
        if (value == 0) {
            SetFactionDispositionBand(0xf, 0);
            return;
        }
        SetFactionDispositionBand(0xf, 2);
        return;
    case 0x1b3:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_ASAIZ, 0);
        return;
    case 0x1b4:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_LURE, 0);
        return;
    case 0x1b5:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_MADEUS, 0);
        return;
    case 0x1b8:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3c);
        trigger = FindTriggerByName("templargate07");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x1ba:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3a);
        trigger = FindTriggerByName("templargate10");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x1bf:
        if (value == 0) {
            return;
        }
        BeginNpcScriptedScene();
        return;
    case 0x1c2:
        if (value == 0) {
            QueueNpcMessageLine(W8_NPC_MSG_SEDEXUS_PASSOUT, 0);
            return;
        }
        BeginSedexusCapture();
        return;
    case 0x1c9:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x44);
        return;
    case 0x1dc:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 500, 1, 0x15180);
        return;
    case 0x1dd:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 0x1f5, 1, 0x15180);
        return;
    case 0x1de:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 0x1f8, 1, 0x15180);
        return;
    case 0x1e7:
        QueueNpcMessageLine(W8_NPC_MSG_ALETHEIDES_LEAVES, 0);
        return;
    case 0x1ec:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x56);
        return;
    case 0x1f1:
        if (value == 0) {
            return;
        }
        ReleaseNpcMonsterByKind(0x55);
        QueueNpcMessageLine(W8_NPC_MSG_PRINCE_NOT_HOME, 0);
        return;
    case 0x1f2:
        QueueNpcMessageLine(W8_NPC_MSG_PRINCE_DISAPPEARS, 0);
        return;
    case 0x203:
        if (value == 0) {
            return;
        }
        position.x = 53550.0f;
        position.y = 3516.0f;
        position.z = 36936.0f;
        CameraLookAt(&position);
        fact_value = EvaluateFact(0x177);
        if (g_status.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x177].symbolic_name,
                        display_value);
        }
        if (fact_value == 0) {
            trigger_name = "TR2ShipTrigger";
        } else {
            trigger_name = "glassTrigger";
        }
        trigger = FindTriggerByName(trigger_name);
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x204:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x52);
        return;
    case 0x206:
        QueueNpcMessageLine(W8_NPC_MSG_HENCHMAN_LEAVES, 0);
        return;
    case 0x208:
        QueueNpcMessageLine(W8_NPC_MSG_CALL_HENCHMAN, 0);
        return;
    case 0x20f:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_TO_BOOK, 0);
        return;
    case 0x213:
        QueueNpcMessageLine(W8_NPC_MSG_TURN_TO_BOOK, 0);
        return;
    case 0x214:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SAVANT_HACK, 0);
        return;
    case 0x215:
        QueueNpcMessageLine(W8_NPC_MSG_SAVANT_APPEARS, 0);
        return;
    case 0x219:
        g_status.endgame2_queued = 1;
        return;
    case 0x21b:
        QueueNpcMessageLine(W8_NPC_MSG_PHOONZANG_SPLIT, 0);
        return;
    case 0x21c:
        g_status.endgame3_queued = 1;
        return;
    case 0x21e:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x29);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x62c, 0);
        }
        MarkNpcOfKind(0x29);
        return;
    case 0x220:
    case 0x221:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLightMeansGo");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x222:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLightMeansGo01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x223:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door18");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLightEwaxx02");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLightEwaxx03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        location_id = GetLocationVarIDByName("ObstacleDoors");
        if (location_id != -1) {
            SetTriggerVariableByName("ObstacleDoors",
                                     GetLocationVarValueByName("ObstacleDoors") | 2);
            return;
        }
        CreateLocationVar("ObstacleDoors", 2);
        return;
    case 0x224:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door19");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLightEwaxx");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLightEwaxx01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        location_id = GetLocationVarIDByName("ObstacleDoors");
        if (location_id != -1) {
            SetTriggerVariableByName("ObstacleDoors",
                                     GetLocationVarValueByName("ObstacleDoors") | 1);
            return;
        }
        CreateLocationVar("ObstacleDoors", 1);
        return;
    case 0x225:
    case 0x226:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLightMeansOpen02");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("greenLightMeansOpen03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case 0x22a:
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SELF, 0);
        return;
    case 0x22b:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x18);
        EndNpcDialogueSession(0);
        return;
    case 0x234:
    case 0x235:
        memset(&sound, -1, sizeof(sound));
        sound.EOSCallback = ClearPotionExplosionSoundFlag;
        if (SoundPlay("Data\\Sound\\misc\\potion_exploding.wav", &sound) != 0xffffffff) {
            SetScriptedSceneActive();
        }
        particle = FindParticleByName(g_world, "FuzzBlast");
        if (particle != 0) {
            particle->SetActive(0);
            particle->start_frame_264 = 0;
            particle->emission_count_188 = 0;
            particle->SetActive(1);
            particle->SetTraversalEnabled(1);
        }
        npc = GetNpcStateByKind(0x49);
        if (npc == 0) {
            return;
        }
        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info == 0) {
            return;
        }
        if (monster_info->p3D->IsCycleInterruptable(monster_info->p3D->m_pRep->pending_cycle) ==
            0) {
            return;
        }
        StartMonsterCycle(monster_info, 0x14, 1);
        return;
    case 0x259:
        npc = GetNpcStateByKind(0x57);
        if (npc == 0) {
            return;
        }
        if (npc->greeting_pending != 0) {
            return;
        }
        ReleaseNpcMonsterByKind(0x57);
        return;
    case 0x27c:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_BALBRAK_HOME, 0);
        return;
    case 0x2a5:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_RUBBLE, 0);
        return;
    case 0x2a6:
        if (value != 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SPACER, 0);
        return;
    case 0x2ed:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SEDEXUS_LEAVES, 0);
        return;
    case 0x2ef:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SEDEXUS_RIFT, 0);
        return;
    case 0x2f0:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_TO_BOOK2, 0);
        return;
    case 0x2f3:
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_RPC_VI, 0);
        return;
    case 0x2f4:
        if (value == 0) {
            return;
        }
        g_status.vi_event_stage_498b = 1;
        group = FindFirstMonsterByID(0x234);
        if (group != 0) {
            SetMonsterGroupHostility(group, 2, 0);
        }
        group = FindFirstMonsterByID(0x1b4);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, 0);
        }
        group = FindFirstMonsterByID(0x1b9);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, 0);
        }
        GetNpcStateByKind(0x89);
        return;
    case 0x314:
        if (value == 0) {
            SetFactionDispositionBand(0x10, 0);
            SetFactionDispositionBand(0xf, 0);
            return;
        }
        SetFactionDispositionBand(0x10, 2);
        SetFactionDispositionBand(0xf, 2);
        return;
    case 0x320:
    case 0x321:
        QueueNpcMessageLine(W8_NPC_MSG_BEGIN_ENDGAME, 0);
        return;
    case 0x322:
        EndNpcDialogueSession(0);
        ResetLevelDataVectors();
        group = FindFirstMonsterByID(0xc2);
        if (group != 0) {
            monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x4b4, NPC_SCRIPTING_FACTS_CPP, group->leader_location_id, 1));
            monster_info->p3D->SetScript("MoveSavantBoffo.msf", 1);
        }
        SoundPlayStreamedFile("Data\\Sound\\Misc\\Earthquake End.wav", 0);
        shake = CreateCameraShakeEffect(6.0f, 0, 1.0f, 0, 0);
        shake->flags_00 |= 0x20;
        shake->completion_callback_48 = ReplayEarthquakeShake;
        return;
    default:
        return;
    }
}

// FUNCTION: WIZ8 0x00508d70
void HandleScriptedNpcDeath(unsigned int monster_list_index)
{
    W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_list_index);
    W8MonsterRecord* record = GetMonsterDataForInfo(monster_info);
    if (record == 0) {
        return;
    }
    if ((record->flags_0d0 & 1) != 0) {
        W8NpcState* npc = GetNpcStateByKind(record->npc_kind_0cd);
        if (npc != 0) {
            npc->spawned_04 = 1;
            if (npc->name_style == 0x18) {
                wchar_t display_value[16];
                unsigned char fact_ok = EvaluateFact(0xc1);
                if (g_status.log_fact_checks_3120 != 0) {
                    if (fact_ok) {
                        wcscpy(display_value, L"TRUE");
                    } else {
                        wcscpy(display_value, L"FALSE");
                    }
                    ShowNoticef(5, L"Checking fact %S which is %s",
                                g_fact_records[0xc1].symbolic_name, display_value);
                }
                if (fact_ok == 0) {
                    fact_ok = EvaluateFact(0xdb);
                    if (g_status.log_fact_checks_3120 != 0) {
                        if (fact_ok) {
                            wcscpy(display_value, L"TRUE");
                        } else {
                            wcscpy(display_value, L"FALSE");
                        }
                        ShowNoticef(5, L"Checking fact %S which is %s",
                                    g_fact_records[0xdb].symbolic_name, display_value);
                    }
                    if (fact_ok == 0) {
                        npc->spawned_04 = 0;
                    }
                }
            }
        }
    }
    if (record->record_id_187 != 0x234) {
        return;
    }
    int lead_index = -1;
    if (NpcLeadHasNameStyle(0x18) != 0) {
        W8NpcState* lead = GetNpcStateByKind(W8_NPC_VI_DOMINA);
        if (lead != 0) {
            lead_index = lead->group_index;
        }
    }
    BeginScriptedWorldAction();
    int eligible_slots[8];
    int eligible_count = 0;
    int slot;
    int index;
    int pick;
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        if (g_status.buffers.XChar[slot].fOccupied != 0 && character->hp_current != 0 &&
            character->highest_condition < 0xf) {
            eligible_slots[eligible_count] = slot;
            ++eligible_count;
        }
    }
    for (index = 0; index < eligible_count; ++index) {
        if (eligible_slots[index] == lead_index) {
            QueueCharacterEvent(&g_status.buffers.Char[eligible_slots[index]], g_effect_005ee618,
                                g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                                g_effect_argument_005ed914);
        }
    }
    if (eligible_count > 2) {
        do {
            pick = Random(eligible_count);
        } while (eligible_slots[pick] == lead_index);
        QueueCharacterEvent(&g_status.buffers.Char[eligible_slots[pick]], g_effect_005ee618,
                            g_event_flag_005ed8e0, g_effect_argument_005ed8c8,
                            g_effect_argument_005ed914);
    }
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        if (g_status.buffers.XChar[slot].fOccupied != 0 && character->hp_current != 0 &&
            character->highest_condition < 0xf) {
            QueueCharacterEvent(character, g_effect_005ee630, g_event_flag_005ed8e0,
                                g_effect_argument_005ed8c8, g_effect_argument_005ed914);
        }
    }
    if (g_status.endgame2_queued != 0 ||
        (g_status.endgame3_queued != 0 && FindNpcOfKind(W8_NPC_PHOONZANG) != 0)) {
        QueueNpcMessageLine(W8_NPC_MSG_TURN_TO_BOOK, 0);
    } else if (g_status.endgame3_queued != 0) {
        QueueNpcMessageLine(W8_NPC_MSG_PHOONZANG_NOTICE, 0);
    }
    for (unsigned int entry_index = 0; entry_index < PLLength(gXStatus.plsMonsterList);
         ++entry_index) {
        W8MonsterInfo* entry = MonsterGetScriptPartByLocationIndex(entry_index);
        if (entry->fActive && entry->ubDisposition == DISP_HOSTILE && entry->p3D->IsDying() == 0) {
            TintHighlightedMonster(entry->p3D, 0);
            MonsterStartsDying(entry, 1);
        }
    }
}

// GLOBAL: WIZ8 0x005ee6f8
int g_effect_005ee6f8 = 131;

/* Scripted kill reactions keyed by monster record id: facts and faction
   changes for the special kills, the Rattkin breeder location-variable count,
   and the 0x22b cleanup that clears the victim's condition and queues the
   death event on the recorded party slot. */
// FUNCTION: WIZ8 0x005090C0
void MonsterKilled(int record_id, int killer_party_slot)
{
    unsigned char value;
    wchar_t display_value[10];

    if (record_id == 0x131) {
        SetFact(0x36, 1, 0);
        return;
    }
    if (record_id == 0x1a9) {
        SetFact(0x318, 1, 0);
        ApplyFactionChange(2, 1, 0x10, 0x14);
        return;
    }
    if (record_id == 0x14f || record_id == 0xdd) {
        if (GetLocationVarIDByName("NumberRattkinBreedersKilled") == -1) {
            CreateLocationVar("NumberRattkinBreedersKilled", 1);
            return;
        }
        SetTriggerVariableByName("NumberRattkinBreedersKilled", 2);
        SetFact(0x19b, 1, 0);
        SetFactionDispositionBand(7, 0);
    } else {
        if (record_id == 0x22b) {
            value = EvaluateFact(0x1be);
            if (g_status.log_fact_checks_3120 != 0) {
                if (value != 0) {
                    wcscpy(display_value, L"TRUE");
                } else {
                    wcscpy(display_value, L"FALSE");
                }
                ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[0x1be].symbolic_name,
                            display_value);
            }
            if (value != 0) {
                SetFact(0x2a6, 0, 0);
            }
            if (g_status.rpc_active_2489 != 0) {
                if (g_status.buffers.Char[g_status.sedexus_party_slot_247f].uiCondition[10] > 0) {
                    RemoveCharacterCondition(g_status.sedexus_party_slot_247f, 10, 0);
                }
                QueueCharacterEvent(&g_status.buffers.Char[g_status.sedexus_party_slot_247f],
                                    g_effect_005ee6f8, 0, g_effect_argument_005ed8c8,
                                    g_effect_argument_005ed914);
            }
            SetFact(0x1b6, 1, 0);
            return;
        }
        if (record_id == 0x181) {
            SetFact(0x1e8, 1, 0);
            return;
        }
        if (record_id == 0x175 || record_id == 0x222) {
            SetFact(0x326, 1, 0);
            return;
        }
    }
}

/* Unresolved fragment: five of the six functions lie in the anchored gap
   between Sight.cpp (ends 0x00505F30) and NPC Scripting Facts.cpp
   (0x00506670); 0x005080F0 sits past that hull in the gap before
   NPC Manager.cpp (0x00509CD0). Two clusters, no proven ownership. */

// GLOBAL: WIZ8 0x00689b78
unsigned char g_fact_values[1000];

// FUNCTION: WIZ8 0x00506280
unsigned char GetFact(int fact_id)
{
    unsigned char value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return 0;
    }

    value = EvaluateFact(fact_id);
    if (g_status.log_fact_checks_3120) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(5, L"Checking fact %S which is %s", g_fact_records[fact_id].symbolic_name,
                    display_value);
    }
    return value;
}

// FUNCTION: WIZ8 0x005061a0
void SetFact(int fact_id, unsigned char value, unsigned char suppress_side_effects)
{
    unsigned char previous_value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return;
    }

    previous_value = g_fact_values[fact_id];
    g_fact_values[fact_id] = value;

    if (fact_id < (int)gXStatus.uiFactsInDatabase) {
        if (value) {
            sprintf((char*)display_value, "TRUE");
        } else {
            sprintf((char*)display_value, "FALSE");
        }
    }

    if (!suppress_side_effects) {
        if (g_fact_values[fact_id] != previous_value) {
            RecordFactChangeForJournal(fact_id);
        }
        HandleFactChange(fact_id, value);

        if (g_status.log_fact_checks_3120) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            ShowNoticef(5, L"%S set to %s", g_fact_records[fact_id].symbolic_name, display_value);
        }
    }
}

/* The whole 1001-byte fact array minus its last entry goes to the save file in
   one write. The original passes the address of its own parameter as the
   bytes-written out-parameter: the handle has already been copied into a
   register, so the incoming slot is dead and doubles as the scratch the callee
   requires. Reproduced literally, because a separate local would cost a stack
   frame the canonical body does not have. */
// FUNCTION: WIZ8 0x00506480
void SaveFactState(int save_handle)
{
    FileWrite(save_handle, g_fact_values, 1000, (unsigned int*)&save_handle);
}

/* Clears every fact, then seeds the ones a fresh party starts with. A party
   imported from Wizardry 7 is the skip-loose-character-check path: ending
   choice 1/2/other maps to facts 0x4c/0x4b/0x4d, then two independent import
   bytes can set 0x199 and 0x7b. The 0x7b path unsuppresses and returns; the
   other imported path and the new-game path unsuppress at the shared exit. */
// FUNCTION: WIZ8 0x00506310
void InitializeFactState(void)
{
    /* Retail memsets 1000 of the 1001 bytes - index 1000 stays BSS-zeroed. */
    memset(g_fact_values, 0, 1000);
    SetFactNotificationsSuppressed(1);
    if (g_status.skip_loose_character_check_2444) {
        SetFact(0x75, 1, 0);
        switch (g_wiz7_ending) {
        case 1:
            SetFact(0x4c, 1, 0);
            break;
        case 2:
            SetFact(0x4b, 1, 0);
            break;
        default:
            SetFact(0x4d, 1, 0);
            break;
        }
        if (g_import_flags[0xb]) {
            SetFact(0x199, 1, 0);
        }
        if (g_import_flags[5]) {
            SetFact(0x7b, 1, 0);
            SetFactNotificationsSuppressed(0);
            return;
        }
    } else {
        SetFact(0x4e, 1, 0);
        SetFact(0x279, 1, 0);
        SetFact(0x27a, 1, 0);
        SetFact(0x27b, 1, 0);
    }
    SetFactNotificationsSuppressed(0);
}
