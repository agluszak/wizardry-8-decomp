#include "wiz8/screen_state.h"

#include "Types.h"
#include "mousesystem.h"
#include "timer.h"

extern "C" {
extern BOOLEAN gfShowFastHelp;
extern MOUSE_REGION* MSYS_CurrRegion;
}
extern void SetHelpBoxText(void* text);
extern void PlaceHelpBox(int x, int y);
extern int g_help_box_width;
extern int g_help_box_height;

/* Retail 0x00650E68: Wizardry's fast-help clock, written on every frame the
   mouse system runs. */
// GLOBAL: WIZ8 0x00650E68
int g_fast_help_last_clock_650e68;
/* Retail 0x005FF7CA: the fast-help enable flag retail's
   Enable/DisableMouseFastHelp maintain. */
unsigned char g_flag_5ff7ca;

// FUNCTION: WIZ8 0x0040c0b0
void RenderFastHelp(void)
{
    int current_clock;
    int elapsed;
    int x;
    int y;

    if (!gfShowFastHelp) {
        return;
    }
    current_clock = GetClock();
    elapsed = current_clock - g_fast_help_last_clock_650e68;
    if (elapsed < 0) {
        elapsed += 0x7fffffff;
    }
    g_fast_help_last_clock_650e68 = current_clock;

    if (MSYS_CurrRegion == 0 || MSYS_CurrRegion->FastHelpText == 0 ||
        !g_flag_5ff7ca) {
        return;
    }
    if (MSYS_CurrRegion->FastHelpTimer == 0) {
        if ((MSYS_CurrRegion->uiFlags &
             (MSYS_ALLOW_DISABLED_FASTHELP | MSYS_REGION_ENABLED)) == 0) {
            return;
        }
        if ((MSYS_CurrRegion->uiFlags & MSYS_MOUSE_IN_AREA) == 0) {
            MSYS_CurrRegion->uiFlags &=
                ~(MSYS_FASTHELP | MSYS_FASTHELP_RESET);
            ReleaseScreenTransitionObjects();
            return;
        }
        MSYS_CurrRegion->uiFlags |= MSYS_FASTHELP;
        SetHelpBoxText(MSYS_CurrRegion->FastHelpText);
        x = MSYS_CurrRegion->RegionTopLeftX + 10;
        if (x < 0) x = 0;
        if (x + g_help_box_width >= 640) x = 636 - g_help_box_width;
        y = MSYS_CurrRegion->RegionTopLeftY - g_help_box_height * 3 / 4;
        if (y < 0) y = 0;
        if (y + g_help_box_height >= 480) y = 465 - g_help_box_height;
        PlaceHelpBox(x, y);
        return;
    }
    if ((MSYS_CurrRegion->uiFlags &
         (MSYS_ALLOW_DISABLED_FASTHELP | MSYS_REGION_ENABLED)) != 0 &&
        (MSYS_CurrRegion->uiFlags & MSYS_MOUSE_IN_AREA) != 0 &&
        MSYS_CurrRegion->ButtonState == 0) {
        if (elapsed > 0) {
            MSYS_CurrRegion->FastHelpTimer -= (short)elapsed;
        }
        if (MSYS_CurrRegion->FastHelpTimer < 0) {
            MSYS_CurrRegion->FastHelpTimer = 0;
        }
    }
}
