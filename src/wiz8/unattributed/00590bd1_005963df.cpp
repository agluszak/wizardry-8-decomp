#include "wiz8/unattributed/quarantine_common.h"

#include "himage.h"
#include "input.h"

#include <stdlib.h>

/* Address quarantine 00590bd1-005963df; bounds come from adjacent
   assertion-backed original translation-unit intervals.

   The declarations below are this unit's own rather than quarantine_common.h's:
   no other unit in the interval scheme reaches them, and that header is a
   mechanism the repository drains rather than grows. The linkage split is not
   cosmetic - it follows each definition site, and getting it wrong makes the
   reference resolve to the image base under /FORCE. */

void ResetRegions(void);
unsigned char SetFlag603C60(void);
unsigned char ClearFlag603C60(void);
void InitializeFactState(void);

extern "C" {

extern void NoOp(void);
extern void ShutdownDisplayList(void);
extern unsigned char ClearPrimarySurface(void);
extern void UpdateHeldItemCursor(void);
extern int Function40B290(void);
extern void Function548F90(int target, int object, int frame, short y,
                           int a5, int a6, int a7, int a8);
extern unsigned char FillSurfaceRect(int surface_id, int left, int top, int right,
                                     int bottom, int colour);
extern void ConfigurePresentation00413FD0(int a, int b, int c, int d, int e);
extern void SetRenderClip00407220(int target, int left, int top, int right,
                                  int bottom, int flags);
/* 0x00422B10 clears the software frame and retires the transient 2D overlays;
   0x00422F10 is the scene-side teardown the same frames are torn down through.
   Only the first is recovered. */
extern void Function422B10(void);
extern void Function422F10(void);
extern void Function425570(int value);
extern void Function426790(void);
extern int Function509750(void);
extern void Function58FD30(void);
/* 0x004F1910, the region manager's own dispatch for one dequeued input atom. It
   reports whether it consumed the atom, so a screen loop can fall through to its
   own key handling. IntroScreen.cpp owns this spelling. */
extern unsigned char DispatchScreenInput004F1910(const void* event);

extern int g_dword_686a70;

}

// FUNCTION: WIZ8 0x00593320
int GetValue64C1C8(void)
{
    return g_value_64c1c8;
}

/* The loading screen's descriptor. Lifecycle record 4's entry handler mallocs
   0x148 bytes, clears them, and fills the tail from the screen-state record it
   was entered with; the record's tick releases it. The leading 0xF0 bytes are
   untouched by either body, so nothing here names them. */
struct W8LevelLoadDescriptor {
    unsigned char m_positional_000[0xf0];
    int mode;                      /* 0x0f0, the screen state's own mode */
    int parameter;                 /* 0x0f4 */
    int parameter_2;               /* 0x0f8 */
    unsigned char unknown_0fc;     /* 0x0fc */
    char name[0x3f];               /* 0x0fd, bounded only by the next field */
    int parameter_3;               /* 0x13c */
    unsigned long entered_tick;    /* 0x140 */
    int unknown_144;               /* 0x144 */
};

/* 0x0069B7C8 is written by lifecycle record 4's entry handler at 0x00590DE0 and
   released by its tick, so the block is owned inside this interval. */
// GLOBAL: WIZ8 0x0069B7C8
W8LevelLoadDescriptor* g_load_descriptor_69b7c8;
// GLOBAL: WIZ8 0x0069B7C4
int g_value_69b7c4;
// GLOBAL: WIZ8 0x006F0628
unsigned char g_flag_6f0628;
// GLOBAL: WIZ8 0x006F04E8
unsigned char g_flag_6f04e8;
// GLOBAL: WIZ8 0x006F04ED
unsigned char g_flag_6f04ed;

/* Lifecycle record 4's entry handler - the loading screen. It builds its
   descriptor once and fills the tail from the screen-state record it was entered
   with, which is what establishes that record's leading fields: mode at 0x04,
   then three parameters, then a name at 0x18. Mode 0 is the new game - it clears
   the fact state and deletes the running save; modes 1 and 2 carry a name; mode 3
   carries two parameters and no name.

   The descriptor is reached through the global at every store rather than
   through a local, which is the original's own shape. */
// FUNCTION: WIZ8 0x00590de0
unsigned char LevelLoadScreenEnter(void)
{
    if (!g_load_descriptor_69b7c8) {
        g_load_descriptor_69b7c8 =
            (W8LevelLoadDescriptor*)malloc(sizeof(W8LevelLoadDescriptor));
        if (!g_load_descriptor_69b7c8) {
            return 0;
        }
        memset(g_load_descriptor_69b7c8, 0, sizeof(W8LevelLoadDescriptor));
        g_load_descriptor_69b7c8->mode = g_screen_state_0068ec78.state.mode;
        switch (g_load_descriptor_69b7c8->mode) {
        case 0:
            InitializeFactState();
            g_load_descriptor_69b7c8->parameter = Function509750();
            Function58FD30();
            DeleteFileA("Saves\\CurrentGame.SAV");
            break;
        case 1:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.state.parameter;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.state.name);
            break;
        case 2:
            g_load_descriptor_69b7c8->parameter = g_dword_686a70;
            strcpy(g_load_descriptor_69b7c8->name, g_screen_state_0068ec78.state.name);
            g_load_descriptor_69b7c8->parameter_3 = g_screen_state_0068ec78.state.parameter_3;
            break;
        case 3:
            g_load_descriptor_69b7c8->parameter = g_screen_state_0068ec78.state.parameter;
            g_load_descriptor_69b7c8->parameter_2 = g_screen_state_0068ec78.state.parameter_2;
            break;
        }
    }
    Function40B290();
    ResetRegions();
    ConfigurePresentation00413FD0(0x500, 0, 0, 0x280, 0x1e0);
    SetRenderClip00407220(-14, 0, 0, 0x280, 0x1e0, 0);
    Function425570(0);
    ClearFlag603C60();
    g_load_descriptor_69b7c8->unknown_144 = 0;
    g_load_descriptor_69b7c8->entered_tick = GetTickCount();
    g_value_69b7c4 = 0;
    return 1;
}

/* Lifecycle record 4's tick - the loading screen. The leaving pass releases the
   descriptor its entry handler allocated; the ordinary pass only tears the frame
   down. Both passes run the teardown, which is why the release sits under the
   flag rather than the whole body. */
// FUNCTION: WIZ8 0x00591560
unsigned char LevelLoadScreenTick(char leaving)
{
    if (leaving) {
        free(g_load_descriptor_69b7c8);
        g_load_descriptor_69b7c8 = 0;
    }
    NoOp();
    ShutdownDisplayList();
    ResetRegions();
    SetFlag603C60();
    return 1;
}

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
    FillSurfaceRect(-14, 0, 0, 0x280, 0x1e0, colour);
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
