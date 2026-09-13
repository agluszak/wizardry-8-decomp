#include "wiz8/sound_man.h"
#include "soundman.h"

/* Unresolved fragment: 0x00479010 lies in the anchored gap between
   stMeshModel.cpp (ends 0x00473BF0) and AmbientSound.cpp (0x0047A670), the
   same interval that holds the unproven GDCamera cluster. */

int g_dword_65a104;

// FUNCTION: WIZ8 0x00479010
void ConfigureSoundCache(void)
{
    SoundSetCacheThreshhold(0xc8000);
    g_dword_65a104 = 0;
}
