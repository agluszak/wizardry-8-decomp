#include "wiz8/local_code/Configuration.h"
#include "wiz8/regions.h"

#include <stdlib.h>
#include <string.h>

/* Original translation-unit ownership is unknown; surrounding anchors do not resolve it. */

// GLOBAL: WIZ8 0x00689b48
int g_region_help_delay;
// GLOBAL: WIZ8 0x00689b38
TIMER g_region_help_clock;

// FUNCTION: WIZ8 0x004f11d0
unsigned char InitializeRegionHelpState(void)
{
    g_region_help_delay = g_settings.tooltip_delay_ms;
    g_current_region_index = 0;
    g_captured_region_index = 0;
    g_hover_region_index = 0;
    g_region_help_force_enabled = false;
    if (g_default_help_text) {
        delete[] g_default_help_text;
    }
    g_default_help_text = 0;
    return 1;
}
