#include "wiz8/regions.h"
#include "wiz8/video_object_catalog.h"

#include "himage.h"
#include "input.h"
#include "vsurface.h"

/* Lifecycle record 12. The original screen and translation-unit names are
   unknown; the existing compilation boundary is retained. */

extern "C" int g_value_64c1c8;

void ResetRegions(void);
unsigned char ClearFlag603C60(void);

extern "C" {

extern void NoOp(void);
extern void MSYS_Shutdown(void);
extern unsigned char ClearPrimarySurface(void);
extern void UpdateHeldItemCursor(void);
/* 0x00422B10 clears the software frame and retires the transient 2D overlays;
   0x00422F10 is the scene-side teardown the same frames are torn down through.
   Only the first is recovered. */
extern void Function422B10(void);
extern void Function422F10(void);
extern void Function426790(void);
}

// FUNCTION: WIZ8 0x00593320
int GetValue64C1C8(void)
{
    return g_value_64c1c8;
}

// GLOBAL: WIZ8 0x006F0628
unsigned char g_flag_6f0628;
// GLOBAL: WIZ8 0x006F04E8
unsigned char g_flag_6f04e8;
// GLOBAL: WIZ8 0x006F04ED
unsigned char g_flag_6f04ed;

/* Lifecycle record 12's entry handler. It paints the whole 640x480 frame in the
   near-black 0x010101 and puts one video-object frame over it, which is the
   shape record 1's much larger main-menu entry starts with too. */
// FUNCTION: WIZ8 0x00591790
unsigned char Screen12Enter(void)
{
    unsigned short colour;

    Function422B10();
    UpdateHeldItemCursor();
    colour = Get16BPPColor(0x10101);
    ColorFillVideoSurfaceArea(-14, 0, 0, 0x280, 0x1e0, colour);
    Function548F90(-14, 0x1e4, 0, 0, 0, 0, 2, 0);
    Function422F10();
    return 1;
}

/* Lifecycle record 12's frame close-out. It drains the input queue through the
   region manager and lets a key press that the regions did not consume clear
   0x006F0628; the screen then tears down unless that flag is still set and
   neither of the two other flags is. The two trailing repeats of 0x00426790 are
   the original's own. */
// FUNCTION: WIZ8 0x005917e0
void Screen12Finish(void)
{
    InputAtom input;

    Function426790();
    while (DequeueEvent(&input) == 1) {
        if (!DispatchScreenInput004F1910(&input)) {
            switch (input.usEvent) {
            case KEY_DOWN:
                g_flag_6f0628 = 0;
                break;
            }
        }
    }
    if (g_flag_6f04ed || g_flag_6f04e8) {
        g_flag_6f0628 = 0;
    }
    else if (g_flag_6f0628) {
        return;
    }
    ClearFlag603C60();
    ClearPrimarySurface();
    Function422F10();
    Function426790();
    Function426790();
}
