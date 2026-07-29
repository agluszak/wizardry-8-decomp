#include "wiz8/sr_api.h"

/* Engine Code\AmbientSound.cpp. The allocation and release pair establishes
   the complete 0x12c-byte object and four members used by its lifetime. */
class W8AmbientSound {
public:
    W8AmbientSound();                    /* 0x00479040 */
    ~W8AmbientSound();                   /* 0x0047A780 */

    char* pacSoundName;                  /* 0x000: assertion-backed at 0x47A790 */
    unsigned char unknown_004[0x98];
    int value_9c;
    unsigned char unknown_0a0[0x1c];
    int sound_handle_bc;
    int sound_handle_c0;
    unsigned char unknown_0c4[0x68];
};

static_assert(sizeof(W8AmbientSound) == 0x12c, "W8AmbientSound_must_be_0x12c");

extern void ReleaseSoundHandle00408F70(int handle);
extern void ReleaseAmbientChannel0040A8E0(int handle, int channel);

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
