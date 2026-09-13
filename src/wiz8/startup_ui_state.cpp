#include "wiz8/local_code/Configuration.h"
#include "wiz8/regions.h"

#include <stdlib.h>
#include <string.h>

/* Unresolved fragment: 0x004F11D0 lies in the anchored gap between
   Noise.cpp (0x004F0E80) and RegionManager.cpp (0x004F2040); the two globals
   sit in the unbracketed .data tail. No original-TU ownership is proven. */

// GLOBAL: WIZ8 0x00689b48
int g_region_help_delay;
// GLOBAL: WIZ8 0x00689b4c
int g_region_help_clock;

// FUNCTION: WIZ8 0x004f11d0
void InitializeRegionHelpState(void)
{
    g_region_help_delay = g_settings_6850c8.tooltip_delay_ms;
    g_region_help_clock = 0;
    g_current_region_index = 0;
    g_captured_region_index = 0;
    g_hover_region_index = 0;
    g_region_help_force_enabled = 0;
    if (g_default_help_text) {
        delete[] g_default_help_text;
    }
    g_default_help_text = 0;
}
