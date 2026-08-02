#include "wiz8/sr_api.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/AmbientSound.h"
#include "wiz8/gameplay_boundaries.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/virtual_file.h"
#include "FileMan.h"
#include "random.h"

#include <string.h>
#include <stdio.h>

extern void ReleaseSoundHandle00408F70(int handle);
extern void ReleaseAmbientChannel0040A8E0(int handle, int channel);
extern void GetPartyPosition(srVector3T<float>* position); /* 0x00421070 */
extern void AudioUpdateBegin00409310();
extern void AudioUpdateStage004095B0();
extern void AudioUpdateFinish004AEFD0();
extern void SetSoundValue00409210(int handle, unsigned int value);

// FUNCTION: WIZ8 0x00479040
W8AmbientSound::W8AmbientSound()
    : value_94(0),
      value_98(0),
      value_a0(0),
      value_a4(0),
      value_a8(0),
      value_ac(0),
      value_b0(0),
      value_b4(0),
      flag_b8(0),
      flag_b9(0),
      sound_handle_bc(-1),
      sound_handle_c0(-1),
      flag_c4(0),
      flag_c5(0),
      value_ec(0)
{
    config_004.match_name[0] = 0;
    vector_88.x = 0.0f;
    vector_88.y = 0.0f;
    vector_88.z = 0.0f;
    pacSoundName = 0;
    flag_84 = 0;
}

/* The contiguous class lifecycle and shared world-list accesses identify these
   pre-assertion bodies as the same AmbientSound.cpp unit. */
// FUNCTION: WIZ8 0x0047a260
W8AmbientSound* W8AmbientSound::FindNextMatching0047A260(
    const char* match_name, W8AmbientSound* previous)
{
    int count;
    int index;

    if (g_world == 0) {
        return 0;
    }
    count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    if (previous == 0) {
        index = 0;
    }
    else {
        index = PListIndexOf(g_world->plsAmbientSounds, previous) + 1;
        if (index < 0 || index > count) {
            return 0;
        }
    }
    if (index >= count) {
        return 0;
    }
    do {
        W8AmbientSound* candidate = static_cast<W8AmbientSound*>(
            PLGet(g_world->plsAmbientSounds, index));
        if (candidate != 0 && candidate != this && candidate->flag_c4 != 0 &&
            _stricmp(candidate->config_004.match_name, match_name) == 0) {
            return candidate;
        }
        ++index;
    } while (index < count);
    return 0;
}

// FUNCTION: WIZ8 0x0047a310
void W8AmbientSound::Update0047A310()
{
    if (flag_c4 != 0) {
        if (sound_handle_bc != -1) {
            W8GameTimer* timer = &timer_108;

            if (timer->GetProgress() >= 1.0f) {
                if (value_9c > value_a0) {
                    ++value_a0;
                }
                else if (value_9c < value_a0) {
                    --value_a0;
                }
                else if ((timer->m_flags & 8) == 0) {
                    timer->m_flags |= 8;
                    timer->m_start = timer->Method00439A60() - timer->m_start;
                }
                SetSoundValue00409210(sound_handle_bc, value_a0);
            }
        }
        if (flag_b8 == 0 && value_9c == value_a0 && sound_handle_bc != -1) {
            ReleaseSoundHandle00408F70(sound_handle_bc);
            sound_handle_bc = -1;
        }
    }
}

// FUNCTION: WIZ8 0x0047a3e0
void UpdateAmbientSounds0047A3E0(W8World* world)
{
    if (world != 0) {
        int count;
        int index;

        AudioUpdateBegin00409310();
        AudioUpdateStage004095B0();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                sound->SetState00479970(0);
                sound->Update0047A310();
            }
        }
        AudioUpdateFinish004AEFD0();
    }
}

extern unsigned char GetRenderOptionState(int option);
extern void BuildFootstepPath0047A540(
    char* path, char surface, char material, char kind, int variant);
extern int PlaySound00408860(const char* path, int* options);
extern unsigned char g_default_footstep_surface_65a108;
extern unsigned char g_default_footstep_material_65a109;
extern unsigned char g_footstep_alternate_65a10a;
extern int g_previous_footstep_variant_65a10c;
extern unsigned char g_footstep_option_6850f9;
extern const char* g_footstep_names_609edc[];
extern const char* g_footstep_surfaces_609eb8[];
extern const char* g_footstep_fixed_name_609f44;
extern const char* g_footstep_scuff_name_609f48;

// FUNCTION: WIZ8 0x0047a440
int PlayFootstep0047A440(char surface, char material, int argument)
{
    char selected_surface;
    char selected_material;
    char path[260];
    int options[8];
    int attempts = 0;
    int index;

    if (GetRenderOptionState(0xf) == 0) {
        return -1;
    }
    selected_surface = surface;
    if (selected_surface < 1 || selected_surface > 9) {
        selected_surface = g_default_footstep_surface_65a108;
    }
    selected_material = material;
    if (selected_material < 1 || selected_material > 25) {
        selected_material = g_default_footstep_material_65a109;
    }
    if (selected_material >= 18) {
        sprintf(path, "Data\\Sound\\Footsteps\\Step_%s.WAV",
                g_footstep_names_609edc[selected_material]);
    }
    else {
        int variant;
        do {
            variant = Random(4) + 1;
            ++attempts;
        } while (variant == g_previous_footstep_variant_65a10c && attempts < 100);
        BuildFootstepPath0047A540(
            path, selected_surface, selected_material, argument, variant);
        g_previous_footstep_variant_65a10c = variant;
    }
    for (index = 0; index < 8; ++index) {
        options[index] = -1;
    }
    options[2] = g_footstep_option_6850f9;
    g_footstep_alternate_65a10a = g_footstep_alternate_65a10a == 0;
    return PlaySound00408860(path, options);
}

// FUNCTION: WIZ8 0x0047a540
void BuildFootstepPath0047A540(
    char* path, char surface, char material, char kind, int variant)
{
    if (kind == 0) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%.2d.WAV",
                g_footstep_names_609edc[material],
                g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], variant);
        return;
    }
    if (kind == 1) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%s.WAV",
                g_footstep_names_609edc[material],
                g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], g_footstep_fixed_name_609f44);
        return;
    }
    if (kind == 2) {
        sprintf(path, "Data\\Sound\\Footsteps\\%s\\Step_%s_%s_%s_%.2d.WAV",
                g_footstep_names_609edc[material],
                g_footstep_surfaces_609eb8[surface],
                g_footstep_names_609edc[material], g_footstep_scuff_name_609f48,
                variant);
    }
}

// FUNCTION: WIZ8 0x0047a600
void RepositionAmbientSounds0047A600(W8World* world)
{
    if (world != 0) {
        int count;
        int index;

        AudioUpdateBegin00409310();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                srVector3T<float> position;
                GetPartyPosition(&position);
                sound->ApplyPosition00479350(&position);
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
    sound->value_9c = 0;
    return sound;
}

// FUNCTION: WIZ8 0x0047a700
void DestroyAmbientSound0047A700(W8AmbientSound* ambient)
{
    if (ambient == 0) {
        srAssertFail("pAmbient", "C:\\Projects\\Wizardry 8\\Engine Code\\AmbientSound.cpp", 0x2fd, 0);
    }
    if (ambient->sound_handle_bc != -1) {
        ReleaseSoundHandle00408F70(ambient->sound_handle_bc);
    }
    if (ambient->sound_handle_c0 != -1) {
        ReleaseAmbientChannel0040A8E0(ambient->sound_handle_c0, 6);
    }
    if (ambient->pacSoundName != 0) {
        operator delete(ambient->pacSoundName);
    }
    delete ambient;
}

// FUNCTION: WIZ8 0x0047a780
W8AmbientSound::~W8AmbientSound()
{
}

/* Build a complete ambient-sound row and attach it to the world's list. The
   twenty parameters and their widths come directly from the stack reads. */
// FUNCTION: WIZ8 0x0047a790
unsigned char AddAmbientSound0047A790(
    W8World* world, const char* name, const W8AmbientSoundConfig0047A790* config,
    const srVector3T<float>* vector_88, const srVector3T<float>* vector_c8,
    const srVector3T<float>* vector_d4, int value_94, int value_98,
    int value_ac, int value_b0, int value_a4, int value_a8, int value_b4,
    unsigned char flag_b9, unsigned char flag_c5,
    const srVector3T<float>* vector_e0, int value_ec,
    const srVector3T<float>* vector_f0, const srVector3T<float>* vector_fc,
    unsigned char flag_c4)
{
    W8AmbientSound* sound = CreateAmbientSound0047A670();

    if (name != 0) {
        sound->pacSoundName = new char[strlen(name) + 1];
        if (sound->pacSoundName == 0) {
            srAssertFail("pSound->pacSoundName",
                         "C:\\Projects\\Wizardry 8\\Engine Code\\AmbientSound.cpp",
                         0x35e, "AmbientSound.cpp: Error allocating sound name");
        }
        strcpy(sound->pacSoundName, name);
    }
    sound->config_004 = *config;
    sound->vector_88 = *vector_88;
    sound->vector_c8 = *vector_c8;
    sound->vector_d4 = *vector_d4;
    sound->vector_e0 = *vector_e0;
    sound->value_ec = value_ec;
    sound->vector_f0 = *vector_f0;
    sound->vector_fc = *vector_fc;
    sound->value_94 = value_94 == -1 ? 0x7f : value_94;
    sound->value_98 = value_98 == -1 ? 0x7f : value_98;
    sound->value_ac = value_ac == -1 ? 5000 : value_ac;
    sound->value_b0 = value_b0 == -1 ? 20000 : value_b0;
    sound->value_a4 = value_a4;
    sound->value_a8 = value_a8;
    sound->value_b4 = value_b4;
    sound->flag_b9 = flag_b9;
    sound->flag_c5 = flag_c5;
    sound->flag_c4 = flag_c4;
    PLAdoptAppend(world->plsAmbientSounds, sound);
    return 1;
}

// FUNCTION: WIZ8 0x0047a950
void PositionAmbientSoundByName0047A950(int /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            srVector3T<float> position;
            GetPartyPosition(&position);
            sound->flag_84 = 0;
            sound->ApplyPosition00479350(&position);
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0047a9e0
void StopAmbientSoundByName0047A9E0(int /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            ReleaseSoundHandle00408F70(sound->sound_handle_bc);
            sound->flag_b8 = 0;
            sound->flag_84 = 1;
            sound->sound_handle_bc = -1;
            return;
        }
    }
}

// FUNCTION: WIZ8 0x0047aa70
void ToggleAmbientSoundByName0047AA70(int /* unused */, const char* name)
{
    int count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PLGet(g_world->plsAmbientSounds, index));
        if (sound->pacSoundName != 0 && _stricmp(sound->pacSoundName, name) == 0) {
            if (sound->flag_84 != 0) {
                srVector3T<float> position;
                GetPartyPosition(&position);
                sound->flag_84 = 0;
                sound->ApplyPosition00479350(&position);
                return;
            }
            ReleaseSoundHandle00408F70(sound->sound_handle_bc);
            sound->flag_b8 = 0;
            sound->flag_84 = 1;
            sound->sound_handle_bc = -1;
            return;
        }
    }
}

extern unsigned char ReadTextLine004CEE40(
    int handle, char* destination, int capacity, unsigned char* more);
extern int PlaySoundConfigured00408D60(const char* path, int* options);
extern "C" unsigned char Function4098D0(int size);
extern "C" void SetMasterSoundVolume(unsigned int volume);
extern unsigned char g_master_ambient_volume_6850f6;
extern unsigned char g_saved_ambient_volume_6850fa;
extern const unsigned short g_empty_ambient_name_65a110;

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
    int direct[8];
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
        for (index = 0; index < 8; ++index) {
            direct[index] = -1;
        }
        memset(line, 0, sizeof(line));
        ReadTextLine004CEE40(handle, line, sizeof(line), &more);
        if (strlen(line) != 0) {
            sscanf(line, "%s %d %d %d %d %d %d %d", name,
                   &configured[2], &configured[3], &configured[4],
                   &configured[5], &configured[0], &configured[1],
                   &direct_selector);
            sprintf(path, "%s\\%s", directory, name);
            Function4098D0(0xc8000);
            if (direct_selector == -1) {
                configured[8] = -16;
                PlaySoundConfigured00408D60(path, configured);
            }
            else {
                direct[4] = direct_selector;
                direct[5] = -16;
                direct[2] =
                    (g_master_ambient_volume_6850f6 * configured[5]) / 0x7f;
                PlaySound00408860(path, direct);
            }
        }
    }
    CloseVirtualFile(handle);
    return 1;
}

// FUNCTION: WIZ8 0x0047ad00
void SetAmbientSoundVolume0047AD00(unsigned char volume)
{
    W8World* world;
    int count;
    int index;

    g_master_ambient_volume_6850f6 = volume;
    SetMasterSoundVolume(volume);
    if (g_world != 0 && g_world->plsAmbientSounds != 0) {
        count = static_cast<int>(PLLength(g_world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                PLGet(g_world->plsAmbientSounds, index));
            if (sound != 0) {
                if (sound->sound_handle_bc != -1) {
                    unsigned int adjusted =
                        (sound->value_98 * g_master_ambient_volume_6850f6) / 0x7f;
                    sound->value_9c = adjusted;
                    sound->value_a0 = adjusted;
                    SetSoundValue00409210(sound->sound_handle_bc, adjusted);
                }
                sound->flag_b8 = 0;
            }
        }
        AudioUpdateFinish004AEFD0();
    }
    world = GetWorld();
    if (world != 0) {
        AudioUpdateBegin00409310();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                srVector3T<float> position;
                GetPartyPosition(&position);
                sound->ApplyPosition00479350(&position);
            }
        }
        AudioUpdateBegin00409310();
        AudioUpdateStage004095B0();
        count = static_cast<int>(PLLength(world->plsAmbientSounds));
        for (index = 0; index < count; ++index) {
            W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                PLGet(world->plsAmbientSounds, index));
            if (sound != 0) {
                sound->SetState00479970(0);
                sound->Update0047A310();
            }
        }
        AudioUpdateFinish004AEFD0();
    }
}

// FUNCTION: WIZ8 0x0047ae90
void SetAmbientSoundMuted0047AE90(char muted)
{
    W8World* world;
    int count;
    int index;

    if (muted == 0) {
        if (g_saved_ambient_volume_6850fa != 0xff) {
            g_master_ambient_volume_6850f6 = g_saved_ambient_volume_6850fa;
            SetMasterSoundVolume(g_saved_ambient_volume_6850fa);
            if (g_world != 0 &&
                g_world->plsAmbientSounds != 0) {
                count = static_cast<int>(
                    PLLength(g_world->plsAmbientSounds));
                for (index = 0; index < count; ++index) {
                    W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                        PLGet(g_world->plsAmbientSounds, index));
                    if (sound != 0) {
                        if (sound->sound_handle_bc != -1) {
                            unsigned int adjusted =
                                (sound->value_98 * g_master_ambient_volume_6850f6) /
                                0x7f;
                            sound->value_9c = adjusted;
                            sound->value_a0 = adjusted;
                            SetSoundValue00409210(sound->sound_handle_bc, adjusted);
                        }
                        sound->flag_b8 = 0;
                    }
                }
                AudioUpdateFinish004AEFD0();
            }
            world = GetWorld();
            if (world != 0) {
                AudioUpdateBegin00409310();
                count = static_cast<int>(PLLength(world->plsAmbientSounds));
                for (index = 0; index < count; ++index) {
                    W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                        PLGet(world->plsAmbientSounds, index));
                    if (sound != 0) {
                        srVector3T<float> position;
                        GetPartyPosition(&position);
                        sound->ApplyPosition00479350(&position);
                    }
                }
                AudioUpdateBegin00409310();
                AudioUpdateStage004095B0();
                count = static_cast<int>(PLLength(world->plsAmbientSounds));
                for (index = 0; index < count; ++index) {
                    W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                        PLGet(world->plsAmbientSounds, index));
                    if (sound != 0) {
                        sound->SetState00479970(0);
                        sound->Update0047A310();
                    }
                }
                AudioUpdateFinish004AEFD0();
            }
            g_saved_ambient_volume_6850fa = 0xff;
        }
    }
    else if (g_saved_ambient_volume_6850fa == 0xff) {
        g_saved_ambient_volume_6850fa = g_master_ambient_volume_6850f6;
        g_master_ambient_volume_6850f6 = 0;
        SetMasterSoundVolume(0);
        if (g_world != 0 &&
            g_world->plsAmbientSounds != 0) {
            count = static_cast<int>(
                PLLength(g_world->plsAmbientSounds));
            for (index = 0; index < count; ++index) {
                W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                    PLGet(g_world->plsAmbientSounds, index));
                if (sound != 0) {
                    if (sound->sound_handle_bc != -1) {
                        unsigned int adjusted =
                            (sound->value_98 * g_master_ambient_volume_6850f6) /
                            0x7f;
                        sound->value_9c = adjusted;
                        sound->value_a0 = adjusted;
                        SetSoundValue00409210(sound->sound_handle_bc, adjusted);
                    }
                    sound->flag_b8 = 0;
                }
            }
            AudioUpdateFinish004AEFD0();
        }
        world = GetWorld();
        if (world != 0) {
            AudioUpdateBegin00409310();
            count = static_cast<int>(PLLength(world->plsAmbientSounds));
            for (index = 0; index < count; ++index) {
                W8AmbientSound* sound = static_cast<W8AmbientSound*>(
                    PLGet(world->plsAmbientSounds, index));
                if (sound != 0) {
                    srVector3T<float> position;
                    GetPartyPosition(&position);
                    sound->ApplyPosition00479350(&position);
                }
            }
            UpdateAmbientSounds0047A3E0(world);
            return;
        }
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
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PLGet(g_world->plsAmbientSounds, index));
        if (sound != 0) {
            if (sound->pacSoundName == 0) {
                if (ok) {
                    ok = FileWrite(handle, empty_name, sizeof(empty_name), 0);
                }
            }
            else if (ok) {
                ok = FileWrite(handle, sound->pacSoundName, 0x80, 0);
            }
            if (ok) {
                ok = FileWrite(handle, &sound->flag_84, 1, 0);
            }
        }
    }
}
