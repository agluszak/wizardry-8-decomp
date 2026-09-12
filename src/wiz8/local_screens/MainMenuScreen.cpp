#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/render_state.h"
#include "wiz8/regions.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/cursor.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/music_playlist.h"
#include "wiz8/game_status.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/ModalDialogBase.h"
#include "wiz8/utility.h"
#include "wiz8/version.h"

#include "input.h"
#include "english.h"
#include "Font.h"
#include "himage.h"
#include "sgp.h"
#include "Types.h"
#include "mousesystem.h"
#include "vsurface.h"

#include <wchar.h>

/*
 * Local Screens\MainMenuScreen.cpp.
 *
 * The unit is named by the assertion in its entry handler. Its six static
 * regions share the global RegionManager catalog and screen-state dispatcher.
 */

extern unsigned char g_flag_689b32;

/* The screen's own state. */
// GLOBAL: WIZ8 0x0069c4ba
unsigned char g_main_menu_has_save_games;
// GLOBAL: WIZ8 0x0069c4b6
unsigned char g_main_menu_redraw;
// GLOBAL: WIZ8 0x0069c4b4
unsigned short g_main_menu_selected_item;
// GLOBAL: WIZ8 0x0069c4c4
unsigned char g_main_menu_warning_shown;
// GLOBAL: WIZ8 0x0069c4bb
unsigned char g_main_menu_overlay_enabled;
// GLOBAL: WIZ8 0x0069c4ac
unsigned int g_main_menu_overlay_surface;
// GLOBAL: WIZ8 0x0069c4b0
unsigned int g_main_menu_hover_region;
// GLOBAL: WIZ8 0x0069c4bc
wchar_t* g_pending_main_menu_message;
// GLOBAL: WIZ8 0x0069c4c0
W8ModalDialogBase* g_main_menu_dialog;

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
unsigned char DrawMainMenuItem(short item, short state)
{
    int slot;
    int top;
    int bottom;

    switch (item) {
    case 0:
        slot = 0;
        top = 0x8a;
        bottom = 0xb1;
        break;
    case 1:
        slot = 1;
        top = 0xbb;
        bottom = 0xe1;
        break;
    case 2:
        slot = 2;
        top = 0xeb;
        bottom = 0x112;
        if (!g_main_menu_has_save_games) {
            state = 3;
        }
        break;
    case 3:
        slot = 3;
        top = 0x11c;
        bottom = 0x145;
        break;
    case 4:
        slot = 4;
        top = 0x14f;
        bottom = 0x193;
        break;
    case 5:
        slot = 5;
        top = 0x1a7;
        bottom = 0x1d3;
        break;
    default:
        return 0;
    }

    switch (state) {
    case 0:
        DrawCatalogImage(-14, 0xea, 0, slot, 0x98, top, 2, 0);
        break;
    case 1:
        DrawCatalogImage(-14, 0xec, 0, slot, 0x98, top, 2, 0);
        break;
    case 2:
        DrawCatalogImage(-14, 0xeb, 0, slot, 0x98, top, 2, 0);
        break;
    case 3:
        DrawCatalogImage(-14, 0xed, 0, slot, 0x98, top, 2, 0);
        break;
    }

    InvalidateRegion(0x98, top, 0x1f2, bottom, 0);
    return 1;
}

/* Zero is the retail BSS state. The first screen synchronization replaces it
   with the default cursor and then records the normal -1 state. */

// FUNCTION: WIZ8 0x005bc800
unsigned char MainMenuScreenInitialize(void)
{
    g_main_menu_selected_item = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005bc810
unsigned char MainMenuScreenEnter(void)
{
    char text[64];
    wchar_t wide[64];
    unsigned short colour;
    W8ModalDialogBase* dialog;
    wchar_t* pending;
    short measured;

    Function422B10();
    MSYS_Init();
    g_status_685170.game_started = 0;
    g_main_menu_has_save_games = SaveGameExists();
    g_main_menu_redraw = 1;
    ClearPrimarySurface();
    colour = Get16BPPColor(0x10101);
    ColorFillVideoSurfaceArea(-14, 0, 0, 0x280, 0x1e0, colour);
    SetViewport(0, 0, 0x280, 0x1e0);
    g_main_menu_selected_item = 0;
    DrawCatalogImage(-14, 0xe8, 0, 0, 0, 0, 2, 0);

    /* Six items cleared then the selected one set, written out rather than
       looped: the original repeats the call with a literal index each time. */
    DrawMainMenuItem(0, 0);
    DrawMainMenuItem(1, 0);
    DrawMainMenuItem(2, 0);
    DrawMainMenuItem(3, 0);
    DrawMainMenuItem(4, 0);
    DrawMainMenuItem(5, 0);
    DrawMainMenuItem(g_main_menu_selected_item, 1);

    FormatVersionBanner004E3620(text, 0, 0, 0);
    wcscpy(wide, ConvertStringToWide(text));
    SetFont(g_font_683660);
    SetFontObjectPalette16BPP(g_font_683660, g_font_state_palettes_68ee1c[8]);
    measured = StringPixLength((unsigned short*)wide, g_font_683660);
    gprintf(0x27b - measured, 5, (unsigned short*)wide);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    ResetRegions();
    RegionSetEnable(1);

    if (gXStatus.uiMonstersInDatabase > 1000) {
        srAssertFail("gXStatus.uiMonstersInDatabase <= MAX_MONSTERS_IN_DATABASE",
                     "C:\\Projects\\Wizardry 8\\Local Screens\\MainMenuScreen.cpp", 0x87, 0);
    }
    if (g_previous_screen_id != 10) {
        StartMusicResource0048FC10("MainMenu.MPL", 0, 1);
    }
    UpdateHeldItemCursor();

    pending = g_pending_main_menu_message;
    if (pending != 0) {
        dialog = static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        dialog->SetMessage(pending, 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, 0);
        g_main_menu_dialog = dialog;
        delete[] g_pending_main_menu_message;
        g_pending_main_menu_message = 0;
        return 1;
    }
    if (!HasEnoughFreeDiskSpace() && !g_main_menu_warning_shown) {
        dialog = static_cast<W8ModalDialogBase*>(CreateDialogByKind(1));
        dialog->SetClientExtent(0xfa, 200);
        dialog->SetMessage(gppStringList[0x1fb8 / 4], 1, 0x32, 1, 0, 1, 1, 0, 0x15e);
        SetDialogDestroyCallback(dialog, 0);
        g_main_menu_warning_shown = 1;
        g_main_menu_dialog = dialog;
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
    POINT point;
    InputAtom input;

    if (g_flag_689b32 != 0) {
        RequestExitScreen();
    }
    if (g_main_menu_dialog != 0) {
        DrawDialog(g_main_menu_dialog);
        if (ProcessDialogInput(g_main_menu_dialog) == 0) {
            delete g_main_menu_dialog;
            g_main_menu_dialog = 0;
            g_main_menu_redraw = 1;
            DrawCatalogImage(-14, 0xe8, 0, 0, 0, 0, 2, 0);
            DrawMainMenuItem(0, 0);
            DrawMainMenuItem(1, 0);
            DrawMainMenuItem(2, 0);
            DrawMainMenuItem(3, 0);
            DrawMainMenuItem(4, 0);
            DrawMainMenuItem(5, 0);
            DrawMainMenuItem(g_main_menu_selected_item, 1);
        }
    } else if (IsMessageBoxActive()) {
        ProcessMessageBoxInput();
    } else {
        SGPMouseGetPos(&point);
        g_main_menu_hover_region = UpdateRegionMousePosition(point.x, point.y);
        while (DequeueEvent(&input) == 1) {
            if (!DispatchRegionInput(&input) && input.usEvent == KEY_DOWN) {
                if (Function5A1140(&input)) {
                    if (g_flag_689b32 != 0) {
                        SetFont(g_font_683660);
                        SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
                        gprintfDirty(5, 5, (unsigned short*)L"Developer mode enabled.");
                    }
                } else {
                    switch (input.usParam) {
                    case ENTER:
                        switch (g_main_menu_selected_item) {
                        case 0:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            RequestScreenTransition();
                            g_settings_6850c8.intro_seen = 0;
                            SetValue64D8AC(0);
                            SetPendingScreenState(W8_SCREEN_INTRO);
                            break;
                        case 1:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            SetPendingScreenState(W8_SCREEN_PARTY_SELECTION);
                            break;
                        case 2:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            if (g_main_menu_has_save_games != 0) {
                                g_pending_screen_state.mode = 1;
                                SetPendingScreenState(W8_SCREEN_OPTIONS);
                            }
                            break;
                        case 3:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            SetPendingScreenState(W8_SCREEN_CREDITS);
                            break;
                        case 4:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            SetPendingScreenState(W8_SCREEN_OPTIONS);
                            break;
                        case 5:
                            DrawMainMenuItem(g_main_menu_selected_item, 2);
                            RequestExitScreen();
                            break;
                        }
                        break;
                    case ESC:
                    case 'E':
                    case 'X':
                        RequestExitScreen();
                        break;
                    case VK_PRIOR:
                        DrawMainMenuItem(g_main_menu_selected_item, 0);
                        g_main_menu_selected_item = 0;
                        DrawMainMenuItem(0, 1);
                        break;
                    case VK_NEXT:
                        DrawMainMenuItem(g_main_menu_selected_item, 0);
                        g_main_menu_selected_item = 5;
                        DrawMainMenuItem(5, 1);
                        break;
                    case VK_UP:
                        DrawMainMenuItem(g_main_menu_selected_item, 0);
                        if (g_main_menu_selected_item > 0) {
                            --g_main_menu_selected_item;
                        } else {
                            g_main_menu_selected_item = 5;
                        }
                        DrawMainMenuItem(g_main_menu_selected_item, 1);
                        break;
                    case VK_DOWN:
                        DrawMainMenuItem(g_main_menu_selected_item, 0);
                        if (g_main_menu_selected_item < 5) {
                            ++g_main_menu_selected_item;
                        } else {
                            g_main_menu_selected_item = 0;
                        }
                        DrawMainMenuItem(g_main_menu_selected_item, 1);
                        break;
                    case 'L':
                        if (g_main_menu_has_save_games != 0) {
                            g_pending_screen_state.mode = 1;
                            SetPendingScreenState(W8_SCREEN_OPTIONS);
                        }
                        break;
                    case 'O':
                        SetPendingScreenState(W8_SCREEN_OPTIONS);
                        break;
                    case 'S':
                        SetPendingScreenState(W8_SCREEN_PARTY_SELECTION);
                        break;
                    }
                }
            }
        }
    }

    NoOp();
    if (g_main_menu_redraw != 0 || IsMessageBoxActive() || g_main_menu_dialog != 0) {
        if (g_main_menu_dialog != 0) {
            DrawDialog(g_main_menu_dialog);
        }
        if (g_main_menu_overlay_enabled != 0) {
            BltVideoSurface(-14, g_main_menu_overlay_surface, 0, 0, 0x1d1, 6, 0);
        }
        RenderMessageBox();
        ResetTransientRenderScenes();
        g_main_menu_redraw = 0;
    }
    RenderFrame();
}

// FUNCTION: WIZ8 0x00591870
unsigned char MainMenuScreenLeave(int)
{
    ReleaseLoadedVideoFrames();
    ResetRegions();
    MSYS_Shutdown();
    return 1;
}

// FUNCTION: WIZ8 0x005bd010
void SetMainMenuMessage(const wchar_t* message)
{
    g_pending_main_menu_message = new wchar_t[wcslen(message) + 1];
    wcscpy(g_pending_main_menu_message, message);
}

// FUNCTION: WIZ8 0x005bd040
unsigned char MainMenuNewGame(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            SetPendingScreenState(W8_SCREEN_PARTY_SELECTION);
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
            return 0;
        }
        if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 1;
            DrawMainMenuItem(1, 1);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005bd110
unsigned char MainMenuLoadGame(const W8RegionEvent* event, W8Region* region)
{
    if (!g_main_menu_has_save_games)
        return 0;
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if ((region->flags & W8_REGION_LEFT_BUTTON_HELD) && g_main_menu_has_save_games) {
            g_pending_screen_state.mode = 1;
            SetPendingScreenState(W8_SCREEN_OPTIONS);
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
        } else if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 2;
            DrawMainMenuItem(2, 1);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005bd1f0
unsigned char MainMenuExit(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            RequestExitScreen();
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
            return 0;
        }
        if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 5;
            DrawMainMenuItem(5, 1);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005bd2b0
unsigned char MainMenuOptions(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            g_pending_screen_state.mode = 0;
            SetPendingScreenState(W8_SCREEN_OPTIONS);
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
            return 0;
        }
        if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 4;
            DrawMainMenuItem(4, 1);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005bd380
unsigned char MainMenuIntroduction(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            RequestScreenTransition();
            g_settings_6850c8.intro_seen = 0;
            SetValue64D8AC(0);
            SetPendingScreenState(W8_SCREEN_INTRO);
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
            return 0;
        }
        if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 0;
            DrawMainMenuItem(0, 1);
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005bd460
unsigned char MainMenuCredits(const W8RegionEvent* event, W8Region* region)
{
    switch (event->reason) {
    case LEFT_BUTTON_DOWN:
        region->flags |= W8_REGION_LEFT_BUTTON_HELD;
        DrawMainMenuItem(g_main_menu_selected_item, 2);
        return 1;
    case LEFT_BUTTON_UP:
        DrawMainMenuItem(g_main_menu_selected_item, 1);
        if (region->flags & W8_REGION_LEFT_BUTTON_HELD) {
            SetPendingScreenState(W8_SCREEN_CREDITS);
        }
        return 1;
    case MOUSE_POS:
        if (region->flags & W8_REGION_MOUSE_LEAVE) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = static_cast<unsigned short>(-1);
            return 0;
        }
        if (region->flags & W8_REGION_MOUSE_ENTER) {
            DrawMainMenuItem(g_main_menu_selected_item, 0);
            g_main_menu_selected_item = 3;
            DrawMainMenuItem(3, 1);
        }
    }
    return 0;
}
