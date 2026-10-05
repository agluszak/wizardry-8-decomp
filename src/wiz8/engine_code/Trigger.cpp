#include "soundman.h"
#include "wiz8/integer_constants.h"
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
static W8GrowableVector<W8TriggerEvent*> g_timed_events;

// GLOBAL: WIZ8 0x006599C8
bool g_trigger_action_active;
// GLOBAL: WIZ8 0x006599AC
srVector3T<float> g_trigger_action_scene_offset;
// GLOBAL: WIZ8 0x00659908
static char g_trigger_parse_buffer[0x88];
// GLOBAL: WIZ8 0x006598E0
W8GrowableVector<int> g_location_variable_levels;
// GLOBAL: WIZ8 0x006598F8
W8GrowableVector<char*> g_location_variable_names;
// GLOBAL: WIZ8 0x00659990
W8GrowableVector<int> g_location_variable_values;

// GLOBAL: WIZ8 0x00606994
bool g_trigger_feedback = 1;

// GLOBAL: WIZ8 0x0068c520
int g_container_event_alt = g_first_remapped_event + 13;

// GLOBAL: WIZ8 0x0068c548
int g_container_event = g_first_remapped_event + 14;

// GLOBAL: WIZ8 0x005ec124
const float g_float_005ec124 = 64.0f;

// FUNCTION: WIZ8 0x00443780
Trigger* FindTriggerByName(const char* name)
{
    char* uppercase_name;
    Trigger* trigger = 0;

    uppercase_name = static_cast<char*>(malloc(strlen(name) + 1));
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
Trigger* FindTriggerForProp(W8World* world, W8Prop* prop)
{
    Trigger* trigger = prop->GetTrigger();

    if (trigger != 0) {
        W8TriggerActionData* action_data = trigger->m_pActionData;

        if (action_data != 0 && action_data->type == 10) {
            return trigger;
        }
        if (trigger->trigger_kind == 1 || trigger->trigger_kind == 2) {
            int trigger_count = world->triggers->GetCount();

            for (int index = 0; index < trigger_count; ++index) {
                Trigger* other = *world->triggers->GetAt(index);
                Trigger* previous = trigger;
                int hops = 0;

                if (other == trigger) {
                    continue;
                }
                while (hops < 5 && other->m_pacRecipients != 0 &&
                       (other->trigger_kind == 1 || other->trigger_kind == 2)) {
                    char* recipient = other->m_pacRecipients;
                    Trigger* linked;

                    do {
                        char* comma;

                        if (recipient == 0) {
                            goto next_trigger;
                        }
                        strcpy(g_trigger_parse_buffer, recipient);
                        comma = strchr(g_trigger_parse_buffer, ',');
                        if (comma == 0) {
                            recipient = 0;
                        } else {
                            recipient = strchr(recipient, ',') + 1;
                            *comma = '\0';
                        }
                        linked = FindTriggerByName(g_trigger_parse_buffer);
                    } while (linked != previous);
                    action_data = other->m_pActionData;
                    if (action_data != 0 && action_data->type == 10) {
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

// FUNCTION: WIZ8 0x00445730
void W8LockState::Reset()
{
    int pins;

    if (lock_type == 1) {
        for (int pin = 0; pin < 8; ++pin) {
            device_state.pins[pin] = static_cast<char>(Random(4));
        }
        pins = difficulty < 8 ? (difficulty < 2 ? 2 : difficulty) : 8;
        lock_countdown = pins * 3;
    }
    last_interaction_clock = -1;
    device_state.completed = 0;
}

// FUNCTION: WIZ8 0x004457A0
bool W8LockState::ConsumeCountdown()
{
    if (lock_countdown > 0) {
        --lock_countdown;
        return true;
    }
    return false;
}

/* True while a type-0x34 destination trigger holds (x, y, z) inside its
   activation annulus: under range_maximum and, unless flags bit 6
   waives the minimum, at or beyond range_minimum. */
// FUNCTION: WIZ8 0x00445940
bool InsideDestinationTrigger(float x, float y, float z)
{
    int count = g_world->triggers->GetCount();

    for (int index = 0; index < count; ++index) {
        Trigger* trigger = *g_world->triggers->GetAt(index);
        if (trigger->initial_action != 0x34) {
            continue;
        }
        float dx = trigger->position.x - x;
        float dy = trigger->position.y - y;
        float dz = trigger->position.z - z;
        float distance = sqrtf(dx * dx + dy * dy + dz * dz);
        if (distance < trigger->range_maximum && ((trigger->flags >> 6) & 1) == 0 &&
            distance >= trigger->range_minimum) {
            return true;
        }
    }
    return false;
}

// FUNCTION: WIZ8 0x0043cb30
void SaveTriggerRuntimeStates(W8World* world, int handle, bool restoring)
{
    int trigger_count = world->triggers->GetCount();
    int saved_count = 0;
    int index;
    int version = 2;
    int restoring_value = restoring;

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->lock_state.lock_type != 0) {
            ++saved_count;
        }
    }

    FileWrite(handle, &version, sizeof(version), 0);
    FileWrite(handle, &saved_count, sizeof(saved_count), 0);
    FileWrite(handle, &restoring_value, sizeof(restoring_value), 0);

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->lock_state.lock_type != 0) {
            FileWrite(handle, trigger->name, 0x80, 0);
            FileWrite(handle, &version, sizeof(version), 0);
            if (restoring) {
                FileWrite(handle, &trigger->lock_state.lock_type,
                          sizeof(trigger->lock_state.lock_type), 0);
                FileWrite(handle, &trigger->lock_state.difficulty,
                          sizeof(trigger->lock_state.difficulty), 0);
                FileWrite(handle, &trigger->lock_state.device_state.completed,
                          sizeof(trigger->lock_state.device_state.completed), 0);
                FileWrite(handle, trigger->lock_state.device_state.pins,
                          sizeof(trigger->lock_state.device_state.pins), 0);
                FileWrite(handle, &trigger->lock_state.device_id,
                          sizeof(trigger->lock_state.device_id), 0);
                FileWrite(handle, &trigger->lock_state.key_id, sizeof(trigger->lock_state.key_id),
                          0);
                FileWrite(handle, &trigger->lock_state.lock_countdown,
                          sizeof(trigger->lock_state.lock_countdown), 0);
            } else {
                FileWrite(handle, &trigger->lock_state.device_state.completed,
                          sizeof(trigger->lock_state.device_state.completed), 0);
                FileWrite(handle, trigger->lock_state.device_state.pins,
                          sizeof(trigger->lock_state.device_state.pins), 0);
                FileWrite(handle, &trigger->lock_state.lock_countdown,
                          sizeof(trigger->lock_state.lock_countdown), 0);
                FileWrite(handle, &trigger->lock_state.last_interaction_clock,
                          sizeof(trigger->lock_state.last_interaction_clock), 0);
            }
        }
    }
}

/* Read the runtime-state records written by SaveTriggerRuntimeStates.
   Each record names its trigger; a record whose trigger no longer exists is
   still consumed through a scratch trigger so the stream stays aligned. When
   restoring, a state byte block is re-randomized and the type-10 action data is
   re-linked to the stored item. */
// FUNCTION: WIZ8 0x0043ccf0
bool LoadTriggerRuntimeStates(int handle)
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
                FileRead(handle, &scratch->lock_state.device_state.completed,
                         sizeof(scratch->lock_state.device_state.completed), 0);
                FileRead(handle, scratch->lock_state.device_state.pins,
                         sizeof(scratch->lock_state.device_state.pins), 0);
                FileRead(handle, &scratch->lock_state.lock_countdown,
                         sizeof(scratch->lock_state.lock_countdown), 0);
                if (record_version > 1) {
                    FileRead(handle, &scratch->lock_state.last_interaction_clock,
                             sizeof(scratch->lock_state.last_interaction_clock), 0);
                }
            } else {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &scratch->lock_state.lock_type,
                         sizeof(scratch->lock_state.lock_type), 0);
                FileRead(handle, &scratch->lock_state.difficulty,
                         sizeof(scratch->lock_state.difficulty), 0);
                FileRead(handle, &scratch->lock_state.device_state.completed,
                         sizeof(scratch->lock_state.device_state.completed), 0);
                FileRead(handle, scratch->lock_state.device_state.pins,
                         sizeof(scratch->lock_state.device_state.pins), 0);
                FileRead(handle, &scratch->lock_state.device_id,
                         sizeof(scratch->lock_state.device_id), 0);
                FileRead(handle, &scratch->lock_state.key_id, sizeof(scratch->lock_state.key_id),
                         0);
                if (record_version > 1) {
                    FileRead(handle, &scratch->lock_state.lock_countdown,
                             sizeof(scratch->lock_state.lock_countdown), 0);
                }
            }
            delete scratch;
        } else {
            int record_version;
            W8TriggerActionData* action_data;

            if (restoring == 0 && version > 1) {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &trigger->lock_state.device_state.completed,
                         sizeof(trigger->lock_state.device_state.completed), 0);
                FileRead(handle, trigger->lock_state.device_state.pins,
                         sizeof(trigger->lock_state.device_state.pins), 0);
                FileRead(handle, &trigger->lock_state.lock_countdown,
                         sizeof(trigger->lock_state.lock_countdown), 0);
                if (record_version > 1) {
                    FileRead(handle, &trigger->lock_state.last_interaction_clock,
                             sizeof(trigger->lock_state.last_interaction_clock), 0);
                }
            } else {
                FileRead(handle, &record_version, sizeof(record_version), 0);
                FileRead(handle, &trigger->lock_state.lock_type,
                         sizeof(trigger->lock_state.lock_type), 0);
                FileRead(handle, &trigger->lock_state.difficulty,
                         sizeof(trigger->lock_state.difficulty), 0);
                FileRead(handle, &trigger->lock_state.device_state.completed,
                         sizeof(trigger->lock_state.device_state.completed), 0);
                FileRead(handle, trigger->lock_state.device_state.pins,
                         sizeof(trigger->lock_state.device_state.pins), 0);
                FileRead(handle, &trigger->lock_state.device_id,
                         sizeof(trigger->lock_state.device_id), 0);
                FileRead(handle, &trigger->lock_state.key_id, sizeof(trigger->lock_state.key_id),
                         0);
                if (record_version > 1) {
                    FileRead(handle, &trigger->lock_state.lock_countdown,
                             sizeof(trigger->lock_state.lock_countdown), 0);
                }
                if (restoring != 0) {
                    if (trigger->lock_state.lock_type == 1) {
                        int size;

                        for (int byte_index = 0; byte_index < 8; ++byte_index) {
                            trigger->lock_state.device_state.pins[byte_index] =
                                static_cast<unsigned char>(Random(4));
                        }
                        size = trigger->lock_state.difficulty;
                        if (size < 2) {
                            size = 2;
                        } else if (size > 7) {
                            size = 8;
                        }
                        trigger->lock_state.lock_countdown = size * 3;
                    }
                    trigger->lock_state.last_interaction_clock = -1;
                    trigger->lock_state.device_state.completed = 0;
                }
            }
            action_data = trigger->m_pActionData;
            if (action_data != 0 && action_data->type == 10 &&
                ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 4) == 0 ||
                 static_cast<W8DoorTriggerActionData*>(action_data)->item == -1)) {
                static_cast<W8DoorTriggerActionData*>(action_data)->door_flags =
                    ((trigger->lock_state.device_state.completed == 0) << 2) |
                    (static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 0xfb);
                static_cast<W8DoorTriggerActionData*>(action_data)->item =
                    static_cast<short>(trigger->lock_state.key_id);
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
bool Trigger::Save(int hFile)
{
    unsigned char version = 5;
    int trigger_count = g_world->triggers->GetCount();
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
                FileWrite(hFile, &trigger_count, sizeof(trigger_count), 0) &&
                FileWrite(hFile, name, 0x80, 0) &&
                FileWrite(hFile, &flags, sizeof(flags), 0) &&
                FileWrite(hFile, &state_index, sizeof(state_index), 0) &&
                FileWrite(hFile, &state_direction, sizeof(state_direction), 0) &&
                FileWrite(hFile, &action, sizeof(action), 0) &&
                FileWrite(hFile, &action_state, sizeof(action_state), 0) &&
                FileWrite(hFile, &required_item_id, sizeof(required_item_id), 0);
    action_data = m_pActionData;
    has_action_data = action_data != 0;
    FileWrite(hFile, &has_action_data, sizeof(has_action_data), 0);
    if (action_data != 0) {
        action_type = action_data->type;
        FileWrite(hFile, &action_type, sizeof(action_type), 0);
        if (action_type == 10) {
            action_flags = 0;
            progress_delay = 0;
            action_kind = 2;
            FileWrite(hFile, &action_kind, sizeof(action_kind), 0);
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 1) != 0) {
                action_flags |= 1;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 2) != 0) {
                action_flags |= 2;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 4) != 0) {
                action_flags |= 4;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 8) != 0) {
                action_flags |= 8;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 0x10) != 0) {
                action_flags |= 0x10;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 0x20) != 0) {
                action_flags |= 0x20;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 0x40) != 0) {
                action_flags |= 0x40;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 0x80) != 0) {
                action_flags |= 0x80;
            }
            if ((static_cast<W8DoorTriggerActionData*>(action_data)->extra_flags & 1) != 0) {
                action_flags |= 0x100;
            }
            if (static_cast<W8DoorTriggerActionData*>(action_data)->item != 0) {
                action_flags |= 0x200;
            }
            FileWrite(hFile, &action_flags, sizeof(action_flags), 0);
            if (m_lData1 != 0 && m_pEvent != 0 && g_timed_events.IndexOf(m_pEvent) != -1) {
                float progress = m_pEvent->timer.GetProgress();
                if (progress <= g_float_005ec124) {
                    progress_delay = static_cast<unsigned int>(m_pEvent->timer.GetProgress());
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
    has_world_item = world_item_group != 0;
    FileWrite(hFile, &has_world_item, sizeof(has_world_item), 0);
    if (has_world_item != 0) {
        SaveItemFile(hFile, world_item_group);
    }
    FileWrite(hFile, &items_generated, sizeof(items_generated), 0);
    FileWrite(hFile, &item_group_seed, sizeof(item_group_seed), 0);
    FileWrite(hFile, &gold, sizeof(gold), 0);
    FileWrite(hFile, &uses_remaining, sizeof(uses_remaining), 0);
    return header_ok;
}

/* Read one trigger back from the save file. The version byte selects how much
   of the trailing block is present; a type-10 action payload rebuilds its
   action data and re-queues the delayed timed event from the saved progress. */
// FUNCTION: WIZ8 0x0043c1b0
bool Trigger::Load(int hFile, char version)
{
    bool header_ok;
    unsigned char has_action_data;
    unsigned char action_type;
    unsigned char flag_mode;
    unsigned char flag;

    if (hFile == 0) {
        srAssertFail("hFile", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1f0, 0);
    }
    header_ok = FileRead(hFile, &flags, sizeof(flags), 0) &&
                FileRead(hFile, &state_index, sizeof(state_index), 0) &&
                FileRead(hFile, &state_direction, sizeof(state_direction), 0) &&
                FileRead(hFile, &action, sizeof(action), 0) &&
                FileRead(hFile, &action_state, sizeof(action_state), 0) &&
                FileRead(hFile, &required_item_id, sizeof(required_item_id), 0);
    if ((flags & W8_TRIGGER_SEARCHED) != 0) {
        UnregisterSearchableTrigger(this);
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
                pDoor->door_flags = 0x40;
                pDoor->extra_flags &= ~1;
                pDoor->item = -1;
                pDoor->type = 10;
                pDoor->position.SetZero();
                pDoor->linked_trigger[0] = 0;
            }
            delete m_pActionData;
            m_pActionData = pDoor;
            pDoor->type = 10;
            FileRead(hFile, &flag_mode, 1, 0);
            if (flag_mode == 1) {
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~1) | (flag & 1);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~2) | ((flag & 1) << 1);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~4) | ((flag & 1) << 2);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~8) | ((flag & 1) << 3);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~0x10) | ((flag & 1) << 4);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~0x20) | ((flag & 1) << 5);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~0x40) | ((flag & 1) << 6);
                FileRead(hFile, &flag, 1, 0);
                pDoor->door_flags = (pDoor->door_flags & ~0x80) | (flag << 7);
                FileRead(hFile, &flag, 1, 0);
                pDoor->extra_flags = (pDoor->extra_flags & ~1) | (flag & 1);
            } else {
                action_flags = 0;
                progress_delay = 0;
                FileRead(hFile, &action_flags, 2, 0);
                if ((action_flags & 1) != 0) {
                    pDoor->door_flags |= 1;
                } else {
                    pDoor->door_flags &= ~1;
                }
                if ((action_flags & 2) != 0) {
                    pDoor->door_flags |= 2;
                } else {
                    pDoor->door_flags &= ~2;
                }
                if ((action_flags & 4) != 0) {
                    pDoor->door_flags |= 4;
                } else {
                    pDoor->door_flags &= ~4;
                }
                if ((action_flags & 8) != 0) {
                    pDoor->door_flags |= 8;
                } else {
                    pDoor->door_flags &= ~8;
                }
                if ((action_flags & 0x10) != 0) {
                    pDoor->door_flags |= 0x10;
                } else {
                    pDoor->door_flags &= ~0x10;
                }
                if ((action_flags & 0x20) != 0) {
                    pDoor->door_flags |= 0x20;
                } else {
                    pDoor->door_flags &= ~0x20;
                }
                if ((action_flags & 0x40) != 0) {
                    pDoor->door_flags |= 0x40;
                } else {
                    pDoor->door_flags &= ~0x40;
                }
                if ((action_flags & 0x80) != 0) {
                    pDoor->door_flags |= 0x80;
                } else {
                    pDoor->door_flags &= ~0x80;
                }
                if ((action_flags & 0x100) != 0) {
                    pDoor->extra_flags |= 1;
                } else {
                    pDoor->extra_flags &= ~1;
                }
                FileRead(hFile, &progress_delay, 2, 0);
                if (m_lData1 != 0 && progress_delay != 0) {
                    if (m_pEvent == 0) {
                        float duration;

                        m_pEvent = new W8TriggerEvent;
                        m_pEvent->trigger = this;
                        m_pEvent->action = 2;
                        if (m_lData1 < 0) {
                            duration = 10.0f;
                        } else {
                            duration = static_cast<float>(m_lData1);
                        }
                        m_pEvent->timer.SetDuration(duration);
                        m_pEvent->timer.Restart();
                        m_pEvent->repeat = 1;
                    }
                    m_pEvent->timer.SetProgress(progress_delay * g_float_005ec128);
                    if (g_timed_events.IndexOf(m_pEvent) == -1) {
                        g_timed_events.Add(m_pEvent);
                    }
                }
                FileRead(hFile, &item, 2, 0);
                pDoor->item = static_cast<short>(item);
            }
        }
    }

    if (version > 1) {
        unsigned char has_world_item;

        FileRead(hFile, &has_world_item, sizeof(has_world_item), 0);
        if (has_world_item != 0) {
            world_item_group = LoadItem(hFile, 0);
        }
        FileRead(hFile, &items_generated, sizeof(items_generated), 0);
        FileRead(hFile, &item_group_seed, sizeof(item_group_seed), 0);
        FileRead(hFile, &gold, sizeof(gold), 0);
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
bool LoadWorldTriggers(W8World* world, int hFile)
{
    int trigger_count = world->triggers->GetCount();
    int index = 0;
    bool header_ok = true;
    bool finished = false;

    for (;;) {
        /* Retail read `tag` (and the tag-5 name/id below) uninitialised when a
           FileRead short-circuited; the recovery keeps that read. */
        char tag;

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
            if (!header_ok || !trigger->Load(hFile, tag)) {
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
                if (!header_ok || !trigger->Load(hFile, tag)) {
                    header_ok = false;
                } else {
                    header_ok = true;
                }
                ++index;
            } else {
                Trigger* scratch = new Trigger;

                scratch->Load(hFile, tag);
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
void SaveWorldTriggers(W8World* world, int hFile)
{
    W8GrowableVector<Trigger*>* triggers = world->triggers;

    for (int index = 0; index < triggers->GetCount(); ++index) {
        if (!(*triggers->GetAt(index))->Save(hFile)) {
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0043d120
void SaveTriggerActionData(W8World* world, int handle)
{
    int trigger_count = world->triggers->GetCount();
    int saved_count = 0;
    int index;
    int version = 1;

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->inline_action_data[0] != '\0') {
            ++saved_count;
        }
    }

    FileWrite(handle, &version, sizeof(version), 0);
    FileWrite(handle, &saved_count, sizeof(saved_count), 0);

    for (index = 0; index < trigger_count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if (trigger->inline_action_data[0] != '\0') {
            FileWrite(handle, trigger->name, 0x80, 0);
            FileWrite(handle, trigger->inline_action_data,
                      sizeof(trigger->inline_action_data), 0);
        }
    }
}

/* Read the trigger action-data chunk written by SaveTriggerActionData:
   a version/count header, then per record the trigger name and its 0x100-byte
   payload. Records for missing triggers are skipped with a seek. */
// FUNCTION: WIZ8 0x0043d1f0
bool LoadTriggerActionData(int handle)
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
            FileRead(handle, trigger->inline_action_data,
                     sizeof(trigger->inline_action_data), 0);
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
    : action(-1), timer(), m_pCountdown(0), trigger(0), repeat(0), completed(0)
{
}

// FUNCTION: WIZ8 0x004409a0
W8TriggerEvent::~W8TriggerEvent() {}

class W8TriggerShakeEvent : public W8TriggerEvent {
public:
    W8TriggerShakeEvent();
    virtual void Update() override;

    W8CameraShakeEffect* effect;
    int intensity;
    bool reverse;
};

static_assert(sizeof(W8TriggerShakeEvent) == 0x44, "W8TriggerShakeEvent_must_be_0x44");

// VTABLE: WIZ8 0x005ec140
// class W8TriggerShakeEvent

/* Retail ICF folds this class's deleting destructor onto W8TriggerEvent's
   retained body at 0x00440980; there is no distinct retail emission to mark. */

W8TriggerShakeEvent::W8TriggerShakeEvent() : effect(0), intensity(1), reverse(0) {}

// GLOBAL: WIZ8 0x006599a0
srVector3T<float> g_trigger_camera;

static void OnItemDialogClosed(W8DialogBase* base);

/* Advance every world trigger: raise the item picker when a prop-bearing
   activation asks for one, then run proximity activations for kind-two
   triggers. The camera position is cached when no trigger fired. */
// FUNCTION: WIZ8 0x00443ae0
void UpdateWorldTriggers(W8World* world)
{
    srVector3T<float> camera;
    bool activated = false;
    bool running = false;

    GetCameraPosition(&camera);
    int count = world->triggers->GetCount();
    for (int index = 0; index < count; ++index) {
        Trigger* trigger = *world->triggers->GetAt(index);
        if ((trigger->flags & W8_TRIGGER_ITEM_PICKER) != 0 && trigger->m_pProp != 0 &&
            (trigger->m_pProp->GetAnimationState() < 2 ||
             trigger->m_pProp->Rep()->animation_playing == 0)) {
            trigger->GenerateItemGroup();
            if (g_modal_owner == 0 && trigger->world_item_group != 0) {
                W8TriggerItemPickerDialog* dialog = new W8TriggerItemPickerDialog;
                if (dialog != 0) {
                    dialog->m_destroy_callback_context = trigger;
                    dialog->SetItemGroup(trigger->world_item_group);
                    dialog->m_destroy_callback = OnItemDialogClosed;
                    gXStatus.item_pick_pending = 0;
                    g_modal_owner = dialog;
                }
            }
        }
        if (g_environment_load_flag != 0 && trigger->trigger_kind == 2 &&
            (trigger->flags & W8_TRIGGER_ENABLED) != 0 &&
            (trigger->flags & W8_TRIGGER_POSITIONED) != 0 &&
            (trigger->flags & W8_TRIGGER_PLANE) == 0) {
            srVector3T<float> trigger_position(trigger->position.x, trigger->position.y,
                                               trigger->position.z);
            float distance = (trigger_position - camera).Length();
            if (trigger->range_maximum <= distance) {
                if ((trigger->flags & W8_TRIGGER_RUNNING) != 0) {
                    trigger->FinishAction();
                }
            } else if ((trigger->flags & W8_TRIGGER_RUNNING) == 0 &&
                       trigger->range_minimum <= distance) {
                activated = true;
                if ((trigger->flags & W8_TRIGGER_EXCLUSIVE) == 0 || !running) {
                    trigger->Run(-1);
                    running = true;
                }
            }
        }
    }
    if (world == g_world && !activated) {
        g_trigger_camera = camera;
    }
}

/* The item picker's destroy callback: hand its items back to the owning
   trigger and clear the trigger's pending-picker bit. */
// FUNCTION: WIZ8 0x004456c0
static void OnItemDialogClosed(W8DialogBase* base)
{
    W8TriggerItemPickerDialog* dialog = static_cast<W8TriggerItemPickerDialog*>(base);

    if (dialog != 0) {
        dialog->ReturnItemsToGroup();
        static_cast<Trigger*>(dialog->m_destroy_callback_context)->flags &=
            ~W8_TRIGGER_ITEM_PICKER;
    }
}

// FUNCTION: WIZ8 0x00443D30
void UpdateTimedTriggerEvents(void)
{
    for (int index = 0; index < g_timed_events.GetCount(); ++index) {
        W8TriggerEvent* event = *g_timed_events.GetAt(index);
        event->Update();
        if (event->completed != 0) {
            g_timed_events.RemoveAt(index);
            --index;
            if (event->trigger != 0) {
                event->trigger->m_pEvent = 0;
            }
            delete event;
        }
    }
}

// FUNCTION: WIZ8 0x00444ec0
void W8TriggerShakeEvent::Update()
{
    if (effect == 0) {
        float intensity = this->intensity / g_float_005ecf9c;

        if (intensity < g_float_one) {
            intensity = g_float_one;
        }
        effect = CreateCameraShakeEffect(m_pCountdown->m_duration_seconds, 0, intensity, 0, 0);
        effect->flags &= ~2;
        if (reverse != 0) {
            effect->flags |= 0x10;
        }
    }

    if ((effect->flags & 1) == 0) {
        delete effect;
        effect = 0;
        if (trigger != 0) {
            trigger->FinishAction();
        }
        if (repeat != 0) {
            completed = true;
        }
    }
}

/* Create one shake-camera event and queue it on the world's timed-event list.
   The caller passes the intensity, the effect duration and the optional
   countdown duration, all scaled by the trigger unit's 0.001 factor. */
// FUNCTION: WIZ8 0x00444F70
bool CreateTriggerShakeEvent(int intensity, float duration, float countdown_duration, bool reverse)
{
    W8TriggerShakeEvent* pEvent = new W8TriggerShakeEvent;

    if (pEvent == 0) {
        srAssertFail("pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1372,
                     "Out of memory creating shake camera event.");
    }
    pEvent->repeat = 1;
    pEvent->intensity = intensity;
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
    pEvent->timer.SetDuration(duration * g_float_005ec128);
    pEvent->timer.Restart();
    pEvent->reverse = reverse;
    g_timed_events.Add(pEvent);
    return 1;
}

// FUNCTION: WIZ8 0x004447F0
void Trigger::CompleteItemInteraction()
{
    W8TriggerActionData* action_data = m_pActionData;
    lock_state.device_state.completed = 1;
    if (action_data != 0 && action_data->type == 10) {
        static_cast<W8DoorTriggerActionData*>(action_data)->door_flags &= ~4;
    }
}

/* Run the trigger now when its action data selects the immediate path, or
   restart its timed-event clocks when it already owns a queued event. The
   immediate path sets the running flag around Run so nested activation sees
   it; the timed path only touches clocks for events still queued. */
// FUNCTION: WIZ8 0x00444750
void Trigger::Activate()
{
    W8TriggerActionData* action_data = m_pActionData;
    if (action_data != 0 && action_data->type == 10 &&
        (lock_state.lock_type == 0 || lock_state.device_state.completed != 0) &&
        (static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 4) == 0) {
        if ((static_cast<W8DoorTriggerActionData*>(action_data)->door_flags & 1) == 0) {
            running = 1;
            Run(-1);
            running = 0;
        } else {
            W8TriggerEvent* event = m_pEvent;
            if (event != 0 && g_timed_events.IndexOf(event) != -1) {
                event->timer.Restart();
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

    if ((flags & W8_TRIGGER_POSITIONED) != 0 || m_pProp != 0) {
        if (m_pProp == 0) {
            center.Set(position.x, position.y, position.z);
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
            g_octree->QueryObjects(&locations, &lower, &upper, W8_OCTREE_KIND_LOCATION, -1);
        for (unsigned int index = 0; index < count; ++index) {
            int location_id = locations[index];
            if (location_id == 0) {
                break;
            }
            unsigned int monster_index = MonsterGetIndexByLocationID(
                0x1246, "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", location_id, 1);
            W8MonsterInfo* monster_info = MonsterGetScriptPartByLocationIndex(monster_index);
            if (monster_info != 0 && monster_info->p3D != 0) {
                srVector3T<float> monster_position = monster_info->p3D->GetPosition();
                if ((monster_position - center).Length() <= radius) {
                    return 1;
                }
            }
        }
    }

    if (include_party) {
        srVector3T<float> party_position = g_startup_world->GetPosition();
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
    if ((flags & W8_TRIGGER_POSITIONED) == 0) {
        if (m_pProp == 0) {
            SOUNDPARMS options;
            memset(&options, -1, sizeof(options));
            options.uiVolume = (g_settings.sound_effects_volume * volume) / 0x7f;
            SoundPlay((STR)sound_name, &options);
            return 0;
        }
        m_pProp->GetCenterPosition(&position);
    } else {
        position.Set(this->position.x, this->position.y, this->position.z);
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
    if (action == 2) {
        unsigned short flags = timer.m_flags;

        if (g_combat_inactive == 0) {
            if ((flags & 8) != 0 || (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
                g_shared_timer_flag0 != 0) {
                return;
            }
            timer.m_flags = flags | 8;
            timer.m_start = timer.GetTime() - timer.m_start;
            return;
        }
        if ((flags & 8) != 0 || (g_shared_timer_paused != 0 && (flags & 1) == 0) ||
            g_shared_timer_flag0 != 0) {
            timer.m_flags = flags & ~8;
            timer.m_start = timer.GetTime() - timer.m_start;
            timer.SetDuration(-1.0f);
        }
    }

    if (timer.GetProgress() <= 1.0f || trigger->HasActorWithinRadius(5000.0f, 1) != 0) {
        return;
    }

    switch (action) {
    case 0x23: {
        W8Dice dice;
        SetDice(&dice, 1, 6, 2);
        ApplyRolledHealthChangeToParty(&dice, 0, 0);
        trigger->UpdateActionAnimation();
        break;
    }

    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b:
        trigger->uses_remaining = trigger->m_lData1;
        completed = true;
        break;

    case 0x0c: {
        if (trigger != 0) {
            srVector3T<float> source;
            srVector3T<float> target;
            srVector3T<float> transformed;
            srVector3T<float> axis;
            srMatrix3T<float> rotation;

            source = trigger->position;
            target = source;
            target.z += 100.0f;

            axis.Set(0.0, 0.0, 1.0);
            rotation.vectors[0].Set(trigger->direction.x, trigger->direction.y,
                                    trigger->direction.z);
            rotation.vectors[1].Set(1.0, 0.0, 0.0);
            rotation.vectors[2].Set(0.0, 1.0, 0.0);

            if (trigger->angle != 0.0f) {
                rotation.RotateAroundAxis(sin(trigger->angle), cos(trigger->angle),
                                          axis);
            }

            transformed.x = DotProduct(rotation.vectors[1], target);
            transformed.y = DotProduct(rotation.vectors[2], target);
            transformed.z = DotProduct(axis, target);
            FireMissile(static_cast<unsigned int>(trigger->m_lData1), &source, &transformed, 0,
                        1, 1, 50000.0f);
        }
        break;
    }

    case 0x3f: {
        Trigger* target = FindTriggerByName(trigger->m_pacRecipients);
        if (target != 0) {
            target->Run(-1);
        }
        break;
    }

    case 0x47: {
        char* name = trigger->m_pacRecipients;
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
                particle->persisted = 1;
                particle->SetActive(0);
            }
        }
        break;
    }

    case 2:
        if (trigger->m_pActionData != 0 && trigger->m_pActionData->type == 10 &&
            (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->door_flags & 1) !=
                0) {
            trigger->Run(-1);
        }
        break;

    default:
        break;
    }

    if (repeat != 0) {
        completed = true;
    }
}

/* A state-driven Prop has one location variable per animation slot.  Ensure
   the complete set exists for the loaded level and select slot zero as the
   initial active state. */
// FUNCTION: WIZ8 0x00445200
void InitializeStateDrivenPropVariables(Trigger* trigger)
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
        while (variable_id < g_location_variable_names.GetCount()) {
            if (_stricmp(*g_location_variable_names.GetAt(variable_id), name) == 0 &&
                *g_location_variable_levels.GetAt(variable_id) == g_status.current_level) {
                break;
            }
            ++variable_id;
        }
        if (variable_id == g_location_variable_names.GetCount()) {
            char* variable_name = new char[strlen(name) + 1];
            if (variable_name == 0) {
                srAssertFail("pacVariableName",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x109c, 0);
            }
            strcpy(variable_name, name);
            g_location_variable_names.Add(variable_name);
            g_location_variable_values.Add(slot == 0);
            g_location_variable_levels.Add(g_status.current_level);
        }
    }
}

/* Set a location variable by name when it belongs to the current level. An
   unknown name fails the lookup assertion; the value only lands while the
   value list still covers the found index. */
// FUNCTION: WIZ8 0x00444030
void SetTriggerVariableByName(const char* name, int value)
{
    int count = g_location_variable_names.GetCount();
    int index;

    for (index = 0; index < count; ++index) {
        if (_stricmp(*g_location_variable_names.GetAt(index), name) == 0 &&
            *g_location_variable_levels.GetAt(index) == g_status.current_level) {
            break;
        }
    }
    if (index >= count) {
        index = -1;
        srAssertFail("iVar != BAD_INDEX", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                     0x10c9, 0);
    }
    if (index < g_location_variable_values.GetCount()) {
        *g_location_variable_values.GetAt(index) = value;
    }
}

// VTABLE: WIZ8 0x005ec138
// class W8TriggerActionData

W8TriggerActionData::W8TriggerActionData() : type(-1) {}

// FUNCTION: WIZ8 0x00445ee0
W8TriggerActionData::~W8TriggerActionData() {}

/* Type 5 installs 0x005EC148 after constructing the common base. Its deleting
   destructor folds with the type-10 wrapper at 0x00445EC0: both call the
   common destructor, then scalar operator delete when requested. */
// VTABLE: WIZ8 0x005ec148
// class W8EnvironmentTriggerActionData

// VTABLE: WIZ8 0x005ec134
// class W8DoorTriggerActionData

// VTABLE: WIZ8 0x005ec158
// class W8StringTriggerActionData

// FUNCTION: WIZ8 0x00443750
W8StringTriggerActionData::~W8StringTriggerActionData()
{
    if (owned_string != 0) {
        delete[] owned_string;
    }
}

/* Clear the running bit and run every comma-separated recipient trigger once
   when the link-out and state-gate bits are set. */
// FUNCTION: WIZ8 0x00441590
void Trigger::RunLinkedTriggers()
{
    char* recipient;

    flags &= ~W8_TRIGGER_RUNNING;
    if ((flags & W8_TRIGGER_FIRE_LINKED) != 0 &&
        (flags & W8_TRIGGER_LINK_ON_DEACTIVATE) != 0 && m_pacRecipients != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer, recipient);
            char* comma = strchr(g_trigger_parse_buffer, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer);
            if (trigger != 0) {
                trigger->Run(-1);
            }
        }
    }
}

/* Store the trigger position and flag the representation dirty; an item
   representation is moved and re-transformed in place. */
// FUNCTION: WIZ8 0x004416f0
void Trigger::SetPosition(srVector3T<float>* position)
{
    flags |= W8_TRIGGER_POSITIONED;
    this->position = *position;
    if (rep_item != 0 && m_bRepType == 1) {
        rep_item->SetLocation(position);
        rep_item->ApplyRepTransform();
    }
}

// FUNCTION: WIZ8 0x004417c0
W8TriggerActionData* ReadDoorTriggerActionData(int handle)
{
    W8DoorTriggerActionData* data = new W8DoorTriggerActionData;
    data->type = 10;
    data->door_flags = 0x40;
    data->extra_flags &= ~1;
    data->item = -1;
    data->linked_trigger[0] = 0;
    data->position.SetZero();

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
            data->door_flags |= 1 << bit;
        } else {
            data->door_flags &= ~(1 << bit);
        }
    }
    if (flags[8] != 0) {
        data->extra_flags |= 1;
    } else {
        data->extra_flags &= ~1;
    }
    data->item = item;
    strcpy(data->linked_trigger, linked_trigger);
    if (has_position != 0) {
        data->position = position;
        data->extra_flags |= 2;
    }
    return data;
}

// FUNCTION: WIZ8 0x00441a20
Trigger* Trigger::CreateAndLoadLevelTrigger(int handle, W8World* world)
{
    /* Retail read these uninitialised when the FileRead chain short-circuited; the recovery keeps
       that read. */
    Trigger* trigger = 0;
    unsigned char record_version;
    unsigned char record_type;
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
        trigger->trigger_id = g_status.next_trigger_id++;
        trigger->m_pWorld = world;
        trigger->flags = (trigger->flags & ~0x20U) | W8_TRIGGER_ON;
    }

    switch (record_type) {
    case 1: {
        unsigned char version;
        int byte0;
        int byte1;
        float animate_states;
        int range;
        int action;
        int value_ac;
        int animate_action;
        unsigned char packed_flags;
        unsigned char enabled;
        float minimum_range = 0.0f;
        int action_value = 0;
        char recipients[0x100];
        char sound[0x80];

        FileRead(handle, &version, 1, 0);
        FileRead(handle, &byte0, 4, 0);
        FileRead(handle, &byte1, 4, 0);
        FileRead(handle, &animate_states, 4, 0);
        FileRead(handle, &range, 4, 0);
        FileRead(handle, &action, 4, 0);
        FileRead(handle, &value_ac, 4, 0);
        FileRead(handle, &animate_action, 4, 0);
        FileRead(handle, &packed_flags, 1, 0);
        FileRead(handle, &enabled, 1, 0);
        FileRead(handle, trigger->name, sizeof(trigger->name), 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        FileRead(handle, sound, sizeof(sound), 0);
        sprintf(trigger->action_data, "data\\sound\\%s", sound);
        _strupr(trigger->name);
        _strupr(recipients);

        if (version > 1) {
            char surface_id[0x40];
            int id = -1;

            FileRead(handle, &minimum_range, 4, 0);
            FileRead(handle, surface_id, sizeof(surface_id), 0);
            /* The id is the four characters after a leading NUL: retail stores
               a terminator at surface_id[5] before atoi, and tests the world
               geometry without a null check on its owner. */
            if (surface_id[0] == 0) {
                if (world->game_data->geometry_index != 0) {
                    surface_id[5] = 0;
                    id = atoi(surface_id + 1);
                }
            } else {
                id = -1;
            }
            trigger->surface_id = id;
        }
        if (version > 2) {
            unsigned char has_action_data;
            FileRead(handle, &has_action_data, 1, 0);
            if (has_action_data != 0) {
                unsigned char action_data_kind;
                FileRead(handle, &action_data_kind, 1, 0);
                if (action_data_kind == 1) {
                    trigger->m_pActionData = ReadDoorTriggerActionData(handle);
                    trigger->state_index =
                        (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->door_flags &
                         1) != 0;
                }
            }
        }
        if (version > 3) {
            FileRead(handle, &action_value, 4, 0);
        }

        trigger->trigger_kind = 1;
        trigger->range_maximum = range * 500.0f;
        trigger->range_minimum = minimum_range * 500.0f;
        trigger->m_pacRecipients = new char[strlen(recipients) + 1];
        strcpy(trigger->m_pacRecipients, recipients);
        trigger->state_count = static_cast<unsigned char>(byte1);
        trigger->state_index = 0;
        trigger->state_direction = 1;
        trigger->cycle_bounce = byte0;
        trigger->action_value = action_value;
        trigger->initial_action = action;
        if (animate_states != 0.0f)
            trigger->flags |= W8_TRIGGER_ANIMATE_STATES;
        else
            trigger->flags &= ~W8_TRIGGER_ANIMATE_STATES;
        if (animate_action != 0)
            trigger->flags |= W8_TRIGGER_ANIMATE_ACTION;
        else
            trigger->flags &= ~W8_TRIGGER_ANIMATE_ACTION;
        if (enabled != 0)
            trigger->flags |= W8_TRIGGER_ENABLED;
        else
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        if ((packed_flags & 2) != 0)
            trigger->flags |= W8_TRIGGER_LINK_ON_DEACTIVATE;
        else
            trigger->flags &= ~W8_TRIGGER_LINK_ON_DEACTIVATE;
        if ((packed_flags & 1) != 0)
            trigger->flags |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags &= ~W8_TRIGGER_FIRE_LINKED;
        world->triggers->Add(trigger);
        if (trigger->name[0] != 0) {
            trigger->setName(trigger->name);
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
        unsigned char fire_linked;
        unsigned char enabled;
        char recipients[0x100];
        unsigned char plane = 0;
        unsigned char keep_on_finish = 0;
        int value_ac = 0;

        FileRead(handle, &version, 1, 0);
        FileRead(handle, &range, 4, 0);
        FileRead(handle, &x, 4, 0);
        FileRead(handle, &y, 4, 0);
        FileRead(handle, &z, 4, 0);
        FileRead(handle, &action, 4, 0);
        FileRead(handle, &value_c8, 4, 0);
        FileRead(handle, &fire_linked, 1, 0);
        FileRead(handle, &enabled, 1, 0);
        FileRead(handle, trigger->name, sizeof(trigger->name), 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        _strupr(trigger->name);
        _strupr(recipients);
        if (version > 1) {
            FileRead(handle, &plane, 1, 0);
            FileRead(handle, trigger->representation_vectors,
                     sizeof(trigger->representation_vectors), 0);
            for (int vector = 0; vector < 4; ++vector) {
                trigger->representation_vectors[vector] *= 500.0f;
            }
        }
        if (version > 2) {
            unsigned char unused;
            char action_string[0x80];
            FileRead(handle, &trigger->angle, 4, 0);
            FileRead(handle, &trigger->direction.x, 4, 0);
            FileRead(handle, &trigger->direction.y, 4, 0);
            FileRead(handle, &trigger->direction.z, 4, 0);
            FileRead(handle, &unused, 1, 0);
            FileRead(handle, action_string, sizeof(action_string), 0);
            if (action == 17) {
                W8StringTriggerActionData* data = new W8StringTriggerActionData;
                data->type = 6;
                data->owned_string = 0;
                delete trigger->m_pActionData;
                data->owned_string = new char[strlen(action_string) + 1];
                strcpy(data->owned_string, action_string);
                trigger->m_pActionData = data;
            }
        }
        if (version > 3) {
            FileRead(handle, &keep_on_finish, 1, 0);
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
                        legacy_vertices[index] *= 500.0f;
                    }
                    FileRead(handle, &legacy_flags[0], 1, 0);
                    FileRead(handle, &legacy_flags[1], 1, 0);
                }
            }
        }

        trigger->trigger_kind = 2;
        trigger->position.Set(x * 500.0f, y * 500.0f, z * 500.0f);
        trigger->range_maximum = range * 500.0f;
        trigger->m_bRepType = 3;
        trigger->action_value = value_ac;
        trigger->initial_action = static_cast<unsigned short>(action);
        trigger->searchable = value_c8;
        trigger->flags |= W8_TRIGGER_POSITIONED;
        if (fire_linked != 0)
            trigger->flags |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags &= ~W8_TRIGGER_FIRE_LINKED;
        if (enabled != 0)
            trigger->flags |= W8_TRIGGER_ENABLED;
        else
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        if (keep_on_finish != 0)
            trigger->flags |= W8_TRIGGER_KEEP_ON_FINISH;
        else
            trigger->flags &= ~W8_TRIGGER_KEEP_ON_FINISH;
        if (plane == 1)
            trigger->flags |= W8_TRIGGER_PLANE;
        trigger->m_pacRecipients = new char[strlen(recipients) + 1];
        strcpy(trigger->m_pacRecipients, recipients);
        if ((trigger->flags & W8_TRIGGER_PLANE) != 0 && world->game_data != 0 &&
            world->game_data->geometry_index != 0) {
            world->game_data->AddTriggerPlane(trigger->representation_vectors, trigger);
        }
        if (trigger->initial_action == 0x34 && trigger->m_pacRecipients[0] == 0) {
            trigger->range_minimum = 0.0f;
            trigger->range_maximum = 0.0f;
        }
        world->triggers->Add(trigger);
        if (trigger->name[0] != 0) {
            trigger->setName(trigger->name);
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
        position *= 500.0f;
        region_u *= 500.0f;
        region_v *= 500.0f;
        radius *= 500.0f;
        AddAmbientSound(world, optional_name, &config, &position, &region_u, &region_v, volume_min,
                        volume_max, speed_min, speed_max, time_min, time_max, radius,
                        unbounded == 0, looping, &region_center, region_angle, &region_min,
                        &region_max, shared);
        return 0;
    }

    case 4: {
        unsigned char version;
        unsigned char packed_flags;
        unsigned char enabled;
        unsigned char fire_linked;
        unsigned char link_on_deactivate;
        unsigned char keep_on_finish;
        unsigned char flag;
        unsigned char alternate_toggles;
        int initial_action;
        int alternate_action;
        int fallback_action;
        char recipients[0x100];
        unsigned char searchable;
        char location_variable[0x100];
        unsigned char consume_item;
        int action_value;
        unsigned char animate_action;
        char sound[0x80];
        float representation_scale = 1.0f;
        unsigned char initial_location_value = 0;
        unsigned char flag0 = 0;
        unsigned char representation_kind = 0;

        FileRead(handle, &version, 1, 0);
        FileRead(handle, trigger->name, sizeof(trigger->name), 0);
        FileRead(handle, &packed_flags, 1, 0);
        FileRead(handle, &enabled, 1, 0);
        FileRead(handle, &fire_linked, 1, 0);
        FileRead(handle, &link_on_deactivate, 1, 0);
        FileRead(handle, &keep_on_finish, 1, 0);
        FileRead(handle, &flag, 1, 0);
        FileRead(handle, &alternate_toggles, 1, 0);
        FileRead(handle, &initial_action, 4, 0);
        FileRead(handle, &alternate_action, 4, 0);
        FileRead(handle, &fallback_action, 4, 0);
        FileRead(handle, recipients, sizeof(recipients), 0);
        FileRead(handle, &searchable, 1, 0);
        FileRead(handle, location_variable, sizeof(location_variable), 0);
        FileRead(handle, &consume_item, 1, 0);
        FileRead(handle, &action_value, 4, 0);
        FileRead(handle, &animate_action, 1, 0);
        FileRead(handle, sound, sizeof(sound), 0);
        sprintf(trigger->action_data, "data\\sound\\%s", sound);
        _strupr(trigger->name);
        _strupr(recipients);
        _strupr(location_variable);

        if (version > 1) {
            FileRead(handle, &trigger->m_lData1, 4, 0);
            FileRead(handle, &trigger->m_lData2, 4, 0);
            FileRead(handle, &trigger->m_lData3, 4, 0);
            FileRead(handle, &representation_scale, 4, 0);
            FileRead(handle, &initial_location_value, 1, 0);
            FileRead(handle, &flag0, 1, 0);
            FileRead(handle, &trigger->action_data_mode, 1, 0);
            FileRead(handle, &trigger->sound_volume, 1, 0);
            int unused;
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
            FileRead(handle, &unused, 4, 0);
        }

        trigger->trigger_kind = ((packed_flags & 1) != 0 || searchable == 1) ? 1 : 2;
        if ((packed_flags & 2) != 0)
            trigger->flags |= W8_TRIGGER_CAN_RUN_LINKED;
        if ((packed_flags & 4) != 0)
            trigger->flags |= W8_TRIGGER_ONCE;
        if ((packed_flags & 8) != 0)
            trigger->flags |= W8_TRIGGER_EXCLUSIVE;
        if ((packed_flags & 0x10) != 0)
            trigger->flags |= W8_TRIGGER_REACTIVATE_LINKED;
        if ((packed_flags & 0x20) != 0)
            trigger->flags |= 0x400000;

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
        trigger->initial_action = static_cast<unsigned short>(initial_action);
        trigger->fallback_action = static_cast<unsigned short>(fallback_action);
        trigger->searchable = searchable;
        if (enabled != 0)
            trigger->flags |= W8_TRIGGER_ENABLED;
        else
            trigger->flags &= ~W8_TRIGGER_ENABLED;
        if (fire_linked != 0)
            trigger->flags |= W8_TRIGGER_FIRE_LINKED;
        else
            trigger->flags &= ~W8_TRIGGER_FIRE_LINKED;
        if (link_on_deactivate != 0)
            trigger->flags |= W8_TRIGGER_LINK_ON_DEACTIVATE;
        else
            trigger->flags &= ~W8_TRIGGER_LINK_ON_DEACTIVATE;
        if (keep_on_finish != 0)
            trigger->flags |= W8_TRIGGER_KEEP_ON_FINISH;
        else
            trigger->flags &= ~W8_TRIGGER_KEEP_ON_FINISH;
        if (flag != 0)
            trigger->flags |= 0x1000;
        else
            trigger->flags &= ~0x1000U;
        if (consume_item != 0)
            trigger->flags |= W8_TRIGGER_CONSUME_ITEM;
        else
            trigger->flags &= ~W8_TRIGGER_CONSUME_ITEM;
        if (animate_action != 0)
            trigger->flags |= W8_TRIGGER_ANIMATE_ACTION;
        else
            trigger->flags &= ~W8_TRIGGER_ANIMATE_ACTION;
        if (alternate_toggles != 0)
            trigger->flags |= W8_TRIGGER_ALTERNATE_TOGGLES;
        else
            trigger->flags &= ~W8_TRIGGER_ALTERNATE_TOGGLES;
        if (trigger->alternate_action != 0)
            trigger->flags |= W8_TRIGGER_HAS_ALTERNATE;

        unsigned char value_b0;
        unsigned char animate_states;
        unsigned char value_b3;
        char required_states[0x100];
        char state_to_modify[0x100];
        unsigned char value_b4;
        FileRead(handle, &value_b0, 1, 0);
        FileRead(handle, &animate_states, 1, 0);
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
        if (animate_states != 0)
            trigger->flags |= W8_TRIGGER_ANIMATE_STATES;
        else
            trigger->flags &= ~W8_TRIGGER_ANIMATE_STATES;

        if (trigger->m_pacStateToMod != 0 &&
            (initial_location_value == 0 || initial_location_value == 1)) {
            int variable_id = 0;
            while (variable_id < g_location_variable_names.GetCount()) {
                if (_stricmp(*g_location_variable_names.GetAt(variable_id),
                             trigger->m_pacStateToMod) == 0 &&
                    *g_location_variable_levels.GetAt(variable_id) == g_status.current_level) {
                    break;
                }
                ++variable_id;
            }
            if (variable_id == g_location_variable_names.GetCount()) {
                char* variable_name = new char[strlen(trigger->m_pacStateToMod) + 1];
                if (variable_name == 0) {
                    srAssertFail("pacVariableName",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x109c, 0);
                }
                strcpy(variable_name, trigger->m_pacStateToMod);
                g_location_variable_names.Add(variable_name);
                g_location_variable_values.Add(initial_location_value);
                g_location_variable_levels.Add(g_status.current_level);
            }
        }

        int unused_value;
        FileRead(handle, &trigger->range_minimum, 4, 0);
        FileRead(handle, &trigger->range_maximum, 4, 0);
        FileRead(handle, trigger->inline_action_data, sizeof(trigger->inline_action_data),
                 0);
        FileRead(handle, &unused_value, 4, 0);
        _strupr(trigger->inline_action_data);
        trigger->range_minimum *= 500.0f;
        trigger->range_maximum *= 500.0f;
        if (version > 2) {
            FileRead(handle, sound, sizeof(sound), 0);
            sprintf(trigger->alternate_action_data, "data\\sound\\%s", sound);
            if (flag0 != 0)
                trigger->flags |= W8_TRIGGER_ALTERNATE_ACTION;
            else
                trigger->flags &= ~W8_TRIGGER_ALTERNATE_ACTION;
        }

        if ((packed_flags & 1) == 0) {
            FileRead(handle, &representation_kind, 1, 0);
            if (representation_kind == 1) {
                FileRead(handle, &trigger->position, sizeof(srVector3T<float>), 0);
                FileRead(handle, &trigger->angle, 4, 0);
                FileRead(handle, &trigger->direction, sizeof(srVector3T<float>), 0);
                trigger->position *= 500.0f;
                trigger->flags |= W8_TRIGGER_POSITIONED;
            } else if (representation_kind == 2) {
                FileRead(handle, trigger->representation_vectors,
                         sizeof(trigger->representation_vectors), 0);
                for (int vector = 0; vector < 4; ++vector) {
                    trigger->representation_vectors[vector] *= 500.0f;
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
                trigger->m_pActionData = ReadDoorTriggerActionData(handle);
                trigger->state_index =
                    (static_cast<W8DoorTriggerActionData*>(trigger->m_pActionData)->door_flags &
                     1) != 0;
            } else if (action_data_kind == 2) {
                unsigned char count;
                srVector3T<float> legacy_vertices[36];
                unsigned char legacy_flags[2];
                FileRead(handle, &count, 1, 0);
                for (int index = 0; index < 36; ++index) {
                    FileRead(handle, &legacy_vertices[index], sizeof(legacy_vertices[index]), 0);
                    legacy_vertices[index] *= 500.0f;
                }
                FileRead(handle, &legacy_flags[0], 1, 0);
                FileRead(handle, &legacy_flags[1], 1, 0);
            }
        }

        if (representation_kind == 2 && world->game_data != 0 &&
            world->game_data->geometry_index != 0) {
            world->game_data->AddTriggerPlane(trigger->representation_vectors, trigger);
        }
        if (trigger->initial_action == 0x34 && trigger->m_pacRecipients == 0) {
            trigger->range_minimum = 0.0f;
            trigger->range_maximum = 0.0f;
        }
        world->triggers->Add(trigger);
        if (trigger->name[0] != 0) {
            trigger->setName(trigger->name);
        }
        if (searchable == 1) {
            RegisterSearchableTrigger(trigger);
        }
        if (trigger->initial_action == 0x0c) {
            if (trigger->m_lData1 < 0 ||
                trigger->m_lData1 >= static_cast<int>(g_missile_table_count)) {
                srAssertFail(
                    "((pTrigger->m_lData1 >= 0) && (pTrigger->m_lData1 < Missile::GetNumTypes()))",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf14,
                    reinterpret_cast<const char*>(String(
                        "Trigger %s: You must enter a valid missile number", trigger->name)));
            }
            if (trigger->m_lData2 == -1) {
                srAssertFail(
                    "(pTrigger->m_lData2!=(-1))",
                    "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf15,
                    reinterpret_cast<const char*>(String(
                        "Trigger %s: You must enter a time value in Data2", trigger->name)));
            }
            if (trigger->m_lData2 < 0) {
                trigger->m_pEvent = new W8TriggerEvent;
                if (trigger->m_pEvent == 0) {
                    srAssertFail("pTrigger->m_pEvent",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0xf1e, 0);
                }
                trigger->m_pEvent->action = static_cast<short>(trigger->initial_action);
                trigger->m_pEvent->timer.SetDuration(abs(trigger->m_lData2) * 0.001f);
                trigger->m_pEvent->timer.Restart();
                trigger->m_pEvent->trigger = trigger;
                trigger->flags |= W8_TRIGGER_RUNNING;
                g_timed_events.Add(trigger->m_pEvent);
            }
        } else if (trigger->initial_action > 0x24 && trigger->initial_action < 0x2c &&
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
    *position = this->position;
}

// FUNCTION: WIZ8 0x00441780
bool Trigger::HasActionMessage()
{
    return (flags & W8_TRIGGER_CAN_RUN_LINKED) != 0;
}

// FUNCTION: WIZ8 0x00441790
bool Trigger::RequiresItem()
{
    if (required_item_id >= 0) {
        return true;
    }
    if (m_pActionData != 0 && m_pActionData->type == 0xa && m_pActionData != 0 &&
        static_cast<W8DoorTriggerActionData*>(m_pActionData)->item != -1) {
        return true;
    }
    return false;
}

// VTABLE: WIZ8 0x005ec104
// class srClassSupport<Trigger,srClass,1,65544>

// FUNCTION: WIZ8 0x0043ba10
Trigger::Trigger()
{
    trigger_kind = 0;
    trigger_id = 0;
    flags = 0;
    range_minimum = 0.0f;
    range_maximum = 0.0f;
    action_value = 0;
    state_count = 0;
    state_index = 0;
    state_direction = 0;
    cycle_bounce = 0;
    state_mod_mode = 0;
    searchable = 0;
    angle = 0.0f;
    m_bRepType = 0;
    m_pProp = 0;
    rep_item = 0;
    m_pWorld = 0;
    initial_action = 0;
    alternate_action = 0;
    fallback_action = 0;
    action = 0;
    action_state = 1;
    m_pActionData = 0;
    m_pacRecipients = 0;
    m_pacRequiredStates = 0;
    m_pacStateToMod = 0;
    m_pEvent = 0;
    world_item_group = 0;
    items_generated = 0;
    activation_callback = 0;
    running = 0;

    surface_id = -1;
    sound_volume = -1;
    required_item_id = -1;
    lock_state.device_id = -1;
    lock_state.key_id = -1;
    lock_state.last_interaction_clock = -1;
    lock_state.lock_type = 0;
    lock_state.difficulty = 0;
    lock_state.device_state.completed = 0;
    lock_state.lock_countdown = 0;
    memset(lock_state.device_state.pins, 0, sizeof(lock_state.device_state.pins));

    flags |= W8_TRIGGER_ON;
    name[0] = 0;
    position.SetZero();
    action_data[0] = 0;
    item_group_seed = GetTickCount() + Random(30000);
    gold = 0;
    uses_remaining = 0;
    trigger_id = g_status.next_trigger_id++;
}

// FUNCTION: WIZ8 0x00440d00
void Trigger::UpdateActionAnimation()
{
    char* action_data = this->action_data;

    if ((flags & W8_TRIGGER_ANIMATE_ACTION) == 0 &&
        (flags & W8_TRIGGER_ALTERNATE_ACTION) == 0) {
        return;
    }
    if ((flags & W8_TRIGGER_ALTERNATE_ACTION) != 0) {
        if (action_data_mode == 0) {
            if ((flags & W8_TRIGGER_ALTERNATE_SELECTED) == 0) {
                flags |= W8_TRIGGER_ALTERNATE_SELECTED;
            } else {
                action_data = alternate_action_data;
                flags &= ~W8_TRIGGER_ALTERNATE_SELECTED;
            }
        } else if (action_data_mode == 1) {
            if (action == alternate_action) {
                PlayActionSound(alternate_action_data, sound_volume);
                return;
            }
        } else if (action_data_mode == 2 && action == fallback_action) {
            PlayActionSound(alternate_action_data, sound_volume);
            return;
        }
    }
    PlayActionSound(action_data, sound_volume);
}

// FUNCTION: WIZ8 0x00441110
void Trigger::FinishAction()
{
    bool was_running = ((flags & W8_TRIGGER_RUNNING) != 0);
    bool action_completed = false;
    char* recipient;

    flags &= ~W8_TRIGGER_RUNNING;

    if (trigger_kind == 1) {
        if (action != 0x39) {
            goto reactivate_linked_triggers;
        }
        if (m_pEvent != 0) {
            m_pEvent->completed = true;
        }
        g_trigger_action_active = false;
        goto finish_linked_triggers;
    }

    if (trigger_kind != 2) {
        goto reactivate_linked_triggers;
    }

    switch (action) {
    case 0x0c:
        if (m_pEvent != 0 && m_lData2 > 0) {
            m_pEvent->completed = true;
        }
        action_completed = true;
        break;

    case 0x23: {
        g_timed_events.Remove(m_pEvent);
        action_completed = true;
        break;
    }

    case 0x39:
        if (m_pEvent != 0) {
            m_pEvent->completed = true;
        }
        g_trigger_action_active = false;
        action_completed = true;
        break;
    }

    if ((flags & W8_TRIGGER_KEEP_ON_FINISH) == 0 && action != 0) {
        if (action == 4) {
            recipient = m_pacRecipients;
            action_completed = false;
            while (recipient != 0) {
                strcpy(g_trigger_parse_buffer, recipient);
                char* comma = strchr(g_trigger_parse_buffer, ',');
                if (comma == 0) {
                    recipient = 0;
                } else {
                    recipient = strchr(recipient, ',') + 1;
                    *comma = '\0';
                }

                stLight* light = FindLightByName(g_trigger_parse_buffer, 0);
                if (light != 0) {
                    light->m_save_marked = 1;
                    if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                        light->setFlag(srNode::FLAG_DISABLE);
                    } else {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                    action_completed = true;
                }
            }
        } else if (action == 0x22) {
            if (m_pActionData == 0) {
                srAssertFail("m_pActionData", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             2822, "Trigger.cpp: Dark Area doesn't have action data");
            }
            SetWorldEnvironmentIntensity(g_world,
                                         static_cast<W8EnvironmentTriggerActionData*>(m_pActionData)
                                             ->previous_environment);
            delete m_pActionData;
            m_pActionData = 0;
            goto finish_linked_triggers;
        }

        if (!action_completed) {
            goto reactivate_linked_triggers;
        }
    }

finish_linked_triggers:
    if ((flags & W8_TRIGGER_FIRE_LINKED) != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer, recipient);
            char* comma = strchr(g_trigger_parse_buffer, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer);
            if (trigger != 0) {
                trigger->FinishAction();
            }
        }
    }

reactivate_linked_triggers:
    if (was_running != 0 && (flags & W8_TRIGGER_ON) != 0 &&
        (flags & W8_TRIGGER_REACTIVATE_LINKED) != 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            strcpy(g_trigger_parse_buffer, recipient);
            char* comma = strchr(g_trigger_parse_buffer, ',');
            if (comma == 0) {
                recipient = 0;
            } else {
                recipient = strchr(recipient, ',') + 1;
                *comma = '\0';
            }

            Trigger* trigger = FindTriggerByName(g_trigger_parse_buffer);
            if (trigger != 0) {
                trigger->Run(-1);
            }
        }
    }
}

// FUNCTION: WIZ8 0x00445480
char* NextTriggerRecipient(char** cursor)
{
    char* comma;

    if (*cursor == 0) {
        return 0;
    }
    strcpy(g_trigger_parse_buffer, *cursor);
    comma = strchr(g_trigger_parse_buffer, ',');
    if (comma != 0) {
        *cursor = strchr(*cursor, ',') + 1;
        *comma = '\0';
    } else {
        *cursor = 0;
    }
    return g_trigger_parse_buffer;
}

// FUNCTION: WIZ8 0x004409b0
void Trigger::CommitActionResult(bool apply_state_changes)
{
    char* recipient;

    UpdateActionAnimation();

    if (m_pacRecipients != 0 && (flags & W8_TRIGGER_FIRE_LINKED) != 0 &&
        (flags & W8_TRIGGER_LINK_ON_DEACTIVATE) == 0) {
        recipient = m_pacRecipients;
        while (recipient != 0) {
            Trigger* trigger = FindTriggerByName(NextTriggerRecipient(&recipient));
            if (trigger != 0) {
                bool was_running = ((flags & W8_TRIGGER_RUNNING) != 0);
                flags |= W8_TRIGGER_RUNNING;
                trigger->Run(m_lData1);
                flags =
                    (flags & ~W8_TRIGGER_RUNNING) | (was_running != 0 ? W8_TRIGGER_RUNNING : 0);
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
            state_value = *g_location_variable_values.GetAt(state_id) == 0;
            state_id = GetLocationVarIDByName(m_pacStateToMod);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4297, 0);
            }
        } else {
            goto show_action_message;
        }
        g_location_variable_values.SetAt(state_id, state_value);
    }

show_action_message:
    if ((flags & W8_TRIGGER_CAN_RUN_LINKED) != 0) {
        const char* level_folder = GetLevelFolderName(GetLoadedLevelID());
        int message_id;

        if (action_state == 2) {
            message_id = m_lData1;
        } else if (action_state == 3) {
            message_id = m_lData2;
        } else if (action_state == 4) {
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
    flags |= W8_TRIGGER_FIRED;
}

/* Resolve an entity or a five-character level/entrance code, then either
   request the level transition or move the current world to the resolved
   destination. The destination Trigger supplies the local portal orientation
   when the name was not one of the world's named entities. */
// FUNCTION: WIZ8 0x00440dd0
void Trigger::RunDestination(const char* destination)
{
    srVector3T<float> destination_position;
    srVector3T<float> destination_direction;
    srVector3T<float> source_position;
    srMatrix3T<float> rotation;
    /* Retail's named-entity path reads saved storage for these integers;
       authored initialization remains unresolved. */
    int location_id;
    int entrance;
    int current_location;
    float angle;
    bool named_entity;

    if (g_modal_owner != 0) {
        return;
    }

    ResetInactiveLevelDataVectors();
    current_location = g_status.current_level;
    named_entity =
        FindEntityByName(destination, &destination_position, &angle, &destination_direction);
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
    ResetInactiveLevelDataVectors();

    if (location_id != current_location) {
        RequestLevelTransition(location_id, entrance,
                               m_lData1 < 0 ? 0 : static_cast<unsigned char>(m_lData1));
        return;
    }

    if (!named_entity) {
        Trigger* target = FindTriggerByName(destination);

        destination_position = target->position;
        angle = target->angle;
        destination_direction = target->direction;
    }

    source_position.Set(position.x, position.y, position.z);
    g_octree->AdjustPortalDestination(&destination_position, &source_position);
    SetWorldScenePosition(GetWorld(), &destination_position);

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
    W8Vector<W8WorldItem*> items(5);
    srVector3T<float> position;
    unsigned int table_id;
    unsigned int maximum_items;
    int index;

    if (inline_action_data[0] == '\0' || items_generated != 0) {
        return;
    }

    srand(item_group_seed);
    table_id = FindItemTableByName(inline_action_data);
    if (table_id == static_cast<unsigned int>(-1)) {
        return;
    }

    maximum_items = RollDice(&g_item_tables[table_id]->item_count_dice);
    GenerateItemsFromTable(&items, table_id, maximum_items);
    if (world_item_group == 0) {
        world_item_group = SpawnItem(0x23c, &position, 0, 0);
    }
    for (index = 0; index < items.GetCount(); ++index) {
        ItemInfoAddToGroup(world_item_group, *items.GetAt(index));
    }
    gold = RollDice(&g_item_tables[table_id]->gold_dice);
    items_generated = 1;
}

/* The trigger's container world item, materialized on demand: when asked to
   create and none exists yet a bare container item is spawned into the world
   and remembered. */
// FUNCTION: WIZ8 0x00445670
W8WorldItem* Trigger::GetOrCreateItemGroup(bool create)
{
    srVector3T<float> position;

    if (create != 0 && world_item_group == 0) {
        world_item_group = SpawnItem(0x23c, &position, 0, 0);
    }
    return world_item_group;
}

/* After a selected-prop Run: while g_trigger_feedback is clear, post either the
   special-item notice (required_item_id != -1) or the nothing-happened notice. */
// FUNCTION: WIZ8 0x004456E0
void Trigger::PrintNothingHappenedOrSpecialItemRequired()
{
    if (g_trigger_feedback != 0) {
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
    ActivationCallback callback = activation_callback;
    bool apply_state_changes = true;
    bool action_succeeded = false;

    if (!SelectAction()) {
        return;
    }
    if (callback != 0 && !callback(this)) {
        return;
    }

    if (trigger_kind == 2) {
        switch (action) {
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
            switch0 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch1");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch1 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch2");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch2 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch3");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch3 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch4");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch4 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch5");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch5 = *g_location_variable_values.GetAt(id);
            id = GetLocationVarIDByName("Switch6");
            if (id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x10dd, 0);
            }
            switch6 = *g_location_variable_values.GetAt(id);

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
            ApplyItemEffectToRandomCharacter(Random(2) != 0 ? g_condition_reaction
                                                            : g_condition_reaction_alt,
                                             -1, 0, g_character_event_no_flags);
            flags |= W8_TRIGGER_RUNNING;
            goto commit_action;

        case 0x11:
            if ((flags & W8_TRIGGER_RUNNING) != 0) {
                break;
            }
            flags |= W8_TRIGGER_RUNNING;
            goto commit_action;

        case 0x22: {
            float previous_value = GetWorldEnvironmentIntensity(g_world);

            delete m_pActionData;
            m_pActionData = new W8EnvironmentTriggerActionData;
            m_pActionData->type = 5;
            static_cast<W8EnvironmentTriggerActionData*>(m_pActionData)->previous_environment =
                previous_value;
            SetWorldEnvironmentIntensity(g_world, 0.0f);
            flags |= W8_TRIGGER_RUNNING;
            goto commit_action;
        }

        case 0x23:
            if (m_pEvent == 0) {
                m_pEvent = new W8TriggerEvent;
                if (m_pEvent == 0) {
                    srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                                 0x5fa, 0);
                }
                m_pEvent->trigger = this;
                m_pEvent->action = static_cast<short>(action);
                m_pEvent->timer.SetDuration(0.5f);
            } else {
                if (g_timed_events.IndexOf(m_pEvent) != -1) {
                    srAssertFail("glsTimedEvents.Find(m_pEvent) == -1",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x603, 0);
                }
            }
            m_pEvent->timer.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
            }
            g_timed_events.Add(m_pEvent);
            flags |= W8_TRIGGER_RUNNING;
            goto commit_action;

        case 0x34:
            if (m_pacRecipients == 0 || m_pacRecipients[0] == '\0') {
                break;
            }
            RunDestination(m_pacRecipients);
            goto commit_action;

        default:
            break;
        }
    }

    if (trigger_kind == 1) {
        switch (action) {
        case 1: {
            W8DoorTriggerActionData* action_data = 0;

            if (m_bRepType != 2 || m_pProp == 0 || state_index != 0 ||
                m_pProp->Rep()->animation_playing != 0) {
                break;
            }
            if (m_pActionData != 0 && m_pActionData->type == 10) {
                action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->door_flags & 4) != 0 &&
                action_data->item != -1) {
                if (FindItemOnParty(action_data->item, 0, 0, 2, 0) == 0) {
                    ShowNoticef(3, L"Your party doesn't have required key.");
                    break;
                }
                action_data->door_flags &= ~4;
            }

            m_pProp->SetRepresentationActive(1, true);
            state_index = 1;
            if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
                m_pWorld->game_data->SetInterfaceState(surface_id, 1);
            }
            flags |= W8_TRIGGER_RUNNING;
            if (action_data != 0) {
                action_data->door_flags |= 1;
            }

            if (m_lData1 != 0) {
                if (m_pEvent != 0 && g_timed_events.IndexOf(m_pEvent) != -1) {
                    m_pEvent->timer.Restart();
                    if (m_pEvent->m_pCountdown != 0) {
                        m_pEvent->m_pCountdown->Restart();
                    }
                    goto commit_action;
                }

                m_pEvent = new W8TriggerEvent;
                m_pEvent->trigger = this;
                m_pEvent->action = 2;
                m_pEvent->timer.SetDuration(m_lData1 < 0 ? 10.0f
                                                             : static_cast<float>(m_lData1));
                m_pEvent->timer.Restart();
                m_pEvent->repeat = 1;
                g_timed_events.Add(m_pEvent);
            }
            goto commit_action;
        }

        case 2: {
            bool active;

            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            active = m_pProp->Rep()->animation_playing;
            if (active) {
                break;
            }
            state_index = state_index == 1 ? 0 : 1;
            m_pProp->SetRepresentationActive(state_index, true);
            if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
                m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
            }
            flags |= W8_TRIGGER_RUNNING;
            if (m_pActionData != 0 && m_pActionData->type == 10) {
                if (state_index == 0) {
                    static_cast<W8DoorTriggerActionData*>(m_pActionData)->door_flags &= ~1;
                } else {
                    static_cast<W8DoorTriggerActionData*>(m_pActionData)->door_flags |= 1;
                }
            }
            goto commit_action;
        }

        case 0x2c: {
            W8DoorTriggerActionData* action_data = 0;
            bool was_active;

            if (m_pActionData != 0 && m_pActionData->type == 10) {
                action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
            }
            if (action_data != 0 && (action_data->door_flags & 4) != 0 &&
                action_data->item != -1) {
                if (FindItemOnParty(action_data->item, 0, 0, 2, 0) == 0) {
                    ShowNoticef(3, L"Your party doesn't have required key.");
                    break;
                }
                action_data->door_flags &= ~4;
            }
            if (m_bRepType != 2 || m_pProp == 0) {
                break;
            }
            was_active = m_pProp->Rep()->animation_playing;
            m_pProp->SetRepresentationActive(!was_active, true);
            state_index = state_index == 0;
            if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
                m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
            }
            if (!was_active) {
                flags |= W8_TRIGGER_RUNNING;
            } else {
                flags &= ~W8_TRIGGER_RUNNING;
            }
            if (action_data != 0) {
                action_data->door_flags = (action_data->door_flags & ~1) | (state_index & 1);
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

            if ((flags & W8_TRIGGER_ANIMATE_STATES) != 0) {
                W8AnimObj* animation;
                /* 0x0043E4C3 tests the AnimObjListCount result signed. */
                int count;
                int index;

                if (m_pProp == 0 || m_bRepType != 2) {
                    srAssertFail("m_pProp && m_bRepType == TRIGGER_REP_PROP",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x592, 0);
                }
                animation = m_pProp->Rep()->animation;
                if (AnimationIsRunning(animation) == 1) {
                    count = AnimObjListCount(animation, 2);
                    for (index = 0; index < count; ++index) {
                        W8PathAI* path =
                            AnimObjListEntry(animation, 2, static_cast<signed char>(index));
                        PathAIUpdate(path,
                                     previous <= static_cast<signed char>(state_index) ? 1 : -1);
                    }
                } else {
                    m_pProp->SetSetting66(static_cast<char>(state_index));
                }
            }
            goto commit_action;
        }

        case 0x37: {
            int tag = source == -1 ? m_lData1 : source;

            if (m_bRepType == 2 && m_pProp != 0 && tag != -1) {
                m_pProp->Rep()->SelectAnimationSlot(static_cast<unsigned char>(tag));
                m_pProp->SetRepresentationActive(1, true);
                state_index = static_cast<unsigned char>(tag);
                goto commit_action;
            }
            break;
        }

        default:
            break;
        }
    }

    switch (action) {
    case 4:
    case 0x30:
    case 0x31: {
        char* recipient = m_pacRecipients;

        while (recipient != 0) {
            stLight* light = FindLightByName(NextTriggerRecipient(&recipient), 0);
            if (light != 0) {
                light->m_save_marked = 1;
                if (action == 4) {
                    if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                        light->setFlag(srNode::FLAG_DISABLE);
                    } else {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                } else if (action == 0x30) {
                    if (light->testFlag(srNode::FLAG_DISABLE) != 0) {
                        light->clearFlag(srNode::FLAG_DISABLE);
                    }
                } else if (light->testFlag(srNode::FLAG_DISABLE) == 0) {
                    light->setFlag(srNode::FLAG_DISABLE);
                }
                action_succeeded = true;
            }
        }
        if (trigger_kind == 2) {
            flags |= W8_TRIGGER_RUNNING;
        }
        if (!action_succeeded) {
            return;
        }
        break;
    }

    case 0:
    case 0x38:
        break;

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
                if (action == 0x2d) {
                    target->flags |= W8_TRIGGER_ON;
                } else if (action == 0x2e) {
                    target->flags &= ~W8_TRIGGER_ON;
                } else {
                    target->flags ^= W8_TRIGGER_ON;
                }
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
            return;
        }
        break;
    }

    case 3: {
        bool was_active;
        bool action_succeeded = true;

        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        was_active = m_pProp->Rep()->animation_playing;

        if (inline_action_data[0] != '\0') {
            if (g_status.item_in_cursor) {
                srVector3T<float> item_position;
                W8WorldItem* item =
                    CreateWorldItem(&g_status.item_in_hand, &item_position, 3, 0);

                if (item != 0) {
                    if (world_item_group == 0) {
                        srVector3T<float> group_position;
                        world_item_group = SpawnItem(0x23c, &group_position, 0, 0);
                    }
                    ItemInfoAddToGroup(world_item_group, item);
                }
                g_trigger_feedback = 1;
                return;
            }
            if ((flags & W8_TRIGGER_ITEM_PICKER) != 0) {
                return;
            }

            g_trigger_feedback = 1;
            if (items_generated == 0) {
                GenerateItemGroup();
            }
            if (world_item_group != 0) {
                int item_count = ItemInfoGetNumInGroup(world_item_group) - 1;
                W8WorldItem* item;
                int contained_items = 0;

                if (item_count != 1 && m_pProp->Rep()->subcycle != 0) {
                    action_succeeded = false;
                }

                item = world_item_group->next;
                while (item != 0) {
                    ++contained_items;
                    if (!item->item.identified) {
                        PartyAttemptsToIdentifyItem(&item->item, 0);
                    }
                    item = item->next;
                }
                if (contained_items > 1) {
                    gXStatus.item_pick_pending = 1;
                }

                if (gold != 0) {
                    AddPartyGold(gold, 1);
                    gold = 0;
                }

                if (item_count == 1) {
                    if (m_pProp->Rep()->subcycle == 0) {
                        ApplyItemEffectToRandomCharacter(Random(2) != 0 ? g_container_event
                                                                        : g_container_event_alt,
                                                         -1, 0, g_character_event_no_flags);
                    }
                } else if (item_count == 2 && !g_status.item_in_cursor) {
                    item = world_item_group->next;
                    CopyItemInstance(&g_status.item_in_hand, &item->item, 0, 1);
                    ItemInfoRemoveFromGroup(world_item_group, item);
                    if (m_pProp->Rep()->subcycle != 0) {
                        goto toggle_item_prop;
                    }
                } else {
                    flags |= W8_TRIGGER_ITEM_PICKER;
                }

                if (!action_succeeded) {
                    return;
                }
            }
        }

    toggle_item_prop:
        m_pProp->SetRepresentationActive(!was_active, true);
        state_index = state_index == 0;
        if (m_pWorld->game_data != 0 && surface_id >= 0) {
            m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
        }
        if (!was_active) {
            flags |= W8_TRIGGER_RUNNING;
        } else {
            flags &= ~W8_TRIGGER_RUNNING;
        }
        break;
    }

    case 0x32:
    case 0x33:
        if (m_bRepType != 2 || m_pProp == 0) {
            return;
        }
        if ((action == 0x32 && m_pProp->Rep()->animation_playing != 0) ||
            (action == 0x33 && m_pProp->Rep()->animation_playing == 0)) {
            return;
        }
        m_pProp->SetRepresentationActive(action == 0x32, true);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
            m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
        }
        if (action == 0x32) {
            flags |= W8_TRIGGER_RUNNING;
        } else {
            flags &= ~W8_TRIGGER_RUNNING;
        }
        break;

    case 0x0f: {
        char* recipient = m_pacRecipients;

        if (recipient == 0) {
            return;
        }
        while (recipient != 0) {
            Trigger* target = FindTriggerByName(NextTriggerRecipient(&recipient));
            if (target != 0) {
                bool was_running = ((flags & W8_TRIGGER_RUNNING) != 0);
                flags |= W8_TRIGGER_RUNNING;
                target->Run(m_lData1);
                flags =
                    (flags & ~W8_TRIGGER_RUNNING) | (was_running != 0 ? W8_TRIGGER_RUNNING : 0);
                action_succeeded = true;
            }
        }
        if (!action_succeeded) {
            return;
        }
        break;
    }

    case 0x10:
        SetCameraSwayMode(m_pWorld->camera, source > 0 ? 1 : -1);
        return;

    case 0x36:
        if (action_state == 4 && action_data_mode == 2) {
            PlayActionSound(alternate_action_data, 0);
        } else {
            PlayActionSound(this->action_data, 0);
        }
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
        SetDice(&dice, static_cast<unsigned char>(m_lData1), static_cast<unsigned char>(m_lData2),
                static_cast<short>(m_lData3));
        ApplyRolledHealthChangeToParty(&dice, 0, 1);
        if (trigger_kind == 2) {
            flags |= W8_TRIGGER_RUNNING;
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
        for (index = 0; index < static_cast<int>(PLLength(gXStatus.plsMonsterGroupList)); ++index) {
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

        group->members_active = 1;
        monster_info->p3D->m_pRep->animation_playing = 1;
        monster_info->p3D->m_pRep->animation_playing = 1;
        monster_info->p3D->m_pRep->timer =
            g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
        monster_info->p3D->ResetRepresentation();
        monster_info->p3D->ResetPathAI();
        monster_info->p3D->reactivated = 1;
        return;
    }

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
            event->trigger = this;
            event->action = static_cast<short>(action);
            event->timer.SetDuration(m_lData2 == -1 ? 0.07f : m_lData2 * 0.001f);
            event->timer.Restart();
            event->intensity = m_lData1 == -1 ? 800 : m_lData1;

            if (m_lData3 != -1) {
                delete event->m_pCountdown;
                event->m_pCountdown = new W8GameTimer;
                if (event->m_pCountdown == 0) {
                    srAssertFail("m_pCountdown",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x12de, 0);
                }
                event->m_pCountdown->SetDuration(abs(m_lData3) * 0.001f);
                event->m_pCountdown->Restart();
                if (m_lData3 < 0) {
                    event->reverse = 1;
                }
            }
        } else {
            event = static_cast<W8TriggerShakeEvent*>(m_pEvent);
            event->timer.Restart();
            if (event->m_pCountdown != 0) {
                event->m_pCountdown->Restart();
            }
        }

        if ((flags & W8_TRIGGER_RUNNING) == 0) {
            g_timed_events.Add(m_pEvent);
            if (trigger_kind == 2) {
                flags |= W8_TRIGGER_RUNNING;
            } else if (m_lData3 == -1) {
                srAssertFail("m_lData3!=-1", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x7e4, "Non-invisible triggers with shake camera must be timed");
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
        if (m_pProp == 0 || source != m_lData1 || m_pProp->Rep()->animation_playing == 0) {
            return;
        }
        m_pProp->SetRepresentationActive(m_pProp->Rep()->animation_playing == 0, true);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
            m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
        }
        break;

    case 0x3c:
        if (m_pProp == 0 || source != m_lData1) {
            return;
        }
        m_pProp->SetRepresentationActive(m_pProp->Rep()->animation_playing == 0, true);
        state_index = state_index == 0;
        if (m_pWorld != 0 && m_pWorld->game_data != 0 && surface_id >= 0) {
            m_pWorld->game_data->SetInterfaceState(surface_id, state_index);
        }
        break;

    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
    case 0x2b: {
        if (uses_remaining != 0 || m_lData1 == -1) {
            if (action == 0x25) {
                RestorePartyStaminaByDice(0, 0, static_cast<short>(m_lData3));
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else if (action == 0x26) {
                HealPartyByDice(0, 0, static_cast<short>(m_lData3));
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else if (action == 0x27) {
                RestorePartySpellPoints(m_lData3);
                ShowString(gppStringList[0x722]);
                PlayActionSound("Data\\Sound\\misc\\fountain_magic.wav", 0);
            } else {
                int spell_id;
                srVector3T<double> position = g_world->camera->getLocation();

                if (action == 0x28) {
                    spell_id = 0x44;
                } else if (action == 0x29) {
                    spell_id = 0x45;
                } else if (action == 0x2a) {
                    spell_id = 0x0c;
                } else {
                    spell_id = 0x2a;
                }
                PointCastSpell(srVector3T<float>(static_cast<float>(position.x),
                                                 static_cast<float>(position.y),
                                                 static_cast<float>(position.z)),
                               spell_id, static_cast<unsigned int>(m_lData3));
                if (action == 0x2b) {
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
                m_pEvent->action = static_cast<short>(action);
                m_pEvent->timer.SetDuration(m_lData2 * 720.0f);
                m_pEvent->timer.Restart();
                m_pEvent->trigger = this;
                m_pEvent->timer.SetMode(1);
                g_timed_events.Add(m_pEvent);
            }
        }
        if (trigger_kind == 2) {
            flags |= W8_TRIGGER_RUNNING;
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

            source_position.Set(this->position.x, this->position.y, this->position.z);
            target_position = source_position;
            target_position.z += 100.0f;
            rotation.SetIdentity();
            axis = rotation.vectors[2];
            if (angle != 0.0f) {
                rotation.RotateAroundAxis(sin(angle), cos(angle), axis);
            }
            transformed = rotation.Transform(target_position);
            FireMissile(static_cast<unsigned int>(m_lData1), &source_position, &transformed, 0, 1,
                        1, 50000.0f);
        } else if (m_pEvent == 0) {
            float duration = abs(m_lData2) * 0.001f;

            if (m_lData2 > 0 && trigger_kind != 2) {
                srAssertFail("0", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x87f,
                             "Continous firing missile must be invisible trigger.");
            }
            m_pEvent = new W8TriggerEvent;
            if (m_pEvent == 0) {
                srAssertFail("m_pEvent", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                             0x883, 0);
            }
            m_pEvent->action = static_cast<short>(action);
            m_pEvent->timer.SetDuration(duration);
            m_pEvent->timer.Restart();
            m_pEvent->trigger = this;
            flags |= W8_TRIGGER_RUNNING;
            g_timed_events.Add(m_pEvent);
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
            m_pEvent->action = static_cast<short>(action);
            m_pEvent->timer.SetDuration(m_lData1 * 0.001f);
            m_pEvent->timer.Restart();
            m_pEvent->trigger = this;
            m_pEvent->repeat = 1;
        } else {
            if (g_timed_events.IndexOf(m_pEvent) != -1) {
                srAssertFail("glsTimedEvents.Find(m_pEvent) == -1",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x8ae, 0);
            }
            m_pEvent->timer.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
            }
        }
        g_timed_events.Add(m_pEvent);
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
            g_location_variable_values.SetAt(state_id, 0);
        }
        state_index = m_pProp->Rep()->AdvanceAnimationSegment();
        m_pProp->SetRepresentationActive(1, false);
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
            g_location_variable_values.SetAt(state_id, 1);
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
            if (action == 0x41) {
                PositionAmbientSoundByName(g_world, name);
            } else if (action == 0x42) {
                StopAmbientSoundByName(g_world, name);
            } else {
                ToggleAmbientSoundByName(g_world, name);
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
                particle->persisted = 1;
                if (action == 0x44) {
                    particle->SetActive(1);
                } else if (action == 0x45) {
                    particle->SetActive(0);
                } else {
                    particle->SetActive(particle->emitting == 0);
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
                particle->persisted = 1;
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
        g_timed_events.Add(m_pEvent);
        m_pEvent->trigger = this;
        m_pEvent->action = static_cast<short>(action);
        delete m_pEvent->m_pCountdown;
        m_pEvent->m_pCountdown = new W8GameTimer;
        if (m_pEvent->m_pCountdown == 0) {
            srAssertFail("m_pCountdown", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp",
                         0x12de, 0);
        }
        m_pEvent->m_pCountdown->SetDuration(m_lData1 == -1 ? 10.0f : m_lData1 * 0.001f);
        m_pEvent->m_pCountdown->Restart();
        m_pEvent->repeat = 1;
        break;
    }

    case 0x48: {
        unsigned int count;

        if (m_pProp == 0 || m_lData1 < 0) {
            return;
        }
        count = AnimObjValue(m_pProp->Rep()->animation, 2);
        if (static_cast<int>(count) <= m_lData1) {
            return;
        }
        m_pProp->SetSetting66(static_cast<char>(m_lData1));
        break;
    }

    case 0x49:
        if (m_pProp == 0 || m_lData1 < 0) {
            return;
        }
        m_pProp->SetAnimationSpeed(static_cast<float>(m_lData1));
        break;

    case 0x4a: {
        char* recipient = m_pacRecipients;

        if (recipient == 0 || m_lData1 < 0) {
            return;
        }
        while (recipient != 0) {
            W8Prop* prop = FindPropByName(g_world, NextTriggerRecipient(&recipient));
            if (prop != 0) {
                prop->SetAnimationSpeed(static_cast<float>(m_lData1));
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
        selected = static_cast<unsigned char>(GetTickCount() % count);
        recipient = m_pacRecipients;
        do {
            NextTriggerRecipient(&recipient);
            if (index == selected) {
                break;
            }
            ++index;
        } while (recipient != 0);
        RunDestination(g_trigger_parse_buffer);
        break;
    }

    default:
        return;
    }

commit_action:
    if (running == 0) {
        g_trigger_feedback = 1;
    }
    CommitActionResult(apply_state_changes);
}

// FUNCTION: WIZ8 0x00444600
bool Trigger::CanRunLinkedTriggers()
{
    char* recipient;

    if (m_pProp != 0 && m_pProp->Rep()->animation_playing != 0) {
        return 0;
    }
    recipient = m_pacRecipients;
    while (recipient != 0) {
        char* comma;
        Trigger* trigger;

        strcpy(g_trigger_parse_buffer, recipient);
        comma = strchr(g_trigger_parse_buffer, ',');
        if (comma != 0) {
            recipient = strchr(recipient, ',') + 1;
            *comma = '\0';
        } else {
            recipient = 0;
        }
        trigger = FindTriggerByName(g_trigger_parse_buffer);
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

    if (g_combat_inactive == 0 && m_pActionData != 0 && m_pActionData->type == 10 &&
        (static_cast<W8DoorTriggerActionData*>(m_pActionData)->door_flags & 1) != 0) {
        return 0;
    }

    if (((flags & W8_TRIGGER_RUNNING) != 0 && action != 0x39) ||
        (flags & W8_TRIGGER_ON) == 0 ||
        ((flags & W8_TRIGGER_ONCE) != 0 && (flags & W8_TRIGGER_FIRED) != 0)) {
        action_state = 1;
        return 0;
    }

    if (m_pacRequiredStates != 0) {
        if (strchr(m_pacRequiredStates, ',') == 0) {
            int state_id = GetLocationVarIDByName(m_pacRequiredStates);
            if (state_id == -1) {
                srAssertFail("iVar != BAD_INDEX",
                             "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 4317, 0);
            }
            if (*g_location_variable_values.GetAt(state_id) == 0) {
                action = fallback_action;
                action_state = 4;
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
                if (*g_location_variable_values.GetAt(state_id) == 0) {
                    action = fallback_action;
                    action_state = 4;
                    fallback_selected = true;
                    break;
                }
            }
        }
    }

    if (m_pActionData == 0 || m_pActionData->type != 10) {
        if (required_item_id >= 0) {
            if (GetItemInHand() == required_item_id) {
                if ((flags & W8_TRIGGER_CONSUME_ITEM) != 0) {
                    RemovePartyItemByID(required_item_id, 0);
                    required_item_id = -1;
                }
            } else {
                if (m_lData2 == 1 && activation_callback != 0) {
                    activation_callback(this);
                }
                action = fallback_action;
                action_state = 4;
                fallback_selected = true;
            }
        }
    } else {
        W8DoorTriggerActionData* action_data = static_cast<W8DoorTriggerActionData*>(m_pActionData);
        bool linked_trigger_blocked = false;

        if (m_pProp != 0 && m_pProp->Rep()->animation_playing != 0) {
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
            if (m_pEvent == 0 || g_timed_events.IndexOf(m_pEvent) == -1) {
                return 0;
            }
            m_pEvent->timer.Restart();
            if (m_pEvent->m_pCountdown != 0) {
                m_pEvent->m_pCountdown->Restart();
            }
            if (running == 0) {
                g_trigger_feedback = 1;
            }
            return 0;
        }

        if ((action_data->door_flags & 4) != 0 && action_data->item != -1) {
            if (GetItemInHand() != action_data->item) {
                if (m_lData2 == 1 && activation_callback != 0) {
                    activation_callback(this);
                }
                action = fallback_action;
                action_state = 4;
                fallback_selected = true;
            } else {
                lock_state.device_state.completed = 1;
                action_data->door_flags &= ~4;
                if (action_data->linked_trigger[0] != '\0') {
                    Trigger* linked_trigger;
                    action_state = 1;
                    result = false;
                    linked_trigger = FindTriggerByName(action_data->linked_trigger);
                    if (linked_trigger != 0) {
                        linked_trigger->Run(-1);
                        if (running == 0) {
                            g_trigger_feedback = 1;
                        }
                    }
                }
            }
        }
    }

    if (lock_state.lock_type != 0 && lock_state.device_state.completed == 0 && running == 0) {
        if (lock_state.lock_type == 1) {
            g_trigger_feedback = 1;
            OpenLockInteraction(this);
            return 0;
        }
        if (lock_state.lock_type == 2) {
            g_trigger_feedback = 1;
            OpenTrapInteraction(this);
            return 0;
        }
    }

    if (!fallback_selected) {
        if ((flags & W8_TRIGGER_USE_ALTERNATE) == 0) {
            action = initial_action;
            action_state = 2;
            if ((flags & W8_TRIGGER_HAS_ALTERNATE) != 0) {
                flags |= W8_TRIGGER_USE_ALTERNATE;
            }
        } else {
            action = alternate_action;
            action_state = 3;
            if ((flags & W8_TRIGGER_ALTERNATE_TOGGLES) != 0) {
                flags &= ~W8_TRIGGER_USE_ALTERNATE;
            }
        }
    } else if (action == 0) {
        action_state = 1;
        return 0;
    }
    return result;
}

// FUNCTION: WIZ8 0x0043bc10
srClass* Trigger::vInstance()
{
    return new Trigger;
}

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
        g_timed_events.Remove(m_pEvent);
        delete m_pEvent;
    }

    if (m_pWorld != 0 && m_pWorld->triggers != 0) {
        m_pWorld->triggers->Remove(this);
    }

    if (world_item_group != 0) {
        FreeWorldItemGroup(world_item_group);
    }
}

/* Resolves a light instance by name under the stLight class node. Retail
   inlines stLight::sGetClassNode, so this emission carries the lazy
   stLight->srLight->srNode registration walk before the registry find. */
// FUNCTION: WIZ8 0x00445a10
stLight* FindLightByName(const char* name, const srRuntimeClass* relative_to)
{
    return static_cast<stLight*>(
        srCore.getRegistry()->find(stLight::sGetClassNode(), name, relative_to));
}

// FUNCTION: WIZ8 0x00443a50
int ResetNextTriggerId(void)
{
    g_status.next_trigger_id = 1;
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
    int variable_count = g_location_variable_names.GetCount();
    int variable_id;
    char** variable_name;
    int* variable_level;

    for (variable_id = 0; variable_id < variable_count; ++variable_id) {
        variable_name = g_location_variable_names.GetAt(variable_id);
        if (_stricmp(*variable_name, name) == 0) {
            variable_level = g_location_variable_levels.GetAt(variable_id);
            if (*variable_level == g_status.current_level) {
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
    variable_count = g_location_variable_names.GetCount();
    for (index = 0; index < variable_count; ++index) {
        if (_stricmp(*g_location_variable_names.GetAt(index), name) == 0 &&
            *g_location_variable_levels.GetAt(index) == g_status.current_level) {
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
    g_location_variable_names.Add(copy);
    g_location_variable_values.Add(value);
    g_location_variable_levels.Add(g_status.current_level);
}

/* The current level's value of the named location variable; asserts when the
   name is unknown. */
// FUNCTION: WIZ8 0x004440d0
int GetLocationVarValueByName(const char* name)
{
    int index;
    int variable_count;

    variable_count = g_location_variable_names.GetCount();
    for (index = 0; index < variable_count; ++index) {
        if (_stricmp(*g_location_variable_names.GetAt(index), name) == 0 &&
            *g_location_variable_levels.GetAt(index) == g_status.current_level) {
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
    return *g_location_variable_values.GetAt(index);
}

/* Write the count then each location variable's value, name and level. */
// FUNCTION: WIZ8 0x004441e0
void SaveLocationVariables(int handle)
{
    int variable_count = g_location_variable_names.GetCount();
    bool written;

    written = FileWrite(handle, &variable_count, sizeof(variable_count), 0) != 0;
    for (int index = 0; index < variable_count; ++index) {
        int value;
        char name[0x80];
        int level;

        if (!written) {
            return;
        }
        value = *g_location_variable_values.GetAt(index);
        strcpy(name, *g_location_variable_names.GetAt(index));
        level = *g_location_variable_levels.GetAt(index);
        written = FileWrite(handle, &value, sizeof(value), 0) &&
                  FileWrite(handle, name, sizeof(name), 0) &&
                  FileWrite(handle, &level, sizeof(level), 0);
    }
}

/* Read the location-variable count then each value/name/level record,
   appending a heap copy of the name to the variable vectors. */
// FUNCTION: WIZ8 0x00444310
bool LoadLocationVariables(int handle)
{
    /* Retail read these uninitialised when a FileRead short-circuited; the recovery keeps that
       read. */
    int variable_count;
    int index;
    bool read_ok;

    read_ok = FileRead(handle, &variable_count, sizeof(variable_count), 0) != 0;
    for (index = 0; index < variable_count; ++index) {
        int value;
        char name[0x80];
        int level;
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
        g_location_variable_names.Add(copy);
        g_location_variable_values.Add(value);
        g_location_variable_levels.Add(level);
    }
    return read_ok;
}

// FUNCTION: WIZ8 0x004445b0
void ReleaseAllTriggers(void)
{
    int index;

    for (index = 0; index < g_location_variable_names.GetCount(); ++index) {
        delete[] *g_location_variable_names.GetAt(index);
    }
    g_location_variable_values.Clear();
    g_location_variable_names.Clear();
    g_location_variable_levels.Clear();
}

/* Index of the prop whose trigger last tested in view; -1 until a prop
   matches. */
// GLOBAL: WIZ8 0x00606998
static int s_last_prop_index = -1;

/* Report whether any world prop's trigger representation is within its
   activation range and projects onto the screen. The remembered index is
   checked first so consecutive frames start at the prop that matched. */
// FUNCTION: WIZ8 0x00445140
bool AnyPropTriggerInView(W8World* world)
{
    srVector3T<float> position;
    unsigned int prop_count;
    int index;

    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\Trigger.cpp", 0x1395, 0);
    }
    GetCameraPosition(&position);
    prop_count = PLLength(world->plsProps);
    if (0 <= s_last_prop_index && s_last_prop_index < static_cast<int>(prop_count)) {
        W8Prop* prop = GetWorldProp(world, s_last_prop_index);

        if (prop->IsTriggerInView(&position)) {
            return 1;
        }
    }
    for (index = 0; index < static_cast<int>(prop_count); ++index) {
        W8Prop* prop = GetWorldProp(world, index);

        if (prop->IsTriggerInView(&position)) {
            s_last_prop_index = index;
            return 1;
        }
    }
    return 0;
}
