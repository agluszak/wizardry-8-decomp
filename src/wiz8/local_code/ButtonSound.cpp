#include "soundman.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/sound_man.h"
#include "wiz8/sr_api.h"
#include "timer.h"

// GLOBAL: WIZ8 0x0062A458
const char* g_button_sound_paths[4][4] = {{0, 0, 0, 0},
                                          {g_button_whoosh, 0, g_button_click_2, g_button_click_1},
                                          {0, 0, g_button_click_2, g_button_click_1},
                                          {g_button_whoosh, 0, g_button_click_2, g_button_click_1}};
// GLOBAL: WIZ8 0x0062A498
int g_button_sound_scheme_stack[32] = {1};
// GLOBAL: WIZ8 0x0062A518
int g_button_sound_override = -1;

// GLOBAL: WIZ8 0x0062A51C
char g_button_click_1[] = "Data\\Sound\\Misc\\Interface Click 01.wav";
// GLOBAL: WIZ8 0x0062A544
char g_button_click_2[] = "Data\\Sound\\Misc\\Interface Click 02.wav";
// GLOBAL: WIZ8 0x0062A56C
char g_button_whoosh[] = "Data\\Sound\\Misc\\Interface Whoosh 01 Soft.wav";

/* Local Code\ButtonSound.cpp's fixed scheme stack. The assertion supplies the
   original top name and the SCHEME_STACK_SIZE spelling; the body establishes
   the 32-entry capacity and pre-increment push convention. */
// GLOBAL: WIZ8 0x0068DE38
int g_button_sound_scheme_stack_top;
// GLOBAL: WIZ8 0x0068DE3C
unsigned int g_button_sound_cooldown;

// FUNCTION: WIZ8 0x00558720
void PlayButtonSound(int sound_id)
{
    int scheme = g_button_sound_override;
    if (scheme < 0) {
        if (g_button_sound_scheme_stack_top < 0) {
            return;
        }
        scheme = g_button_sound_scheme_stack[g_button_sound_scheme_stack_top];
    } else {
        g_button_sound_override = -1;
    }

    if (scheme < 0) {
        return;
    }
    const char* path = g_button_sound_paths[scheme][sound_id];
    if (path == 0) {
        return;
    }
    if (sound_id < 2) {
        if (ClockIsTicking(g_button_sound_cooldown) != 0) {
            return;
        }
        g_button_sound_cooldown = SetCountdownClock(200);
    }

    SOUNDPARMS options;
    memset(&options, -1, sizeof(options));
    options.uiVolume = g_settings.sound_effects_volume >> 1;
    SoundPlay((STR)path, &options);
}

// FUNCTION: WIZ8 0x005587c0
void PushButtonSoundScheme(int scheme, char replace_current)
{
    if (replace_current != 0) {
        g_button_sound_override = scheme;
        return;
    }
    if (g_button_sound_scheme_stack_top >= 32) {
        srAssertFail("giSchemeStackTop < SCHEME_STACK_SIZE",
                     "C:\\Projects\\Wizardry 8\\Local Code\\ButtonSound.cpp", 0x5d, 0);
    }
    ++g_button_sound_scheme_stack_top;
    g_button_sound_scheme_stack[g_button_sound_scheme_stack_top] = scheme;
}

// FUNCTION: WIZ8 0x00558810
void ResetButtonSoundScheme(void)
{
    g_button_sound_override = -1;
}
