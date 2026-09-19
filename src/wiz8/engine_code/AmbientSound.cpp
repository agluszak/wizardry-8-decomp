#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/float_constants.h"
#include "wiz8/geometry.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/sound_man.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "random.h"
#include "soundman.h"
#include "wiz8/engine_code/Spells.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/PolyPick.h"

// GLOBAL: WIZ8 0x005ec5a8
float g_float_005ec5a8 = 0.6000000238418579f;

// FUNCTION: WIZ8 0x00479030
void StopAllAmbientSounds()
{
    SoundStopAllRandom();
    SoundStopGroup(-16);
}

// FUNCTION: WIZ8 0x00479040
W8AmbientSound::W8AmbientSound()
    : volume_min(0), volume_max(0), current_volume(0), speed_min(0), speed_max(0), time_min(0),
      time_max(0), radius(0), in_range(0), looping(0), sound_handle(-1), sample_handle(-1),
      shared(0), bounded(0), region_angle(0)
{
    config_004.wave_name[0] = 0;
    position.SetZero();
    pacSoundName = 0;
    stopped = 0;
}

/* Region test used by UpdatePosition and the group handoff: translate the
   listener into region-center space, un-apply the per-axis scale, rotate back
   around the configured axis, then test against the min/max bounds. */
// FUNCTION: WIZ8 0x004790d0
unsigned char W8AmbientSound::IsInsideRegion(const srVector3T<float>* listener)
{
    srVector3T<float> relative;
    float scale_x;
    float scale_y;
    float scale_z;

    relative = *listener - region_center;
    scale_x = g_float_005ebb38 / region_scale.x;
    scale_y = g_float_005ebb38 / region_scale.y;
    scale_z = g_float_005ebb38 / region_scale.z;
    if (scale_x != g_float_005ebb38 || scale_y != g_float_005ebb38 || scale_z != g_float_005ebb38) {
        relative.x = scale_x * relative.x;
        relative.y = relative.y * scale_y;
        relative.z = relative.z * scale_z;
    }
    if (region_angle != g_float_005ebb34 &&
        (region_axis.x != g_float_005ebb34 || region_axis.y != g_float_005ebb34 ||
         region_axis.z != g_float_005ebb34)) {
        srMatrix3T<float> rotation;
        double angle = -region_angle;

        rotation.SetIdentity();
        if (angle != g_zero_005ebb40) {
            rotation.RotateAroundAxis(sin(angle), cos(angle), region_axis);
        }
        relative.Transform(rotation);
    }
    return PointInsideBounds004BE870(&relative, &region_min, &region_max);
}

/* Listener-relative servicing. Out of range: shared sounds try to hand their
   live voice to a matching group member that is in range, otherwise fade to
   zero; positional sounds stop immediately. In range: mark the entry, restart
   one-shot scheduling, re-aim a live 3D voice around the camera, and nudge its
   volume up inside the inner 40% of the radius. Shared sounds retarget the
   fade at the bus-scaled maximum. */
// FUNCTION: WIZ8 0x00479350
void W8AmbientSound::UpdatePosition(const srVector3T<float>* listener)
{
    unsigned char handed_off = 0;

    if (stopped != 0) {
        return;
    }
    {
        srVector3T<float> to_listener = *listener - position;
        float distance = to_listener.Length();
        unsigned char inside = IsInsideRegion(listener);

        if (radius <= distance || (bounded != 0 && inside == 0)) {
            if (in_range != 0) {
                in_range = 0;
                if (shared != 0) {
                    W8AmbientSound* match = FindNextMatching0047A260(config_004.wave_name, 0);
                    while (match != 0) {
                        if (handed_off != 0) {
                            return;
                        }
                        {
                            srVector3T<float> to_match = *listener - match->position;
                            unsigned char match_inside = match->IsInsideRegion(listener);
                            if (to_match.Length() < match->radius &&
                                (match->bounded == 0 || match_inside != 0) &&
                                match->sound_handle == -1) {
                                match->in_range = 1;
                                match->sound_handle = sound_handle;
                                match->target_volume =
                                    (match->volume_max * g_settings_6850c8.sound_effects_volume) /
                                    0x7f;
                                match->current_volume = SoundGetVolume(sound_handle);
                                match->fade_timer.SetDuration(
                                    g_float_005ec3b8 / static_cast<float>(match->target_volume));
                                match->fade_timer.Restart();
                                match->fade_timer.m_flags &= ~8;
                                match->fade_timer.m_start =
                                    match->fade_timer.Method00439A60() - match->fade_timer.m_start;
                                match->fade_timer.SetDuration(-1.0f);
                                sound_handle = -1;
                                handed_off = 1;
                            }
                        }
                        match = FindNextMatching0047A260(config_004.wave_name, match);
                    }
                    if (handed_off != 0) {
                        return;
                    }
                    target_volume = 0;
                    {
                        unsigned int full_volume =
                            (volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
                        fade_timer.SetDuration(g_float_005ec3b8 / static_cast<float>(full_volume));
                        fade_timer.Restart();
                        fade_timer.m_flags &= ~8;
                        fade_timer.m_start = fade_timer.Method00439A60() - fade_timer.m_start;
                        fade_timer.SetDuration(-1.0f);
                    }
                    return;
                }
                if (sound_handle != -1) {
                    SoundStop(sound_handle);
                    sound_handle = -1;
                }
            }
            return;
        }
        if (in_range == 0) {
            in_range = 1;
            Service(1);
        }
        if (shared == 0) {
            if (sound_handle != -1) {
                float angle = -GetCameraYawRadians();
                srMatrix3T<float> rotation;
                srVector3T<float> transformed;
                unsigned int volume;

                rotation.SetIdentity();
                if (static_cast<double>(angle) != g_zero_005ebb40) {
                    rotation.RotateAboutY(sin(static_cast<double>(angle)),
                                          cos(static_cast<double>(angle)));
                }
                srVector3T<float> offset = position - *listener;
                transformed = rotation.Transform(offset);
                Sound3DSetPosition(sound_handle, transformed.x, transformed.y, transformed.z);
                Sound3DSetDirection(sound_handle, -transformed.x, -transformed.y, -transformed.z,
                                    0.0f, g_float_005ebb38, 0.0f);
                if (radius * g_navigator_mode3_scale_005ebca4 <= distance) {
                    volume = current_volume;
                } else {
                    volume = static_cast<unsigned int>(
                        (g_float_005ebb38 - (distance - radius * g_navigator_mode3_scale_005ebca4) /
                                                (radius * g_float_005ec5a8)) *
                        current_volume);
                }
                SoundSetVolume(sound_handle, volume);
            }
            return;
        }
        {
            unsigned int full_volume = (volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
            if (target_volume != full_volume) {
                target_volume = full_volume;
                fade_timer.SetDuration(g_float_005ec3b8 / static_cast<float>(full_volume));
                fade_timer.Restart();
                fade_timer.m_flags &= ~8;
                fade_timer.m_start = fade_timer.Method00439A60() - fade_timer.m_start;
                fade_timer.SetDuration(-1.0f);
            }
        }
    }
}

/* Per-frame service while in range. Non-looping sounds are registered once as
   an SGP random sample; each call asks the scheduler (or a 2/3 roll on range
   entry) whether to fire a 3D one-shot at the emitter. Looping sounds start
   once: positional emitters go through Sound3DPlay, shared group sounds go
   through SoundPlay at zero volume so UpdateFade can ramp them up. */
// FUNCTION: WIZ8 0x00479970
void W8AmbientSound::Service(unsigned char entered)
{
    if (stopped != 0 || in_range == 0) {
        return;
    }
    if (looping == 0) {
        if (sample_handle == -1) {
            RANDOMPARMS parms;

            memset(&parms, -1, sizeof(parms));
            parms.uiMaxInstances = 1;
            parms.uiPriority = -16;
            parms.uiVolMin = (volume_min * g_settings_6850c8.sound_effects_volume) / 0x7f;
            parms.uiVolMax = (volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
            parms.uiPanMin = 0x40;
            parms.uiPanMax = 0x40;
            parms.uiTimeMin = time_min;
            parms.uiTimeMax = time_max;
            parms.uiSpeedMin = speed_min;
            parms.uiSpeedMax = speed_max;
            sample_handle = SoundPlayRandom(config_004.wave_name, &parms);
            if (sample_handle != -1) {
                SoundSetSampleFlags(sample_handle, 8);
            }
        }
        if (sample_handle != -1) {
            unsigned char should_play;

            if (entered == 0) {
                should_play = SoundRandomShouldPlay(sample_handle);
            } else {
                should_play = Random(3);
            }
            if (gXStatus.fCombatMode == 0 && should_play != 0 &&
                g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                float angle = -GetCameraYawRadians();
                srVector3T<float> camera;
                srMatrix3T<float> rotation;
                srVector3T<float> offset;
                srVector3T<float> transformed;
                srVector3T<float> to_listener;
                float distance;
                unsigned int volume;
                SOUND3DPOS pos;

                GetCameraPosition(&camera);
                rotation.SetIdentity();
                if (static_cast<double>(angle) != g_zero_005ebb40) {
                    rotation.RotateAboutY(sin(static_cast<double>(angle)),
                                          cos(static_cast<double>(angle)));
                }
                offset = position - camera;
                transformed = rotation.Transform(offset);
                memset(&pos, 0, sizeof(pos));
                pos.flX = transformed.x;
                pos.flY = transformed.y;
                pos.flZ = transformed.z;
                pos.flVelX = 0.0f;
                pos.flVelY = 0.0f;
                pos.flVelZ = 0.0f;
                pos.flFaceX = -transformed.x;
                pos.flFaceY = -transformed.y;
                pos.flFaceZ = -transformed.z;
                pos.flUpX = 0.0f;
                pos.flUpY = g_float_005ebb38;
                pos.flUpZ = 0.0f;
                pos.flFalloffMin = radius;
                pos.flFalloffMax = radius;
                to_listener = camera - position;
                distance = to_listener.Length();
                volume = static_cast<unsigned int>(((Random(volume_max - volume_min) + volume_min) *
                                                    g_settings_6850c8.sound_effects_volume) /
                                                   0x7f);
                current_volume = volume;
                if (distance < radius * g_navigator_mode3_scale_005ebca4) {
                    pos.uiVolume = current_volume;
                } else {
                    pos.uiVolume = static_cast<unsigned int>(
                        (g_float_005ebb38 - (distance - radius * g_navigator_mode3_scale_005ebca4) /
                                                (radius * g_float_005ec5a8)) *
                        current_volume);
                }
                sound_handle = Sound3DStartRandom(sample_handle, &pos);
            }
        }
        return;
    }
    if (sound_handle != -1) {
        return;
    }
    if (shared == 0) {
        float angle = -GetCameraYawRadians();
        srVector3T<float> camera;
        srMatrix3T<float> rotation;
        srVector3T<float> offset;
        srVector3T<float> transformed;
        SOUND3DPARMS parms;

        GetCameraPosition(&camera);
        current_volume = (volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
        rotation.SetIdentity();
        if (static_cast<double>(angle) != g_zero_005ebb40) {
            rotation.RotateAboutY(sin(static_cast<double>(angle)), cos(static_cast<double>(angle)));
        }
        offset = position - camera;
        transformed = rotation.Transform(offset);
        memset(&parms, -1, sizeof(parms));
        parms.uiVolume = current_volume;
        parms.Pos.flFaceX = -transformed.x;
        parms.Pos.flX = transformed.x;
        parms.Pos.flFaceY = -transformed.y;
        parms.Pos.flY = transformed.y;
        parms.Pos.flFalloffMin = radius;
        parms.Pos.flZ = transformed.z;
        parms.Pos.flFaceZ = -transformed.z;
        parms.uiLoop = 0;
        parms.Pos.flVelX = 0.0f;
        parms.Pos.flVelY = 0.0f;
        parms.Pos.flVelZ = 0.0f;
        parms.Pos.flUpX = 0.0f;
        parms.Pos.flUpY = g_float_005ebb38;
        parms.Pos.flUpZ = 0.0f;
        parms.Pos.flFalloffMax = parms.Pos.flFalloffMin;
        parms.Pos.uiVolume = parms.uiVolume;
        sound_handle = Sound3DPlay(config_004.wave_name, &parms);
        if (sound_handle == -1) {
            in_range = 0;
        }
        return;
    }
    if (SoundFileIsPlaying(config_004.wave_name) != 0) {
        return;
    }
    {
        SOUNDPARMS parms;

        memset(&parms, -1, sizeof(parms));
        parms.uiVolume = 0;
        parms.uiLoop = 0;
        current_volume = 0;
        target_volume = (volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
        sound_handle = SoundPlay(config_004.wave_name, &parms);
        fade_timer.SetDuration(g_float_005ec3b8 / static_cast<float>(target_volume));
        fade_timer.Restart();
        fade_timer.m_flags &= ~8;
        fade_timer.m_start = fade_timer.Method00439A60() - fade_timer.m_start;
        fade_timer.SetDuration(-1.0f);
    }
}

/* The contiguous class lifecycle and shared world-list accesses identify these
   pre-assertion bodies as the same AmbientSound.cpp unit. */
// FUNCTION: WIZ8 0x0047a260
W8AmbientSound* W8AmbientSound::FindNextMatching0047A260(const char* match_name,
                                                         W8AmbientSound* previous)
{
    int count;
    int index;

    if (g_world == 0) {
        return 0;
    }
    count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    if (previous == 0) {
        index = 0;
    } else {
        index = PListIndexOf(g_world->plsAmbientSounds, previous) + 1;
        if (index < 0 || index > count) {
            return 0;
        }
    }
    if (index >= count) {
        return 0;
    }
    do {
        W8AmbientSound* candidate =
            static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
        if (candidate != 0 && candidate != this && candidate->shared != 0 &&
            _stricmp(candidate->config_004.wave_name, match_name) == 0) {
            return candidate;
        }
        ++index;
    } while (index < count);
    return 0;
}

// FUNCTION: WIZ8 0x0047a310
void W8AmbientSound::UpdateFade()
{
    if (shared != 0) {
        if (sound_handle != -1) {
            W8GameTimer* timer = &fade_timer;

            if (timer->GetProgress() >= 1.0f) {
                if (target_volume > current_volume) {
                    ++current_volume;
                } else if (target_volume < current_volume) {
                    --current_volume;
                } else if ((timer->m_flags & 8) == 0) {
                    timer->m_flags |= 8;
                    timer->m_start = timer->Method00439A60() - timer->m_start;
                }
                SoundSetVolume(sound_handle, current_volume);
            }
        }
        if (in_range == 0 && target_volume == current_volume && sound_handle != -1) {
            SoundStop(sound_handle);
            sound_handle = -1;
        }
    }
}

// FUNCTION: WIZ8 0x0047a3e0
void UpdateAmbientSounds0047A3E0(W8World* world)
{
    if (world != 0) {
        int count;
        int index;

        SoundServiceRandom();
        SoundServiceStreams();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                sound->Service(0);
                sound->UpdateFade();
            }
        }
        Update3DSounds();
    }
}

// GLOBAL: WIZ8 0x0065a108
unsigned char g_default_footstep_surface_65a108;

// GLOBAL: WIZ8 0x0065a109
unsigned char g_default_footstep_material_65a109;

// GLOBAL: WIZ8 0x0065a10a
unsigned char g_footstep_alternate_65a10a;

// GLOBAL: WIZ8 0x0065a10c
int g_previous_footstep_variant_65a10c;

// GLOBAL: WIZ8 0x00609edc
const char* g_footstep_names_609edc[] = {
    "None",        "Gritty",     "Grass",     "Stone",       "ShallowWater",   "CreakyWood",
    "SolidWood",   "HollowWood", "Metal",     "Gravel",      "RoughStone",     "Marble",
    "Mud",         "Sand",       "Leaves",    "Snow",        "Carpet",         "Magic",
    "ClimbLadder", "ClimbRock",  "ClimbRope", "SwimSurface", "SwimUnderwater", "Crawl",
    "Fly",         "",
};
// GLOBAL: WIZ8 0x00609eb8
const char* g_footstep_surfaces_609eb8[] = {
    "None",       "SmallCave", "MediumCave",   "LargeCave",      "SmallRoom",
    "MediumRoom", "LargeRoom", "OutdoorsFlat", "OutdoorsCanyon",
};
// GLOBAL: WIZ8 0x00609f44
const char* g_footstep_fixed_name_609f44 = "Jump";
// GLOBAL: WIZ8 0x00609f48
const char* g_footstep_scuff_name_609f48 = "Scuff";

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wchar-subscripts"
/* Surface and material ids are recovered as char and range-checked against
   1..9 / 1..25 before indexing the name tables. */
// FUNCTION: WIZ8 0x0047a440
int PlayFootstep0047A440(char surface, char material, int kind)
{
    char selected_surface;
    char selected_material;
    char path[260];
    SOUNDPARMS options;
    int attempts = 0;
    int index;

    if (GetRenderOptionState(0xf) == 0) {
        return -1;
    }
    selected_surface = surface;
    if (selected_surface < W8_FOOTSTEP_SURFACE_SMALL_CAVE ||
        selected_surface > W8_FOOTSTEP_SURFACE_MAX) {
        selected_surface = g_default_footstep_surface_65a108;
    }
    selected_material = material;
    if (selected_material < W8_FOOTSTEP_MATERIAL_GRITTY ||
        selected_material > W8_FOOTSTEP_MATERIAL_MAX) {
        selected_material = g_default_footstep_material_65a109;
    }
    if (selected_material >= W8_FOOTSTEP_MATERIAL_CLIMB_LADDER) {
        sprintf(path, "Data\\Sound\\Footsteps\\Step_%s.WAV",
                g_footstep_names_609edc[selected_material]);
    } else {
        int variant;
        do {
            variant = Random(4) + 1;
            ++attempts;
        } while (variant == g_previous_footstep_variant_65a10c && attempts < 100);
        BuildFootstepPath0047A540(path, selected_surface, selected_material, kind, variant);
        g_previous_footstep_variant_65a10c = variant;
    }
    memset(&options, -1, sizeof(options));
    options.uiVolume = g_settings_6850c8.footstep_volume;
    g_footstep_alternate_65a10a = g_footstep_alternate_65a10a == 0;
    return SoundPlay(path, &options);
}

// FUNCTION: WIZ8 0x0047a540
void BuildFootstepPath0047A540(char* path, char surface, char material, char kind, int variant)
{
    if (kind == W8_FOOTSTEP_KIND_STEP) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%.2d.WAV",
                g_footstep_names_609edc[material], g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], variant);
        return;
    }
    if (kind == W8_FOOTSTEP_KIND_JUMP) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%s.WAV",
                g_footstep_names_609edc[material], g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], g_footstep_fixed_name_609f44);
        return;
    }
    if (kind == W8_FOOTSTEP_KIND_SCUFF) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%s_%.2d.WAV",
                g_footstep_names_609edc[material], g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], g_footstep_scuff_name_609f48, variant);
    }
}
#pragma clang diagnostic pop

// FUNCTION: WIZ8 0x0047a600
void RepositionAmbientSounds0047A600(W8World* world)
{
    if (world != 0) {
        int count;
        int index;

        SoundServiceRandom();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                srVector3T<float> position;
                GetCameraPosition(&position);
                sound->UpdatePosition(&position);
            }
        }
    }
}

// FUNCTION: WIZ8 0x0047a670
W8AmbientSound* CreateAmbientSound0047A670()
{
    W8AmbientSound* sound = new W8AmbientSound;

    if (sound == 0) {
        srAssertFail("pSound", "C:\\Projects\\Wizardry 8\\Engine Code\\AmbientSound.cpp", 0x2f5, 0);
    }
    sound->target_volume = 0;
    return sound;
}

// FUNCTION: WIZ8 0x0047a700
void DestroyAmbientSound0047A700(W8AmbientSound* ambient)
{
    if (ambient == 0) {
        srAssertFail("pAmbient", "C:\\Projects\\Wizardry 8\\Engine Code\\AmbientSound.cpp", 0x2fd,
                     0);
    }
    if (ambient->sound_handle != -1) {
        SoundStop(ambient->sound_handle);
    }
    if (ambient->sample_handle != -1) {
        SoundRemoveSampleFlags(ambient->sample_handle, 6);
    }
    if (ambient->pacSoundName != 0) {
        delete[] ambient->pacSoundName;
    }
    delete ambient;
}

// FUNCTION: WIZ8 0x0047a780
W8AmbientSound::~W8AmbientSound() {}

/* Build a complete ambient-sound row and attach it to the world's list. The
   twenty parameters and their widths come directly from the stack reads. */
// FUNCTION: WIZ8 0x0047a790
unsigned char AddAmbientSound0047A790(W8World* world, const char* name,
                                      const W8AmbientSoundConfig* config,
                                      const srVector3T<float>* position,
                                      const srVector3T<float>* region_min,
                                      const srVector3T<float>* region_max, int volume_min,
                                      int volume_max, int time_min, int time_max, int speed_min,
                                      int speed_max, float radius, unsigned char looping,
                                      unsigned char bounded, const srVector3T<float>* region_center,
                                      float region_angle, const srVector3T<float>* region_axis,
                                      const srVector3T<float>* region_scale, unsigned char shared)
{
    W8AmbientSound* sound = CreateAmbientSound0047A670();

    if (name != 0) {
        sound->pacSoundName = new char[strlen(name) + 1];
        if (sound->pacSoundName == 0) {
            srAssertFail("pSound->pacSoundName",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\AmbientSound.cpp", 0x35e,
                         "AmbientSound.cpp: Error allocating sound name");
        }
        strcpy(sound->pacSoundName, name);
    }
    sound->config_004 = *config;
    sound->position = *position;
    sound->region_min = *region_min;
    sound->region_max = *region_max;
    sound->region_center = *region_center;
    sound->region_angle = region_angle;
    sound->region_axis = *region_axis;
    sound->region_scale = *region_scale;
    sound->volume_min = volume_min == -1 ? 0x7f : volume_min;
    sound->volume_max = volume_max == -1 ? 0x7f : volume_max;
    sound->time_min = time_min == -1 ? 5000 : time_min;
    sound->time_max = time_max == -1 ? 20000 : time_max;
    sound->speed_min = speed_min;
    sound->speed_max = speed_max;
    sound->radius = radius;
    sound->looping = looping;
    sound->bounded = bounded;
    sound->shared = shared;
    PLAdoptAppend(world->plsAmbientSounds, sound);
    return 1;
}

// FUNCTION: WIZ8 0x0047a950
void PositionAmbientSoundByName0047A950(W8World* /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound =
            static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            srVector3T<float> position;
            GetCameraPosition(&position);
            sound->stopped = 0;
            sound->UpdatePosition(&position);
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0047a9e0
void StopAmbientSoundByName0047A9E0(W8World* /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound =
            static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            SoundStop(sound->sound_handle);
            sound->in_range = 0;
            sound->stopped = 1;
            sound->sound_handle = -1;
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0047aa70
void ToggleAmbientSoundByName0047AA70(W8World* /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound =
            static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            if (sound->stopped != 0) {
                srVector3T<float> position;
                GetCameraPosition(&position);
                sound->stopped = 0;
                sound->UpdatePosition(&position);
                return;
            }
            SoundStop(sound->sound_handle);
            sound->in_range = 0;
            sound->stopped = 1;
            sound->sound_handle = -1;
            return;
        }
    }
}

// GLOBAL: WIZ8 0x0065A110
unsigned short g_empty_ambient_name_65a110;

// FUNCTION: WIZ8 0x0047ab40
unsigned char LoadAmbientSoundList0047AB40(char* filename)
{
    int handle;
    unsigned char more = 1;
    char directory[260];
    char name[260];
    char path[260];
    char line[260];
    int configured[10];
    SOUNDPARMS direct;
    int direct_selector = 0;

    handle = FileOpen(filename, 0x41, 0);
    if (handle == 0) {
        return 0;
    }
    ReadTextLine004CEE40(handle, directory, 100, &more);
    while (more != 0) {
        int index;
        for (index = 0; index < 10; ++index) {
            configured[index] = -1;
        }
        memset(&direct, -1, sizeof(direct));
        memset(line, 0, sizeof(line));
        ReadTextLine004CEE40(handle, line, sizeof(line), &more);
        if (strlen(line) != 0) {
            sscanf(line, "%s %d %d %d %d %d %d %d", name, &configured[2], &configured[3],
                   &configured[4], &configured[5], &configured[0], &configured[1],
                   &direct_selector);
            sprintf(path, "%s\\%s", directory, name);
            SoundSetCacheThreshhold(0xc8000);
            if (direct_selector == -1) {
                configured[8] = -16;
                SoundPlayRandom(path, (RANDOMPARMS*)configured);
            } else {
                direct.uiLoop = direct_selector;
                direct.uiPriority = -16;
                direct.uiVolume = (g_settings_6850c8.sound_effects_volume * configured[5]) / 0x7f;
                SoundPlay(path, &direct);
            }
        }
    }
    FileClose(handle);
    return 1;
}

/* The volume/mute helpers operate on the whole sound-effects bus, not just the
   ambient list: each writes the configured effects volume through the SGP
   default, then re-drives every live ambient voice and 3D instance so the new
   level is heard immediately. */
// FUNCTION: WIZ8 0x0047ad00
void SetSoundEffectsVolume0047AD00(unsigned char volume)
{
    W8World* world;
    int count;
    int index;

    g_settings_6850c8.sound_effects_volume = volume;
    SoundSetDefaultVolume(volume);
    if (g_world != 0 && g_world->plsAmbientSounds != 0) {
        count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
            if (sound != 0) {
                if (sound->sound_handle != -1) {
                    unsigned int adjusted =
                        (sound->volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
                    sound->target_volume = adjusted;
                    sound->current_volume = adjusted;
                    SoundSetVolume(sound->sound_handle, adjusted);
                }
                sound->in_range = 0;
            }
        }
        Update3DSounds();
    }
    world = GetWorld();
    if (world != 0) {
        SoundServiceRandom();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                srVector3T<float> position;
                GetCameraPosition(&position);
                sound->UpdatePosition(&position);
            }
        }
        SoundServiceRandom();
        SoundServiceStreams();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                sound->Service(0);
                sound->UpdateFade();
            }
        }
        Update3DSounds();
    }
}

// FUNCTION: WIZ8 0x0047ae70
unsigned char GetSoundEffectsVolume(void)
{
    return g_settings_6850c8.sound_effects_volume;
}

// FUNCTION: WIZ8 0x0047ae80
bool IsSoundEffectsMuted(void)
{
    return g_settings_6850c8.muted_sound_effects_volume != 0xff;
}

// FUNCTION: WIZ8 0x0047ae90
void SetSoundEffectsMuted(unsigned char muted)
{
    W8World* world;
    int count;
    int index;

    if (muted != 0) {
        if (g_settings_6850c8.muted_sound_effects_volume == 0xff) {
            g_settings_6850c8.muted_sound_effects_volume = g_settings_6850c8.sound_effects_volume;
            g_settings_6850c8.sound_effects_volume = 0;
            SoundSetDefaultVolume(0);
            if (g_world != 0 && g_world->plsAmbientSounds != 0) {
                count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
                for (index = 0; index < count; ++index) {
                    W8AmbientSound* sound =
                        static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
                    if (sound != 0) {
                        if (sound->sound_handle != -1) {
                            unsigned int adjusted =
                                (sound->volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
                            sound->target_volume = adjusted;
                            sound->current_volume = adjusted;
                            SoundSetVolume(sound->sound_handle, adjusted);
                        }
                        sound->in_range = 0;
                    }
                }
                Update3DSounds();
            }
            world = GetWorld();
            if (world != 0) {
                SoundServiceRandom();
                count = static_cast<int>(PLLength(world->plsAmbientSounds));
                for (index = 0; index < count; ++index) {
                    W8AmbientSound* sound =
                        static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
                    if (sound != 0) {
                        srVector3T<float> position;
                        GetCameraPosition(&position);
                        sound->UpdatePosition(&position);
                    }
                }
                UpdateAmbientSounds0047A3E0(world);
                return;
            }
        }
    } else if (g_settings_6850c8.muted_sound_effects_volume != 0xff) {
        g_settings_6850c8.sound_effects_volume = g_settings_6850c8.muted_sound_effects_volume;
        SoundSetDefaultVolume(g_settings_6850c8.muted_sound_effects_volume);
        if (g_world != 0 && g_world->plsAmbientSounds != 0) {
            count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
            for (index = 0; index < count; ++index) {
                W8AmbientSound* sound =
                    static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
                if (sound != 0) {
                    if (sound->sound_handle != -1) {
                        unsigned int adjusted =
                            (sound->volume_max * g_settings_6850c8.sound_effects_volume) / 0x7f;
                        sound->target_volume = adjusted;
                        sound->current_volume = adjusted;
                        SoundSetVolume(sound->sound_handle, adjusted);
                    }
                    sound->in_range = 0;
                }
            }
            Update3DSounds();
        }
        world = GetWorld();
        if (world != 0) {
            SoundServiceRandom();
            count = static_cast<int>(PLLength(world->plsAmbientSounds));
            for (index = 0; index < count; ++index) {
                W8AmbientSound* sound =
                    static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
                if (sound != 0) {
                    srVector3T<float> position;
                    GetCameraPosition(&position);
                    sound->UpdatePosition(&position);
                }
            }
            SoundServiceRandom();
            SoundServiceStreams();
            count = static_cast<int>(PLLength(world->plsAmbientSounds));
            for (index = 0; index < count; ++index) {
                W8AmbientSound* sound =
                    static_cast<W8AmbientSound*>(PLGet(world->plsAmbientSounds, index));
                if (sound != 0) {
                    sound->Service(0);
                    sound->UpdateFade();
                }
            }
            Update3DSounds();
        }
        g_settings_6850c8.muted_sound_effects_volume = 0xff;
    }
}

// FUNCTION: WIZ8 0x0047b140
void SaveAmbientSoundList0047B140(HWFILE handle)
{
    unsigned char version = 1;
    unsigned int count;
    unsigned char ok;
    char empty_name[0x80];
    int index;

    memcpy(empty_name, &g_empty_ambient_name_65a110, 2);
    memset(empty_name + 2, 0, sizeof(empty_name) - 2);
    ok = FileWrite(handle, &version, 1, 0);
    if (g_world->plsAmbientSounds == 0) {
        count = 0;
        FileWrite(handle, &count, 4, 0);
        return;
    }
    count = PLLength(g_world->plsAmbientSounds);
    ok = ok && FileWrite(handle, &count, 4, 0);
    for (index = 0; index < static_cast<int>(count); ++index) {
        W8AmbientSound* sound =
            static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, index));
        if (sound != 0) {
            if (sound->pacSoundName == 0) {
                if (ok) {
                    ok = FileWrite(handle, empty_name, sizeof(empty_name), 0);
                }
            } else if (ok) {
                ok = FileWrite(handle, sound->pacSoundName, 0x80, 0);
            }
            if (ok) {
                ok = FileWrite(handle, &sound->stopped, 1, 0);
            }
        }
    }
}

/* The serialized per-record payload is just the 0x80-byte name and the stopped
   byte; on load each name is matched against the live list and the matching
   script commands are replayed — the helpers rescan the list themselves, and
   VC6 inlines that rescan twice here. */
// FUNCTION: WIZ8 0x0047b270
void LoadAmbientSoundList0047B270(HWFILE handle)
{
    unsigned char version;
    unsigned char ok;
    int count = 0;
    int index;
    char name[0x80];
    unsigned char stopped_flag;

    if (g_world->plsAmbientSounds == 0) {
        return;
    }
    ok = FileRead(handle, &version, 1, 0);
    if (ok != 0) {
        ok = FileRead(handle, &count, 4, 0);
    }
    for (index = 0; index < count; ++index) {
        int length;
        int scan;

        if (ok != 0 && FileRead(handle, name, 0x80, 0) != 0) {
            ok = FileRead(handle, &stopped_flag, 1, 0);
        } else {
            ok = 0;
        }
        length = static_cast<int>(PLLength(g_world->plsAmbientSounds));
        for (scan = 0; scan < length; ++scan) {
            W8AmbientSound* sound =
                static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, scan));
            if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
                if (sound != 0) {
                    if (stopped_flag == 0) {
                        PositionAmbientSoundByName0047A950(0, name);
                    } else {
                        StopAmbientSoundByName0047A9E0(0, name);
                    }
                }
                break;
            }
        }
    }
}
