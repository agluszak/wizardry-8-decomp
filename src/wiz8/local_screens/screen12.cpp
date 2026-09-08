#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/cursor.h"
#include "wiz8/input_hooks.h"
#include "wiz8/render_state.h"

#include "himage.h"
#include "input.h"
#include "vsurface.h"

/* Lifecycle record 12 is the exit screen selected by Main Menu's Exit row and
   its Escape/E/X shortcuts. The original translation-unit name is unknown, so
   the existing compilation boundary is retained. */

extern "C" int g_value_64c1c8;

void ResetRegions(void);
unsigned char ClearFlag603C60(void);

// FUNCTION: WIZ8 0x00591780
void RequestExitScreen(void)
{
    SetPendingScreenState(W8_SCREEN_EXIT);
}

// FUNCTION: WIZ8 0x00593320
int GetValue64C1C8(void)
{
    return g_value_64c1c8;
}

// GLOBAL: WIZ8 0x006F0628
unsigned char g_flag_6f0628;

/* Lifecycle record 12's entry handler. It paints the whole 640x480 frame in the
   near-black 0x010101 and puts one video-object frame over it, which is the
   shape record 1's much larger main-menu entry starts with too. */
// FUNCTION: WIZ8 0x00591790
unsigned char ExitScreenEnter(void)
{
    unsigned short colour;

    Function422B10();
    UpdateHeldItemCursor();
    colour = Get16BPPColor(0x10101);
    ColorFillVideoSurfaceArea(-14, 0, 0, 0x280, 0x1e0, colour);
    DrawCatalogImage(-14, 0x1e4, 0, 0, 0, 0, 2, 0);
    ResetTransientRenderScenes();
    return 1;
}

/* Lifecycle record 12's frame close-out. It drains the input queue through the
   region manager and lets a key press that the regions did not consume clear
   0x006F0628; the screen then tears down unless that flag is still set and
   neither mouse-button latch is. The two trailing repeats of 0x00426790 are
   the original's own. */
// FUNCTION: WIZ8 0x005917e0
void ExitScreenFrame(void)
{
    InputAtom input;

    RenderFrame();
    while (DequeueEvent(&input) == 1) {
        if (!DispatchRegionInput(&input)) {
            switch (input.usEvent) {
            case KEY_DOWN:
                g_flag_6f0628 = 0;
                break;
            }
        }
    }
    if (g_flag_6f04ed == 0 && g_flag_6f04e8 == 0) {
        if (g_flag_6f0628 != 0) {
            return;
        }
    }
    else {
        g_flag_6f0628 = 0;
    }
    ClearFlag603C60();
    ClearPrimarySurface();
    ResetTransientRenderScenes();
    RenderFrame();
    RenderFrame();
}
