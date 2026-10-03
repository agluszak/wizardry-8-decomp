#include "wiz8/sound_man.h"
#include "soundman.h"

/* Original translation unit is not established by the surrounding source anchors. */

// GLOBAL: WIZ8 0x0065A104
static int g_dword_65a104;

// FUNCTION: WIZ8 0x00479010
void ConfigureSoundCache(void)
{
    SoundSetCacheThreshhold(0xc8000);
    g_dword_65a104 = 0;
}
