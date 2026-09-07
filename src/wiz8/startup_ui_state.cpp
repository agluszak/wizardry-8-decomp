#include "wiz8/local_code/Configuration.h"
#include "wiz8/regions.h"

#include <stdlib.h>
#include <string.h>

int g_region_help_delay;
int g_region_help_clock;

// FUNCTION: WIZ8 0x004f11d0
void InitializeRegionHelpState(void)
{
    g_region_help_delay = g_settings_6850c8.field_025;
    g_region_help_clock = 0;
    g_current_region_index = 0;
    g_captured_region_index = 0;
    g_hover_region_index = 0;
    g_dword_689b50 = 0;
    if (g_default_help_text) {
        delete[] g_default_help_text;
    }
    g_default_help_text = 0;
}
