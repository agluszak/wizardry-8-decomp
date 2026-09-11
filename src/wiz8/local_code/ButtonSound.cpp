#include "soundman.h"
#include "wiz8/local_code/ButtonSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/sound_man.h"
#include "wiz8/sr_api.h"
#include "timer.h"

extern char g_button_click_1_62a51c[];
extern char g_button_click_2_62a544[];
extern char g_button_whoosh_62a56c[];

// GLOBAL: WIZ8 0x0062A458
const char* g_button_sound_paths_62a458[4][4] = {
    {0, 0, 0, 0},
    {g_button_whoosh_62a56c, 0, g_button_click_2_62a544,
     g_button_click_1_62a51c},
    {0, 0, g_button_click_2_62a544, g_button_click_1_62a51c},
    {g_button_whoosh_62a56c, 0, g_button_click_2_62a544,
     g_button_click_1_62a51c}
};
// GLOBAL: WIZ8 0x0062A498
int g_button_sound_scheme_stack_62a498[32] = {1};
// GLOBAL: WIZ8 0x0062A518
int g_button_sound_override_62a518 = -1;

// GLOBAL: WIZ8 0x0062A51C
char g_button_click_1_62a51c[] =
    "Data\\Sound\\Misc\\Interface Click 01.wav";
// GLOBAL: WIZ8 0x0062A544
char g_button_click_2_62a544[] =
    "Data\\Sound\\Misc\\Interface Click 02.wav";
// GLOBAL: WIZ8 0x0062A56C
char g_button_whoosh_62a56c[] =
    "Data\\Sound\\Misc\\Interface Whoosh 01 Soft.wav";

/* Local Code\ButtonSound.cpp's fixed scheme stack. The assertion supplies the
   original top name and the SCHEME_STACK_SIZE spelling; the body establishes
   the 32-entry capacity and pre-increment push convention. */
// GLOBAL: WIZ8 0x0068DE38
int g_button_sound_scheme_stack_top_68de38;
// GLOBAL: WIZ8 0x0068DE3C
unsigned int g_button_sound_cooldown_68de3c;

// FUNCTION: WIZ8 0x00558720
void PlayButtonSound(int sound_id)
{
    int scheme = g_button_sound_override_62a518;
    if (scheme < 0) {
        if (g_button_sound_scheme_stack_top_68de38 < 0) {
            return;
        }
        scheme = g_button_sound_scheme_stack_62a498[
            g_button_sound_scheme_stack_top_68de38];
    }
    else {
        g_button_sound_override_62a518 = -1;
    }

    if (scheme < 0) {
        return;
    }
    const char* path = g_button_sound_paths_62a458[scheme][sound_id];
    if (path == 0) {
        return;
    }
    if (sound_id < 2) {
        if (ClockIsTicking(g_button_sound_cooldown_68de3c) != 0) {
            return;
        }
        g_button_sound_cooldown_68de3c = SetCountdownClock(200);
    }

    SOUNDPARMS options;
    memset(&options, -1, sizeof(options));
    options.uiVolume = g_settings_6850c8.sound_effects_volume >> 1;
    SoundPlay((STR)path, &options);
}

// FUNCTION: WIZ8 0x005587c0
void PushButtonSoundScheme005587C0(int scheme, char replace_current)
{
    if (replace_current != 0) {
        g_button_sound_override_62a518 = scheme;
        return;
    }
    if (g_button_sound_scheme_stack_top_68de38 >= 32) {
        srAssertFail("giSchemeStackTop < SCHEME_STACK_SIZE",
                     "C:\\Projects\\Wizardry 8\\Local Code\\ButtonSound.cpp", 0x5d, 0);
    }
    ++g_button_sound_scheme_stack_top_68de38;
    g_button_sound_scheme_stack_62a498[g_button_sound_scheme_stack_top_68de38] = scheme;
}

// FUNCTION: WIZ8 0x00558810
void ResetButtonSoundScheme(void)
{
    g_button_sound_override_62a518 = -1;
}
