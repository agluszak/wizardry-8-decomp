#include "wiz8/fonts.h"
#include "wiz8/fact_state.h"
#include "wiz8/integer_constants.h"
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

/* Camera-shake completion callback stored on W8CameraShakeEffect::completion_callback. */
// FUNCTION: WIZ8 0x005092d0
static void ReplayEarthquakeShake(void)
{
    CreateCameraShakeEffect(3.0f, false, 1.0f, 0, 0);
    BeginEndgameSequence();
}

// FUNCTION: WIZ8 0x00506670
void HandleFactChange(W8FactId fact_id, unsigned char value)
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
    unsigned char fact_value;
    unsigned int added;
    int location_id;
    int slot;
    const char* trigger_name;

    switch (fact_id) {
    case W8_FACT_ANTONE_ZYNARYX_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28a, 1, 0x15180);
        return;
    case W8_FACT_ANTONE_STEELHIDE_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28b, 1, 0x15180);
        return;
    case W8_FACT_ANTONE_FEATHERWEIGHT_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28c, 1, 0x15180);
        return;
    case W8_FACT_ANTONE_BEASTSLAYER_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28d, 1, 0x15180);
        return;
    case W8_FACT_ANTONE_EBON_STAFF_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(3), 0x28e, 1, 0x15180);
        return;
    case W8_FACT_RAPAX_MYLES_IN_JAIL:
        if (value == 0) {
            return;
        }
        if (NpcLeadHasNameStyle(W8_NPC_MYLES)) {
            return;
        }
        npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        ReleaseNpcMonsterBinding(npc, 0);
        RestoreNamedNpcAtLevel(npc->name_style, 0x11, "NP_MylesCell");
        return;
    case W8_FACT_ASTRAL_POSSESS:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x48);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x614, 0);
        }
        ClearNpcScheduledItem(npc, 0x242, 0);
        return;
    case W8_FACT_PEACE_SAVANT_BLOWUP_DONE:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_TRIGGER_FIX, 0);
        return;
    case W8_FACT_CROCK_KIDNAPPED_PLAYER:
        if (value != 0 && FindEntityByName("NP_BlueFlowers", &position, 0, 0)) {
            world_item =
                SpawnItem(0x2eb, &position, W8_ITEM_ENTITY_PULSE | W8_ITEM_ENTITY_ROTATE, true);
            if (world_item != 0) {
                ActivateItem(world_item);
            }
            world_item =
                SpawnItem(0x2eb, &position, W8_ITEM_ENTITY_PULSE | W8_ITEM_ENTITY_ROTATE, true);
            if (world_item != 0) {
                ActivateItem(world_item);
            }
        }
        return;
    case W8_FACT_CROCK_BREKEK_ASSIGN:
        fact_value = GetFact(W8_FACT_CROCK_BREKEK_DEAD);
        if (fact_value == 0 && FindEntityByName("Brekek", &position, 0, 0)) {
            SpawnMonsters(0x131, 1, &position, 1, true, true, false);
        }
        return;
    case W8_FACT_CROCK_PLAYER_RETURNED_TO_PARTY:
        if (value != 0 &&
            g_status.buffers.Char[g_status.party_slot].uiCondition[W8_CONDITION_MISSING] != 0) {
            RemoveCharacterCondition(g_status.party_slot, W8_CONDITION_MISSING, true);
        }
        return;
    case W8_FACT_ALIGNMENT_TRANG:
        trigger = FindTriggerByName("ZaHealthDoor");
        if (trigger == 0) {
            srAssertFail("pTrigger", NPC_SCRIPTING_FACTS_CPP, 0x640, 0);
        }
        trigger->Run(-1);
        return;
    case W8_FACT_RFS81_HAS_BEEN_FIXED:
        if (value != 0) {
            npc = GetNpcStateByKind(0x20);
            if (npc != 0) {
                ReleaseNpcScriptFile(npc->script_file);
                ReloadNpcScriptResources(npc);
                if (!npc->is_grouped) {
                    CancelNpcDialogue();
                    QueueNpcScriptLine(0, false, false, false);
                    return;
                }
                character = GetNpcGroupCharacter(npc);
                QueueCharacterEvent(character, 0, 0, g_character_event_no_flags,
                                    g_character_event_full_volume);
            }
        }
        return;
    case W8_FACT_UMISSION_OBCOURSE_DONE:
        if (value != 0) {
            MarkNpcOfKind(0x29);
        }
        return;
    case W8_FACT_KUNAR_IS_DEAD:
        ReleaseNpcMonsterByKind(0x2c);
        return;
    case W8_FACT_UMISSION_TRAIN_COVERT_ASSIGN:
        if (value != 0) {
            for (slot = 0; slot < 8; ++slot) {
                character = &g_status.buffers.Char[slot];
                if (g_status.buffers.XChar[slot].fOccupied &&
                    character->highest_condition < W8_CONDITION_DEAD &&
                    g_profession_skill_availability[7][character->iProfession] != 0 &&
                    character->skills[W8_SKILL_MODERN_WEAPONS].points < 10) {
                    character->skills[W8_SKILL_MODERN_WEAPONS].points = 10;
                    ApplySkillChange(character, W8_SKILL_MODERN_WEAPONS);
                }
            }
        }
        return;
    case W8_FACT_UMISSION_IUFPASS_LEVEL4:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x29);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x636, 0);
        }
        MarkNpcOfKind(0x29);
        return;
    case W8_FACT_UMISSION_TRAIN_COVERT_OPEN:
        if (value != 0) {
            trigger = FindTriggerByName("Door08");
            if (trigger != 0 && (trigger->flags & W8_TRIGGER_FIRED) == 0) {
                trigger->Run(-1);
            }
        }
        return;
    case W8_FACT_LOCATION_GIGAS_MOTION_SENSORS_OFF:
        if (value != 0) {
            trigger = FindTriggerByName("SecurityLasers");
            if (trigger != 0) {
                trigger->Run(-1);
            }
            CreateLocationVar("LasersOff", 1);
        }
        return;
    case W8_FACT_LOCATION_GIGAS_OUTER_GATE_OPEN:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("ewaxxdoortrigger01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_UMISSION_TRAIN_GUNNARY_OPEN:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door07");
        if (trigger != 0 && (trigger->flags & W8_TRIGGER_FIRED) == 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_UMISSION_TRAIN_UTU_OPEN:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("Door06");
        if (trigger != 0 && (trigger->flags & W8_TRIGGER_FIRED) == 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_ARNIKA_HLL_OPEN:
        trigger = FindTriggerByName("scannerdoor");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("Scanner_Trigger_Plane");
        if (trigger == 0) {
            return;
        }
        trigger->flags &= ~W8_TRIGGER_ENABLED;
        return;
    case W8_FACT_MOOK_MOOK_PC_SWAP_COMPLETE:
        if (value != 0) {
            SetFact(W8_FACT_QUEST_MOOK_BUDDY_TELLS_OF_CM, 1, false);
        }
        return;
    case W8_FACT_MOOK_MOOK_PC_REMOVED:
        if (value != 0) {
            QueueNpcMessageLine(W8_NPC_MSG_MOOK_COMMENT, 0);
        }
        return;
    case W8_FACT_MOOK_HALL_OPEN:
        trigger = FindTriggerByName("MookFrontDoor");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        trigger = FindTriggerByName("Mookoff-01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_ARNIKA_BLACKBOX_RECEIVED:
        trigger = FindTriggerByName("blackboxrecorder");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_ARNIKA_MYLES_TROOPERS_HOSTILE:
        group = FindFirstMonsterByID(0x13);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, false);
        }
        return;
    case W8_FACT_ARNIKA_SAFETY_DEPOSIT_OPEN:
        trigger = FindTriggerByName("redbutton");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_FACTION_HIGARDI_HLL_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(0xb, 0);
            return;
        }
        SetFactionDispositionBand(0xb, 2);
        return;
    case W8_FACT_FACTION_HIGARDI_COMMON_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(0xc, 0);
            return;
        }
        SetFactionDispositionBand(0xc, 2);
        return;
    case W8_FACT_FACTION_HIGARDI_BANK_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(10, 0);
            return;
        }
        SetFactionDispositionBand(10, 2);
        return;
    case W8_FACT_FACTION_HIGARDI_FELLOW_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(9, 0);
            return;
        }
        SetFactionDispositionBand(9, 2);
        return;
    case W8_FACT_ARNIKA_VAULT_TELEPORT:
    case W8_FACT_ARNIKA_VAULT_ENTERED:
        if (value == 0) {
            return;
        }
        fact_value = GetFact(W8_FACT_MYLES_IN_PARTY);
        if (fact_value != 0) {
            SetFact(W8_FACT_MYLES_IN_PARTY_WHEN_BANK_DONE, 1, false);
        }
        SetFact(W8_FACT_QUEST_MYLES_BANK_VAULT, 0, false);
        return;
    case W8_FACT_MYLES_MISSION_RESCUE_VI_ASSIGNED:
        npc = GetNpcStateByKind(7);
        if (npc == 0) {
            return;
        }
        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info == 0) {
            return;
        }
        monster_info->p3D->SetScript("Guard.msf", true);
        return;
    case W8_FACT_MURAL_OPEN:
        if (value != 0) {
            trigger = FindTriggerByName("MartenMural");
            if (trigger != 0) {
                trigger->Run(-1);
            }
            trigger = FindTriggerByName("Muraltrigger");
            if (trigger == 0) {
                return;
            }
            trigger->flags &= ~W8_TRIGGER_ENABLED;
            return;
        }
        trigger = FindTriggerByName("Muraltrigger");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_FACTION_TRANG_COMMON_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(5, 0);
            return;
        }
        SetFactionDispositionBand(5, 2);
        return;
    case W8_FACT_AP_GOLEM_ATTACK:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_GOLEM, 0);
        return;
    case W8_FACT_CM_RIDDLE2:
        trigger = FindTriggerByName("ChaosBTrigger");
        if (trigger != 0) {
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("ChaosDoor01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_AD, 0);
        return;
    case W8_FACT_DD_RIDDLE2:
        trigger = FindTriggerByName("KnowBTrigger");
        if (trigger != 0) {
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("DoorKnow03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_DD, 0);
        return;
    case W8_FACT_AD_RIDDLE2:
        trigger = FindTriggerByName("LifeBTrigger");
        if (trigger != 0) {
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        }
        trigger = FindTriggerByName("DoorLife03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_ALETHEIDES_CM, 0);
        return;
    case W8_FACT_AP_SAVANT_DISAPPEARS:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_SAVANT, 0);
        return;
    case W8_FACT_TEMPLAR:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3b);
        SetFactionFlag(0x10, true);
        return;
    case W8_FACT_RATTKIN_HAVE_AD:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x48), 0x242, 1, 0);
        return;
    case W8_FACT_BAYJIN_JANETTE_DEAD:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_JANETTE, 0);
        return;
    case W8_FACT_MARTEN_FINISHED:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_MARTEN, 0);
        return;
    case W8_FACT_UMISSION_SCUBA_SOLVED_NO_ALIGN:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x2b);
        return;
    case W8_FACT_FACTION_UMPANI_COMMON_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(4, 0);
            return;
        }
        SetFactionDispositionBand(4, 2);
        return;
    case W8_FACT_LOCATION_GIGAS_TELEPORT_OPEN:
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
    case W8_FACT_TRYNNIE_FOUNTAIN_ANSWERED_CORRECT:
        if (value == 0) {
            return;
        }
        for (slot = 0; slot < 8; ++slot) {
            character = &g_status.buffers.Char[slot];
            if (g_status.buffers.XChar[slot].fOccupied && character->hp_current != 0 &&
                character->highest_condition < W8_CONDITION_DEAD) {
                added = 100 - character->attributes[W8_ATTRIBUTE_INTELLIGENCE].base;
                if (added > 5) {
                    added = 5;
                }
                character->attributes[W8_ATTRIBUTE_INTELLIGENCE].base += added;
                ApplyAttributeChange(character, W8_ATTRIBUTE_INTELLIGENCE);
            }
        }
        ShowString(gppStringList[0x1dc]);
        trigger = FindTriggerByName("FOUNT_RIDDLE");
        if (trigger == 0) {
            return;
        }
        trigger->flags &= ~W8_TRIGGER_ENABLED;
        return;
    case W8_FACT_RATTKIN_WHO_LOOKING_FOR:
        QueueNpcMessageLine(W8_NPC_MSG_MILANO_RAT_DOOR, 0);
        return;
    case W8_FACT_TRYNNIE_KILL_RATS_DONE:
        if (value == 0) {
            return;
        }
        SetFact(W8_FACT_QUEST_MADRAS_OPEXTERM, 1, false);
        return;
    case W8_FACT_TRYNNIE_KILL_RATS_ASSIGNED:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_GARI, 0);
        return;
    case W8_FACT_TRYNNIE_FUZZFAS_POTION:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x49), 0x1b0, 1, 0x15180);
        return;
    case W8_FACT_AP_BELA_DONE:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_BELA, 0);
        return;
    case W8_FACT_TRYNNIE_GOT_DESTINY:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SHAMAN, 0);
        return;
    case W8_FACT_FACTION_RAPAX_COMMON_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(0xf, 0);
            return;
        }
        SetFactionDispositionBand(0xf, 2);
        return;
    case W8_FACT_RAPAX_RIDDLE1:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_ASAIZ, 0);
        return;
    case W8_FACT_RAPAX_RIDDLE3:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_LURE, 0);
        return;
    case W8_FACT_RAPAX_RIDDLE2:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_PILLARGATE_MADEUS, 0);
        return;
    case W8_FACT_RAPAX_INITIATES:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3c);
        trigger = FindTriggerByName("templargate07");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_RAPAX_OPEN_TEMPLE:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x3a);
        trigger = FindTriggerByName("templargate10");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_ALSEDEXUS_SACRIFICE_SELECT:
        if (value == 0) {
            return;
        }
        BeginNpcScriptedScene();
        return;
    case W8_FACT_ALSEDEXUS_PARTY_PASSOUT:
        if (value == 0) {
            QueueNpcMessageLine(W8_NPC_MSG_SEDEXUS_PASSOUT, 0);
            return;
        }
        BeginSedexusCapture();
        return;
    case W8_FACT_RIFT_RAFE_FREE:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x44);
        return;
    case W8_FACT_FERRO_MIRROR_ARMOR_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 500, 1, 0x15180);
        return;
    case W8_FACT_FERRO_IVORY_BLADE_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 0x1f5, 1, 0x15180);
        return;
    case W8_FACT_FERRO_VAMPIRE_CHAIN_ALL_INGREDIENTS:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x39), 0x1f8, 1, 0x15180);
        return;
    case W8_FACT_ALETHEIDES_DISAPPEARS:
        QueueNpcMessageLine(W8_NPC_MSG_ALETHEIDES_LEAVES, 0);
        return;
    case W8_FACT_QUEEN_IS_FREE:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x56);
        return;
    case W8_FACT_PRINCE_NOT_IN_CASTLE:
        if (value == 0) {
            return;
        }
        ReleaseNpcMonsterByKind(0x55);
        QueueNpcMessageLine(W8_NPC_MSG_PRINCE_NOT_HOME, 0);
        return;
    case W8_FACT_PRINCE_DISAPPEARS:
        QueueNpcMessageLine(W8_NPC_MSG_PRINCE_DISAPPEARS, 0);
        return;
    case W8_FACT_UMPANI_INITIATE_LANDING:
        if (value == 0) {
            return;
        }
        position.Set(53550.0f, 3516.0f, 36936.0f);
        CameraLookAt(&position);
        fact_value = GetFact(W8_FACT_TMISSION_KILL_UMPANI_CORD_SEVERED);
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
    case W8_FACT_RATTUS_BANK_ROBBERY:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x52);
        return;
    case W8_FACT_HENCHMAN_DISAPPEARS:
        QueueNpcMessageLine(W8_NPC_MSG_HENCHMAN_LEAVES, 0);
        return;
    case W8_FACT_ALETHEIDES_CALLS_HENCHMAN:
        QueueNpcMessageLine(W8_NPC_MSG_CALL_HENCHMAN, 0);
        return;
    case W8_FACT_ENDGAME_MOVE_TO_BOOK:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_TO_BOOK, 0);
        return;
    case W8_FACT_ENDGAME_TURN_TOWARD_BOOK_WRITE:
        QueueNpcMessageLine(W8_NPC_MSG_TURN_TO_BOOK, 0);
        return;
    case W8_FACT_SAVANT_HACK_ALETHEIDES:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SAVANT_HACK, 0);
        return;
    case W8_FACT_ENDGAME_SAVANT_APPEARS:
        QueueNpcMessageLine(W8_NPC_MSG_SAVANT_APPEARS, 0);
        return;
    case W8_FACT_QUE_ENDGAME2:
        g_status.endgame2_queued = 1;
        return;
    case W8_FACT_ENDGAME_SAVANT_PHOON_SPLIT:
        QueueNpcMessageLine(W8_NPC_MSG_PHOONZANG_SPLIT, 0);
        return;
    case W8_FACT_QUE_ENDGAME3:
        g_status.endgame3_queued = 1;
        return;
    case W8_FACT_UMISSION_MOVE_RUBBLE_COVERT:
        if (value == 0) {
            return;
        }
        npc = GetNpcStateByKind(0x29);
        if (npc == 0) {
            srAssertFail("pNPC", NPC_SCRIPTING_FACTS_CPP, 0x62c, 0);
        }
        MarkNpcOfKind(0x29);
        return;
    case W8_FACT_LOCATION_GIGAS_TOGGLE_LIFT3:
    case W8_FACT_LOCATION_GIGAS_TOGGLE_LIFT2:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLightMeansGo");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_LOCATION_GIGAS_TOGGLE_LIFT1:
        if (value == 0) {
            return;
        }
        trigger = FindTriggerByName("greenLightMeansGo01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        return;
    case W8_FACT_LOCATION_GIGAS_OFFICERS_QUARTERS_GATE_OPEN2:
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
    case W8_FACT_LOCATION_GIGAS_OFFICERS_QUARTERS_GATE_OPEN1:
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
    case W8_FACT_LOCATION_GIGAS_TOP_DOOR_OPEN1:
    case W8_FACT_LOCATION_GIGAS_TOP_DOOR_OPEN2:
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
    case W8_FACT_TRANG_INTRO_DISAPPEAR_GUARD:
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SELF, 0);
        return;
    case W8_FACT_INTRO_VI_DISAPPEARS:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x18);
        EndNpcDialogueSession(false);
        return;
    case W8_FACT_FUZZFAS_BOOM1:
    case W8_FACT_FUZZFAS_BOOM2:
        memset(&sound, -1, sizeof(sound));
        sound.EOSCallback = ClearPotionExplosionSoundFlag;
        if (SoundPlay("Data\\Sound\\misc\\potion_exploding.wav", &sound) != 0xffffffff) {
            SetScriptedSceneActive();
        }
        particle = FindParticleByName(g_world, "FuzzBlast");
        if (particle != 0) {
            particle->SetActive(0);
            particle->start_frame = 0;
            particle->emission_count = 0;
            particle->SetActive(1);
            particle->SetTraversalEnabled(true);
        }
        npc = GetNpcStateByKind(0x49);
        if (npc == 0) {
            return;
        }
        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info == 0) {
            return;
        }
        if (!monster_info->p3D->IsCycleInterruptable(monster_info->p3D->m_pRep->pending_cycle)) {
            return;
        }
        StartMonsterCycle(monster_info, 0x14, 1);
        return;
    case W8_FACT_QUEST_SAVANT_BOMB_DEACTIVATED:
        npc = GetNpcStateByKind(0x57);
        if (npc == 0) {
            return;
        }
        if (npc->greeting_pending) {
            return;
        }
        ReleaseNpcMonsterByKind(0x57);
        return;
    case W8_FACT_UMPANI_BALBRAK_INTRO_DISAPPEAR:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_BALBRAK_HOME, 0);
        return;
    case W8_FACT_UMPANI_MOVE_RUBBLE:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_RUBBLE, 0);
        return;
    case W8_FACT_QUEST_KILL_ALSEDEXUS:
        if (value != 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SPACER, 0);
        return;
    case W8_FACT_ALSEDEXUS_DISAPPEAR:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_SEDEXUS_LEAVES, 0);
        return;
    case W8_FACT_ALSEDEXUS_DISAPPEARS_RIFT:
        if (value == 0) {
            return;
        }
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_SEDEXUS_RIFT, 0);
        return;
    case W8_FACT_MOVE_TO_BOOK2:
        QueueNpcMessageLine(W8_NPC_MSG_MOVE_TO_BOOK2, 0);
        return;
    case W8_FACT_CC_REMOVE_RPC_VI:
        QueueNpcMessageLine(W8_NPC_MSG_REMOVE_RPC_VI, 0);
        return;
    case W8_FACT_ENDGAME_JOIN_SAVANT:
        if (value == 0) {
            return;
        }
        g_status.vi_event_stage = 1;
        group = FindFirstMonsterByID(0x234);
        if (group != 0) {
            SetMonsterGroupHostility(group, 2, false);
        }
        group = FindFirstMonsterByID(0x1b4);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, false);
        }
        group = FindFirstMonsterByID(0x1b9);
        if (group != 0) {
            SetMonsterGroupHostility(group, 1, false);
        }
        GetNpcStateByKind(0x89);
        return;
    case W8_FACT_FACTION_RAPAX_TEMPLAR_FRIENDLY:
        if (value == 0) {
            SetFactionDispositionBand(0x10, 0);
            SetFactionDispositionBand(0xf, 0);
            return;
        }
        SetFactionDispositionBand(0x10, 2);
        SetFactionDispositionBand(0xf, 2);
        return;
    case W8_FACT_ENDGAME_QUE_CONGRAT_TWO:
    case W8_FACT_ENDGAME_QUE_CONGRAT_THREE:
        QueueNpcMessageLine(W8_NPC_MSG_BEGIN_ENDGAME, 0);
        return;
    case W8_FACT_ENDGAME_QUE_CONGRAT_FOUR:
        EndNpcDialogueSession(false);
        ResetLevelDataVectors();
        group = FindFirstMonsterByID(0xc2);
        if (group != 0) {
            monster_info = MonsterGetScriptPartByLocationIndex(MonsterGetIndexByLocationID(
                0x4b4, NPC_SCRIPTING_FACTS_CPP, group->leader_location_id, true));
            monster_info->p3D->SetScript("MoveSavantBoffo.msf", true);
        }
        SoundPlayStreamedFile("Data\\Sound\\Misc\\Earthquake End.wav", 0);
        shake = CreateCameraShakeEffect(6.0f, false, 1.0f, 0, 0);
        shake->flags |= 0x20;
        shake->completion_callback = ReplayEarthquakeShake;
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
    if ((record->flags & W8_MONSTER_FLAG_NPC) != 0) {
        W8NpcState* npc = GetNpcStateByKind(record->npc_kind);
        if (npc != 0) {
            npc->spawned = 1;
            if (npc->name_style == W8_NPC_VI_DOMINA) {
                wchar_t display_value[16];
                unsigned char fact_ok = GetFact(W8_FACT_VI_RESCUED);
                if (fact_ok == 0) {
                    fact_ok = GetFact(W8_FACT_MYLES_MISSION_RESCUE_VI_ASSIGNED);
                    if (fact_ok == 0) {
                        npc->spawned = 0;
                    }
                }
            }
        }
    }
    if (record->record_id != 0x234) {
        return;
    }
    int lead_index = -1;
    if (NpcLeadHasNameStyle(W8_NPC_VI_DOMINA)) {
        W8NpcState* lead = GetNpcStateByKind(W8_NPC_VI_DOMINA);
        if (lead != 0) {
            lead_index = lead->group_index;
        }
    }
    BeginScriptedWorldAction();
    int eligible_slots[8];
    /* 0x00508F08 and 0x00508F58 test the eligible count unsigned. */
    unsigned int eligible_count = 0;
    int slot;
    unsigned int index;
    int pick;
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        if (g_status.buffers.XChar[slot].fOccupied && character->hp_current != 0 &&
            character->highest_condition < W8_CONDITION_ASLEEP) {
            eligible_slots[eligible_count] = slot;
            ++eligible_count;
        }
    }
    for (index = 0; index < eligible_count; ++index) {
        if (eligible_slots[index] == lead_index) {
            QueueCharacterEvent(&g_status.buffers.Char[eligible_slots[index]], g_effect24,
                                g_character_event_no_npc_defer, g_character_event_no_flags,
                                g_character_event_full_volume);
        }
    }
    if (eligible_count > 2) {
        do {
            pick = Random(eligible_count);
        } while (eligible_slots[pick] == lead_index);
        QueueCharacterEvent(&g_status.buffers.Char[eligible_slots[pick]], g_effect24,
                            g_character_event_no_npc_defer, g_character_event_no_flags,
                            g_character_event_full_volume);
    }
    for (slot = 0; slot < 8; ++slot) {
        W8Character* character = &g_status.buffers.Char[slot];
        if (g_status.buffers.XChar[slot].fOccupied && character->hp_current != 0 &&
            character->highest_condition < W8_CONDITION_ASLEEP) {
            QueueCharacterEvent(character, g_effect28, g_character_event_no_npc_defer,
                                g_character_event_no_flags, g_character_event_full_volume);
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
        if (entry->fActive && entry->ubDisposition == W8_DISPOSITION_HOSTILE &&
            !entry->p3D->IsDying()) {
            TintHighlightedMonster(entry->p3D, W8_TARGET_HIGHLIGHT_NONE);
            MonsterStartsDying(entry, 1);
        }
    }
}

/* Scripted kill reactions keyed by monster record id: facts and faction
   changes for the special kills, the Rattkin breeder location-variable count,
   and the 0x22b cleanup that clears the victim's condition and queues the
   death event on the recorded party slot. */
// FUNCTION: WIZ8 0x005090C0
void MonsterKilled(int record_id, int killer_party_slot)
{
    unsigned char value;

    if (record_id == 0x131) {
        SetFact(W8_FACT_CROCK_BREKEK_DEAD, 1, false);
        return;
    }
    if (record_id == 0x1a9) {
        SetFact(W8_FACT_QUEEN_IS_DEAD, 1, false);
        ApplyFactionChange(2, 1, 0x10, 0x14);
        return;
    }
    if (record_id == 0x14f || record_id == 0xdd) {
        if (GetLocationVarIDByName("NumberRattkinBreedersKilled") == -1) {
            CreateLocationVar("NumberRattkinBreedersKilled", 1);
            return;
        }
        SetTriggerVariableByName("NumberRattkinBreedersKilled", 2);
        SetFact(W8_FACT_TRYNNIE_KILL_RATS_DONE, 1, false);
        SetFactionDispositionBand(7, 0);
    } else {
        if (record_id == 0x22b) {
            value = GetFact(W8_FACT_ALSEDEXUS_ATTACK);
            if (value != 0) {
                SetFact(W8_FACT_QUEST_KILL_ALSEDEXUS, 0, false);
            }
            if (g_status.rpc_active) {
                if (g_status.buffers.Char[g_status.sedexus_party_slot]
                        .uiCondition[W8_CONDITION_INFATUATED] > 0) {
                    RemoveCharacterCondition(g_status.sedexus_party_slot, W8_CONDITION_INFATUATED,
                                             false);
                }
                QueueCharacterEvent(&g_status.buffers.Char[g_status.sedexus_party_slot],
                                    g_effect37, 0, g_character_event_no_flags,
                                    g_character_event_full_volume);
            }
            SetFact(W8_FACT_RAPAX_ALSEDEXUS_DEAD, 1, false);
            return;
        }
        if (record_id == 0x181) {
            SetFact(W8_FACT_KING_IS_DEAD, 1, false);
            return;
        }
        if (record_id == 0x175 || record_id == 0x222) {
            SetFact(W8_FACT_LAVALORD_DIE, 1, false);
            return;
        }
    }
}

/* Original translation-unit ownership of these helpers is unknown. */

// GLOBAL: WIZ8 0x00689b78
unsigned char g_fact_values[1000];

// FUNCTION: WIZ8 0x00506280
unsigned char GetFact(W8FactId fact_id)
{
    unsigned char value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return 0;
    }

    value = EvaluateFact(fact_id);
    if (g_status.log_fact_checks) {
        if (value) {
            wcscpy(display_value, L"TRUE");
        } else {
            wcscpy(display_value, L"FALSE");
        }
        ShowNoticef(W8_FONT_PALETTE_YELLOW, L"Checking fact %S which is %s",
                    g_fact_records[fact_id].symbolic_name, display_value);
    }
    return value;
}

// FUNCTION: WIZ8 0x005061a0
void SetFact(W8FactId fact_id, unsigned char value, bool suppress_side_effects)
{
    unsigned char previous_value;
    wchar_t display_value[10];

    if (fact_id > 1000) {
        return;
    }

    previous_value = g_fact_values[fact_id];
    g_fact_values[fact_id] = value;

    if (fact_id < static_cast<int>(gXStatus.uiFactsInDatabase)) {
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

        if (g_status.log_fact_checks) {
            if (value) {
                wcscpy(display_value, L"TRUE");
            } else {
                wcscpy(display_value, L"FALSE");
            }
            ShowNoticef(W8_FONT_PALETTE_YELLOW, L"%S set to %s",
                        g_fact_records[fact_id].symbolic_name, display_value);
        }
    }
}

/* The whole 1001-byte fact array minus its last entry goes to the save file in
   one write. Retail passes the handle's own dead stack slot as the
   bytes-written out-parameter; that is VC6 reusing the slot for this local. */
// FUNCTION: WIZ8 0x00506480
void SaveFactState(int save_handle)
{
    unsigned int bytes_written;

    FileWrite(save_handle, g_fact_values, 1000, &bytes_written);
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
    SetFactNotificationsSuppressed(true);
    if (g_status.skip_loose_character_check) {
        SetFact(W8_FACT_IMPORT, 1, false);
        switch (g_wiz7_ending) {
        case 1:
            SetFact(W8_FACT_IMPORT_UMPANI, 1, false);
            break;
        case 2:
            SetFact(W8_FACT_IMPORT_TRANG, 1, false);
            break;
        default:
            SetFact(W8_FACT_IMPORT_NOALIGN, 1, false);
            break;
        }
        if (g_import_flags[0xb]) {
            SetFact(W8_FACT_BARLONE_WAS_DEAD, 1, false);
        }
        if (g_import_flags[5]) {
            SetFact(W8_FACT_RODAN_WAS_DEAD, 1, false);
            SetFactNotificationsSuppressed(false);
            return;
        }
    } else {
        SetFact(W8_FACT_VIRGIN, 1, false);
        SetFact(W8_FACT_QUEST_VIRGIN_1, 1, false);
        SetFact(W8_FACT_QUEST_VIRGIN_2, 1, false);
        SetFact(W8_FACT_QUEST_VIRGIN_3, 1, false);
    }
    SetFactNotificationsSuppressed(false);
}
