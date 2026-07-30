#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/registry_classes.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/item_spawning.h"
#include "wiz8/vector.h"
#include "Random.h"
#include "surrender/srCore.h"
#include "surrender/srMath.h"

#include <windows.h>

#include <math.h>
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
        if (trigger_030->m_pCountdown != 0 &&
            trigger_030->m_pCountdown->type_004 == 10 &&
            (trigger_030->m_pCountdown->flags_008 & 1) != 0) {
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
        Trigger* target = FindTriggerByName(trigger_030->m_pActionData);
        if (target != 0) {
            target->Run(-1);
        }
        break;
    }

    case 0x47: {
        char* name = trigger_030->m_pActionData;
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
// class W8TriggerCountdown

W8TriggerCountdown::W8TriggerCountdown()
    : type_004(-1)
{
}

// SYNTHETIC: WIZ8 0x0043c7f0
// W8TriggerCountdown::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00445ee0
W8TriggerCountdown::~W8TriggerCountdown()
{
}

// VTABLE: WIZ8 0x005ec148
// class W8TriggerCountdown005EC148

// SYNTHETIC: WIZ8 0x00445ec0
// W8TriggerCountdown005EC148::`scalar deleting destructor'

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
    m_pCountdown = 0;
    m_pActionData = 0;
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
    if (m_pActionData != 0) {
        delete[] m_pActionData;
    }
    if (m_pacRequiredStates != 0) {
        delete[] m_pacRequiredStates;
    }
    if (m_pacStateToMod != 0) {
        delete[] m_pacStateToMod;
    }

    if (m_pCountdown != 0) {
        delete m_pCountdown;
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
