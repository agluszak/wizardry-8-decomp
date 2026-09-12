#include "wiz8/music_playlist.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/sr_api.h"
#include "Container.h"
#include "Font.h"
#include "sgp.h"
#include "surrender/srTypeRegistry.h"

#include <string.h>

/*
 * Local Code\Gameloop.cpp. GameloopExit at 0x004E34B0 asserts this unit
 * (Gameloop.cpp:630). GameLoop at 0x004E3340 immediately precedes it and no
 * assertion names its unit, so it is placed here with its asserted companion
 * rather than claimed as proven.
 *
 * The per-frame tick WinMain calls when no message is waiting and the
 * application is active. It drives a screen-state stack: the current state
 * descriptor sits at 0x0068EC78 and the pending one immediately after it at
 * 0x0068ED10, both 0x98 bytes, which is the element size InitializeGame gives
 * CreateStack. A state transition copies pending over current, and the
 * displaced state is pushed so it can be returned to.
 *
 * Each state owns five dwords in the table at 0x00647BC8: initialize, enter,
 * frame, leave, and finalize. Startup and shutdown walk the outside pair; the
 * transition loop dispatches the middle three.
 */

// GLOBAL: WIZ8 0x0068ec78
W8ScreenStateRuntime g_current_screen_state;
// GLOBAL: WIZ8 0x0068ed10
W8ScreenStateRuntime g_pending_screen_state;
// GLOBAL: WIZ8 0x0068edac
unsigned char g_screen_return_requested;
// GLOBAL: WIZ8 0x0068eda8
void* g_screen_return_stack;

// GLOBAL: WIZ8 0x00647bc8
W8ScreenStateHandlers g_screen_handlers[W8_SCREEN_COUNT] = {
    {ScreenLifecycleSuccess, IntroScreenEnter, IntroScreenFrame, IntroScreenLeave,
     ScreenLifecycleSuccess},
    {MainMenuScreenInitialize, MainMenuScreenEnter, MainMenuScreenFrame, MainMenuScreenLeave,
     ScreenLifecycleSuccess},
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"
    /* Retail stores the zero-argument success sentinel in the int-taking
       leave slot. Do not invent a thunk or change ScreenLifecycleSuccess. */
    {ScreenLifecycleSuccess, ScreenLifecycleSuccess, GameStartRouterFrame,
     (unsigned char (*)(int))ScreenLifecycleSuccess, ScreenLifecycleSuccess},
#pragma clang diagnostic pop
    {ScreenLifecycleSuccess, CharacterScreenEnter, CharacterScreenFrame, CharacterScreenLeave,
     ScreenLifecycleSuccess},
    {PleaseWaitScreenInitialize, PleaseWaitScreenEnter, PleaseWaitScreenFrame,
     PleaseWaitScreenLeave, ScreenLifecycleSuccess},
    {ScreenLifecycleSuccess, PartySelectionScreenEnter, PartySelectionScreenFrame,
     PartySelectionScreenLeave, ScreenLifecycleSuccess},
    {CampScreenInitialize, CampScreenEnter, CampScreenFrame, CampScreenLeave,
     ScreenLifecycleSuccess},
    {MainGameScreenInitialize, MainGameScreenEnter, MainGameScreenFrame, MainGameScreenLeave,
     ScreenLifecycleSuccess},
    {AutomapScreenInitialize, AutomapScreenEnter, AutomapScreenFrame, AutomapScreenLeave,
     AutomapScreenFinalize},
    {ScreenLifecycleSuccess, CreditsScreenEnter, CreditsScreenFrame, CreditsScreenLeave,
     ScreenLifecycleSuccess},
    {OptionsScreenInitialize, OptionsScreenEnter, OptionsScreenFrame, OptionsScreenLeave,
     OptionsScreenFinalize},
    {JournalScreenInitialize, JournalScreenEnter, JournalScreenFrame, JournalScreenLeave,
     JournalScreenFinalize},
    {ScreenLifecycleSuccess, ExitScreenEnter, ExitScreenFrame, MainMenuScreenLeave,
     ScreenLifecycleSuccess}};
// GLOBAL: WIZ8 0x00647bc0
int g_previous_screen_id = -1;
// GLOBAL: WIZ8 0x00647bc4
int g_suspended_screen_id = -1;

// FUNCTION: WIZ8 0x004e3340
void GameLoop(void)
{
    int state;

    SoundServiceStreams();
    ServiceMusicPlaylist0048F9E0();
    state = g_current_screen_state.id;
    if (g_screen_return_requested) {
        g_previous_screen_id = state;
        VideoRemoveToolTip();
        if (!g_screen_handlers[g_current_screen_state.id].leave(1)) {
            gfProgramIsRunning = 0;
            g_current_screen_state.id = -1;
            return;
        }
        g_current_screen_state.id = -1;
        if (g_pending_screen_state.id == -1) {
            if (!StackSize(g_screen_return_stack)) {
                goto stop;
            }
            if (!Pop(g_screen_return_stack, &g_pending_screen_state)) {
                goto stop;
            }
        }
        state = -1;
        g_current_screen_state.id = state;
        g_screen_return_requested = 0;
    }
    if (g_pending_screen_state.id == -1 || g_pending_screen_state.id == state) {
        goto finish;
    }
    /* The original tests only the low byte of the vector count. Preserve that
       aliasing instead of widening the load to the field's full int type. */
    if (*reinterpret_cast<const unsigned char*>(&gXStatus.character_event_queue->vector_40.count) !=
        0) {
        gXStatus.character_event_queue->ProcessNextPendingEntry();
        state = g_current_screen_state.id;
    }
    if (state != -1) {
        g_previous_screen_id = state;
        VideoRemoveToolTip();
        if (!g_screen_handlers[g_current_screen_state.id].leave(0)) {
            goto clear;
        }
        g_suspended_screen_id = g_current_screen_state.id;
        g_screen_return_stack = Push(g_screen_return_stack, &g_current_screen_state);
    }
    state = g_pending_screen_state.id;
    memcpy(&g_current_screen_state, &g_pending_screen_state, sizeof(W8ScreenStateRuntime));
    if (!g_screen_handlers[state].enter()) {
        goto clear;
    }
    state = g_current_screen_state.id;
    g_pending_screen_state.id = -1;

finish:
    if (state == -1) {
        goto stop;
    }
    g_screen_handlers[state].frame();
    return;

clear:
    g_current_screen_state.id = -1;
stop:
    gfProgramIsRunning = 0;
}

// FUNCTION: WIZ8 0x004e34b0
void GameloopExit(unsigned char release_screens)
{
    int state;

    SetFontObjectPalette16BPP(g_smfnt_font_683694, g_font_palette_smfnt_68ee10);
    SetFontObjectPalette16BPP(g_calligraphy_font_6835f8, g_font_palette_calligraphy_68edfc);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font_6835f4,
                              g_font_palette_calligraphy_shadow_68ee18);
    SetFontObjectPalette16BPP(g_wiz_text_font_683640, g_font_palette_wiz_text_68ee14);
    SetFontObjectPalette16BPP(g_button_font_683670, g_font_palette_button_68ee04);
    SetFontObjectPalette16BPP(g_font_683660, g_colour_68ee08);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font_683664, g_font_palette_wiz_text_bold_68ee0c);
    SetFontObjectPalette16BPP(g_options_detail_font_683614, g_font_palette_options_detail_68ee00);
    if (!release_screens) {
        return;
    }
    for (;;) {
        if (g_current_screen_state.id != -1) {
            g_previous_screen_id = g_current_screen_state.id;
            VideoRemoveToolTip();
            g_screen_handlers[g_current_screen_state.id].leave(1);
            g_current_screen_state.id = -1;
        }
        if (!StackSize(g_screen_return_stack) ||
            !Pop(g_screen_return_stack, &g_pending_screen_state)) {
            break;
        }
        if (g_pending_screen_state.id != -1) {
            if (g_current_screen_state.id != -1) {
                srAssertFail("gCurrentScreen.iScreenId == NO_SCREEN",
                             "C:\\Projects\\Wizardry 8\\Local Code\\Gameloop.cpp", 0x276, 0);
            }
            state = g_pending_screen_state.id;
            memcpy(&g_current_screen_state, &g_pending_screen_state,
                   sizeof(g_current_screen_state));
            if (!g_screen_handlers[state].enter()) {
                g_current_screen_state.id = -1;
            } else {
                g_pending_screen_state.id = -1;
            }
        }
    }
}
