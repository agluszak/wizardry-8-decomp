#include "wiz8/game_state.h"

#include <stdlib.h>
#include <string.h>

extern "C" {

extern W8GameSettings g_settings_6850c8;

unsigned char g_message_box_state_6e1240[1600];
unsigned int g_message_box_runtime_650ea4;
unsigned char g_flag_5ff7ca;
extern short g_word_5ff7c8;

int g_region_help_delay;
int g_region_help_clock;
void* g_region_help_text;
unsigned int g_active_region_index;
unsigned int g_forced_region_index;
unsigned int g_dword_689b50;
void* g_default_help_text;

/* The buffer and selector state are independent of the four STI button
   objects.  Those source-backed SGP video objects are part of wiz8-xb9. */
void InitializeMessageBoxState(void)
{
    memset(g_message_box_state_6e1240, 0, sizeof(g_message_box_state_6e1240));
    g_message_box_runtime_650ea4 = 0;
}

// FUNCTION: WIZ8 0x004F11D0
void InitializeRegionHelpState(void)
{
    g_region_help_delay = g_settings_6850c8.field_025;
    g_region_help_clock = 0;
    g_active_region_index = 0;
    g_forced_region_index = 0;
    g_dword_689b50 = 0;
    if (g_default_help_text) {
        free(g_default_help_text);
    }
    g_default_help_text = 0;
}

// FUNCTION: WIZ8 0x0040C200
void SetMessageBoxModeEnabled(void)
{
    g_flag_5ff7ca = 1;
}

// FUNCTION: WIZ8 0x0040C210
void SetMessageBoxModeDisabled(void)
{
    g_flag_5ff7ca = 0;
}

// FUNCTION: WIZ8 0x0040C1F0
void SetMessageBoxWord(unsigned short value)
{
    g_word_5ff7c8 = static_cast<short>(value);
}

}
