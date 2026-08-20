#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/game_state.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_tables.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"
#include "Random.h"
#include "surrender/srCore.h"
#include "surrender/srMath.h"
#include "surrender/srScene.h"

#include <windows.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Engine Code\Trigger.cpp.
 *
 * Trigger owns the action state reconstructed by the level loader and drives
 * it from Run. Its registry support is the ordinary srClassSupport template.
 */

extern W8GrowableVector<W8TriggerEvent*> g_timed_events_006599b8;
extern unsigned char g_flag_6081e4;
extern void ApplyRolledHealthChangeToParty(
    const W8Dice* dice, int argument_2, int argument_3);
extern float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);
extern int PlaySound00408860(const char* path, int* options);
extern unsigned char g_master_ambient_volume_6850f6;
extern W8Navigator* g_startup_world_659c0c;
extern unsigned int FindMonsterLocationsInBox0042F280(
    int** locations, const srVector3T<float>* lower,
    const srVector3T<float>* upper, int kind, int excluded_location);
extern stLight* FindLightByName00445A10(
    const char* name, const srRuntimeClass* relative_to);
extern void SetWorldEnvironmentValue00483AE0(W8World* world, float value);
unsigned char g_trigger_action_active_006599c8;
srVector3T<float> g_trigger_action_scene_offset_006599ac;
extern char g_trigger_parse_buffer_00659908[256];
W8GrowableVector<int> g_location_variable_levels_006598e0;
W8GrowableVector<char*> g_location_variable_names_006598f8;
W8GrowableVector<int> g_location_variable_values_00659990;
extern unsigned char g_flag_00606994;
extern unsigned char RemovePartyItemByID005215D0(int item_id, char remove_all);
extern int OpenLockInteraction00587510(Trigger* trigger);
extern int OpenTrapInteraction0058A470(Trigger* trigger);
extern void* g_modal_owner_0068edd0;
extern unsigned char FindEntityByName(
    const char* name,
    srVector3T<float>* position,
    int* location_id,
    srVector3T<float>* direction);
extern void RequestLevelTransition005615F0(
    int location_id, int entrance, unsigned char show_message);
extern void* SpawnSpellEffect004AD080(
    const char* name, int animation, int value_1, int value_2);
extern void Function48F280(W8World* world, const char* name, int active);
extern unsigned char Function521060(
    int item_id, int value_1, int value_2, int value_3, int value_4);
extern void Function41C680(int interface_id, int state);
extern float GetWorldValue24(const void* world);
extern void PositionAmbientSoundByName0047A950(int unused, const char* name);
extern void StopAmbientSoundByName0047A9E0(int unused, const char* name);
extern void ToggleAmbientSoundByName0047AA70(int unused, const char* name);
extern void UpdateCameraView00450080(srCamera* camera, int mode);
extern void RestorePartyStaminaByDice(
    unsigned char count, unsigned char sides, short base);
extern void HealPartyByDice(
    unsigned char count, unsigned char sides, short base);
extern void RestorePartySpellPoints(int amount);
extern int PointCastSpell(
    float x, float y, float z, int spell_id, unsigned int power_level);
extern void RemoveAllConditionsFromParty(void);
extern void GetCameraPosition(srVector3T<float>* position);

extern int ApplyItemEffectToRandomCharacter0052E5C0(
    unsigned int item_id, int character_filter, int value_3, int value_4);
extern void AddPartyGold(int amount, char announce);
extern char PartyAttemptsToIdentifyItem(
    W8ItemInstance* item, int argument_2);
extern void MoveItem(
    W8ItemInstance* destination, W8ItemInstance* source,
    int argument_3, int argument_4);
extern unsigned char g_flag_0068506e;
extern int g_value_0068c520;
extern int g_value_0068c548;
extern int g_value_005ee59c;
extern int g_value_005ee5a0;
extern int g_value_005ed8c8;
static int g_next_trigger_id_006874c6;

Trigger* FindTriggerByName(const char* name)
{
    char* uppercase_name;
    Trigger* trigger = 0;

    if (name == 0) {
        return 0;
    }
    uppercase_name = (char*)malloc(strlen(name) + 1);
    if (uppercase_name != 0) {
        strcpy(uppercase_name, name);
        _strupr(uppercase_name);
        trigger = static_cast<Trigger*>(srCore.getRegistry()->find(
            Trigger::sGetClassNode(), uppercase_name, 0));
    }
    free(uppercase_name);
    return trigger;
}

// VTABLE: WIZ8 0x005ec12c
// class W8TriggerEvent

W8TriggerEvent::W8TriggerEvent()
    : action_004(-1), timer_008(), auxiliary_timer_02c(0), trigger_030(0),
      repeat_034(0), completed_035(0)
{
}

// SYNTHETIC: WIZ8 0x00440980
// W8TriggerEvent::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004409a0
W8TriggerEvent::~W8TriggerEvent()
{
}

class W8TriggerShakeEvent : public W8TriggerEvent {
public:
    W8TriggerShakeEvent();
    virtual void Update() override;

    W8CameraShakeEffect* effect_038;
    int intensity_03c;
    unsigned char reverse_040;
    unsigned char unknown_041[3];
};

static_assert(sizeof(W8TriggerShakeEvent) == 0x44,
              "W8TriggerShakeEvent_must_be_0x44");

// VTABLE: WIZ8 0x005ec140
// class W8TriggerShakeEvent

W8TriggerShakeEvent::W8TriggerShakeEvent()
    : effect_038(0), intensity_03c(1), reverse_040(0)
{
}

// FUNCTION: WIZ8 0x00444ec0
void W8TriggerShakeEvent::Update()
{
    if (effect_038 == 0) {
        float intensity = (float)intensity_03c / 1000.0f;

        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
        effect_038 = CreateCameraShakeEffect004AE080(
            auxiliary_timer_02c->m_duration_seconds, 0, intensity, 0, 0);
        effect_038->flags_00 &= ~2;
        if (reverse_040 != 0) {
            effect_038->flags_00 |= 0x10;
        }
    }

    if ((effect_038->flags_00 & 1) == 0) {
        delete effect_038;
        effect_038 = 0;
        if (trigger_030 != 0) {
            trigger_030->FinishAction();
        }
        if (repeat_034 != 0) {
            completed_035 = 1;
        }
    }
}

// FUNCTION: WIZ8 0x004447F0
void Trigger::CompleteItemInteraction004447F0()
{
    W8TriggerActionData* action_data = m_pActionData;
    state_370.state = 1;
    if (action_data != 0 && action_data->type_004 == 10) {
        action_data->flags_008 &= ~4;
    }
}

// FUNCTION: WIZ8 0x00444810
unsigned char Trigger::HasActorWithinRadius(
    float radius, unsigned char include_party)
{
    srVector3T<float> center;

    if (flag_0a0_11 != 0 || m_pProp != 0) {
        if (m_pProp == 0) {
            center.x = position_118;
            center.y = position_11c;
            center.z = position_120;
        }
        else {
            m_pProp->GetCenterPosition(&center);
        }

        srVector3T<float> lower;
        srVector3T<float> upper;
        int* locations = 0;
        lower.x = center.x - radius;
        lower.y = center.y - radius;
        lower.z = center.z - radius;
        upper.x = center.x + radius;
        upper.y = center.y + radius;
        upper.z = center.z + radius;

        unsigned int count = FindMonsterLocationsInBox0042F280(
            &locations, &lower, &upper, 0x0c, -1);
        for (unsigned int index = 0; index < count; ++index) {
            int location_id = locations[index];
            if (location_id == 0) {
                break;
            }
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x1246,
                "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                location_id, 1);
            W8MonsterInfo* monster_info =
                MonsterGetScriptPartByLocationIndex(monster_index);
            if (monster_info != 0 && monster_info->monster != 0) {
                srVector3T<float> monster_position =
                    monster_info->monster->GetPosition();
                float x = monster_position.x - center.x;
                float y = monster_position.y - center.y;
                float z = monster_position.z - center.z;
                if (sqrt(x * x + y * y + z * z) <= radius) {
                    return 1;
                }
            }
        }
    }

    if (include_party != 0) {
        srVector3T<float> party_position =
            g_startup_world_659c0c->GetPosition();
        float x = party_position.x - center.x;
        float y = party_position.y - center.y;
        float z = party_position.z - center.z;
        if (sqrt(x * x + y * y + z * z) <= radius) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004457c0
unsigned char Trigger::PlayActionSound(const char* sound_name, int volume)
{
    srVector3T<float> position;

    if (volume == 0) {
        volume = 100;
    }
    if (flag_0a0_11 == 0) {
        if (m_pProp == 0) {
            int options[8];
            for (int index = 0; index < 8; ++index) {
                options[index] = -1;
            }
            options[2] = (g_master_ambient_volume_6850f6 * volume) / 0x7f;
            PlaySound00408860(sound_name, options);
            return 0;
        }
        m_pProp->GetCenterPosition(&position);
    }
    else {
        position.x = position_118;
        position.y = position_11c;
        position.z = position_120;
    }

    stSound3D* sound = new stSound3D(sound_name, 0);
    if (sound != 0) {
        srVector3T<double> sound_position;
        sound_position.x = position.x;
        sound_position.y = position.y;
        sound_position.z = position.z;
        sound->value_140 = volume;
        sound->setLocation(sound_position);
        if (sound->Play004AEBF0(0, 1) != 0) {
            return 1;
        }
        sound->release();
    }
    return 0;
}

/* Timed actions pause their private timer with the game clock, then dispatch
   the small set of delayed Trigger effects once the timer completes. */
// FUNCTION: WIZ8 0x00444a00
void W8TriggerEvent::Update()
{
    if (action_004 == 2) {
        unsigned short flags = timer_008.m_flags;

        if (g_flag_6081e4 == 0) {
            if ((flags & 8) != 0 ||
                (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
                g_shared_timer_flag_d1 != 0) {
                return;
            }
            timer_008.m_flags = flags | 8;
            timer_008.m_start =
                timer_008.Method00439A60() - timer_008.m_start;
            return;
        }
        if ((flags & 8) != 0 ||
            (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
            g_shared_timer_flag_d1 != 0) {
            timer_008.m_flags = flags & ~8;
            timer_008.m_start =
                timer_008.Method00439A60() - timer_008.m_start;
            timer_008.SetDuration(-1.0f);
        }
    }

    if (timer_008.GetProgress() <= 1.0f ||
        trigger_030->HasActorWithinRadius(5000.0f, 1) != 0) {
        return;
    }

    switch (action_004) {
    case 2:
        if (trigger_030->m_pActionData != 0 &&
            trigger_030->m_pActionData->type_004 == 10 &&
            (trigger_030->m_pActionData->flags_008 & 1) != 0) {
            trigger_030->Run(-1);
        }
        break;

    case 0x0c: {
        if (trigger_030 != 0) {
            srVector3T<float> source;
            srVector3T<float> target;
            srVector3T<float> transformed;
            srVector3T<float> axis;
            srVector3T<float> row_1;
            srVector3T<float> row_2;
            srVector3T<float> row_3;
            srMatrix3T<float> rotation;

            source.x = trigger_030->position_118;
            source.y = trigger_030->position_11c;
            source.z = trigger_030->position_120;
            target = source;
            target.z += 100.0f;

            row_1.method_00421680(1.0, 0.0, 0.0);
            row_2.method_00421680(0.0, 1.0, 0.0);
            row_3.method_00421680(0.0, 0.0, 1.0);
            axis = row_3;
            rotation.vectors[0].x = trigger_030->value_100;
            rotation.vectors[0].y = trigger_030->value_104;
            rotation.vectors[0].z = trigger_030->value_108;
            rotation.vectors[1] = row_1;
            rotation.vectors[2] = row_2;

            if (trigger_030->angle_0fc != 0.0f) {
                RotateMatrixAroundAxis0042B910(
                    &rotation.vectors[0].x,
                    sin(trigger_030->angle_0fc),
                    cos(trigger_030->angle_0fc),
                    &axis.x);
            }

            transformed.x = Function4218E0(rotation.vectors[1], target);
            transformed.y = Function4218E0(rotation.vectors[2], target);
            transformed.z = Function4218E0(row_3, target);
            Function4A2D30(
                (unsigned int)trigger_030->m_lData1,
                &source, &transformed,
                0, 1, 1, 0x47435000);
        }
        break;
    }

    case 0x23: {
        W8Dice dice;
        SetDice(&dice, 1, 6, 2);
        ApplyRolledHealthChangeToParty(&dice, 0, 0);
        trigger_030->UpdateActionAnimation();
        break;
    }

    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
        trigger_030->value_35c = trigger_030->m_lData1;
        completed_035 = 1;
        break;

    case 0x3f: {
        Trigger* target = FindTriggerByName(trigger_030->m_pacRecipients);
        if (target != 0) {
            target->Run(-1);
        }
        break;
    }

    case 0x47: {
        char* name = trigger_030->m_pacRecipients;
        while (name != 0) {
            char buffer[256];
            strcpy(buffer, name);
            char* comma = strchr(buffer, ',');

            if (comma == 0) {
                name = 0;
            }
            else {
                name = strchr(name, ',') + 1;
                *comma = '\0';
            }
            stParticle* particle = FindParticleByName(g_world, buffer);
            if (particle != 0) {
                particle->trigger_flag_192 = 1;
                particle->SetActive(0);
            }
        }
        break;
    }

    default:
        break;
    }

    if (repeat_034 != 0) {
        completed_035 = 1;
    }
}

/* A state-driven Prop has one location variable per animation slot.  Ensure
   the complete set exists for the loaded level and select slot zero as the
   initial active state. */
// FUNCTION: WIZ8 0x00445200
void Function445200(Trigger* trigger)
{
    int slot;

    if (trigger->m_pacStateToMod == 0) {
        return;
    }
    for (slot = 0;
         slot < static_cast<signed char>(trigger->m_pProp->Rep()->slots.GetCount());
         ++slot) {
        char name[132];
        int variable_id;

        if (trigger->m_bRepType != 2) {
            srAssertFail(
                "m_bRepType == TRIGGER_REP_PROP",
                "..\\Engine Code\\Include\\Trigger.hpp", 0x3ed, 0);
        }
        sprintf(name, "%s%d", trigger->m_pacStateToMod, slot);
        if (name == 0) {
            srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1094, 0);
        }

        variable_id = 0;
        while (variable_id < g_location_variable_names_006598f8.GetCount()) {
            if (_stricmp(
                    *g_location_variable_names_006598f8.GetAt(variable_id),
                    name) == 0 &&
                *g_location_variable_levels_006598e0.GetAt(variable_id) ==
                    g_loaded_level_id) {
                break;
            }
            ++variable_id;
        }
        if (variable_id == g_location_variable_names_006598f8.GetCount()) {
            char* variable_name = new char[strlen(name) + 1];
            if (variable_name == 0) {
                srAssertFail(
                    "pacVariableName",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x109c, 0);
            }
            strcpy(variable_name, name);
            g_location_variable_names_006598f8.Add(variable_name);
            g_location_variable_values_00659990.Add(slot == 0);
            g_location_variable_levels_006598e0.Add(g_loaded_level_id);
        }
    }
}

// VTABLE: WIZ8 0x005ec138
// class W8TriggerActionData

W8TriggerActionData::W8TriggerActionData()
    : type_004(-1)
{
}

// SYNTHETIC: WIZ8 0x0043c7f0
// W8TriggerActionData::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00445ee0
W8TriggerActionData::~W8TriggerActionData()
{
}

// VTABLE: WIZ8 0x005ec148
// class W8TriggerActionData005EC148

// SYNTHETIC: WIZ8 0x00445ec0
// W8TriggerActionData005EC148::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ec134
// class W8TriggerActionData005EC134

// VTABLE: WIZ8 0x005ec158
// class W8TriggerActionData005EC158

// SYNTHETIC: WIZ8 0x00443730
// W8TriggerActionData005EC158::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00443750
W8TriggerActionData005EC158::~W8TriggerActionData005EC158()
{
    if (owned_string_008 != 0) {
        delete[] owned_string_008;
    }
}

// VTABLE: WIZ8 0x005ec0e4
// class Trigger

// FUNCTION: WIZ8 0x00441750
void Trigger::GetPosition(srVector3T<float>* position) const
{
    position->x = position_118;
    position->y = position_11c;
    position->z = position_120;
}

// VTABLE: WIZ8 0x005ec104
// class srClassSupport<Trigger,srClass,1,65544>

// FUNCTION: WIZ8 0x0043ba10
Trigger::Trigger()
{
    trigger_kind_018 = 0;
    trigger_id_09c = 0;
    flags_0a0 = 0;
    value_0a4 = 0;
    value_0a8 = 0;
    value_0ac = 0;
    value_0b0 = 0;
    value_0b1 = 0;
    value_0b2 = 0;
    value_0b3 = 0;
    value_0b4 = 0;
    value_0c8 = 0;
    angle_0fc = 0.0f;
    m_bRepType = 0;
    m_pProp = 0;
    value_114 = 0;
    m_pWorld = 0;
    initial_action_22a = 0;
    value_22c = 0;
    fallback_action_22e = 0;
    action_230 = 0;
    action_state_232 = 1;
    m_pActionData = 0;
    m_pacRecipients = 0;
    m_pacRequiredStates = 0;
    m_pacStateToMod = 0;
    m_pEvent = 0;
    world_item_group_34c = 0;
    flag_350 = 0;
    activation_callback_360 = 0;
    flag_364 = 0;

    value_0b8 = -1;
    value_229 = -1;
    value_23c = -1;
    value_37c = -1;
    value_380 = -1;
    value_388 = -1;
    value_368 = 0;
    value_36c = 0;
    state_370.state = 0;
    value_384 = 0;
    state_370.value_01 = 0;
    state_370.value_05 = 0;

    flags_0a0 |= 0x10;
    state_01c = 0;
    position_118 = 0.0f;
    position_11c = 0.0f;
    position_120 = 0.0f;
    action_data_128[0] = 0;
    next_activation_time_354 = GetTickCount() + Random(30000);
    gold_358 = 0;
    value_35c = 0;
    trigger_id_09c = g_next_trigger_id_006874c6++;
}

// FUNCTION: WIZ8 0x00440d00
void Trigger::UpdateActionAnimation()
{
    unsigned char* action_data = action_data_128;

    if (flag_0a0_01 == 0 && flag_0a0_23 == 0) {
        return;
    }
    if (flag_0a0_23 != 0) {
        if (action_data_mode_228 == 0) {
            if (flag_0a0_24 == 0) {
                flag_0a0_24 = 1;
            }
            else {
                action_data = alternate_action_data_1a8;
                flag_0a0_24 = 0;
            }
        }
        else if (action_data_mode_228 == 1) {
            if (action_230 == value_22c) {
                PlayActionSound(
                    (const char*)alternate_action_data_1a8, value_229);
                return;
            }
        }
        else if (action_data_mode_228 == 2 && action_230 == fallback_action_22e) {
            PlayActionSound(
                (const char*)alternate_action_data_1a8, value_229);
            return;
        }
    }
    PlayActionSound((const char*)action_data, value_229);
}

// FUNCTION: WIZ8 0x00441110
void Trigger::FinishAction()
{
    unsigned char was_running = flag_0a0_06;
    unsigned char action_completed = 0;
    char* recipient;

    flag_0a0_06 = 0;

    if (trigger_kind_018 == 1) {
        if (action_230 != 0x39) {
            goto reactivate_linked_triggers;
        }
        if (m_pEvent != 0) {
            m_pEvent->completed_035 = 1;
        }
        g_trigger_action_active_006599c8 = 0;
        goto finish_linked_triggers;
    }

    if (trigger_kind_018 != 2) {
        goto reactivate_linked_triggers;
    }

    switch (action_230) {
    case 0x0c:
        if (m_pEvent != 0 && m_lData2 > 0) {
            m_pEvent->completed_035 = 1;
        }
        action_completed = 1;
        break;

    case 0x23: {
        int index = g_timed_events_006599b8.IndexOf(m_pEvent);
        if (index >= 0) {
            g_timed_events_006599b8.RemoveAt(index);
        }
        action_completed = 1;
        break;
    }

    case 0x39:
        if (m_pEvent != 0) {
            m_pEvent->completed_035 = 1;
        }
        g_trigger_action_active_006599c8 = 0;
        action_completed = 1;
        break;
    }

    if (flag_0a0_03 == 0 && action_230 != 0) {
        if (action_230 == 4) {
            recipient = m_pacRecipients;
            action_completed = 0;
            while (recipient != 0) {
                strcpy(g_trigger_parse_buffer_00659908, recipient);
                char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
                if (comma == 0) {
                    recipient = 0;
                }
                else {
                    recipient = strchr(recipient, ',') + 1;
                    *comma = '\0';
                }

                stLight* light = FindLightByName00445A10(
                    g_trigger_parse_buffer_00659908, 0);
                if (light != 0) {
                    light->m_positional_23a = 1;
                    if (light->testFlag(srNode::FLAG_POSITIONAL_0) == 0) {
                        light->setFlag(srNode::FLAG_POSITIONAL_0);
                    }
                    else {
                        light->clearFlag(srNode::FLAG_POSITIONAL_0);
                    }
                    action_completed = 1;
                }
            }
        }
        else if (action_230 == 0x22) {
            if (m_pActionData == 0) {
                srAssertFail(
                    "m_pActionData",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    2822,
                    "Trigger.cpp: Dark Area doesn't have action data");
            }
            SetWorldEnvironmentValue00483AE0(
                g_world, m_pActionData->float_value_008);
            delete m_pActionData;
            m_pActionData = 0;
            goto finish_linked_triggers;
        }

        if (action_completed == 0) {
            goto reactivate_linked_triggers;
        }
    }

finish_linked_triggers:
    if (flag_0a0_07 != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer_00659908, recipient);
            char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
            if (comma == 0) {
                recipient = 0;
            }
            else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(
                g_trigger_parse_buffer_00659908);
            if (trigger != 0) {
                trigger->FinishAction();
            }
        }
    }

reactivate_linked_triggers:
    if (was_running != 0 && flag_0a0_04 != 0 && flag_0a0_21 != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer_00659908, recipient);
            char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
            if (comma == 0) {
                recipient = 0;
            }
            else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(
                g_trigger_parse_buffer_00659908);
            if (trigger != 0) {
                trigger->Run(-1);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00445480
static char* NextTriggerRecipient(char** cursor)
{
    char* comma;

    if (*cursor == 0) {
        return 0;
    }
    strcpy(g_trigger_parse_buffer_00659908, *cursor);
    comma = strchr(g_trigger_parse_buffer_00659908, ',');
    if (comma != 0) {
        *cursor = strchr(*cursor, ',') + 1;
        *comma = '\0';
    }
    else {
        *cursor = 0;
    }
    return g_trigger_parse_buffer_00659908;
}

// FUNCTION: WIZ8 0x004409b0
void Trigger::CommitActionResult(unsigned char apply_state_changes)
{
    char* recipient;

    UpdateActionAnimation();

    if (m_pacRecipients != 0 && flag_0a0_07 != 0 && flag_0a0_09 == 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            Trigger* trigger = FindTriggerByName(
                NextTriggerRecipient(&recipient));
            if (trigger != 0) {
                unsigned char was_running = flag_0a0_06;
                flag_0a0_06 = 1;
                trigger->Run(m_lData1);
                flag_0a0_06 = was_running;
            }
        }
    }

    if (m_pacStateToMod != 0 && apply_state_changes != 0) {
        int state_id;
        int state_value;

        if (value_0b4 == 1) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    4297, 0);
            }
            state_value = 1;
        }
        else if (value_0b4 == 2) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    4297, 0);
            }
            state_value = 0;
        }
        else if (value_0b4 == 3) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    4317, 0);
            }
            state_value =
                *g_location_variable_values_00659990.GetAt(state_id) == 0;
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    4297, 0);
            }
        }
        else {
            goto show_action_message;
        }
        g_location_variable_values_00659990.SetAt(state_id, state_value);
    }

show_action_message:
    if (flag_0a0_17 != 0) {
        const char* level_folder = GetLevelFolderName(GetLoadedLevelID());
        int message_id;

        if (action_state_232 == 2) {
            message_id = m_lData1;
        }
        else if (action_state_232 == 3) {
            message_id = m_lData2;
        }
        else if (action_state_232 == 4) {
            message_id = m_lData3;
        }
        else {
            goto action_complete;
        }

        if (message_id != -1) {
            char path[512];
            W8WideChar text[1996];

            if (level_folder == 0) {
                level_folder = "";
            }
            sprintf(path, "Data\\Messages\\%s.msg", level_folder);
            if (GetStringFromStringDatabase(
                    path, message_id, text, 0, 0) != 0) {
                ShowString(text);
            }
        }
    }

action_complete:
    flag_0a0_19 = 1;
}

/* Resolve an entity or a five-character level/entrance code, then either
   request the level transition or move the current world to the resolved
   destination. The destination Trigger supplies the local portal orientation
   when the name was not one of the world's named entities. */
// FUNCTION: WIZ8 0x00440dd0
void Trigger::RunDestination00440DD0(const char* destination)
{
    srVector3T<float> destination_position;
    srVector3T<float> destination_direction;
    srVector3T<float> source_position;
    srMatrix3T<float> rotation;
    int location_id;
    int entrance;
    int entity_value;
    int current_location;
    float angle;
    unsigned char named_entity;

    if (g_modal_owner_0068edd0 != 0) {
        return;
    }

    Function41EF50();
    current_location = g_current_level;
    named_entity = FindEntityByName(
        destination, &destination_position, &entity_value,
        &destination_direction);
    /* Retail leaves location_id and entrance uninitialised on the named-entity
       path. Both GOG builds then read the stack slot holding this for those
       values. Preserve that source bug rather than assigning entity_value and
       silently making the path behave differently. */
    if (named_entity == 0) {
        char location_code[4];
        char entrance_code[3];

        strncpy(location_code, destination, 3);
        location_code[3] = '\0';
        location_id = FindLevelIdByLocationCode(location_code);
        if (location_id == -1) {
            return;
        }
        strncpy(entrance_code, destination + 3, 2);
        entrance_code[2] = '\0';
        entrance = atoi(entrance_code);
    }
    Function41EF50();

    if (location_id != current_location) {
        RequestLevelTransition005615F0(
            location_id, entrance,
            m_lData1 < 0 ? 0 : (unsigned char)m_lData1);
        return;
    }

    if (named_entity == 0) {
        Trigger* target = FindTriggerByName(destination);

        destination_position.x = target->position_118;
        destination_position.y = target->position_11c;
        destination_position.z = target->position_120;
        angle = target->angle_0fc;
        destination_direction.x = target->value_100;
        destination_direction.y = target->value_104;
        destination_direction.z = target->value_108;
    }
    else {
        angle = 0.0f;
    }

    source_position.x = position_118;
    source_position.y = position_11c;
    source_position.z = position_120;
    g_octree_6598a4->AdjustPortalDestination(
        &destination_position, &source_position);
    SetWorldScenePosition004511D0(GetWorld(), &destination_position);

    rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
    rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
    rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);
    if (angle != 0.0f) {
        RotateMatrixAroundAxis0042B910(
            &rotation.vectors[0].x, sin(angle), cos(angle),
            &destination_direction.x);
    }
    ApplyCameraRotation(&rotation);
    SpawnSpellEffect004AD080("set_portal", 1, 0, 0);
}

/* Materialize this trigger's item table once. The two dice fields are the
   item-count and gold rolls at ItemTable record offsets 0x1cd and 0x1d5. */
// FUNCTION: WIZ8 0x00445500
void Trigger::GenerateItemGroup()
{
    W8GrowableVector<W8WorldItem*> items(5);
    srVector3T<float> position;
    unsigned int table_id;
    unsigned int maximum_items;
    int index;

    if (inline_action_data_24c[0] == '\0' || flag_350 != 0) {
        return;
    }

    srand(next_activation_time_354);
    table_id = FindItemTableByName(inline_action_data_24c);
    if (table_id == (unsigned int)-1) {
        return;
    }

    maximum_items = RollDice(&g_item_tables[table_id]->item_count_dice);
    GenerateItemsFromTable(&items, table_id, maximum_items);
    if (world_item_group_34c == 0) {
        world_item_group_34c = SpawnItem(0x23c, &position, 0, 0);
    }
    for (index = 0; index < items.GetCount(); ++index) {
        ItemInfoAddToGroup(world_item_group_34c, *items.GetAt(index));
    }
    gold_358 = RollDice(&g_item_tables[table_id]->gold_dice);
    flag_350 = 1;
}

/* Execute the selected Trigger action. The original keeps the three trigger
   kinds in one dispatcher: kind two handles invisible/timed actions first,
   kind one handles Prop-backed actions, and everything else falls through to
   the common action table. */
// FUNCTION: WIZ8 0x0043d940
void Trigger::Run(int source)
{
    ActivationCallback callback = activation_callback_360;
    unsigned char apply_state_changes = 1;
    unsigned char action_succeeded = 0;

    if (SelectAction() == 0) {
        return;
    }
    if (callback != 0 && callback(this) == 0) {
        return;
    }

    if (trigger_kind_018 == 2) {
        switch (action_230) {
        case 0:
            goto commit_action;

        case 9:
            if (m_pacRecipients == 0) {
                break;
            }
            Function48F280(m_pWorld, m_pacRecipients, 1);
            FinishAction();
            goto commit_action;

        case 0x0a: {
            int camera = -1;
            int id;
            int switch0;
            int switch1;
            int switch2;
            int switch3;
            int switch4;
            int switch5;
            int switch6;

            id = GetLocationVarIDByName("Switch0");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch0 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch1");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch1 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch2");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch2 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch3");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch3 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch4");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch4 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch5");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch5 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch6");
            if (id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10dd, 0);
            }
            switch6 = *g_location_variable_values_00659990.GetAt(id);

            if (switch0 == 1 && switch1 == 1) {
                camera = 2;
            }
            else if (switch0 == 0 && switch1 == 1) {
                if (switch4 == 0 && switch6 == 1) {
                    camera = 1;
                }
                else if (switch3 == 1 && switch4 == 1) {
                    camera = 3;
                }
                else if (switch4 == 0 && switch6 == 0) {
                    camera = 4;
                }
                else if (switch3 == 0 && switch4 == 1) {
                    camera = 5;
                }
            }
            else if (switch1 == 0) {
                if (switch2 == 1) {
                    if (switch3 == 1 && switch5 == 1) {
                        camera = 6;
                    }
                    else if (switch5 == 0 && switch6 == 0) {
                        camera = 7;
                    }
                    else if (switch3 == 0 && switch5 == 1) {
                        camera = 9;
                    }
                    else if (switch5 == 0 && switch6 == 1) {
                        camera = 10;
                    }
                }
                else if (switch2 == 0) {
                    camera = 8;
                }
            }

            if (camera != -1) {
                char name[24];

                sprintf(name, "Camera0%d", camera);
                Function48F280(m_pWorld, name, 1);
            }
            FinishAction();
            goto commit_action;
        }

        case 0x0e:
            if (m_pacRecipients == 0 ||
                _stricmp(m_pacRecipients, "party") != 0) {
                break;
            }
            ApplyItemEffectToRandomCharacter0052E5C0(
                Random(2) != 0 ? g_value_005ee59c : g_value_005ee5a0,
                -1, 0, g_value_005ed8c8);
            flag_0a0_06 = 1;
            goto commit_action;

        case 0x11:
            if (flag_0a0_06 != 0) {
                break;
            }
            flag_0a0_06 = 1;
            goto commit_action;

        case 0x22: {
            float previous_value = GetWorldValue24(g_world);

            delete m_pActionData;
            m_pActionData = new W8TriggerActionData005EC148;
            m_pActionData->type_004 = 5;
            m_pActionData->float_value_008 = previous_value;
            SetWorldEnvironmentValue00483AE0(g_world, 0.0f);
            flag_0a0_06 = 1;
            goto commit_action;
        }

        case 0x23:
            if (m_pEvent == 0) {
                m_pEvent = new W8TriggerEvent;
                if (m_pEvent == 0) {
                    srAssertFail(
                        "m_pEvent",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        0x5fa, 0);
                }
                m_pEvent->trigger_030 = this;
                m_pEvent->action_004 = (short)action_230;
                m_pEvent->timer_008.SetDuration(0.5f);
            }
            else {
                if (g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                    srAssertFail(
                        "glsTimedEvents.Find(m_pEvent) == -1",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        0x603, 0);
                }
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->auxiliary_timer_02c != 0) {
                m_pEvent->auxiliary_timer_02c->Restart();
            }
            g_timed_events_006599b8.Add(m_pEvent);
            flag_0a0_06 = 1;
            goto commit_action;

        case 0x34:
            if (m_pacRecipients == 0 || m_pacRecipients[0] == '\0') {
                break;
            }
            RunDestination00440DD0(m_pacRecipients);
            goto commit_action;

        default:
            break;
        }
    }

    if (trigger_kind_018 == 1) {
        switch (action_230) {
        case 1: {
            W8TriggerActionData005EC134* action_data = 0;

            if (m_bRepType != 2 || m_pProp == 0 || value_0b1 != 0 ||
                m_pProp->Rep()->flag_06d != 0) {
                break;
            }
            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                action_data =
                    static_cast<W8TriggerActionData005EC134*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->flags_008 & 4) != 0 &&
                action_data->item_00a != -1) {
                if (Function521060(
                        action_data->item_00a, 0, 2, 0, 0) == 0) {
                    return;
                }
                action_data->flags_008 &= ~4;
            }

            m_pProp->SetRepresentationActive(1, 1);
            value_0b1 = 1;
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
                Function41C680(value_0b8, 1);
            }
            flag_0a0_06 = 1;
            if (action_data != 0) {
                action_data->flags_008 |= 1;
            }

            if (m_lData1 != 0) {
                if (m_pEvent != 0 &&
                    g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                    m_pEvent->timer_008.Restart();
                    if (m_pEvent->auxiliary_timer_02c != 0) {
                        m_pEvent->auxiliary_timer_02c->Restart();
                    }
                    goto commit_action;
                }

                m_pEvent = new W8TriggerEvent;
                m_pEvent->trigger_030 = this;
                m_pEvent->action_004 = 2;
                m_pEvent->timer_008.SetDuration(
                    m_lData1 < 0 ? 10.0f : (float)m_lData1);
                m_pEvent->timer_008.Restart();
                m_pEvent->repeat_034 = 1;
                g_timed_events_006599b8.Add(m_pEvent);
            }
            goto commit_action;
        }

        case 2: {
            unsigned char active;

            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            active = m_pProp->Rep()->flag_06d;
            if (active != 0) {
                break;
            }
            value_0b1 = value_0b1 == 1 ? 0 : 1;
            m_pProp->SetRepresentationActive(value_0b1, 1);
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
                Function41C680(value_0b8, value_0b1);
            }
            flag_0a0_06 = 1;
            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                if (value_0b1 == 0) {
                    m_pActionData->flags_008 &= ~1;
                }
                else {
                    m_pActionData->flags_008 |= 1;
                }
            }
            goto commit_action;
        }

        case 8: {
            signed char previous = (signed char)value_0b1;

            if (value_0b0 > 1) {
                signed char next =
                    (signed char)value_0b1 + (signed char)value_0b2;
                value_0b1 = (unsigned char)next;
                if ((unsigned char)next == value_0b0) {
                    if (value_0b3 == 0) {
                        value_0b1 = 0;
                    }
                    else {
                        value_0b1 = value_0b0 - 2;
                        value_0b2 = 0xff;
                    }
                }
                else if (next < 0) {
                    value_0b1 = 1;
                    value_0b2 = 1;
                }
            }

            if (flag_0a0_00 != 0) {
                W8AnimObj* animation;
                unsigned int count;
                unsigned int index;

                if (m_pProp == 0 || m_bRepType != 2) {
                    srAssertFail(
                        "m_pProp && m_bRepType == TRIGGER_REP_PROP",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        0x592, 0);
                }
                animation = m_pProp->Rep()->animation;
                if (AnimationIsRunning(animation) == 1) {
                    count = AnimObjListCount004A1620(animation, 2);
                    for (index = 0; index < count; ++index) {
                        W8PathAI* path = static_cast<W8PathAI*>(
                            AnimObjListEntry004A16C0(
                                animation, 2, (signed char)index));
                        PathAIUpdate004A9260(
                            path, previous <= (signed char)value_0b1 ? 1 : -1);
                    }
                }
                else {
                    m_pProp->SetSetting66((char)value_0b1);
                }
            }
            goto commit_action;
        }

        case 0x2c: {
            W8TriggerActionData005EC134* action_data = 0;
            unsigned char was_active;

            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                action_data =
                    static_cast<W8TriggerActionData005EC134*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->flags_008 & 4) != 0 &&
                action_data->item_00a != -1) {
                if (Function521060(
                        action_data->item_00a, 0, 2, 0, 0) == 0) {
                    return;
                }
                action_data->flags_008 &= ~4;
            }
            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            was_active = m_pProp->Rep()->flag_06d;
            m_pProp->SetRepresentationActive(was_active == 0, 1);
            value_0b1 = value_0b1 == 0;
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
                Function41C680(value_0b8, value_0b1);
            }
            if (was_active == 0) {
                flag_0a0_06 = 1;
            }
            else {
                flag_0a0_06 = 0;
            }
            if (action_data != 0) {
                action_data->flags_008 =
                    (action_data->flags_008 & ~1) | (value_0b1 & 1);
            }
            goto commit_action;
        }

        case 0x37: {
            int tag = source == -1 ? m_lData1 : source;

            if (m_bRepType == 2 && m_pProp != 0 && tag != -1) {
                m_pProp->Rep()->SelectAnimationSlot((unsigned char)tag);
                m_pProp->SetRepresentationActive(1, 1);
                value_0b1 = (unsigned char)tag;
                goto commit_action;
            }
            break;
        }

        default:
            break;
        }
    }

    switch (action_230) {
    case 0:
    case 0x38:
        break;

    case 3: {
        unsigned char was_active;
        unsigned char action_succeeded = 1;

        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        was_active = m_pProp->Rep()->flag_06d;

        if (inline_action_data_24c[0] != '\0') {
            if (g_status_685170.item_in_hand_shown_235a != 0) {
                srVector3T<float> item_position;
                W8WorldItem* item = CreateWorldItem(
                    &g_status_685170.item_in_hand_235b, &item_position, 3, 0);

                if (item != 0) {
                    if (world_item_group_34c == 0) {
                        srVector3T<float> group_position;
                        world_item_group_34c = SpawnItem(
                            0x23c, &group_position, 0, 0);
                    }
                    ItemInfoAddToGroup(world_item_group_34c, item);
                }
                g_flag_00606994 = 1;
                return;
            }
            if (flag_0a0_25 != 0) {
                return;
            }

            g_flag_00606994 = 1;
            if (flag_350 == 0) {
                GenerateItemGroup();
            }
            if (world_item_group_34c != 0) {
                int item_count =
                    ItemInfoGetNumInGroup(world_item_group_34c) - 1;
                W8WorldItem* item;
                int contained_items = 0;

                if (item_count != 1 &&
                    m_pProp->Rep()->flag_064 != 0) {
                    action_succeeded = 0;
                }

                item = world_item_group_34c->next;
                while (item != 0) {
                    ++contained_items;
                    if (item->item.identified == 0) {
                        PartyAttemptsToIdentifyItem(&item->item, 0);
                    }
                    item = item->next;
                }
                if (contained_items > 1) {
                    g_flag_0068506e = 1;
                }

                if (gold_358 != 0) {
                    AddPartyGold(gold_358, 1);
                    gold_358 = 0;
                }

                if (item_count == 1) {
                    if (m_pProp->Rep()->flag_064 == 0) {
                        ApplyItemEffectToRandomCharacter0052E5C0(
                            Random(2) != 0 ? g_value_0068c548
                                           : g_value_0068c520,
                            -1, 0, g_value_005ed8c8);
                    }
                }
                else if (item_count == 2 &&
                         g_status_685170.item_in_hand_shown_235a == 0) {
                    item = world_item_group_34c->next;
                    MoveItem(&g_status_685170.item_in_hand_235b,
                             &item->item, 0, 1);
                    ItemInfoRemoveFromGroup(world_item_group_34c, item);
                    if (m_pProp->Rep()->flag_064 != 0) {
                        goto toggle_item_prop;
                    }
                }
                else {
                    flag_0a0_25 = 1;
                }

                if (action_succeeded == 0) {
                    return;
                }
            }
        }

toggle_item_prop:
        m_pProp->SetRepresentationActive(was_active == 0, 1);
        value_0b1 = value_0b1 == 0;
        if (m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
            Function41C680(value_0b8, value_0b1);
        }
        if (was_active == 0) {
            flag_0a0_06 = 1;
        }
        else {
            flag_0a0_06 = 0;
        }
        break;
    }

    case 4:
    case 0x30:
    case 0x31: {
        char* recipient = m_pacRecipients;

        while (recipient != 0) {
            stLight* light = FindLightByName00445A10(
                NextTriggerRecipient(&recipient), 0);
            if (light != 0) {
                light->m_positional_23a = 1;
                if (action_230 == 4) {
                    if (light->testFlag(srNode::FLAG_POSITIONAL_0) == 0) {
                        light->setFlag(srNode::FLAG_POSITIONAL_0);
                    }
                    else {
                        light->clearFlag(srNode::FLAG_POSITIONAL_0);
                    }
                }
                else if (action_230 == 0x30) {
                    if (light->testFlag(srNode::FLAG_POSITIONAL_0) != 0) {
                        light->clearFlag(srNode::FLAG_POSITIONAL_0);
                    }
                }
                else if (light->testFlag(srNode::FLAG_POSITIONAL_0) == 0) {
                    light->setFlag(srNode::FLAG_POSITIONAL_0);
                }
                action_succeeded = 1;
            }
        }
        if (trigger_kind_018 == 2) {
            flag_0a0_06 = 1;
        }
        if (action_succeeded == 0) {
            return;
        }
        break;
    }

    case 0x0b: {
        W8MonsterGroup* group = 0;
        W8MonsterInfo* monster_info = 0;
        int monster_id;
        int index;

        if (m_pacRecipients == 0) {
            return;
        }
        monster_id = atoi(m_pacRecipients);
        for (index = 0;
             index < (int)PLLength(g_monster_group_list);
             ++index) {
            group = static_cast<W8MonsterGroup*>(
                PLGet(g_monster_group_list, index));
            if (group->monster_id == monster_id) {
                int location_id = IListGetAt(group->monsters, 0);
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0x7aa,
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    location_id, 1);

                monster_info = MonsterGetScriptPartByLocationIndex(
                    monster_index);
                break;
            }
        }
        if (group == 0 || monster_info == 0) {
            return;
        }

        group->flag_28 = 1;
        monster_info->monster->m_pRep->flag_06d = 1;
        monster_info->monster->m_pRep->flag_06d = 1;
        monster_info->monster->m_pRep->timer_068 =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        monster_info->monster->ResetRepresentation004A7420();
        monster_info->monster->ResetPathAI();
        monster_info->monster->fields.unknown_09d[0] = 1;
        return;
    }

    case 0x0f: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            Trigger* target = FindTriggerByName(
                NextTriggerRecipient(&recipient));
            if (target != 0) {
                unsigned char was_running = flag_0a0_06;
                flag_0a0_06 = 1;
                target->Run(m_lData1);
                flag_0a0_06 = was_running;
                action_succeeded = 1;
            }
        }
        if (action_succeeded == 0) {
            return;
        }
        break;
    }

    case 0x0c:
        if (m_lData1 < 0) {
            return;
        }
        if (m_lData2 == 0) {
            srVector3T<float> source_position;
            srVector3T<float> target_position;
            srVector3T<float> transformed;
            srVector3T<float> axis;
            srMatrix3T<float> rotation;

            source_position.x = position_118;
            source_position.y = position_11c;
            source_position.z = position_120;
            target_position = source_position;
            target_position.z += 100.0f;
            rotation.vectors[0].method_00421680(1.0, 0.0, 0.0);
            rotation.vectors[1].method_00421680(0.0, 1.0, 0.0);
            rotation.vectors[2].method_00421680(0.0, 0.0, 1.0);
            axis = rotation.vectors[2];
            if (angle_0fc != 0.0f) {
                RotateMatrixAroundAxis0042B910(
                    &rotation.vectors[0].x, sin(angle_0fc), cos(angle_0fc),
                    &axis.x);
            }
            transformed.x = Function4218E0(
                rotation.vectors[0], target_position);
            transformed.y = Function4218E0(
                rotation.vectors[1], target_position);
            transformed.z = Function4218E0(
                rotation.vectors[2], target_position);
            Function4A2D30(
                (unsigned int)m_lData1, &source_position,
                &transformed, 0, 1, 1, 0x47435000);
        }
        else if (m_pEvent == 0) {
            float duration = (float)abs(m_lData2) * 0.001f;

            if (m_lData2 > 0 && trigger_kind_018 != 2) {
                srAssertFail(
                    "m_lData2 < 0 || m_iType == TRIGGER_INVISIBLE",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x87f,
                    "Continous firing missile must be invisible trigger.");
            }
            m_pEvent = new W8TriggerEvent;
            if (m_pEvent == 0) {
                srAssertFail(
                    "m_pEvent",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x883, 0);
            }
            m_pEvent->action_004 = (short)action_230;
            m_pEvent->timer_008.SetDuration(duration);
            m_pEvent->timer_008.Restart();
            m_pEvent->trigger_030 = this;
            flag_0a0_06 = 1;
            g_timed_events_006599b8.Add(m_pEvent);
        }
        break;

    case 0x10:
        UpdateCameraView00450080(
            m_pWorld->camera, source > 0 ? 1 : -1);
        return;

    case 0x24: {
        W8Dice dice;

        if (m_lData1 < 0) {
            m_lData1 = 1;
        }
        if (m_lData2 < 0) {
            m_lData2 = 6;
        }
        if (m_lData3 < 0) {
            m_lData3 = 2;
        }
        SetDice(
            &dice, (unsigned char)m_lData1, (unsigned char)m_lData2,
            (short)m_lData3);
        ApplyRolledHealthChangeToParty(&dice, 0, 1);
        if (trigger_kind_018 == 2) {
            flag_0a0_06 = 1;
        }
        break;
    }

    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b: {
        if (value_35c != 0 || m_lData1 == -1) {
            if (action_230 == 0x25) {
                RestorePartyStaminaByDice(0, 0, (short)m_lData3);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            }
            else if (action_230 == 0x26) {
                HealPartyByDice(0, 0, (short)m_lData3);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            }
            else if (action_230 == 0x27) {
                RestorePartySpellPoints(m_lData3);
                ShowString(gppStringList[0x1c88 / 4]);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            }
            else {
                int spell_id;
                srVector3T<double> position = g_world->camera->getLocation();

                if (action_230 == 0x28) {
                    spell_id = 0x44;
                }
                else if (action_230 == 0x29) {
                    spell_id = 0x45;
                }
                else if (action_230 == 0x2a) {
                    spell_id = 0x0c;
                }
                else {
                    spell_id = 0x2a;
                }
                PointCastSpell(
                    (float)position.x, (float)position.y, (float)position.z,
                    spell_id, (unsigned int)m_lData3);
                if (action_230 == 0x2b) {
                    RemoveAllConditionsFromParty();
                }
            }
        }

        if (value_35c == 0) {
            if (m_lData1 != -1) {
                return;
            }
        }
        else {
            --value_35c;
            if (m_lData2 > 0 && m_pEvent == 0) {
                m_pEvent = new W8TriggerEvent;
                if (m_pEvent == 0) {
                    srAssertFail(
                        "m_pEvent",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        0x84c, 0);
                }
                m_pEvent->action_004 = (short)action_230;
                m_pEvent->timer_008.SetDuration(m_lData2 * 720.0f);
                m_pEvent->timer_008.Restart();
                m_pEvent->trigger_030 = this;
                m_pEvent->timer_008.SetMode(1);
                g_timed_events_006599b8.Add(m_pEvent);
            }
        }
        if (trigger_kind_018 == 2) {
            flag_0a0_06 = 1;
        }
        break;
    }

    case 0x2d:
    case 0x2e:
    case 0x2f: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            Trigger* target = FindTriggerByName(
                NextTriggerRecipient(&recipient));
            if (target != 0) {
                if (action_230 == 0x2d) {
                    target->flag_0a0_04 = 1;
                }
                else if (action_230 == 0x2e) {
                    target->flag_0a0_04 = 0;
                }
                else {
                    target->flag_0a0_04 = target->flag_0a0_04 == 0;
                }
                action_succeeded = 1;
            }
        }
        if (action_succeeded == 0) {
            return;
        }
        break;
    }

    case 0x32:
    case 0x33:
        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        if ((action_230 == 0x32 &&
             m_pProp->Rep()->flag_06d != 0) ||
            (action_230 == 0x33 &&
             m_pProp->Rep()->flag_06d == 0)) {
            return;
        }
        m_pProp->SetRepresentationActive(action_230 == 0x32, 1);
        value_0b1 = value_0b1 == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
            Function41C680(value_0b8, value_0b1);
        }
        if (action_230 == 0x32) {
            flag_0a0_06 = 1;
        }
        else {
            flag_0a0_06 = 0;
        }
        break;

    case 0x36:
        if (action_state_232 == 4 && action_data_mode_228 == 2) {
            PlayActionSound((const char*)alternate_action_data_1a8, 0);
        }
        else {
            PlayActionSound((const char*)action_data_128, 0);
        }
        return;

    case 0x39: {
        srVector3T<float> party_position;
        W8TriggerShakeEvent* event;

        GetCameraPosition(&party_position);
        if (m_pEvent == 0) {
            event = new W8TriggerShakeEvent;
            m_pEvent = event;
            if (m_pEvent == 0) {
                srAssertFail(
                    "m_pEvent",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x7c4, 0);
            }
            event->trigger_030 = this;
            event->action_004 = (short)action_230;
            event->timer_008.SetDuration(
                m_lData2 == -1 ? 0.07f : m_lData2 * 0.001f);
            event->timer_008.Restart();
            event->intensity_03c = m_lData1 == -1 ? 800 : m_lData1;

            if (m_lData3 != -1) {
                delete event->auxiliary_timer_02c;
                event->auxiliary_timer_02c = new W8GameTimer;
                if (event->auxiliary_timer_02c == 0) {
                    srAssertFail(
                        "m_pCountdown",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        0x12de, 0);
                }
                event->auxiliary_timer_02c->SetDuration(
                    (float)abs(m_lData3) * 0.001f);
                event->auxiliary_timer_02c->Restart();
                if (m_lData3 < 0) {
                    event->reverse_040 = 1;
                }
            }
        }
        else {
            event = static_cast<W8TriggerShakeEvent*>(m_pEvent);
            event->timer_008.Restart();
            if (event->auxiliary_timer_02c != 0) {
                event->auxiliary_timer_02c->Restart();
            }
        }

        if (flag_0a0_06 == 0) {
            g_timed_events_006599b8.Add(m_pEvent);
            if (trigger_kind_018 == 2) {
                flag_0a0_06 = 1;
            }
            else if (m_lData3 == -1) {
                srAssertFail(
                    "m_lData3 != -1",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x7e4,
                    "Non invisible triggers with shake must have a duration.");
            }
        }
        break;
    }

    case 0x3a:
        if (m_pProp == 0 || source != m_lData1) {
            return;
        }
        m_pProp->SetSetting6C(0);
        break;

    case 0x3b:
        if (m_pProp == 0 || source != m_lData1 ||
            m_pProp->Rep()->flag_06d == 0) {
            return;
        }
        m_pProp->SetRepresentationActive(
            m_pProp->Rep()->flag_06d == 0, 1);
        value_0b1 = value_0b1 == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
            Function41C680(value_0b8, value_0b1);
        }
        break;

    case 0x3c:
        if (m_pProp == 0 || source != m_lData1) {
            return;
        }
        m_pProp->SetRepresentationActive(
            m_pProp->Rep()->flag_06d == 0, 1);
        value_0b1 = value_0b1 == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && value_0b8 >= 0) {
            Function41C680(value_0b8, value_0b1);
        }
        break;

    case 0x3d:
        if (m_pProp == 0) {
            return;
        }
        m_pProp->SetSetting6C(1);
        break;

    case 0x3e:
        if (m_pProp == 0) {
            return;
        }
        m_pProp->SetSetting6C(0);
        break;

    case 0x3f:
        if (m_pEvent == 0) {
            m_pEvent = new W8TriggerEvent;
            if (m_pEvent == 0) {
                srAssertFail(
                    "m_pEvent",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x8a4, 0);
            }
            m_pEvent->action_004 = (short)action_230;
            m_pEvent->timer_008.SetDuration(m_lData1 * 0.001f);
            m_pEvent->timer_008.Restart();
            m_pEvent->trigger_030 = this;
            m_pEvent->repeat_034 = 1;
        }
        else {
            if (g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                srAssertFail(
                    "glsTimedEvents.Find(m_pEvent) == -1",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x8ae, 0);
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->auxiliary_timer_02c != 0) {
                m_pEvent->auxiliary_timer_02c->Restart();
            }
        }
        g_timed_events_006599b8.Add(m_pEvent);
        break;

    case 0x40:
        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        if (m_pacStateToMod != 0) {
            char state_name[132];
            int state_id;

            sprintf(
                state_name, "%s%d", m_pacStateToMod,
                (int)(signed char)value_0b1);
            state_id = GetLocationVarIDByName(state_name);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10c9, 0);
            }
            g_location_variable_values_00659990.SetAt(state_id, 0);
        }
        value_0b1 = m_pProp->Rep()->AdvanceAnimationSegment();
        m_pProp->SetRepresentationActive(1, 0);
        if (m_pacStateToMod != 0) {
            char state_name[132];
            int state_id;

            sprintf(
                state_name, "%s%d", m_pacStateToMod,
                (int)(signed char)value_0b1);
            state_id = GetLocationVarIDByName(state_name);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    0x10c9, 0);
            }
            g_location_variable_values_00659990.SetAt(state_id, 1);
        }
        apply_state_changes = 0;
        break;

    case 0x41:
    case 0x42:
    case 0x43: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            const char* name = NextTriggerRecipient(&recipient);
            if (action_230 == 0x41) {
                PositionAmbientSoundByName0047A950((int)g_world, name);
            }
            else if (action_230 == 0x42) {
                StopAmbientSoundByName0047A9E0((int)g_world, name);
            }
            else {
                ToggleAmbientSoundByName0047AA70((int)g_world, name);
            }
            action_succeeded = 1;
        }
        break;
    }

    case 0x44:
    case 0x45:
    case 0x46: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            stParticle* particle = FindParticleByName(
                g_world, NextTriggerRecipient(&recipient));
            if (particle != 0) {
                particle->trigger_flag_192 = 1;
                if (action_230 == 0x44) {
                    particle->SetActive(1);
                }
                else if (action_230 == 0x45) {
                    particle->SetActive(0);
                }
                else {
                    particle->SetActive(particle->active_1a0 == 0);
                }
                action_succeeded = 1;
            }
        }
        if (action_succeeded == 0) {
            return;
        }
        break;
    }

    case 0x47: {
        char* recipient = m_pacRecipients;

        if (recipient == 0 || m_pEvent != 0) {
            return;
        }
        action_succeeded = 0;
        while (recipient != 0) {
            stParticle* particle = FindParticleByName(
                g_world, NextTriggerRecipient(&recipient));
            if (particle != 0) {
                particle->trigger_flag_192 = 1;
                particle->SetActive(1);
                action_succeeded = 1;
            }
        }
        if (action_succeeded == 0) {
            return;
        }

        m_pEvent = new W8TriggerEvent;
        if (m_pEvent == 0) {
            srAssertFail(
                "m_pEvent",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                0x940, 0);
        }
        g_timed_events_006599b8.Add(m_pEvent);
        m_pEvent->trigger_030 = this;
        m_pEvent->action_004 = (short)action_230;
        delete m_pEvent->auxiliary_timer_02c;
        m_pEvent->auxiliary_timer_02c = new W8GameTimer;
        if (m_pEvent->auxiliary_timer_02c == 0) {
            srAssertFail(
                "m_pCountdown",
                "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                0x12de, 0);
        }
        m_pEvent->auxiliary_timer_02c->SetDuration(
            m_lData1 == -1 ? 10.0f : m_lData1 * 0.001f);
        m_pEvent->auxiliary_timer_02c->Restart();
        m_pEvent->repeat_034 = 1;
        break;
    }

    case 0x48: {
        unsigned int count;

        if (m_pProp == 0 || m_lData1 < 0) {
            return;
        }
        count = AnimObjValue004A15D0(m_pProp->Rep()->animation, 2);
        if ((int)count <= m_lData1) {
            return;
        }
        m_pProp->SetSetting66((char)m_lData1);
        break;
    }

    case 0x49:
        if (m_pProp == 0 || m_lData1 < 0) {
            return;
        }
        m_pProp->SetAnimationSpeed((float)m_lData1);
        break;

    case 0x4a: {
        char* recipient = m_pacRecipients;

        if (recipient == 0 || m_lData1 < 0) {
            return;
        }
        while (recipient != 0) {
            W8Prop* prop = FindPropByName(
                g_world, NextTriggerRecipient(&recipient));
            if (prop != 0) {
                prop->SetAnimationSpeed((float)m_lData1);
            }
        }
        break;
    }

    case 0x4b: {
        char* recipient;
        unsigned char count = 0;
        unsigned char selected;
        unsigned char index = 0;

        if (m_pacRecipients == 0 || m_pacRecipients[0] == '\0') {
            return;
        }
        recipient = m_pacRecipients;
        while (recipient != 0) {
            NextTriggerRecipient(&recipient);
            ++count;
        }
        selected = (unsigned char)(GetTickCount() % count);
        recipient = m_pacRecipients;
        do {
            NextTriggerRecipient(&recipient);
            if (index == selected) {
                break;
            }
            ++index;
        } while (recipient != 0);
        RunDestination00440DD0(g_trigger_parse_buffer_00659908);
        break;
    }

    default:
        return;
    }

commit_action:
    if (flag_364 == 0) {
        g_flag_00606994 = 1;
    }
    CommitActionResult(apply_state_changes);
}

// FUNCTION: WIZ8 0x00444600
unsigned char Trigger::CanRunLinkedTriggers()
{
    char* recipient;

    if (m_pProp != 0 && m_pProp->Rep()->flag_06d != 0) {
        return 0;
    }
    recipient = m_pacRecipients;
    while (recipient != 0) {
        char* comma;
        Trigger* trigger;

        strcpy(g_trigger_parse_buffer_00659908, recipient);
        comma = strchr(g_trigger_parse_buffer_00659908, ',');
        if (comma != 0) {
            recipient = strchr(recipient, ',') + 1;
            *comma = '\0';
        }
        else {
            recipient = 0;
        }
        trigger = FindTriggerByName(g_trigger_parse_buffer_00659908);
        if (trigger != 0 && trigger->CanRunLinkedTriggers() == 0) {
            return 0;
        }
    }
    return 1;
}

// FUNCTION: WIZ8 0x0043d340
unsigned char Trigger::SelectAction()
{
    unsigned char fallback_selected = 0;
    unsigned char result = 1;

    if (g_flag_6081e4 == 0 && m_pActionData != 0 &&
        m_pActionData->type_004 == 10 &&
        (m_pActionData->flags_008 & 1) != 0) {
        return 0;
    }

    if ((flag_0a0_06 != 0 && action_230 != 0x39) || flag_0a0_04 == 0 ||
        (flag_0a0_18 != 0 && flag_0a0_19 != 0)) {
        action_state_232 = 1;
        return 0;
    }

    if (m_pacRequiredStates != 0) {
        if (strchr(m_pacRequiredStates, ',') == 0) {
            int state_id = GetLocationVarIDByName(m_pacRequiredStates);
            if (state_id == -1) {
                srAssertFail(
                    "iVar != BAD_INDEX",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                    4317, 0);
            }
            if (*g_location_variable_values_00659990.GetAt(state_id) == 0) {
                action_230 = fallback_action_22e;
                action_state_232 = 4;
                fallback_selected = 1;
            }
        }
        else {
            char* required_state = 0;
            unsigned char more_states = 1;
            char state_name[128];

            while (more_states != 0) {
                char* comma;
                int state_id;

                if (required_state == 0) {
                    required_state = m_pacRequiredStates;
                }
                else {
                    required_state = strchr(required_state, ',') + 1;
                }
                strcpy(state_name, required_state);
                comma = strchr(state_name, ',');
                if (comma == 0) {
                    more_states = 0;
                }
                else {
                    *comma = '\0';
                }

                state_id = GetLocationVarIDByName(state_name);
                if (state_id == -1) {
                    srAssertFail(
                        "iVar != BAD_INDEX",
                        "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                        4317, 0);
                }
                if (*g_location_variable_values_00659990.GetAt(state_id) == 0) {
                    action_230 = fallback_action_22e;
                    action_state_232 = 4;
                    fallback_selected = 1;
                    break;
                }
            }
        }
    }

    if (m_pActionData == 0 || m_pActionData->type_004 != 10) {
        if (value_23c >= 0) {
            if (GetItemInHand() == value_23c) {
                if (flag_0a0_16 != 0) {
                    RemovePartyItemByID005215D0(value_23c, 0);
                    value_23c = -1;
                }
            }
            else {
                if (m_lData2 == 1 && activation_callback_360 != 0) {
                    activation_callback_360(this);
                }
                action_230 = fallback_action_22e;
                action_state_232 = 4;
                fallback_selected = 1;
            }
        }
    }
    else {
        W8TriggerActionData005EC134* action_data =
            static_cast<W8TriggerActionData005EC134*>(m_pActionData);
        unsigned char linked_trigger_blocked = 0;

        if (m_pProp != 0 && m_pProp->Rep()->flag_06d != 0) {
            linked_trigger_blocked = 1;
        }
        else {
            char* cursor = m_pacRecipients;
            char* name;
            while ((name = NextTriggerRecipient(&cursor)) != 0) {
                Trigger* trigger = FindTriggerByName(name);
                if (trigger != 0 && trigger->CanRunLinkedTriggers() == 0) {
                    linked_trigger_blocked = 1;
                    break;
                }
            }
        }

        if (linked_trigger_blocked != 0) {
            if (m_pEvent == 0 || g_timed_events_006599b8.IndexOf(m_pEvent) == -1) {
                return 0;
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->auxiliary_timer_02c != 0) {
                m_pEvent->auxiliary_timer_02c->Restart();
            }
            if (flag_364 == 0) {
                g_flag_00606994 = 1;
            }
            return 0;
        }

        if ((action_data->flags_008 & 4) != 0 && action_data->item_00a != -1) {
            if (GetItemInHand() != action_data->item_00a) {
                if (m_lData2 == 1 && activation_callback_360 != 0) {
                    activation_callback_360(this);
                }
                action_230 = fallback_action_22e;
                action_state_232 = 4;
                fallback_selected = 1;
            }
            else {
                state_370.state = 1;
                action_data->flags_008 &= ~4;
                if (action_data->linked_trigger_00c[0] != '\0') {
                    Trigger* linked_trigger;
                    action_state_232 = 1;
                    result = 0;
                    linked_trigger = FindTriggerByName(
                        action_data->linked_trigger_00c);
                    if (linked_trigger != 0) {
                        linked_trigger->Run(-1);
                        if (flag_364 == 0) {
                            g_flag_00606994 = 1;
                        }
                    }
                }
            }
        }
    }

    if (value_368 != 0 && state_370.state == 0 && flag_364 == 0) {
        if (value_368 == 1) {
            g_flag_00606994 = 1;
            OpenLockInteraction00587510(this);
            return 0;
        }
        if (value_368 == 2) {
            g_flag_00606994 = 1;
            OpenTrapInteraction0058A470(this);
            return 0;
        }
    }

    if (fallback_selected == 0) {
        if (flag_0a0_14 == 0) {
            action_230 = initial_action_22a;
            action_state_232 = 2;
            if (flag_0a0_13 != 0) {
                flag_0a0_14 = 1;
            }
        }
        else {
            action_230 = value_22c;
            action_state_232 = 3;
            if (flag_0a0_15 != 0) {
                flag_0a0_14 = 0;
            }
        }
    }
    else if (action_230 == 0) {
        action_state_232 = 1;
        return 0;
    }
    return result;
}

// FUNCTION: WIZ8 0x0043bc10
srClass* Trigger::vInstance()
{
    return new Trigger;
}

// SYNTHETIC: WIZ8 0x0043bc70
// Trigger::`scalar deleting destructor'

// FUNCTION: WIZ8 0x0043bca0
Trigger::~Trigger()
{
    if (m_pacRecipients != 0) {
        delete[] m_pacRecipients;
    }
    if (m_pacRequiredStates != 0) {
        delete[] m_pacRequiredStates;
    }
    if (m_pacStateToMod != 0) {
        delete[] m_pacStateToMod;
    }

    if (m_pActionData != 0) {
        delete m_pActionData;
    }

    if (m_pEvent != 0) {
        int index = g_timed_events_006599b8.IndexOf(m_pEvent);
        if (index != -1) {
            g_timed_events_006599b8.RemoveAt(index);
        }
        delete m_pEvent;
    }

    if (m_pWorld != 0 && m_pWorld->triggers != 0) {
        int index = m_pWorld->triggers->IndexOf(this);
        if (index != -1) {
            m_pWorld->triggers->RemoveAt(index);
        }
    }

    if (world_item_group_34c != 0) {
        FreeWorldItemGroup(world_item_group_34c);
    }
}

// TEMPLATE: WIZ8 0x00445ad0
// srClassSupport<Trigger,srClass,1,65544>::getClassID

// TEMPLATE: WIZ8 0x00445ae0
// srClassSupport<Trigger,srClass,1,65544>::getClassName

// TEMPLATE: WIZ8 0x00445af0
// srClassSupport<Trigger,srClass,1,65544>::clone

// TEMPLATE: WIZ8 0x00445e00
// srClassSupport<Trigger,srClass,1,65544>::~srClassSupport<Trigger,srClass,1,65544>

// SYNTHETIC: WIZ8 0x00445e90
// srClassSupport<Trigger,srClass,1,65544>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00445f30
// srClassSupport<Trigger,srClass,1,65544>::getClassNode
