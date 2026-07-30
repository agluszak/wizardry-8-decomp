#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_state_006598a4.h"
#include "wiz8/game_state.h"
#include "wiz8/item_spawning.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/sr_api.h"
#include "wiz8/vector.h"
#include "Random.h"
#include "surrender/srCore.h"
#include "surrender/srMath.h"

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
extern void Function4A2D30(
    unsigned int owner, unsigned int source, unsigned int target,
    unsigned int argument_4, unsigned int argument_5,
    unsigned int argument_6, unsigned int argument_7);
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
extern unsigned char g_trigger_action_active_006599c8;
extern char g_trigger_parse_buffer_00659908[256];
extern W8GrowableVector<int> g_location_variable_values_00659990;
extern unsigned char g_flag_00606994;
extern unsigned char RemovePartyItemByID005215D0(int item_id, char remove_all);
extern int OpenLockInteraction00587510(Trigger* trigger);
extern int OpenTrapInteraction0058A470(Trigger* trigger);
extern void* g_modal_owner_0068edd0;
extern unsigned char FindEntityByName(
    const char* name,
    W8Position* position,
    int* location_id,
    W8Position* direction);
extern void Function41EF50(void);
extern void RequestLevelTransition005615F0(
    int location_id, int entrance, unsigned char show_message);
extern void SetWorldScenePosition004511D0(
    W8World* world, const W8Position* position);
extern void* SpawnSpellEffect004AD080(
    const char* name, int animation, int value_1, int value_2);
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
                (unsigned int)&source, (unsigned int)&transformed,
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

// VTABLE: WIZ8 0x005ec104
// class srClassSupport<Trigger,srClass,0,65544>

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
        const char* level_folder = LevelGetFolderNameByID(GetLoadedLevelID());
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
    W8Position destination_position;
    W8Position destination_direction;
    W8Position source_position;
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
        location_id = GetLocationIDFromCode(location_code);
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
    g_engine_state_6598a4->AdjustPortalDestination00434A30(
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
    Function421030(&rotation);
    SpawnSpellEffect004AD080("set_portal", 1, 0, 0);
}

// FUNCTION: WIZ8 0x00444600
unsigned char Trigger::CanRunLinkedTriggers()
{
    char* recipient;

    if (m_pProp != 0 && m_pProp->m_owned_14->unknown_06d != 0) {
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

        if (m_pProp != 0 && m_pProp->m_owned_14->unknown_06d != 0) {
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
// srClassSupport<Trigger,srClass,0,65544>::getClassID

// TEMPLATE: WIZ8 0x00445ae0
// srClassSupport<Trigger,srClass,0,65544>::getClassName

// TEMPLATE: WIZ8 0x00445af0
// srClassSupport<Trigger,srClass,0,65544>::vClone

// TEMPLATE: WIZ8 0x00445e00
// srClassSupport<Trigger,srClass,0,65544>::~srClassSupport<Trigger,srClass,0,65544>

// SYNTHETIC: WIZ8 0x00445e90
// srClassSupport<Trigger,srClass,0,65544>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00445f30
// srClassSupport<Trigger,srClass,0,65544>::getClassNode
