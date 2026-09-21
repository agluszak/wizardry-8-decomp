/* Enable the pinned SDK's SendInput declarations for the harness only. */
#define _WIN32_WINNT 0x0500
#include "game_thread_executor.h"
#include "runtime_scenario.h"
#include "wiz8/regions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/IntroScreen.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/CharGeneration.h"
#include "wiz8/local_code/GameplayInit.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/local_code/LoadSaveGame.h"
#include "wiz8/local_code/NPCScripting.h"
#include "wiz8/local_code/PartyImport.h"
#include "wiz8/music_playlist.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/xstatus.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/fonts.h"
#include "wiz8/notices.h"
#include "wiz8_crash_report.h"
#include "oct_file_semantic_test.h"
#include "keyboard_menu_semantic_test.h"
#include "npc_dialogue_semantic_test.h"
#include "mongen_semantic_test.h"
#include "mouth_gap_semantic_test.h"
#include "sight_semantic_test.h"
#include "split_stack_semantic_test.h"
#include "party_movement_semantic_test.h"
#include "audio_semantic_test.h"

#include "english.h"
#include "FileMan.h"
#include "input.h"
#include "LibraryDataBase.h"
#include "shading.h"
#include "sgp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/layouts/game_status.h"

extern "C" {

extern InputAtom gEventQueue[256];
}

struct W8LevelLoadDescriptor;
extern W8LevelLoadDescriptor* g_load_descriptor_69b7c8;

static RuntimeObservation g_observation;
static const char* g_scenario;
static const RuntimeScenario* g_scenario_spec;

static bool RunSearchModeSemanticTest(void)
{
    W8LevelRuntimeBlock level;
    memset(&level, 0, sizeof(level));
    level.text_box_font = g_font_683660;
    level.text_box_right = 30000;
    W8LevelRuntimeBlock* saved_level = g_level_block;
    int saved_screen = g_current_screen_state.id;
    unsigned char saved_search = g_status_685170.search_mode;
    unsigned char saved_combat = gXStatus.fCombatMode;
    unsigned char saved_camp = gXStatus.fCampMode;
    unsigned char saved_dialogue = gXStatus.fNpcDialogueMode;
    unsigned int saved_clock = g_search_pulse_clock_00689fcc;
    unsigned int used[4];
    memcpy(used, g_status_685170.text_box_lines_used_4997, sizeof(used));
    g_level_block = &level;
    /* The loading screen accepts notices without advancing the live game UI. */
    g_current_screen_state.id = W8_SCREEN_PLEASE_WAIT;
    gXStatus.fCombatMode = 0;
    gXStatus.fCampMode = 0;
    gXStatus.fNpcDialogueMode = 0;
    g_status_685170.search_mode = 0;
    g_search_pulse_clock_00689fcc = 0;
    ToggleSearchMode();
    bool on = g_status_685170.search_mode != 0 && g_search_pulse_clock_00689fcc != 0 &&
              g_status_685170.text_box_lines_used_4997[0] == used[0] + 1 &&
              wcscmp(g_message_storage_68f2d8[0][used[0]].wString,
                     gppStringList[W8_NOTICE_SEARCH_MODE_ON]) == 0;
    ToggleSearchMode();
    bool off = g_status_685170.search_mode == 0 &&
               g_status_685170.text_box_lines_used_4997[0] == used[0] + 2 &&
               wcscmp(g_message_storage_68f2d8[0][used[0] + 1].wString,
                      gppStringList[W8_NOTICE_SEARCH_MODE_OFF]) == 0;
    gXStatus.fCombatMode = 1;
    unsigned int clock = g_search_pulse_clock_00689fcc;
    ToggleSearchMode();
    int combat_box = GetFlag68F105() ? 0 : 1;
    unsigned int combat_index = used[combat_box] + (combat_box == 0 ? 2 : 0);
    bool blocked = g_status_685170.search_mode == 0 && g_search_pulse_clock_00689fcc == clock &&
                   g_status_685170.text_box_lines_used_4997[combat_box] == combat_index + 1 &&
                   wcscmp(g_message_storage_68f2d8[combat_box][combat_index].wString,
                          gppStringList[W8_NOTICE_SEARCH_BLOCKED_COMBAT]) == 0;
    for (int box = 0; box < 4; ++box) {
        for (unsigned int index = used[box]; index < g_status_685170.text_box_lines_used_4997[box];
             ++index) {
            W8MessageStorageRecord* record = &g_message_storage_68f2d8[box][index];
            free(record->wString);
            if (record->entries_18) {
                for (unsigned int entry = 0; entry < PLLength(record->entries_18); ++entry) {
                    free(PLGet(record->entries_18, entry));
                }
                PListClear(record->entries_18);
                PLDestroy(record->entries_18);
            }
            memset(record, 0, sizeof(*record));
        }
    }
    memcpy(g_status_685170.text_box_lines_used_4997, used, sizeof(used));
    g_level_block = saved_level;
    g_current_screen_state.id = saved_screen;
    g_status_685170.search_mode = saved_search;
    gXStatus.fCombatMode = saved_combat;
    gXStatus.fCampMode = saved_camp;
    gXStatus.fNpcDialogueMode = saved_dialogue;
    g_search_pulse_clock_00689fcc = saved_clock;
    fprintf(stderr, "WIZ8_SEARCH_MODE on=%u off=%u combat_blocked=%u\n", on, off, blocked);
    return on && off && blocked;
}

/* Bounds the driver join after WinMain returns. Python owns the hard
   process deadline, including hangs inside WinMain. */

static DWORD g_scenario_started;

static void ReportStep(const char* step)
{
    fprintf(stderr, "WIZ8_RUNTIME_STEP scenario=%s step=%s state=pass elapsed_ms=%lu\n", g_scenario,
            step, GetTickCount() - g_scenario_started);
    fflush(stderr);
}

static void WriteRuntimeTestContext(FILE* stream)
{
    fprintf(stream, "runtime-test context: pending=%d current=%d\n", g_pending_screen_state.id,
            g_current_screen_state.id);
}

static bool VerifyShadeTable(FLOAT coefficient)
{
    // The runtime uses ARGB1555. Check independent colour values before
    // comparing tables that both depend on Get16BPPColor: unresolved SGP
    // mask globals previously made both sides agree on the PE's MZ bytes.
    if (Get16BPPColor(FROMRGB(255, 0, 0)) != 0xfc00 ||
        Get16BPPColor(FROMRGB(0, 255, 0)) != 0x83e0 ||
        Get16BPPColor(FROMRGB(0, 0, 255)) != 0x801f ||
        Get16BPPColor(FROMRGB(255, 255, 255)) != 0xffff) {
        fprintf(stderr, "SGP pixel-format conversion is not ARGB1555\n");
        return false;
    }
    static UINT16 expected[65536];
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int index;

    memset(expected, 0, sizeof(expected));
    for (red = 0; red < 256; red += 4) {
        for (green = 0; green < 256; green += 4) {
            for (blue = 0; blue < 256; blue += 4) {
                index = Get16BPPColor(FROMRGB(red, green, blue));
                expected[index] =
                    Get16BPPColor(FROMRGB((UINT8)(red * coefficient), (UINT8)(green * coefficient),
                                          (UINT8)(blue * coefficient)));
            }
        }
    }
    if (memcmp(expected, ShadeTable, sizeof(expected)) != 0) {
        for (index = 0; index < 65536; ++index) {
            if (expected[index] != ShadeTable[index]) {
                fprintf(stderr,
                        "shade-table mismatch coefficient=%g index=%u expected=%u actual=%u\n",
                        coefficient, index, expected[index], ShadeTable[index]);
                break;
            }
        }
        return false;
    }
    for (index = 0; index < 256; ++index) {
        if (White16BPPPalette[index] != 0xffff) {
            fprintf(stderr, "white-palette mismatch index=%u actual=%u\n", index,
                    White16BPPPalette[index]);
            return false;
        }
    }
    return true;
}

/* Move the Wine pointer to a client point without clicking. */
static void MoveScenarioMouse(int client_x, int client_y)
{
    POINT point;
    point.x = client_x;
    point.y = client_y;
    if (ghWindow == NULL || !ClientToScreen(ghWindow, &point)) {
        fprintf(stderr, "runtime-test ClientToScreen failed: %lu\n", GetLastError());
        return;
    }
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);
    if (screen_width < 2)
        screen_width = 2;
    if (screen_height < 2)
        screen_height = 2;
    INPUT event;
    memset(&event, 0, sizeof(event));
    event.type = INPUT_MOUSE;
    event.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    event.mi.dx = (point.x * 65535) / (screen_width - 1);
    event.mi.dy = (point.y * 65535) / (screen_height - 1);
    SetForegroundWindow(ghWindow);
    SendInput(1, &event, sizeof(INPUT));
}

/* SGP's queue belongs to the game thread. SendInput reaches its WH_KEYBOARD
   hook; posting WM_KEYDOWN directly does not. The private display parks the
   pointer at the window centre, which sits on the Load Game item, so every
   key send first moves it off every enabled region: otherwise hover overrules
   the keyboard selection non-deterministically. */
static bool PointHitsEnabledRegion(int x, int y)
{
    unsigned short px = static_cast<unsigned short>(x);
    unsigned short py = static_cast<unsigned short>(y);
    for (unsigned int set = 0; set < g_region_set_count; ++set) {
        if (g_region_sets[set].enabled != 1 ||
            g_region_sets[set].first_region > g_region_sets[set].last_region) {
            continue;
        }
        unsigned int first = g_region_sets[set].first_region;
        unsigned int last = g_region_sets[set].last_region;
        for (unsigned int region = first; region <= last && region < g_region_count; ++region) {
            if (RegionContainsPoint(region, px, py)) {
                return true;
            }
        }
    }
    return false;
}

static void ParkMouseOutsideActiveRegions()
{
    RECT client;
    if (ghWindow == NULL || !GetClientRect(ghWindow, &client)) {
        return;
    }
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width < 1 || height < 1) {
        return;
    }
    int candidates[8][2] = {
        {width - 1, 0},          {0, 0},
        {width - 1, height - 1}, {0, height - 1},
        {width / 2, 0},          {width - 1, height / 2},
        {0, height / 2},         {width / 2, height - 1},
    };
    for (int index = 0; index < 8; ++index) {
        int x = candidates[index][0];
        int y = candidates[index][1];
        if (!PointHitsEnabledRegion(x, y)) {
            MoveScenarioMouse(x, y);
            return;
        }
    }
    for (int top_x = 0; top_x < width; ++top_x) {
        if (!PointHitsEnabledRegion(top_x, 0)) {
            MoveScenarioMouse(top_x, 0);
            return;
        }
    }
    for (int bottom_x = 0; bottom_x < width; ++bottom_x) {
        if (!PointHitsEnabledRegion(bottom_x, height - 1)) {
            MoveScenarioMouse(bottom_x, height - 1);
            return;
        }
    }
    MoveScenarioMouse(width - 1, 0);
}

static void SendScenarioKey(WORD key, DWORD flags = 0)
{
    ParkMouseOutsideActiveRegions();
    INPUT events[2];
    memset(events, 0, sizeof(events));
    events[0].type = INPUT_KEYBOARD;
    events[0].ki.wVk = key;
    events[0].ki.dwFlags = flags;
    events[1] = events[0];
    events[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    SetForegroundWindow(ghWindow);
    if (SendInput(2, events, sizeof(INPUT)) != 2) {
        fprintf(stderr, "runtime-test keyboard injection failed: %lu\n", GetLastError());
    }
}

/* The navigation cluster is always extended on PC keyboards; Wine's
   MapVirtualKey does not report the 0xe000 scancode prefix, so extendedness is
   decided from the virtual key itself. */
static int IsExtendedScenarioKey(WORD key)
{
    switch (key) {
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_END:
    case VK_HOME:
    case VK_INSERT:
    case VK_DELETE:
        return 1;
    }
    return 0;
}

/* Held input uses the OS keyboard path, including SGP's hook, not driver-thread
   writes to gfKeyState or its event queue. The physical scan and extended bit
   distinguish dedicated arrows from the numeric keypad. */
static void SendScenarioKeyHeld(WORD key, unsigned char release)
{
    ParkMouseOutsideActiveRegions();
    INPUT event;
    memset(&event, 0, sizeof(event));
    event.type = INPUT_KEYBOARD;
    event.ki.wVk = key;
    event.ki.wScan = static_cast<WORD>(MapVirtualKey(key, 0));
    if (IsExtendedScenarioKey(key)) {
        event.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (release) {
        event.ki.dwFlags |= KEYEVENTF_KEYUP;
    }
    SetForegroundWindow(ghWindow);
    if (SendInput(1, &event, sizeof(event)) != 1) {
        fprintf(stderr,
                "WIZ8_RUNTIME_FAILURE scenario=%s step=input reason=sendinput-failed error=%lu\n",
                g_scenario, GetLastError());
        fflush(stderr);
    }
}

/* SGP's mouse hook consumes client coordinates, so the scenario converts the
   target point before handing the absolute move to SendInput. */
static void SendScenarioMouse(int client_x, int client_y)
{
    MoveScenarioMouse(client_x, client_y);
    INPUT events[2];
    memset(events, 0, sizeof(events));
    events[0].type = INPUT_MOUSE;
    events[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    events[1].type = INPUT_MOUSE;
    events[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SetForegroundWindow(ghWindow);
    if (SendInput(2, events, sizeof(INPUT)) != 2) {
        fprintf(stderr, "runtime-test mouse injection failed: %lu\n", GetLastError());
    }
}

/* Click the live centre of a product control through its registered region,
   the same rectangle the input dispatch uses. */
static void ClickControl(W8TextControl* control)
{
    if (control == 0 || control->m_region < 0 ||
        static_cast<unsigned int>(control->m_region) >= g_region_count) {
        return;
    }
    W8Region* bounds = &g_regions[control->m_region];
    SendScenarioMouse((bounds->x1 + bounds->x2) / 2, (bounds->y1 + bounds->y2) / 2);
}

static DWORD FailScenarioAt(const char* step, const char* reason, int line)
{
    fprintf(stderr, "WIZ8_RUNTIME_FAILURE scenario=%s step=%s reason=%s line=%d\n", g_scenario,
            step, reason, line);
    fprintf(stderr,
            "runtime-test failed: state=%d pending=%d transition=%u entered=%u final=%u "
            "redrawn=%u committed=%u in_party=%u main_game=%u page=%d running=%u active=%u\n",
            g_current_screen_state.id, g_pending_screen_state.id, g_observation.transition_observed,
            g_observation.character_entered, g_observation.final_page_entered,
            g_observation.final_page_redrawn, g_observation.character_committed,
            g_observation.character_in_party, g_observation.main_game_entered,
            g_observation.character_page_after, gfProgramIsRunning, gfApplicationActive);
    if (gzErrorMsg[0] != '\0') {
        fprintf(stderr, "runtime-test shutdown error: %s\n", gzErrorMsg);
    }
    fflush(stderr);
    gfProgramIsRunning = 0;
    if (ghWindow != NULL) {
        PostMessage(ghWindow, WM_CLOSE, 0, 0);
    }
    return 2;
}

#define FailScenario(step, reason) FailScenarioAt(step, reason, __LINE__)

/* Walk a live region's current bounds instead of a fixed pixel. */
static bool RegionCenter(int region_index, int* x, int* y)
{
    if (region_index < 0 || static_cast<unsigned int>(region_index) >= g_region_count) {
        return false;
    }
    W8Region* region = &g_regions[region_index];
    *x = (region->x1 + region->x2) / 2;
    *y = (region->y1 + region->y2) / 2;
    return true;
}

static void HoverRegion(int region_index)
{
    int x;
    int y;
    if (RegionCenter(region_index, &x, &y)) {
        MoveScenarioMouse(x, y);
    }
}

static void ClickRegion(int region_index)
{
    int x;
    int y;
    if (RegionCenter(region_index, &x, &y)) {
        SendScenarioMouse(x, y);
    }
}

/* The region a control registered for its help text. The party-builder start
   control is the only one in the bottom action panel with help id 0x6cb, so
   the scenario clicks the product control rather than a fixed pixel. */
static int RegionWithHelpText(unsigned int region_set, int help_text_id)
{
    if (region_set >= g_region_set_count) {
        return -1;
    }
    unsigned int first = g_region_sets[region_set].first_region;
    unsigned int last = g_region_sets[region_set].last_region;
    for (unsigned int region = first; region <= last && region < g_region_count; ++region) {
        if (g_regions[region].help_text_id == help_text_id) {
            return (int)region;
        }
    }
    return -1;
}

/* The tooltip's owning object count is the structural marker; the pixel
   output is not part of this assertion. */
static bool WaitForTooltip(bool present, unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms) {
        if (HasScreenTransitionObjects() == present) {
            return true;
        }
        Sleep(5);
    }
    return false;
}

static bool WaitForMainMenu(unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    W8BinkVideo* dismissed_video = 0;
    unsigned int dismissed_at = 0;
    while (GetTickCount() - started < timeout_ms) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_MENU &&
            *(HWND volatile*)&ghWindow != NULL && g_region_sets[1].enabled) {
            return true;
        }
        // State zero also exists before input initialization clears the queue.
        // Wait until startup finishes before posting the intro-dismiss events.
        W8BinkVideo* video = *(W8BinkVideo* volatile*)&gpVideo;
        unsigned int now = GetTickCount();
        if (gfGameInitialized && gfApplicationActive &&
            *(volatile int*)&g_current_screen_state.id == W8_SCREEN_INTRO && video != NULL &&
            (video != dismissed_video || now - dismissed_at > 1000)) {
            SendScenarioKey(VK_ESCAPE);
            dismissed_video = video;
            dismissed_at = now;
        }
        Sleep(10);
    }
    return false;
}

static bool VerifyPatchPrecedence(void)
{
    for (int library_id = NUMBER_OF_LIBRARIES; library_id < gFileDataBase.usNumberOfLibraries;
         ++library_id) {
        LibraryHeaderStruct* library = &gFileDataBase.pLibraries[library_id];
        if (!library->fPatchLibrary) {
            continue;
        }
        for (unsigned int entry = 0; entry < library->usNumberOfEntries; ++entry) {
            char path[512];
            sprintf(path, "%s%s", library->sLibraryPath, library->pFileHeader[entry].pFileName);
            if (GetLibraryIDFromFileName(path) == library_id) {
                HWFILE handle = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
                short selected_library = -1;
                unsigned int file_id = 0;
                if (handle != 0) {
                    GetLibraryAndFileIDFromLibraryFileHandle(handle, &selected_library, &file_id);
                    FileClose(handle);
                }
                return selected_library == library_id;
            }
        }
    }
    return false;
}

static bool VerifyPhysicalFileFallback(void)
{
    HWFILE handle = FileOpen("3DVideo.CFG", FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
    short library_id = -1;
    unsigned int file_id = 0;
    if (handle == 0) {
        return false;
    }
    GetLibraryAndFileIDFromLibraryFileHandle(handle, &library_id, &file_id);
    FileClose(handle);
    return library_id == REAL_FILE_LIBRARY_ID;
}

struct NpcStateResetContext {
    int result;
};

static void ResetNpcStateOnGameThread(void* opaque)
{
    NpcStateResetContext* context = static_cast<NpcStateResetContext*>(opaque);
    W8MessageBoxLine* line = new W8MessageBoxLine;
    memset(line, 0, sizeof(W8MessageBoxLine));
    /* FINISH_ACTION with a null payload is benign outside dialogue; the
       reset must drain it without trying to process an active NPC. */
    line->type = W8_NPC_MSG_FINISH_ACTION;
    if (g_npc_scripting.message_lines.Add(line) < 0) {
        delete line;
        context->result = -1;
        return;
    }
    g_npc_scripting.restore_staged_session = 1;
    g_npc_scripting.voice_handle = 7;
    g_npc_scripting.staging_restore.current_quote_index = 0x1234;
    g_npc_scripting.gap_track.mouth_open = 1;
    g_npc_scripting.last_tick = 99;
    ResetLiveSessionForLoad();
    context->result =
        g_npc_scripting.message_lines.GetCount() == 0 &&
                g_npc_scripting.pending_script_values.GetCount() == 0 &&
                g_npc_scripting.restore_staged_session == 0 && g_npc_scripting.voice_handle == 0 &&
                g_npc_scripting.staging_restore.current_quote_index == 0 &&
                g_npc_scripting.gap_track.mouth_open == 0 && g_npc_scripting.last_tick == 0
            ? 1
            : 0;
    /* Reset retires the live session. Stop before WinMain can run another
       frame against it while the driver is waking from the completion event. */
    gfProgramIsRunning = 0;
}

static void RunMenuChecksOnGameThread(void*)
{
    g_observation.playlist_active = g_music_playlist_active_65ba7e;
    g_observation.playlist_tracks = g_music_playlist_track_count_65ba84;
    g_observation.playlist_weight = g_music_playlist_weight_total_65ba80;
    g_observation.playlist_pause_min = g_music_state_60aae8;
    g_observation.playlist_pause_max = g_music_state_60aaec;
    g_observation.playlist_pause_chance = g_music_state_60aaf0;
    g_observation.patch_catalog_count = gFileDataBase.usNumberOfLibraries - NUMBER_OF_LIBRARIES;
    g_observation.item_database_count = gXStatus.uiItemsInDatabase;
    g_observation.monster_database_count = gXStatus.uiMonstersInDatabase;
    g_observation.npc_database_count = gXStatus.uiNpcsInDatabase;
    g_observation.patch_precedence_ok = VerifyPatchPrecedence();
    g_observation.physical_fallback_ok = VerifyPhysicalFileFallback();
    const bool initial_table = VerifyShadeTable((FLOAT)0.66);
    SetShadeTablePercent((FLOAT)0.50);
    const bool changed_table = VerifyShadeTable((FLOAT)0.50);
    SetShadeTablePercent((FLOAT)0.66);
    const bool restored_table = VerifyShadeTable((FLOAT)0.66);
    g_observation.shade_table_ok = initial_table && changed_table && restored_table;
    fprintf(stderr,
            "runtime-test menu: scenario=%s state=%d regions=%u first=%u last=%u "
            "selected=%u playlist=%u tracks=%d weight=%d pause=%d..%d@%d "
            "patches=%d patch_precedence=%u physical_fallback=%u\n",
            g_scenario, g_observation.menu_state, g_observation.region_set_enabled,
            g_observation.first_region, g_observation.last_region, g_main_menu_selected_item,
            g_observation.playlist_active, g_observation.playlist_tracks,
            g_observation.playlist_weight, g_observation.playlist_pause_min,
            g_observation.playlist_pause_max, g_observation.playlist_pause_chance,
            g_observation.patch_catalog_count, g_observation.patch_precedence_ok,
            g_observation.physical_fallback_ok);
    fflush(stderr);
}

static DWORD RunOctFileScenario()
{
    OctFileSemanticResult oct_result;
    g_observation.semantic_ok = RunOctFileSemanticTests(&oct_result);
    PrintOctFileSemanticResults(&oct_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunSightScenario()
{
    SightSemanticResult sight_result;
    g_observation.semantic_ok = RunSightSemanticTests(&sight_result);
    PrintSightSemanticResults(&sight_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunSplitStackScenario()
{
    SplitStackSemanticResult split_result;
    g_observation.semantic_ok = RunSplitStackSemanticTest(&split_result);
    PrintSplitStackSemanticResults(&split_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunPartyMovementScenario()
{
    PartyMovementSemanticResult movement_result;
    g_observation.semantic_ok = RunPartyMovementSemanticTest(&movement_result);
    PrintPartyMovementSemanticResults(&movement_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunAudioScenario()
{
    AudioSemanticResult audio_result;
    g_observation.semantic_ok = RunAudioSemanticTests(&audio_result);
    PrintAudioSemanticResults(&audio_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunKeyboardMenuScenario()
{
    KeyboardMenuSemanticResult keyboard_result;
    g_observation.semantic_ok = RunKeyboardMenuSemanticTest(&keyboard_result);
    PrintKeyboardMenuSemanticResults(&keyboard_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunMouthGapScenario()
{
    MouthGapSemanticResult mouth_gap_result;
    g_observation.semantic_ok = RunMouthGapSemanticTest(&mouth_gap_result);
    PrintMouthGapSemanticResults(&mouth_gap_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunNpcDialogueScenario()
{
    NpcDialogueSemanticResult dialogue_result;
    g_observation.semantic_ok = RunNpcDialogueSemanticTest(&dialogue_result);
    PrintNpcDialogueSemanticResults(&dialogue_result);
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunSearchModeScenario()
{
    g_observation.semantic_ok = RunSearchModeSemanticTest();
    return g_observation.semantic_ok ? 0 : 1;
}

static DWORD RunMonGenScenario()
{
    g_observation.semantic_ok = RunMonGenSemanticTest();
    return g_observation.semantic_ok ? 0 : 1;
}

struct HostileEncounterContext {
    srVector3T<float> party_position;
    W8MonsterInfo* info;
    float distance;
    int location_id;
};

static void ProvokeHostileEncounterOnGameThread(void* opaque)
{
    HostileEncounterContext* context = static_cast<HostileEncounterContext*>(opaque);
    srVector3T<float> party_position;
    W8MonsterInfo* provoked_info = 0;
    float provoked_distance = 1e30f;
    GetCameraPosition(&party_position);
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info != 0 && info->fActive != 0 && info->monster != 0 && info->monster_group_id != 0 &&
            info->condition_turns[13] == 0) {
            float dist = (info->monster->GetPosition() - party_position).Length();
            if (dist < provoked_distance) {
                provoked_distance = dist;
                provoked_info = info;
            }
        }
    }
    if (provoked_info != 0) {
        unsigned int provoked_location_id = provoked_info->location_id;
        int provoked_group_id = provoked_info->monster_group_id;
        /* At ~17k units the hostile group's own navigation
           never finds a route to the party, so its combat
           turn can never commit a move. Relocate the group
           beside the camera through
           PositionMonsterGroupNearCamera00511050 - the same
           placement GroupAttacks uses for summon encounters -
           so the party stays grounded where it stands. A
           party teleport drops the collision state the frame
           loop needs: without ground contact the camera
           falls below the level bounds, BeginPartyMovement
           reads as a fall death and PumpReviewTransition
           unloads the world before StartCombat ever sees a
           grounded party. */
        W8MonsterGroup* provoked_group = GetMonsterGroupByListIndex(
            GetMonsterGroupIndexByID(__LINE__, "runtime-test", provoked_info->monster_group_id, 0));
        unsigned char placed = 0;
        if (provoked_group != 0) {
            placed = PositionMonsterGroupNearCamera00511050(provoked_group, 0.0f, 0.0f, 1);
            if (placed == 0) {
                placed = PositionMonsterGroupNearCamera00511050(provoked_group, 1500.0f, 0.0f, 1);
            }
            if (placed == 0) {
                placed = PositionMonsterGroupNearCamera00511050(provoked_group, 3000.0f, 0.0f, 1);
            }
        }
        fprintf(stderr, "runtime-test drop: group=%p placed=%d\n", (void*)provoked_group, placed);
        if (provoked_group != 0 && placed != 0) {
            RefreshAllSight();
            SetMonsterGroupNavigatorDirty(provoked_group, 0);
        }
        /* Placement may drop members that found no scatter
           spot; RemoveMonster detaches their monster and frees
           the info, so re-resolve the chosen member and fall
           back to any surviving member of the same group. */
        {
            unsigned int re_index =
                MonsterGetIndexByLocationID(__LINE__, "runtime-test", provoked_location_id, 0);
            provoked_info =
                re_index != (unsigned int)-1 ? MonsterGetScriptPartByLocationIndex(re_index) : 0;
        }
        if (provoked_info == 0 || provoked_info->monster == 0) {
            provoked_info = 0;
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info != 0 && info->fActive != 0 && info->monster != 0 &&
                    info->monster_group_id == provoked_group_id) {
                    provoked_info = info;
                    break;
                }
            }
        }
    }
    if (provoked_info != 0 && provoked_info->monster != 0) {
        provoked_distance = (provoked_info->monster->GetPosition() - party_position).Length();
        W8TargetSource source;
        W8CombatSlot target;
        memset(&target, 0, sizeof(target));
        target.iChar = -1;
        target.iMonsterID = -1;
        target.iGroupID = -1;
        SetTargetSourceToCharacter(0, &source);
        target.iType = W8_TARGET_KIND_MONSTER;
        target.iMonsterID = provoked_info->location_id;
        MakeTargetGroupHostile(&source, &target);
        fprintf(stderr,
                "runtime-test provoke: hp=%d cond=%d active=%d incombat=%d "
                "hostile=%u combat=%u\n",
                provoked_info->hp_current, provoked_info->highest_condition, provoked_info->fActive,
                provoked_info->fInCombat, gXStatus.hostile_monster_count, gXStatus.fCombatMode);
    }
    context->party_position = party_position;
    context->info = provoked_info;
    context->distance = provoked_distance;
    context->location_id = provoked_info != 0 ? provoked_info->location_id : -1;
}

/* Everything the hostile-encounter driver needs from live game state, filled
   entirely on the game thread so the observation loop never walks mutable
   product structures off-thread. */
struct HostileEngagementSnapshot {
    int screen;
    int pending;
    unsigned int hostile_count;
    unsigned int active_monsters;
    unsigned int engaged_hostiles;
    unsigned int hostile_condition_monsters;
    float nearest_engaged_distance;
    unsigned int party_hp_total;
    unsigned int provoked_active;
    int provoked_hp;
    int provoked_condition;
    int provoked_in_combat;
    float provoked_distance;
    unsigned int combat_mode;
    unsigned int round_active;
    int action_status;
    int action_monster;
};

struct HostileSnapshotQuery {
    int location_id;
    HostileEngagementSnapshot snapshot;
};

static void ReadHostileEngagementOnGameThread(void* opaque)
{
    HostileSnapshotQuery* query = static_cast<HostileSnapshotQuery*>(opaque);
    HostileEngagementSnapshot* s = &query->snapshot;
    srVector3T<float> party_position;
    memset(s, 0, sizeof(*s));
    GetCameraPosition(&party_position);
    s->screen = g_current_screen_state.id;
    s->pending = g_pending_screen_state.id;
    s->combat_mode = gXStatus.fCombatMode != 0;
    s->hostile_count = gXStatus.hostile_monster_count;
    s->nearest_engaged_distance = 1e30f;
    s->provoked_hp = -1;
    s->provoked_condition = -1;
    s->provoked_in_combat = -1;
    s->provoked_distance = -1.0f;
    if (gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info == 0 || info->fActive == 0 || info->monster == 0)
                continue;
            ++s->active_monsters;
            float distance = (info->monster->GetPosition() - party_position).Length();
            if (info->condition_turns[W8_CONDITION_HOSTILE] != 0)
                ++s->hostile_condition_monsters;
            if (info->fInCombat != 0) {
                ++s->engaged_hostiles;
                if (distance < s->nearest_engaged_distance)
                    s->nearest_engaged_distance = distance;
            }
            if (info->location_id == query->location_id) {
                s->provoked_active = 1;
                s->provoked_hp = static_cast<int>(info->hp_current);
                s->provoked_condition = static_cast<int>(info->highest_condition);
                s->provoked_in_combat = info->fInCombat;
                s->provoked_distance = distance;
            }
        }
    }
    if (g_status_685170.buffers.characters != 0) {
        for (int slot = 0; slot < 8; ++slot) {
            const W8Character* character = &g_status_685170.buffers.characters[slot];
            if (character->in_party != 0)
                s->party_hp_total += character->hp_current;
        }
    }
    s->round_active = g_combat_state != 0 ? g_combat_state->flag_000 : 0;
    s->action_status = g_combat_state != 0 ? g_combat_state->eCombatActionStatus : 0;
    s->action_monster = g_combat_state != 0 && g_combat_state->pActionMonsterInfo != 0
                            ? g_combat_state->pActionMonsterInfo->location_id
                            : -1;
}

struct ExecutorProbe {
    DWORD thread_id;
    unsigned int calls;
};

static void ProbeExecutorOnGameThread(void* opaque)
{
    ExecutorProbe* probe = static_cast<ExecutorProbe*>(opaque);
    probe->thread_id = GetCurrentThreadId();
    ++probe->calls;
}

static bool WaitForEngineReady(unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms) {
        if (*(volatile unsigned char*)&gfGameInitialized && *(HWND volatile*)&ghWindow != 0 &&
            gFileDataBase.pLibraries != 0 && gXStatus.uiItemsInDatabase != 0 &&
            gXStatus.uiMonstersInDatabase != 0 && gXStatus.uiNpcsInDatabase != 0) {
            return true;
        }
        Sleep(5);
    }
    return false;
}

static void PrepareMainGameFixtureOnGameThread(void* opaque)
{
    const char** failure = static_cast<const char**>(opaque);
    // Reproducible integration fixtures; acceptance keeps normal product randomness.
    srand(0x57495a38);
    ResetForNewGame();
    W8Character character;
    W8CharacterCreationState creation;
    InitializeCharacterCreation(&character, &creation);
    SetCharacterRace(&character, &creation, 0);
    SetCharacterGender(&character, &creation, W8_GENDER_MALE);
    RebuildLevelUpPoolsForProfession(&character, &creation, W8_PROFESSION_FIGHTER);
    for (int attribute = 0; attribute < 7; ++attribute) {
        AdjustAllocatedAttribute(&character, &creation, attribute,
                                 creation.attribute_points_remaining);
    }
    for (unsigned int skill = 0; skill < 0x29; ++skill) {
        if (character.skills[skill].flag_00) {
            InitializeLevelUpAttributePool(&character, &creation, skill,
                                           creation.skill_points_remaining);
        }
    }
    if (!creation.attributes_complete || !creation.skills_complete) {
        *failure = "character-allocation-incomplete";
        return;
    }
    DeriveCharacterPersonality004EFA30(&character);
    wcscpy(character.name, L"Fixture");
    wcscpy(character.name_part_2, L"Runtime");
    FinalizeCreatedCharacter(&character, &creation, true);
    if (character.hp_current <= 0) {
        *failure = "character-not-alive";
        return;
    }
    for (int member = 0; member < 6; ++member) {
        character.name[0] = L'A' + member;
        if (AddCharacterToParty(&character, -1) < 0) {
            *failure = "party-member-add-failed";
            return;
        }
    }
    // Before the first screen enters there is no screen to leave. Once one
    // exists, use the same leave/replace route as the party-start control.
    RunNewGameOpeningSequence(g_current_screen_state.id != -1, 0);
}

static void CheckGameplayReadyOnGameThread(void* opaque)
{
    bool* ready = static_cast<bool*>(opaque);
    // StartCombat and QuickSave require ground contact, not merely an entered screen.
    *ready = g_current_screen_state.id == W8_SCREEN_MAIN_GAME && g_pending_screen_state.id == -1 &&
             g_mgs_keyboard != 0 && g_level_block != 0 && !g_level_block->flag_328 &&
             !g_level_block->review_transition_active && g_level_data_00652dac != 0 &&
             GetLevelDataFlag4() && !IsScreenInputBlocked();
}

static bool WaitForGameplay(unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms && gfProgramIsRunning) {
        bool ready = false;
        if (!RunOnGameThread(CheckGameplayReadyOnGameThread, &ready))
            return false;
        if (ready) {
            g_observation.main_game_entered = 1;
            ReportStep("main-game-entered");
            return true;
        }
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_INTRO) {
            SendScenarioKey(VK_ESCAPE);
        }
        Sleep(20);
    }
    return false;
}

static DWORD PrepareMainGameFixture()
{
    const char* failure = 0;
    if (!RunOnGameThread(PrepareMainGameFixtureOnGameThread, &failure)) {
        return FailScenario("main-game-fixture", "game-thread-executor-failed");
    }
    if (failure != 0) {
        return FailScenario("main-game-fixture", failure);
    }
    if (!WaitForGameplay(10000)) {
        return FailScenario("main-game-fixture", "main-game-not-ready");
    }
    return 0;
}

static DWORD FinishGameplayScenario()
{
    gfProgramIsRunning = 0;
    PostMessage(ghWindow, WM_NULL, 0, 0);
    return 0;
}

static bool SendGameplayCommand(int command, bool release)
{
    int index = g_mgs_keyboard != 0 ? g_mgs_keyboard->FindBinding(command) : -1;
    MGSKeyBinding* binding = index >= 0 ? g_mgs_keyboard->GetBinding(index) : 0;
    if (binding == 0) {
        return false;
    }
    WORD keys[] = {VK_SHIFT, VK_CONTROL, VK_MENU};
    unsigned int flags[] = {SHIFT_DOWN, CTRL_DOWN, ALT_DOWN};
    if (!release) {
        for (int i = 0; i < 3; ++i) {
            if (binding->modifiers & flags[i])
                SendScenarioKeyHeld(keys[i], 0);
        }
    }
    SendScenarioKeyHeld(binding->key, release ? 1 : 0);
    if (release) {
        for (int i = 2; i >= 0; --i) {
            if (binding->modifiers & flags[i])
                SendScenarioKeyHeld(keys[i], 1);
        }
    }
    return true;
}

static bool TapGameplayCommand(int command)
{
    return SendGameplayCommand(command, false) && SendGameplayCommand(command, true);
}

struct GameplaySnapshot {
    srVector3T<float> position;
    float yaw;
    float input_motion;
    float world_motion;
    int screen;
    int pending;
    bool combat;
    bool movement_ui;
    unsigned int round_active;
    unsigned int party_action_status;
    int action_status;
    int action_monster;
    int movement_budget;
};

static void ReadGameplaySnapshotOnGameThread(void* opaque)
{
    GameplaySnapshot* s = static_cast<GameplaySnapshot*>(opaque);
    W8CameraAngleRecord yaw, pitch;
    s->position.Set(0, 0, 0);
    s->yaw = s->input_motion = s->world_motion = 0;
    if (g_level_data_00652dac != 0) {
        GetCameraPosition(&s->position);
        GetCameraOrientation(yaw, pitch);
        s->yaw = yaw[0];
        s->input_motion = g_level_data_00652dac->vector_40.Length();
        s->world_motion = g_level_data_00652dac->vector_a0.Length();
    }
    s->screen = g_current_screen_state.id;
    s->pending = g_pending_screen_state.id;
    s->combat = gXStatus.fCombatMode != 0;
    s->movement_ui = gXStatus.fPartyMovementUi != 0;
    s->movement_budget = g_level_block != 0 ? g_level_block->move_budget_2dc : 0;
    s->round_active = g_combat_state != 0 ? g_combat_state->flag_000 : 0;
    s->party_action_status = g_combat_state != 0 ? g_combat_state->uiCurrentPartyActionStatus : 0;
    s->action_status = g_combat_state != 0 ? g_combat_state->eCombatActionStatus : 0;
    s->action_monster = g_combat_state != 0 && g_combat_state->pActionMonsterInfo != 0
                            ? g_combat_state->pActionMonsterInfo->location_id
                            : -1;
}

static bool ReadGameplaySnapshot(GameplaySnapshot& s)
{
    return RunOnGameThread(ReadGameplaySnapshotOnGameThread, &s);
}

static bool MoveParty(int command, bool combat_move = false)
{
    GameplaySnapshot before, now;
    if (!ReadGameplaySnapshot(before) || !SendGameplayCommand(command, false))
        return false;
    if (combat_move && !TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND)) {
        SendGameplayCommand(command, true);
        return false;
    }
    bool moved = false;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(now))
            break;
        srVector3T<float> delta = now.position - before.position;
        delta.y = 0;
        if (delta.Length() > 1.0f &&
            (!combat_move || now.movement_budget < before.movement_budget)) {
            moved = true;
            break;
        }
    }
    SendGameplayCommand(command, true);
    if (!moved && ReadGameplaySnapshot(now)) {
        fprintf(stderr,
                "runtime-test movement: command=%d horizontal=(%.2f %.2f) input=%.2f world=%.2f "
                "budget=%d\n",
                command, now.position.x - before.position.x, now.position.z - before.position.z,
                now.input_motion, now.world_motion, now.movement_budget);
    }
    return moved;
}

static DWORD RunAcceptanceMovement()
{
    // Acceptance requires observed motion, not StartCombat's grounding precondition.
    if (!MoveParty(W8_MGS_COMMAND_MOVE_FORWARD)) {
        return FailScenario("party-movement", "forward-motion-not-observed");
    }
    g_observation.party_moved = 1;
    ReportStep("party-moved");
    return FinishGameplayScenario();
}

static DWORD RunExplorationInputScenario()
{
    if (!MoveParty(W8_MGS_COMMAND_MOVE_FORWARD))
        return FailScenario("forward", "motion-not-observed");
    g_observation.party_moved = 1;
    ReportStep("party-moved");
    if (!MoveParty(W8_MGS_COMMAND_MOVE_BACKWARD))
        return FailScenario("backward", "motion-not-observed");
    g_observation.party_moved_backward = 1;
    ReportStep("party-moved-backward");
    GameplaySnapshot before, now;
    if (!ReadGameplaySnapshot(before) || !SendGameplayCommand(W8_MGS_COMMAND_TURN_LEFT, false)) {
        return FailScenario("turn", "binding-or-snapshot-failed");
    }
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(now))
            break;
        if (fabsf(now.yaw - before.yaw) > 0.01f) {
            g_observation.party_turned = 1;
            ReportStep("party-turned");
            break;
        }
    }
    SendGameplayCommand(W8_MGS_COMMAND_TURN_LEFT, true);
    if (!g_observation.party_turned)
        return FailScenario("turn", "orientation-not-changed");
    return FinishGameplayScenario();
}

static DWORD RunSaveLoadMoveScenario()
{
    if (!MoveParty(W8_MGS_COMMAND_MOVE_FORWARD))
        return FailScenario("before-save", "motion-not-observed");
    GameplaySnapshot saved, now;
    if (!ReadGameplaySnapshot(saved))
        return FailScenario("save-position", "snapshot-failed");
    unsigned int started = GetTickCount();
    bool settled = false;
    while (GetTickCount() - started < 2000) {
        Sleep(20);
        if (!ReadGameplaySnapshot(now))
            break;
        if ((now.position - saved.position).Length() < 0.05f) {
            settled = true;
            break;
        }
        saved = now;
    }
    if (!settled)
        return FailScenario("save-position", "camera-not-settled");
    if (!TapGameplayCommand(W8_MGS_COMMAND_QUICK_SAVE))
        return FailScenario("save", "binding-missing");
    started = GetTickCount();
    while (GetTickCount() - started < 5000 && gfProgramIsRunning) {
        for (int slot = 1; slot <= 3; ++slot) {
            char path[64];
            sprintf(path, "Saves\\Quick %d.SAV", slot);
            if (GetFileAttributesA(path) != static_cast<DWORD>(-1) && ReadGameplaySnapshot(now)) {
                g_observation.game_saved = 1;
                break;
            }
        }
        if (g_observation.game_saved)
            break;
        Sleep(20);
    }
    if (!g_observation.game_saved)
        return FailScenario("save", "quick-save-not-written");
    ReportStep("game-saved");
    if (!MoveParty(W8_MGS_COMMAND_MOVE_BACKWARD))
        return FailScenario("after-save", "motion-not-observed");
    if (!TapGameplayCommand(W8_MGS_COMMAND_QUICK_LOAD))
        return FailScenario("load", "binding-missing");
    bool loading = false;
    started = GetTickCount();
    while (GetTickCount() - started < 7000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(now))
            break;
        loading =
            loading || now.screen == W8_SCREEN_PLEASE_WAIT || now.pending == W8_SCREEN_PLEASE_WAIT;
        if (loading && now.screen == W8_SCREEN_MAIN_GAME && now.pending == -1) {
            g_observation.game_loaded = 1;
            srVector3T<float> delta = now.position - saved.position;
            // Compare the serialized horizontal anchor, excluding ground-contact settling.
            delta.y = 0;
            g_observation.load_position_restored = delta.Length() < 1.0f;
            break;
        }
    }
    if (!g_observation.game_loaded || !g_observation.load_position_restored) {
        return FailScenario("load", "saved-position-not-restored");
    }
    ReportStep("load-position-restored");
    if (!MoveParty(W8_MGS_COMMAND_MOVE_FORWARD)) {
        return FailScenario("after-load", "motion-not-observed");
    }
    g_observation.moved_after_load = 1;
    ReportStep("moved-after-load");
    return FinishGameplayScenario();
}

static DWORD RunAutomapRoundtripScenario()
{
    if (!TapGameplayCommand(W8_MGS_COMMAND_AUTOMAP)) {
        return FailScenario("automap-open", "binding-missing");
    }
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_AUTOMAP &&
            *(volatile int*)&g_pending_screen_state.id == -1) {
            g_observation.automap_opened = 1;
            ReportStep("automap-opened");
            break;
        }
        Sleep(20);
    }
    if (!g_observation.automap_opened)
        return FailScenario("automap-open", "not-observed");
    if (!TapGameplayCommand(W8_MGS_COMMAND_AUTOMAP) || !WaitForGameplay(3000)) {
        return FailScenario("automap-close", "main-game-not-restored");
    }
    g_observation.automap_closed = 1;
    ReportStep("automap-closed");
    return FinishGameplayScenario();
}

static bool WaitForCombatMode(bool enabled)
{
    unsigned int started = GetTickCount();
    GameplaySnapshot state;
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(state))
            return false;
        if (state.combat == enabled)
            return true;
    }
    return false;
}

static DWORD RunCombatRoundtripScenario()
{
    if (!TapGameplayCommand(W8_MGS_COMMAND_TOGGLE_COMBAT) || !WaitForCombatMode(true)) {
        return FailScenario("combat-start", "combat-not-entered");
    }
    g_observation.combat_started = 1;
    ReportStep("combat-started");
    if (!TapGameplayCommand(W8_MGS_COMMAND_PARTY_WALK))
        return FailScenario("combat-walk", "binding-missing");
    GameplaySnapshot state;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(state))
            break;
        if (state.movement_ui) {
            g_observation.combat_action_queued = 1;
            break;
        }
    }
    if (!g_observation.combat_action_queued)
        return FailScenario("combat-walk", "action-not-queued");
    ReportStep("combat-action-queued");
    if (!MoveParty(W8_MGS_COMMAND_MOVE_FORWARD, true))
        return FailScenario("combat-walk", "action-did-not-move-party");
    g_observation.combat_party_moved = 1;
    ReportStep("combat-party-moved");
    // START_COMBAT_ROUND finishes the live movement action through BeginFreeTurnPhase.
    if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND))
        return FailScenario("combat-round", "binding-missing");
    started = GetTickCount();
    while (GetTickCount() - started < 5000 && gfProgramIsRunning) {
        Sleep(10);
        if (!ReadGameplaySnapshot(state))
            return FailScenario("combat-round", "snapshot-failed");
        if (!state.round_active)
            break;
    }
    if (state.round_active)
        return FailScenario("combat-round", "round-did-not-finish");
    if (state.combat && !TapGameplayCommand(W8_MGS_COMMAND_TOGGLE_COMBAT))
        return FailScenario("combat-end", "binding-missing");
    if (!WaitForCombatMode(false))
        return FailScenario("combat-end", "combat-not-ended");
    g_observation.combat_ended = 1;
    ReportStep("combat-ended");
    return FinishGameplayScenario();
}

static DWORD RunHostileEncounterScenario()
{
    HostileEncounterContext context;
    if (!RunOnGameThread(ProvokeHostileEncounterOnGameThread, &context)) {
        return FailScenario("hostile-fixture", "game-thread-executor-failed");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");
    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    bool round_requested = false;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 10000 && gfProgramIsRunning) {
        Sleep(5);
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query))
            return FailScenario("hostile-action", "snapshot-failed");
        const HostileEngagementSnapshot& state = query.snapshot;
        // Combat.cpp schedules at status 1, executes at 2, and retires at 3.
        // Requiring execution/retirement for this actor is stronger than distance or HP drift.
        if (state.action_monster == context.location_id && state.action_status >= 2) {
            g_observation.monster_engaged = 1;
            ReportStep("hostile-action-executed");
            return FinishGameplayScenario();
        }
        if (state.round_active)
            round_requested = false;
        if (state.combat_mode && !state.round_active && !round_requested) {
            if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND))
                return FailScenario("hostile-round", "binding-missing");
            round_requested = true;
        }
    }
    /* First missing product transition report: scheduler, engagement and the
       provoked monster's own state narrow where the frontier actually is. */
    fprintf(stderr,
            "runtime-test hostile-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u cond13=%u nearest=%.0f party_hp=%u provoked active=%u hp=%d "
            "cond=%d incombat=%d dist=%.0f combat=%u round=%u action=%d monster=%d\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.hostile_condition_monsters, query.snapshot.nearest_engaged_distance,
            query.snapshot.party_hp_total, query.snapshot.provoked_active,
            query.snapshot.provoked_hp, query.snapshot.provoked_condition,
            query.snapshot.provoked_in_combat, query.snapshot.provoked_distance,
            query.snapshot.combat_mode, query.snapshot.round_active, query.snapshot.action_status,
            query.snapshot.action_monster);
    return FailScenario("hostile-action", "monster-execution-not-observed");
}

static DWORD RunWorldSoakScenario()
{
    GameplaySnapshot state;
    unsigned int started = GetTickCount();
    unsigned int samples = 0;
    while (GetTickCount() - started < 15000 && gfProgramIsRunning) {
        Sleep(100);
        if (!ReadGameplaySnapshot(state) || state.screen != W8_SCREEN_MAIN_GAME) {
            return FailScenario("world-soak", "main-game-lost");
        }
        ++samples;
    }
    if (!gfProgramIsRunning || samples < 2)
        return FailScenario("world-soak", "simulation-stopped");
    g_observation.world_soaked = 1;
    ReportStep("world-soaked");
    return FinishGameplayScenario();
}

enum CharacterFlow { CHARACTER_RETURN, CHARACTER_ACCEPTANCE };

static DWORD RunCharacterFlow(CharacterFlow flow)
{
    /* New Game is the second live menu region. Click its current bounds so
       the scenario uses the ordinary region callback without racing
       several keyboard pairs through SGP's hook in one frame. */
    ClickRegion(g_region_sets[1].first_region + 1);
    /* Entering is only complete after GameLoop clears the pending state and
       the controller has built and enabled the mode-zero character panel.
       The panel's shared region-set slot is the readiness marker. */
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        unsigned int region_set =
            *(volatile unsigned int*)&g_party_selection_character_region_set_69c4f0;
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION &&
            *(volatile int*)&g_pending_screen_state.id == -1 && region_set != 0 &&
            *(volatile unsigned int*)&g_region_sets[region_set].enabled) {
            g_observation.transition_observed = 1;
            break;
        }
        Sleep(10);
    }
    if (!g_observation.transition_observed) {
        return FailScenario("party-selection", "new-game-transition-not-observed");
    }

    /* The left action panel registers its controls in creation order, so
       the first region in its live set is "Create Character". Click the
       centre of that region's current bounds rather than a fixed pixel. */
    unsigned int left_action_set =
        *(volatile unsigned int*)&g_party_selection_left_action_region_set_69c504;
    if (left_action_set == 0 || left_action_set >= g_region_set_count) {
        return FailScenario("character-entry", "create-character-region-set-missing");
    }
    unsigned int create_region =
        *(volatile unsigned int*)&g_region_sets[left_action_set].first_region;
    if (create_region >= g_region_count) {
        return FailScenario("character-entry", "create-character-region-missing");
    }
    W8Region* create_bounds = &g_regions[create_region];
    SendScenarioMouse((create_bounds->x1 + create_bounds->x2) / 2,
                      (create_bounds->y1 + create_bounds->y2) / 2);
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        W8CharacterScreen* screen = *(W8CharacterScreen* volatile*)&g_character_screen_0069c2e8;
        unsigned int page_region_set =
            *(volatile unsigned int*)&g_character_stats_region_set_0069c550;
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_CHARACTER &&
            *(volatile int*)&g_pending_screen_state.id == -1 && screen != 0 &&
            screen->m_pages_1b0c[0] != 0 && page_region_set != 0 &&
            *(volatile unsigned int*)&g_region_sets[page_region_set].enabled) {
            g_observation.character_entered = 1;
            ReportStep("character-entered");
            break;
        }
        Sleep(10);
    }
    if (!g_observation.character_entered) {
        return FailScenario("character-entry", "character-screen-not-entered");
    }

    /* Walk the creation pages through their real controls. The first
       profession record is a non-caster, so the spell page is skipped. */
    W8CharacterScreen* screen = *(W8CharacterScreen* volatile*)&g_character_screen_0069c2e8;
    if (screen == 0 || screen->m_page_index_00c != 0 || screen->m_pages_1b0c[0] == 0) {
        return FailScenario("character-attributes", "initial-page-state-invalid");
    }
    W8CharacterPage005EF778* stats_page =
        static_cast<W8CharacterPage005EF778*>(screen->m_pages_1b0c[0]);
    W8CharacterCreationState* creation = &screen->m_creation_state_187c;

    /* Take the first profession, race and sex record. */
    ClickControl(stats_page->m_profession_row_07c->m_increment_020);
    started = GetTickCount();
    while (stats_page->m_profession_row_07c->m_value_004 == -1) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "profession-selection-timeout");
        Sleep(10);
    }
    ClickControl(stats_page->m_race_row_080->m_increment_020);
    started = GetTickCount();
    while (stats_page->m_race_row_080->m_value_004 == -1) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "race-selection-timeout");
        Sleep(10);
    }
    ClickControl(stats_page->m_gender_row_084->m_increment_020);
    started = GetTickCount();
    while (stats_page->m_gender_row_084->m_value_004 == -1) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "gender-selection-timeout");
        Sleep(10);
    }

    /* The row callback enables the attribute entries once profession and
       race exist. */
    started = GetTickCount();
    while (!stats_page->m_entries_04c.data[0]->m_enabled_03a) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "attribute-controls-not-enabled");
        Sleep(10);
    }

    /* Spend the whole attribute pool through each entry's own increment
       control until the page itself reports the allocation complete. */
    bool progress = true;
    started = GetTickCount();
    while (creation->attributes_complete == 0 && progress && GetTickCount() - started < 20000) {
        progress = false;
        for (int index = 0; index < stats_page->m_entries_04c.count; ++index) {
            W8CharacterPageEntry* entry = stats_page->m_entries_04c.data[index];
            if (entry == 0 || !entry->m_enabled_03a) {
                continue;
            }
            while (true) {
                volatile int* spent = entry->m_second_024;
                volatile int* limit = entry->m_third_028;
                if (*spent >= *limit)
                    break;
                int before = *spent;
                ClickControl(entry->m_increment_008);
                unsigned int click_started = GetTickCount();
                while (*spent == before && GetTickCount() - click_started < 1000) {
                    Sleep(5);
                }
                if (*spent == before)
                    break;
                progress = true;
                if (creation->attributes_complete != 0)
                    break;
            }
            if (creation->attributes_complete != 0)
                break;
        }
    }
    if (creation->attributes_complete == 0) {
        return FailScenario("character-attributes", "attribute-points-not-committed");
    }

    /* Hover the Next control long enough to raise its help box and move
       off it to take the box down before using it. */
    HoverRegion(screen->m_next_1af8->m_region);
    if (WaitForTooltip(true, 2000)) {
        g_observation.tooltip_shown = 1;
    }
    ParkMouseOutsideActiveRegions();
    if (WaitForTooltip(false, 2000)) {
        g_observation.tooltip_removed = 1;
    }

    ClickControl(screen->m_next_1af8);
    started = GetTickCount();
    while (*(volatile int*)&screen->m_page_index_00c != 2) {
        if (GetTickCount() - started > 5000)
            return FailScenario("character-skills", "skill-page-transition-timeout");
        Sleep(10);
    }

    /* Spend the skill pool the same way on the skill page. */
    W8CharacterPage005EF5C8* skills_page = 0;
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        skills_page = static_cast<W8CharacterPage005EF5C8*>(screen->m_pages_1b0c[2]);
        if (skills_page != 0 && skills_page->m_entries_04c.count > 0 &&
            skills_page->m_entries_04c.data[0]->m_enabled_03a) {
            break;
        }
        Sleep(10);
    }
    if (skills_page == 0 || skills_page->m_entries_04c.count == 0) {
        return FailScenario("character-skills", "skill-page-controls-missing");
    }

    /* The skills page's first enabled row raises and clears its own help
       box through the row's help control. */
    for (int index = 0; index < skills_page->m_entries_04c.count; ++index) {
        W8CharacterPageEntry* entry = skills_page->m_entries_04c.data[index];
        if (entry == 0 || !entry->m_enabled_03a) {
            continue;
        }
        HoverRegion(entry->m_help_010->m_region);
        if (WaitForTooltip(true, 2000)) {
            g_observation.skill_tooltip_shown = 1;
        }
        ParkMouseOutsideActiveRegions();
        if (WaitForTooltip(false, 2000)) {
            g_observation.skill_tooltip_removed = 1;
        }
        break;
    }

    progress = true;
    started = GetTickCount();
    while (creation->skills_complete == 0 && progress && GetTickCount() - started < 20000) {
        progress = false;
        for (int index = 0; index < skills_page->m_entries_04c.count; ++index) {
            W8CharacterPageEntry* entry = skills_page->m_entries_04c.data[index];
            if (entry == 0 || !entry->m_enabled_03a || !entry->m_increment_allowed_03b) {
                continue;
            }
            volatile int* spent = entry->m_second_024;
            volatile int* limit = entry->m_third_028;
            if (*spent >= *limit)
                continue;
            int before = *spent;
            ClickControl(entry->m_increment_008);
            unsigned int click_started = GetTickCount();
            while (*spent == before && GetTickCount() - click_started < 1000) {
                Sleep(5);
            }
            if (*spent != before)
                progress = true;
            if (*spent != before) {
                g_observation.skill_interacted = 1;
            }
            if (creation->skills_complete != 0)
                break;
        }
    }
    if (creation->skills_complete == 0) {
        return FailScenario("character-skills", "skill-points-not-committed");
    }

    /* Enter the final page and let its redraw complete: the prepared block
       clearing is the product-side proof that a frame ran. */
    ClickControl(screen->m_next_1af8);
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        W8CharacterPage005EF57C* final_page =
            *(W8CharacterPage005EF57C* volatile*)&screen->m_pages_1b0c[3];
        if (*(volatile int*)&screen->m_page_index_00c == 3 && final_page != 0) {
            g_observation.final_page_entered = 1;
            ReportStep("character-final-page");
            if (final_page->m_prepared_06c == 0) {
                g_observation.final_page_redrawn = 1;
                break;
            }
        }
        Sleep(10);
    }
    if (!g_observation.final_page_entered) {
        return FailScenario("character-final-page", "final-page-not-entered");
    }

    /* Exercise the final page through Wine's real keyboard path.  This
       scenario intentionally reaches the page with both fields empty, so
       the product selects field zero; Tab then transfers focus to the
       second field. */
    if (flow == CHARACTER_RETURN || flow == CHARACTER_ACCEPTANCE) {
        const WORD first_name_keys[] = {'P', 'R', 'O', 'B', 'E'};
        const WORD second_name_keys[] = {'N', 'A', 'M', 'E'};
        for (int first_name_index = 0; first_name_index < 5; ++first_name_index) {
            SendScenarioKey(first_name_keys[first_name_index]);
            Sleep(20);
        }
        SendScenarioKey(VK_TAB);
        Sleep(20);
        for (int second_name_index = 0; second_name_index < 4; ++second_name_index) {
            SendScenarioKey(second_name_keys[second_name_index]);
            Sleep(20);
        }
        started = GetTickCount();
        while (GetTickCount() - started < 3000) {
            if (wcscmp(screen->m_character_018.name_part_2, L"probe") == 0 &&
                wcscmp(screen->m_character_018.name, L"name") == 0) {
                g_observation.character_name_typed = 1;
                break;
            }
            Sleep(10);
        }
        if (!g_observation.character_name_typed) {
            return FailScenario("character-final-page", "name-input-not-observed");
        }

        if (flow == CHARACTER_RETURN) {
            int voice_sample_region =
                RegionWithHelpText(g_character_page4_region_set_0069c52c, 0xf5);
            if (voice_sample_region < 0) {
                return FailScenario("character-final-page", "voice-sample-control-missing");
            }
            ClickRegion(voice_sample_region);
            started = GetTickCount();
            while (GetTickCount() - started < 3000) {
                if (screen->m_dialog_1b1c != 0) {
                    g_observation.character_summary_opened = 1;
                    break;
                }
                Sleep(10);
            }
            if (!g_observation.character_summary_opened) {
                return FailScenario("character-summary", "summary-not-opened");
            }
            SendScenarioKey(VK_SPACE);
            started = GetTickCount();
            while (GetTickCount() - started < 3000) {
                if (screen->m_dialog_1b1c == 0) {
                    break;
                }
                Sleep(10);
            }
            if (screen->m_dialog_1b1c != 0) {
                return FailScenario("character-summary", "summary-not-closed");
            }
        }
    }

    g_observation.character_page_start = 0;
    g_observation.character_page_after = screen->m_page_index_00c;

    if (flow == CHARACTER_ACCEPTANCE) {
        /* Commit the character through the final page's own next control.
           A broken AdvancePage callback must fail the scenario rather than
           be papered over by calling it directly. */
        ClickControl(screen->m_next_1af8);
        started = GetTickCount();
        while (GetTickCount() - started < 5000) {
            if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION &&
                *(volatile int*)&g_pending_screen_state.id == -1) {
                g_observation.character_committed = 1;
                ReportStep("character-committed");
                break;
            }
            Sleep(10);
        }
        if (!g_observation.character_committed) {
            return FailScenario("character-commit", "character-not-committed");
        }

        /* Return toggles the selected roster row into the active party
           through the party builder's toggle. */
        SendScenarioKey(VK_RETURN);
        started = GetTickCount();
        while (GetTickCount() - started < 5000) {
            if (CountActiveCharacters() != 0) {
                g_observation.character_in_party = 1;
                break;
            }
            Sleep(10);
        }
        if (!g_observation.character_in_party) {
            return FailScenario("party-selection", "committed-character-not-in-party");
        }

        unsigned int bottom_set =
            *(volatile unsigned int*)&g_party_selection_bottom_action_region_set_69c508;
        for (int click = 0; click < 4; ++click) {
            int start_region = RegionWithHelpText(bottom_set, 0x6cb);
            if (start_region < 0) {
                return FailScenario("party-start", "start-party-control-missing");
            }
            ClickRegion(start_region);
            started = GetTickCount();
            while (GetTickCount() - started < 2000) {
                if (*(volatile int*)&g_pending_screen_state.id != -1 ||
                    *(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                    break;
                }
                if (flow == CHARACTER_ACCEPTANCE && g_captured_region_index == 0x138) {
                    // The ordinary fewer-than-six party confirmation.
                    SendScenarioKey(VK_RETURN);
                    unsigned int dialog_started = GetTickCount();
                    while (g_captured_region_index == 0x138 &&
                           GetTickCount() - dialog_started < 1000)
                        Sleep(10);
                    break;
                }
                Sleep(10);
            }
            if (*(volatile int*)&g_pending_screen_state.id != -1 ||
                *(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                break;
            }
        }

        started = GetTickCount();
        int observed_state = -2;
        while (GetTickCount() - started < 30000) {
            if (observed_state != *(volatile int*)&g_current_screen_state.id) {
                observed_state = *(volatile int*)&g_current_screen_state.id;
                fprintf(stderr,
                        "runtime-test new-game state: current=%d pending=%d intro=%lu "
                        "skip=%u router=%d\n",
                        observed_state, *(volatile int*)&g_pending_screen_state.id,
                        *(volatile unsigned long*)&g_intro_video_index,
                        g_status_685170.skip_loose_character_check_2444, g_value_68de50);
                fflush(stderr);
            }
            if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                *(volatile int*)&g_pending_screen_state.id == -1) {
                g_observation.main_game_entered = 1;
                ReportStep("main-game-entered");
                break;
            }
            if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_INTRO) {
                SendScenarioKey(VK_ESCAPE);
                Sleep(250);
                continue;
            }
            Sleep(10);
        }
        if (!g_observation.main_game_entered) {
            return FailScenario("main-game-entry", "main-game-not-entered");
        }
        return RunAcceptanceMovement();
    }

    /* Escape raises the discard dialog; accept it once it is up. */
    SendScenarioKey(VK_ESCAPE);
    started = GetTickCount();
    while (GetTickCount() - started < 2000) {
        if (screen != 0 && screen->m_dialog_1b1c != 0) {
            break;
        }
        Sleep(10);
    }
    SendScenarioKey(VK_RETURN);
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION &&
            *(volatile int*)&g_pending_screen_state.id == -1) {
            g_observation.character_returned = 1;
            break;
        }
        Sleep(10);
    }
    if (!g_observation.character_returned) {
        return FailScenario("character-return", "party-selection-not-restored");
    }

    SendScenarioKey(VK_ESCAPE);
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_MENU &&
            *(volatile int*)&g_pending_screen_state.id == -1) {
            g_observation.return_observed = 1;
            gfProgramIsRunning = 0;
            return 0;
        }
        Sleep(10);
    }
    g_observation.timed_out = 1;
    PostMessage(ghWindow, WM_CLOSE, 0, 0);
    return 2;
}

static DWORD RunCharacterReturnScenario()
{
    return RunCharacterFlow(CHARACTER_RETURN);
}
static DWORD RunMainGameScenario()
{
    return RunCharacterFlow(CHARACTER_ACCEPTANCE);
}
static DWORD RunNpcResetScenario()
{
    NpcStateResetContext context;
    context.result = 0;
    if (!RunOnGameThread(ResetNpcStateOnGameThread, &context)) {
        return FailScenario("npc-state-reset", "game-thread-executor-failed");
    }
    if (context.result < 0) {
        return FailScenario("npc-state-reset", "finish-message-queue-insert-failed");
    }
    g_observation.npc_state_reset_ok = context.result != 0;
    if (!g_observation.npc_state_reset_ok) {
        return FailScenario("npc-state-reset", "live-session-state-not-cleared");
    }
    return 0;
}

static DWORD RunMenuExitScenario()
{
    SendScenarioKey(VK_NEXT, KEYEVENTF_EXTENDEDKEY);
    SendScenarioKey(VK_RETURN);
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_EXIT) {
            break;
        }
        Sleep(10);
    }
    /* The exit screen only clears the loop flag for input its regions do
       not consume, so a key arriving after the transition ends the run the
       way a held key's auto-repeat does on retail. */
    SendScenarioKey(VK_RETURN);
    started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        if (*(volatile unsigned char*)&gfProgramIsRunning == 0) {
            g_observation.exit_observed = 1;
            return 0;
        }
        Sleep(10);
    }
    g_observation.timed_out = 1;
    PostMessage(ghWindow, WM_CLOSE, 0, 0);
    return 2;
}

static DWORD RunMenuStartupScenario()
{
    ExecutorProbe probe = {0, 0};
    DWORD expected_thread = GetWindowThreadProcessId(ghWindow, 0);
    for (unsigned int index = 0; index < 100; ++index) {
        if (!RunOnGameThread(ProbeExecutorOnGameThread, &probe) ||
            probe.thread_id != expected_thread || probe.calls != index + 1) {
            return FailScenario("game-thread-executor", "callback-thread-or-count-mismatch");
        }
    }
    ReportStep("game-thread-executor-checked");

    gfProgramIsRunning = 0;
    return 0;
}

static void RunSemanticScenarioOnGameThread(void* opaque)
{
    DWORD* result = static_cast<DWORD*>(opaque);
    *result = g_scenario_spec->run();
    gfProgramIsRunning = 0;
}

static DWORD WINAPI DriveScenario(void*)
{
    if (!WaitForEngineReady(g_scenario_spec->timeout_ms)) {
        return FailScenario("engine-ready", "initialization-timeout");
    }
    g_observation.engine_ready = 1;
    ReportStep("engine-ready");
    if (!InitializeRuntimeGameThreadExecutor(ghWindow)) {
        return FailScenario("game-thread-executor", "install-failed");
    }
    if (g_scenario_spec->phase == RUNTIME_MAIN_MENU) {
        if (!WaitForMainMenu(g_scenario_spec->timeout_ms)) {
            return FailScenario("main-menu", "startup-timeout");
        }
        g_observation.menu_seen = 1;
        ReportStep("main-menu-reached");
        g_observation.menu_state = g_current_screen_state.id;
        g_observation.region_set_enabled = g_region_sets[1].enabled;
        g_observation.first_region = g_region_sets[1].first_region;
        g_observation.last_region = g_region_sets[1].last_region;
        /* The menu music starts on a later frame than the menu state and its
           regions; the observation is only stable once the list is live. */
        unsigned int playlist_started = GetTickCount();
        while (*(volatile unsigned char*)&g_music_playlist_active_65ba7e == 0 &&
               GetTickCount() - playlist_started < 3000) {
            Sleep(10);
        }
        if (!RunOnGameThread(RunMenuChecksOnGameThread, 0)) {
            return FailScenario("main-menu-checks", "game-thread-executor-failed");
        }
    }
    if (g_scenario_spec->kind == RUNTIME_SEMANTIC) {
        DWORD result = 1;
        if (!RunOnGameThread(RunSemanticScenarioOnGameThread, &result)) {
            return FailScenario("semantic-check", "game-thread-executor-failed");
        }
        if (result != 0) {
            return FailScenario("semantic-check", "required-invariant-failed");
        }
        return result;
    }
    if (g_scenario_spec->phase == RUNTIME_MAIN_GAME && PrepareMainGameFixture() != 0) {
        return 1;
    }
    return g_scenario_spec->run();
}

static bool ValidateSemantic(const RuntimeObservation& o)
{
    return o.engine_ready && o.semantic_ok;
}

static bool ValidateMenuStartup(const RuntimeObservation& o)
{
    return o.menu_seen && o.menu_state == W8_SCREEN_MAIN_MENU && o.region_set_enabled &&
           o.playlist_active && o.playlist_tracks > 0 && o.patch_precedence_ok &&
           o.physical_fallback_ok && o.shade_table_ok;
}

static bool ValidateCharacterUi(const RuntimeObservation& o)
{
    return o.transition_observed && o.character_entered && o.final_page_entered &&
           o.final_page_redrawn && o.character_page_start == 0 &&
           o.character_page_after > o.character_page_start && o.tooltip_shown &&
           o.tooltip_removed && o.skill_tooltip_shown && o.skill_tooltip_removed &&
           o.skill_interacted;
}

static bool ValidateCharacterReturn(const RuntimeObservation& o)
{
    return ValidateCharacterUi(o) && o.character_name_typed && o.character_summary_opened &&
           o.character_returned && o.return_observed;
}

static bool ValidateNewGame(const RuntimeObservation& o)
{
    return ValidateCharacterUi(o) && o.character_committed && o.character_in_party &&
           o.main_game_entered;
}

static bool ValidateMainGame(const RuntimeObservation& o)
{
    return ValidateNewGame(o) && o.character_name_typed && o.party_moved;
}

static bool ValidateNpcReset(const RuntimeObservation& o)
{
    return o.main_game_entered && o.npc_state_reset_ok;
}

static bool ValidateMenuExit(const RuntimeObservation& o)
{
    return o.menu_seen && o.exit_observed;
}

static bool ValidateAutomap(const RuntimeObservation& o)
{
    return o.main_game_entered && o.automap_opened && o.automap_closed;
}

static bool ValidateExploration(const RuntimeObservation& o)
{
    return o.main_game_entered && o.party_moved && o.party_moved_backward && o.party_turned;
}

static bool ValidateSaveLoad(const RuntimeObservation& o)
{
    return o.main_game_entered && o.game_saved && o.game_loaded && o.load_position_restored &&
           o.moved_after_load;
}

static bool ValidateCombat(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_started && o.combat_action_queued &&
           o.combat_party_moved && o.combat_ended;
}

static bool ValidateHostile(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_aggroed && o.monster_engaged;
}

static bool ValidateSoak(const RuntimeObservation& o)
{
    return o.main_game_entered && o.world_soaked;
}

static const RuntimeScenario kScenarios[] = {
    {"combat-roundtrip", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunCombatRoundtripScenario, ValidateCombat},
    {"hostile-encounter", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunHostileEncounterScenario, ValidateHostile},
    {"world-soak", RUNTIME_MAIN_GAME, RUNTIME_NIGHTLY, RUNTIME_INTEGRATION, 45000,
     RunWorldSoakScenario, ValidateSoak},
    {"exploration-input", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunExplorationInputScenario, ValidateExploration},
    {"save-load-move", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunSaveLoadMoveScenario, ValidateSaveLoad},
    {"automap-roundtrip", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunAutomapRoundtripScenario, ValidateAutomap},
    {"oct-file", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunOctFileScenario,
     ValidateSemantic},
    {"sight-threshold", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunSightScenario,
     ValidateSemantic},
    {"split-stack", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     RunSplitStackScenario, ValidateSemantic},
    {"party-movement", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     RunPartyMovementScenario, ValidateSemantic},
    {"audio-semantics", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunAudioScenario,
     ValidateSemantic},
    {"mongen", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunMonGenScenario,
     ValidateSemantic},
    {"keyboard-menu", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     RunKeyboardMenuScenario, ValidateSemantic},
    {"mouth-gap", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunMouthGapScenario,
     ValidateSemantic},
    {"npc-dialogue", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunNpcDialogueScenario,
     ValidateSemantic},
    {"search-mode", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunSearchModeScenario,
     ValidateSemantic},
    {"main-menu-startup", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_INTEGRATION, 20000,
     RunMenuStartupScenario, ValidateMenuStartup},
    {"main-menu-exit-auto-repeat", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_ACCEPTANCE, 20000,
     RunMenuExitScenario, ValidateMenuExit},
    {"main-menu-new-game", RUNTIME_MAIN_MENU, RUNTIME_MAIN, RUNTIME_ACCEPTANCE, 30000,
     RunCharacterReturnScenario, ValidateCharacterReturn},
    {"main-game-start", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_ACCEPTANCE, 30000,
     RunMainGameScenario, ValidateMainGame},
    {"npc-state-reset", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 30000,
     RunNpcResetScenario, ValidateNpcReset},
};

static void ListScenarios()
{
    static const char* phases[] = {"engine-ready", "main-menu", "main-game"};
    static const char* tiers[] = {"pr", "main", "nightly"};
    static const char* kinds[] = {"acceptance", "integration", "semantic"};
    printf("name\tphase\ttier\tkind\ttimeout_ms\n");
    for (unsigned int index = 0; index < sizeof(kScenarios) / sizeof(kScenarios[0]); ++index) {
        const RuntimeScenario& scenario = kScenarios[index];
        printf("%s\t%s\t%s\t%s\t%u\n", scenario.name, phases[scenario.phase], tiers[scenario.tier],
               kinds[scenario.kind], scenario.timeout_ms);
    }
}

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--list-scenarios") == 0) {
        ListScenarios();
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--scenario") == 0) {
        for (unsigned int index = 0; index < sizeof(kScenarios) / sizeof(kScenarios[0]); ++index) {
            if (strcmp(argv[2], kScenarios[index].name) == 0) {
                g_scenario_spec = &kScenarios[index];
                break;
            }
        }
    }
    if (g_scenario_spec == 0) {
        fprintf(stderr, "usage: Wiz8RuntimeTest --list-scenarios | --scenario NAME\n");
        return 64;
    }
    W8SetCrashContextWriter(WriteRuntimeTestContext);
    g_scenario = g_scenario_spec->name;
    g_scenario_started = GetTickCount();
    memset(&g_observation, 0, sizeof(g_observation));
    g_observation.menu_state = -1;
    HANDLE driver = CreateThread(NULL, 0, DriveScenario, NULL, 0, NULL);
    if (driver == NULL) {
        fprintf(stderr, "could not start in-process scenario driver\n");
        return 70;
    }

    char command_line[] = "";
    int game_status = WinMain(GetModuleHandle(NULL), NULL, command_line, SW_SHOWNORMAL);
    fprintf(stderr,
            "WIZ8_RUNTIME_STEP scenario=%s step=winmain-returned state=pass "
            "elapsed_ms=%lu status=%d\n",
            g_scenario, GetTickCount() - g_scenario_started, game_status);
    fflush(stderr);
    /* Python enforces the process deadline while WinMain or this join runs. */
    if (WaitForSingleObject(driver, g_scenario_spec->timeout_ms) != WAIT_OBJECT_0) {
        fprintf(stderr,
                "WIZ8_RUNTIME_FAILURE scenario=%s step=shutdown reason=driver-join-timeout\n",
                g_scenario);
        fflush(stderr);
        TerminateProcess(GetCurrentProcess(), 1);
        return 1;
    }
    DWORD driver_status = 2;
    GetExitCodeThread(driver, &driver_status);
    CloseHandle(driver);
    ShutdownRuntimeGameThreadExecutor();

    /* Keep the scenario result on stderr before teardown: a teardown failure
       must not erase whether the flow itself reached its goal. */
    fprintf(stderr,
            "runtime-test progress: transition=%u entered=%u final=%u redrawn=%u "
            "committed=%u in_party=%u main_game=%u timed_out=%u\n",
            g_observation.transition_observed, g_observation.character_entered,
            g_observation.final_page_entered, g_observation.final_page_redrawn,
            g_observation.character_committed, g_observation.character_in_party,
            g_observation.main_game_entered, g_observation.timed_out);
    fflush(stderr);

    /* TerminateProcess below deliberately bypasses the CRT atexit chain, so
       invoke the registered product exit hook explicitly. */
    SGPExit();
    const bool teardown_ok = g_cursor_node_659694 == NULL && gFileDataBase.pLibraries == NULL &&
                             gFileDataBase.RealFiles.pRealFilesOpen == NULL;

    printf(
        "WIZ8_RUNTIME_TEST scenario=%s engine_ready=%u semantic_ok=%u menu_seen=%u menu_state=%d "
        "regions_enabled=%u first_region=%u last_region=%u "
        "playlist_active=%u playlist_tracks=%d playlist_weight=%d "
        "playlist_pause_min=%d playlist_pause_max=%d playlist_pause_chance=%d "
        "patch_catalog_count=%d item_database_count=%u "
        "monster_database_count=%u npc_database_count=%u "
        "patch_precedence_ok=%u physical_fallback_ok=%u "
        "shade_table_ok=%u exit_observed=%u transition_observed=%u "
        "character_entered=%u character_returned=%u "
        "final_page_entered=%u final_page_redrawn=%u "
        "character_name_typed=%u character_summary_opened=%u "
        "character_committed=%u character_in_party=%u main_game_entered=%u "
        "party_moved=%u party_moved_backward=%u moved_after_load=%u party_turned=%u "
        "world_soaked=%u automap_opened=%u automap_closed=%u "
        "game_saved=%u game_loaded=%u load_restored=%u "
        "combat_started=%u combat_action_queued=%u combat_party_moved=%u combat_ended=%u "
        "combat_aggroed=%u monster_engaged=%u "
        "return_observed=%u teardown=%u timed_out=%u "
        "npc_state_reset_ok=%u "
        "character_page_start=%d character_page_after=%d "
        "tooltip_shown=%u tooltip_removed=%u "
        "skill_tooltip_shown=%u skill_tooltip_removed=%u "
        "skill_interacted=%u\n",
        g_scenario, g_observation.engine_ready, g_observation.semantic_ok, g_observation.menu_seen,
        g_observation.menu_state, g_observation.region_set_enabled, g_observation.first_region,
        g_observation.last_region, g_observation.playlist_active, g_observation.playlist_tracks,
        g_observation.playlist_weight, g_observation.playlist_pause_min,
        g_observation.playlist_pause_max, g_observation.playlist_pause_chance,
        g_observation.patch_catalog_count, g_observation.item_database_count,
        g_observation.monster_database_count, g_observation.npc_database_count,
        g_observation.patch_precedence_ok, g_observation.physical_fallback_ok,
        g_observation.shade_table_ok, g_observation.exit_observed,
        g_observation.transition_observed, g_observation.character_entered,
        g_observation.character_returned, g_observation.final_page_entered,
        g_observation.final_page_redrawn, g_observation.character_name_typed,
        g_observation.character_summary_opened, g_observation.character_committed,
        g_observation.character_in_party, g_observation.main_game_entered,
        g_observation.party_moved, g_observation.party_moved_backward,
        g_observation.moved_after_load, g_observation.party_turned, g_observation.world_soaked,
        g_observation.automap_opened, g_observation.automap_closed, g_observation.game_saved,
        g_observation.game_loaded, g_observation.load_position_restored,
        g_observation.combat_started, g_observation.combat_action_queued,
        g_observation.combat_party_moved, g_observation.combat_ended, g_observation.combat_aggroed,
        g_observation.monster_engaged, g_observation.return_observed, teardown_ok ? 1 : 0,
        g_observation.timed_out, g_observation.npc_state_reset_ok,
        g_observation.character_page_start, g_observation.character_page_after,
        g_observation.tooltip_shown, g_observation.tooltip_removed,
        g_observation.skill_tooltip_shown, g_observation.skill_tooltip_removed,
        g_observation.skill_interacted);

    const int result =
        driver_status == 0 && g_scenario_spec->validate(g_observation) && teardown_ok ? 0 : 1;
    if (result != 0) {
        fprintf(stderr,
                "WIZ8_RUNTIME_FAILURE scenario=%s step=validation "
                "reason=required-observations-or-teardown-failed\n",
                g_scenario);
        fflush(stderr);
    }
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), result);
    return result;
}
