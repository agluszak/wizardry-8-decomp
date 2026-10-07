#include "wiz8/music_playlist.h"
#ifdef WIZ8_RUNTIME_TESTS
#include "runtime_instrumentation.h"
#endif
#include "wiz8/character_event_queue.h"
#include "wiz8/xstatus.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/PleaseWaitScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_screens/ReviewCharacterScreen.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/local_screens/AutomapScreen.h"
#include "wiz8/local_screens/CreditsScreen.h"
#include "wiz8/local_screens/OptionsScreen.h"
#include "wiz8/local_screens/JournalScreen.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/fonts.h"
#include "wiz8/sr_api.h"
#include "Container.h"
#include "Font.h"
#include "sgp.h"
#include "surrender/srTypeRegistry.h"

#include <string.h>
#include "wiz8/local_code/CombatSound.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/engine_code/Missile.h"
#include "wiz8/engine_code/Spells.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/NPCManager.h"
#include "wiz8/local_code/PC_Item.h"
#include "wiz8/local_code/UtilityFunctions.h"
#include "wiz8/monster_generators.h"
#include "wiz8/regions.h"
#include "Button System.h"
#include "LibraryDataBase.h"
#include <stdlib.h>

/* GameloopExit names this unit; GameLoop is placed with that companion. */

// GLOBAL: WIZ8 0x0068ec78
W8ScreenStateRuntime g_current_screen_state;
// GLOBAL: WIZ8 0x0068ed10
W8ScreenStateRuntime g_pending_screen_state;
// GLOBAL: WIZ8 0x0068edac
bool g_screen_return_requested;
// GLOBAL: WIZ8 0x0068eda8
HSTACK g_screen_return_stack;

// GLOBAL: WIZ8 0x00647bc8
W8ScreenStateHandlers g_screen_handlers[W8_SCREEN_COUNT] = {
    {ScreenLifecycleSuccess, IntroScreenEnter, IntroScreenFrame, IntroScreenLeave,
     ScreenLifecycleSuccess},
    {MainMenuScreenInitialize, MainMenuScreenEnter, MainMenuScreenFrame, MainMenuScreenLeave,
     ScreenLifecycleSuccess},
    {ScreenLifecycleSuccess, ScreenLifecycleSuccess, GameStartRouterFrame, GameStartRouterLeave,
     ScreenLifecycleSuccess},
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
W8ScreenId g_previous_screen_id = W8_SCREEN_NONE;
// GLOBAL: WIZ8 0x00647bc4
W8ScreenId g_suspended_screen_id = W8_SCREEN_NONE;

// FUNCTION: WIZ8 0x004e3290
void ShutdownGame(void)
{
    int index;

    ReleaseHitSoundDatabase();
    ReleaseGenericItemNames();
    ReleaseNpcStates();
    UnloadEncounterTables();
    ReleaseMissileDatabase();
    ReleaseSpellDatabase();
    DestroyItemDatabase();
    DestroyItemTables();
    FreeIfNotNull(0);
    DestroyNpcDatabase();
    DestroyFactDatabase();
    DestroyLevelDatabase();
    for (index = 0; index < 15; ++index) {
        free(g_font_state_palettes[index]);
    }
    ReleaseDefaultHelpText();
    ShutdownButtonSystem();
    for (index = 0; index < W8_SCREEN_COUNT; ++index) {
        g_screen_handlers[index].finalize();
    }
    DeleteStack(g_screen_return_stack);
    SaveGameConfiguration();
    ReleaseAllTriggers();
    FreeStringTable();
    ShutDownFileDatabase();
    DestroyGameplayObjects();
}

// FUNCTION: WIZ8 0x004e3340
void GameLoop(void)
{
    W8ScreenId state;

    SoundServiceStreams();
    ServiceMusicPlaylist();
    state = g_current_screen_state.id;
    if (g_screen_return_requested) {
        g_previous_screen_id = state;
        VideoRemoveToolTip();
        if (!g_screen_handlers[g_current_screen_state.id].leave(1)) {
            gfProgramIsRunning = 0;
            g_current_screen_state.id = W8_SCREEN_NONE;
            return;
        }
        g_current_screen_state.id = W8_SCREEN_NONE;
        if (g_pending_screen_state.id == W8_SCREEN_NONE &&
            (!StackSize(g_screen_return_stack) ||
             !Pop(g_screen_return_stack, &g_pending_screen_state))) {
            gfProgramIsRunning = 0;
            return;
        }
        state = W8_SCREEN_NONE;
        g_current_screen_state.id = state;
        g_screen_return_requested = false;
    }
    if (g_pending_screen_state.id != W8_SCREEN_NONE && g_pending_screen_state.id != state) {
        /* Retail tests only the low byte of the count. */
        if (static_cast<unsigned char>(gXStatus.character_event_queue->active_events.GetCount()) !=
            0) {
            gXStatus.character_event_queue->CompleteFirstActiveEvent();
            state = g_current_screen_state.id;
        }
        if (state != W8_SCREEN_NONE) {
            g_previous_screen_id = state;
            VideoRemoveToolTip();
            if (!g_screen_handlers[g_current_screen_state.id].leave(0)) {
                g_current_screen_state.id = W8_SCREEN_NONE;
                gfProgramIsRunning = 0;
                return;
            }
            g_suspended_screen_id = g_current_screen_state.id;
            g_screen_return_stack = Push(g_screen_return_stack, &g_current_screen_state);
        }
        state = g_pending_screen_state.id;
        g_current_screen_state = g_pending_screen_state;
        if (!g_screen_handlers[state].enter()) {
            g_current_screen_state.id = W8_SCREEN_NONE;
            gfProgramIsRunning = 0;
            return;
        }
        state = g_current_screen_state.id;
        g_pending_screen_state.id = W8_SCREEN_NONE;
#ifdef WIZ8_RUNTIME_TESTS
        RuntimeObserve(RUNTIME_SCREEN_CHANGED, g_previous_screen_id, state, -1);
        if (state == W8_SCREEN_MAIN_GAME) {
            RuntimeObserve(RUNTIME_MAIN_GAME_ENTERED, state, 0, 0);
        }
#endif
    }
    if (state == W8_SCREEN_NONE) {
        gfProgramIsRunning = 0;
        return;
    }
    g_screen_handlers[state].frame();
}

// FUNCTION: WIZ8 0x004e34b0
void GameloopExit(unsigned char release_screens)
{
    W8ScreenId state;

    SetFontObjectPalette16BPP(g_smfnt_font, g_font_palette_smfnt);
    SetFontObjectPalette16BPP(g_calligraphy_font, g_font_palette_calligraphy);
    SetFontObjectPalette16BPP(g_calligraphy_shadow_font, g_font_palette_calligraphy_shadow);
    SetFontObjectPalette16BPP(g_wiz_text_font, g_font_palette_wiz_text);
    SetFontObjectPalette16BPP(g_button_font, g_font_palette_button);
    SetFontObjectPalette16BPP(g_wiz_text_font_secondary, g_wiz_text_font_secondary_palette);
    SetFontObjectPalette16BPP(g_wiz_text_bold_font, g_font_palette_wiz_text_bold);
    SetFontObjectPalette16BPP(g_options_detail_font, g_font_palette_options_detail);
    if (!release_screens) {
        return;
    }
    for (;;) {
        if (g_current_screen_state.id != W8_SCREEN_NONE) {
            g_previous_screen_id = g_current_screen_state.id;
            VideoRemoveToolTip();
            g_screen_handlers[g_current_screen_state.id].leave(1);
            g_current_screen_state.id = W8_SCREEN_NONE;
        }
        if (!StackSize(g_screen_return_stack) ||
            !Pop(g_screen_return_stack, &g_pending_screen_state)) {
            break;
        }
        if (g_pending_screen_state.id != W8_SCREEN_NONE) {
            if (g_current_screen_state.id != W8_SCREEN_NONE) {
                srAssertFail("gCurrentScreen.iScreenId == NO_SCREEN",
                             "C:\\Projects\\Wizardry 8\\Local Code\\Gameloop.cpp", 0x276, 0);
            }
            state = g_pending_screen_state.id;
            memcpy(&g_current_screen_state, &g_pending_screen_state,
                   sizeof(g_current_screen_state));
            if (!g_screen_handlers[state].enter()) {
                g_current_screen_state.id = W8_SCREEN_NONE;
            } else {
                g_pending_screen_state.id = W8_SCREEN_NONE;
#ifdef WIZ8_RUNTIME_TESTS
                RuntimeObserve(RUNTIME_SCREEN_CHANGED, g_previous_screen_id, state, -1);
                if (state == W8_SCREEN_MAIN_GAME) {
                    RuntimeObserve(RUNTIME_MAIN_GAME_ENTERED, state, 0, 0);
                }
#endif
            }
        }
    }
}
