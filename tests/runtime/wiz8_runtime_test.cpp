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

static DWORD RunLockDeviceScenario()
{
    LockDeviceSemanticResult lock_result;
    g_observation.semantic_ok = RunLockDeviceSemanticTest(&lock_result);
    PrintLockDeviceSemanticResults(&lock_result);
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
    {
        W8PathingService* pathing = g_pathing_00659c60;
        fprintf(stderr,
                "runtime-test pathing: svc=%p size=%d grid=%f bounds=(%.0f %.0f %.0f)-(%.0f %.0f "
                "%.0f)\n",
                (void*)pathing, pathing != 0 ? pathing->size_004 : -1,
                pathing != 0 ? pathing->grid_scale_01c : 0.0f,
                pathing != 0 ? pathing->level_bounds[0] : 0.0f,
                pathing != 0 ? pathing->level_bounds[1] : 0.0f,
                pathing != 0 ? pathing->level_bounds[2] : 0.0f,
                pathing != 0 ? pathing->level_bounds[3] : 0.0f,
                pathing != 0 ? pathing->level_bounds[4] : 0.0f,
                pathing != 0 ? pathing->level_bounds[5] : 0.0f);
        if (pathing != 0) {
            srVector3T<float> probe = party_position;
            unsigned char snap = pathing->SnapWaypointPosition00462E60(&probe, 0);
            fprintf(stderr, "runtime-test pathing: party snap=%d y=%f\n", snap, probe.y);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* mi = MonsterGetScriptPartByLocationIndex(i);
                if (mi != 0 && mi->fActive != 0 && mi->monster != 0) {
                    srVector3T<float> mp = mi->monster->GetPosition();
                    unsigned char msnap = pathing->SnapWaypointPosition00462E60(&mp, 0);
                    fprintf(stderr,
                            "runtime-test pathing: monster=%u pos=(%.0f %.0f %.0f) snap=%d y=%f\n",
                            i, mp.x, mp.y, mp.z, msnap, mp.y);
                    if (i > 5)
                        break;
                }
            }
        }
    }
    W8MonsterInfo* melee_info = 0;
    float melee_distance = 1e30f;
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info != 0 && info->fActive != 0 && info->monster != 0 && info->monster_group_id != 0 &&
            info->condition_turns[13] == 0) {
            float dist = (info->monster->GetPosition() - party_position).Length();
            if (dist < provoked_distance) {
                provoked_distance = dist;
                provoked_info = info;
            }
            /* A monster whose attacks all stop at TOUCH must close inside the
               party's own melee band before it can hit, so both sides' swings
               resolve. Anything with SHORT or better reach can stand off at
               ~1800 and chip the party without ever being hit back. */
            W8MonsterRecord* record = MonsterDBFromSpecies(info->monster_species);
            if (record != 0 && GetBestMonsterAttackRange(record, 0) == W8_RANGE_TOUCH &&
                dist < melee_distance) {
                melee_distance = dist;
                melee_info = info;
            }
        }
    }
    if (melee_info != 0) {
        provoked_info = melee_info;
        provoked_distance = melee_distance;
    }
    if (provoked_info != 0) {
        /* Guarantee the provoked monster drops something on death: retail's
           DropMonsterLoot reads *items.GetAt(0) whenever count <= 1, and an
           all-failed treasure table leaves data[0] uninitialized - a real
           retail crash. Seeding one always-hit slot exercises the adoption
           path instead of reproducing that empty-drop bug. */
        W8MonsterRecord* provoked_record = GetMonsterDataForInfo(provoked_info);
        if (provoked_record != 0) {
            W8MonsterTreasureEntry* treasure = &provoked_record->treasure_1c3.slots[0];
            treasure->type = 0;
            treasure->count = 1;
            treasure->item_id = 0x23c; /* the container item SpawnItem drops */
            treasure->chance = 100;
            treasure->dice.base = 1;
            treasure->dice.count = 0;
            treasure->dice.sides = 0;
        }
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
            /* Flag clear moves the group straight onto the camera position -
               the deterministic placement the scatter path falls short of.
               The scatter picks a random heading each call; single spots can
               fail MoveMonsterGroupToPosition, so retry it the way the summon
               path retries its three distances before giving up. */
            placed = PositionMonsterGroupNearCamera00511050(provoked_group, 0.0f, 0.0f, 0);
            for (int attempt = 0; attempt < 32 && placed == 0; ++attempt) {
                static const float distances[3] = {0.0f, 1500.0f, 3000.0f};
                placed = PositionMonsterGroupNearCamera00511050(provoked_group,
                                                                distances[attempt % 3], 0.0f, 1);
            }
        }
        fprintf(stderr, "runtime-test drop: group=%p placed=%d\n", (void*)provoked_group, placed);
        if (placed == 0 && provoked_info != 0 && provoked_info->monster != 0 &&
            g_pathing_00659c60 != 0) {
            /* The Monastery start point sits off the pathing grid - the snap
               query finds no path cell under the party, so retail's own
               summon placement has nowhere to land the group. Move the party
               onto navigable ground next to the target instead: snap a cell
               near the monster and reinstall the camera and navigator the
               way WorldSetCameraLocation / save-load do. The next move nudge
               re-latches ground contact. */
            srVector3T<float> anchor = provoked_info->monster->GetPosition();
            /* Ring-search path-valid spots around the monster at melee
               distance: SnapWaypointPosition with snap_to_cell=0 only tests
               the enclosing cell, and SettlePositionToGround restores the
               ground height WorldSetCameraLocation needs. */
            static const float radii[4] = {400.0f, 600.0f, 800.0f, 300.0f};
            static const float dirs[8][2] = {{1.0f, 0.0f},  {-1.0f, 0.0f}, {0.0f, 1.0f},
                                             {0.0f, -1.0f}, {0.7f, 0.7f},  {-0.7f, 0.7f},
                                             {0.7f, -0.7f}, {-0.7f, -0.7f}};
            int teleported = 0;
            for (int r = 0; r < 4 && !teleported; ++r) {
                for (int d = 0; d < 8 && !teleported; ++d) {
                    srVector3T<float> nav = anchor;
                    nav.x += dirs[d][0] * radii[r];
                    nav.z += dirs[d][1] * radii[r];
                    if (g_pathing_00659c60->SnapWaypointPosition00462E60(&nav, 0) == 0) {
                        continue;
                    }
                    nav.y = anchor.y + 2000.0f;
                    nav.y = SettlePositionToGround00420BD0(&nav, 0);
                    float cam[3] = {nav.x, nav.y + g_default_world_height_00603ac8, nav.z};
                    WorldSetCameraLocation(GetWorld659AB8(), cam);
                    g_startup_world_659c0c->SetPositionInternal00453590(&nav);
                    RefreshAllSight();
                    party_position = nav;
                    teleported = 1;
                    fprintf(stderr, "runtime-test teleport: party -> (%.0f %.0f %.0f)\n", nav.x,
                            nav.y, nav.z);
                }
            }
        }
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
    unsigned int incapacitated_members;
    unsigned int provoked_active;
    int provoked_hp;
    int provoked_condition;
    int provoked_in_combat;
    float provoked_distance;
    unsigned int combat_mode;
    unsigned int round_active;
    int action_status;
    int action_monster;
    int action_char;
    int queued_attacks;
    int provoked_dead;
    unsigned int engaged_hp_total;
    unsigned int engaged_dead;
    int provoked_threat_state;
    int first_target_type;
    int first_target_monster;
    unsigned int report_count;
    unsigned int report_amount;
    unsigned int report_missed;
    unsigned int aim_active;
    int aim_hp;
    int aim_dead;
    int report_target_type;
    int report_target_monster;
};

struct HostileSnapshotQuery {
    int location_id;
    int aim_location_id;
    HostileEngagementSnapshot snapshot;
};

static void ReadHostileEngagementOnGameThread(void* opaque)
{
    HostileSnapshotQuery* query = static_cast<HostileSnapshotQuery*>(opaque);
    HostileEngagementSnapshot* s = &query->snapshot;
    srVector3T<float> party_position;
    memset(s, 0, sizeof(*s));
    GetCameraPosition(&party_position);
    /* The camera rides the environ's world_height above the party's feet;
       monster distances are ground distances, so measure from the feet. */
    party_position.y -= g_environ_00652DB4 != 0 ? g_environ_00652DB4->world_height_30
                                                : g_default_world_height_00603ac8;
    s->screen = g_current_screen_state.id;
    s->pending = g_pending_screen_state.id;
    s->combat_mode = gXStatus.fCombatMode != 0;
    s->hostile_count = gXStatus.hostile_monster_count;
    s->nearest_engaged_distance = 1e30f;
    s->provoked_hp = -1;
    s->provoked_condition = -1;
    s->provoked_in_combat = -1;
    s->provoked_distance = -1.0f;
    s->aim_hp = -1;
    s->aim_dead = 0;
    s->report_target_type = -1;
    s->report_target_monster = -1;
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
                if (info->hp_current == 0 || info->condition_turns[W8_CONDITION_DEAD] != 0) {
                    ++s->engaged_dead;
                } else {
                    s->engaged_hp_total += info->hp_current;
                }
            }
            if (info->location_id == query->location_id) {
                s->provoked_active = 1;
                s->provoked_hp = static_cast<int>(info->hp_current);
                s->provoked_condition = static_cast<int>(info->highest_condition);
                s->provoked_in_combat = info->fInCombat;
                s->provoked_distance = distance;
                s->provoked_dead = info->condition_turns[W8_CONDITION_DEAD] != 0;
                s->provoked_threat_state = info->party_threat.sight_state_04;
            }
            if (info->location_id == query->aim_location_id) {
                s->aim_active = 1;
                s->aim_hp = static_cast<int>(info->hp_current);
                s->aim_dead =
                    info->condition_turns[W8_CONDITION_DEAD] != 0 || info->hp_current == 0;
            }
        }
    }
    if (g_status_685170.buffers.Char != 0) {
        for (int slot = 0; slot < 8; ++slot) {
            const W8Character* character = &g_status_685170.buffers.Char[slot];
            if (character->fInParty != 0) {
                s->party_hp_total += character->hp_current;
                if (character->hp_current == 0 ||
                    character->highest_condition >= W8_CONDITION_UNCONSCIOUS)
                    ++s->incapacitated_members;
            }
        }
    }
    if (g_status_685170.buffers.XChar != 0) {
        s->first_target_type = -1;
        s->first_target_monster = -1;
        for (int slot = 0; slot < 8; ++slot) {
            const W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
            if (row->fOccupied != 0 && row->action_03d == W8_ACTION_ATTACK) {
                ++s->queued_attacks;
                if (s->first_target_type < 0) {
                    s->first_target_type = row->target_in_combat.iType;
                    s->first_target_monster = row->target_in_combat.iMonsterID;
                }
            }
        }
    }
    s->round_active = g_combat_state != 0 ? g_combat_state->combat_over_000 : 0;
    s->action_status = g_combat_state != 0 ? g_combat_state->eCombatActionStatus : 0;
    s->action_monster = g_combat_state != 0 && g_combat_state->pActionMonsterInfo != 0
                            ? g_combat_state->pActionMonsterInfo->location_id
                            : -1;
    s->action_char = g_combat_state != 0 ? g_combat_state->iActionChar : -1;
    if (g_combat_state != 0) {
        s->report_count = g_combat_state->attack_report.count;
        s->report_amount = g_combat_state->attack_report.amount;
        s->report_missed = g_combat_state->attack_report.missed;
        s->report_target_type = g_combat_state->attack_report.target.iType;
        s->report_target_monster = g_combat_state->attack_report.target.iMonsterID;
    }
}

/* Queue the ordinary melee attack for every living party slot - the same
   ChooseAction call the ATTACK keyboard command dispatches. Slots that
   already hold an attack keep it so the repeat does not churn targeting. */
struct CombatAttackQuery {
    int location_id;
    /* -1 normally; a location_id here is excluded from the aim search so the
       queued attacks switch to a different live in-combat monster. */
    int exclude_location_id;
    int eligible;
    int queued;
    int aimed;
    int aim_location_id;
    int aim_hp;
};

static void QueuePartyAttacksOnGameThread(void* opaque)
{
    CombatAttackQuery* query = static_cast<CombatAttackQuery*>(opaque);
    query->eligible = 0;
    query->queued = 0;
    query->aimed = 0;
    if (g_status_685170.buffers.XChar == 0) {
        return;
    }
    /* Aim each slot at a live target: once the provoked monster dies the
       queued attack needs to retarget or the round swings at a corpse. Fall
       back to the nearest live in-combat monster. */
    int aim_location_id = query->location_id;
    if (gXStatus.plsMonsterList != 0) {
        bool alive = false;
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->fActive != 0 && info->fInCombat != 0 && info->hp_current != 0 &&
                info->condition_turns[W8_CONDITION_DEAD] == 0 &&
                info->location_id == aim_location_id &&
                aim_location_id != query->exclude_location_id) {
                alive = true;
                break;
            }
        }
        if (!alive) {
            srVector3T<float> camera;
            float best = 1e30f;
            GetCameraPosition(&camera);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info == 0 || info->fActive == 0 || info->fInCombat == 0 || info->monster == 0 ||
                    info->hp_current == 0 || info->condition_turns[W8_CONDITION_DEAD] != 0 ||
                    info->location_id == query->exclude_location_id) {
                    continue;
                }
                float distance = (info->monster->GetPosition() - camera).Length();
                if (distance < best) {
                    best = distance;
                    aim_location_id = info->location_id;
                }
            }
        }
    }
    query->aim_location_id = aim_location_id;
    query->aim_hp = -1;
    if (aim_location_id >= 0 && gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->location_id == aim_location_id) {
                query->aim_hp = static_cast<int>(info->hp_current);
                break;
            }
        }
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        ++query->eligible;
        /* The fixture party never passes through the equip paths that run
           CalcAttacks, so hand_attacks[].in_play stays clear and
           CanAnyHandReachTarget refuses every swing. Recompute through the
           product's own entry point. */
        CalcAttacks(character);
        if (row->action_03d != W8_ACTION_ATTACK) {
            ChooseAction(slot, W8_ACTION_ATTACK, -1, 0, 0, 1);
        }
        /* ChooseAction only records the action; the swing resolves against
           target_in_combat, which the player path fills through AimAtTarget.
           Without it the queued attack swings at nothing and can never
           land. */
        if (aim_location_id >= 0 && row->action_03d == W8_ACTION_ATTACK) {
            if (row->target_in_combat.iType != W8_TARGET_KIND_MONSTER ||
                row->target_in_combat.iMonsterID != aim_location_id) {
                W8CombatSlot target;
                memset(&target, 0, sizeof(target));
                target.iChar = -1;
                target.iMonsterID = -1;
                target.iGroupID = -1;
                target.iType = W8_TARGET_KIND_MONSTER;
                target.iMonsterID = aim_location_id;
                AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_IN_COMBAT);
            }
            /* ResolveCharacterAttack validates the swing against the
               out-of-combat block, so both contexts need the target. */
            if (row->target_out_of_combat.iType != W8_TARGET_KIND_MONSTER ||
                row->target_out_of_combat.iMonsterID != aim_location_id) {
                W8CombatSlot target;
                memset(&target, 0, sizeof(target));
                target.iChar = -1;
                target.iMonsterID = -1;
                target.iGroupID = -1;
                target.iType = W8_TARGET_KIND_MONSTER;
                target.iMonsterID = aim_location_id;
                AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            }
            if (row->target_in_combat.iType == W8_TARGET_KIND_MONSTER &&
                row->target_out_of_combat.iType == W8_TARGET_KIND_MONSTER) {
                ++query->aimed;
            }
        }
        if (row->action_03d == W8_ACTION_ATTACK) {
            ++query->queued;
        }
    }
}

/* Pre-wound every living party slot to a single hit point - the fixture
   equivalent of arriving at the encounter already battered - so the first
   landed monster swing crosses the incapacitation threshold and exercises
   the damage-to-condition transition deterministically. */
static void WeakenPartyOnGameThread(void* opaque)
{
    int* weakened = static_cast<int*>(opaque);
    *weakened = 0;
    if (g_status_685170.buffers.XChar == 0 || g_status_685170.buffers.Char == 0) {
        return;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied == 0 || character->fInParty == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        character->hp_current = 1;
        ++*weakened;
    }
}

/* Queue DEFEND on every living party slot - the same ChooseAction call the
   DEFEND command dispatches - so the round resolves monster swings without
   the party killing the attackers first. */
static void QueuePartyDefendOnGameThread(void* opaque)
{
    int* queued = static_cast<int*>(opaque);
    *queued = 0;
    if (g_status_685170.buffers.XChar == 0 || g_status_685170.buffers.Char == 0) {
        return;
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        if (row->action_03d != W8_ACTION_DEFEND) {
            ChooseAction(slot, W8_ACTION_DEFEND, -1, 0, 0, 1);
        }
        if (row->action_03d == W8_ACTION_DEFEND) {
            ++*queued;
        }
    }
}

struct CombatSpellQuery {
    int location_id;
    int spell_id;
    int queued;
    int aimed;
    int aim_location_id;
    int aim_hp;
};

/* Queue a single-enemy combat spell on every living slot: the fixture grants
   the learned flag and spell points directly (the party-builder path never
   produced a caster), then runs the product's own SetCharacterSpell so the
   cast lands in the same state a player-queued one would. The spell_target
   block comes out of AimAtTarget the same way combat melee aims do. */
static void QueuePartySpellsOnGameThread(void* opaque)
{
    CombatSpellQuery* query = static_cast<CombatSpellQuery*>(opaque);
    query->queued = 0;
    query->aimed = 0;
    if (g_status_685170.buffers.XChar == 0 || g_spell_records == 0) {
        return;
    }
    if (query->spell_id <= 0) {
        for (int id = 1; id < W8_SPELL_COUNT; ++id) {
            const W8SpellRuntimeRecord* record = &g_spell_records[id];
            if (record->target_type == W8_TARGET_TYPE_ENEMY && record->spell_point_cost > 0 &&
                record->spell_level <= 2 && record->usable_when <= W8_SPELL_USABLE_IN_COMBAT &&
                record->effect_dice.count != 0) {
                query->spell_id = id;
                break;
            }
        }
        if (query->spell_id <= 0) {
            return;
        }
    }
    int aim_location_id = query->location_id;
    if (gXStatus.plsMonsterList != 0) {
        bool alive = false;
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->fActive != 0 && info->fInCombat != 0 && info->hp_current != 0 &&
                info->condition_turns[W8_CONDITION_DEAD] == 0 &&
                info->location_id == aim_location_id) {
                alive = true;
                break;
            }
        }
        if (!alive) {
            srVector3T<float> camera;
            float best = 1e30f;
            GetCameraPosition(&camera);
            for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
                W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
                if (info == 0 || info->fActive == 0 || info->fInCombat == 0 || info->monster == 0 ||
                    info->hp_current == 0 || info->condition_turns[W8_CONDITION_DEAD] != 0) {
                    continue;
                }
                float distance = (info->monster->GetPosition() - camera).Length();
                if (distance < best) {
                    best = distance;
                    aim_location_id = info->location_id;
                }
            }
        }
    }
    query->aim_location_id = aim_location_id;
    query->aim_hp = -1;
    if (aim_location_id >= 0 && gXStatus.plsMonsterList != 0) {
        for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
            W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
            if (info != 0 && info->location_id == aim_location_id) {
                query->aim_hp = static_cast<int>(info->hp_current);
                break;
            }
        }
    }
    for (int slot = 0; slot < 8; ++slot) {
        W8PartySlotRow* row = &g_status_685170.buffers.XChar[slot];
        W8Character* character = &g_status_685170.buffers.Char[slot];
        if (row->fOccupied == 0 || character->hp_current == 0 ||
            character->highest_condition >= W8_CONDITION_DEAD) {
            continue;
        }
        const W8SpellRuntimeRecord* record = &g_spell_records[query->spell_id];
        character->spell_learned[query->spell_id] = 1;
        if (character->iSPLeft[record->realm] < record->spell_point_cost * 8) {
            character->iSPLeft[record->realm] = record->spell_point_cost * 8;
        }
        if (aim_location_id >= 0) {
            W8CombatSlot target;
            memset(&target, 0, sizeof(target));
            target.iChar = -1;
            target.iMonsterID = -1;
            target.iGroupID = -1;
            target.iType = W8_TARGET_KIND_MONSTER;
            target.iMonsterID = aim_location_id;
            /* The spell dialog holds fSpellCastMode and gpSCSV while a cast is
               picked; without them SetCharacterCombatAction clears the current
               target before SetCharacterSpell can copy it, and the SHARED
               branch of ChooseCombatAction dereferences the view. SHARED
               resolves to the spell block for the selected slot, IN_COMBAT
               for the rest. */
            W8SpellCastingView* held_view = gpSCSV;
            unsigned char held_mode = gXStatus.fSpellCastMode;
            if (gpSCSV == 0) {
                gpSCSV = static_cast<W8SpellCastingView*>(calloc(1, sizeof(W8SpellCastingView)));
            }
            gpSCSV->override_spell_104 = query->spell_id;
            gpSCSV->caster = character;
            gXStatus.fSpellCastMode = 1;
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_SPELL);
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_IN_COMBAT);
            AimAtTarget(slot, &target, W8_TARGETING_CONTEXT_OUT_OF_COMBAT);
            SetCharacterSpell(character, query->spell_id, 1);
            gXStatus.fSpellCastMode = held_mode;
            /* The view block stays allocated: UI code may still read it while
               a cast is in flight, and the real dialog only frees it on close. */
            (void)held_view;
        } else {
            SetCharacterSpell(character, query->spell_id, 1);
        }
        if (row->action_03d == W8_ACTION_CAST_SPELL) {
            ++query->queued;
            if (row->spell_target.iType == W8_TARGET_KIND_MONSTER) {
                ++query->aimed;
            }
        }
    }
}

static void NoopOnGameThread(void* opaque)
{
    (void)opaque;
}

/* Drop the party on the nearest engaged monster's doorstep: a ring of
   snap-tested offsets around its live position, settled to ground, then the
   WorldSetCameraLocation + navigator reinstall save-load uses. The monster
   AI holds a standoff at its attack-band edge (~1000 for TOUCH monsters),
   and the combat movement phase only moves the party through the scripted
   ResetLevelMovement path, so closing with a queued move does not converge
   inside a test budget. */
static void TeleportPartyNearEngagedOnGameThread(void* opaque)
{
    bool* moved = static_cast<bool*>(opaque);
    srVector3T<float> camera;
    srVector3T<float> anchor;
    float best = 1e30f;

    *moved = false;
    GetCameraPosition(&camera);
    if (gXStatus.plsMonsterList == 0 || g_pathing_00659c60 == 0) {
        return;
    }
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info == 0 || info->fActive == 0 || info->monster == 0 || info->fInCombat == 0) {
            continue;
        }
        float distance = (info->monster->GetPosition() - camera).Length();
        if (distance < best) {
            best = distance;
            anchor = info->monster->GetPosition();
        }
    }
    if (best == 1e30f) {
        return;
    }
    static const float radii[4] = {60.0f, 120.0f, 250.0f, 400.0f};
    static const float dirs[8][2] = {{1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f},  {0.0f, -1.0f},
                                     {0.7f, 0.7f}, {-0.7f, 0.7f}, {0.7f, -0.7f}, {-0.7f, -0.7f}};
    for (int r = 0; r < 4 && !*moved; ++r) {
        for (int d = 0; d < 8 && !*moved; ++d) {
            srVector3T<float> nav = anchor;
            nav.x += dirs[d][0] * radii[r];
            nav.z += dirs[d][1] * radii[r];
            if (g_pathing_00659c60->SnapWaypointPosition00462E60(&nav, 0) == 0) {
                continue;
            }
            nav.y = anchor.y + 2000.0f;
            nav.y = SettlePositionToGround00420BD0(&nav, 0);
            /* SettleFrom-above lands on the highest floor under the start
               point; a raised ledge or roof leaves the party out of every
               band, so only accept landings near the monster's own level. */
            if (nav.y - anchor.y > 400.0f || anchor.y - nav.y > 400.0f) {
                continue;
            }
            float cam[3] = {nav.x, nav.y + g_default_world_height_00603ac8, nav.z};
            WorldSetCameraLocation(GetWorld659AB8(), cam);
            g_startup_world_659c0c->SetPositionInternal00453590(&nav);
            *moved = true;
        }
    }
    if (*moved) {
        /* Aim requires party_threat.sight_state_04 == 1 - currently seen - and a
           teleport leaves the sight bookkeeping stale. */
        RefreshAllSight();
        srVector3T<float> after;
        GetCameraPosition(&after);
        fprintf(
            stderr,
            "runtime-test approach-tp: cam=(%.0f %.0f %.0f) anchor=(%.0f %.0f %.0f) dist=%.0f\n",
            after.x, after.y, after.z, anchor.x, anchor.y, anchor.z, (anchor - after).Length());
    }
}

/* Point the camera at the nearest engaged monster so combat movement walks
   the formation toward it - the same SetCameraYawDegrees the facing helpers
   use. Placement can only drop a group on the nearest navigable spot, which
   the spawn area leaves ~1800 units out, inside the monsters' SHORT reach but
   outside the party's TOUCH band. */
static void FaceNearestEngagedOnGameThread(void* opaque)
{
    bool* faced = static_cast<bool*>(opaque);
    srVector3T<float> camera;
    srVector3T<float> target;
    float best = 1e30f;

    *faced = false;
    GetCameraPosition(&camera);
    if (gXStatus.plsMonsterList == 0) {
        return;
    }
    for (unsigned int i = 0; i < PLLength(gXStatus.plsMonsterList); ++i) {
        W8MonsterInfo* info = MonsterGetScriptPartByLocationIndex(i);
        if (info == 0 || info->fActive == 0 || info->monster == 0 || info->fInCombat == 0) {
            continue;
        }
        srVector3T<float> position = info->monster->GetPosition();
        float distance = (position - camera).Length();
        if (distance < best) {
            best = distance;
            target = position;
        }
    }
    if (best == 1e30f) {
        return;
    }
    *faced = true;
    SetCameraYawDegrees(atan2f(target.x - camera.x, target.z - camera.z) * 57.2957795f);
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

/* Schedule a callback within budget. A timeout means the game thread never
   reached the hook; that is not evidence it is stopped or that its objects
   are safe to touch, so the callback never runs on the driver thread. */
static bool ScheduleOnGameThread(const char* step, RuntimeGameThreadCallback callback,
                                 void* context, unsigned long budget_ms)
{
    unsigned int started = GetTickCount();
    if (RunOnGameThread(callback, context, budget_ms))
        return true;
    fprintf(stderr,
            "WIZ8_RUNTIME_SYNC scenario=%s step=%s state=game-thread-unresponsive waited_ms=%lu\n",
            g_scenario, step, GetTickCount() - started);
    fflush(stderr);
    return false;
}

static DWORD PrepareMainGameFixture(RuntimeCase& test)
{
    const char* failure = 0;
    if (!test.on_game_thread("main-game-fixture", PrepareMainGameFixtureOnGameThread, &failure,
                             60000)) {
        return 1;
    }
    if (failure != 0) {
        return FailScenario("main-game-fixture", failure);
    }
    /* The load transition pumps few messages on a slow display, so polls are
       sparse; once the world is up the held forward key produces the ground
       contact frame the readiness check is waiting for. */
    GameplayWait ready = test.wait_gameplay_ready(300000, "main-game-ready");
    if (ready == GAMEPLAY_EXECUTOR_UNRESPONSIVE) {
        return 1;
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
        return FailScenario("main-game-fixture", "main-game-not-ready");
    }
    g_observation.main_game_entered = 1;
    ReportStep("main-game-entered");
    return 0;
}

static DWORD FinishGameplayScenario()
{
    gfProgramIsRunning = 0;
    PostMessage(ghWindow, WM_NULL, 0, 0);
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

struct CombatModeRequest {
    bool enabled;
    bool achieved;
    bool waiting_on_ground;
    bool input_blocked;
};

/* Manual combat entry has a one-frame ground-contact precondition in retail.
   Do the readiness check and command dispatch on the game thread in one
   callback; polling flag4 from the driver and injecting the key afterward
   races ApplyCameraMotion clearing the flag on the next frame. */
static void RequestCombatModeOnGameThread(void* opaque)
{
    CombatModeRequest* request = static_cast<CombatModeRequest*>(opaque);
    request->achieved = (gXStatus.fCombatMode != 0) == request->enabled;
    request->waiting_on_ground = false;
    request->input_blocked = IsScreenInputBlocked() != 0;
    if (request->achieved || request->input_blocked) {
        return;
    }
    if (request->enabled && GetLevelDataFlag4() == 0) {
        request->waiting_on_ground = true;
        return;
    }
    DispatchMGSCommand(W8_MGS_COMMAND_TOGGLE_COMBAT);
    request->achieved = (gXStatus.fCombatMode != 0) == request->enabled;
}

static bool RequestCombatMode(bool enabled, unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    bool nudging = false;
    CombatModeRequest last = {enabled, false, false, false};
    while (GetTickCount() - started < timeout_ms && gfProgramIsRunning) {
        CombatModeRequest request = {enabled, false, false, false};
        unsigned long elapsed = GetTickCount() - started;
        if (elapsed >= timeout_ms) {
            break;
        }
        if (!RunOnGameThread(RequestCombatModeOnGameThread, &request, timeout_ms - elapsed)) {
            break;
        }
        last = request;
        if (request.achieved) {
            if (nudging) {
                SendGameplayCommand(W8_MGS_COMMAND_MOVE_FORWARD, true);
            }
            return true;
        }
        if (enabled && request.waiting_on_ground) {
            nudging = SendGameplayCommand(W8_MGS_COMMAND_MOVE_FORWARD, false) || nudging;
        }
        Sleep(20);
    }
    if (nudging) {
        SendGameplayCommand(W8_MGS_COMMAND_MOVE_FORWARD, true);
    }
    fprintf(stderr,
            "runtime-test combat-mode: desired=%d achieved=%d waiting_on_ground=%d "
            "input_blocked=%d\n",
            enabled, last.achieved, last.waiting_on_ground, last.input_blocked);
    return false;
}

static DWORD RunCombatRoundtripScenario()
{
    if (!RequestCombatMode(true, 3000)) {
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
    if (!RequestCombatMode(false, 3000))
        return FailScenario("combat-end", "combat-not-ended");
    g_observation.combat_ended = 1;
    ReportStep("combat-ended");
    return FinishGameplayScenario();
}

static DWORD RunHostileEncounterScenario()
{
    HostileEncounterContext context;
    if (!ScheduleOnGameThread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                              240000)) {
        return FailScenario("hostile-fixture", "game-thread-unresponsive");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");
    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    query.aim_location_id = context.location_id;
    bool round_requested = false;
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 10000 && gfProgramIsRunning) {
        Sleep(5);
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query))
            return FailScenario("hostile-action", "snapshot-failed");
        const HostileEngagementSnapshot& state = query.snapshot;
        // Combat.cpp schedules at status 1, executes at 2, and retires at 3.
        // Any engaged hostile executing proves the turn pipeline; the provoked
        // monster's allies reach the party first as often as it does.
        if (state.action_monster >= 0 && state.action_status >= 2) {
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

/* Beyond engagement: queue real melee attacks aimed at a live monster on
   every living slot, drive bounded rounds, and require durable HP loss or
   death of that aimed monster after the round starts. The fixture queues only
   party melee against this target; hostile actions target the party, so this
   is durable evidence of a party hit. attack_report is diagnostic only:
   retail consumes and clears it synchronously before the harness can poll it. */
static DWORD RunCombatAttackScenario()
{
    HostileEncounterContext context;
    if (!ScheduleOnGameThread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                              240000)) {
        return FailScenario("hostile-fixture", "game-thread-unresponsive");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");

    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    query.aim_location_id = context.location_id;
    unsigned int last_trace = 0;
    int aim_id = context.location_id;
    int baseline_hp = -1;
    /* Success requires evidence that a party member's attack struck the aimed
       monster and that monster durably lost HP or died. Enemy actions and
       party damage are never evidence. Each round is bounded so a stall in
       the product pipeline fails at the stage that stopped, not at one
       monolithic timeout. */
    const int kMaxRounds = 6;
    for (int round = 0; round < kMaxRounds && gfProgramIsRunning; ++round) {
        /* combat-round-queue: close on the nearest engaged monster if the
           party is outside melee reach, then queue a real attack aimed at a
           live monster on every living slot. */
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query)) {
            return FailScenario("combat-round-queue", "game-thread-unresponsive");
        }
        if (query.snapshot.nearest_engaged_distance > 800.0f) {
            bool moved = false;
            RunOnGameThread(TeleportPartyNearEngagedOnGameThread, &moved);
            fprintf(stderr, "runtime-test approach: moved=%d dist=%.0f\n", moved,
                    query.snapshot.nearest_engaged_distance);
        }
        CombatAttackQuery attack;
        attack.location_id = context.location_id;
        attack.exclude_location_id = -1;
        if (!RunOnGameThread(QueuePartyAttacksOnGameThread, &attack)) {
            return FailScenario("combat-round-queue", "game-thread-unresponsive");
        }
        fprintf(stderr, "runtime-test aim: queued=%d aimed=%d aim=%d hp=%d\n", attack.queued,
                attack.aimed, attack.aim_location_id, attack.aim_hp);
        if (attack.queued == 0) {
            return FailScenario("combat-round-queue", "no-attack-queued");
        }
        if (!g_observation.combat_attack_queued) {
            g_observation.combat_attack_queued = 1;
            ReportStep("combat-attack-queued");
        }
        baseline_hp = attack.aim_hp;
        aim_id = attack.aim_location_id;
        query.aim_location_id = aim_id;

        /* combat-round-start: the injected command must be consumed into an
           active round or the exchange never begins. */
        if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND)) {
            return FailScenario("combat-round-start", "binding-missing");
        }
        unsigned int stage_started = GetTickCount();
        bool round_started = false;
        while (GetTickCount() - stage_started < 15000 && gfProgramIsRunning) {
            Sleep(5);
            if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query)) {
                return FailScenario("combat-round-start", "game-thread-unresponsive");
            }
            if (query.snapshot.round_active != 0) {
                round_started = true;
                break;
            }
        }
        if (!round_started) {
            return FailScenario("combat-round-start", "round-start-not-consumed");
        }

        /* combat-round-resolve: attack_report is transient scratch state and
           ReportCharacterAttackResult0053FB00 clears it before returning.
           Observe the durable consequence instead: this round queued only
           party melee at aim_id, so HP loss/deactivation of that monster is
           the integration proof that a queued party attack landed. */
        stage_started = GetTickCount();
        bool round_finished = false;
        while (GetTickCount() - stage_started < 90000 && gfProgramIsRunning) {
            Sleep(5);
            if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query)) {
                return FailScenario("combat-round-resolve", "game-thread-unresponsive");
            }
            const HostileEngagementSnapshot& state = query.snapshot;
            if (GetTickCount() - last_trace > 2000) {
                last_trace = GetTickCount();
                fprintf(stderr,
                        "runtime-test trace: engaged=%u nearest=%.0f party_hp=%u "
                        "ehp=%u dead=%u combat=%u round=%u action=%d char=%d monster=%d "
                        "aim=%d hp=%d active=%u dead=%d "
                        "report(hits=%u dmg=%u miss=%u target=%d:%d) "
                        "threat=%d target=%d:%d\n",
                        state.engaged_hostiles, state.nearest_engaged_distance,
                        state.party_hp_total, state.engaged_hp_total, state.engaged_dead,
                        state.combat_mode, state.round_active, state.action_status,
                        state.action_char, state.action_monster, aim_id, state.aim_hp,
                        state.aim_active, state.aim_dead, state.report_count, state.report_amount,
                        state.report_missed, state.report_target_type, state.report_target_monster,
                        state.provoked_threat_state, state.first_target_type,
                        state.first_target_monster);
            }
            if (!g_observation.target_damaged && aim_id >= 0 && baseline_hp >= 0 &&
                (state.aim_dead || !state.aim_active ||
                 (state.aim_hp >= 0 && state.aim_hp < baseline_hp))) {
                g_observation.party_attack_hit = 1;
                g_observation.target_damaged = 1;
                ReportStep("party-attack-hit");
                ReportStep("target-damaged");
                fprintf(stderr,
                        "runtime-test hit: target=%d baseline_hp=%d hp=%d active=%u dead=%d "
                        "action=%d char=%d report=%u/%u\n",
                        aim_id, baseline_hp, state.aim_hp, state.aim_active, state.aim_dead,
                        state.action_status, state.action_char, state.report_count,
                        state.report_amount);
            }
            /* A kill ends combat in the same frame: the latched success and
               a finished round outrank the combat-mode drop. */
            if (state.round_active == 0 ||
                (g_observation.party_attack_hit && g_observation.target_damaged)) {
                round_finished = true;
                break;
            }
            if (state.combat_mode == 0) {
                return FailScenario("combat-attack", "combat-ended-without-party-hit");
            }
        }
        if (g_observation.party_attack_hit && g_observation.target_damaged) {
            return FinishGameplayScenario();
        }
        if (!round_finished) {
            return FailScenario("combat-round-resolve", "round-did-not-resolve");
        }
        if (query.snapshot.combat_mode == 0) {
            return FailScenario("combat-attack", "combat-ended-without-party-hit");
        }
    }
    fprintf(stderr,
            "runtime-test attack-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u party_hp=%u nearest=%.0f provoked active=%u hp=%d cond=%d incombat=%d "
            "dead=%d dist=%.0f threat=%d combat=%u round=%u action=%d char=%d monster=%d "
            "aim=%d aim_hp=%d aim_active=%u aim_dead=%d queued=%d target=%d:%d "
            "report(hits=%u dmg=%u miss=%u target=%d:%d) obs(q=%u hit=%u tdmg=%u)\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.party_hp_total, query.snapshot.nearest_engaged_distance,
            query.snapshot.provoked_active, query.snapshot.provoked_hp,
            query.snapshot.provoked_condition, query.snapshot.provoked_in_combat,
            query.snapshot.provoked_dead, query.snapshot.provoked_distance,
            query.snapshot.provoked_threat_state, query.snapshot.combat_mode,
            query.snapshot.round_active, query.snapshot.action_status, query.snapshot.action_char,
            query.snapshot.action_monster, aim_id, query.snapshot.aim_hp, query.snapshot.aim_active,
            query.snapshot.aim_dead, query.snapshot.queued_attacks,
            query.snapshot.first_target_type, query.snapshot.first_target_monster,
            query.snapshot.report_count, query.snapshot.report_amount, query.snapshot.report_missed,
            query.snapshot.report_target_type, query.snapshot.report_target_monster,
            g_observation.combat_attack_queued, g_observation.party_attack_hit,
            g_observation.target_damaged);
    return FailScenario("combat-attack", g_observation.party_attack_hit
                                             ? "target-damage-not-observed"
                                             : "party-attack-hit-not-observed");
}

/* The spell twin of the attack round-trip: queue a learned single-enemy
   spell on every living slot, drive rounds until a party cast executes and
   the aimed monster durably loses HP or dies - and report which product
   transition never arrived on failure. */
static DWORD RunCombatSpellScenario()
{
    HostileEncounterContext context;
    if (!ScheduleOnGameThread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                              240000)) {
        return FailScenario("hostile-fixture", "game-thread-unresponsive");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");

    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    query.aim_location_id = context.location_id;
    bool round_requested = false;
    bool baseline_taken = false;
    int spell_id = 0;
    int aim_id = context.location_id;
    int baseline_hp = -1;
    unsigned int started = GetTickCount();
    unsigned int last_trace = 0;
    while (GetTickCount() - started < 180000 && gfProgramIsRunning) {
        Sleep(5);
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query))
            continue;
        if (GetTickCount() - last_trace > 2000) {
            last_trace = GetTickCount();
            const HostileEngagementSnapshot& t = query.snapshot;
            fprintf(stderr,
                    "runtime-test trace: engaged=%u nearest=%.0f party_hp=%u "
                    "ehp=%u dead=%u combat=%u round=%u action=%d char=%d monster=%d "
                    "aim=%d hp=%d active=%u dead=%d "
                    "report(hits=%u dmg=%u miss=%u) threat=%d target=%d:%d\n",
                    t.engaged_hostiles, t.nearest_engaged_distance, t.party_hp_total,
                    t.engaged_hp_total, t.engaged_dead, t.combat_mode, t.round_active,
                    t.action_status, t.action_char, t.action_monster, aim_id, t.aim_hp,
                    t.aim_active, t.aim_dead, t.report_count, t.report_amount, t.report_missed,
                    t.provoked_threat_state, t.first_target_type, t.first_target_monster);
        }
        const HostileEngagementSnapshot& state = query.snapshot;
        if (!baseline_taken && state.engaged_hostiles != 0) {
            baseline_taken = true;
        }
        /* Monsters never own iActionChar: an executing char action is a party
           actor. The melee attack_report target does not describe spells. */
        if (state.action_char >= 0 && state.action_status >= 2 &&
            !g_observation.party_cast_executed) {
            g_observation.party_cast_executed = 1;
            ReportStep("party-cast-executed");
        }
        if (g_observation.party_cast_executed && !g_observation.target_damaged && aim_id >= 0 &&
            baseline_hp >= 0 &&
            (state.aim_dead || !state.aim_active ||
             (state.aim_hp >= 0 && state.aim_hp < baseline_hp))) {
            g_observation.target_damaged = 1;
            ReportStep("target-damaged");
        }
        if (baseline_taken && state.engaged_hostiles == 0 && !state.combat_mode) {
            g_observation.combat_ended = 1;
            ReportStep("combat-ended");
            /* A cast still queued when combat ends resolves through the
               out-of-combat spell UI, which runs its own modal loop and never
               reaches the quit flag. Dismiss it through the cancel binding. */
            for (int probe = 0; probe < 8; ++probe) {
                if (!IsModalOpen()) {
                    break;
                }
                TapGameplayCommand(W8_MGS_COMMAND_CANCEL);
                Sleep(500);
            }
            if (g_observation.target_damaged) {
                return FinishGameplayScenario();
            }
            return FailScenario("combat-spell", "combat-ended-without-target-damage");
        }
        if (state.round_active)
            round_requested = false;
        if (state.combat_mode && !state.round_active && !round_requested) {
            if (state.nearest_engaged_distance > 800.0f) {
                bool moved = false;
                RunOnGameThread(TeleportPartyNearEngagedOnGameThread, &moved);
                fprintf(stderr, "runtime-test approach: moved=%d dist=%.0f\n", moved,
                        state.nearest_engaged_distance);
            }
            CombatSpellQuery cast;
            cast.location_id = context.location_id;
            cast.spell_id = spell_id;
            if (!RunOnGameThread(QueuePartySpellsOnGameThread, &cast))
                continue;
            if (cast.spell_id <= 0)
                return FailScenario("combat-spell", "no-single-enemy-spell");
            spell_id = cast.spell_id;
            baseline_hp = cast.aim_hp;
            aim_id = cast.aim_location_id;
            query.aim_location_id = aim_id;
            fprintf(stderr, "runtime-test cast: spell=%d queued=%d aimed=%d aim=%d hp=%d\n",
                    cast.spell_id, cast.queued, cast.aimed, cast.aim_location_id, cast.aim_hp);
            if (cast.queued > 0 && !g_observation.combat_action_queued) {
                g_observation.combat_action_queued = 1;
                ReportStep("combat-cast-queued");
            }
            if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND))
                return FailScenario("combat-round", "binding-missing");
            round_requested = true;
        }
        if (g_observation.combat_action_queued && g_observation.party_cast_executed &&
            g_observation.target_damaged) {
            return FinishGameplayScenario();
        }
    }
    fprintf(stderr,
            "runtime-test cast-frontier: screen=%d pending=%d hostile=%u active=%u "
            "engaged=%u party_hp=%u nearest=%.0f provoked active=%u hp=%d cond=%d incombat=%d "
            "dead=%d dist=%.0f threat=%d combat=%u round=%u action=%d char=%d monster=%d "
            "spell=%d aim=%d aim_hp=%d aim_active=%u aim_dead=%d queued=%d target=%d:%d "
            "report(hits=%u dmg=%u miss=%u) obs(q=%u exec=%u tdmg=%u)\n",
            query.snapshot.screen, query.snapshot.pending, query.snapshot.hostile_count,
            query.snapshot.active_monsters, query.snapshot.engaged_hostiles,
            query.snapshot.party_hp_total, query.snapshot.nearest_engaged_distance,
            query.snapshot.provoked_active, query.snapshot.provoked_hp,
            query.snapshot.provoked_condition, query.snapshot.provoked_in_combat,
            query.snapshot.provoked_dead, query.snapshot.provoked_distance,
            query.snapshot.provoked_threat_state, query.snapshot.combat_mode,
            query.snapshot.round_active, query.snapshot.action_status, query.snapshot.action_char,
            query.snapshot.action_monster, spell_id, aim_id, query.snapshot.aim_hp,
            query.snapshot.aim_active, query.snapshot.aim_dead, query.snapshot.queued_attacks,
            query.snapshot.first_target_type, query.snapshot.first_target_monster,
            query.snapshot.report_count, query.snapshot.report_amount, query.snapshot.report_missed,
            g_observation.combat_action_queued, g_observation.party_cast_executed,
            g_observation.target_damaged);
    return FailScenario("combat-spell", g_observation.party_cast_executed
                                            ? "target-damage-not-observed"
                                            : "party-cast-not-observed");
}

/* The defensive twin of the attack round-trip: pre-wound the party, queue
   DEFEND on every slot, and drive rounds until a monster action executes and
   a member's durable incapacitation (hp 0 / UNCONSCIOUS-or-worse) is
   observed - the monster-swing and condition-transition branch the attack
   and spell scenarios never reach. */
static DWORD RunCombatCasualtyScenario()
{
    HostileEncounterContext context;
    if (!ScheduleOnGameThread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                              240000)) {
        return FailScenario("hostile-fixture", "game-thread-unresponsive");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");

    int weakened = 0;
    if (!RunOnGameThread(WeakenPartyOnGameThread, &weakened) || weakened == 0)
        return FailScenario("combat-casualty", "party-weaken-failed");
    fprintf(stderr, "runtime-test casualty: weakened=%d\n", weakened);

    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    query.aim_location_id = context.location_id;
    bool round_requested = false;
    unsigned int started = GetTickCount();
    unsigned int last_trace = 0;
    while (GetTickCount() - started < 220000 && gfProgramIsRunning) {
        Sleep(5);
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query)) {
            return FailScenario("combat-casualty", "game-thread-unresponsive");
        }
        const HostileEngagementSnapshot& state = query.snapshot;
        if (GetTickCount() - last_trace > 2000) {
            last_trace = GetTickCount();
            fprintf(stderr,
                    "runtime-test trace: engaged=%u party_hp=%u down=%u combat=%u "
                    "round=%u action=%d char=%d monster=%d\n",
                    state.engaged_hostiles, state.party_hp_total, state.incapacitated_members,
                    state.combat_mode, state.round_active, state.action_status, state.action_char,
                    state.action_monster);
        }
        /* Monsters never own iActionChar: an executing monster action is a
           monster actor resolving its queued swing. */
        if (!g_observation.monster_attack_executed && state.action_monster >= 0 &&
            state.action_status >= 2) {
            g_observation.monster_attack_executed = 1;
            ReportStep("monster-attack-executed");
        }
        if (!g_observation.party_casualty && state.incapacitated_members != 0) {
            g_observation.party_casualty = 1;
            ReportStep("party-casualty");
        }
        /* A wipe ends combat in the same frame: the latched casualty outranks
           the combat-mode drop. */
        if (g_observation.monster_attack_executed && g_observation.party_casualty) {
            return FinishGameplayScenario();
        }
        if (state.combat_mode == 0) {
            return FailScenario("combat-casualty", "combat-ended-without-casualty");
        }
        if (state.round_active) {
            round_requested = false;
        }
        if (state.combat_mode && !state.round_active && !round_requested) {
            int queued = 0;
            if (!RunOnGameThread(QueuePartyDefendOnGameThread, &queued))
                continue;
            if (queued > 0 && !g_observation.combat_defend_queued) {
                g_observation.combat_defend_queued = 1;
                ReportStep("combat-defend-queued");
            }
            if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND))
                return FailScenario("combat-round", "binding-missing");
            round_requested = true;
        }
    }
    return FailScenario("combat-casualty", g_observation.monster_attack_executed
                                               ? "party-casualty-not-observed"
                                               : "monster-attack-not-observed");
}

/* The target-switching branch of the attack round-trip: damage the aimed
   monster, then exclude it from the aim search so the queued attacks move
   to a second live in-combat monster - the same AimAtTarget retarget the
   player path runs when picking a new victim mid-fight - and observe that
   monster lose HP too. */
static DWORD RunCombatRetargetScenario()
{
    HostileEncounterContext context;
    if (!ScheduleOnGameThread("hostile-fixture", ProvokeHostileEncounterOnGameThread, &context,
                              240000)) {
        return FailScenario("hostile-fixture", "game-thread-unresponsive");
    }
    if (context.location_id < 0)
        return FailScenario("hostile-fixture", "active-monster-not-found");
    if (!WaitForCombatMode(true))
        return FailScenario("hostile-combat", "combat-not-entered");
    g_observation.combat_aggroed = 1;
    ReportStep("combat-aggroed");

    HostileSnapshotQuery query;
    query.location_id = context.location_id;
    int aim_id = context.location_id;
    int baseline_hp = -1;
    bool retargeted = false;
    bool round_requested = false;
    unsigned int started = GetTickCount();
    unsigned int last_trace = 0;
    while (GetTickCount() - started < 220000 && gfProgramIsRunning) {
        Sleep(5);
        if (!RunOnGameThread(ReadHostileEngagementOnGameThread, &query)) {
            return FailScenario("combat-retarget", "game-thread-unresponsive");
        }
        const HostileEngagementSnapshot& state = query.snapshot;
        if (GetTickCount() - last_trace > 2000) {
            last_trace = GetTickCount();
            fprintf(stderr,
                    "runtime-test trace: engaged=%u nearest=%.0f party_hp=%u combat=%u "
                    "round=%u action=%d aim=%d hp=%d active=%u dead=%d target=%d:%d\n",
                    state.engaged_hostiles, state.nearest_engaged_distance, state.party_hp_total,
                    state.combat_mode, state.round_active, state.action_status, aim_id,
                    state.aim_hp, state.aim_active, state.aim_dead, state.first_target_type,
                    state.first_target_monster);
        }
        if (aim_id >= 0 && baseline_hp >= 0 &&
            (state.aim_dead || !state.aim_active ||
             (state.aim_hp >= 0 && state.aim_hp < baseline_hp))) {
            if (!g_observation.target_damaged) {
                g_observation.target_damaged = 1;
                ReportStep("first-target-damaged");
            } else if (retargeted && !g_observation.second_target_damaged) {
                g_observation.second_target_damaged = 1;
                ReportStep("second-target-damaged");
                return FinishGameplayScenario();
            }
        }
        if (state.combat_mode == 0) {
            return FailScenario("combat-retarget", "combat-ended-early");
        }
        if (state.round_active) {
            round_requested = false;
        }
        if (state.combat_mode && !state.round_active && !round_requested) {
            if (state.nearest_engaged_distance > 800.0f) {
                bool moved = false;
                RunOnGameThread(TeleportPartyNearEngagedOnGameThread, &moved);
            }
            CombatAttackQuery attack;
            attack.location_id = aim_id;
            /* Once the first target is hurt, exclude it once so the aim
               search lands on another engaged monster; afterwards the row
               targets track the new aim directly. */
            attack.exclude_location_id = g_observation.target_damaged && !retargeted ? aim_id : -1;
            if (!RunOnGameThread(QueuePartyAttacksOnGameThread, &attack)) {
                return FailScenario("combat-round-queue", "game-thread-unresponsive");
            }
            if (attack.queued == 0) {
                return FailScenario("combat-retarget", "no-attacks-queued");
            }
            if (g_observation.target_damaged && !retargeted) {
                if (attack.aim_location_id == aim_id || attack.aim_location_id < 0) {
                    return FailScenario("combat-retarget", "second-target-unavailable");
                }
                retargeted = true;
                g_observation.combat_retargeted = 1;
                ReportStep("combat-retargeted");
                fprintf(stderr, "runtime-test retarget: %d -> %d hp=%d\n", aim_id,
                        attack.aim_location_id, attack.aim_hp);
            }
            aim_id = attack.aim_location_id;
            baseline_hp = attack.aim_hp;
            query.aim_location_id = aim_id;
            if (!g_observation.combat_attack_queued) {
                g_observation.combat_attack_queued = 1;
                ReportStep("combat-attack-queued");
            }
            if (!TapGameplayCommand(W8_MGS_COMMAND_START_COMBAT_ROUND))
                return FailScenario("combat-round", "binding-missing");
            round_requested = true;
        }
    }
    return FailScenario("combat-retarget",
                        !g_observation.target_damaged      ? "first-target-not-damaged"
                        : !g_observation.combat_retargeted ? "retarget-not-executed"
                                                           : "second-target-not-damaged");
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
    /* Integration drivers own one case for the whole session so the
       compatibility input helpers also resolve bindings on the game thread
       for menu-phase scenarios that enter the world mid-run. */
    unsigned long elapsed = GetTickCount() - g_scenario_started;
    unsigned long budget =
        g_scenario_spec->timeout_ms > elapsed ? g_scenario_spec->timeout_ms - elapsed : 0;
    RuntimeCase test(g_scenario, budget);
    g_case = &test;
    if (g_scenario_spec->phase == RUNTIME_MAIN_GAME) {
        if (PrepareMainGameFixture(test) != 0) {
            test.finish(false);
            /* The runner still owns game shutdown even when the fixture
               already failed through FailScenario. */
            FinishGameplayScenario();
            ReleaseHeldGameplayCommands();
            g_case = 0;
            return 1;
        }
        if (g_scenario_spec->case_run != 0) {
            bool passed = g_scenario_spec->case_run(test);
            if (test.failed()) {
                passed = false;
            } else if (!passed) {
                test.fail("case", "returned-false-without-failure");
            }
            g_observation.case_passed = passed ? 1 : 0;
            test.finish(passed);
            FinishGameplayScenario();
            ReleaseHeldGameplayCommands();
            g_case = 0;
            return passed ? 0 : 2;
        }
    }
    DWORD result = g_scenario_spec->run();
    ReleaseHeldGameplayCommands();
    g_case = 0;
    return result;
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

static bool ValidateCase(const RuntimeObservation& o)
{
    return o.main_game_entered && o.case_passed;
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

static bool ValidateCombatAttack(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_aggroed && o.combat_attack_queued &&
           o.party_attack_hit && o.target_damaged;
}

static bool ValidateCombatSpell(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_aggroed && o.combat_action_queued &&
           o.party_cast_executed && o.target_damaged;
}

static bool ValidateCombatCasualty(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_aggroed && o.combat_defend_queued &&
           o.monster_attack_executed && o.party_casualty;
}

static bool ValidateCombatRetarget(const RuntimeObservation& o)
{
    return o.main_game_entered && o.combat_aggroed && o.combat_attack_queued && o.target_damaged &&
           o.combat_retargeted && o.second_target_damaged;
}

static bool ValidateSoak(const RuntimeObservation& o)
{
    return o.main_game_entered && o.world_soaked;
}

static const RuntimeScenario kScenarios[] = {
    {"combat-roundtrip", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 120000,
     RunCombatRoundtripScenario, ValidateCombat, 0},
    {"hostile-encounter", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 300000,
     RunHostileEncounterScenario, ValidateHostile, 0},
    {"combat-attack", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 540000,
     RunCombatAttackScenario, ValidateCombatAttack, 0},
    {"combat-spell", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 180000,
     RunCombatSpellScenario, ValidateCombatSpell, 0},
    {"combat-casualty", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 240000,
     RunCombatCasualtyScenario, ValidateCombatCasualty, 0},
    {"combat-retarget", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 240000,
     RunCombatRetargetScenario, ValidateCombatRetarget, 0},
    {"world-soak", RUNTIME_MAIN_GAME, RUNTIME_NIGHTLY, RUNTIME_INTEGRATION, 120000,
     RunWorldSoakScenario, ValidateSoak, 0},
    {"exploration-input", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 120000, 0,
     ValidateCase, ExplorationInputCase},
    {"save-load-move", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 120000, 0, ValidateCase,
     SaveLoadMoveCase},
    {"automap-roundtrip", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 120000, 0,
     ValidateCase, AutomapRoundtripCase},
    {"oct-file", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunOctFileScenario,
     ValidateSemantic, 0},
    {"sight-threshold", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunSightScenario,
     ValidateSemantic, 0},
    {"split-stack", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     RunSplitStackScenario, ValidateSemantic, 0},
    {"party-movement", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     RunPartyMovementScenario, ValidateSemantic, 0},
    {"audio-semantics", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunAudioScenario,
     ValidateSemantic, 0},
    {"mongen", RUNTIME_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000, RunMonGenScenario,
     ValidateSemantic, 0},
    {"keyboard-menu", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     RunKeyboardMenuScenario, ValidateSemantic, 0},
    {"mouth-gap", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunMouthGapScenario,
     ValidateSemantic, 0},
    {"npc-dialogue", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunNpcDialogueScenario,
     ValidateSemantic, 0},
    {"lock-device", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunLockDeviceScenario,
     ValidateSemantic, 0},
    {"search-mode", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000, RunSearchModeScenario,
     ValidateSemantic, 0},
    {"main-menu-startup", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_INTEGRATION, 20000,
     RunMenuStartupScenario, ValidateMenuStartup, 0},
    {"main-menu-exit-auto-repeat", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_ACCEPTANCE, 20000,
     RunMenuExitScenario, ValidateMenuExit, 0},
    {"main-menu-new-game", RUNTIME_MAIN_MENU, RUNTIME_MAIN, RUNTIME_ACCEPTANCE, 30000,
     RunCharacterReturnScenario, ValidateCharacterReturn, 0},
    {"main-game-start", RUNTIME_MAIN_MENU, RUNTIME_PR, RUNTIME_ACCEPTANCE, 30000,
     RunMainGameScenario, ValidateMainGame, 0},
    {"npc-state-reset", RUNTIME_MAIN_GAME, RUNTIME_PR, RUNTIME_INTEGRATION, 120000,
     RunNpcResetScenario, ValidateNpcReset, 0},
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
        "party_moved=%u world_soaked=%u case_passed=%u "
        "combat_started=%u combat_action_queued=%u combat_party_moved=%u combat_ended=%u "
        "combat_aggroed=%u monster_engaged=%u "
        "party_attack_hit=%u target_damaged=%u party_cast_executed=%u "
        "combat_defend_queued=%u monster_attack_executed=%u party_casualty=%u "
        "combat_retargeted=%u second_target_damaged=%u "
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
        g_observation.party_moved, g_observation.world_soaked, g_observation.case_passed,
        g_observation.combat_started, g_observation.combat_action_queued,
        g_observation.combat_party_moved, g_observation.combat_ended, g_observation.combat_aggroed,
        g_observation.monster_engaged, g_observation.party_attack_hit, g_observation.target_damaged,
        g_observation.party_cast_executed, g_observation.combat_defend_queued,
        g_observation.monster_attack_executed, g_observation.party_casualty,
        g_observation.combat_retargeted, g_observation.second_target_damaged,
        g_observation.return_observed, teardown_ok ? 1 : 0, g_observation.timed_out,
        g_observation.npc_state_reset_ok, g_observation.character_page_start,
        g_observation.character_page_after, g_observation.tooltip_shown,
        g_observation.tooltip_removed, g_observation.skill_tooltip_shown,
        g_observation.skill_tooltip_removed, g_observation.skill_interacted);

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
