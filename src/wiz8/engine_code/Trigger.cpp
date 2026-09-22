#include "soundman.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/dialog_code/DialogFactoryDialogs.h"
#include "wiz8/local_code/ConditionsAndEnchantments.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GrCycle.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/sound_man.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/location_variables.h"
#include "wiz8/string_database.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/startup_world.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Navigator.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/PathAI.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/layouts/game_status.h"
#include "wiz8/item_spawning.h"
#include "wiz8/item_tables.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/monster_runtime.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"
#include "wiz8/virtual_file.h"
#include "wiz8/xstatus.h"
#include "wiz8/save_game.h"
#include "Random.h"
#include "DEBUG.H"
#include "FileMan.h"
#include "surrender/srCamera.h"
#include "surrender/srCore.h"
#include "surrender/srMath.h"
#include "surrender/srScene.h"
#include "wiz8/local_code/character_events.h"

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

// GLOBAL: WIZ8 0x006599B8
W8GrowableVector<W8TriggerEvent*> g_timed_events_006599b8;

// GLOBAL: WIZ8 0x006599C8
unsigned char g_trigger_action_active_006599c8;
// GLOBAL: WIZ8 0x006599AC
srVector3T<float> g_trigger_action_scene_offset_006599ac;
// GLOBAL: WIZ8 0x00659908
char g_trigger_parse_buffer_00659908[0x88];
// GLOBAL: WIZ8 0x006598E0
W8GrowableVector<int> g_location_variable_levels_006598e0;
// GLOBAL: WIZ8 0x006598F8
W8GrowableVector<char*> g_location_variable_names_006598f8;
// GLOBAL: WIZ8 0x00659990
W8GrowableVector<int> g_location_variable_values_00659990;

// GLOBAL: WIZ8 0x00606994
unsigned char g_trigger_feedback_00606994 = 1;

// GLOBAL: WIZ8 0x0068c520
int g_container_event_alt_0068c520;

// GLOBAL: WIZ8 0x0068c548
int g_container_event_0068c548;

// GLOBAL: WIZ8 0x005ee59c
int g_condition_reaction_005ee59c = 5;

// GLOBAL: WIZ8 0x005ee5a0
int g_condition_reaction_alt_005ee5a0 = 6;

// GLOBAL: WIZ8 0x005ec124
const float g_float_005ec124 = 64.0f;

// FUNCTION: WIZ8 0x00443780
Trigger* FindTriggerByName(const char* name)
{
    char* uppercase_name;
    Trigger* trigger = 0;

    uppercase_name = (char*)malloc(strlen(name) + 1);
    if (uppercase_name != 0) {
        strcpy(uppercase_name, name);
        _strupr(uppercase_name);
        trigger = static_cast<Trigger*>(
            srCore.getRegistry()->find(Trigger::sGetClassNode(), uppercase_name, 0));
    }
    free(uppercase_name);
    return trigger;
}

/* Resolve the trigger that drives a prop's use action. A kind-one or kind-two
   trigger without type-10 action data is followed along its recipient chain —
   each step picks the first recipient that is not the link we came from — until
   a trigger carrying type-10 action data is found, five hops at most. */
// FUNCTION: WIZ8 0x00443830
Trigger* FindTriggerForProp00443830(W8World* world, W8Prop* prop)
{
    Trigger* trigger = prop->GetValue18();

    if (trigger != 0) {
        W8TriggerActionData* action_data = trigger->m_pActionData;

        if (action_data != 0 && action_data->type_004 == 10) {
            return trigger;
        }
        if (trigger->trigger_kind_018 == 1 || trigger->trigger_kind_018 == 2) {
            int trigger_count = world->triggers->GetCount();

            for (int index = 0; index < trigger_count; ++index) {
                Trigger* other = *world->triggers->GetAt(index);
                Trigger* previous = trigger;
                int hops = 0;

                if (other == trigger) {
                    continue;
                }
                while (hops < 5 && other->m_pacRecipients != 0 &&
                       (other->trigger_kind_018 == 1 || other->trigger_kind_018 == 2)) {
                    char* recipient = other->m_pacRecipients;
                    Trigger* linked;

                    do {
                        char* comma;

                        if (recipient == 0) {
                            goto next_trigger;
                        }
                        strcpy(g_trigger_parse_buffer_00659908, recipient);
                        comma = strchr(g_trigger_parse_buffer_00659908, ',');
                        if (comma == 0) {
                            recipient = 0;
                        } else {
                            recipient = strchr(recipient, ',') + 1;
                            *comma = '\0';
                        }
                        linked = FindTriggerByName(g_trigger_parse_buffer_00659908);
                    } while (linked != previous);
                    action_data = other->m_pActionData;
                    if (action_data != 0 && action_data->type_004 == 10) {
                        return other;
                    }
                    ++hops;
                    previous = other;
                    if (other == 0) {
                        break;
                    }
                }
            next_trigger:;
            }
        }
    }
    return 0;
}

/* Re-rolls the eight pin bytes of a pickable lock (lock_state[0] == 1) and
   resets its difficulty-derived seed/state fields. lock_state is
   &Trigger::lock_type. */
// FUNCTION: WIZ8 0x00445730
void __fastcall UpdateTriggerLock00445730(int* lock_state)
{
    int pins;

    if (*lock_state == 1) {
        for (int pin = 0; pin < 8; ++pin) {
            // reinterpret-ok: lock-state pins are byte storage inside the int blob
            reinterpret_cast<char*>(lock_state)[pin + 9] = static_cast<char>(Random(4));
        }
        pins = lock_state[1];
        if (pins < 8) {
            if (pins < 2) {
                lock_state[7] = 6;
                lock_state[8] = -1;
                // reinterpret-ok: byte field inside the lock-state int blob
                reinterpret_cast<char*>(lock_state)[8] = 0;
                return;
            }
            if (pins > 7) {
                pins = 8;
            }
        } else {
            pins = 8;
        }
        lock_state[7] = pins * 3;
    }
    lock_state[8] = -1;
    // reinterpret-ok: byte field inside the lock-state int blob
    reinterpret_cast<char*>(lock_state)[8] = 0;
}

/* Ticks the lock countdown at lock_state[7]; returns 1 while a tick remained. */
// FUNCTION: WIZ8 0x004457A0
unsigned char __fastcall ConsumeLockQuality004457A0(int* lock_state)
{
    if (lock_state[7] > 0) {
        --lock_state[7];
        return 1;
    }
    return 0;
}

/* True while a type-0x34 destination trigger holds (x, y, z) inside its
   activation annulus: under range_maximum_0a8 and, unless flags_0a0 bit 6
   waives the minimum, at or beyond range_minimum_0a4. */
// FUNCTION: WIZ8 0x00445940
bool InsideDestinationTrigger00445940(float x, float y, float z)
{
    int count = g_world->triggers->GetCount();

    for (int index = 0; index < count; ++index) {
        Trigger* trigger = *g_world->triggers->GetAt(index);
        if (trigger->initial_action_22a != 0x34) {
            continue;
        }
        float dx = trigger->position_118.x - x;
        float dy = trigger->position_118.y - y;
        float dz = trigger->position_118.z - z;
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
        if (distance < trigger->range_maximum_0a8 && ((trigger->flags_0a0 >> 6) & 1) == 0 &&
            distance >= trigger->range_minimum_0a4) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x0043cb30
void SaveTriggerRuntimeStates0043CB30(W8World* world, int handle, bool restoring)
{
    int trigger_count = world->triggers->GetCount();
    int saved_count = 0;
    int index;
    int version = 2;
    int restoring_value = restoring;

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->lock_type != 0) {
            ++saved_count;
        }
    }

    FileWrite(handle, &version, sizeof(version), 0);
    FileWrite(handle, &saved_count, sizeof(saved_count), 0);
    FileWrite(handle, &restoring_value, sizeof(restoring_value), 0);

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->lock_type != 0) {
            FileWrite(handle, trigger->name_01c, 0x80, 0);
            FileWrite(handle, &version, sizeof(version), 0);
            if (restoring) {
                FileWrite(handle, &trigger->lock_type, sizeof(trigger->lock_type), 0);
                FileWrite(handle, &trigger->difficulty, sizeof(trigger->difficulty), 0);
                FileWrite(handle, &trigger->device_state.completed,
                          sizeof(trigger->device_state.completed), 0);
                FileWrite(handle, trigger->device_state.pins, sizeof(trigger->device_state.pins),
                          0);
                FileWrite(handle, &trigger->device_id, sizeof(trigger->device_id), 0);
                FileWrite(handle, &trigger->key_id, sizeof(trigger->key_id), 0);
                FileWrite(handle, &trigger->lock_countdown, sizeof(trigger->lock_countdown), 0);
            } else {
                FileWrite(handle, &trigger->device_state.completed,
                          sizeof(trigger->device_state.completed), 0);
                FileWrite(handle, trigger->device_state.pins, sizeof(trigger->device_state.pins),
                          0);
                FileWrite(handle, &trigger->lock_countdown, sizeof(trigger->lock_countdown), 0);
                FileWrite(handle, &trigger->last_interaction_clock,
                          sizeof(trigger->last_interaction_clock), 0);
            }
        }
    }
}

/* Read the runtime-state records written by SaveTriggerRuntimeStates0043CB30.
   Each record names its trigger; a record whose trigger no longer exists is
   still consumed through a scratch trigger so the stream stays aligned. When
   restoring, a state byte block is re-randomized and the type-10 action data is
   re-linked to the stored item. */
// FUNCTION: WIZ8 0x0043ccf0
bool LoadTriggerRuntimeStates0043CCF0(int handle)
{
    int version;
    int saved_count;
    int restoring;
    int index = 0;

    FileRead(handle, &version, sizeof(version), 0);
    FileRead(handle, &saved_count, sizeof(saved_count), 0);
    FileRead(handle, &restoring, sizeof(restoring), 0);
    if (saved_count < 1) {
        return 1;
    }
    for (;;) {
        char name[0x80];
        Trigger* trigger;

        FileRead(handle, name, sizeof(name), 0);
        trigger = FindTriggerByName(name);
        if (trigger == 0) {
            Trigger* scratch = new Trigger;
            int record_version;

            if (restoring == 0 && version > 1) {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &scratch->device_state.completed,
                         sizeof(scratch->device_state.completed), 0);
                FileRead(handle, scratch->device_state.pins, sizeof(scratch->device_state.pins), 0);
                FileRead(handle, &scratch->lock_countdown, sizeof(scratch->lock_countdown), 0);
                if (record_version > 1) {
                    FileRead(handle, &scratch->last_interaction_clock,
                             sizeof(scratch->last_interaction_clock), 0);
                }
            } else {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &scratch->lock_type, sizeof(scratch->lock_type), 0);
                FileRead(handle, &scratch->difficulty, sizeof(scratch->difficulty), 0);
                FileRead(handle, &scratch->device_state.completed,
                         sizeof(scratch->device_state.completed), 0);
                FileRead(handle, scratch->device_state.pins, sizeof(scratch->device_state.pins), 0);
                FileRead(handle, &scratch->device_id, sizeof(scratch->device_id), 0);
                FileRead(handle, &scratch->key_id, sizeof(scratch->key_id), 0);
                if (record_version > 1) {
                    FileRead(handle, &scratch->lock_countdown, sizeof(scratch->lock_countdown), 0);
                }
            }
            delete scratch;
        } else {
            int record_version;
            W8TriggerActionData* action_data;

            if (restoring == 0 && version > 1) {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &trigger->device_state.completed,
                         sizeof(trigger->device_state.completed), 0);
                FileRead(handle, trigger->device_state.pins, sizeof(trigger->device_state.pins), 0);
                FileRead(handle, &trigger->lock_countdown, sizeof(trigger->lock_countdown), 0);
                if (record_version > 1) {
                    FileRead(handle, &trigger->last_interaction_clock,
                             sizeof(trigger->last_interaction_clock), 0);
                }
            } else {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &trigger->lock_type, sizeof(trigger->lock_type), 0);
                FileRead(handle, &trigger->difficulty, sizeof(trigger->difficulty), 0);
                FileRead(handle, &trigger->device_state.completed,
                         sizeof(trigger->device_state.completed), 0);
                FileRead(handle, trigger->device_state.pins, sizeof(trigger->device_state.pins), 0);
                FileRead(handle, &trigger->device_id, sizeof(trigger->device_id), 0);
                FileRead(handle, &trigger->key_id, sizeof(trigger->key_id), 0);
                if (record_version > 1) {
                    FileRead(handle, &trigger->lock_countdown, sizeof(trigger->lock_countdown), 0);
                }
                if (restoring != 0) {
                    if (trigger->lock_type == 1) {
                        int size;

                        for (int byte_index = 0; byte_index < 8; ++byte_index) {
                            trigger->device_state.pins[byte_index] =
                                static_cast<unsigned char>(Random(4));
                        }
                        size = trigger->difficulty;
                        if (size < 2) {
                            size = 2;
                        } else if (size > 7) {
                            size = 8;
                        }
                        trigger->lock_countdown = size * 3;
                    }
                    trigger->last_interaction_clock = -1;
                    trigger->device_state.completed = 0;
                }
            }
            action_data = trigger->m_pActionData;
            if (action_data != 0 && action_data->type_004 == 10 &&
                ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 4) == 0 ||
                 static_cast<W8DoorTriggerActionData*>(action_data)->item_00a == -1)) {
                static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 =
                    ((trigger->device_state.completed == 0) << 2) |
                    (static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 0xfb);
                static_cast<W8DoorTriggerActionData*>(action_data)->item_00a =
                    static_cast<short>(trigger->key_id);
            }
        }
        ++index;
        if (saved_count <= index) {
            return 1;
        }
    }
}

/* Serialize one trigger for the save file. The header carries a version byte
   and the action-state block; the action payload follows only when one is
   attached, with its flag bits packed and the timed-event delay resolved from
   the live event queue. Returns whether the header went out completely. */
// FUNCTION: WIZ8 0x0043BE60
bool Trigger::Save0043BE60(int hFile)
{
    unsigned char version = 5;
    unsigned char reserved[4];
    bool header_ok;
    W8TriggerActionData* action_data;
    unsigned char has_action_data;
    unsigned char action_type;
    unsigned short action_flags;
    unsigned char action_kind;
    unsigned int progress_delay;
    unsigned char has_world_item;

    if (hFile == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x183, 0);
    }
    header_ok = FileWrite(hFile, &version, sizeof(version), 0) &&
                FileWrite(hFile, reserved, sizeof(reserved), 0) &&
                FileWrite(hFile, name_01c, 0x80, 0) &&
                FileWrite(hFile, &flags_0a0, sizeof(flags_0a0), 0) &&
                FileWrite(hFile, &state_index, sizeof(state_index), 0) &&
                FileWrite(hFile, &state_direction, sizeof(state_direction), 0) &&
                FileWrite(hFile, &action_230, sizeof(action_230), 0) &&
                FileWrite(hFile, &action_state_232, sizeof(action_state_232), 0) &&
                FileWrite(hFile, &required_item_id, sizeof(required_item_id), 0);
    action_data = m_pActionData;
    has_action_data = action_data != 0;
    FileWrite(hFile, &has_action_data, sizeof(has_action_data), 0);
    if (action_data != 0) {
        action_type = action_data->type_004;
        FileWrite(hFile, &action_type, sizeof(action_type), 0);
        if (action_type == 10) {
            action_flags = 0;
            progress_delay = 0;
            action_kind = 2;
            FileWrite(hFile, &action_kind, sizeof(action_kind), 0);
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 1) != 0) {
                action_flags |= 1;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 2) != 0) {
                action_flags |= 2;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 4) != 0) {
                action_flags |= 4;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 8) != 0) {
                action_flags |= 8;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 0x10) != 0) {
                action_flags |= 0x10;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 0x20) != 0) {
                action_flags |= 0x20;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 0x40) != 0) {
                action_flags |= 0x40;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 0x80) != 0) {
                action_flags |= 0x80;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_009 & 1) != 0) {
                action_flags |= 0x100;
            }
            if (static_cast<W8DoorTriggerActionData*>(action_data)->item_00a != 0) {
                action_flags |= 0x200;
            }
            FileWrite(hFile, &action_flags, sizeof(action_flags), 0);
            if (m_lData1 != 0 && m_pEvent != 0 && g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                float progress = m_pEvent->timer_008.GetProgress();
                if (progress <= g_float_005ec124) {
                    progress_delay = (unsigned int)m_pEvent->timer_008.GetProgress();
                } else {
                    progress_delay = 64000;
                }
            }
            FileWrite(hFile, &progress_delay, 2, 0);
            {
                unsigned short zero = 0;
                FileWrite(hFile, &zero, sizeof(zero), 0);
            }
        }
    }
    has_world_item = world_item_group_34c != 0;
    FileWrite(hFile, &has_world_item, sizeof(has_world_item), 0);
    if (has_world_item != 0) {
        SaveItemFile(hFile, world_item_group_34c);
    }
    FileWrite(hFile, &items_generated, sizeof(items_generated), 0);
    FileWrite(hFile, &item_group_seed_354, sizeof(item_group_seed_354), 0);
    FileWrite(hFile, &gold_358, sizeof(gold_358), 0);
    FileWrite(hFile, &uses_remaining, sizeof(uses_remaining), 0);
    return header_ok;
}

/* Read one trigger back from the save file. The version byte selects how much
   of the trailing block is present; a type-10 action payload rebuilds its
   action data and re-queues the delayed timed event from the saved progress. */
// FUNCTION: WIZ8 0x0043c1b0
bool Trigger::Load0043C1B0(int hFile, char version)
{
    bool header_ok;
    unsigned char has_action_data;
    unsigned char action_type;
    unsigned char flag_mode;
    unsigned char flag;

    if (hFile == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1f0, 0);
    }
    header_ok = FileRead(hFile, &flags_0a0, sizeof(flags_0a0), 0) &&
                FileRead(hFile, &state_index, sizeof(state_index), 0) &&
                FileRead(hFile, &state_direction, sizeof(state_direction), 0) &&
                FileRead(hFile, &action_230, sizeof(action_230), 0) &&
                FileRead(hFile, &action_state_232, sizeof(action_state_232), 0) &&
                FileRead(hFile, &required_item_id, sizeof(required_item_id), 0);
    if ((flags_0a0 & W8_TRIGGER_SEARCHED) != 0) {
        UnregisterSearchableTrigger00516FE0(this);
    }
    FileRead(hFile, &has_action_data, sizeof(has_action_data), 0);
    if (has_action_data != 0) {
        FileRead(hFile, &action_type, sizeof(action_type), 0);
        if (action_type == 10) {
            W8DoorTriggerActionData* pDoor = new W8DoorTriggerActionData;
            unsigned short action_flags;
            unsigned short progress_delay;
            unsigned short item;

            if (pDoor == 0) {
                srAssertFail("pDoor", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x20b,
                             0);
            } else {
                pDoor->flags_008 = 0x40;
                pDoor->flags_009 &= ~1;
                pDoor->item_00a = -1;
                pDoor->type_004 = 10;
                pDoor->position_08c.SetZero();
                pDoor->linked_trigger_00c[0] = 0;
            }
            delete m_pActionData;
            m_pActionData = pDoor;
            pDoor->type_004 = 10;
            FileRead(hFile, &flag_mode, 1, 0);
            if (flag_mode == 1) {
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~1) | (flag & 1);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~2) | ((flag & 1) << 1);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~4) | ((flag & 1) << 2);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~8) | ((flag & 1) << 3);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~0x10) | ((flag & 1) << 4);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~0x20) | ((flag & 1) << 5);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~0x40) | ((flag & 1) << 6);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_008 = (pDoor->flags_008 & ~0x80) | (flag << 7);
                FileRead(hFile, &flag, 1, 0);
                pDoor->flags_009 = (pDoor->flags_009 & ~1) | (flag & 1);
            } else {
                action_flags = 0;
                progress_delay = 0;
                FileRead(hFile, &action_flags, 2, 0);
                if ((action_flags & 1) != 0) {
                    pDoor->flags_008 |= 1;
                } else {
                    pDoor->flags_008 &= ~1;
                }
                if ((action_flags & 2) != 0) {
                    pDoor->flags_008 |= 2;
                } else {
                    pDoor->flags_008 &= ~2;
                }
                if ((action_flags & 4) != 0) {
                    pDoor->flags_008 |= 4;
                } else {
                    pDoor->flags_008 &= ~4;
                }
                if ((action_flags & 8) != 0) {
                    pDoor->flags_008 |= 8;
                } else {
                    pDoor->flags_008 &= ~8;
                }
                if ((action_flags & 0x10) != 0) {
                    pDoor->flags_008 |= 0x10;
                } else {
                    pDoor->flags_008 &= ~0x10;
                }
                if ((action_flags & 0x20) != 0) {
                    pDoor->flags_008 |= 0x20;
                } else {
                    pDoor->flags_008 &= ~0x20;
                }
                if ((action_flags & 0x40) != 0) {
                    pDoor->flags_008 |= 0x40;
                } else {
                    pDoor->flags_008 &= ~0x40;
                }
                if ((action_flags & 0x80) != 0) {
                    pDoor->flags_008 |= 0x80;
                } else {
                    pDoor->flags_008 &= ~0x80;
                }
                if ((action_flags & 0x100) != 0) {
                    pDoor->flags_009 |= 1;
                } else {
                    pDoor->flags_009 &= ~1;
                }
                FileRead(hFile, &progress_delay, 2, 0);
                if (m_lData1 != 0 && progress_delay != 0) {
                    if (m_pEvent == 0) {
                        float duration;

                        m_pEvent = new W8TriggerEvent;
                        m_pEvent->trigger_030 = this;
                        m_pEvent->action_004 = 2;
                        if (m_lData1 < 0) {
                            duration = 10.0f;
                        } else {
                            duration = static_cast<float>(m_lData1);
                        }
                        m_pEvent->timer_008.SetDuration(duration);
                        m_pEvent->timer_008.Restart();
                        m_pEvent->repeat_034 = 1;
                    }
                    m_pEvent->timer_008.SetProgress(static_cast<float>(progress_delay) *
                                                    g_float_005ec128);
                    if (g_timed_events_006599b8.IndexOf(m_pEvent) == -1) {
                        g_timed_events_006599b8.Add(m_pEvent);
                    }
                }
                FileRead(hFile, &item, 2, 0);
                pDoor->item_00a = static_cast<short>(item);
            }
        }
    }

    if (version > 1) {
        unsigned char has_world_item;

        FileRead(hFile, &has_world_item, sizeof(has_world_item), 0);
        if (has_world_item != 0) {
            world_item_group_34c = LoadItem(hFile, 0);
        }
        FileRead(hFile, &items_generated, sizeof(items_generated), 0);
        FileRead(hFile, &item_group_seed_354, sizeof(item_group_seed_354), 0);
        FileRead(hFile, &gold_358, sizeof(gold_358), 0);
        FileRead(hFile, &uses_remaining, sizeof(uses_remaining), 0);
    }
    if (version == 3) {
        FileSeek(hFile, 0x1d, FILE_SEEK_FROM_CURRENT);
    }
    return header_ok;
}

/* Read every trigger record of the save file's trigger chunk. Tags 1-4 update
   the matching in-place trigger, tag 5 names its trigger and is consumed
   through a scratch trigger when the name is gone, and anything else pushes
   the tag byte back and ends the walk. */
// FUNCTION: WIZ8 0x0043c860
bool LoadWorldTriggers0043C860(W8World* world, int hFile)
{
    int trigger_count = world->triggers->GetCount();
    int index = 0;
    bool header_ok = true;
    bool finished = false;

    for (;;) {
        /* Retail read `tag` (and the tag-5 name/id below) uninitialised when a
           FileRead short-circuited; deterministic values model that defect
           path. */
        char tag = 0;

        if (finished || trigger_count <= index) {
            return header_ok;
        }
        if (!header_ok || !FileRead(hFile, &tag, 1, 0)) {
            header_ok = false;
        } else {
            header_ok = true;
        }
        if (tag < 1 || tag > 5) {
            finished = true;
            FileSeek(hFile, -1, FILE_SEEK_FROM_CURRENT);
        } else if (tag < 5) {
            Trigger* trigger = *world->triggers->GetAt(index);

            ++index;
            if (!header_ok || !trigger->Load0043C1B0(hFile, tag)) {
                return false;
            }
            header_ok = true;
        } else {
            int trigger_id = 0;
            char name[0x80] = {0};
            Trigger* trigger;

            if (!header_ok || !FileRead(hFile, &trigger_id, sizeof(trigger_id), 0) ||
                !FileRead(hFile, name, sizeof(name), 0)) {
                header_ok = false;
            } else {
                header_ok = true;
            }
            trigger = FindTriggerByName(name);
            if (trigger != 0) {
                if (!header_ok || !trigger->Load0043C1B0(hFile, tag)) {
                    header_ok = false;
                } else {
                    header_ok = true;
                }
                ++index;
            } else {
                Trigger* scratch = new Trigger;

                scratch->Load0043C1B0(hFile, tag);
                delete scratch;
                ++index;
            }
        }
        if (!header_ok) {
            return false;
        }
    }
}

/* Write every trigger of a world for the save file's trigger chunk. A trigger
   whose own serialization reports failure stops the walk. */
// FUNCTION: WIZ8 0x0043C810
void SaveWorldTriggers0043C810(W8World* world, int hFile)
{
    W8GrowableVector<Trigger*>* triggers = world->triggers;

    for (int index = 0; index < triggers->GetCount(); ++index) {
        if (!(*triggers->GetAt(index))->Save0043BE60(hFile)) {
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0043d120
void SaveTriggerActionData0043D120(W8World* world, int handle)
{
    int trigger_count = world->triggers->GetCount();
    int saved_count = 0;
    int index;
    int version = 1;

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->inline_action_data_24c[0] != '\0') {
            ++saved_count;
        }
    }

    FileWrite(handle, &version, sizeof(version), 0);
    FileWrite(handle, &saved_count, sizeof(saved_count), 0);

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->inline_action_data_24c[0] != '\0') {
            FileWrite(handle, trigger->name_01c, 0x80, 0);
            FileWrite(handle, trigger->inline_action_data_24c,
                      sizeof(trigger->inline_action_data_24c), 0);
        }
    }
}

/* Read the trigger action-data chunk written by SaveTriggerActionData0043D120:
   a version/count header, then per record the trigger name and its 0x100-byte
   inline payload. Records for missing triggers are skipped with a seek. */
// FUNCTION: WIZ8 0x0043d1f0
bool LoadTriggerActionData0043D1F0(int handle)
{
    int version;
    int saved_count;
    int index = 0;

    FileRead(handle, &version, sizeof(version), 0);
    FileRead(handle, &saved_count, sizeof(saved_count), 0);
    if (saved_count < 1) {
        return 1;
    }
    for (;;) {
        char name[0x80];
        Trigger* trigger;

        FileRead(handle, name, sizeof(name), 0);
        trigger = FindTriggerByName(name);
        if (trigger != 0) {
            FileRead(handle, trigger->inline_action_data_24c,
                     sizeof(trigger->inline_action_data_24c), 0);
        } else {
            FileSeek(handle, 0x100, FILE_SEEK_FROM_CURRENT);
        }
        ++index;
        if (saved_count <= index) {
            return 1;
        }
    }
}

// VTABLE: WIZ8 0x005ec12c
// class W8TriggerEvent

W8TriggerEvent::W8TriggerEvent()
    : action_004(-1), timer_008(), m_pCountdown(0), trigger_030(0), repeat_034(0), completed_035(0)
{
}

// SYNTHETIC: WIZ8 0x00440980
// W8TriggerEvent::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004409a0
W8TriggerEvent::~W8TriggerEvent() {}

class W8TriggerShakeEvent : public W8TriggerEvent {
public:
    W8TriggerShakeEvent();
    virtual void Update() override;

    W8CameraShakeEffect* effect_038;
    int intensity_03c;
    unsigned char reverse_040;
    unsigned char unknown_041[3];
};

static_assert(sizeof(W8TriggerShakeEvent) == 0x44, "W8TriggerShakeEvent_must_be_0x44");

// VTABLE: WIZ8 0x005ec140
// class W8TriggerShakeEvent

/* Retail ICF folds this class's deleting destructor onto W8TriggerEvent's. */
// SYNTHETIC: WIZ8 0x00440980 FOLDED
// W8TriggerShakeEvent::`scalar deleting destructor'

W8TriggerShakeEvent::W8TriggerShakeEvent() : effect_038(0), intensity_03c(1), reverse_040(0) {}

// GLOBAL: WIZ8 0x006599a0
srVector3T<float> g_trigger_camera_006599a0;

void OnItemDialogClosed004456C0(W8DialogBase* base);

/* Advance every world trigger: raise the item picker when a prop-bearing
   activation asks for one, then run proximity activations for kind-two
   triggers. The camera position is cached when no trigger fired. */
// FUNCTION: WIZ8 0x00443ae0
void UpdateWorldTriggers00443AE0(W8World* world)
{
    srVector3T<float> camera;
    bool activated = false;
    bool running = false;

    GetCameraPosition(&camera);
    int count = world->triggers->GetCount();
    for (int index = 0; index < count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if ((trigger->flags_0a0 & W8_TRIGGER_ITEM_PICKER) != 0 && trigger->m_pProp != 0 &&
            (trigger->m_pProp->GetAnimationState0044EBE0() < 2 ||
             trigger->m_pProp->Rep()->animation_playing_06d == 0)) {
            trigger->GenerateItemGroup();
            if (g_modal_owner_0068edd0 == 0 && trigger->world_item_group_34c != 0) {
                W8TriggerItemPickerDialog* dialog = new W8TriggerItemPickerDialog;
                if (dialog != 0) {
                    dialog->m_user_data = trigger;
                    dialog->SetItemGroup(trigger->world_item_group_34c);
                    dialog->m_destroy_callback = OnItemDialogClosed004456C0;
                    gXStatus.item_pick_pending_19b6 = 0;
                    g_modal_owner_0068edd0 = dialog;
                }
            }
        }
        if (g_environment_load_flag_00603ad0 != 0 && trigger->trigger_kind_018 == 2 &&
            (trigger->flags_0a0 & W8_TRIGGER_ENABLED) != 0 &&
            (trigger->flags_0a0 & W8_TRIGGER_POSITIONED) != 0 && (trigger->flags_0a0 & 0x4U) == 0) {
            srVector3T<float> trigger_position(trigger->position_118.x, trigger->position_118.y,
                                               trigger->position_118.z);
            float distance = (trigger_position - camera).Length();
            if (trigger->range_maximum_0a8 <= distance) {
                if ((trigger->flags_0a0 & W8_TRIGGER_RUNNING) != 0) {
                    trigger->FinishAction();
                }
            } else if ((trigger->flags_0a0 & W8_TRIGGER_RUNNING) == 0 &&
                       trigger->range_minimum_0a4 <= distance) {
                activated = true;
                if ((trigger->flags_0a0 & W8_TRIGGER_EXCLUSIVE) == 0 || !running) {
                    trigger->Run(-1);
                    running = true;
                }
            }
        }
    }
    if (world == g_world && !activated) {
        g_trigger_camera_006599a0 = camera;
    }
}

/* The item picker's destroy callback: hand its items back to the owning
   trigger and clear the trigger's pending-picker bit. */
// FUNCTION: WIZ8 0x004456c0
void OnItemDialogClosed004456C0(W8DialogBase* base)
{
    W8TriggerItemPickerDialog* dialog = static_cast<W8TriggerItemPickerDialog*>(base);

    if (dialog != 0) {
        dialog->ReturnItemsToGroup();
        static_cast<Trigger*>(dialog->m_user_data)->flags_0a0 &= ~W8_TRIGGER_ITEM_PICKER;
    }
}

// FUNCTION: WIZ8 0x00443D30
void UpdateTimedTriggerEvents00443D30(void)
{
    for (int index = 0; index < g_timed_events_006599b8.GetCount(); ++index) {
        W8TriggerEvent* event = *g_timed_events_006599b8.GetAt(index);
        event->Update();
        if (event->completed_035 != 0) {
            g_timed_events_006599b8.RemoveAt(index);
            --index;
            if (event->trigger_030 != 0) {
                event->trigger_030->m_pEvent = 0;
            }
            delete event;
        }
    }
}

// FUNCTION: WIZ8 0x00444ec0
void W8TriggerShakeEvent::Update()
{
    if (effect_038 == 0) {
        float intensity = (float)intensity_03c / 1000.0f;

        if (intensity > 1.0f) {
            intensity = 1.0f;
        }
        effect_038 =
            CreateCameraShakeEffect004AE080(m_pCountdown->m_duration_seconds, 0, intensity, 0, 0);
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

/* Create one shake-camera event and queue it on the world's timed-event list.
   The caller passes the intensity, the effect duration and the optional
   countdown duration, all scaled by the trigger unit's 0.001 factor. */
// FUNCTION: WIZ8 0x00444F70
bool CreateTriggerShakeEvent00444F70(int intensity, float duration, float countdown_duration,
                                     bool reverse)
{
    W8TriggerShakeEvent* pEvent = new W8TriggerShakeEvent;

    if (pEvent == 0) {
        srAssertFail("pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1372,
                     "Out of memory creating shake camera event");
    }
    pEvent->repeat_034 = 1;
    pEvent->intensity_03c = intensity;
    if (pEvent->m_pCountdown != 0) {
        delete pEvent->m_pCountdown;
    }
    pEvent->m_pCountdown = new W8GameTimer;
    if (pEvent->m_pCountdown == 0) {
        srAssertFail("m_pCountdown", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x12de,
                     0);
    }
    pEvent->m_pCountdown->SetDuration(countdown_duration * g_float_005ec128);
    pEvent->m_pCountdown->Restart();
    pEvent->timer_008.SetDuration(duration * g_float_005ec128);
    pEvent->timer_008.Restart();
    pEvent->reverse_040 = reverse;
    g_timed_events_006599b8.Add(pEvent);
    return 1;
}

// FUNCTION: WIZ8 0x004447F0
void Trigger::CompleteItemInteraction004447F0()
{
    W8TriggerActionData* action_data = m_pActionData;
    device_state.completed = 1;
    if (action_data != 0 && action_data->type_004 == 10) {
        static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 &= ~4;
    }
}

/* Run the trigger now when its action data selects the immediate path, or
   restart its timed-event clocks when it already owns a queued event. The
   immediate path sets the running flag around Run so nested activation sees
   it; the timed path only touches clocks for events still queued. */
// FUNCTION: WIZ8 0x00444750
void Trigger::Activate00444750()
{
    W8TriggerActionData* action_data = m_pActionData;
    if (action_data != 0 && action_data->type_004 == 10 &&
        (lock_type == 0 || device_state.completed != 0) &&
        (static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 4) == 0) {
        if ((static_cast<W8DoorTriggerActionData*>(action_data)->flags_008 & 1) == 0) {
            running = 1;
            Run(-1);
            running = 0;
        } else {
            W8TriggerEvent* event = m_pEvent;
            if (event != 0 && g_timed_events_006599b8.IndexOf(event) != -1) {
                event->timer_008.Restart();
                if (event->m_pCountdown != 0) {
                    event->m_pCountdown->Restart();
                }
            }
        }
    }
}

// FUNCTION: WIZ8 0x00444810
bool Trigger::HasActorWithinRadius(float radius, bool include_party)
{
    srVector3T<float> center;

    if ((flags_0a0 & W8_TRIGGER_POSITIONED) != 0 || m_pProp != 0) {
        if (m_pProp == 0) {
            center.Set(position_118.x, position_118.y, position_118.z);
        } else {
            m_pProp->GetCenterPosition(&center);
        }

        srVector3T<float> lower;
        srVector3T<float> upper;
        srVector3T<float> extent;
        unsigned long* locations = 0;
        extent.Set(radius, radius, radius);
        lower = center - extent;
        upper = center + extent;

        unsigned int count =
            g_octree_6598a4->QueryObjects(&locations, &lower, &upper, W8_OCTREE_KIND_LOCATION, -1);
        for (unsigned int index = 0; index < count; ++index) {
            int location_id = locations[index];
            if (location_id == 0) {
                break;
            }
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x1246, "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (monster_info != 0 && monster_info->monster != 0) {
                srVector3T<float> monster_position = monster_info->monster->GetPosition();
                if ((monster_position - center).Length() <= radius) {
                    return 1;
                }
            }
        }
    }

    if (include_party) {
        srVector3T<float> party_position = g_startup_world_659c0c->GetPosition();
        if ((party_position - center).Length() <= radius) {
            return 1;
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x004457c0
bool Trigger::PlayActionSound(const char* sound_name, int volume)
{
    srVector3T<float> position;

    if (volume == 0) {
        volume = 100;
    }
    if ((flags_0a0 & W8_TRIGGER_POSITIONED) == 0) {
        if (m_pProp == 0) {
            SOUNDPARMS options;
            memset(&options, -1, sizeof(options));
            options.uiVolume = (g_settings_6850c8.sound_effects_volume * volume) / 0x7f;
            SoundPlay((STR)sound_name, &options);
            return 0;
        }
        m_pProp->GetCenterPosition(&position);
    } else {
        position.Set(position_118.x, position_118.y, position_118.z);
    }

    stSound3D* sound = new stSound3D(sound_name, 0);
    if (sound != 0) {
        srVector3T<double> sound_position;
        sound_position.SetFromFloat(&position);
        sound->volume = volume;
        sound->setLocation(sound_position);
        if (sound->Play(0, 1) != 0) {
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

        if (g_combat_inactive_006081e4 == 0) {
            if ((flags & 8) != 0 || (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
                g_shared_timer_flag_d1 != 0) {
                return;
            }
            timer_008.m_flags = flags | 8;
            timer_008.m_start = timer_008.GetTime00439A60() - timer_008.m_start;
            return;
        }
        if ((flags & 8) != 0 || (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
            g_shared_timer_flag_d1 != 0) {
            timer_008.m_flags = flags & ~8;
            timer_008.m_start = timer_008.GetTime00439A60() - timer_008.m_start;
            timer_008.SetDuration(-1.0f);
        }
    }

    if (timer_008.GetProgress() <= 1.0f || trigger_030->HasActorWithinRadius(5000.0f, 1) != 0) {
        return;
    }

    switch (action_004) {
    case 2:
        if (trigger_030->m_pActionData != 0 && trigger_030->m_pActionData->type_004 == 10 &&
            (static_cast<W8DoorTriggerActionData*>(trigger_030->m_pActionData)->flags_008 & 1) !=
                0) {
            trigger_030->Run(-1);
        }
        break;

    case 0x0c: {
        if (trigger_030 != 0) {
            srVector3T<float> source;
            srVector3T<float> target;
            srVector3T<float> transformed;
            srVector3T<float> axis;
            srMatrix3T<float> rotation;

            source.x = trigger_030->position_118.x;
            source.y = trigger_030->position_118.y;
            source.z = trigger_030->position_118.z;
            target = source;
            target.z += 100.0f;

            axis.Set(0.0, 0.0, 1.0);
            rotation.vectors[0].Set(trigger_030->direction_100.x, trigger_030->direction_100.y,
                                    trigger_030->direction_100.z);
            rotation.vectors[1].Set(1.0, 0.0, 0.0);
            rotation.vectors[2].Set(0.0, 1.0, 0.0);

            if (trigger_030->angle_0fc != 0.0f) {
                rotation.RotateAroundAxis(sin(trigger_030->angle_0fc), cos(trigger_030->angle_0fc),
                                          axis);
            }

            transformed.x = DotProduct(rotation.vectors[1], target);
            transformed.y = DotProduct(rotation.vectors[2], target);
            transformed.z = DotProduct(axis, target);
            FireMissile004A2D30((unsigned int)trigger_030->m_lData1, &source, &transformed, 0, 1, 1,
                                50000.0f);
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
        trigger_030->uses_remaining = trigger_030->m_lData1;
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
            } else {
                name = strchr(name, ',') + 1;
                *comma = '\0';
            }
            stParticle* particle = FindParticleByName(g_world, buffer);
            if (particle != 0) {
                particle->persisted_192 = 1;
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
void InitializeStateDrivenPropVariables00445200(Trigger* trigger)
{
    int slot;

    if (trigger->m_pacStateToMod == 0) {
        return;
    }
    for (slot = 0; slot < static_cast<signed char>(trigger->m_pProp->Rep()->slots.GetCount());
         ++slot) {
        char name[132];
        int variable_id;

        if (trigger->m_bRepType != 2) {
            srAssertFail("m_bRepType == TRIGGER_REP_PROP", "..\\Engine Code\\Include\\Trigger.hpp",
                         0x3ed, 0);
        }
        sprintf(name, "%s%d", trigger->m_pacStateToMod, slot);
        /* `name` is a stack array; the assertion still names pacName. The
           source pointer is already rejected at the top of this function.
           Do not collapse the recovered test until a body comparison says
           retail omitted it. */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-compare"
        if (name == 0) {
#pragma clang diagnostic pop
            srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1094,
                         0);
        }

        variable_id = 0;
        while (variable_id < g_location_variable_names_006598f8.GetCount()) {
            if (_stricmp(*g_location_variable_names_006598f8.GetAt(variable_id), name) == 0 &&
                *g_location_variable_levels_006598e0.GetAt(variable_id) ==
                    g_status_685170.current_level) {
                break;
            }
            ++variable_id;
        }
        if (variable_id == g_location_variable_names_006598f8.GetCount()) {
            char* variable_name = new char[strlen(name) + 1];
            if (variable_name == 0) {
                srAssertFail("pacVariableName",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x109c, 0);
            }
            strcpy(variable_name, name);
            g_location_variable_names_006598f8.Add(variable_name);
            g_location_variable_values_00659990.Add(slot == 0);
            g_location_variable_levels_006598e0.Add(g_status_685170.current_level);
        }
    }
}

/* Set a location variable by name when it belongs to the current level. An
   unknown name fails the lookup assertion; the value only lands while the
   value list still covers the found index. */
// FUNCTION: WIZ8 0x00444030
void SetTriggerVariableByName00444030(const char* name, int value)
{
    int count = g_location_variable_names_006598f8.GetCount();
    int index;

    for (index = 0; index < count; ++index) {
        if (_stricmp(*g_location_variable_names_006598f8.GetAt(index), name) == 0 &&
            *g_location_variable_levels_006598e0.GetAt(index) == g_status_685170.current_level) {
            break;
        }
    }
    if (index >= count) {
        index = -1;
        srAssertFail("iVar != BAD_INDEX", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                     0x10c9, 0);
    }
    if (index < g_location_variable_values_00659990.GetCount()) {
        *g_location_variable_values_00659990.GetAt(index) = value;
    }
}

// VTABLE: WIZ8 0x005ec138
// class W8TriggerActionData

W8TriggerActionData::W8TriggerActionData() : type_004(-1) {}

// SYNTHETIC: WIZ8 0x0043c7f0
// W8TriggerActionData::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00445ee0
W8TriggerActionData::~W8TriggerActionData() {}

/* Type 5 installs 0x005EC148 after constructing the common base. Its deleting
   destructor folds with the type-10 wrapper at 0x00445EC0: both call the
   common destructor, then scalar operator delete when requested. */
// VTABLE: WIZ8 0x005ec148
// class W8EnvironmentTriggerActionData
// SYNTHETIC: WIZ8 0x00445ec0
// W8DoorTriggerActionData::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ec134
// class W8DoorTriggerActionData

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

/* Clear the running bit and run every comma-separated recipient trigger once
   when the link-out and state-gate bits are set. */
// FUNCTION: WIZ8 0x00441590
void Trigger::RunLinkedTriggers00441590()
{
    char* recipient;

    flags_0a0 &= ~W8_TRIGGER_RUNNING;
    if ((flags_0a0 & W8_TRIGGER_FIRE_LINKED) != 0 &&
        (flags_0a0 & W8_TRIGGER_LINK_ON_DEACTIVATE) != 0 && m_pacRecipients != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer_00659908, recipient);
            char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer_00659908);
            if (trigger != 0) {
                trigger->Run(-1);
            }
        }
    }
}

/* Store the trigger position and flag the representation dirty; an item
   representation is moved and re-transformed in place. */
// FUNCTION: WIZ8 0x004416f0
void Trigger::SetPosition004416F0(srVector3T<float>* position)
{
    flags_0a0 |= W8_TRIGGER_POSITIONED;
    position_118.x = position->x;
    position_118.y = position->y;
    position_118.z = position->z;
    if (rep_item_114 != 0 && m_bRepType == 1) {
        rep_item_114->SetLocation0049F720(position);
        rep_item_114->ApplyRepTransform0049FAA0();
    }
}

// FUNCTION: WIZ8 0x004417c0
W8TriggerActionData* LoadTriggerActionData004417C0(int handle)
{
    W8DoorTriggerActionData* data = new W8DoorTriggerActionData;
    data->type_004 = 10;
    data->flags_008 = 0x40;
    data->flags_009 &= ~1;
    data->item_00a = -1;
    data->linked_trigger_00c[0] = 0;
    data->position_08c.SetZero();

    unsigned char version;
    unsigned char flags[9];
    unsigned short item;
    unsigned char has_position;
    srVector3T<float> position;
    char linked_trigger[0x80];
    FileRead(handle, &version, 1, 0);
    for (int index = 0; index < 9; ++index) {
        FileRead(handle, &flags[index], 1, 0);
    }
    FileRead(handle, &item, 2, 0);
    FileRead(handle, &has_position, 1, 0);
    FileRead(handle, &position, sizeof(position), 0);
    position *= 500.0f;
    FileRead(handle, linked_trigger, sizeof(linked_trigger), 0);

    for (int bit = 0; bit < 8; ++bit) {
        if (flags[bit] != 0) {
            data->flags_008 |= 1 << bit;
        } else {
            data->flags_008 &= ~(1 << bit);
        }
    }
    if (flags[8] != 0) {
        data->flags_009 |= 1;
    } else {
        data->flags_009 &= ~1;
    }
    data->item_00a = item;
    strcpy(data->linked_trigger_00c, linked_trigger);
    if (has_position != 0) {
        data->position_08c = position;
        data->flags_009 |= 2;
    }
    return data;
}

// FUNCTION: WIZ8 0x00441a20
Trigger* Trigger::CreateAndLoadLevelTrigger(int handle, W8World* world)
{
    /* Retail read these uninitialised when the FileRead chain short-circuited;
       deterministic zeroes model that defect path. */
    Trigger* trigger = 0;
    unsigned char record_version = 0;
    unsigned char record_type = 0;
    if (handle == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xca3, 0);
    }
    if (FileRead(handle, &record_version, 1, 0) != 0) {
        FileRead(handle, &record_type, 1, 0);
    }

    if (record_type != 3) {
        trigger = new Trigger;
        if (trigger == 0) {
            srAssertFail("pTrigger", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xcb2,
                         "Out of memory - Trigger::CreateAndLoadLevelTrigger");
        }
        trigger->trigger_id_09c = g_status_685170.next_trigger_id_2356++;
        trigger->m_pWorld = world;
        trigger->flags_0a0 = (trigger->flags_0a0 & ~0x20U) | W8_TRIGGER_ON;
    }

    switch (record_type) {
    case 1: {
        unsigned char version;
        int byte_b3;
        int byte_b0;
        float flag_0;
        int range;
        int action;
        int value_ac;
        int flag_1;
        unsigned char packed_flags;
        unsigned char flag_8;
        float minimum_range = 0.0f;
        int action_value = 0;
        char recipients[0x100];
        char sound[0x80];

        FileRead(handle, &version, 1, 0);
        FileRead(handle, &byte_b3, 4, 0);
        FileRead(handle, &byte_b0, 4, 0);
        FileRead(handle, &flag_0, 4, 0);
        FileRead(handle, &range, 4, 0);
        FileRead(handle, &action, 4, 0);
        FileRead(handle, &value_ac, 4, 0);
        FileRead(handle, &flag_1, 4, 0);
        FileRead(handle, &packed_flags, 1, 0);
        FileRead(handle, &flag_8, 1, 0);
        FileRead(handle, trigger->name_01c, sizeof(trigger->name_01c), 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        FileRead(handle, sound, sizeof(sound), 0);
        sprintf(trigger->action_data_128, "data\\sound\\%s", sound);
        _strupr(trigger->name_01c);
        _strupr(recipients);

        trigger->surface_id = -1;
        if (version > 1) {
            char surface_id[0x40];
            FileRead(handle, &minimum_range, 4, 0);
            FileRead(handle, surface_id, sizeof(surface_id), 0);
            if (surface_id[0] == 0 && world->m_owned_04c != 0 &&
                world->m_owned_04c->geometry_index_00 != 0) {
                trigger->surface_id = atoi(surface_id + 1);
            }
        }
        if (version > 2) {
            unsigned char has_action_data;
            FileRead(handle, &has_action_data, 1, 0);
            if (has_action_data != 0) {
                unsigned char action_data_kind;
                FileRead(handle, &action_data_kind, 1, 0);
                if (action_data_kind == 1) {
                    trigger->m_pActionData = LoadTriggerActionData004417C0(handle);
                    trigger->state_index =
                        (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->flags_008 &
                         1) != 0;
                }
            }
        }
        if (version > 3) {
            FileRead(handle, &action_value, 4, 0);
        }

        trigger->trigger_kind_018 = 1;
        trigger->range_maximum_0a8 = range * 500.0f;
        trigger->range_minimum_0a4 = minimum_range * 500.0f;
        trigger->m_pacRecipients = new char[strlen(recipients) + 1];
        strcpy(trigger->m_pacRecipients, recipients);
        trigger->state_count = static_cast<unsigned char>(byte_b0);
        trigger->state_index = 0;
        trigger->state_direction = 1;
        trigger->cycle_bounce = byte_b3;
        trigger->action_value = action_value;
        trigger->initial_action_22a = action;
        if (flag_0 != 0.0f)
            trigger->flags_0a0 |= 1;
        else
            trigger->flags_0a0 &= ~1U;
        if (flag_1 != 0)
            trigger->flags_0a0 |= 2;
        else
            trigger->flags_0a0 &= ~2U;
        if (flag_8 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_ENABLED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        if ((packed_flags & 2) != 0)
            trigger->flags_0a0 |= W8_TRIGGER_LINK_ON_DEACTIVATE;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_LINK_ON_DEACTIVATE;
        if ((packed_flags & 1) != 0)
            trigger->flags_0a0 |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_FIRE_LINKED;
        world->triggers->Add(trigger);
        if (trigger->name_01c[0] != 0) {
            trigger->setName(trigger->name_01c);
        }
        break;
    }

    case 2: {
        unsigned char version;
        float range;
        float x;
        float y;
        float z;
        int action;
        int value_c8;
        unsigned char flag_7;
        unsigned char flag_8;
        char recipients[0x100];
        unsigned char packed_flag = 0;
        unsigned char flag_3 = 0;
        int value_ac = 0;

        FileRead(handle, &version, 1, 0);
        FileRead(handle, &range, 4, 0);
        FileRead(handle, &x, 4, 0);
        FileRead(handle, &y, 4, 0);
        FileRead(handle, &z, 4, 0);
        FileRead(handle, &action, 4, 0);
        FileRead(handle, &value_c8, 4, 0);
        FileRead(handle, &flag_7, 1, 0);
        FileRead(handle, &flag_8, 1, 0);
        FileRead(handle, trigger->name_01c, sizeof(trigger->name_01c), 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        _strupr(trigger->name_01c);
        _strupr(recipients);
        if (version > 1) {
            FileRead(handle, &packed_flag, 1, 0);
            FileRead(handle, trigger->representation_vectors_0cc,
                     sizeof(trigger->representation_vectors_0cc), 0);
            for (int vector = 0; vector < 4; ++vector) {
                trigger->representation_vectors_0cc[vector].x *= 500.0f;
                trigger->representation_vectors_0cc[vector].y *= 500.0f;
                trigger->representation_vectors_0cc[vector].z *= 500.0f;
            }
        }
        if (version > 2) {
            unsigned char unused;
            char action_string[0x80];
            FileRead(handle, &trigger->angle_0fc, 4, 0);
            FileRead(handle, &trigger->direction_100.x, 4, 0);
            FileRead(handle, &trigger->direction_100.y, 4, 0);
            FileRead(handle, &trigger->direction_100.z, 4, 0);
            FileRead(handle, &unused, 1, 0);
            FileRead(handle, action_string, sizeof(action_string), 0);
            if (action == 17) {
                W8TriggerActionData005EC158* data = new W8TriggerActionData005EC158;
                data->type_004 = 6;
                data->owned_string_008 = 0;
                delete trigger->m_pActionData;
                data->owned_string_008 = new char[strlen(action_string) + 1];
                strcpy(data->owned_string_008, action_string);
                trigger->m_pActionData = data;
            }
        }
        if (version > 3) {
            FileRead(handle, &flag_3, 1, 0);
            FileRead(handle, &value_ac, 4, 0);
        }
        if (version > 4) {
            unsigned char has_legacy_geometry;
            FileRead(handle, &has_legacy_geometry, 1, 0);
            if (has_legacy_geometry != 0) {
                unsigned char geometry_kind;
                FileRead(handle, &geometry_kind, 1, 0);
                if (geometry_kind == 2) {
                    unsigned char count;
                    srVector3T<float> legacy_vertices[36];
                    unsigned char legacy_flags[2];
                    FileRead(handle, &count, 1, 0);
                    for (int index = 0; index < 36; ++index) {
                        FileRead(handle, &legacy_vertices[index], sizeof(legacy_vertices[index]),
                                 0);
                        legacy_vertices[index].x *= 500.0f;
                        legacy_vertices[index].y *= 500.0f;
                        legacy_vertices[index].z *= 500.0f;
                    }
                    FileRead(handle, &legacy_flags[0], 1, 0);
                    FileRead(handle, &legacy_flags[1], 1, 0);
                }
            }
        }

        trigger->trigger_kind_018 = 2;
        trigger->position_118.x = x * 500.0f;
        trigger->position_118.y = y * 500.0f;
        trigger->position_118.z = z * 500.0f;
        trigger->range_maximum_0a8 = range * 500.0f;
        trigger->m_bRepType = 3;
        trigger->action_value = value_ac;
        trigger->initial_action_22a = static_cast<unsigned short>(action);
        trigger->searchable = value_c8;
        trigger->flags_0a0 |= W8_TRIGGER_POSITIONED;
        if (flag_7 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_FIRE_LINKED;
        if (flag_8 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_ENABLED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        if (flag_3 != 0)
            trigger->flags_0a0 |= 8;
        else
            trigger->flags_0a0 &= ~8U;
        if (packed_flag == 1)
            trigger->flags_0a0 |= 4;
        trigger->m_pacRecipients = new char[strlen(recipients) + 1];
        strcpy(trigger->m_pacRecipients, recipients);
        if ((trigger->flags_0a0 & 4) != 0 && world->m_owned_04c != 0 &&
            world->m_owned_04c->geometry_index_00 != 0) {
            world->m_owned_04c->AddTriggerPlane(trigger->representation_vectors_0cc, trigger);
        }
        if (trigger->initial_action_22a == 0x34 && trigger->m_pacRecipients[0] == 0) {
            trigger->range_minimum_0a4 = 0.0f;
            trigger->range_maximum_0a8 = 0.0f;
        }
        world->triggers->Add(trigger);
        if (trigger->name_01c[0] != 0) {
            trigger->setName(trigger->name_01c);
        }
        break;
    }

    case 3: {
        unsigned char version;
        int volume_min, volume_max, speed_min, speed_max, time_min, time_max;
        int unbounded;
        float radius;
        srVector3T<float> position, region_u, region_v;
        srVector3T<float> region_center;
        float region_angle = 0.0f;
        srVector3T<float> region_min;
        srVector3T<float> region_max;
        char wave[0x80];
        char name[0x80];
        const char* optional_name = 0;
        unsigned char looping = 0;
        unsigned char shared = 0;

        region_center.x = region_center.y = region_center.z = 0.0f;
        region_min.x = region_min.y = region_min.z = 0.0f;
        region_max.x = region_max.y = region_max.z = 0.0f;

        FileRead(handle, &version, 1, 0);
        FileRead(handle, &volume_min, 4, 0);
        FileRead(handle, &volume_max, 4, 0);
        FileRead(handle, &speed_min, 4, 0);
        FileRead(handle, &speed_max, 4, 0);
        FileRead(handle, &time_min, 4, 0);
        FileRead(handle, &time_max, 4, 0);
        FileRead(handle, &unbounded, 4, 0);
        FileRead(handle, &radius, 4, 0);
        FileRead(handle, &position, sizeof(position), 0);
        FileRead(handle, &region_u, sizeof(region_u), 0);
        FileRead(handle, &region_v, sizeof(region_v), 0);
        FileRead(handle, wave, sizeof(wave), 0);
        W8AmbientSoundConfig config;
        sprintf(config.wave_name, "data\\sound\\%s", wave);
        if (version > 1) {
            unsigned char has_position;
            FileRead(handle, &has_position, 1, 0);
            FileRead(handle, &looping, 1, 0);
        }
        if (version > 2) {
            FileRead(handle, &region_center, sizeof(region_center), 0);
            FileRead(handle, &region_angle, 4, 0);
            FileRead(handle, &region_min, sizeof(region_min), 0);
            FileRead(handle, &region_max, sizeof(region_max), 0);
            region_center *= 500.0f;
        }
        if (version > 3) {
            FileRead(handle, name, sizeof(name), 0);
            optional_name = name;
        }
        if (version > 4) {
            FileRead(handle, &shared, 1, 0);
        }
        position.x *= 500.0f;
        position.y *= 500.0f;
        position.z *= 500.0f;
        region_u.x *= 500.0f;
        region_u.y *= 500.0f;
        region_u.z *= 500.0f;
        region_v.x *= 500.0f;
        region_v.y *= 500.0f;
        region_v.z *= 500.0f;
        radius *= 500.0f;
        AddAmbientSound0047A790(world, optional_name, &config, &position, &region_u, &region_v,
                                volume_min, volume_max, speed_min, speed_max, time_min, time_max,
                                radius, unbounded == 0, looping, &region_center, region_angle,
                                &region_min, &region_max, shared);
        return 0;
    }

    case 4: {
        unsigned char version;
        unsigned char packed_flags;
        unsigned char flag_8;
        unsigned char flag_7;
        unsigned char flag_9;
        unsigned char flag_3;
        unsigned char flag_12;
        unsigned char flag_15;
        int initial_action;
        int alternate_action;
        int fallback_action;
        char recipients[0x100];
        unsigned char searchable;
        char location_variable[0x100];
        unsigned char flag_16;
        int action_value;
        unsigned char flag_1;
        char sound[0x80];
        float representation_scale = 1.0f;
        unsigned char initial_location_value = 0;
        unsigned char flag_23 = 0;
        unsigned char representation_kind = 0;

        FileRead(handle, &version, 1, 0);
        FileRead(handle, trigger->name_01c, sizeof(trigger->name_01c), 0);
        FileRead(handle, &packed_flags, 1, 0);
        FileRead(handle, &flag_8, 1, 0);
        FileRead(handle, &flag_7, 1, 0);
        FileRead(handle, &flag_9, 1, 0);
        FileRead(handle, &flag_3, 1, 0);
        FileRead(handle, &flag_12, 1, 0);
        FileRead(handle, &flag_15, 1, 0);
        FileRead(handle, &initial_action, 4, 0);
        FileRead(handle, &alternate_action, 4, 0);
        FileRead(handle, &fallback_action, 4, 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        FileRead(handle, &searchable, 1, 0);
        FileRead(handle, location_variable, sizeof(location_variable), 0);
        FileRead(handle, &flag_16, 1, 0);
        FileRead(handle, &action_value, 4, 0);
        FileRead(handle, &flag_1, 1, 0);
        FileRead(handle, sound, sizeof(sound), 0);
        sprintf(trigger->action_data_128, "data\\sound\\%s", sound);
        _strupr(trigger->name_01c);
        _strupr(recipients);
        _strupr(location_variable);

        if (version > 1) {
            FileRead(handle, &trigger->m_lData1, 4, 0);
            FileRead(handle, &trigger->m_lData2, 4, 0);
            FileRead(handle, &trigger->m_lData3, 4, 0);
            FileRead(handle, &representation_scale, 4, 0);
            FileRead(handle, &initial_location_value, 1, 0);
            FileRead(handle, &flag_23, 1, 0);
            FileRead(handle, &trigger->action_data_mode_228, 1, 0);
            FileRead(handle, &trigger->sound_volume, 1, 0);
            int unused;
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
        }

        trigger->trigger_kind_018 = ((packed_flags & 1) != 0 || searchable == 1) ? 1 : 2;
        if ((packed_flags & 2) != 0)
            trigger->flags_0a0 |= W8_TRIGGER_CAN_RUN_LINKED;
        if ((packed_flags & 4) != 0)
            trigger->flags_0a0 |= 0x40000;
        if ((packed_flags & 8) != 0)
            trigger->flags_0a0 |= W8_TRIGGER_EXCLUSIVE;
        if ((packed_flags & 0x10) != 0)
            trigger->flags_0a0 |= W8_TRIGGER_REACTIVATE_LINKED;
        if ((packed_flags & 0x20) != 0)
            trigger->flags_0a0 |= 0x400000;

        if (recipients[0] != 0) {
            trigger->m_pacRecipients = new char[strlen(recipients) + 1];
            if (trigger->m_pacRecipients == 0) {
                srAssertFail("pTrigger->m_pacRecipients",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xe48,
                             "Out of memory - Trigger.cpp");
            }
            strcpy(trigger->m_pacRecipients, recipients);
        }
        trigger->required_item_id = location_variable[0] == 0 ? -1 : atoi(location_variable);
        trigger->alternate_action = static_cast<unsigned short>(alternate_action);
        trigger->initial_action_22a = static_cast<unsigned short>(initial_action);
        trigger->fallback_action_22e = static_cast<unsigned short>(fallback_action);
        trigger->searchable = searchable;
        if (flag_8 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_ENABLED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_ENABLED;
        if (flag_7 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_FIRE_LINKED;
        if (flag_9 != 0)
            trigger->flags_0a0 |= W8_TRIGGER_LINK_ON_DEACTIVATE;
        else
            trigger->flags_0a0 &= ~W8_TRIGGER_LINK_ON_DEACTIVATE;
        if (flag_3 != 0)
            trigger->flags_0a0 |= 8;
        else
            trigger->flags_0a0 &= ~8U;
        if (flag_12 != 0)
            trigger->flags_0a0 |= 0x1000;
        else
            trigger->flags_0a0 &= ~0x1000U;
        if (flag_16 != 0)
            trigger->flags_0a0 |= 0x10000;
        else
            trigger->flags_0a0 &= ~0x10000U;
        if (flag_1 != 0)
            trigger->flags_0a0 |= 2;
        else
            trigger->flags_0a0 &= ~2U;
        if (flag_15 != 0)
            trigger->flags_0a0 |= 0x8000;
        else
            trigger->flags_0a0 &= ~0x8000U;
        if (trigger->alternate_action != 0)
            trigger->flags_0a0 |= 0x2000;

        unsigned char value_b0;
        unsigned char flag_0;
        unsigned char value_b3;
        char required_states[0x100];
        char state_to_modify[0x100];
        unsigned char value_b4;
        FileRead(handle, &value_b0, 1, 0);
        FileRead(handle, &flag_0, 1, 0);
        FileRead(handle, &value_b3, 1, 0);
        FileRead(handle, required_states, sizeof(required_states), 0);
        FileRead(handle, state_to_modify, sizeof(state_to_modify), 0);
        FileRead(handle, &value_b4, 1, 0);
        _strupr(required_states);
        _strupr(state_to_modify);
        if (required_states[0] != 0) {
            trigger->m_pacRequiredStates = new char[strlen(required_states) + 1];
            if (trigger->m_pacRequiredStates == 0) {
                srAssertFail("pTrigger->m_pacRequiredStates",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xe6f,
                             "Out of memory - Trigger.cpp");
            }
            strcpy(trigger->m_pacRequiredStates, required_states);
        }
        if (state_to_modify[0] != 0) {
            trigger->m_pacStateToMod = new char[strlen(state_to_modify) + 1];
            if (trigger->m_pacStateToMod == 0) {
                srAssertFail("pTrigger->m_pacStateToMod",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xe75,
                             "Out of memory - Trigger.cpp");
            }
            strcpy(trigger->m_pacStateToMod, state_to_modify);
        }
        trigger->state_count = value_b0;
        trigger->cycle_bounce = value_b3;
        trigger->state_mod_mode = value_b4;
        if (flag_0 != 0)
            trigger->flags_0a0 |= 1;
        else
            trigger->flags_0a0 &= ~1U;

        if (trigger->m_pacStateToMod != 0 &&
            (initial_location_value == 0 || initial_location_value == 1)) {
            int variable_id = 0;
            while (variable_id < g_location_variable_names_006598f8.GetCount()) {
                if (_stricmp(*g_location_variable_names_006598f8.GetAt(variable_id),
                             trigger->m_pacStateToMod) == 0 &&
                    *g_location_variable_levels_006598e0.GetAt(variable_id) ==
                        g_status_685170.current_level) {
                    break;
                }
                ++variable_id;
            }
            if (variable_id == g_location_variable_names_006598f8.GetCount()) {
                char* variable_name = new char[strlen(trigger->m_pacStateToMod) + 1];
                if (variable_name == 0) {
                    srAssertFail("pacVariableName",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x109c, 0);
                }
                strcpy(variable_name, trigger->m_pacStateToMod);
                g_location_variable_names_006598f8.Add(variable_name);
                g_location_variable_values_00659990.Add(initial_location_value);
                g_location_variable_levels_006598e0.Add(g_status_685170.current_level);
            }
        }

        int unused_value;
        FileRead(handle, &trigger->range_minimum_0a4, 4, 0);
        FileRead(handle, &trigger->range_maximum_0a8, 4, 0);
        FileRead(handle, trigger->inline_action_data_24c, sizeof(trigger->inline_action_data_24c),
                 0);
        FileRead(handle, &unused_value, 4, 0);
        _strupr(trigger->inline_action_data_24c);
        trigger->range_minimum_0a4 *= 500.0f;
        trigger->range_maximum_0a8 *= 500.0f;
        if (version > 2) {
            FileRead(handle, sound, sizeof(sound), 0);
            sprintf(trigger->alternate_action_data_1a8, "data\\sound\\%s", sound);
            if (flag_23 != 0)
                trigger->flags_0a0 |= W8_TRIGGER_ALTERNATE_ACTION;
            else
                trigger->flags_0a0 &= ~W8_TRIGGER_ALTERNATE_ACTION;
        }

        if ((packed_flags & 1) == 0) {
            FileRead(handle, &representation_kind, 1, 0);
            if (representation_kind == 1) {
                FileRead(handle, &trigger->position_118, sizeof(srVector3T<float>), 0);
                FileRead(handle, &trigger->angle_0fc, 4, 0);
                FileRead(handle, &trigger->direction_100, sizeof(srVector3T<float>), 0);
                trigger->position_118.x *= 500.0f;
                trigger->position_118.y *= 500.0f;
                trigger->position_118.z *= 500.0f;
                trigger->flags_0a0 |= W8_TRIGGER_POSITIONED;
            } else if (representation_kind == 2) {
                FileRead(handle, trigger->representation_vectors_0cc,
                         sizeof(trigger->representation_vectors_0cc), 0);
                for (int vector = 0; vector < 4; ++vector) {
                    trigger->representation_vectors_0cc[vector].x *= 500.0f;
                    trigger->representation_vectors_0cc[vector].y *= 500.0f;
                    trigger->representation_vectors_0cc[vector].z *= 500.0f;
                }
            }
            unsigned char has_legacy_action;
            FileRead(handle, &has_legacy_action, 1, 0);
            if (has_legacy_action != 0) {
                unsigned char legacy_kind;
                float legacy_value;
                FileRead(handle, &legacy_kind, 1, 0);
                FileRead(handle, &legacy_value, 4, 0);
                FileRead(handle, sound, sizeof(sound), 0);
            }
        }

        unsigned char has_action_data;
        FileRead(handle, &has_action_data, 1, 0);
        if (has_action_data != 0) {
            unsigned char action_data_kind;
            FileRead(handle, &action_data_kind, 1, 0);
            if (action_data_kind == 1) {
                trigger->m_pActionData = LoadTriggerActionData004417C0(handle);
                trigger->state_index =
                    (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->flags_008 &
                     1) != 0;
            } else if (action_data_kind == 2) {
                unsigned char count;
                srVector3T<float> legacy_vertices[36];
                unsigned char legacy_flags[2];
                FileRead(handle, &count, 1, 0);
                for (int index = 0; index < 36; ++index) {
                    FileRead(handle, &legacy_vertices[index], sizeof(legacy_vertices[index]), 0);
                    legacy_vertices[index].x *= 500.0f;
                    legacy_vertices[index].y *= 500.0f;
                    legacy_vertices[index].z *= 500.0f;
                }
                FileRead(handle, &legacy_flags[0], 1, 0);
                FileRead(handle, &legacy_flags[1], 1, 0);
            }
        }

        if (representation_kind == 2 && world->m_owned_04c != 0 &&
            world->m_owned_04c->geometry_index_00 != 0) {
            world->m_owned_04c->AddTriggerPlane(trigger->representation_vectors_0cc, trigger);
        }
        if (trigger->initial_action_22a == 0x34 && trigger->m_pacRecipients == 0) {
            trigger->range_minimum_0a4 = 0.0f;
            trigger->range_maximum_0a8 = 0.0f;
        }
        world->triggers->Add(trigger);
        if (trigger->name_01c[0] != 0) {
            trigger->setName(trigger->name_01c);
        }
        if (searchable == 1) {
            RegisterSearchableTrigger00516F00(trigger);
        }
        if (trigger->initial_action_22a == 0x0c) {
            if (trigger->m_lData1 < 0 ||
                trigger->m_lData1 >= static_cast<int>(g_missile_table_count_65bddc)) {
                srAssertFail(
                    "((pTrigger->m_lData1 >= 0) && (pTrigger->m_lData1 < Missile::GetNumTypes()))",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf14,
                    reinterpret_cast<const char*>(String(
                        "Trigger %s: You must enter a valid missile number", trigger->name_01c)));
            }
            if (trigger->m_lData2 == -1) {
                srAssertFail(
                    "(pTrigger->m_lData2!=(-1))",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf15,
                    reinterpret_cast<const char*>(String(
                        "Trigger %s: You must enter a time value in Data2", trigger->name_01c)));
            }
            if (trigger->m_lData2 < 0) {
                trigger->m_pEvent = new W8TriggerEvent;
                if (trigger->m_pEvent == 0) {
                    srAssertFail("pTrigger->m_pEvent",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf1e, 0);
                }
                trigger->m_pEvent->action_004 = static_cast<short>(trigger->initial_action_22a);
                trigger->m_pEvent->timer_008.SetDuration(
                    static_cast<float>(abs(trigger->m_lData2)) * 0.001f);
                trigger->m_pEvent->timer_008.Restart();
                trigger->m_pEvent->trigger_030 = trigger;
                trigger->flags_0a0 |= W8_TRIGGER_RUNNING;
                g_timed_events_006599b8.Add(trigger->m_pEvent);
            }
        } else if (trigger->initial_action_22a > 0x24 && trigger->initial_action_22a < 0x2c &&
                   trigger->m_lData1 >= 0) {
            trigger->uses_remaining = trigger->m_lData1;
        }
        break;
    }
    }
    return trigger;
}

// VTABLE: WIZ8 0x005ec0e4
// class Trigger

// FUNCTION: WIZ8 0x00441750
void Trigger::GetPosition(srVector3T<float>* position) const
{
    position->x = position_118.x;
    position->y = position_118.y;
    position->z = position_118.z;
}

// FUNCTION: WIZ8 0x00441780
bool Trigger::HasActionMessage00441780()
{
    return (flags_0a0 & W8_TRIGGER_CAN_RUN_LINKED) != 0;
}

// FUNCTION: WIZ8 0x00441790
bool Trigger::RequiresItem00441790()
{
    if (required_item_id >= 0) {
        return true;
    }
    if (m_pActionData != 0 && m_pActionData->type_004 == 0xa && m_pActionData != 0 &&
        static_cast<W8DoorTriggerActionData*>(m_pActionData)->item_00a != -1) {
        return true;
    }
    return false;
}

// VTABLE: WIZ8 0x005ec104
// class srClassSupport<Trigger,srClass,1,65544>

// FUNCTION: WIZ8 0x0043ba10
Trigger::Trigger()
{
    trigger_kind_018 = 0;
    trigger_id_09c = 0;
    flags_0a0 = 0;
    range_minimum_0a4 = 0.0f;
    range_maximum_0a8 = 0.0f;
    action_value = 0;
    state_count = 0;
    state_index = 0;
    state_direction = 0;
    cycle_bounce = 0;
    state_mod_mode = 0;
    searchable = 0;
    angle_0fc = 0.0f;
    m_bRepType = 0;
    m_pProp = 0;
    rep_item_114 = 0;
    m_pWorld = 0;
    initial_action_22a = 0;
    alternate_action = 0;
    fallback_action_22e = 0;
    action_230 = 0;
    action_state_232 = 1;
    m_pActionData = 0;
    m_pacRecipients = 0;
    m_pacRequiredStates = 0;
    m_pacStateToMod = 0;
    m_pEvent = 0;
    world_item_group_34c = 0;
    items_generated = 0;
    activation_callback_360 = 0;
    running = 0;

    surface_id = -1;
    sound_volume = -1;
    required_item_id = -1;
    device_id = -1;
    key_id = -1;
    last_interaction_clock = -1;
    lock_type = 0;
    difficulty = 0;
    device_state.completed = 0;
    lock_countdown = 0;
    memset(device_state.pins, 0, sizeof(device_state.pins));

    flags_0a0 |= W8_TRIGGER_ON;
    name_01c[0] = 0;
    position_118.x = 0.0f;
    position_118.y = 0.0f;
    position_118.z = 0.0f;
    action_data_128[0] = 0;
    item_group_seed_354 = GetTickCount() + Random(30000);
    gold_358 = 0;
    uses_remaining = 0;
    trigger_id_09c = g_status_685170.next_trigger_id_2356++;
}

// FUNCTION: WIZ8 0x00440d00
void Trigger::UpdateActionAnimation()
{
    char* action_data = action_data_128;

    if ((flags_0a0 & 0x2U) == 0 && (flags_0a0 & W8_TRIGGER_ALTERNATE_ACTION) == 0) {
        return;
    }
    if ((flags_0a0 & W8_TRIGGER_ALTERNATE_ACTION) != 0) {
        if (action_data_mode_228 == 0) {
            if ((flags_0a0 & W8_TRIGGER_ALTERNATE_SELECTED) == 0) {
                flags_0a0 |= W8_TRIGGER_ALTERNATE_SELECTED;
            } else {
                action_data = alternate_action_data_1a8;
                flags_0a0 &= ~W8_TRIGGER_ALTERNATE_SELECTED;
            }
        } else if (action_data_mode_228 == 1) {
            if (action_230 == alternate_action) {
                PlayActionSound(alternate_action_data_1a8, sound_volume);
                return;
            }
        } else if (action_data_mode_228 == 2 && action_230 == fallback_action_22e) {
            PlayActionSound(alternate_action_data_1a8, sound_volume);
            return;
        }
    }
    PlayActionSound(action_data, sound_volume);
}

// FUNCTION: WIZ8 0x00441110
void Trigger::FinishAction()
{
    bool was_running = ((flags_0a0 & W8_TRIGGER_RUNNING) != 0);
    bool action_completed = false;
    char* recipient;

    flags_0a0 &= ~W8_TRIGGER_RUNNING;

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
        action_completed = true;
        break;

    case 0x23: {
        int index = g_timed_events_006599b8.IndexOf(m_pEvent);
        if (index >= 0) {
            g_timed_events_006599b8.RemoveAt(index);
        }
        action_completed = true;
        break;
    }

    case 0x39:
        if (m_pEvent != 0) {
            m_pEvent->completed_035 = 1;
        }
        g_trigger_action_active_006599c8 = 0;
        action_completed = true;
        break;
    }

    if ((flags_0a0 & 0x8U) == 0 && action_230 != 0) {
        if (action_230 == 4) {
            recipient = m_pacRecipients;
            action_completed = false;
            while (recipient != 0) {
                strcpy(g_trigger_parse_buffer_00659908, recipient);
                char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
                if (comma == 0) {
                    recipient = 0;
                } else {
                    recipient = strchr(recipient, ',') + 1;
                    *comma = '\0';
                }

                stLight* light = FindLightByName00445A10(g_trigger_parse_buffer_00659908, 0);
                if (light != 0) {
                    light->m_save_marked_23a = 1;
                    if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                        light->setFlag(srNode::FLAG_DISABLE);
                    } else {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                    action_completed = true;
                }
            }
        } else if (action_230 == 0x22) {
            if (m_pActionData == 0) {
                srAssertFail("m_pActionData", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             2822, "Trigger.cpp: Dark Area doesn't have action data");
            }
            SetWorldEnvironmentValue00483AE0(
                g_world, static_cast<W8EnvironmentTriggerActionData*>(m_pActionData)
                             ->previous_environment_008);
            delete m_pActionData;
            m_pActionData = 0;
            goto finish_linked_triggers;
        }

        if (!action_completed) {
            goto reactivate_linked_triggers;
        }
    }

finish_linked_triggers:
    if ((flags_0a0 & W8_TRIGGER_FIRE_LINKED) != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer_00659908, recipient);
            char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer_00659908);
            if (trigger != 0) {
                trigger->FinishAction();
            }
        }
    }

reactivate_linked_triggers:
    if (was_running != 0 && (flags_0a0 & W8_TRIGGER_ON) != 0 &&
        (flags_0a0 & W8_TRIGGER_REACTIVATE_LINKED) != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer_00659908, recipient);
            char* comma = strchr(g_trigger_parse_buffer_00659908, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer_00659908);
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
    } else {
        *cursor = 0;
    }
    return g_trigger_parse_buffer_00659908;
}

// FUNCTION: WIZ8 0x004409b0
void Trigger::CommitActionResult(bool apply_state_changes)
{
    char* recipient;

    UpdateActionAnimation();

    if (m_pacRecipients != 0 && (flags_0a0 & W8_TRIGGER_FIRE_LINKED) != 0 &&
        (flags_0a0 & W8_TRIGGER_LINK_ON_DEACTIVATE) == 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            Trigger* trigger = FindTriggerByName(NextTriggerRecipient(&recipient));
            if (trigger != 0) {
                bool was_running = ((flags_0a0 & W8_TRIGGER_RUNNING) != 0);
                flags_0a0 |= W8_TRIGGER_RUNNING;
                trigger->Run(m_lData1);
                flags_0a0 =
                    (flags_0a0 & ~W8_TRIGGER_RUNNING) | (was_running != 0 ? W8_TRIGGER_RUNNING : 0);
            }
        }
    }

    if (m_pacStateToMod != 0 && apply_state_changes) {
        int state_id;
        int state_value;

        if (state_mod_mode == 1) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4297, 0);
            }
            state_value = 1;
        } else if (state_mod_mode == 2) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4297, 0);
            }
            state_value = 0;
        } else if (state_mod_mode == 3) {
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4317, 0);
            }
            state_value = *g_location_variable_values_00659990.GetAt(state_id) == 0;
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4297, 0);
            }
        } else {
            goto show_action_message;
        }
        g_location_variable_values_00659990.SetAt(state_id, state_value);
    }

show_action_message:
    if ((flags_0a0 & W8_TRIGGER_CAN_RUN_LINKED) != 0) {
        const char* level_folder = GetLevelFolderName(GetLoadedLevelID());
        int message_id;

        if (action_state_232 == 2) {
            message_id = m_lData1;
        } else if (action_state_232 == 3) {
            message_id = m_lData2;
        } else if (action_state_232 == 4) {
            message_id = m_lData3;
        } else {
            goto action_complete;
        }

        if (message_id != -1) {
            char path[512];
            wchar_t text[1996];

            if (level_folder == 0) {
                level_folder = "";
            }
            sprintf(path, "Data\\Messages\\%s.msg", level_folder);
            if (GetStringFromStringDatabase(path, message_id, text, 0, 0) != 0) {
                ShowString(text);
            }
        }
    }

action_complete:
    flags_0a0 |= 0x80000U;
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
    /* Retail left location_id/entrance uninitialised on the named-entity path
       and read the stack slot holding `this`. Named entities only resolve in
       the loaded world, so the deterministic model of the intended same-level
       move is the current level. */
    int location_id = g_status_685170.current_level;
    int entrance = 0;
    int current_location;
    float entity_value;
    float angle;
    bool named_entity;

    if (g_modal_owner_0068edd0 != 0) {
        return;
    }

    ResetInactiveLevelDataVectors0041EF50();
    current_location = g_status_685170.current_level;
    named_entity =
        FindEntityByName(destination, &destination_position, &entity_value, &destination_direction);
    /* Retail leaves location_id and entrance uninitialised on the named-entity
       path. Both GOG builds then read the stack slot holding this for those
       values. Preserve that source bug rather than assigning entity_value and
       silently making the path behave differently. */
    if (!named_entity) {
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
    ResetInactiveLevelDataVectors0041EF50();

    if (location_id != current_location) {
        RequestLevelTransition005615F0(location_id, entrance,
                                       m_lData1 < 0 ? 0 : (unsigned char)m_lData1);
        return;
    }

    if (!named_entity) {
        Trigger* target = FindTriggerByName(destination);

        destination_position.x = target->position_118.x;
        destination_position.y = target->position_118.y;
        destination_position.z = target->position_118.z;
        angle = target->angle_0fc;
        destination_direction.x = target->direction_100.x;
        destination_direction.y = target->direction_100.y;
        destination_direction.z = target->direction_100.z;
    } else {
        angle = 0.0f;
    }

    source_position.Set(position_118.x, position_118.y, position_118.z);
    g_octree_6598a4->AdjustPortalDestination(&destination_position, &source_position);
    SetWorldScenePosition004511D0(GetWorld(), &destination_position);

    rotation.SetIdentity();
    if (angle != 0.0f) {
        rotation.RotateAroundAxis(sin(angle), cos(angle), destination_direction);
    }
    ApplyCameraRotation(&rotation);
    SpawnCameraSpellEffect("set_portal", 1, 0, 0);
}

/* Materialize this trigger's item table once. The two dice fields are the
   item-count and gold rolls at ItemTable record offsets 0x1cd and 0x1d5.
   The trigger's persistent seed reseeds the CRT RNG before generation. */
// FUNCTION: WIZ8 0x00445500
void Trigger::GenerateItemGroup()
{
    W8GrowableVector<W8WorldItem*> items(5);
    srVector3T<float> position;
    unsigned int table_id;
    unsigned int maximum_items;
    int index;

    if (inline_action_data_24c[0] == '\0' || items_generated != 0) {
        return;
    }

    srand(item_group_seed_354);
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
    items_generated = 1;
}

/* The trigger's container world item, materialized on demand: when asked to
   create and none exists yet a bare container item is spawned into the world
   and remembered. */
// FUNCTION: WIZ8 0x00445670
W8WorldItem* Trigger::GetOrCreateItemGroup00445670(char create)
{
    srVector3T<float> position;

    if (create != 0 && world_item_group_34c == 0) {
        world_item_group_34c = SpawnItem(0x23c, &position, 0, 0);
    }
    return world_item_group_34c;
}

/* After a selected-prop Run: while g_trigger_feedback_00606994 is clear, post either the
   special-item notice (required_item_id != -1) or the nothing-happened notice. */
// FUNCTION: WIZ8 0x004456E0
void Trigger::PrintNothingHappenedOrSpecialItemRequired004456E0()
{
    if (g_trigger_feedback_00606994 != 0) {
        return;
    }
    if (required_item_id != -1) {
        ShowNotice(0xf, gppStringList[0x96b], -1, -1, 0);
        return;
    }
    ShowNotice(0xf, gppStringList[0x964], -1, -1, 0);
}

/* Execute the selected Trigger action. The original keeps the three trigger
   kinds in one dispatcher: kind two handles invisible/timed actions first,
   kind one handles Prop-backed actions, and everything else falls through to
   the common action table. */
// FUNCTION: WIZ8 0x0043d940
void Trigger::Run(int source)
{
    ActivationCallback callback = activation_callback_360;
    bool apply_state_changes = true;
    bool action_succeeded = false;

    if (!SelectAction()) {
        return;
    }
    if (callback != 0 && !callback(this)) {
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
            UpdateCameraPathStateByName(m_pWorld, m_pacRecipients, 1);
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
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch0 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch1");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch1 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch2");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch2 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch3");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch3 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch4");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch4 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch5");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch5 = *g_location_variable_values_00659990.GetAt(id);
            id = GetLocationVarIDByName("Switch6");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch6 = *g_location_variable_values_00659990.GetAt(id);

            if (switch0 == 1 && switch1 == 1) {
                camera = 2;
            } else if (switch0 == 0 && switch1 == 1) {
                if (switch4 == 0 && switch6 == 1) {
                    camera = 1;
                } else if (switch3 == 1 && switch4 == 1) {
                    camera = 3;
                } else if (switch4 == 0 && switch6 == 0) {
                    camera = 4;
                } else if (switch3 == 0 && switch4 == 1) {
                    camera = 5;
                }
            } else if (switch1 == 0) {
                if (switch2 == 1) {
                    if (switch3 == 1 && switch5 == 1) {
                        camera = 6;
                    } else if (switch5 == 0 && switch6 == 0) {
                        camera = 7;
                    } else if (switch3 == 0 && switch5 == 1) {
                        camera = 9;
                    } else if (switch5 == 0 && switch6 == 1) {
                        camera = 10;
                    }
                } else if (switch2 == 0) {
                    camera = 8;
                }
            }

            if (camera != -1) {
                char name[24];

                sprintf(name, "Camera0%d", camera);
                UpdateCameraPathStateByName(m_pWorld, name, 1);
            }
            FinishAction();
            goto commit_action;
        }

        case 0x0e:
            if (m_pacRecipients == 0 || _stricmp(m_pacRecipients, "party") != 0) {
                break;
            }
            ApplyItemEffectToRandomCharacter(Random(2) != 0 ? g_condition_reaction_005ee59c
                                                            : g_condition_reaction_alt_005ee5a0,
                                             -1, 0, g_effect_argument_005ed8c8);
            flags_0a0 |= W8_TRIGGER_RUNNING;
            goto commit_action;

        case 0x11:
            if ((flags_0a0 & W8_TRIGGER_RUNNING) != 0) {
                break;
            }
            flags_0a0 |= W8_TRIGGER_RUNNING;
            goto commit_action;

        case 0x22: {
            float previous_value = GetWorldValue24(g_world);

            delete m_pActionData;
            m_pActionData = new W8EnvironmentTriggerActionData;
            m_pActionData->type_004 = 5;
            static_cast<W8EnvironmentTriggerActionData*>(m_pActionData)->previous_environment_008 =
                previous_value;
            SetWorldEnvironmentValue00483AE0(g_world, 0.0f);
            flags_0a0 |= W8_TRIGGER_RUNNING;
            goto commit_action;
        }

        case 0x23:
            if (m_pEvent == 0) {
                m_pEvent = new W8TriggerEvent;
                if (m_pEvent == 0) {
                    srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                                 0x5fa, 0);
                }
                m_pEvent->trigger_030 = this;
                m_pEvent->action_004 = (short)action_230;
                m_pEvent->timer_008.SetDuration(0.5f);
            } else {
                if (g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                    srAssertFail("glsTimedEvents.Find(m_pEvent) == -1",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x603, 0);
                }
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
            }
            g_timed_events_006599b8.Add(m_pEvent);
            flags_0a0 |= W8_TRIGGER_RUNNING;
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
            W8DoorTriggerActionData* action_data = 0;

            if (m_bRepType != 2 || m_pProp == 0 || state_index != 0 ||
                m_pProp->Rep()->animation_playing_06d != 0) {
                break;
            }
            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->flags_008 & 4) != 0 &&
                action_data->item_00a != -1) {
                if (FindItemOnParty(action_data->item_00a, 0, 0, 2, 0) == 0) {
                    return;
                }
                action_data->flags_008 &= ~4;
            }

            m_pProp->SetRepresentationActive(1, 1);
            state_index = 1;
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
                m_pWorld->m_owned_04c->SetInterfaceState(surface_id, 1);
            }
            flags_0a0 |= W8_TRIGGER_RUNNING;
            if (action_data != 0) {
                action_data->flags_008 |= 1;
            }

            if (m_lData1 != 0) {
                if (m_pEvent != 0 && g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                    m_pEvent->timer_008.Restart();
                    if (m_pEvent->m_pCountdown != 0) {
                        m_pEvent->m_pCountdown->Restart();
                    }
                    goto commit_action;
                }

                m_pEvent = new W8TriggerEvent;
                m_pEvent->trigger_030 = this;
                m_pEvent->action_004 = 2;
                m_pEvent->timer_008.SetDuration(m_lData1 < 0 ? 10.0f : (float)m_lData1);
                m_pEvent->timer_008.Restart();
                m_pEvent->repeat_034 = 1;
                g_timed_events_006599b8.Add(m_pEvent);
            }
            goto commit_action;
        }

        case 2: {
            bool active;

            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            active = m_pProp->Rep()->animation_playing_06d;
            if (active) {
                break;
            }
            state_index = state_index == 1 ? 0 : 1;
            m_pProp->SetRepresentationActive(state_index, 1);
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
                m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
            }
            flags_0a0 |= W8_TRIGGER_RUNNING;
            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                if (state_index == 0) {
                    static_cast<W8DoorTriggerActionData*>(m_pActionData)->flags_008 &= ~1;
                } else {
                    static_cast<W8DoorTriggerActionData*>(m_pActionData)->flags_008 |= 1;
                }
            }
            goto commit_action;
        }

        case 8: {
            signed char previous = static_cast<signed char>(state_index);

            if (state_count > 1) {
                signed char next = static_cast<signed char>(state_index) +
                                   static_cast<signed char>(state_direction);
                state_index = static_cast<unsigned char>(next);
                if (static_cast<unsigned char>(next) == state_count) {
                    if (cycle_bounce == 0) {
                        state_index = 0;
                    } else {
                        state_index = state_count - 2;
                        state_direction = 0xff;
                    }
                } else if (next < 0) {
                    state_index = 1;
                    state_direction = 1;
                }
            }

            if ((flags_0a0 & 0x1U) != 0) {
                W8AnimObj* animation;
                unsigned int count;
                unsigned int index;

                if (m_pProp == 0 || m_bRepType != 2) {
                    srAssertFail("m_pProp && m_bRepType == TRIGGER_REP_PROP",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x592, 0);
                }
                animation = m_pProp->Rep()->animation;
                if (AnimationIsRunning(animation) == 1) {
                    count = AnimObjListCount004A1620(animation, 2);
                    for (index = 0; index < count; ++index) {
                        W8PathAI* path = AnimObjListEntry004A16C0(animation, 2, (signed char)index);
                        PathAIUpdate004A9260(
                            path, previous <= static_cast<signed char>(state_index) ? 1 : -1);
                    }
                } else {
                    m_pProp->SetSetting66(static_cast<char>(state_index));
                }
            }
            goto commit_action;
        }

        case 0x2c: {
            W8DoorTriggerActionData* action_data = 0;
            bool was_active;

            if (m_pActionData != 0 && m_pActionData->type_004 == 10) {
                action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->flags_008 & 4) != 0 &&
                action_data->item_00a != -1) {
                if (FindItemOnParty(action_data->item_00a, 0, 0, 2, 0) == 0) {
                    return;
                }
                action_data->flags_008 &= ~4;
            }
            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            was_active = m_pProp->Rep()->animation_playing_06d;
            m_pProp->SetRepresentationActive(!was_active, 1);
            state_index = state_index == 0;
            if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
                m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
            }
            if (!was_active) {
                flags_0a0 |= W8_TRIGGER_RUNNING;
            } else {
                flags_0a0 &= ~W8_TRIGGER_RUNNING;
            }
            if (action_data != 0) {
                action_data->flags_008 = (action_data->flags_008 & ~1) | (state_index & 1);
            }
            goto commit_action;
        }

        case 0x37: {
            int tag = source == -1 ? m_lData1 : source;

            if (m_bRepType == 2 && m_pProp != 0 && tag != -1) {
                m_pProp->Rep()->SelectAnimationSlot((unsigned char)tag);
                m_pProp->SetRepresentationActive(1, 1);
                state_index = static_cast<unsigned char>(tag);
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
        bool was_active;
        bool action_succeeded = true;

        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        was_active = m_pProp->Rep()->animation_playing_06d;

        if (inline_action_data_24c[0] != '\0') {
            if (g_status_685170.item_in_cursor != 0) {
                srVector3T<float> item_position;
                W8WorldItem* item =
                    CreateWorldItem(&g_status_685170.item_in_hand_235b, &item_position, 3, 0);

                if (item != 0) {
                    if (world_item_group_34c == 0) {
                        srVector3T<float> group_position;
                        world_item_group_34c = SpawnItem(0x23c, &group_position, 0, 0);
                    }
                    ItemInfoAddToGroup(world_item_group_34c, item);
                }
                g_trigger_feedback_00606994 = 1;
                return;
            }
            if ((flags_0a0 & W8_TRIGGER_ITEM_PICKER) != 0) {
                return;
            }

            g_trigger_feedback_00606994 = 1;
            if (items_generated == 0) {
                GenerateItemGroup();
            }
            if (world_item_group_34c != 0) {
                int item_count = ItemInfoGetNumInGroup(world_item_group_34c) - 1;
                W8WorldItem* item;
                int contained_items = 0;

                if (item_count != 1 && m_pProp->Rep()->subcycle_064 != 0) {
                    action_succeeded = false;
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
                    gXStatus.item_pick_pending_19b6 = 1;
                }

                if (gold_358 != 0) {
                    AddPartyGold(gold_358, 1);
                    gold_358 = 0;
                }

                if (item_count == 1) {
                    if (m_pProp->Rep()->subcycle_064 == 0) {
                        ApplyItemEffectToRandomCharacter(Random(2) != 0
                                                             ? g_container_event_0068c548
                                                             : g_container_event_alt_0068c520,
                                                         -1, 0, g_effect_argument_005ed8c8);
                    }
                } else if (item_count == 2 && g_status_685170.item_in_cursor == 0) {
                    item = world_item_group_34c->next;
                    MoveItem(&g_status_685170.item_in_hand_235b, &item->item, 0, 1);
                    ItemInfoRemoveFromGroup(world_item_group_34c, item);
                    if (m_pProp->Rep()->subcycle_064 != 0) {
                        goto toggle_item_prop;
                    }
                } else {
                    flags_0a0 |= W8_TRIGGER_ITEM_PICKER;
                }

                if (!action_succeeded) {
                    return;
                }
            }
        }

    toggle_item_prop:
        m_pProp->SetRepresentationActive(!was_active, 1);
        state_index = state_index == 0;
        if (m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
            m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
        }
        if (!was_active) {
            flags_0a0 |= W8_TRIGGER_RUNNING;
        } else {
            flags_0a0 &= ~W8_TRIGGER_RUNNING;
        }
        break;
    }

    case 4:
    case 0x30:
    case 0x31: {
        char* recipient = m_pacRecipients;

        while (recipient != 0) {
            stLight* light = FindLightByName00445A10(NextTriggerRecipient(&recipient), 0);
            if (light != 0) {
                light->m_save_marked_23a = 1;
                if (action_230 == 4) {
                    if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                        light->setFlag(srNode::FLAG_DISABLE);
                    } else {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                } else if (action_230 == 0x30) {
                    if (light->testFlag(srNode::FLAG_DISABLE) != 0) {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                } else if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                    light->setFlag(srNode::FLAG_DISABLE);
                }
                action_succeeded = true;
            }
        }
        if (trigger_kind_018 == 2) {
            flags_0a0 |= W8_TRIGGER_RUNNING;
        }
        if (!action_succeeded) {
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
        for (index = 0; index < (int)PLLength(gXStatus.plsMonsterGroupList); ++index) {
            group = static_cast<W8MonsterGroup*>(PLGet(gXStatus.plsMonsterGroupList, index));
            if (group->monster_id == monster_id) {
                int location_id = IListGetAt(group->monsters, 0);
                unsigned int monster_index = MonsterGetIndexByLocationID(
                    0x7aa, "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", location_id, 1);

                monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
                break;
            }
        }
        if (group == 0 || monster_info == 0) {
            return;
        }

        group->members_active_28 = 1;
        monster_info->monster->m_pRep->animation_playing_06d = 1;
        monster_info->monster->m_pRep->animation_playing_06d = 1;
        monster_info->monster->m_pRep->timer_068 =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        monster_info->monster->ResetRepresentation004A7420();
        monster_info->monster->ResetPathAI();
        monster_info->monster->unknown_09d[0] = 1;
        return;
    }

    case 0x0f: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            Trigger* target = FindTriggerByName(NextTriggerRecipient(&recipient));
            if (target != 0) {
                bool was_running = ((flags_0a0 & W8_TRIGGER_RUNNING) != 0);
                flags_0a0 |= W8_TRIGGER_RUNNING;
                target->Run(m_lData1);
                flags_0a0 =
                    (flags_0a0 & ~W8_TRIGGER_RUNNING) | (was_running != 0 ? W8_TRIGGER_RUNNING : 0);
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
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

            source_position.Set(position_118.x, position_118.y, position_118.z);
            target_position = source_position;
            target_position.z += 100.0f;
            rotation.SetIdentity();
            axis = rotation.vectors[2];
            if (angle_0fc != 0.0f) {
                rotation.RotateAroundAxis(sin(angle_0fc), cos(angle_0fc), axis);
            }
            transformed = rotation.Transform(target_position);
            FireMissile004A2D30((unsigned int)m_lData1, &source_position, &transformed, 0, 1, 1,
                                50000.0f);
        } else if (m_pEvent == 0) {
            float duration = (float)abs(m_lData2) * 0.001f;

            if (m_lData2 > 0 && trigger_kind_018 != 2) {
                srAssertFail("m_lData2 < 0 || m_iType == TRIGGER_INVISIBLE",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x87f,
                             "Continous firing missile must be invisible trigger.");
            }
            m_pEvent = new W8TriggerEvent;
            if (m_pEvent == 0) {
                srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x883, 0);
            }
            m_pEvent->action_004 = (short)action_230;
            m_pEvent->timer_008.SetDuration(duration);
            m_pEvent->timer_008.Restart();
            m_pEvent->trigger_030 = this;
            flags_0a0 |= W8_TRIGGER_RUNNING;
            g_timed_events_006599b8.Add(m_pEvent);
        }
        break;

    case 0x10:
        SetCameraSwayMode(m_pWorld->camera, source > 0 ? 1 : -1);
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
        SetDice(&dice, (unsigned char)m_lData1, (unsigned char)m_lData2, (short)m_lData3);
        ApplyRolledHealthChangeToParty(&dice, 0, 1);
        if (trigger_kind_018 == 2) {
            flags_0a0 |= W8_TRIGGER_RUNNING;
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
        if (uses_remaining != 0 || m_lData1 == -1) {
            if (action_230 == 0x25) {
                RestorePartyStaminaByDice(0, 0, (short)m_lData3);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else if (action_230 == 0x26) {
                HealPartyByDice(0, 0, (short)m_lData3);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else if (action_230 == 0x27) {
                RestorePartySpellPoints(m_lData3);
                ShowString(gppStringList[0x1c88 / 4]);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else {
                int spell_id;
                srVector3T<double> position = g_world->camera->getLocation();

                if (action_230 == 0x28) {
                    spell_id = 0x44;
                } else if (action_230 == 0x29) {
                    spell_id = 0x45;
                } else if (action_230 == 0x2a) {
                    spell_id = 0x0c;
                } else {
                    spell_id = 0x2a;
                }
                PointCastSpell(
                    srVector3T<float>((float)position.x, (float)position.y, (float)position.z),
                    spell_id, (unsigned int)m_lData3);
                if (action_230 == 0x2b) {
                    RemoveAllConditionsFromParty();
                }
            }
        }

        if (uses_remaining == 0) {
            if (m_lData1 != -1) {
                return;
            }
        } else {
            --uses_remaining;
            if (m_lData2 > 0 && m_pEvent == 0) {
                m_pEvent = new W8TriggerEvent;
                if (m_pEvent == 0) {
                    srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
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
            flags_0a0 |= W8_TRIGGER_RUNNING;
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
            Trigger* target = FindTriggerByName(NextTriggerRecipient(&recipient));
            if (target != 0) {
                if (action_230 == 0x2d) {
                    target->flags_0a0 |= W8_TRIGGER_ON;
                } else if (action_230 == 0x2e) {
                    target->flags_0a0 &= ~W8_TRIGGER_ON;
                } else {
                    target->flags_0a0 ^= W8_TRIGGER_ON;
                }
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
            return;
        }
        break;
    }

    case 0x32:
    case 0x33:
        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        if ((action_230 == 0x32 && m_pProp->Rep()->animation_playing_06d != 0) ||
            (action_230 == 0x33 && m_pProp->Rep()->animation_playing_06d == 0)) {
            return;
        }
        m_pProp->SetRepresentationActive(action_230 == 0x32, 1);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
            m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
        }
        if (action_230 == 0x32) {
            flags_0a0 |= W8_TRIGGER_RUNNING;
        } else {
            flags_0a0 &= ~W8_TRIGGER_RUNNING;
        }
        break;

    case 0x36:
        if (action_state_232 == 4 && action_data_mode_228 == 2) {
            PlayActionSound(alternate_action_data_1a8, 0);
        } else {
            PlayActionSound(action_data_128, 0);
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
                srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x7c4, 0);
            }
            event->trigger_030 = this;
            event->action_004 = (short)action_230;
            event->timer_008.SetDuration(m_lData2 == -1 ? 0.07f : m_lData2 * 0.001f);
            event->timer_008.Restart();
            event->intensity_03c = m_lData1 == -1 ? 800 : m_lData1;

            if (m_lData3 != -1) {
                delete event->m_pCountdown;
                event->m_pCountdown = new W8GameTimer;
                if (event->m_pCountdown == 0) {
                    srAssertFail("m_pCountdown",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x12de, 0);
                }
                event->m_pCountdown->SetDuration((float)abs(m_lData3) * 0.001f);
                event->m_pCountdown->Restart();
                if (m_lData3 < 0) {
                    event->reverse_040 = 1;
                }
            }
        } else {
            event = static_cast<W8TriggerShakeEvent*>(m_pEvent);
            event->timer_008.Restart();
            if (event->m_pCountdown != 0) {
                event->m_pCountdown->Restart();
            }
        }

        if ((flags_0a0 & W8_TRIGGER_RUNNING) == 0) {
            g_timed_events_006599b8.Add(m_pEvent);
            if (trigger_kind_018 == 2) {
                flags_0a0 |= W8_TRIGGER_RUNNING;
            } else if (m_lData3 == -1) {
                srAssertFail("m_lData3 != -1", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x7e4, "Non invisible triggers with shake must have a duration.");
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
        if (m_pProp == 0 || source != m_lData1 || m_pProp->Rep()->animation_playing_06d == 0) {
            return;
        }
        m_pProp->SetRepresentationActive(m_pProp->Rep()->animation_playing_06d == 0, 1);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
            m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
        }
        break;

    case 0x3c:
        if (m_pProp == 0 || source != m_lData1) {
            return;
        }
        m_pProp->SetRepresentationActive(m_pProp->Rep()->animation_playing_06d == 0, 1);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->m_owned_04c != 0 && surface_id >= 0) {
            m_pWorld->m_owned_04c->SetInterfaceState(surface_id, state_index);
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
                srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x8a4, 0);
            }
            m_pEvent->action_004 = (short)action_230;
            m_pEvent->timer_008.SetDuration(m_lData1 * 0.001f);
            m_pEvent->timer_008.Restart();
            m_pEvent->trigger_030 = this;
            m_pEvent->repeat_034 = 1;
        } else {
            if (g_timed_events_006599b8.IndexOf(m_pEvent) != -1) {
                srAssertFail("glsTimedEvents.Find(m_pEvent) == -1",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x8ae, 0);
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
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

            sprintf(state_name, "%s%d", m_pacStateToMod,
                    static_cast<int>(static_cast<signed char>(state_index)));
            state_id = GetLocationVarIDByName(state_name);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10c9, 0);
            }
            g_location_variable_values_00659990.SetAt(state_id, 0);
        }
        state_index = m_pProp->Rep()->AdvanceAnimationSegment();
        m_pProp->SetRepresentationActive(1, 0);
        if (m_pacStateToMod != 0) {
            char state_name[132];
            int state_id;

            sprintf(state_name, "%s%d", m_pacStateToMod,
                    static_cast<int>(static_cast<signed char>(state_index)));
            state_id = GetLocationVarIDByName(state_name);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10c9, 0);
            }
            g_location_variable_values_00659990.SetAt(state_id, 1);
        }
        apply_state_changes = false;
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
                PositionAmbientSoundByName0047A950(g_world, name);
            } else if (action_230 == 0x42) {
                StopAmbientSoundByName0047A9E0(g_world, name);
            } else {
                ToggleAmbientSoundByName0047AA70(g_world, name);
            }
            action_succeeded = true;
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
            stParticle* particle = FindParticleByName(g_world, NextTriggerRecipient(&recipient));
            if (particle != 0) {
                particle->persisted_192 = 1;
                if (action_230 == 0x44) {
                    particle->SetActive(1);
                } else if (action_230 == 0x45) {
                    particle->SetActive(0);
                } else {
                    particle->SetActive(particle->emitting_1a0 == 0);
                }
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
            return;
        }
        break;
    }

    case 0x47: {
        char* recipient = m_pacRecipients;

        if (recipient == 0 || m_pEvent != 0) {
            return;
        }
        action_succeeded = false;
        while (recipient != 0) {
            stParticle* particle = FindParticleByName(g_world, NextTriggerRecipient(&recipient));
            if (particle != 0) {
                particle->persisted_192 = 1;
                particle->SetActive(1);
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
            return;
        }

        m_pEvent = new W8TriggerEvent;
        if (m_pEvent == 0) {
            srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x940,
                         0);
        }
        g_timed_events_006599b8.Add(m_pEvent);
        m_pEvent->trigger_030 = this;
        m_pEvent->action_004 = (short)action_230;
        delete m_pEvent->m_pCountdown;
        m_pEvent->m_pCountdown = new W8GameTimer;
        if (m_pEvent->m_pCountdown == 0) {
            srAssertFail("m_pCountdown", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                         0x12de, 0);
        }
        m_pEvent->m_pCountdown->SetDuration(m_lData1 == -1 ? 10.0f : m_lData1 * 0.001f);
        m_pEvent->m_pCountdown->Restart();
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
            W8Prop* prop = FindPropByName(g_world, NextTriggerRecipient(&recipient));
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
    if (running == 0) {
        g_trigger_feedback_00606994 = 1;
    }
    CommitActionResult(apply_state_changes);
}

// FUNCTION: WIZ8 0x00444600
bool Trigger::CanRunLinkedTriggers()
{
    char* recipient;

    if (m_pProp != 0 && m_pProp->Rep()->animation_playing_06d != 0) {
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
        } else {
            recipient = 0;
        }
        trigger = FindTriggerByName(g_trigger_parse_buffer_00659908);
        if (trigger != 0 && !trigger->CanRunLinkedTriggers()) {
            return false;
        }
    }
    return true;
}

// FUNCTION: WIZ8 0x0043d340
bool Trigger::SelectAction()
{
    bool fallback_selected = false;
    bool result = true;

    if (g_combat_inactive_006081e4 == 0 && m_pActionData != 0 && m_pActionData->type_004 == 10 &&
        (static_cast<W8DoorTriggerActionData*>(m_pActionData)->flags_008 & 1) != 0) {
        return 0;
    }

    if (((flags_0a0 & W8_TRIGGER_RUNNING) != 0 && action_230 != 0x39) ||
        (flags_0a0 & W8_TRIGGER_ON) == 0 ||
        ((flags_0a0 & 0x40000U) != 0 && (flags_0a0 & 0x80000U) != 0)) {
        action_state_232 = 1;
        return 0;
    }

    if (m_pacRequiredStates != 0) {
        if (strchr(m_pacRequiredStates, ',') == 0) {
            int state_id = GetLocationVarIDByName(m_pacRequiredStates);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4317, 0);
            }
            if (*g_location_variable_values_00659990.GetAt(state_id) == 0) {
                action_230 = fallback_action_22e;
                action_state_232 = 4;
                fallback_selected = true;
            }
        } else {
            char* required_state = 0;
            bool more_states = true;
            char state_name[128];

            while (more_states) {
                char* comma;
                int state_id;

                if (required_state == 0) {
                    required_state = m_pacRequiredStates;
                } else {
                    required_state = strchr(required_state, ',') + 1;
                }
                strcpy(state_name, required_state);
                comma = strchr(state_name, ',');
                if (comma == 0) {
                    more_states = false;
                } else {
                    *comma = '\0';
                }

                state_id = GetLocationVarIDByName(state_name);
                if (state_id == -1) {
                    srAssertFail("iVar != BAD_INDEX",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4317, 0);
                }
                if (*g_location_variable_values_00659990.GetAt(state_id) == 0) {
                    action_230 = fallback_action_22e;
                    action_state_232 = 4;
                    fallback_selected = true;
                    break;
                }
            }
        }
    }

    if (m_pActionData == 0 || m_pActionData->type_004 != 10) {
        if (required_item_id >= 0) {
            if (GetItemInHand() == required_item_id) {
                if ((flags_0a0 & 0x10000U) != 0) {
                    RemovePartyItemByID005215D0(required_item_id, 0);
                    required_item_id = -1;
                }
            } else {
                if (m_lData2 == 1 && activation_callback_360 != 0) {
                    activation_callback_360(this);
                }
                action_230 = fallback_action_22e;
                action_state_232 = 4;
                fallback_selected = true;
            }
        }
    } else {
        W8DoorTriggerActionData* action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
        bool linked_trigger_blocked = false;

        if (m_pProp != 0 && m_pProp->Rep()->animation_playing_06d != 0) {
            linked_trigger_blocked = true;
        } else {
            char* cursor = m_pacRecipients;
            char* name;
            while ((name = NextTriggerRecipient(&cursor)) != 0) {
                Trigger* trigger = FindTriggerByName(name);
                if (trigger != 0 && !trigger->CanRunLinkedTriggers()) {
                    linked_trigger_blocked = true;
                    break;
                }
            }
        }

        if (linked_trigger_blocked) {
            if (m_pEvent == 0 || g_timed_events_006599b8.IndexOf(m_pEvent) == -1) {
                return 0;
            }
            m_pEvent->timer_008.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
            }
            if (running == 0) {
                g_trigger_feedback_00606994 = 1;
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
                fallback_selected = true;
            } else {
                device_state.completed = 1;
                action_data->flags_008 &= ~4;
                if (action_data->linked_trigger_00c[0] != '\0') {
                    Trigger* linked_trigger;
                    action_state_232 = 1;
                    result = false;
                    linked_trigger = FindTriggerByName(action_data->linked_trigger_00c);
                    if (linked_trigger != 0) {
                        linked_trigger->Run(-1);
                        if (running == 0) {
                            g_trigger_feedback_00606994 = 1;
                        }
                    }
                }
            }
        }
    }

    if (lock_type != 0 && device_state.completed == 0 && running == 0) {
        if (lock_type == 1) {
            g_trigger_feedback_00606994 = 1;
            OpenLockInteraction00587510(this);
            return 0;
        }
        if (lock_type == 2) {
            g_trigger_feedback_00606994 = 1;
            OpenTrapInteraction0058A470(this);
            return 0;
        }
    }

    if (!fallback_selected) {
        if ((flags_0a0 & 0x4000U) == 0) {
            action_230 = initial_action_22a;
            action_state_232 = 2;
            if ((flags_0a0 & 0x2000U) != 0) {
                flags_0a0 |= 0x4000U;
            }
        } else {
            action_230 = alternate_action;
            action_state_232 = 3;
            if ((flags_0a0 & 0x8000U) != 0) {
                flags_0a0 &= ~0x4000U;
            }
        }
    } else if (action_230 == 0) {
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

/* Resolves a light instance by name under the stLight class node. Retail
   inlines stLight::sGetClassNode, so this emission carries the lazy
   stLight->srLight->srNode registration walk before the registry find. */
// FUNCTION: WIZ8 0x00445a10
stLight* FindLightByName00445A10(const char* name, const srRuntimeClass* relative_to)
{
    return static_cast<stLight*>(
        srCore.getRegistry()->find(stLight::sGetClassNode(), name, relative_to));
}

// TEMPLATE: WIZ8 0x00445EF0
// srClassSupport<srNode,srNode,0,4096>::getClassNode

// TEMPLATE: WIZ8 0x00445f30
// srClassSupport<Trigger,srClass,1,65544>::getClassNode

// FUNCTION: WIZ8 0x00443a50
int ResetNextTriggerId(void)
{
    g_status_685170.next_trigger_id_2356 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00443A60
void DestroyAllWorldTriggers(W8World* world)
{
    if (world != 0 && world->triggers != 0) {
        while (world->triggers->GetCount() != 0) {
            Trigger* trigger = *world->triggers->GetAt(0);
            world->triggers->RemoveAt(0);
            if (trigger != 0) {
                trigger->release();
            }
        }
        world->triggers->Clear();
    }
}

// FUNCTION: WIZ8 0x00444170
int GetLocationVarIDByName(const char* name)
{
    int variable_count = g_location_variable_names_006598f8.GetCount();
    int variable_id;
    char** variable_name;
    int* variable_level;

    for (variable_id = 0; variable_id < variable_count; ++variable_id) {
        variable_name = g_location_variable_names_006598f8.GetAt(variable_id);
        if (_stricmp(*variable_name, name) == 0) {
            variable_level = g_location_variable_levels_006598e0.GetAt(variable_id);
            if (*variable_level == g_status_685170.current_level) {
                return variable_id;
            }
        }
    }
    return -1;
}

/* Create a location variable for the current level unless one with this name
   already exists. */
// FUNCTION: WIZ8 0x00443dc0
void CreateLocationVar(const char* name, int value)
{
    int index;
    int variable_count;
    char* copy;

    if (name == 0) {
        srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1094, 0);
    }
    variable_count = g_location_variable_names_006598f8.GetCount();
    for (index = 0; index < variable_count; ++index) {
        if (_stricmp(*g_location_variable_names_006598f8.GetAt(index), name) == 0 &&
            *g_location_variable_levels_006598e0.GetAt(index) == g_status_685170.current_level) {
            break;
        }
    }
    if (index == variable_count) {
        index = -1;
    }
    if (index != -1) {
        return;
    }
    copy = new char[strlen(name) + 1];
    if (copy == 0) {
        srAssertFail("pacVariableName", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                     0x109c, 0);
    }
    strcpy(copy, name);
    g_location_variable_names_006598f8.Add(copy);
    g_location_variable_values_00659990.Add(value);
    g_location_variable_levels_006598e0.Add(g_status_685170.current_level);
}

/* The current level's value of the named location variable; asserts when the
   name is unknown. */
// FUNCTION: WIZ8 0x004440d0
int GetLocationVarValueByName(const char* name)
{
    int index;
    int variable_count;

    variable_count = g_location_variable_names_006598f8.GetCount();
    for (index = 0; index < variable_count; ++index) {
        if (_stricmp(*g_location_variable_names_006598f8.GetAt(index), name) == 0 &&
            *g_location_variable_levels_006598e0.GetAt(index) == g_status_685170.current_level) {
            break;
        }
    }
    if (index == variable_count) {
        index = -1;
    }
    if (index == -1) {
        srAssertFail("iVar != BAD_INDEX", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                     0x10dd, 0);
    }
    return *g_location_variable_values_00659990.GetAt(index);
}

/* Write the count then each location variable's value, name and level. */
// FUNCTION: WIZ8 0x004441e0
void SaveLocationVariables004441E0(int handle)
{
    int variable_count = g_location_variable_names_006598f8.GetCount();
    bool written;

    written = FileWrite(handle, &variable_count, sizeof(variable_count), 0) != 0;
    for (int index = 0; index < variable_count; ++index) {
        int value;
        char name[0x80];
        int level;

        if (!written) {
            return;
        }
        value = *g_location_variable_values_00659990.GetAt(index);
        strcpy(name, *g_location_variable_names_006598f8.GetAt(index));
        level = *g_location_variable_levels_006598e0.GetAt(index);
        written = FileWrite(handle, &value, sizeof(value), 0) &&
                  FileWrite(handle, name, sizeof(name), 0) &&
                  FileWrite(handle, &level, sizeof(level), 0);
    }
}

/* Read the location-variable count then each value/name/level record,
   appending a heap copy of the name to the variable vectors. */
// FUNCTION: WIZ8 0x00444310
bool LoadLocationVariables00444310(int handle)
{
    /* Retail read these uninitialised when a FileRead short-circuited;
       deterministic zeroes model that defect path. */
    int variable_count = 0;
    int index;
    bool read_ok;

    read_ok = FileRead(handle, &variable_count, sizeof(variable_count), 0) != 0;
    for (index = 0; index < variable_count; ++index) {
        int value = 0;
        char name[0x80] = {0};
        int level = 0;
        char* copy;

        if (!read_ok) {
            break;
        }
        read_ok = FileRead(handle, &value, sizeof(value), 0) &&
                  FileRead(handle, name, sizeof(name), 0) &&
                  FileRead(handle, &level, sizeof(level), 0);
        copy = new char[strlen(name) + 1];
        if (copy == 0) {
            srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1143,
                         0);
        }
        strcpy(copy, name);
        g_location_variable_names_006598f8.Add(copy);
        g_location_variable_values_00659990.Add(value);
        g_location_variable_levels_006598e0.Add(level);
    }
    return read_ok;
}

// FUNCTION: WIZ8 0x004445b0
void ReleaseAllTriggers(void)
{
    int index;

    for (index = 0; index < g_location_variable_names_006598f8.GetCount(); ++index) {
        delete[] *g_location_variable_names_006598f8.GetAt(index);
    }
    g_location_variable_values_00659990.Clear();
    g_location_variable_names_006598f8.Clear();
    g_location_variable_levels_006598e0.Clear();
}

/* Index of the prop whose trigger last tested in view; -1 until a prop
   matches. */
// GLOBAL: WIZ8 0x00606998
static int s_last_prop_index_00606998 = -1;

/* Report whether any world prop's trigger representation is within its
   activation range and projects onto the screen. The remembered index is
   checked first so consecutive frames start at the prop that matched. */
// FUNCTION: WIZ8 0x00445140
bool AnyPropTriggerInView00445140(W8World* world)
{
    srVector3T<float> position;
    unsigned int prop_count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1395, 0);
    }
    GetCameraPosition(&position);
    prop_count = PLLength(world->plsProps);
    if (0 <= s_last_prop_index_00606998 &&
        s_last_prop_index_00606998 < static_cast<int>(prop_count)) {
        W8Prop* prop = static_cast<W8Prop*>(PLGet(world->plsProps, s_last_prop_index_00606998));

        if (prop->IsTriggerInView0044E3A0(&position)) {
            return 1;
        }
    }
    for (index = 0; index < static_cast<int>(prop_count); ++index) {
        W8Prop* prop = static_cast<W8Prop*>(PLGet(world->plsProps, index));

        if (prop->IsTriggerInView0044E3A0(&position)) {
            s_last_prop_index_00606998 = index;
            return 1;
        }
    }
    return 0;
}
