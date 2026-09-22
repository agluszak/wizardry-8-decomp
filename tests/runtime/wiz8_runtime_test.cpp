/* Enable the pinned SDK's SendInput declarations for the harness only. */
#define _WIN32_WINNT 0x0500
#include "game_thread_executor.h"
#include "runtime_scenario.h"
#include "wiz8/regions.h"
#include "wiz8/layouts/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/local_screens/AutomapScreen.h"
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
#include "wiz8/local_screens/MGSSpellCasting.h"
#include "wiz8/local_screens/MGSKeyboard.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/utility.h"
#include "wiz8/local_code/Combat.h"
#include "wiz8/local_code/CombatPartyMovement.h"
#include "wiz8/local_code/CombatHostility.h"
#include "wiz8/local_code/CombatRange.h"
#include "wiz8/local_code/Targeting.h"
#include "wiz8/local_code/Magic.h"
#include "wiz8/local_code/MonsterGroup.h"
#include "wiz8/local_code/MonsterManager.h"
#include "wiz8/local_code/Sight.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/OctPath.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/startup_world.h"
#include "wiz8/3d_code/IList.h"
#include "wiz8/local_code/Traps.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/GameTimeAccumulator0043A910.h"
#include "wiz8/engine_code/Levels.h"
#include "wiz8/local_screens/mipe.h"
#include "wiz8/layouts/main_game_screen.h"
#include "wiz8/fonts.h"
#include "wiz8/notices.h"
#include "wiz8_crash_report.h"
#include "runtime_case.h"
#include "runtime_fixture.h"
#include "gameplay_actions.h"
#include "oct_file_semantic_test.h"
#include "keyboard_menu_semantic_test.h"
#include "npc_dialogue_semantic_test.h"
#include "lock_device_semantic_test.h"
#include "mongen_semantic_test.h"
#include "mouth_gap_semantic_test.h"
#include "sight_semantic_test.h"
#include "split_stack_semantic_test.h"
#include "party_movement_semantic_test.h"
#include "audio_semantic_test.h"
#include "combat_actions.h"

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
/* One or more scenarios to run in this process; g_scenario_spec always points
   at the in-flight entry. Batches contain only batch=1 entries sharing one
   fixture. */
static const RuntimeScenario* g_scenario_specs[64];
static unsigned int g_scenario_count;

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

/* Product controls and their registered regions are live game state: the
   game thread reads them and copies a centre point out, and the driver only
   sends the OS events at that copied point. */
struct ControlCenterQuery {
    W8TextControl* control;
    int ok;
    int x;
    int y;
};

static void ReadControlCenterOnGameThread(void* opaque)
{
    ControlCenterQuery* query = static_cast<ControlCenterQuery*>(opaque);
    if (query->control == 0 || query->control->m_region < 0 ||
        static_cast<unsigned int>(query->control->m_region) >= g_region_count) {
        query->ok = 0;
        return;
    }
    W8Region* bounds = &g_regions[query->control->m_region];
    query->x = (bounds->x1 + bounds->x2) / 2;
    query->y = (bounds->y1 + bounds->y2) / 2;
    query->ok = 1;
}

static bool ControlCenter(W8TextControl* control, int* x, int* y)
{
    ControlCenterQuery query;
    query.control = control;
    query.ok = 0;
    query.x = 0;
    query.y = 0;
    if (!RunOnGameThread(ReadControlCenterOnGameThread, &query, 5000) || !query.ok) {
        return false;
    }
    *x = query.x;
    *y = query.y;
    return true;
}

/* Click the live centre of a product control through its registered region,
   the same rectangle the input dispatch uses. */
static void ClickControl(W8TextControl* control)
{
    int x;
    int y;
    if (ControlCenter(control, &x, &y)) {
        SendScenarioMouse(x, y);
    }
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

struct RegionCenterQuery {
    int region_index;
    int ok;
    int x;
    int y;
};

static void ReadRegionCenterOnGameThread(void* opaque)
{
    RegionCenterQuery* query = static_cast<RegionCenterQuery*>(opaque);
    if (query->region_index < 0 ||
        static_cast<unsigned int>(query->region_index) >= g_region_count) {
        query->ok = 0;
        return;
    }
    W8Region* region = &g_regions[query->region_index];
    query->x = (region->x1 + region->x2) / 2;
    query->y = (region->y1 + region->y2) / 2;
    query->ok = 1;
}

/* Walk a live region's current bounds instead of a fixed pixel. */
static bool RegionCenter(int region_index, int* x, int* y)
{
    RegionCenterQuery query;
    query.region_index = region_index;
    query.ok = 0;
    query.x = 0;
    query.y = 0;
    if (!RunOnGameThread(ReadRegionCenterOnGameThread, &query, 5000) || !query.ok) {
        return false;
    }
    *x = query.x;
    *y = query.y;
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

struct HelpTextRegionQuery {
    unsigned int region_set;
    int help_text_id;
    int region;
};

static void FindHelpTextRegionOnGameThread(void* opaque)
{
    HelpTextRegionQuery* query = static_cast<HelpTextRegionQuery*>(opaque);
    query->region = -1;
    if (query->region_set >= g_region_set_count) {
        return;
    }
    unsigned int first = g_region_sets[query->region_set].first_region;
    unsigned int last = g_region_sets[query->region_set].last_region;
    for (unsigned int region = first; region <= last && region < g_region_count; ++region) {
        if (g_regions[region].help_text_id == query->help_text_id) {
            query->region = (int)region;
            return;
        }
    }
}

/* The region a control registered for its help text. The party-builder start
   control is the only one in the bottom action panel with help id 0x6cb, so
   the scenario clicks the product control rather than a fixed pixel. */
static int RegionWithHelpText(unsigned int region_set, int help_text_id)
{
    HelpTextRegionQuery query;
    query.region_set = region_set;
    query.help_text_id = help_text_id;
    query.region = -1;
    if (!RunOnGameThread(FindHelpTextRegionOnGameThread, &query, 5000)) {
        return -1;
    }
    return query.region;
}

struct TransitionObjectsProbe {
    int present;
};

static void ProbeTransitionObjectsOnGameThread(void* opaque)
{
    static_cast<TransitionObjectsProbe*>(opaque)->present = HasScreenTransitionObjects() ? 1 : 0;
}

/* The tooltip's owning object count is the structural marker; the pixel
   output is not part of this assertion. */
static bool WaitForTooltip(bool present, unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms) {
        TransitionObjectsProbe probe;
        probe.present = 0;
        if (RunOnGameThread(ProbeTransitionObjectsOnGameThread, &probe, 5000) &&
            (probe.present != 0) == present) {
            return true;
        }
        Sleep(5);
    }
    return false;
}

/* Everything the menu wait needs from the product, copied on the game
   thread; the driver only sends the intro-dismiss key between reads. The
   video pointer is an identity token for debouncing - never dereferenced
   off-thread. */
struct MainMenuCheck {
    int ready;
    int screen;
    unsigned int region_enabled;
    unsigned int first_region;
    unsigned int last_region;
    int playlist_active;
    int game_initialized;
    int app_active;
    int intro_screen;
    W8BinkVideo* video;
};

static void CheckMainMenuOnGameThread(void* opaque)
{
    MainMenuCheck* check = static_cast<MainMenuCheck*>(opaque);
    check->ready = g_current_screen_state.id == W8_SCREEN_MAIN_MENU && ghWindow != NULL &&
                   g_region_sets[1].enabled;
    check->screen = g_current_screen_state.id;
    check->region_enabled = g_region_sets[1].enabled;
    check->first_region = g_region_sets[1].first_region;
    check->last_region = g_region_sets[1].last_region;
    check->playlist_active = g_music_playlist_active_65ba7e;
    check->game_initialized = gfGameInitialized;
    check->app_active = gfApplicationActive;
    check->intro_screen = g_current_screen_state.id == W8_SCREEN_INTRO;
    check->video = gpVideo;
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

static bool OctFileInvariant(void* ctx)
{
    OctFileSemanticResult* result = static_cast<OctFileSemanticResult*>(ctx);
    bool ok = RunOctFileSemanticTests(result) != 0;
    PrintOctFileSemanticResults(result);
    return ok;
}

static bool OctFileCase(RuntimeCase& test)
{
    OctFileSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("oct-file", OctFileInvariant, &result));
    return true;
}

static bool SightInvariant(void* ctx)
{
    SightSemanticResult* result = static_cast<SightSemanticResult*>(ctx);
    bool ok = RunSightSemanticTests(result) != 0;
    PrintSightSemanticResults(result);
    return ok;
}

static bool SightThresholdCase(RuntimeCase& test)
{
    SightSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("sight-threshold", SightInvariant, &result));
    return true;
}

static bool SplitStackInvariant(void* ctx)
{
    SplitStackSemanticResult* result = static_cast<SplitStackSemanticResult*>(ctx);
    bool ok = RunSplitStackSemanticTest(result) != 0;
    PrintSplitStackSemanticResults(result);
    return ok;
}

static bool SplitStackCase(RuntimeCase& test)
{
    SplitStackSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("split-stack", SplitStackInvariant, &result));
    return true;
}

static bool PartyMovementInvariant(void* ctx)
{
    PartyMovementSemanticResult* result = static_cast<PartyMovementSemanticResult*>(ctx);
    bool ok = RunPartyMovementSemanticTest(result) != 0;
    PrintPartyMovementSemanticResults(result);
    return ok;
}

static bool PartyMovementCase(RuntimeCase& test)
{
    PartyMovementSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("party-movement", PartyMovementInvariant, &result));
    return true;
}

static bool AudioInvariant(void* ctx)
{
    AudioSemanticResult* result = static_cast<AudioSemanticResult*>(ctx);
    bool ok = RunAudioSemanticTests(result) != 0;
    PrintAudioSemanticResults(result);
    return ok;
}

static bool AudioSemanticsCase(RuntimeCase& test)
{
    AudioSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("audio-semantics", AudioInvariant, &result));
    return true;
}

static bool KeyboardMenuInvariant(void* ctx)
{
    KeyboardMenuSemanticResult* result = static_cast<KeyboardMenuSemanticResult*>(ctx);
    bool ok = RunKeyboardMenuSemanticTest(result) != 0;
    PrintKeyboardMenuSemanticResults(result);
    return ok;
}

static bool KeyboardMenuCase(RuntimeCase& test)
{
    KeyboardMenuSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("keyboard-menu", KeyboardMenuInvariant, &result));
    return true;
}

static bool MouthGapInvariant(void* ctx)
{
    MouthGapSemanticResult* result = static_cast<MouthGapSemanticResult*>(ctx);
    bool ok = RunMouthGapSemanticTest(result) != 0;
    PrintMouthGapSemanticResults(result);
    return ok;
}

static bool MouthGapCase(RuntimeCase& test)
{
    MouthGapSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("mouth-gap", MouthGapInvariant, &result));
    return true;
}

static bool NpcDialogueInvariant(void* ctx)
{
    NpcDialogueSemanticResult* result = static_cast<NpcDialogueSemanticResult*>(ctx);
    bool ok = RunNpcDialogueSemanticTest(result) != 0;
    PrintNpcDialogueSemanticResults(result);
    return ok;
}

static bool NpcDialogueCase(RuntimeCase& test)
{
    NpcDialogueSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("npc-dialogue", NpcDialogueInvariant, &result));
    return true;
}

static bool LockDeviceInvariant(void* ctx)
{
    LockDeviceSemanticResult* result = static_cast<LockDeviceSemanticResult*>(ctx);
    bool ok = RunLockDeviceSemanticTest(result) != 0;
    PrintLockDeviceSemanticResults(result);
    return ok;
}

static bool LockDeviceCase(RuntimeCase& test)
{
    LockDeviceSemanticResult result;
    RT_REQUIRE(test, test.run_invariant("lock-device", LockDeviceInvariant, &result));
    return true;
}

static bool SearchModeInvariant(void*)
{
    return RunSearchModeSemanticTest() != 0;
}

static bool SearchModeCase(RuntimeCase& test)
{
    RT_REQUIRE(test, test.run_invariant("search-mode", SearchModeInvariant, 0));
    return true;
}

static bool MonGenInvariant(void*)
{
    return RunMonGenSemanticTest() != 0;
}

static bool MonGenCase(RuntimeCase& test)
{
    RT_REQUIRE(test, test.run_invariant("mongen", MonGenInvariant, 0));
    return true;
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
        if (character.skills[skill].active_00) {
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

static bool SendGameplayCommand(int command, bool release);

static bool PrepareMainGameFixture(RuntimeCase& test)
{
    const char* failure = 0;
    if (!test.on_game_thread("main-game-fixture", PrepareMainGameFixtureOnGameThread, &failure,
                             60000)) {
        return false;
    }
    if (failure != 0) {
        FailScenario("main-game-fixture", failure);
        return false;
    }
    /* The load transition pumps few messages on a slow display, so polls are
       sparse; once the world is up the held forward key produces the ground
       contact frame the readiness check is waiting for. */
    GameplayWait ready = test.wait_gameplay_ready(300000, "main-game-ready");
    if (ready == GAMEPLAY_EXECUTOR_UNRESPONSIVE) {
        return false;
    }
    if (ready == GAMEPLAY_NOT_READY) {
        /* Report the last snapshot the game thread itself filled; live state
           is not safe to read from here. */
        const GameplayReadyCheck& last = test.last_ready_check();
        fprintf(stderr,
                "runtime-test readiness: screen=%d pending=%d keyboard=%d "
                "level_block=%d flag328=%d transition=%d level_data=%d flags=%04x "
                "flag4_eff=%d input_blocked=%d camera=(%.0f %.0f %.0f) "
                "timer_flags=%02x paused=%d d1=%d d2=%d scale=%.3f latch=%d "
                "world_blocked=%u ready_calls=%u\n",
                last.screen, last.pending, last.keyboard_present, last.level_block_present,
                last.review_transition_done_328, last.review_transition_active,
                last.level_data_present, last.flags, last.flag4_effective, last.blocked,
                last.camera_x, last.camera_y, last.camera_z, last.timer_flags, last.timer_paused,
                last.timer_d1, last.timer_d2, last.timer_scale, last.ground_latch,
                last.world_update_blocked, last.calls);
        FailScenario("main-game-fixture", "main-game-not-ready");
        return false;
    }
    g_observation.main_game_entered = 1;
    ReportStep("main-game-entered");
    return true;
}

static DWORD FinishGameplayScenario()
{
    gfProgramIsRunning = 0;
    PostMessage(ghWindow, WM_NULL, 0, 0);
    return 0;
}

static void ReleaseHeldGameplayCommands();

static bool EnterEngineReadyFixture(RuntimeCase&)
{
    /* DriveScenario already established engine-ready before fixtures run. */
    return true;
}

static void LeaveFixture(RuntimeCase&) {}

static bool EnterMainMenuFixture(RuntimeCase& test)
{
    /* The menu wait polls a copied MainMenuCheck: product globals are read
       on the game thread, and the driver only sends the intro-dismiss key. */
    unsigned int started = GetTickCount();
    W8BinkVideo* dismissed_video = 0;
    unsigned int dismissed_at = 0;
    MainMenuCheck check;
    memset(&check, 0, sizeof(check));
    while (!check.ready && GetTickCount() - started < test.remaining_ms() && gfProgramIsRunning) {
        if (!test.on_game_thread("main-menu", CheckMainMenuOnGameThread, &check, 5000)) {
            return false;
        }
        unsigned int now = GetTickCount();
        if (!check.ready && check.game_initialized && check.app_active && check.intro_screen &&
            check.video != 0 && (check.video != dismissed_video || now - dismissed_at > 1000)) {
            SendScenarioKey(VK_ESCAPE);
            dismissed_video = check.video;
            dismissed_at = now;
        }
        if (!check.ready) {
            Sleep(10);
        }
    }
    if (!check.ready) {
        return test.fail("main-menu", "startup-timeout");
    }
    g_observation.menu_seen = 1;
    ReportStep("main-menu-reached");
    g_observation.menu_state = check.screen;
    g_observation.region_set_enabled = check.region_enabled;
    g_observation.first_region = check.first_region;
    g_observation.last_region = check.last_region;
    /* The menu music starts on a later frame than the menu state and its
       regions; the observation is only stable once the list is live. */
    unsigned int playlist_started = GetTickCount();
    while (!check.playlist_active && GetTickCount() - playlist_started < 3000) {
        if (!test.on_game_thread("main-menu", CheckMainMenuOnGameThread, &check, 5000)) {
            return false;
        }
        Sleep(10);
    }
    return test.on_game_thread("main-menu-checks", RunMenuChecksOnGameThread, 0, 5000);
}

static bool EnterMonasteryPartyFixture(RuntimeCase& test)
{
    return PrepareMainGameFixture(test);
}

static void LeaveMonasteryPartyFixture(RuntimeCase&)
{
    ReleaseHeldGameplayCommands();
}

static const RuntimeFixtureSpec kFixtures[] = {
    {FIXTURE_ENGINE_READY, "engine-ready", RUNTIME_ENGINE_READY, FIXTURE_PATH_NATURAL,
     EnterEngineReadyFixture, LeaveFixture},
    {FIXTURE_MAIN_MENU, "main-menu", RUNTIME_MAIN_MENU, FIXTURE_PATH_NATURAL, EnterMainMenuFixture,
     LeaveFixture},
    {FIXTURE_MONASTERY_PARTY, "monastery-party", RUNTIME_MAIN_GAME, FIXTURE_PATH_SHORTCUT,
     EnterMonasteryPartyFixture, LeaveMonasteryPartyFixture},
};

const RuntimeFixtureSpec* FindRuntimeFixture(RuntimeFixtureId id)
{
    for (unsigned int index = 0; index < sizeof(kFixtures) / sizeof(kFixtures[0]); ++index) {
        if (kFixtures[index].id == id) {
            return &kFixtures[index];
        }
    }
    return 0;
}

static RuntimeCase* g_case;

/* Held bindings resolved on the game thread ride in this map so the real
   press/release semantics of bound commands survive behind the driver-facing
   SendGameplayCommand name the unmigrated scenarios still use. */
struct HeldGameplayCommand {
    int command;
    HeldCommand* held;
};

static HeldGameplayCommand g_held_commands[8];

/* HeldCommand objects borrow the driver-owned RuntimeCase, so every tracked
   hold must be released before the case goes out of scope; destruction sends
   the matching key-ups. */
static void ReleaseHeldGameplayCommands()
{
    for (int slot = 0; slot < 8; ++slot) {
        if (g_held_commands[slot].held != 0) {
            delete g_held_commands[slot].held;
            g_held_commands[slot].held = 0;
        }
    }
}

static bool SendGameplayCommand(int command, bool release)
{
    if (g_case == 0) {
        return false;
    }
    if (!release) {
        for (int slot = 0; slot < 8; ++slot) {
            if (g_held_commands[slot].held != 0 && g_held_commands[slot].command == command) {
                g_held_commands[slot].held->repeat();
                return true;
            }
        }
        for (int free_slot = 0; free_slot < 8; ++free_slot) {
            if (g_held_commands[free_slot].held == 0) {
                HeldCommand* held = new HeldCommand(*g_case, command);
                if (!held->begin()) {
                    delete held;
                    return false;
                }
                g_held_commands[free_slot].command = command;
                g_held_commands[free_slot].held = held;
                return true;
            }
        }
        return false;
    }
    for (int held_slot = 0; held_slot < 8; ++held_slot) {
        if (g_held_commands[held_slot].held != 0 && g_held_commands[held_slot].command == command) {
            HeldCommand* held = g_held_commands[held_slot].held;
            g_held_commands[held_slot].held = 0;
            delete held;
            return true;
        }
    }
    /* A key-up for an untracked command still needs the binding resolved on
       the game thread, so an interrupted hold cannot leave a key stuck. */
    CommandBinding binding;
    if (!g_case->resolve_binding(command, binding, "input") || binding.key == 0) {
        return false;
    }
    SendScenarioKeyHeld(binding.key, 1);
    return true;
}

static bool TapGameplayCommand(int command)
{
    return g_case != 0 && g_case->tap(command, "tap");
}

static bool ReadGameplaySnapshot(GameplaySnapshot& s)
{
    return g_case != 0 && g_case->snapshot(s);
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
    unsigned int last_repeat = 0;
    while (GetTickCount() - started < 3000 && gfProgramIsRunning) {
        Sleep(10);
        /* Injected key presses do not autorepeat, and a bound command only
           feeds the motion accumulator while the driver keeps pressing it.
           Re-send the down event the way a physical held key would. */
        if (GetTickCount() - last_repeat > 30) {
            last_repeat = GetTickCount();
            SendGameplayCommand(command, false);
        }
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
    if (!moved && g_case != 0 && g_case->has_snapshot()) {
        const GameplaySnapshot& last = g_case->last_snapshot();
        fprintf(stderr,
                "runtime-test movement: command=%d horizontal=(%.2f %.2f) input=%.2f world=%.2f "
                "budget=%d modal=%u blocked=%u render_flags=%02x held_key=%u held_down=%u\n",
                command, last.position.x - before.position.x, last.position.z - before.position.z,
                last.input_motion, last.world_motion, last.movement_budget,
                last.modal_owner_present, last.world_update_blocked, last.world_render_flags,
                last.held_key, last.held_key_down);
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

static bool AutomapRoundtripCase(RuntimeCase& test)
{
    RT_REQUIRE(test, OpenAutomap(test));
    RT_REQUIRE(test, CloseAutomap(test));
    return true;
}

static bool ExplorationInputCase(RuntimeCase& test)
{
    RT_REQUIRE(test, MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_FORWARD, "party-moved"));
    RT_REQUIRE(test,
               MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_BACKWARD, "party-moved-backward"));
    RT_REQUIRE(test, TurnUntilYawChanged(test, W8_MGS_COMMAND_TURN_LEFT, "party-turned"));
    return true;
}

static bool SaveLoadMoveCase(RuntimeCase& test)
{
    RT_REQUIRE(test, MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_FORWARD, "moved-before-save"));
    RuntimeCheckpoint saved;
    RT_REQUIRE(test, QuickSave(test, saved));
    RT_REQUIRE(test, MoveAwayFrom(test, W8_MGS_COMMAND_MOVE_BACKWARD, saved, "moved-after-save"));
    RT_REQUIRE(test, QuickLoad(test, saved));
    RT_REQUIRE(test, ExpectRestoredPosition(test, saved));
    RT_REQUIRE(test, MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_FORWARD, "moved-after-load"));
    return true;
}

static bool WorldSoakCase(RuntimeCase& test)
{
    /* A sustained-condition wait is the inverse of wait_until: every sample
       must keep the world on the main-game screen until the budget elapses. */
    unsigned int started = GetTickCount();
    unsigned int samples = 0;
    while (GetTickCount() - started < 15000 && gfProgramIsRunning) {
        Sleep(100);
        GameplaySnapshot state;
        if (!test.snapshot(state, "world-soak") || state.screen != W8_SCREEN_MAIN_GAME) {
            return test.failed() ? false : test.fail("world-soak", "main-game-lost");
        }
        ++samples;
    }
    if (!gfProgramIsRunning || samples < 2) {
        return test.fail("world-soak", "simulation-stopped");
    }
    test.step("world-soaked");
    return true;
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
    while (stats_page->m_profession_row_07c->m_index_004 == -1) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "profession-selection-timeout");
        Sleep(10);
    }
    ClickControl(stats_page->m_race_row_080->m_increment_020);
    started = GetTickCount();
    while (stats_page->m_race_row_080->m_index_004 == -1) {
        if (GetTickCount() - started > 3000)
            return FailScenario("character-attributes", "race-selection-timeout");
        Sleep(10);
    }
    ClickControl(stats_page->m_gender_row_084->m_increment_020);
    started = GetTickCount();
    while (stats_page->m_gender_row_084->m_index_004 == -1) {
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
                        g_status_685170.skip_loose_character_check_2444, g_wiz7_ending_68de50);
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
static bool NpcResetCase(RuntimeCase& test)
{
    NpcStateResetContext context;
    context.result = 0;
    RT_REQUIRE(test,
               test.on_game_thread("npc-state-reset", ResetNpcStateOnGameThread, &context, 60000));
    if (context.result < 0) {
        return test.fail("npc-state-reset", "finish-message-queue-insert-failed");
    }
    if (context.result == 0) {
        return test.fail("npc-state-reset", "live-session-state-not-cleared");
    }
    test.step("npc-state-cleared");
    return true;
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

static void PrintBatchObservation(const RuntimeScenario* spec, unsigned char case_passed)
{
    const RuntimeFixtureSpec* fixture = FindRuntimeFixture(spec->fixture);
    printf("WIZ8_RUNTIME_TEST scenario=%s engine_ready=%u case_passed=%u fixture=%s path=%s\n",
           spec->name, g_observation.engine_ready, case_passed,
           fixture != 0 ? fixture->name : "unknown",
           fixture != 0 && fixture->path == FIXTURE_PATH_SHORTCUT ? "shortcut" : "natural");
    fflush(stdout);
}

/* A failed case or batch abort must not contaminate the cases still queued in
   this process: between cases the executor answers a probe on the game thread,
   otherwise the batch stops and the runner re-runs the rest in a fresh
   process. */
static bool SessionHealthy()
{
    ExecutorProbe probe = {0, 0};
    return RunOnGameThread(ProbeExecutorOnGameThread, &probe, 5000) && probe.calls == 1;
}

static DWORD WINAPI DriveScenario(void*)
{
    if (!WaitForEngineReady(g_scenario_specs[0]->timeout_ms)) {
        return FailScenario("engine-ready", "initialization-timeout");
    }
    g_observation.engine_ready = 1;
    ReportStep("engine-ready");
    if (!InitializeRuntimeGameThreadExecutor(ghWindow)) {
        return FailScenario("game-thread-executor", "install-failed");
    }
    DWORD result = 0;
    for (unsigned int index = 0; index < g_scenario_count; ++index) {
        g_scenario_spec = g_scenario_specs[index];
        g_scenario = g_scenario_spec->name;
        const RuntimeFixtureSpec* fixture = FindRuntimeFixture(g_scenario_spec->fixture);
        if (fixture == 0) {
            FailScenario("fixture", "fixture-unknown");
            return 1;
        }
        RuntimeCase test(g_scenario, g_scenario_spec->timeout_ms);
        test.set_fixture(fixture->name,
                         fixture->path == FIXTURE_PATH_SHORTCUT ? "shortcut" : "natural");
        g_case = &test;
        bool entered = fixture->enter(test);
        DWORD case_result = 1;
        bool passed = false;
        if (entered && g_scenario_spec->case_run != 0) {
            passed = g_scenario_spec->case_run(test);
            if (test.failed()) {
                passed = false;
            } else if (!passed) {
                test.fail("case", "returned-false-without-failure");
            }
            g_observation.case_passed = passed ? 1 : 0;
            case_result = passed ? 0 : 2;
        } else if (entered && g_scenario_spec->run != 0) {
            case_result = g_scenario_spec->run();
            passed = case_result == 0;
        } else {
            case_result = test.failed() ? 2 : 1;
        }
        fixture->leave(test);
        if (g_scenario_spec->case_run != 0) {
            test.finish(passed);
        }
        g_case = 0;
        if (g_scenario_count > 1) {
            PrintBatchObservation(g_scenario_spec, passed ? 1 : 0);
        }
        if (case_result != 0) {
            result = case_result;
        }
        if (index + 1 < g_scenario_count) {
            /* A failed case poisons this process even when the executor still
               answers its probe: its dirty state must not contaminate the
               cases still queued. The reported case keeps its own result; the
               runner re-runs the unreported remainder in fresh processes. */
            const char* abort_reason = !entered            ? "fixture-enter-failed"
                                       : case_result != 0  ? "case-failed"
                                       : !SessionHealthy() ? "session-unhealthy"
                                                           : 0;
            if (abort_reason != 0) {
                fprintf(stderr, "WIZ8_RUNTIME_BATCH scenario=%s event=aborted reason=%s\n",
                        g_scenario_specs[index + 1]->name, abort_reason);
                fflush(stderr);
                /* Returning without finishing leaves WinMain's loop running
                   until the process deadline; stop the game so teardown and
                   the session record run promptly. */
                FinishGameplayScenario();
                return result != 0 ? result : 1;
            }
        }
    }
    /* The runner owns shutdown; scenarios that already stopped the program
       make this a harmless repeat. */
    FinishGameplayScenario();
    return result;
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

static bool ValidateMenuExit(const RuntimeObservation& o)
{
    return o.menu_seen && o.exit_observed;
}

static bool ValidateCase(const RuntimeObservation& o)
{
    return o.case_passed != 0;
}

static const RuntimeScenario kScenarios[] = {
    {"combat-roundtrip", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, 0, ValidateCase, CombatRoundtripCase, 0},
    {"hostile-encounter", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 300000, 0, ValidateCase, HostileEncounterCase, 0},
    {"combat-attack", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     540000, 0, ValidateCase, CombatAttackCase, 0},
    {"combat-spell", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     180000, 0, ValidateCase, CombatSpellCase, 0},
    {"world-soak", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_NIGHTLY, RUNTIME_INTEGRATION,
     120000, 0, ValidateCase, WorldSoakCase, 0},
    {"exploration-input", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, 0, ValidateCase, ExplorationInputCase, 0},
    {"save-load-move", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     120000, 0, ValidateCase, SaveLoadMoveCase, 0},
    {"automap-roundtrip", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, 0, ValidateCase, AutomapRoundtripCase, 0},
    {"oct-file", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, 0,
     ValidateCase, OctFileCase, 1},
    {"sight-threshold", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, 0, ValidateCase, SightThresholdCase, 1},
    {"split-stack", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     0, ValidateCase, SplitStackCase, 1},
    {"party-movement", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, 0, ValidateCase, PartyMovementCase, 1},
    {"audio-semantics", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, 0, ValidateCase, AudioSemanticsCase, 1},
    {"mongen", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, 0,
     ValidateCase, MonGenCase, 1},
    {"keyboard-menu", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, 0,
     ValidateCase, KeyboardMenuCase, 1},
    {"mouth-gap", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, 0,
     ValidateCase, MouthGapCase, 1},
    {"npc-dialogue", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, 0,
     ValidateCase, NpcDialogueCase, 1},
    {"lock-device", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, 0,
     ValidateCase, LockDeviceCase, 1},
    {"search-mode", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, 0,
     ValidateCase, SearchModeCase, 1},
    {"main-menu-startup", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_INTEGRATION,
     20000, RunMenuStartupScenario, ValidateMenuStartup, 0, 0},
    {"main-menu-exit-auto-repeat", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR,
     RUNTIME_ACCEPTANCE, 20000, RunMenuExitScenario, ValidateMenuExit, 0, 0},
    {"main-menu-new-game", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_MAIN, RUNTIME_ACCEPTANCE,
     30000, RunCharacterReturnScenario, ValidateCharacterReturn, 0, 0},
    {"main-game-start", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_ACCEPTANCE, 30000,
     RunMainGameScenario, ValidateMainGame, 0, 0},
    {"npc-state-reset", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     120000, 0, ValidateCase, NpcResetCase, 0},
};

static void ListScenarios()
{
    static const char* phases[] = {"engine-ready", "main-menu", "main-game"};
    static const char* tiers[] = {"pr", "main", "nightly"};
    static const char* kinds[] = {"acceptance", "integration", "semantic"};
    static const char* paths[] = {"natural", "shortcut"};
    printf("name\tphase\ttier\tkind\ttimeout_ms\tfixture\tpath\tbatch\n");
    for (unsigned int index = 0; index < sizeof(kScenarios) / sizeof(kScenarios[0]); ++index) {
        const RuntimeScenario& scenario = kScenarios[index];
        const RuntimeFixtureSpec* fixture = FindRuntimeFixture(scenario.fixture);
        printf("%s\t%s\t%s\t%s\t%u\t%s\t%s\t%s\n", scenario.name, phases[scenario.phase],
               tiers[scenario.tier], kinds[scenario.kind], scenario.timeout_ms,
               fixture != 0 ? fixture->name : "unknown",
               fixture != 0 ? paths[fixture->path] : "unknown", scenario.batch ? "yes" : "no");
    }
}

static const RuntimeScenario* FindScenario(const char* name)
{
    for (unsigned int index = 0; index < sizeof(kScenarios) / sizeof(kScenarios[0]); ++index) {
        if (strcmp(name, kScenarios[index].name) == 0) {
            return &kScenarios[index];
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--list-scenarios") == 0) {
        ListScenarios();
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "--scenario") == 0) {
        g_scenario_specs[0] = FindScenario(argv[2]);
        g_scenario_count = g_scenario_specs[0] != 0 ? 1 : 0;
    } else if (argc == 3 && strcmp(argv[1], "--scenarios") == 0) {
        /* Same-process batch: a comma list of batch=1 cases sharing one
           fixture. The driver runs them in order and reports each case. */
        char names[1024];
        strncpy(names, argv[2], sizeof(names) - 1);
        names[sizeof(names) - 1] = 0;
        for (char* token = strtok(names, ","); token != 0; token = strtok(0, ",")) {
            const RuntimeScenario* spec = FindScenario(token);
            if (spec == 0 ||
                g_scenario_count == sizeof(g_scenario_specs) / sizeof(g_scenario_specs[0]) ||
                spec->batch == 0 || spec->case_run == 0 ||
                (g_scenario_count != 0 && spec->fixture != g_scenario_specs[0]->fixture)) {
                g_scenario_count = 0;
                break;
            }
            g_scenario_specs[g_scenario_count++] = spec;
        }
    }
    if (g_scenario_count == 0) {
        fprintf(stderr, "usage: Wiz8RuntimeTest --list-scenarios | --scenario NAME | "
                        "--scenarios NAME[,NAME...]\n");
        return 64;
    }
    for (unsigned int index = 0; index < g_scenario_count; ++index) {
        const RuntimeFixtureSpec* fixture = FindRuntimeFixture(g_scenario_specs[index]->fixture);
        if (fixture == 0 || fixture->phase != g_scenario_specs[index]->phase) {
            fprintf(stderr,
                    "WIZ8_RUNTIME_FAILURE scenario=%s step=fixture "
                    "reason=fixture-phase-mismatch line=0\n",
                    g_scenario_specs[index]->name);
            return 2;
        }
    }
    g_scenario_spec = g_scenario_specs[0];
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
    unsigned long join_budget = 30000;
    for (unsigned int spec_index = 0; spec_index < g_scenario_count; ++spec_index) {
        join_budget += g_scenario_specs[spec_index]->timeout_ms;
    }
    if (WaitForSingleObject(driver, join_budget) != WAIT_OBJECT_0) {
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

    if (g_scenario_count == 1)
        printf("WIZ8_RUNTIME_TEST scenario=%s engine_ready=%u menu_seen=%u menu_state=%d "
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
               "party_moved=%u case_passed=%u "
               "return_observed=%u teardown=%u timed_out=%u "
               "character_page_start=%d character_page_after=%d "
               "tooltip_shown=%u tooltip_removed=%u "
               "skill_tooltip_shown=%u skill_tooltip_removed=%u "
               "skill_interacted=%u\n",
               g_scenario, g_observation.engine_ready, g_observation.menu_seen,
               g_observation.menu_state, g_observation.region_set_enabled,
               g_observation.first_region, g_observation.last_region, g_observation.playlist_active,
               g_observation.playlist_tracks, g_observation.playlist_weight,
               g_observation.playlist_pause_min, g_observation.playlist_pause_max,
               g_observation.playlist_pause_chance, g_observation.patch_catalog_count,
               g_observation.item_database_count, g_observation.monster_database_count,
               g_observation.npc_database_count, g_observation.patch_precedence_ok,
               g_observation.physical_fallback_ok, g_observation.shade_table_ok,
               g_observation.exit_observed, g_observation.transition_observed,
               g_observation.character_entered, g_observation.character_returned,
               g_observation.final_page_entered, g_observation.final_page_redrawn,
               g_observation.character_name_typed, g_observation.character_summary_opened,
               g_observation.character_committed, g_observation.character_in_party,
               g_observation.main_game_entered, g_observation.party_moved,
               g_observation.case_passed, g_observation.return_observed, teardown_ok ? 1 : 0,
               g_observation.timed_out, g_observation.character_page_start,
               g_observation.character_page_after, g_observation.tooltip_shown,
               g_observation.tooltip_removed, g_observation.skill_tooltip_shown,
               g_observation.skill_tooltip_removed, g_observation.skill_interacted);

    /* Batch mode already emitted one WIZ8_RUNTIME_TEST line per case from the
       driver; its verdict is the per-case results plus teardown. */
    const int result =
        g_scenario_count > 1
            ? (driver_status == 0 && teardown_ok ? 0 : 1)
            : (driver_status == 0 && g_scenario_spec->validate(g_observation) && teardown_ok ? 0
                                                                                             : 1);
    if (result != 0) {
        fprintf(stderr,
                "WIZ8_RUNTIME_FAILURE scenario=%s step=validation "
                "reason=required-observations-or-teardown-failed\n",
                g_scenario);
        fflush(stderr);
    }
    /* The session record is the authoritative teardown verdict: per-case
       lines already say how each case ended, and the exit code alone cannot
       say whether final SGPExit teardown held. */
    printf("WIZ8_RUNTIME_SESSION cases=%u driver=%lu teardown=%u\n", g_scenario_count,
           driver_status, teardown_ok ? 1u : 0u);
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), result);
    return result;
}
