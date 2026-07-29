#include "wiz8/sr_api.h"

/* Local Code\ButtonSound.cpp's fixed scheme stack. The assertion supplies the
   original top name and the SCHEME_STACK_SIZE spelling; the body establishes
   the 32-entry capacity and pre-increment push convention. */
extern "C" {
int g_button_sound_scheme_stack_62a498[32];
int g_value_62a518;
int g_button_sound_scheme_stack_top_68de38;
}

// FUNCTION: WIZ8 0x005587c0
void PushButtonSoundScheme005587C0(int scheme, char replace_current)
{
    if (replace_current != 0) {
        g_value_62a518 = scheme;
        return;
    }
    if (g_button_sound_scheme_stack_top_68de38 >= 32) {
        srAssertFail("giSchemeStackTop < SCHEME_STACK_SIZE",
                     "C:\\Projects\\Wizardry 8\\Local Code\\ButtonSound.cpp", 0x5d, 0);
    }
    ++g_button_sound_scheme_stack_top_68de38;
    g_button_sound_scheme_stack_62a498[g_button_sound_scheme_stack_top_68de38] = scheme;
}
