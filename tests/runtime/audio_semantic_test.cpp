#include "audio_semantic_test.h"

#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Quality.h"
#include "wiz8/engine_code/stSound3D.h"
#include "wiz8/layouts/world.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/3d_code/PList.h"
#include "surrender/srScene.h"

#include "FileMan.h"

#include <stdio.h>
#include <string.h>

/* The save/load pair reads the world's ambient list through g_world. Keep the
   live world identity stable because the render thread reads its scene and
   camera concurrently; only replace the ambient-list field the check owns. */
static W8PList* g_saved_ambient_sounds;
static W8World g_audio_test_world;
static W8World* g_saved_world;

static void SetupAudioTestWorld()
{
    g_saved_world = g_world;
    if (g_world == 0) {
        memset(&g_audio_test_world, 0, sizeof(g_audio_test_world));
        g_audio_test_world.static_scene = new srScene(0);
        g_world = &g_audio_test_world;
    }
    g_saved_ambient_sounds = g_world->plsAmbientSounds;
    g_world->plsAmbientSounds = PLCreate();
}

/* GetCameraPosition reads the lazy global camera; create it if the menu state
   left it unbuilt and pin it so the listener position is deterministic. */
static void EnsureTestCamera()
{
    if (g_gd_camera_65a0f8 == 0) {
        g_gd_camera_65a0f8 = new GDCamera();
    }
    g_gd_camera_65a0f8->m_position_08c.Set(0.0f, 0.0f, 0.0f);
    g_gd_camera_65a0f8->m_yaw = 0.0f;
}

static unsigned char CheckFootstepPaths()
{
    char path[260];

    BuildFootstepPath(path, W8_FOOTSTEP_SURFACE_OUTDOORS_FLAT, W8_FOOTSTEP_MATERIAL_STONE,
                      W8_FOOTSTEP_KIND_STEP, 2);
    int step = strcmp(path, "Data\\Sound\\Footsteps\\Stone\\Step_OutdoorsFlat_Stone_02.WAV") == 0;

    BuildFootstepPath(path, W8_FOOTSTEP_SURFACE_OUTDOORS_FLAT, W8_FOOTSTEP_MATERIAL_STONE,
                      W8_FOOTSTEP_KIND_JUMP, 7);
    int jump = strcmp(path, "Data\\Sound\\Footsteps\\Stone\\Step_OutdoorsFlat_Stone_Jump.WAV") == 0;

    BuildFootstepPath(path, W8_FOOTSTEP_SURFACE_SMALL_CAVE, W8_FOOTSTEP_MATERIAL_GRASS,
                      W8_FOOTSTEP_KIND_SCUFF, 3);
    int scuff =
        strcmp(path, "Data\\Sound\\Footsteps\\Grass\\Step_SmallCave_Grass_Scuff_03.WAV") == 0;

    /* The material table drives both the directory name and the >= CLIMB_LADDER
       single-file bypass in PlayFootstep. */
    int vocabulary =
        strcmp(g_footstep_names_609edc[W8_FOOTSTEP_MATERIAL_CLIMB_LADDER], "ClimbLadder") == 0 &&
        strcmp(g_footstep_names_609edc[W8_FOOTSTEP_MATERIAL_SHALLOW_WATER], "ShallowWater") == 0 &&
        strcmp(g_footstep_names_609edc[W8_FOOTSTEP_MATERIAL_GRAVEL], "Gravel") == 0 &&
        strcmp(g_footstep_surfaces_609eb8[W8_FOOTSTEP_SURFACE_OUTDOORS_FLAT], "OutdoorsFlat") == 0;

    /* The >= CLIMB_LADDER single-file bypass never assigns the anti-repeat
       global; the ordinary variant path always does. That makes the branch
       observable regardless of which footstep waves SOUND.SLF carries. */
    unsigned char saved_option = GetRenderOptionState(15);
    int saved_variant = g_previous_footstep_variant_65a10c;
    SetRenderOption(15, 1);
    g_previous_footstep_variant_65a10c = 0;
    PlayFootstep0047A440(W8_FOOTSTEP_SURFACE_MEDIUM_ROOM, W8_FOOTSTEP_MATERIAL_CLIMB_LADDER,
                         W8_FOOTSTEP_KIND_STEP);
    int bypass = g_previous_footstep_variant_65a10c == 0;
    g_previous_footstep_variant_65a10c = 0;
    PlayFootstep0047A440(W8_FOOTSTEP_SURFACE_MEDIUM_ROOM, W8_FOOTSTEP_MATERIAL_STONE,
                         W8_FOOTSTEP_KIND_STEP);
    int variant =
        g_previous_footstep_variant_65a10c >= 1 && g_previous_footstep_variant_65a10c <= 4;
    g_previous_footstep_variant_65a10c = saved_variant;
    SetRenderOption(15, saved_option);

    return step | (jump << 1) | (scuff << 2) | (vocabulary << 3) | ((bypass & variant) << 4);
}

static unsigned char CheckSound3DLifecycle()
{
    int base = g_sound3d_instances_65be40.GetCount();
    unsigned char flags = 0;

    stSound3D* first = new stSound3D("alpha.wav", static_cast<srNode*>(0));
    if (g_sound3d_instances_65be40.GetCount() == base + 1 && first->sound_handle == -1) {
        flags |= 1;
    }

    stSound3D* second = new stSound3D("beta.wav", static_cast<srNode*>(0));
    first->volume = 0x40;
    first->falloff = 750.0f;
    *second = *first;
    /* operator= registers the destination again: retail's clone path treats
       assignment as publish-into-registry, so the entry count grows by one and
       the name is deep-copied while the live handle is not. */
    if (g_sound3d_instances_65be40.GetCount() == base + 3 && second->wave_name != 0 &&
        strcmp(second->wave_name, "alpha.wav") == 0 && second->wave_name != first->wave_name &&
        second->volume == 0x40 && second->falloff == 750.0f && second->sound_handle == -1) {
        flags |= 2;
    }

    delete first;
    if (g_sound3d_instances_65be40.GetCount() == base + 2 &&
        g_sound3d_instances_65be40.IndexOf(first) == -1) {
        flags |= 4;
    }

    delete second;
    /* The assignment added a second registration for the same object; remove
       the leftover entry so the global vector is exactly as found. */
    int duplicate = g_sound3d_instances_65be40.IndexOf(second);
    if (duplicate != -1) {
        g_sound3d_instances_65be40.RemoveAt(duplicate);
    }
    return flags;
}

static unsigned char CheckAmbientSerializeRoundtrip()
{
    W8AmbientSoundConfig config;
    srVector3T<float> far_position;
    srVector3T<float> zero;

    memset(&config, 0, sizeof(config));
    strcpy(config.wave_name, "wind.wav");
    far_position.Set(1000000.0f, 0.0f, 0.0f);
    zero.Set(0.0f, 0.0f, 0.0f);

    /* Radius one with a far emitter keeps UpdatePosition on the out-of-range
       path, so the load applies names and stopped flags without touching SGP. */
    AddAmbientSound(g_world, "cavewind", &config, &far_position, &zero, &zero, 0x40, 0x7f, 5000,
                    20000, 0x40, 0x40, 1.0f, 1, 0, &zero, 0.0f, &zero, &zero, 0);
    AddAmbientSound(g_world, "sewerdrip", &config, &far_position, &zero, &zero, 0x40, 0x7f, 5000,
                    20000, 0x40, 0x40, 1.0f, 1, 0, &zero, 0.0f, &zero, &zero, 0);

    W8AmbientSound* wind = static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, 0));
    W8AmbientSound* drip = static_cast<W8AmbientSound*>(PLGet(g_world->plsAmbientSounds, 1));
    if (wind == 0 || drip == 0) {
        return 0;
    }
    wind->stopped = 0;
    drip->stopped = 1;

    HWFILE file = FileOpen("AudioSemanticTest.sav", FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, 0);
    if (file == 0) {
        return 0;
    }
    SaveAmbientSoundList(file);
    FileClose(file);

    /* Flip the live state; the load must restore each record by name. */
    wind->stopped = 1;
    drip->stopped = 0;

    file = FileOpen("AudioSemanticTest.sav", FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    if (file == 0) {
        return 0;
    }
    LoadAmbientSoundList0047B270(file);
    FileClose(file);

    return wind->stopped == 0 && drip->stopped == 1 && wind->in_range == 0 && drip->in_range == 0;
}

static unsigned char CheckSound3DFalloff()
{
    SOUND3DPARMS options;
    srVector3T<float> listener;
    unsigned char flags = 0;
    unsigned char saved_volume = g_settings_6850c8.sound_effects_volume;

    g_settings_6850c8.sound_effects_volume = 0x7f;
    stSound3D* probe = new stSound3D("probe.wav", static_cast<srNode*>(0));
    probe->setLocation(0.0, 0.0, 0.0);
    probe->volume = 0x7f;
    probe->falloff = 1000.0f;

    listener.Set(0.0f, 0.0f, 0.0f);
    probe->BuildSoundOptions(&listener, &options);
    /* Distance zero keeps the full scaled volume; uiLoop defaults to one-shot
       (Play overrides it to infinite on request). */
    if (options.uiVolume == 0x7f && options.uiLoop == 1 && options.Pos.flFalloffMin == 1000.0f &&
        options.Pos.flFalloffMax == 1000.0f) {
        flags |= 1;
    }

    listener.Set(500.0f, 0.0f, 0.0f);
    probe->BuildSoundOptions(&listener, &options);
    /* Half the falloff distance halves the volume: (1 - 500/1000) * 0x7f,
       truncated through _ftol. */
    if (options.uiVolume == 63) {
        flags |= 2;
    }

    listener.Set(1000.0f, 0.0f, 0.0f);
    probe->BuildSoundOptions(&listener, &options);
    if (options.uiVolume == 0) {
        flags |= 4;
    }

    listener.Set(1500.0f, 0.0f, 0.0f);
    probe->BuildSoundOptions(&listener, &options);
    /* Retail does not clamp beyond the edge: the negative factor wraps the
       unsigned volume. */
    if (options.uiVolume > 0x7fffffff) {
        flags |= 8;
    }

    /* Muted bus: effects volume zero collapses every distance to silence. */
    g_settings_6850c8.sound_effects_volume = 0;
    listener.Set(0.0f, 0.0f, 0.0f);
    probe->BuildSoundOptions(&listener, &options);
    if (options.uiVolume == 0) {
        flags |= 0x10;
    }

    g_settings_6850c8.sound_effects_volume = saved_volume;
    delete probe;
    return flags;
}

static unsigned char CheckMuteState()
{
    unsigned char saved_volume = g_settings_6850c8.sound_effects_volume;
    unsigned char saved_muted = g_settings_6850c8.muted_sound_effects_volume;
    unsigned char ok = 1;

    /* 0xff is the unmuted sentinel; muting saves the live volume there and
       zeroes the bus, unmuting restores it. */
    g_settings_6850c8.sound_effects_volume = 100;
    g_settings_6850c8.muted_sound_effects_volume = 0xff;
    SetSoundEffectsMuted(1);
    ok = ok && IsSoundEffectsMuted() && GetSoundEffectsVolume() == 0;
    SetSoundEffectsMuted(0);
    ok = ok && !IsSoundEffectsMuted() && GetSoundEffectsVolume() == 100;

    g_settings_6850c8.sound_effects_volume = saved_volume;
    g_settings_6850c8.muted_sound_effects_volume = saved_muted;
    return ok;
}

bool RunAudioSemanticTests(AudioSemanticResult* result)
{
    unsigned char footstep = CheckFootstepPaths();

    memset(result, 0, sizeof(*result));
    result->footstep_step_path = footstep & 1;
    result->footstep_jump_path = (footstep >> 1) & 1;
    result->footstep_scuff_path = (footstep >> 2) & 1;
    result->footstep_material_vocabulary = (footstep >> 3) & 1;
    result->footstep_bypass = (footstep >> 4) & 1;

    unsigned char lifecycle = CheckSound3DLifecycle();
    result->sound3d_ctor_registers = lifecycle & 1;
    result->sound3d_assign_registers_and_copies = (lifecycle >> 1) & 1;
    result->sound3d_dtor_removes = (lifecycle >> 2) & 1;

    EnsureTestCamera();
    SetupAudioTestWorld();
    result->ambient_serialize_roundtrip = CheckAmbientSerializeRoundtrip();
    result->sound3d_falloff_volume = CheckSound3DFalloff() == 0x1f;
    result->mute_state_roundtrip = CheckMuteState();
    g_world->plsAmbientSounds = g_saved_ambient_sounds;
    g_world = g_saved_world;

    return result->footstep_step_path && result->footstep_jump_path &&
           result->footstep_scuff_path && result->footstep_material_vocabulary &&
           result->footstep_bypass && result->sound3d_ctor_registers &&
           result->sound3d_assign_registers_and_copies && result->sound3d_dtor_removes &&
           result->ambient_serialize_roundtrip && result->sound3d_falloff_volume &&
           result->mute_state_roundtrip;
}

void PrintAudioSemanticResults(const AudioSemanticResult* result)
{
    fprintf(stderr,
            "runtime-test audio: footstep_step=%u footstep_jump=%u footstep_scuff=%u "
            "footstep_vocabulary=%u footstep_bypass=%u sound3d_ctor=%u sound3d_assign=%u "
            "sound3d_dtor=%u serialize_roundtrip=%u falloff=%u mute=%u\n",
            result->footstep_step_path, result->footstep_jump_path, result->footstep_scuff_path,
            result->footstep_material_vocabulary, result->footstep_bypass,
            result->sound3d_ctor_registers, result->sound3d_assign_registers_and_copies,
            result->sound3d_dtor_removes, result->ambient_serialize_roundtrip,
            result->sound3d_falloff_volume, result->mute_state_roundtrip);
    fflush(stderr);
}
