/*
 * Engine Code\Spells.cpp.
 *
 * A spell's visual side. It hangs off the same emitter record a missile does,
 * one offset along, and the four accessors below are the missile's four
 * bodies with 0x1e0 in place of 0x1dc.
 */

#include "wiz8/float_constants.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/local_code/MagicEffects.h"
#include "wiz8/layouts/gameplay_databases.h"
#include "wiz8/layouts/character.h"
#include "wiz8/local_code/HealthStaminaMana.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/engine_code/Emitter.h"
#include "wiz8/engine_code/AnimObj.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/GrObject.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/virtual_file.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/Item.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/quad.h"
#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/engine_code/stLight.hpp"
#include "wiz8/engine_code/stParticle.h"
#include "wiz8/engine_code/SoundEvent.h"
#include "wiz8/engine_code/SpellEmitterHost.h"
#include "wiz8/engine_code/SpellVisual.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/3d.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "wiz8/utility.h"
#include "wiz8/vector.h"
#include "surrender/srCamera.h"
#include "surrender/srTimer.h"
#include "soundman.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/engine_code/GameData.h"

W8GrowableVector<stSound3D*> g_sound3d_instances_65be40;

/* Names of the seven bitmap cycles in each of the four spell-visual groups,
   in load order: FLASH, EXPLOSION, TARGET and CONE. */
// GLOBAL: WIZ8 0x0060CF80
const char* g_spell_cycle_names[28] = {
    "FLASH1",     "FLASH2",     "FLASH3",     "FLASH4",     "FLASH5",     "FLASH6",
    "FLASH7",     "EXPLOSION1", "EXPLOSION2", "EXPLOSION3", "EXPLOSION4", "EXPLOSION5",
    "EXPLOSION6", "EXPLOSION7", "TARGET1",    "TARGET2",    "TARGET3",    "TARGET4",
    "TARGET5",    "TARGET6",    "TARGET7",    "CONE1",      "CONE2",      "CONE3",
    "CONE4",      "CONE5",      "CONE6",      "CONE7",
};

/* The persistent TargetCone visual the targeting code toggles on and off. */
// GLOBAL: WIZ8 0x0065BE20
W8SpellVisual* g_target_cone_visual_65be20;

static __inline int MinimumCasterLevel(int spell_level)
{
    switch (spell_level) {
    case 2:
        return 3;
    case 3:
        return 5;
    case 4:
        return 8;
    case 5:
        return 11;
    case 6:
        return 14;
    case 7:
        return 18;
    default:
        return 1;
    }
}

// FUNCTION: WIZ8 0x004ac9d0
int GetSpellTargetType(int spell_id, unsigned char normalize_single_target)
{
    int target_type = g_spell_records[spell_id].target_type;

    if (target_type == 1 && normalize_single_target) {
        target_type = 0;
    }
    return target_type;
}

// FUNCTION: WIZ8 0x004acb40
int MinimumCasterLevelForSpellLevel(int spell_level)
{
    return MinimumCasterLevel(spell_level);
}

// FUNCTION: WIZ8 0x004acba0
int GetMinimumCasterLevelForSpell(int spell_id)
{
    return MinimumCasterLevel(g_spell_records[spell_id].spell_level);
}

/* The emitter record a spell's visual hangs off. */
// FUNCTION: WIZ8 0x004ac890
W8EmitterHost* W8SpellVisual::GetRepresentation()
{
    return this->host;
}

/* The emitter it is currently coming out of. */
// FUNCTION: WIZ8 0x004ac820
W8AnimObj* W8SpellVisual::GetCurrentAnimation()
{
    return this->host->emitters[this->host->current_cycle];
}

/* That emitter's own value. */
// FUNCTION: WIZ8 0x004ac870
float W8SpellVisual::GetCurrentAnimationScale()
{
    return this->host->emitters[this->host->current_cycle]->playback_scale_08;
}

// FUNCTION: WIZ8 0x004ac8a0
W8AniMesh* W8SpellVisual::GetCurrentAniMesh()
{
    W8AnimObj* emitter = this->host->emitters[this->host->current_cycle];

    if (emitter == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x653, 0);
    }
    return emitter->entries_18[this->host->m_bLOD];
}

/* How many emitters the host has, counted by testing each for null. */
// FUNCTION: WIZ8 0x004ac840
signed char W8SpellVisual::GetTotalAnimationCount()
{
    char count = 0;

    if (this->host->emitters[0] != 0) {
        count = 1;
    }

    if (this->host->emitters[1] != 0) {
        ++count;
    }
    return count;
}

/* Hand the host's LOD to the current animation. */
// FUNCTION: WIZ8 0x004ac360
signed char W8SpellVisual::GetNumSubCycles()
{
    W8AnimObj* animation = GetCurrentAnimation();

    return static_cast<signed char>(AnimObjValue004A15D0(animation, host->m_bLOD));
}

// FUNCTION: WIZ8 0x004ac4e0
bool W8SpellVisual::IsCycleSupported(signed char cycle)
{
    if (cycle >= 28) {
        srAssertFail("bCycle<SPELL_NUM_CYCLES", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                     0x55a, 0);
    }
    return host->emitters[cycle] != 0;
}

/* Reset the host's two frame counters before the ordinary GrCycle step. */
// FUNCTION: WIZ8 0x004ac390
void W8SpellVisual::AdvanceAnimationFrame(int value, int flags)
{
    W8SpellEmitterHost* representation_before;

    host->counter_094 = 0;
    representation_before = host;
    representation_before->counter_095 = GetNumSubCycles() - 1;
    W8GrCycle::AdvanceAnimationFrame(value, flags);
}

/* Select one of the spell host's 28 emitters and rebuild its light and
   particle attachment state. */
// FUNCTION: WIZ8 0x004ac580
void W8SpellVisual::SetCycle(signed char cycle)
{
    W8GrowableVector<stLight*>* lights;
    W8AnimObj* animation;
    int index;

    if (cycle < 0 || cycle >= 28) {
        srAssertFail("bCycle >= SPELL_CYCLE_FIRST && bCycle <= SPELL_CYCLE_LAST",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x5c1, 0);
    }

    lights = *host->light_lists[host->current_cycle].GetAt(0);
    if (lights != 0) {
        for (index = 0; index < lights->GetCount(); ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(0, 1);
            if (light->definition() != 0) {
                int world_index = g_world->lights_to_update->IndexOf(light);
                if (world_index != -1) {
                    g_world->lights_to_update->RemoveAt(world_index);
                }
            }
        }
    }

    host->current_cycle = cycle;
    animation = host->emitters[cycle];
    host->active = 1;
    host->flag_06e = 1;
    if (host->SetCycleFrameLod(cycle, 0, 2) != 0) {
        host->m_bLOD = 2;
    } else if (host->SetCycleFrameLod(cycle, 0, 1) != 0) {
        host->m_bLOD = 1;
    } else {
        host->m_bLOD = 0;
    }
    host->timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    host->flag_06f = animation->value_02;
    host->flag_06d = animation->unknown_01;
    host->flag_064 = 0;

    lights = *host->light_lists[cycle].GetAt(0);
    SetLights(lights);
    if (g_render_flag_60a20c != 0 && lights != 0) {
        for (index = 0; index < lights->GetCount(); ++index) {
            stLight* light = *lights->GetAt(index);

            light->setParent(g_world->dynamic_scene, 1);
            if (light->definition() != 0) {
                g_world->lights_to_update->Add(light);
            }
        }
    }

    if (m_plsParticles != 0) {
        for (index = 0; index < m_plsParticles->GetCount(); ++index) {
            W8GrCycleParticleAttachment* event = *m_plsParticles->GetAt(index);

            if (event->cycle_00 == cycle) {
                event->particle_08->SetActive(1);
                event->particle_08->value_188 = 0;
            } else {
                event->particle_08->SetActive(0);
            }
        }
    }
}

/* Position and orient each live spell visual according to its attachment
   mode, then let the common GrCycle update submit the resulting state. */
// FUNCTION: WIZ8 0x004abe00
void W8SpellVisual::UpdateRepresentation(W8World* world)
{
    srMatrix3T<float> rotation;
    srVector3T<float> camera_position;
    srVector3T<float> position;
    bool apply_rotation = false;

    if (value_1d8 == 0) {
        float angle;
        float pitch;

        GetCameraPosition(&position);
        SetPosition004A6DF0(&position);
        rotation.SetIdentity();
        angle = GetCameraYawRadians() - g_monster_rotation_offset_005ec04c;
        if ((double)angle != g_zero_005ebb40) {
            rotation.RotateAboutY(sin((double)angle), cos((double)angle));
        }
        pitch = -GetCameraPitchRadians();
        if ((double)pitch != g_zero_005ebb40) {
            rotation.RotateAboutX(sin((double)pitch), cos((double)pitch));
        }
        apply_rotation = true;
    } else if (value_1d8 == 2) {
        W8Monster* monster = GetMonsterByLocationID(target_location_id_1dc);

        if (monster != 0) {
            srModelInstance* instance = GetCurrentModelInstance004A8250();

            while (instance != 0) {
                srVector3T<double> scale(value_1e8, value_1e8, value_1e8);
                instance->setScale(scale);
                instance = static_cast<srModelInstance*>(instance->firstChild());
            }

            srVector3T<float> minimum;
            srVector3T<float> maximum;
            srVector3T<float> monster_position = monster->GetPosition();

            monster->GetAnimationBounds(&minimum, &maximum);
            position.x = monster_position.x;
            position.y = monster_position.y + (maximum.y - minimum.y) * g_float_005ebc7c;
            position.z = monster_position.z;
            SetPosition004A6DF0(&position);
        }
    } else if (value_1d8 == 3) {
        GetCurrentModelInstance004A8250();
        if (flag_1e7 == 0) {
            GetCameraPosition(&camera_position);
            if (value_1ec == 0) {
                SetPosition004A6DF0(&camera_position);
                g_gd_camera_65a0f8->GetRotationMatrix(&rotation);
                apply_rotation = true;
            } else {
                W8Monster* monster = GetMonsterByLocationID(value_1ec);

                if (monster != 0) {
                    if (monster->Query(6) == 0x19 &&
                        monster->GetSpellPosition004C78E0(&position) != 0) {
                        SetPosition004A6DF0(&position);
                    }

                    rotation.SetIdentity();
                    float angle = monster->GetYaw();
                    if ((double)angle != g_zero_005ebb40) {
                        rotation.RotateAboutY(sin((double)angle), cos((double)angle));
                    }
                    float pitch = GetElevationAngle(&position, &camera_position);
                    if ((double)pitch != g_zero_005ebb40) {
                        rotation.RotateAboutX(sin((double)pitch), cos((double)pitch));
                    }
                    apply_rotation = true;
                }
            }
        }
    }

    if (apply_rotation) {
        host->SetRotation004B88D0(&rotation);
    }

    if (host->flag_378 != 0) {
        srMatrix3T<float> billboard;
        srVector3T<float> visual_position = GetPosition();
        float angle;

        billboard.SetIdentity();
        camera_position = g_gd_camera_65a0f8->m_position_08c;
        angle = GetHeadingAngle(&visual_position, &camera_position) + (float)g_camera_pi_005ec2a0;
        if ((double)angle != g_zero_005ebb40) {
            billboard.RotateAboutY(sin((double)angle), cos((double)angle));
        }
        host->SetRotation004B88D0(&billboard);
    }

    srModelInstance* instance = GetCurrentModelInstance004A8250();
    if (instance != 0) {
        static_cast<stModelInstance*>(instance)->state_178 |= 0x10;
    }
    W8GrCycle::UpdateRepresentation(world);
}

/* Step every live spell visual and drop the finished ones.

   A visual still starting, or not yet marked for removal, starts and updates
   in place; a finished one is unlinked from the world collection, hands its
   lights back through the world light boundary, and deletes itself. */
// FUNCTION: WIZ8 0x004aab80
void UpdateWorldSpellVisuals004AAB80(W8World* world)
{
    if (world == 0) {
        srAssertFail("pWorld", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x130, 0);
    }
    srVector3T<double> camera_location = world->camera->getLocation();
    int index = 0;
    int count = world->spell_visuals->GetCount();
    while (index < count) {
        W8SpellVisual* visual = *world->spell_visuals->GetAt(index);
        if (visual != 0) {
            visual->DetachRepresentation004A7A70(world);
            if (visual->started == 0 || visual->flag_1e6 == 0) {
                visual->StartIfHostActive();
                visual->UpdateRepresentation(world);
                visual->UpdateNavigation004553A0(0, 0);
            } else {
                world->spell_visuals->RemoveAt(world->spell_visuals->IndexOf(visual));
                if (visual->m_plsLights != 0) {
                    int light_count = visual->m_plsLights->GetCount();
                    while (light_count != 0) {
                        stLight* light = visual->m_plsLights->RemoveAt(0);
                        WorldRemoveLight(g_world, light);
                        --light_count;
                    }
                }
                delete visual;
                --index;
                --count;
            }
        }
        ++index;
    }
}

W8SpellEmitterHost::W8SpellEmitterHost() : value_0ac(0), value_0b0(0), flag_378(0)
{
    int emitter;

    for (emitter = 0; emitter < 28; ++emitter) {
        emitters[emitter] = 0;
        emitter_values[emitter] = 15.0f;
    }
}

/* Copy the two proven host values and terminal flag, clone every populated
   animation, and deep-copy all optional per-emitter light vectors. */
// FUNCTION: WIZ8 0x004aad20
W8SpellEmitterHost::W8SpellEmitterHost(const W8SpellEmitterHost& other)
    : W8EmitterHost(other), value_0ac(other.value_0ac), value_0b0(other.value_0b0),
      flag_378(other.flag_378)
{
    int emitter;

    for (emitter = 0; emitter < 28; ++emitter) {
        if (other.emitters[emitter] == 0) {
            emitters[emitter] = 0;
            emitter_values[emitter] = 15.0f;
        } else {
            emitters[emitter] = CloneAnimObj004A0320(other.emitters[emitter]);
            emitter_values[emitter] = other.emitter_values[emitter];
        }
    }

    active = 1;
    current_cycle = other.current_cycle;

    for (emitter = 0; emitter < 28; ++emitter) {
        int list_index;

        for (list_index = 0; list_index < other.light_lists[emitter].GetCount(); ++list_index) {
            W8GrowableVector<stLight*>* source_lights =
                *other.light_lists[emitter].GetAt(list_index);
            W8GrowableVector<stLight*>* copied_lights = 0;

            if (source_lights != 0) {
                int light_index;

                copied_lights = new W8GrowableVector<stLight*>;
                if (copied_lights == 0) {
                    srAssertFail("plsNewLights",
                                 "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x198,
                                 "Out of memory creating monster light list");
                }
                for (light_index = 0; light_index < source_lights->GetCount(); ++light_index) {
                    stLight* source_light = *source_lights->GetAt(light_index);
                    float x = source_light->positionalX();
                    float y = source_light->positionalY();
                    float z = source_light->positionalZ();
                    stLight* copied_light = new stLight;

                    if (copied_light != 0) {
                        *copied_light = *source_light;
                    }
                    if (copied_light == 0) {
                        srAssertFail("pstNewLight",
                                     "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x1a0,
                                     "Out of memory creating monster light");
                    }
                    copied_light->ConfigureMonsterCopy();
                    copied_light->setLocation(x, y, z);
                    copied_light->setParent(0, 0);
                    PLAdoptAppend(&g_world->m_lights_0a8, copied_light);
                    copied_lights->Add(copied_light);
                }
            }
            light_lists[emitter].Add(copied_lights);
        }
    }
}

// FUNCTION: WIZ8 0x004AB340
unsigned char W8SpellEmitterHost::ReadCycleData004AB340(W8ReadLevelInfo* info,
                                                        W8SpellVisual* visual, int,
                                                        int emitter_index)
{
    W8GrowableVector<stLight*>* lights = new W8GrowableVector<stLight*>;
    W8AnimObj* animation;
    unsigned char success;
    signed char emitter;

    if (info == 0 || info->hFile == 0 || visual == 0) {
        srAssertFail("pInfo && pInfo->hFile && pSpell",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x25a, 0);
    }
    animation = CreateAnimObj004A01A0();
    success = AnimObjReadFromFile004A05C0(info, animation, 1, lights, 1);
    emitter = static_cast<signed char>(animation->cycle);

    if (lights->GetCount() == 0) {
        delete lights;
        lights = 0;
    } else {
        visual->SetLights(lights);
    }
    light_lists[emitter_index].Add(lights);

    if (emitter_index != -1) {
        emitter = static_cast<signed char>(emitter_index);
        current_cycle = emitter;
    }
    emitter_values[emitter] = animation->playback_scale_08;
    active = 1;
    flag_06e = 1;
    timer_068 = g_shared_timer_base->getMsTime(srTimer::TIMER_READ_DEFAULT);
    flag_070 = animation->unknown_03;
    flag_06f = animation->value_02;
    flag_06d = animation->unknown_01;
    emitters[emitter] = animation;

    if (visual != 0) {
        if (SetCycleFrameLod(current_cycle, 0, 2) != 0) {
            m_bLOD = 2;
        } else if (SetCycleFrameLod(current_cycle, 0, 1) != 0) {
            m_bLOD = 1;
        } else {
            m_bLOD = 0;
        }
    }
    return success;
}

/* Load one named spell visual resource. When a shared visual of the same
   cycle group is already registered the answer is a clone of it; otherwise
   the .mls text resource is parsed: each of the seven cycle rows of the
   requested group loads its bitmaps, SOUND_FRAME/SOUND_CYCLE rows attach timed
   sound events, and SHAKE_FRAME rows attach camera-shake effects. */
// FUNCTION: WIZ8 0x004ab580
unsigned char LoadSpellVisualResource004AB580(const W8GrCycleLoadContext* context, const char* name,
                                              int cycle_type, W8SpellVisual** visual, int unused)
{
    W8SpellVisual* shared = static_cast<W8SpellVisual*>(FindFirstGrCycleByName(name));
    if (shared != 0 && shared->value_1d8 == cycle_type) {
        W8SpellVisual* spell = new W8SpellVisual(*shared);
        if (spell == 0) {
            srAssertFail("pSpell", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x406, 0);
        }
        *visual = spell;
        if (*visual == 0) {
            srAssertFail("*ppSpell", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x2f8, 0);
        }
        RegisterGrCycle(name, *visual);
        return 1;
    }

    PauseSharedGameTimers00439BC0();

    unsigned char more = 1;
    unsigned char success = 1;
    char path[100];
    sprintf(path, "data\\Spells\\%s.mls", name);
    int handle = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    *visual = 0;
    if (handle == 0) {
        success = 0;
    } else {
        char line[100];
        char pac_command[52];
        char pac_name[52];
        char pac_value[52];
        char loop_name[128];
        char wave_path[256];
        int frame;
        int index;
        int i;
        int sound_type;
        float intensity;
        float duration;
        float distance;
        W8SoundEvent* event;
        W8CameraShakeEffect* effect;

        while (more != 0 && success != 0) {
            ReadTextLine004CEE40(handle, line, sizeof(line), &more);
            sscanf(line, "%s %s", pac_name, pac_value);
            if (strlen(line) <= 2) {
                continue;
            }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
            if (pac_name == 0) {
                srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x532,
                             0);
            }
#pragma clang diagnostic pop

            index = -1;
            for (i = 0; i < 28; ++i) {
                if (_strnicmp(pac_name, g_spell_cycle_names[i], strlen(g_spell_cycle_names[i])) ==
                    0) {
                    index = i;
                    break;
                }
            }
            if (index != -1) {
                if (index / 7 == cycle_type) {
                    W8GrCycle* loaded = *visual;
                    if (LoadGrCycle004A67E0(context, pac_value, &loaded, index, 1, "Data\\Spells",
                                            2, 0) != 0) {
                        *visual = static_cast<W8SpellVisual*>(loaded);
                        RegisterGrCycle(pac_value, *visual);
                        success = 1;
                    } else {
                        success = 0;
                    }
                }
                continue;
            }

            sound_type = 0;
            if (_stricmp(pac_name, "SOUND_FRAME") == 0) {
                sound_type = 1;
            } else if (_stricmp(pac_name, "SOUND_CYCLE") == 0) {
                sound_type = 2;
            } else {
                if (_stricmp(pac_name, "SHAKE_FRAME") == 0) {
                    intensity = 1.0f;
                    duration = 1.0f;
                    distance = 10.0f;
                    sscanf(line, "%s %s %d %f %f %f", pac_command, pac_name, &frame, &intensity,
                           &duration, &distance);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
                    if (pac_name == 0) {
                        srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                                     0x532, 0);
                    }
#pragma clang diagnostic pop
                    index = -1;
                    for (i = 0; i < 28; ++i) {
                        if (_strnicmp(pac_name, g_spell_cycle_names[i],
                                      strlen(g_spell_cycle_names[i])) == 0) {
                            index = i;
                            break;
                        }
                    }
                    effect = new W8CameraShakeEffect(
                        duration, 1, intensity, static_cast<int>(distance * g_world_scale_005ebc40),
                        0);
                    if (effect != 0) {
                        effect->cycle_3c = index;
                        effect->frame_40 = frame;
                        effect->subcycle_44 = 0;
                        (*visual)->AddShakeEffect004A8530(effect);
                    }
                }
                continue;
            }

            memcpy(loop_name, &g_empty_ambient_name_65a110, 2);
            memset(loop_name + 2, 0, sizeof(loop_name) - 2);
            sscanf(line, "%s %s %d %s %s", pac_command, pac_name, &frame, pac_value, loop_name);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wtautological-pointer-compare"
            if (pac_name == 0) {
                srAssertFail("pacName", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x532,
                             0);
            }
#pragma clang diagnostic pop
            index = -1;
            for (i = 0; i < 28; ++i) {
                if (_strnicmp(pac_name, g_spell_cycle_names[i], strlen(g_spell_cycle_names[i])) ==
                    0) {
                    index = i;
                    break;
                }
            }
            if (*visual != 0 && (*visual)->IsCycleSupported((signed char)index)) {
                sprintf(wave_path, "Data\\Spells\\Sounds\\%s.WAV", pac_value);
                event = CreateSoundEvent(sound_type, index, frame, 0, wave_path,
                                         _stricmp(loop_name, "LOOP") == 0);
                if (event != 0) {
                    (*visual)->AddSoundEvent(event);
                }
            }
        }

        FileClose(handle);
        if (*visual == 0) {
            srAssertFail("*ppSpell", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x365,
                         FormatString("Spell %s missing cycle of type %d", name, cycle_type));
        }
        (*visual)->host->flag_378 = 0;
        (*visual)->host->behaviour_071 = 1;
    }

    ResumeSharedGameTimers00439CA0();
    return success;
}

// FUNCTION: WIZ8 0x004ABBB0
W8SpellVisual::W8SpellVisual()
    : value_1d8(-1), host(0), started(0), flag_1e5(0), flag_1e6(1), flag_1e7(0), value_1e8(1.0f),
      value_1ec(0)
{
    W8GrObject::unknown_004 = 1;
    unknown_008 = IncrementValue60DFAC();
    host = new W8SpellEmitterHost;
    if (host == 0) {
        srAssertFail("m_pRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x3c0, 0);
    }
}

/* Release the emitter host, leave the world's spell collection, and unregister
   the common GrCycle identity before the base classes tear down. */
// FUNCTION: WIZ8 0x004abd00
W8SpellVisual::~W8SpellVisual()
{
    SetLights(0);
    delete host;
    host = 0;

    int index = g_world->spell_visuals->IndexOf(this);
    if (index != -1) {
        g_world->spell_visuals->RemoveAt(index);
    }
    UnregisterGrCycle(this);
}

/* Delete every spell visual owned by a world. Its light list is first handed
   back through the world light boundary, exactly as the retail teardown does. */
// FUNCTION: WIZ8 0x004ac3d0
void DestroyAllSpellVisuals(W8World* world)
{
    while (world->spell_visuals->GetCount() != 0) {
        W8SpellVisual* spell = *world->spell_visuals->GetAt(0);

        if (spell == 0) {
            srAssertFail("pSpell", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x51c, 0);
        }
        g_world->spell_visuals->RemoveAt(g_world->spell_visuals->IndexOf(spell));
        if (spell->m_plsLights != 0) {
            int count = spell->m_plsLights->GetCount();

            while (count != 0) {
                stLight* light = spell->m_plsLights->RemoveAt(0);
                WorldRemoveLight(g_world, light);
                --count;
            }
        }
        delete spell;
    }
}

/* Start the visual, but only while the host is live. A spell of the seventh
   kind already running takes the slot instead of a fresh start. */
// FUNCTION: WIZ8 0x004abdc0
void W8SpellVisual::StartIfHostActive()
{
    if (this->host->active == 0) {
        return;
    }
    if (CountSpellsOfKind(7) != 0) {
        this->started = 1;
        return;
    }
    TickAnimation(1.0f);
}

/* Search backward from a subcycle for the first cycle this visual supports,
   seven cycles per group; -1 when none does. */
// FUNCTION: WIZ8 0x004ac530
int W8SpellVisual::FindSupportedCycle004AC530(signed char group, signed char subcycle)
{
    for (signed char index = subcycle; index >= 0; --index) {
        signed char cycle = (signed char)(group * 7 + index);

        if (IsCycleSupported(cycle)) {
            return cycle;
        }
    }
    return -1;
}

/* Create one spell visual from a named bitmap resource, falling back to the
   Generic visual when the named one is missing or has no supported cycle.
   The visual joins the world's list, is positioned, and carries the caller's
   two extra arguments. */
// FUNCTION: WIZ8 0x004ad430
W8SpellVisual* SpawnSpellEffect(const srVector3T<float>* position, const char* resource_name,
                                int argument_3, int argument_4, int argument_5)
{
    if (resource_name == 0 || resource_name[0] == '\0') {
        srAssertFail("pMLS && strlen(pMLS)", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                     0x8b4, 0);
    }
    W8GrCycleLoadContext context;
    context.world_00 = g_world;
    context.directory_08 = "Data\\Spells\\Bitmaps";
    W8SpellVisual* visual = 0;
    unsigned char loaded = 0;
    int cycle = -1;

    if (resource_name != 0) {
        loaded = LoadSpellVisualResource004AB580(&context, resource_name, 1, &visual, 1);
    }
    if (loaded) {
        visual->SetNavigationMode(4);
        visual->state_088 = 0;
        visual->SetPitchRollEnabled00453CA0(1, 1);
        g_world->spell_visuals->Add(visual);
        cycle = visual->FindSupportedCycle004AC530(1, argument_3 - 1);
        if (cycle == -1) {
            delete visual;
            loaded = 0;
        }
    }
    if (!loaded) {
        loaded = LoadSpellVisualResource004AB580(&context, "Generic", 1, &visual, 1);
        if (loaded) {
            visual->SetNavigationMode(4);
            visual->state_088 = 0;
            visual->SetPitchRollEnabled00453CA0(1, 1);
            g_world->spell_visuals->Add(visual);
            cycle = visual->FindSupportedCycle004AC530(1, argument_3 - 1);
        }
    }
    if (visual != 0) {
        visual->value_1d8 = 1;
        visual->host->pending_cycle = (signed char)cycle;
        visual->host->flag_378 = 1;
        visual->value_1f0 = argument_4;
        visual->value_1f4 = argument_5;
        visual->SetPositionInternal00453590(position);
    }
    return visual;
}

/* Spawn a kind-0 (camera-anchored) spell visual, falling back to the Generic
   resource when the named one is missing or has no supported cycle. The
   visual is placed at the camera and turned to face the camera's yaw/pitch. */
// FUNCTION: WIZ8 0x004ad080
W8SpellVisual* SpawnSpellEffect004AD080(const char* name, int animation, int value_1, int value_2)
{
    if (name == 0 || name[0] == '\0') {
        srAssertFail("pMLS && strlen(pMLS)", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                     0x879, 0);
    }
    W8GrCycleLoadContext context;
    context.world_00 = g_world;
    context.directory_08 = "Data\\Spells\\Bitmaps";
    W8SpellVisual* visual = 0;
    W8SpellVisual* generic;
    int cycle = -1;

    if (name != 0) {
        if (LoadSpellVisualResource004AB580(&context, name, 0, &visual, 1) != 0) {
            visual->SetNavigationMode(4);
            visual->state_088 = 0;
            visual->SetPitchRollEnabled00453CA0(1, 1);
            g_world->spell_visuals->Add(visual);
        }
    }
    if (visual != 0) {
        cycle = visual->FindSupportedCycle004AC530(0, animation - 1);
        if (cycle != -1) {
            goto placed;
        }
        delete visual;
    }
    generic = 0;
    if (LoadSpellVisualResource004AB580(&context, "Generic", 0, &generic, 1) != 0) {
        generic->SetNavigationMode(4);
        generic->state_088 = 0;
        generic->SetPitchRollEnabled00453CA0(1, 1);
        g_world->spell_visuals->Add(generic);
        visual = generic;
    } else {
        visual = 0;
    }
    cycle = visual->FindSupportedCycle004AC530(0, animation - 1);
placed:
    if (visual != 0) {
        srVector3T<float> position;
        srMatrix3T<float> rotation;
        float angle;
        float pitch;

        visual->value_1d8 = 0;
        visual->host->pending_cycle = (signed char)cycle;
        visual->value_1f0 = value_1;
        visual->value_1f4 = value_2;
        GetCameraPosition(&position);
        visual->SetPosition004A6DF0(&position);
        rotation.SetIdentity();
        angle = GetCameraYawRadians() - g_monster_rotation_offset_005ec04c;
        if ((double)angle != g_zero_005ebb40) {
            rotation.RotateAboutY(sin((double)angle), cos((double)angle));
        }
        pitch = -GetCameraPitchRadians();
        if ((double)pitch != g_zero_005ebb40) {
            rotation.RotateAboutX(sin((double)pitch), cos((double)pitch));
        }
        visual->host->SetRotation004B88D0(&rotation);
    }
    return visual;
}

/* Create a kind-3 spell visual attached to a monster — its position comes from
   the monster's spell socket (or mapped position) and its scale from the
   monster's animation bounds. Without a parent it anchors at the camera with
   the camera's rotation. Falls back to the Generic resource like above. */
// FUNCTION: WIZ8 0x004ad8a0
W8SpellVisual* CreateSpellEffect004AD8A0(const char* mls_name, int animation, W8Monster* parent,
                                         int value, int flags)
{
    if (mls_name == 0 || mls_name[0] == '\0') {
        srAssertFail("pMLS && strlen(pMLS)", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                     0x93f, 0);
    }
    W8GrCycleLoadContext context;
    context.world_00 = g_world;
    context.directory_08 = "Data\\Spells\\Bitmaps";
    W8SpellVisual* visual = 0;
    W8SpellVisual* generic;
    int cycle = -1;

    if (mls_name != 0) {
        if (LoadSpellVisualResource004AB580(&context, mls_name, 3, &visual, 1) != 0) {
            visual->SetNavigationMode(4);
            visual->state_088 = 0;
            visual->SetPitchRollEnabled00453CA0(1, 1);
            g_world->spell_visuals->Add(visual);
        }
    }
    if (visual != 0) {
        cycle = visual->FindSupportedCycle004AC530(3, animation - 1);
        if (cycle != -1) {
            goto placed;
        }
        delete visual;
    }
    generic = 0;
    if (LoadSpellVisualResource004AB580(&context, "Generic", 3, &generic, 1) != 0) {
        generic->SetNavigationMode(4);
        generic->state_088 = 0;
        generic->SetPitchRollEnabled00453CA0(1, 1);
        g_world->spell_visuals->Add(generic);
        visual = generic;
    } else {
        visual = 0;
    }
    cycle = visual->FindSupportedCycle004AC530(3, animation - 1);
placed:
    if (visual != 0) {
        srVector3T<float> position;

        visual->value_1d8 = 3;
        visual->host->pending_cycle = (signed char)cycle;
        visual->value_1f0 = value;
        visual->value_1f4 = flags;
        if (parent != 0) {
            srVector3T<float> minimum;
            srVector3T<float> maximum;
            float height;
            float width;

            visual->value_1ec = parent->propagated_value_1e4;
            parent->GetAnimationBounds(&minimum, &maximum);
            height = maximum.y - minimum.y;
            width = maximum.x - minimum.x;
            if (height <= width) {
                height = width;
            }
            visual->value_1e8 = height * g_float_005ec128;
            if (parent->GetSpellPosition004C78E0(&position) == 0) {
                parent->GetMappedPosition004C72A0(&position);
            }
            visual->SetPosition004A6DF0(&position);
        } else {
            srMatrix3T<float> rotation;

            GetCameraPosition(&position);
            g_gd_camera_65a0f8->GetRotationMatrix(&rotation);
            visual->SetPosition004A6DF0(&position);
            visual->host->SetRotation004B88D0(&rotation);
        }
    }
    return visual;
}

/* The persistent TargetCone targeting visual: created the first time the mode
   is enabled, deleted when it is disabled, and kept in representation mode 3
   while it lives. */
// FUNCTION: WIZ8 0x004add30
void SetTargetConeEnabled004ADD30(char enabled)
{
    if (enabled != 0) {
        if (g_target_cone_visual_65be20 == 0) {
            g_target_cone_visual_65be20 = CreateSpellEffect004AD8A0("TargetCone", 1, 0, 0, 0);
            if (g_target_cone_visual_65be20 != 0) {
                g_target_cone_visual_65be20->host->behaviour_071 = 3;
            }
        }
        return;
    }
    if (g_target_cone_visual_65be20 != 0) {
        delete g_target_cone_visual_65be20;
        g_target_cone_visual_65be20 = 0;
    }
}

/* Send something to one named emitter. The arguments are handed on in the
   reverse of the order they arrive. */
// FUNCTION: WIZ8 0x004ab290
srModelInstance* W8SpellEmitterHost::SetCycleFrameLod(signed char emitter, signed char frame,
                                                      signed char lod)
{
    return AnimObjDispatch004A14D0(this->emitters[emitter], lod, frame);
}

/* Apply the host setting to one required emitter.  The source assertion names
   that local pointer `pao`; assertions do not replace the following call. */
// FUNCTION: WIZ8 0x004ab2c0
unsigned int W8SpellEmitterHost::ApplyEmitterSetting(char emitter)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
    W8AnimObj* target = this->emitters[emitter];
#pragma clang diagnostic pop

    if (target == 0) {
        srAssertFail("pao", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x200, 0);
    }
    return AnimObjValue004A15D0(target, this->m_bLOD);
}

/* One named emitter's AniMesh, looked up with the host's own setting; an
   empty slot yields none. */
// FUNCTION: WIZ8 0x004ab310
W8AniMesh* W8SpellEmitterHost::GetEmitterAniMesh(char emitter)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
    W8AnimObj* target = this->emitters[emitter];
#pragma clang diagnostic pop

    if (target == 0) {
        return 0;
    }
    return static_cast<W8AniMesh*>(AnimObjEntry004A1660(target, this->m_bLOD, 0));
}

/* The clone slot owns both the 0x37c allocation and the copy-construction
   call.  The constructor body remains the next lifecycle member to recover. */
// FUNCTION: WIZ8 0x004ade70
W8AnimRepBase005EC1D8* W8SpellEmitterHost::Clone()
{
    return new W8SpellEmitterHost(*this);
}

/* Release each owned emitter and every per-emitter vector of cloned lights.
   The member-array and base destructors then run in reverse construction
   order, matching the two vector/base cleanup phases in the image. */
// SYNTHETIC: WIZ8 0x004aad00
// W8SpellEmitterHost::`scalar deleting destructor'
// FUNCTION: WIZ8 0x004ab1c0
W8SpellEmitterHost::~W8SpellEmitterHost()
{
    int emitter;
    int light_list;

    for (emitter = 0; emitter < 28; ++emitter) {
        if (emitters[emitter] != 0) {
            DestroyAnimObj004A01E0(emitters[emitter]);
            emitters[emitter] = 0;
        }
    }
    for (emitter = 0; emitter < 28; ++emitter) {
        for (light_list = 0; light_list < light_lists[emitter].GetCount(); ++light_list) {
            DestroyLightVector(*light_lists[emitter].GetAt(light_list));
        }
        light_lists[emitter].Clear();
    }
}

/* Whether one spell id is among the six the caller singles out. */
// FUNCTION: WIZ8 0x004aca00
bool IsSpellInSingledOutSet(int spell_id)
{
    switch (spell_id) {
    case 0x30:
    case 0x31:
    case 0x4c:
    case 0x50:
    case 0x51:
    case 0x5d:
        return true;
    default:
        return false;
    }
}

/* Release the spell database and forget the version with it, so the two are
   never out of step. */
// FUNCTION: WIZ8 0x004acd50
void ReleaseSpellDatabase(void)
{
    if (g_spell_records != 0) {
        delete[] g_spell_records;
        g_spell_records = 0;
        g_spell_database_version = 0;
    }
}

/* The forty icon resource names under Data\Icons\MonsterSpells. Entries 8 and
   9 share the pooled "Hexed" literal in retail. */
// GLOBAL: WIZ8 0x0060d50c
const char* g_monster_spell_icon_names_0060d50c[40] = {
    "Hexed",           "Diseased",     "Irritated",      "Nauseated",      "Slowed",
    "Afraid",          "Poisoned",     "Silenced",       "Hexed",          "Hexed",
    "Insane",          "Blind",        "Turncoat",       "Webbed",         "Asleep",
    "Paralyzed",       "Unconscious",  "Dracon_Breath",  "Guardian_Angel", "Razor_Cloak",
    "Eye4Eye",         "Haste",        "Super_Man",      "Body_Stone",     "Armor_Plate",
    "Enchanted_Blade", "Magic_Screen", "Missile_Shield", "Armor_Melt",     "Acid_Cloud",
    "Toxic_Cloud",     "Fire_Storm",   "Death_Cloud",    "Draining_Cloud", "Bless",
    "Element_Shield",  "Soul_Shield",  "Ring_Of_Fire",   "Charmed",        "Summoned",
};

/* Attach or remove a spell/condition icon on a monster's representation. The
   add path builds the billboard's texture path, creates the icon item and
   adopts the link record; the remove path finds the record by icon id, pulls
   the item out of the world list, deletes it and frees the record. */
// FUNCTION: WIZ8 0x004acd80
void DropMonsterVisual(W8Monster* pMonster, int iIcon, char add)
{
    W8MonsterRep* pMonRep;
    W8MonsterLinkedItem005E8* pSpellMI;
    W8PList* ppl;
    char path[260];
    int count;
    int index;

    if (pMonster == 0) {
        srAssertFail("pMonster", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x829, 0);
    }
    pMonRep = pMonster->m_pRep;
    if (pMonRep == 0) {
        srAssertFail("pMonRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x82b, 0);
    }
    if (pMonRep->GetSpellIcons() == 0) {
        srAssertFail("pMonRep->GetSpellIcons()",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x82c, 0);
    }
    ppl = pMonRep->GetSpellIcons();
    if (iIcon == -1) {
        srAssertFail("iIcon != SPELL_ICON_NONE",
                     "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x82f, 0);
    }
    if (add == 0) {
        if (ppl == 0) {
            return;
        }
        count = PLLength(ppl);
        for (index = 0; index < count; ++index) {
            pSpellMI = static_cast<W8MonsterLinkedItem005E8*>(PLGet(ppl, index));
            if (pSpellMI->icon_00 == iIcon) {
                PListRemove(ppl, pSpellMI);
                if (pSpellMI == 0) {
                    srAssertFail("pSpellMI", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                                 0x81b, 0);
                }
                if (pSpellMI->psrBMO != 0) {
                    pSpellMI->psrBMO->DetachMesh0049FA30(g_world);
                    PListRemove(g_world->plsItems, pSpellMI->psrBMO);
                    if (pSpellMI->psrBMO != 0) {
                        delete pSpellMI->psrBMO;
                    }
                }
                free(pSpellMI);
                index = 0;
                --count;
            }
        }
    } else {
        if (iIcon >= 0x28) {
            srAssertFail("uiIcon < SPELL_NUM_ICONS",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x80a, 0);
        }
        pSpellMI = static_cast<W8MonsterLinkedItem005E8*>(malloc(8));
        pSpellMI->icon_00 = 0;
        pSpellMI->psrBMO = 0;
        sprintf(path, "%s\\%s_A.TGA", "Data\\Icons\\MonsterSpells",
                g_monster_spell_icon_names_0060d50c[iIcon]);
        pSpellMI->icon_00 = iIcon;
        pSpellMI->psrBMO = CreateMonsterIconItem004C5500(g_world, path, 1);
        if (pSpellMI->psrBMO == 0) {
            srAssertFail("pSpellMI->psrBMO", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp",
                         0x814, 0);
        }
        if (pSpellMI == 0) {
            srAssertFail("pSpellMI", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x84a, 0);
        }
        PLAdoptAppend(ppl, pSpellMI);
    }
}

// SYNTHETIC: WIZ8 0x004abce0
// W8SpellVisual::`scalar deleting destructor'

// VTABLE: WIZ8 0x005ecf40 W8SpellVisual
// VTABLE: WIZ8 0x005ecf2c W8Navigator
// class W8SpellVisual

// VTABLE: WIZ8 0x005ecf18 W8SpellEmitterHost
// class W8SpellEmitterHost

// VTABLE: WIZ8 0x005ecfb0
// class stSound3D

// VTABLE: WIZ8 0x005ecfe4
// class srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AEA70
// stSound3D::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x004AF3D0
// srClassSupport<stSound3D,srNode,0,65547>::getClassID

// TEMPLATE: WIZ8 0x004AF3E0
// srClassSupport<stSound3D,srNode,0,65547>::getClassName

// TEMPLATE: WIZ8 0x004AF3F0
// srClassSupport<stSound3D,srNode,0,65547>::getClassNode

// TEMPLATE: WIZ8 0x004AF460
// srClassSupport<stSound3D,srNode,0,65547>::clone

// TEMPLATE: WIZ8 0x004AF5A0
// srClassSupport<stSound3D,srNode,0,65547>::~srClassSupport<stSound3D,srNode,0,65547>

// SYNTHETIC: WIZ8 0x004AF660
// srClassSupport<stSound3D,srNode,0,65547>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x004AE6D0
stSound3D::stSound3D(const char* name, srNode* parent)
    : srClassSupport<stSound3D, srNode, 0, 0x1000b>(static_cast<srNode*>(0)), unknown_138(0),
      sound_handle(-1), volume(0x7f), falloff(25000.0f), wave_name(0), auto_release(0)
{
    if (parent != 0) {
        setParent(parent, 0);
    }
    if (name != 0) {
        wave_name = static_cast<char*>(malloc(strlen(name) + 1));
        strcpy(wave_name, name);
    }
    g_sound3d_instances_65be40.Add(this);
}

// FUNCTION: WIZ8 0x004AEAA0
stSound3D::~stSound3D()
{
    if (wave_name != 0) {
        free(wave_name);
    }
    if (sound_handle != -1) {
        SoundStop(sound_handle);
    }
    int index = g_sound3d_instances_65be40.IndexOf(this);
    if (index != -1) {
        g_sound3d_instances_65be40.RemoveAt(index);
    }
}

// FUNCTION: WIZ8 0x004AE8B0
srClass* stSound3D::vInstance()
{
    return new stSound3D(0, 0);
}

// FUNCTION: WIZ8 0x004AEBF0
unsigned char stSound3D::Play(unsigned char loop, unsigned char release_when_done)
{
    SOUND3DPARMS options;
    srVector3T<float> listener;

    if (wave_name == 0) {
        return 0;
    }
    GetCameraPosition(&listener);
    BuildSoundOptions(&listener, &options);
    if (loop != 0) {
        options.uiLoop = 0;
    }
    sound_handle = Sound3DPlay(wave_name, &options);
    auto_release = release_when_done;
    return sound_handle != -1;
}

// FUNCTION: WIZ8 0x004AECC0
void stSound3D::BuildSoundOptions(const srVector3T<float>* listener, SOUND3DPARMS* options)
{
    float angle = -GetCameraYawRadians();
    unsigned int scaled_volume = (volume * g_settings_6850c8.sound_effects_volume) / 0x7f;
    srMatrix3T<float> rotation;
    srVector3T<float> node_position;
    srVector3T<float> offset;

    rotation.SetIdentity();
    if ((double)angle != g_zero_005ebb40) {
        rotation.RotateAboutY(sin(angle), cos(angle));
    }

    getLocation(node_position);
    offset = node_position - *listener;
    srVector3T<float> transformed = rotation.Transform(offset);
    float x = transformed.x;
    float y = transformed.y;
    float z = transformed.z;

    memset(options, 0xff, sizeof(*options));
    srVector3T<float> listener_offset(listener->x - node_position.x, listener->y - node_position.y,
                                      listener->z - node_position.z);
    options->uiVolume = static_cast<unsigned int>(
        (g_float_005ebb38 - listener_offset.Length() / falloff) * scaled_volume);
    options->uiLoop = 1;
    options->Pos.flX = x;
    options->Pos.flY = y;
    options->Pos.flZ = z;
    options->Pos.flVelX = 0.0f;
    options->Pos.flVelY = 0.0f;
    options->Pos.flVelZ = 0.0f;
    options->Pos.flFaceX = -x;
    options->Pos.flFaceY = -y;
    options->Pos.flFaceZ = -z;
    options->Pos.flUpX = 0.0f;
    options->Pos.flUpY = g_float_005ebb38;
    options->Pos.flUpZ = 0.0f;
    options->Pos.flFalloffMin = falloff;
    options->Pos.flFalloffMax = falloff;
    options->Pos.uiVolume = options->uiVolume;
}

// FUNCTION: WIZ8 0x004AEC70
bool stSound3D::IsPlaying()
{
    if (sound_handle != -1 && SoundIsPlaying(sound_handle) != 0) {
        return true;
    }
    return false;
}

/* Per-frame 3D sound servicing for the instanced sound list: each live entry
   that stopped playing is dropped (releasing it removes it from the vector, so
   the walk count and index step back together); each playing one is re-aimed
   relative to the camera and re-volumed by distance and the configured
   effects-volume. */
// FUNCTION: WIZ8 0x004aefd0
void Update3DSounds()
{
    srVector3T<float> listener;

    GetCameraPosition(&listener);
    int index = 0;
    int count = g_sound3d_instances_65be40.GetCount();
    if (count > 0) {
        do {
            stSound3D* sound = *g_sound3d_instances_65be40.GetAt(index);
            if (sound->sound_handle != -1) {
                srVector3T<double> world = sound->getWorldSpaceLocation();
                if (SoundIsPlaying(sound->sound_handle) == 0) {
                    unsigned char dead = sound->auto_release;
                    sound->sound_handle = -1;
                    if (dead != 0) {
                        sound->release();
                        --count;
                        --index;
                    }
                } else {
                    srVector3T<float> to_listener;
                    srVector3T<float> offset;
                    srVector3T<float> transformed;
                    srMatrix3T<float> rotation;
                    float distance;
                    float angle;
                    unsigned int volume;

                    to_listener.Set(listener.x - (float)world.x, listener.y - (float)world.y,
                                    listener.z - (float)world.z);
                    distance = (float)sqrt(DotProduct(to_listener, to_listener));
                    if (sound->falloff <= distance) {
                        SoundSetVolume(sound->sound_handle, 0);
                    } else {
                        angle = -GetCameraYawRadians();
                        rotation.SetIdentity();
                        if ((double)angle != g_zero_005ebb40) {
                            rotation.RotateAboutY(sin((double)angle), cos((double)angle));
                        }
                        offset.x = (float)world.x - listener.x;
                        offset.y = (float)world.y - listener.y;
                        offset.z = (float)world.z - listener.z;
                        transformed = rotation.Transform(offset);
                        Sound3DSetPosition(sound->sound_handle, transformed.x, transformed.y,
                                           transformed.z);
                        Sound3DSetDirection(sound->sound_handle, -transformed.x, -transformed.y,
                                            -transformed.z, 0.0f, g_float_005ebb38, 0.0f);
                        volume = (sound->volume * g_settings_6850c8.sound_effects_volume) / 0x7f;
                        SoundSetVolume(
                            sound->sound_handle,
                            static_cast<UINT32>((g_float_005ebb38 - distance / sound->falloff) *
                                                volume));
                    }
                }
            }
            ++index;
        } while (index < count);
    }
}

/* Detach every item visualisation the monster's cycle owns before the cycle
   itself goes away. Each linked entry carries the item at +4; the item is
   detached from the world, unlinked from the world's item list, and deleted,
   and the entry storage is freed. */
// FUNCTION: WIZ8 0x004ACF90
void PrepareMonsterCycleForDestruction004ACF90(W8Monster* monster)
{
    if (monster == 0) {
        srAssertFail("pMonster", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x852, 0);
    }
    W8MonsterRep* rep = monster->m_pRep;
    if (rep == 0) {
        srAssertFail("pMonRep", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x854, 0);
    }
    W8PList* list = rep->linked_objects_5e8;
    if (list != 0) {
        unsigned int count = PLLength(list);
        for (int index = 0; index < (int)count; ++index) {
            W8MonsterLinkedItem005E8* entry = (W8MonsterLinkedItem005E8*)PLGet(list, index);
            if (entry == 0) {
                srAssertFail("pSpellMI", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x81b,
                             0);
            }
            if (entry->psrBMO != 0) {
                entry->psrBMO->DetachMesh0049FA30(g_world);
                PListRemove(g_world->plsItems, entry->psrBMO);
                delete entry->psrBMO;
            }
            free(entry);
        }
        PListClear(list);
    }
}

// FUNCTION: WIZ8 0x004aca60
bool CanSpellBackfire(int spell_id)
{
    switch (g_spell_records[spell_id].target_type) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        switch (spell_id) {
        case 77:
        case 118:
        case 131:
            break;
        default:
            return true;
        }
        break;
    case 0:
    case 1:
    case 2:
        switch (spell_id) {
        case 3:
        case 13:
        case 16:
        case 34:
        case 44:
        case 56:
        case 58:
        case 74:
            return true;
        }
        break;
    case 10:
        return spell_id == 39;
    }
    return false;
}

// FUNCTION: WIZ8 0x004acc10
unsigned char InitializeSpellDatabase(void)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsometimes-uninitialized"
    /* The decompiled body reads this storage only after the same short-circuit
   chain that clang's flow analysis cannot see through; retail leaves it
   uninitialised on the failed-read path. Suppress only this diagnostic. */
    int handle;
    unsigned int index;
    int offset = 0;
    unsigned char ok;
    int allocation_count;
    unsigned int row_count;

    if (g_spell_records != 0) {
        delete[] g_spell_records;
        g_spell_records = 0;
        g_spell_database_version = 0;
    }
    handle = FileOpen("Data\\Databases\\SpellTables.dbs", 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    ok = 0;
    if (FileRead(handle, &allocation_count, 4, 0) && FileRead(handle, &row_count, 4, 0)) {
        ok = 1;
    }
    g_spell_records = new W8SpellRuntimeRecord[allocation_count];
    if (g_spell_records == 0) {
        srAssertFail("s_pSpellTable", "C:\\Projects\\Wizardry 8\\Engine Code\\Spells.cpp", 0x774,
                     0);
    }
    for (index = 0; index < row_count; ++index) {
        if (ok == 0) {
            goto discard;
        }
        ok = 0;
        if (FileSeek(handle, 0x101, FILE_SEEK_FROM_CURRENT) &&
            FileRead(handle, reinterpret_cast<unsigned char*>(g_spell_records) + offset,
                     sizeof(W8SpellRuntimeRecord), 0)) {
            ok = 1;
        }
        offset += sizeof(W8SpellRuntimeRecord);
    }
    if (ok == 0) {
    discard:
        delete[] g_spell_records;
        g_spell_records = 0;
    }
    FileClose(handle);
    g_spell_database_version = row_count;
    return ok;
#pragma clang diagnostic pop
}
