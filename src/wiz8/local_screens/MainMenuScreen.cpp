#include "wiz8/regions.h"
#include "wiz8/cursor.h"
#include "wiz8/screen_state.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/music_playlist.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/game_status.h"
#include "wiz8/sgp_video.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_base.h"
#include "wiz8/utility.h"
#include "wiz8/version.h"

#include "input.h"
#include "english.h"
#include "Font.h"
#include "himage.h"
#include "sgp.h"
#include "vsurface.h"

#include <wchar.h>

unsigned char SaveGameExists(void);
void ResetRegions(void);
void ShutdownDisplayList(void);
void SetPendingScreenState(int state);
void RequestScreenTransition(void);
void SetValue64D8AC(unsigned long value);

/*
 * Local Screens\MainMenuScreen.cpp.
 *
 * The unit is named by the assertion this body embeds at line 135. Nothing here
 * is named beyond that: the callees and globals carry address-derived names
 * because no evidence assigns them meaning yet, and inventing one would be a
 * guess dressed as a recovery.
 */

extern "C" {

extern unsigned char g_flag_68510e;
extern unsigned char g_flag_689b32;


/* The screen's own state. */
unsigned char g_flag_69c4ba;
unsigned char g_flag_69c4b6;
unsigned short g_selected_item_0069c4b4;
unsigned char g_flag_69c4c4;
unsigned char g_flag_69c4bb;
int g_dword_69c4ac;
unsigned int g_dword_69c4b0;
int g_dword_69c4bc;
W8ModalDialogBase* g_dword_69c4c0;
extern int g_dword_647bc0;
extern int g_font_683660;
extern unsigned short* g_font_state_palettes_68ee1c[15];
extern unsigned short* g_colour_68ee08;
extern unsigned short gfAltState;
extern unsigned short gfCtrlState;
extern unsigned short gfShiftState;
extern void Function422B10(void);
extern void Function40B290(void);
extern unsigned char ClearPrimarySurface(void);
extern void SetViewport(int left, int top, int right, int bottom);
extern void UpdateHeldItemCursor(void);
extern unsigned char Function4298F0(void);
unsigned char Function5BCAB0(short item, short state);
extern void ReleaseLoadedVideoFrames(void);
extern unsigned char SetValue5FF5F0(int font);
extern void Function406DC0(int font, unsigned short* palette);
extern unsigned int Function4F1360(int x, int y);
extern unsigned char Function5A1140(const InputAtom* input);
extern void Function518B30(void);
extern void Function5189B0(void);
extern void Function422F10(void);
extern void Function4229C0(void);
extern void NoOp(void);
extern "C" unsigned char Function402ED0(
    int destination, unsigned int source, short region,
    int x, int y, int flags, int effects);

static void MainMenuRegionEvent(
    short item, const W8RegionEvent* event, W8Region* region)
{
    if (event->reason == MOUSE_POS) {
        if (region->flags & 0x20) {
            Function5BCAB0(item, 0);
            if (g_selected_item_0069c4b4 == item) {
                g_selected_item_0069c4b4 = (unsigned short)-1;
            }
        }
        if (region->flags & 0x10) {
            if (g_selected_item_0069c4b4 < 6) {
                Function5BCAB0(g_selected_item_0069c4b4, 0);
            }
            g_selected_item_0069c4b4 = item;
            Function5BCAB0(item, 1);
        }
        return;
    }
    if (event->reason == LEFT_BUTTON_DOWN) {
        region->flags |= 0x40;
        Function5BCAB0(item, 2);
        return;
    }
    if (event->reason != LEFT_BUTTON_UP) {
        return;
    }

    Function5BCAB0(item, 1);
    if ((region->flags & 0x40) == 0) {
        return;
    }
    region->flags &= ~0x40u;
    switch (item) {
    case 0:
        RequestScreenTransition();
        g_flag_68510e = 0;
        SetValue64D8AC(0);
        SetPendingScreenState(0);
        break;
    case 1:
        SetPendingScreenState(5);
        break;
    case 2:
        if (g_flag_69c4ba) {
            g_dword_68ed10.mode = 1;
            SetPendingScreenState(10);
        }
        break;
    case 3:
        SetPendingScreenState(9);
        break;
    case 4:
        g_dword_68ed10.mode = 0;
        SetPendingScreenState(10);
        break;
    case 5:
        gfProgramIsRunning = 0;
        break;
    }
}

static void MainMenuIntroduction(
    const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(0, event, region); }
static void MainMenuNewGame(const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(1, event, region); }
static void MainMenuLoadGame(const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(2, event, region); }
static void MainMenuCredits(const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(3, event, region); }
static void MainMenuOptions(const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(4, event, region); }
static void MainMenuExit(const W8RegionEvent* event, W8Region* region)
{ MainMenuRegionEvent(5, event, region); }

static void InitializeMainMenuRegions(void)
{
    static const short bounds[6][4] = {
        { 174, 138, 467, 182 },
        { 140, 187, 501, 231 },
        { 204, 235, 436, 279 },
        { 239, 284, 403, 328 },
        { 234, 335, 408, 379 },
        { 279, 423, 364, 467 }
    };
    static W8RegionCallback callbacks[6] = {
        MainMenuIntroduction, MainMenuNewGame, MainMenuLoadGame,
        MainMenuCredits, MainMenuOptions, MainMenuExit
    };

    g_region_sets[1].first_region = 0;
    g_region_sets[1].last_region = 6;
    for (int item = 0; item != 6; ++item) {
        W8Region* region = &g_regions[item + 1];
        region->flags = 1;
        region->x1 = bounds[item][0];
        region->y1 = bounds[item][1];
        region->x2 = bounds[item][2];
        region->y2 = bounds[item][3];
        region->callback = callbacks[item];
        region->callback_id = 0;
        region->help_enabled = 0;
        region->unknown_13 = 0;
        region->help_text_id = -1;
        region->owner = 0;
    }
}

static unsigned int MainMenuRegionAt(unsigned short x, unsigned short y)
{
    if (!g_region_sets[1].enabled) {
        return 0;
    }
    for (unsigned int region = 1; region <= 6; ++region) {
        if (RegionContainsPoint(region, x, y)) {
            return region;
        }
    }
    return 0;
}

static void UpdateMainMenuHover(unsigned short x, unsigned short y)
{
    unsigned int next = MainMenuRegionAt(x, y);
    W8RegionEvent event;

    if (next == g_hot_region_689b4c) {
        return;
    }
    event.time = GetTickCount();
    event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.reason = MOUSE_POS;
    if (g_hot_region_689b4c != 0) {
        W8Region* previous = &g_regions[g_hot_region_689b4c];
        previous->flags |= 0x20;
        previous->callback(&event, previous);
        previous->flags &= ~0x30u;
    }
    g_hot_region_689b3c = next;
    g_hot_region_689b4c = next;
    if (next != 0) {
        W8Region* current = &g_regions[next];
        current->flags |= 0x10;
        current->callback(&event, current);
        current->flags &= ~0x30u;
    }
}

static void SelectMainMenuItem(unsigned short item)
{
    if (g_selected_item_0069c4b4 < 6) {
        Function5BCAB0(g_selected_item_0069c4b4, 0);
    }
    g_selected_item_0069c4b4 = item;
    Function5BCAB0(item, 1);
}

static void ActivateMainMenuItem(unsigned short item)
{
    W8RegionEvent event;
    W8Region* region = &g_regions[item + 1];

    event.time = GetTickCount();
    event.modifiers = gfAltState | gfCtrlState | gfShiftState;
    event.reason = LEFT_BUTTON_DOWN;
    region->callback(&event, region);
    event.reason = LEFT_BUTTON_UP;
    region->callback(&event, region);
}

static void ProcessMainMenuInput(void)
{
    InputAtom input;
    POINT mouse;

    /* The released SGP hook reports screen coordinates.  Retail's per-frame
       cursor update converts them into the 640x480 client before region
       dispatch; that conversion is observable in windowed Wine too. */
    GetCursorPos(&mouse);
    ScreenToClient(ghWindow, &mouse);
    UpdateMainMenuHover(
        static_cast<unsigned short>(mouse.x),
        static_cast<unsigned short>(mouse.y));
    while (DequeueEvent(&input)) {
        if (input.usEvent == MOUSE_POS) {
            GetCursorPos(&mouse);
            ScreenToClient(ghWindow, &mouse);
            UpdateMainMenuHover(
                static_cast<unsigned short>(mouse.x),
                static_cast<unsigned short>(mouse.y));
        } else if ((input.usEvent == LEFT_BUTTON_DOWN
                    || input.usEvent == LEFT_BUTTON_UP)
                   && g_hot_region_689b4c != 0) {
            W8Region* region = &g_regions[g_hot_region_689b4c];
            region->callback((const W8RegionEvent*)&input, region);
        } else if (input.usEvent == KEY_DOWN || input.usEvent == KEY_REPEAT) {
            if (input.usParam == UPARROW) {
                SelectMainMenuItem(
                    g_selected_item_0069c4b4 == 0 ? 5 : g_selected_item_0069c4b4 - 1);
            } else if (input.usParam == DNARROW) {
                SelectMainMenuItem(
                    g_selected_item_0069c4b4 >= 5 ? 0 : g_selected_item_0069c4b4 + 1);
            } else if (input.usParam == HOME) {
                SelectMainMenuItem(0);
            } else if (input.usParam == KEY_END) {
                SelectMainMenuItem(5);
            } else if (input.usParam == ENTER && g_selected_item_0069c4b4 < 6) {
                ActivateMainMenuItem(g_selected_item_0069c4b4);
            }
        }
    }
}

/* Draws one of the six menu items. The first switch turns the item index into
   its sprite slot and its top and bottom rows; the second turns the requested
   state into a sprite id. Item two is forced to state three whenever the flag
   0x005BC810 stores from 0x00512FB0 is clear, which is the only item whose
   state the screen overrides.

   The sprite call is written in every case rather than assigning the id and
   calling once: the original pushes each id as a literal and lets VC6 cross-jump
   the four identical calls together. An unrecognised state draws no sprite but
   still redraws the row. */
// FUNCTION: WIZ8 0x005bcab0
unsigned char Function5BCAB0(short item, short state)
{
    int slot;
    int top;
    int bottom;
    int sprite;

    switch (item) {
    case 0: slot = 0; top = 0x8a;  bottom = 0xb1;  break;
    case 1: slot = 1; top = 0xbb;  bottom = 0xe1;  break;
    case 2:
        slot = 2;
        top = 0xeb;
        bottom = 0x112;
        if (!g_flag_69c4ba) {
            state = 3;
        }
        break;
    case 3: slot = 3; top = 0x11c; bottom = 0x145; break;
    case 4: slot = 4; top = 0x14f; bottom = 0x193; break;
    case 5: slot = 5; top = 0x1a7; bottom = 0x1d3; break;
    default:
        return 0;
    }

    switch (state) {
    case 0: Function548F90(-14, 0xea, 0, slot, 0x98, top, 2, 0); break;
    case 1: Function548F90(-14, 0xec, 0, slot, 0x98, top, 2, 0); break;
    case 2: Function548F90(-14, 0xeb, 0, slot, 0x98, top, 2, 0); break;
    case 3: Function548F90(-14, 0xed, 0, slot, 0x98, top, 2, 0); break;
    }

    MarkScreenRectDirty(0x98, top, 0x1f2, bottom, 0);
    return 1;
}

// FUNCTION: WIZ8 0x005bc810
unsigned char MainMenuScreenFunction005BC810(void)
{
    char text[64];
    wchar_t wide[64];
    unsigned short colour;
    W8ModalDialogBase* dialog;
    int pending;
    short measured;

    Function422B10();
    Function40B290();
    g_status_685170.game_started = 0;
    g_flag_69c4ba = SaveGameExists();
    g_flag_69c4b6 = 1;
    ClearPrimarySurface();
    colour = Get16BPPColor(0x10101);
    ColorFillVideoSurfaceArea(-14, 0, 0, 0x280, 0x1e0, colour);
    SetViewport(0, 0, 0x280, 0x1e0);
    g_selected_item_0069c4b4 = 0;
    Function548F90(-14, 0xe8, 0, 0, 0, 0, 2, 0);

    /* Six items cleared then the selected one set, written out rather than
       looped: the original repeats the call with a literal index each time. */
    Function5BCAB0(0, 0);
    Function5BCAB0(1, 0);
    Function5BCAB0(2, 0);
    Function5BCAB0(3, 0);
    Function5BCAB0(4, 0);
    Function5BCAB0(5, 0);
    Function5BCAB0(g_selected_item_0069c4b4, 1);

    Function4E3620(text, 0, 0, 0);
    wcscpy(wide, ConvertStringToWide(text));
    SetFont(g_font_683660);
    SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[8]);
    measured = StringPixLength((unsigned short*)wide, g_font_683660);
    mprintf(0x27b - measured, 5, (unsigned short*)wide);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    ResetRegions();
    InitializeMainMenuRegions();
    RegionSetEnable(1);

    if (gXStatus.uiMonstersInDatabase > 1000) {
        srAssertFail(
            "gXStatus.uiMonstersInDatabase <= MAX_MONSTERS_IN_DATABASE",
            "C:\\Projects\\Wizardry 8\\Local Screens\\MainMenuScreen.cpp",
            0x87,
            0);
    }
    if (g_dword_647bc0 != 10) {
        Function48FC10("MainMenu.MPL", 0, 1);
    }
    UpdateHeldItemCursor();

    pending = g_dword_69c4bc;
    if (pending != 0) {
        dialog = static_cast<W8ModalDialogBase*>(Function5CF300(1));
        dialog->SetClientExtent(0xfa, 200);
        dialog->SetMessage((void*)pending, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        Function5CF580(dialog, 0);
        g_dword_69c4c0 = dialog;
        ::operator delete((void*)g_dword_69c4bc);
        g_dword_69c4bc = 0;
        return 1;
    }
    if (!Function4298F0() && !g_flag_69c4c4) {
        int message = *(int*)&gppStringList[0x1fb8 / 4];

        dialog = static_cast<W8ModalDialogBase*>(Function5CF300(1));
        dialog->SetClientExtent(0xfa, 200);
        dialog->SetMessage((void*)message, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        Function5CF580(dialog, 0);
        g_flag_69c4c4 = 1;
        g_dword_69c4c0 = dialog;
    }
    return 1;
}

/* The canonical state-1 frame services a modal dialog first.  Without one it
   dispatches queued region and keyboard input, including the direct New Game,
   Load, Options and exit shortcuts, then completes the shared 2D redraw and
   renderer transaction. */
// FUNCTION: WIZ8 0x005bcbf0
void MainMenuScreenFrame()
{
    W8ScreenPoint point;
    InputAtom input;

    if (g_flag_689b32 != 0) {
        SetPendingScreenState(12);
    }
    if (g_dword_69c4c0 != 0) {
        Function5CF520(g_dword_69c4c0);
        if (Function5CF550(g_dword_69c4c0) == 0) {
            delete g_dword_69c4c0;
            g_dword_69c4c0 = 0;
            g_flag_69c4b6 = 1;
            Function548F90(-14, 0xe8, 0, 0, 0, 0, 2, 0);
            Function5BCAB0(0, 0);
            Function5BCAB0(1, 0);
            Function5BCAB0(2, 0);
            Function5BCAB0(3, 0);
            Function5BCAB0(4, 0);
            Function5BCAB0(5, 0);
            Function5BCAB0(g_selected_item_0069c4b4, 1);
        }
    }
    else if (IsStringTableLoaded()) {
        Function518B30();
    }
    else {
        GetScreenPoint004284F0(&point);
        g_dword_69c4b0 = Function4F1360(point.x, point.y);
        while (DequeueEvent(&input) == 1) {
            if (!DispatchScreenInput004F1910(&input) &&
                input.usEvent == KEY_DOWN) {
                if (Function5A1140(&input)) {
                    if (g_flag_689b32 != 0) {
                        SetValue5FF5F0(g_font_683660);
                        Function406DC0(g_font_683660, g_colour_68ee08);
                        mprintf(5, 5, (unsigned short*)L"Developer mode enabled.");
                    }
                }
                else {
                    switch (input.usParam) {
                    case ENTER:
                        switch (g_selected_item_0069c4b4) {
                        case 0:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            RequestScreenTransition();
                            g_flag_68510e = 0;
                            SetValue64D8AC(0);
                            SetPendingScreenState(0);
                            break;
                        case 1:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            SetPendingScreenState(5);
                            break;
                        case 2:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            if (g_flag_69c4ba != 0) {
                                g_dword_68ed10.mode = 1;
                                SetPendingScreenState(10);
                            }
                            break;
                        case 3:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            SetPendingScreenState(9);
                            break;
                        case 4:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            SetPendingScreenState(10);
                            break;
                        case 5:
                            Function5BCAB0(g_selected_item_0069c4b4, 2);
                            SetPendingScreenState(12);
                            break;
                        }
                        break;
                    case ESC:
                    case 'E':
                    case 'X':
                        SetPendingScreenState(12);
                        break;
                    case HOME:
                        Function5BCAB0(g_selected_item_0069c4b4, 0);
                        g_selected_item_0069c4b4 = 0;
                        Function5BCAB0(0, 1);
                        break;
                    case KEY_END:
                        Function5BCAB0(g_selected_item_0069c4b4, 0);
                        g_selected_item_0069c4b4 = 5;
                        Function5BCAB0(5, 1);
                        break;
                    case UPARROW:
                        Function5BCAB0(g_selected_item_0069c4b4, 0);
                        if (g_selected_item_0069c4b4 > 0) {
                            --g_selected_item_0069c4b4;
                        }
                        else {
                            g_selected_item_0069c4b4 = 5;
                        }
                        Function5BCAB0(g_selected_item_0069c4b4, 1);
                        break;
                    case DNARROW:
                        Function5BCAB0(g_selected_item_0069c4b4, 0);
                        if (g_selected_item_0069c4b4 < 5) {
                            ++g_selected_item_0069c4b4;
                        }
                        else {
                            g_selected_item_0069c4b4 = 0;
                        }
                        Function5BCAB0(g_selected_item_0069c4b4, 1);
                        break;
                    case 'L':
                        if (g_flag_69c4ba != 0) {
                            g_dword_68ed10.mode = 1;
                            SetPendingScreenState(10);
                        }
                        break;
                    case 'O':
                        SetPendingScreenState(10);
                        break;
                    case 'S':
                        SetPendingScreenState(5);
                        break;
                    }
                }
            }
        }
    }

    NoOp();
    if (g_flag_69c4b6 != 0 || IsStringTableLoaded()) {
        if (g_dword_69c4c0 != 0) {
            Function5CF520(g_dword_69c4c0);
        }
        if (g_flag_69c4bb != 0) {
            Function402ED0(-14, g_dword_69c4ac, 0, 0, 0x1d1, 6, 0);
        }
        Function5189B0();
        Function422F10();
        g_flag_69c4b6 = 0;
    }
    Function4229C0();
}

// FUNCTION: WIZ8 0x00591870
unsigned char MainMenuScreenLeave(int)
{
    ReleaseLoadedVideoFrames();
    ResetRegions();
    ShutdownDisplayList();
    return 1;
}

}
