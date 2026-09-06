#include "wiz8/local_code/Configuration.h"
#include "wiz8/regions.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

unsigned char g_message_box_state_6e1240[1600];
unsigned int g_message_box_runtime_650ea4;
unsigned char g_flag_5ff7ca;
extern short g_word_5ff7c8;

int g_region_help_delay;
int g_region_help_clock;

/* The buffer and selector state are independent of the four STI button
   objects.  Those source-backed SGP video objects are part of wiz8-xb9. */
void InitializeMessageBoxState(void)
{
    memset(g_message_box_state_6e1240, 0, sizeof(g_message_box_state_6e1240));
    g_message_box_runtime_650ea4 = 0;
}

// FUNCTION: WIZ8 0x004f11d0
void InitializeRegionHelpState(void)
{
    g_region_help_delay = g_settings_6850c8.field_025;
    g_region_help_clock = 0;
    g_hot_region_689b3c = 0;
    g_hot_region_689b44 = 0;
    g_hot_region_689b4c = 0;
    g_dword_689b50 = 0;
    if (g_default_help_text) {
        delete[] g_default_help_text;
    }
    g_default_help_text = 0;
}

// FUNCTION: WIZ8 0x0040c200
void SetMessageBoxModeEnabled(void)
{
    g_flag_5ff7ca = 1;
}

// FUNCTION: WIZ8 0x0040c210
void SetMessageBoxModeDisabled(void)
{
    g_flag_5ff7ca = 0;
}

// FUNCTION: WIZ8 0x0040c1f0
void SetMessageBoxWord(unsigned short value)
{
    g_word_5ff7c8 = static_cast<short>(value);
}

}
