#include "wiz8/sr_api.h"
#include "wiz8/3d_code/PList.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/gameplay_boundaries.h"

#include <string.h>

struct W8AmbientSoundConfig0047A790 {
    unsigned char bytes[0x80];
};

/* Engine Code\AmbientSound.cpp. The allocation and release pair establishes
   the complete 0x12c-byte object and four members used by its lifetime. */
class W8AmbientSound {
public:
    W8AmbientSound();                    /* 0x00479040 */
    ~W8AmbientSound();                   /* 0x0047A780 */

    char* pacSoundName;                  /* 0x000: assertion-backed at 0x47A790 */
    W8AmbientSoundConfig0047A790 config_004; /* 0x004 */
    unsigned char flag_84;
    unsigned char unknown_085[3];
    srVector3T<float> vector_88;
    int value_94;
    int value_98;
    int value_9c;
    unsigned char unknown_0a0[4];
    int value_a4;
    int value_a8;
    int value_ac;
    int value_b0;
    int value_b4;
    unsigned char flag_b8;
    unsigned char flag_b9;
    unsigned char unknown_0ba[2];
    int sound_handle_bc;
    int sound_handle_c0;
    unsigned char flag_c4;
    unsigned char flag_c5;
    unsigned char unknown_0c6[2];
    srVector3T<float> vector_c8;
    srVector3T<float> vector_d4;
    srVector3T<float> vector_e0;
    int value_ec;
    srVector3T<float> vector_f0;
    srVector3T<float> vector_fc;
    W8Timer005EC0A4 timer_108;

    void ApplyPosition00479350(const srVector3T<float>* position);
};

static_assert(sizeof(W8AmbientSound) == 0x12c, "W8AmbientSound_must_be_0x12c");

extern void ReleaseSoundHandle00408F70(int handle);
extern void ReleaseAmbientChannel0040A8E0(int handle, int channel);
extern void GetPartyPosition(srVector3T<float>* position); /* 0x00421070 */
extern W8World* g_world_00659ab4;

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
    PListAdd(world->plsAmbientSounds, sound);
    return 1;
}

// FUNCTION: WIZ8 0x0047a950
void PositionAmbientSoundByName0047A950(int /* unused */, const char* name)
{
    int count = static_cast<int>(PListGetCount(g_world_00659ab4->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PListGetAt(g_world_00659ab4->plsAmbientSounds, index));
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
    int count = static_cast<int>(PListGetCount(g_world_00659ab4->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PListGetAt(g_world_00659ab4->plsAmbientSounds, index));
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
    int count = static_cast<int>(PListGetCount(g_world_00659ab4->plsAmbientSounds));
    int index;

    for (index = 0; index < count; ++index) {
        W8AmbientSound* sound = static_cast<W8AmbientSound*>(
            PListGetAt(g_world_00659ab4->plsAmbientSounds, index));
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
