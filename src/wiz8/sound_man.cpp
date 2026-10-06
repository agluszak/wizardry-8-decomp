#include "wiz8/sound_man.h"
#include "soundman.h"

/* Original translation unit is not established by the surrounding source anchors. */

/* The sole retail access clears this state while configuring the sound cache. */
// GLOBAL: WIZ8 0x0065A104
static int g_sound_cache_reset_state;

// FUNCTION: WIZ8 0x00479010
void ConfigureSoundCache(void)
{
    SoundSetCacheThreshhold(0xc8000);
    g_sound_cache_reset_state = 0;
}
