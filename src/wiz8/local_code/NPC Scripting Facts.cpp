#include "wiz8/fact_state.h"

#include "soundman.h"
#include "wiz8/character.h"
#include "wiz8/combat_state.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/game_status.h"
#include "wiz8/item_spawning.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/character_events.h"
#include "wiz8/location_variables.h"
#include "wiz8/magic.h"
#include "wiz8/npc_state.h"
#include "wiz8/record_file_0055a480.h"
#include "wiz8/sr_api.h"
#include "wiz8/string_database.h"
#include "wiz8/targeting.h"
#include "wiz8/utility.h"

#include <string.h>
#include <wchar.h>

#define NPC_SCRIPTING_FACTS_CPP "C:\\Projects\\Wizardry 8\\Local Code\\NPC Scripting Facts.cpp"

/* Local Code\NPC Scripting Facts.cpp. Direct assertion ownership. */

/* SOUNDPARMS EOS callback adapter: retail stores this JMP thunk rather than
   ClearFlag68C4F7's void() entry. */
// FUNCTION: WIZ8 0x005092c0
static void ClearPotionExplosionSoundFlag(void*)
{
    ClearFlag68C4F7();
}

/* Camera-shake completion callback stored on W8CameraShakeEffect::value_48. */
// FUNCTION: WIZ8 0x005092d0
static void ReplayEarthquakeShake(void)
{
    CreateCameraShakeEffect004AE080(3.0f, 0, 1.0f, 0, 0);
    Function5A6580();
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
        Function50C440(npc, 0);
        Function50C1C0(npc->name_style, 0x11, "NP_MylesCell");
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
        Function5289B0(0x2b, 0);
        return;
    case 0x33:
        if (value != 0 && FindEntityByName("NP_BlueFlowers", &position, 0, 0) != 0) {
            world_item = SpawnItem(0x2eb, &position, 3, 1);
            if (world_item != 0) {
                Function4F6CF0(world_item);
            }
            world_item = SpawnItem(0x2eb, &position, 3, 1);
            if (world_item != 0) {
                Function4F6CF0(world_item);
            }
        }
        return;
    case 0x37:
        fact_value = EvaluateFact(0x36);
        if (g_status_685170.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            WriteGameLog(5, L"Checking fact %S which is %s", g_fact_records[0x36].symbolic_name,
                         display_value);
        }
        if (fact_value == 0 && FindEntityByName("Brekek", &position, 0, 0) != 0) {
            SpawnMonsters(0x131, 1, &position, 1, 1, 1, 0);
        }
        return;
    case 0x39:
        if (value != 0 && g_status_685170.buffers.characters[g_status_685170.party_slot_249c]
                                  .condition_turns[0x13] != 0) {
            RemoveCharacterCondition(g_status_685170.party_slot_249c, 0x13, 1);
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
                ReleaseRecordFile0055A0A0(npc->record_file);
                Function524CA0(npc);
                if (!npc->is_grouped) {
                    SetFlag68C4F4();
                    Function528830(0, 0, 0, 0);
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
                character = &g_status_685170.buffers.characters[slot];
                if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                    character->highest_condition < 0x12 &&
                    g_profession_skill_availability[7][character->current_profession] != 0 &&
                    character->skills[7].value_02 < 10) {
                    character->skills[7].value_02 = 10;
                    Function553C10(character, 7);
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
        trigger->flags_0a0 &= ~0x100u;
        return;
    case 0xb6:
        if (value != 0) {
            SetFact(0x308, 1, 0);
        }
        return;
    case 0xb7:
        if (value != 0) {
            Function5289B0(0x2e, 0);
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
        if (g_status_685170.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            WriteGameLog(5, L"Checking fact %S which is %s", g_fact_records[0xd1].symbolic_name,
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
        monster_info->monster->SetScript004C7F10("Guard.msf", 1);
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
            trigger->flags_0a0 &= ~0x100u;
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
        Function5289B0(0x12, 0);
        return;
    case 0x151:
        trigger = FindTriggerByName("ChaosBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x100u;
        }
        trigger = FindTriggerByName("ChaosDoor01");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        Function5289B0(0x3e, 0);
        return;
    case 0x153:
        trigger = FindTriggerByName("KnowBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x100u;
        }
        trigger = FindTriggerByName("DoorKnow03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        Function5289B0(0x40, 0);
        return;
    case 0x155:
        trigger = FindTriggerByName("LifeBTrigger");
        if (trigger != 0) {
            trigger->flags_0a0 &= ~0x100u;
        }
        trigger = FindTriggerByName("DoorLife03");
        if (trigger != 0) {
            trigger->Run(-1);
        }
        Function5289B0(0x3f, 0);
        return;
    case 0x157:
        Function5289B0(0xf, 0);
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
        Function5289B0(0x22, 0);
        return;
    case 0x173:
        if (value == 0) {
            return;
        }
        Function5289B0(0x23, 0);
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
            character = &g_status_685170.buffers.characters[slot];
            if (g_status_685170.buffers.party_rows[slot].occupied != 0 &&
                character->hp_current != 0 && character->highest_condition < 0x12) {
                added = 100 - character->attributes[1].value;
                if (added > 5) {
                    added = 5;
                }
                character->attributes[1].value += added;
                Function553AD0(character, 1);
            }
        }
        ShowString(gppStringList[0x770 / 4]);
        trigger = FindTriggerByName("FOUNT_RIDDLE");
        if (trigger == 0) {
            return;
        }
        trigger->flags_0a0 &= ~0x100u;
        return;
    case 0x197:
        Function5289B0(0x25, 0);
        return;
    case 0x19b:
        if (value == 0) {
            return;
        }
        SetFact(0x273, 1, 0);
        return;
    case 0x19c:
        Function5289B0(0x24, 0);
        return;
    case 0x19e:
        if (value == 0) {
            return;
        }
        AddNpcItemWithDelay(GetNpcStateByKind(0x49), 0x1b0, 1, 0x15180);
        return;
    case 0x1a0:
        Function5289B0(0x10, 0);
        return;
    case 0x1a5:
        if (value == 0) {
            return;
        }
        Function5289B0(0x26, 0);
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
        Function5289B0(0x1c, 0);
        return;
    case 0x1b4:
        if (value == 0) {
            return;
        }
        Function5289B0(0x1a, 0);
        return;
    case 0x1b5:
        if (value == 0) {
            return;
        }
        Function5289B0(0x1b, 0);
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
        Function529BE0();
        return;
    case 0x1c2:
        if (value == 0) {
            Function5289B0(0x41, 0);
            return;
        }
        Function529EF0();
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
        Function5289B0(0x13, 0);
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
        Function5289B0(0x39, 0);
        return;
    case 0x1f2:
        Function5289B0(0x20, 0);
        return;
    case 0x203:
        if (value == 0) {
            return;
        }
        position.x = 53550.0f;
        position.y = 3516.0f;
        position.z = 36936.0f;
        Function420F90(&position);
        fact_value = EvaluateFact(0x177);
        if (g_status_685170.log_fact_checks_3120) {
            if (fact_value == 0) {
                wcscpy(display_value, L"FALSE");
            } else {
                wcscpy(display_value, L"TRUE");
            }
            WriteGameLog(5, L"Checking fact %S which is %s", g_fact_records[0x177].symbolic_name,
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
        Function5289B0(0x16, 0);
        return;
    case 0x208:
        Function5289B0(0x15, 0);
        return;
    case 0x20f:
        Function5289B0(0x30, 0);
        return;
    case 0x213:
        Function5289B0(0x3d, 0);
        return;
    case 0x214:
        if (value == 0) {
            return;
        }
        Function5289B0(0x2f, 0);
        return;
    case 0x215:
        Function5289B0(0x32, 0);
        return;
    case 0x219:
        g_status_685170.value_498f = 1;
        return;
    case 0x21b:
        Function5289B0(0x34, 0);
        return;
    case 0x21c:
        g_status_685170.value_4993 = 1;
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
            SetTriggerVariableByName00444030("ObstacleDoors",
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
            SetTriggerVariableByName00444030("ObstacleDoors",
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
        Function5289B0(0x21, 0);
        return;
    case 0x22b:
        if (value == 0) {
            return;
        }
        MarkNpcOfKind(0x18);
        Function56E800(0);
        return;
    case 0x234:
    case 0x235:
        memset(&sound, -1, sizeof(sound));
        sound.EOSCallback = ClearPotionExplosionSoundFlag;
        if (SoundPlay("Data\\Sound\\misc\\potion exploding.wav", &sound) != 0xffffffff) {
            SetFlag68C4F7();
        }
        particle = FindParticleByName(g_world, "FuzzBlast");
        if (particle != 0) {
            particle->SetActive(0);
            particle->start_frame_264 = 0;
            particle->value_188 = 0;
            particle->SetActive(1);
            particle->SetTraversalEnabled00498D90(1);
        }
        npc = GetNpcStateByKind(0x49);
        if (npc == 0) {
            return;
        }
        monster_info = GetNpcMonsterInfo(npc);
        if (monster_info == 0) {
            return;
        }
        if (monster_info->monster->IsCycleInterruptable(
                monster_info->monster->m_pRep->pending_cycle) == 0) {
            return;
        }
        StartMonsterCycle(monster_info, 0x14, 1);
        return;
    case 0x259:
        npc = GetNpcStateByKind(0x57);
        if (npc == 0) {
            return;
        }
        if (npc->unknown_1d != 0) {
            return;
        }
        ReleaseNpcMonsterByKind(0x57);
        return;
    case 0x27c:
        if (value == 0) {
            return;
        }
        Function5289B0(0x2d, 0);
        return;
    case 0x2a5:
        Function5289B0(0x29, 0);
        return;
    case 0x2a6:
        if (value != 0) {
            return;
        }
        Function5289B0(0x3b, 0);
        return;
    case 0x2ed:
        if (value == 0) {
            return;
        }
        Function5289B0(0x2a, 0);
        return;
    case 0x2ef:
        if (value == 0) {
            return;
        }
        Function5289B0(0x2c, 0);
        return;
    case 0x2f0:
        Function5289B0(0x31, 0);
        return;
    case 0x2f3:
        Function5289B0(0x33, 0);
        return;
    case 0x2f4:
        if (value == 0) {
            return;
        }
        g_status_685170.value_498b = 1;
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
        Function5289B0(0x35, 0);
        return;
    case 0x322:
        Function56E800(0);
        ResetLevelDataVectors0041F0D0();
        group = FindFirstMonsterByID(0xc2);
        if (group != 0) {
            monster_info = MonsterGetScriptPartByLocationIndex(
                MonsterGetIndexByLocationID(0x4b4, NPC_SCRIPTING_FACTS_CPP, group->value_9f, 1));
            monster_info->monster->SetScript004C7F10("MoveSavantBoffo.msf", 1);
        }
        SoundPlayStreamedFile("Data\\Sound\\Misc\\Earthquake_End.wav", 0);
        shake = CreateCameraShakeEffect004AE080(6.0f, 0, 1.0f, 0, 0);
        shake->flags_00 |= 0x20;
        shake->value_48 = reinterpret_cast<int>(
            ReplayEarthquakeShake); /* reinterpret-ok: shake completion callback stored as int */
        return;
    default:
        return;
    }
}
