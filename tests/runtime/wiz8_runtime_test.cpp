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
#include "wiz8/local_code/character_events.h"
#include "wiz8/character_event_queue.h"
#include "wiz8/local_code/Search.h"
#include "wiz8/local_code/Strings.h"
#include "wiz8/local_screens/MGSTextBox.h"
#include "wiz8/local_screens/IntroScreen.h"
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
#include "character_actions.h"

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
    RuntimeWriteRecentEvents(stream, g_scenario);
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

/* Product controls and their registered regions are live game state: the
   game thread reads them and copies a centre point out, and the driver only
   sends the OS events at that copied point. */
static DWORD FailScenarioAt(const char* step, const char* reason, int line)
{
    fprintf(stderr, "WIZ8_RUNTIME_FAILURE scenario=%s step=%s reason=%s line=%d\n", g_scenario,
            step, reason, line);
    fprintf(stderr, "runtime-test failed: state=%d pending=%d main_game=%u running=%u active=%u\n",
            g_current_screen_state.id, g_pending_screen_state.id, g_observation.main_game_entered,
            gfProgramIsRunning, gfApplicationActive);
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

struct MenuChecksResult {
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    int playlist_active;
    int playlist_tracks;
    int playlist_weight;
    int playlist_pause_min;
    int playlist_pause_max;
    int playlist_pause_chance;
    int patch_catalog_count;
    unsigned int item_database_count;
    unsigned int monster_database_count;
    unsigned int npc_database_count;
    int patch_precedence_ok;
    int physical_fallback_ok;
    int shade_table_ok;
};

static void ReadMenuChecksOnGameThread(void* opaque)
{
    MenuChecksResult* checks = static_cast<MenuChecksResult*>(opaque);
    checks->menu_state = g_current_screen_state.id;
    checks->region_set_enabled = g_region_sets[1].enabled;
    checks->first_region = g_region_sets[1].first_region;
    checks->last_region = g_region_sets[1].last_region;
    checks->playlist_active = g_music_playlist_active_65ba7e;
    checks->playlist_tracks = g_music_playlist_track_count_65ba84;
    checks->playlist_weight = g_music_playlist_weight_total_65ba80;
    checks->playlist_pause_min = g_music_state_60aae8;
    checks->playlist_pause_max = g_music_state_60aaec;
    checks->playlist_pause_chance = g_music_state_60aaf0;
    checks->patch_catalog_count = gFileDataBase.usNumberOfLibraries - NUMBER_OF_LIBRARIES;
    checks->item_database_count = gXStatus.uiItemsInDatabase;
    checks->monster_database_count = gXStatus.uiMonstersInDatabase;
    checks->npc_database_count = gXStatus.uiNpcsInDatabase;
    checks->patch_precedence_ok = VerifyPatchPrecedence();
    checks->physical_fallback_ok = VerifyPhysicalFileFallback();
    const bool initial_table = VerifyShadeTable((FLOAT)0.66);
    SetShadeTablePercent((FLOAT)0.50);
    const bool changed_table = VerifyShadeTable((FLOAT)0.50);
    SetShadeTablePercent((FLOAT)0.66);
    const bool restored_table = VerifyShadeTable((FLOAT)0.66);
    checks->shade_table_ok = initial_table && changed_table && restored_table;
    fprintf(stderr,
            "runtime-test menu: scenario=%s state=%d regions=%u first=%u last=%u "
            "selected=%u playlist=%u tracks=%d weight=%d pause=%d..%d@%d "
            "patches=%d patch_precedence=%u physical_fallback=%u\n",
            g_scenario, checks->menu_state, checks->region_set_enabled, checks->first_region,
            checks->last_region, g_main_menu_selected_item, checks->playlist_active,
            checks->playlist_tracks, checks->playlist_weight, checks->playlist_pause_min,
            checks->playlist_pause_max, checks->playlist_pause_chance, checks->patch_catalog_count,
            checks->patch_precedence_ok, checks->physical_fallback_ok);
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

struct MainGameFixtureRequest {
    int party_size;
    const char* failure;
};

static void PrepareMainGameFixtureOnGameThread(void* opaque)
{
    MainGameFixtureRequest* request = static_cast<MainGameFixtureRequest*>(opaque);
    ReportStep("fixture-reset-begin");
    // Reproducible integration fixtures; acceptance keeps normal product randomness.
    srand(0x57495a38);
    ResetForNewGame();
    ReportStep("fixture-reset-end");
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
        request->failure = "character-allocation-incomplete";
        return;
    }
    DeriveCharacterPersonality004EFA30(&character);
    CalcCharacterTableValue(&character);
    if (character.portrait_index < 0 || character.portrait_index >= 0x50) {
        request->failure = "character-portrait-invalid";
        return;
    }
    wcscpy(character.name, L"Fixture");
    wcscpy(character.name_part_2, L"Runtime");
    FinalizeCreatedCharacter(&character, &creation, true);
    if (character.hp_current <= 0) {
        request->failure = "character-not-alive";
        return;
    }
    for (int member = 0; member < request->party_size; ++member) {
        character.name[0] = L'A' + member;
        if (AddCharacterToParty(&character, -1) < 0) {
            request->failure = "party-member-add-failed";
            return;
        }
    }
    ReportStep("fixture-party-created");
    // Before the first screen enters there is no screen to leave. Once one
    // exists, use the same leave/replace route as the party-start control.
    RunNewGameOpeningSequence(g_current_screen_state.id != -1, 0);
    // The new-game router normally queues character-specific opening videos.
    // Gameplay fixtures retain its initialized state and use the normal level
    // loader directly, without waiting for the cinematics.
    g_pending_screen_state.mode = 0;
    SetPendingScreenState(W8_SCREEN_PLEASE_WAIT);
    ReportStep("fixture-opening-started");
}

static bool PrepareMainGameFixture(RuntimeCase& test, int party_size)
{
    MainGameFixtureRequest request = {party_size, 0};
    if (!test.on_game_thread("main-game-fixture", PrepareMainGameFixtureOnGameThread, &request,
                             60000)) {
        return false;
    }
    if (request.failure != 0) {
        FailScenario("main-game-fixture", request.failure);
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
            SendScenarioKeyPress(VK_ESCAPE, 0);
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
    return true;
}

static bool EnterMonasteryPartyFixture(RuntimeCase& test)
{
    return PrepareMainGameFixture(test, 6);
}

static bool EnterMonasterySinglePartyFixture(RuntimeCase& test)
{
    return PrepareMainGameFixture(test, 1);
}

static const RuntimeFixtureSpec kFixtures[] = {
    {FIXTURE_ENGINE_READY, "engine-ready", RUNTIME_ENGINE_READY, FIXTURE_PATH_NATURAL,
     EnterEngineReadyFixture, LeaveFixture},
    {FIXTURE_MAIN_MENU, "main-menu", RUNTIME_MAIN_MENU, FIXTURE_PATH_NATURAL, EnterMainMenuFixture,
     LeaveFixture},
    {FIXTURE_MONASTERY_PARTY, "monastery-party", RUNTIME_MAIN_GAME, FIXTURE_PATH_SHORTCUT,
     EnterMonasteryPartyFixture, LeaveFixture},
    {FIXTURE_MONASTERY_SINGLE_PARTY, "monastery-single-party", RUNTIME_MAIN_GAME,
     FIXTURE_PATH_SHORTCUT, EnterMonasterySinglePartyFixture, LeaveFixture},
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

static bool MouselookDragCase(RuntimeCase& test)
{
    GameplaySnapshot before;
    RT_REQUIRE(test, test.snapshot(before, "right-drag-before"));
    POINT point = {320, 240};
    ClientToScreen(ghWindow, &point);
    SetForegroundWindow(ghWindow);
    SetCursorPos(point.x, point.y);
    Sleep(50);
    test.step("cursor-positioned");
    SendScenarioRightMouseButton(false);
    Sleep(50);
    test.step("right-button-down");
    for (int move = 0; move < 12; ++move) {
        point.x = 340 + move;
        point.y = 250;
        ClientToScreen(ghWindow, &point);
        SetCursorPos(point.x, point.y);
        Sleep(40);
        if (move == 0 || move == 5 || move == 11) {
            test.step("right-drag-moving");
        }
    }
    SendScenarioRightMouseButton(true);
    test.step("right-drag-sent");
    GameplaySnapshot after;
    RT_REQUIRE(test, test.snapshot(after, "right-drag-after"));
    float yaw_delta = after.yaw - before.yaw;
    if (yaw_delta < 0.0f) {
        yaw_delta = -yaw_delta;
    }
    test.expected("mouselook yaw change after right-drag");
    return yaw_delta > 0.01f || test.fail("right-drag", "yaw-unchanged");
}

static void QueueVoiceEventOnGameThread(void* opaque)
{
    bool* queued = static_cast<bool*>(opaque);
    W8Character* character = &g_status_685170.buffers.Char[2];
    character->gender = W8_GENDER_FEMALE;
    character->personality_0081 = 0;
    character->voice_0085 = 0;
    g_status_685170.greeting_pending_2497 = 0;
    *queued = QueueCharacterEvent(character, 4, 0, W8_EVENT_BYPASS_CHECKS, 0x7f) != 0;
}

static bool VoicePortraitSyncCase(RuntimeCase& test)
{
    bool queued = false;
    RT_REQUIRE(test, test.on_game_thread("voice-event-queue", QueueVoiceEventOnGameThread, &queued,
                                         30000));
    RT_REQUIRE(test, queued || test.fail("voice-event-queue", "event-not-queued"));
    RT_REQUIRE(test, test.wait_for_event(RUNTIME_VOICE_STARTED, 5000));
    RT_REQUIRE(test, test.wait_for_event(RUNTIME_MOUTH_CHANGED, 5000));
    RT_REQUIRE(test, test.wait_for_event(RUNTIME_PORTRAIT_FRAME_CHANGED, 5000));
    RT_REQUIRE(test, test.wait_for_event(RUNTIME_PORTRAIT_BLIT, 5000));
    if (getenv("WIZ8_RUNTIME_VOICE_TRACE") != 0) {
        Sleep(250);
    }
    RuntimeEvent events[512];
    unsigned long count = RuntimeCopyRecentEvents(events, 512);
    unsigned long voice_sequence = 0;
    unsigned long mouth_sequence = 0;
    bool frame_synced = false;
    bool frame_drawn = false;
    for (unsigned long index = 0; index < count; ++index) {
        const RuntimeEvent& event = events[index];
        if (getenv("WIZ8_RUNTIME_VOICE_TRACE") != 0 &&
            (event.kind == RUNTIME_VOICE_STARTED || event.kind == RUNTIME_VOICE_TIMING ||
             event.kind == RUNTIME_MOUTH_CHANGED || event.kind == RUNTIME_PORTRAIT_FRAME_CHANGED ||
             event.kind == RUNTIME_PORTRAIT_BLIT || event.kind == RUNTIME_PORTRAIT_REFRESH_STATE ||
             event.kind == RUNTIME_PORTRAIT_REFRESH_REQUESTED)) {
            fprintf(stderr, "voice-trace %lu %s %lu %lu %lu\n", event.sequence,
                    RuntimeEventName(event.kind), event.a, event.b, event.c);
        }
        if (event.a != 2) {
            continue;
        }
        if (event.kind == RUNTIME_VOICE_STARTED && event.c > 0) {
            voice_sequence = event.sequence;
        } else if (event.kind == RUNTIME_MOUTH_CHANGED && event.c == 1 &&
                   event.sequence > voice_sequence && voice_sequence != 0) {
            mouth_sequence = event.sequence;
        } else if (event.kind == RUNTIME_PORTRAIT_FRAME_CHANGED && event.b == 6 && event.c == 1 &&
                   event.sequence > mouth_sequence && mouth_sequence != 0) {
            frame_synced = true;
        } else if (event.kind == RUNTIME_PORTRAIT_BLIT && event.b == 6 && frame_synced) {
            frame_drawn = true;
        }
    }
    test.expected("voice starts, GAP changes the frame, and the portrait blits it");
    return frame_drawn || test.fail("voice-portrait-sync", "event-order-or-frame-not-drawn");
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

static bool WaitWorldVisible(RuntimeCase& test)
{
    unsigned int visibility_started = GetTickCount();
    bool world_visible = false;
    while (GetTickCount() - visibility_started < 10000 && gfProgramIsRunning) {
        RT_REQUIRE(test, test.wait_for_event(RUNTIME_WORLD_VIEWPORT_APPLIED, 5000));
        RuntimeEvent events[512];
        unsigned long count = RuntimeCopyRecentEvents(events, 512);
        for (unsigned long index = 0; index < count; ++index) {
            if (events[index].kind != RUNTIME_WORLD_VIEWPORT_APPLIED) {
                continue;
            }
            const RuntimeWorldRenderData& world = events[index].world;
            if (world.applied_viewport[0] == 0 || world.applied_viewport[1] == 0 ||
                world.applied_viewport[2] >= world.renderer_size[0] ||
                world.applied_viewport[3] >= world.renderer_size[1]) {
                return test.fail("main-game-visible", "world-viewport-outside-screen");
            }
            if (world.visible_meshes > 0) {
                world_visible = true;
                break;
            }
        }
        if (world_visible) {
            break;
        }
    }
    if (!world_visible) {
        return test.fail("main-game-visible", "no-visible-world-meshes");
    }
    test.step("main-game-visible");
    return true;
}

static bool MainGameShortcutCase(RuntimeCase& test)
{
    RT_REQUIRE(test, WaitWorldVisible(test));
    if (getenv("WIZ8_RUNTIME_VOICE_TRACE") != 0) {
        unsigned long last_sequence = 0;
        fprintf(stderr, "voice-counts started=%lu timing=%lu mouth=%lu frame=%lu blit=%lu\n",
                RuntimeEventCount(RUNTIME_VOICE_STARTED), RuntimeEventCount(RUNTIME_VOICE_TIMING),
                RuntimeEventCount(RUNTIME_MOUTH_CHANGED),
                RuntimeEventCount(RUNTIME_PORTRAIT_FRAME_CHANGED),
                RuntimeEventCount(RUNTIME_PORTRAIT_BLIT));
        for (int sample = 0; sample < 12; ++sample) {
            RuntimeEvent events[512];
            unsigned long count = RuntimeCopyRecentEvents(events, 512);
            for (unsigned long index = 0; index < count; ++index) {
                const RuntimeEvent& event = events[index];
                if (event.sequence <= last_sequence) {
                    continue;
                }
                if (event.kind == RUNTIME_VOICE_STARTED || event.kind == RUNTIME_VOICE_TIMING ||
                    event.kind == RUNTIME_MOUTH_CHANGED ||
                    event.kind == RUNTIME_PORTRAIT_FRAME_CHANGED ||
                    event.kind == RUNTIME_PORTRAIT_BLIT) {
                    fprintf(stderr, "voice-trace %lu %s %lu %lu %lu\n", event.sequence,
                            RuntimeEventName(event.kind), event.a, event.b, event.c);
                }
            }
            if (count != 0) {
                last_sequence = events[count - 1].sequence;
            }
            Sleep(250);
        }
    }
    RT_REQUIRE(test, MoveUntilDisplaced(test, W8_MGS_COMMAND_MOVE_FORWARD, "party-moved"));
    return true;
}

static bool WorldSoakCase(RuntimeCase& test)
{
    RT_REQUIRE(test, WaitWorldVisible(test));
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

static bool ExitScreenShown(const GameplaySnapshot& state, void*)
{
    return state.screen == W8_SCREEN_EXIT;
}

static bool MenuStartupCase(RuntimeCase& test)
{
    ExecutorProbe probe = {0, 0};
    DWORD expected_thread = GetWindowThreadProcessId(ghWindow, 0);
    for (unsigned int index = 0; index < 100; ++index) {
        if (!test.on_game_thread("game-thread-executor", ProbeExecutorOnGameThread, &probe, 5000) ||
            probe.thread_id != expected_thread || probe.calls != index + 1) {
            return test.failed()
                       ? false
                       : test.fail("game-thread-executor", "callback-thread-or-count-mismatch");
        }
    }
    test.step("game-thread-executor-checked");

    MenuChecksResult checks;
    memset(&checks, 0, sizeof(checks));
    RT_REQUIRE(test,
               test.on_game_thread("main-menu-checks", ReadMenuChecksOnGameThread, &checks, 5000));
    if (checks.menu_state != W8_SCREEN_MAIN_MENU || checks.region_set_enabled == 0 ||
        checks.playlist_active == 0 || checks.playlist_tracks <= 0 ||
        checks.patch_precedence_ok == 0 || checks.physical_fallback_ok == 0 ||
        checks.shade_table_ok == 0) {
        return test.fail("menu-checks", "menu-checks-not-satisfied");
    }
    test.step("menu-checks-ok");
    return true;
}

static bool MenuExitCase(RuntimeCase& test)
{
    SendScenarioKeyPress(VK_NEXT, KEYEVENTF_EXTENDEDKEY);
    SendScenarioKeyPress(VK_RETURN, 0);
    RT_REQUIRE(test, test.wait_until("exit-screen", 5000, ExitScreenShown, 0));
    /* The exit screen only clears the loop flag for input its regions do
       not consume, so a key arriving after the transition ends the run the
       way a held key's auto-repeat does on retail. */
    SendScenarioKeyPress(VK_RETURN, 0);
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < 5000) {
        if (*(volatile unsigned char*)&gfProgramIsRunning == 0) {
            break;
        }
        Sleep(10);
    }
    if (gfProgramIsRunning != 0) {
        return test.fail("exit-observed", "program-did-not-stop");
    }
    test.step("exit-observed");
    return true;
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
        bool entered = fixture->enter(test);
        DWORD case_result = 1;
        bool passed = false;
        if (entered) {
            passed = g_scenario_spec->case_run(test);
            if (test.failed()) {
                passed = false;
            } else if (!passed) {
                test.fail("case", "returned-false-without-failure");
            }
            g_observation.case_passed = passed ? 1 : 0;
            case_result = passed ? 0 : 2;
        } else {
            case_result = test.failed() ? 2 : 1;
        }
        fixture->leave(test);
        test.finish(passed);
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

static const RuntimeScenario kScenarios[] = {
    {"combat-roundtrip", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, CombatRoundtripCase, 0},
    {"hostile-encounter", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 300000, HostileEncounterCase, 0},
    {"combat-attack", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     540000, CombatAttackCase, 0},
    {"combat-spell", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     180000, CombatSpellCase, 0},
    {"world-soak", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_NIGHTLY, RUNTIME_INTEGRATION,
     120000, WorldSoakCase, 0},
    {"exploration-input", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, ExplorationInputCase, 0},
    {"mouselook-drag", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_NIGHTLY,
     RUNTIME_INTEGRATION, 120000, MouselookDragCase, 0},
    {"voice-portrait-sync", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_NIGHTLY,
     RUNTIME_INTEGRATION, 120000, VoicePortraitSyncCase, 0},
    {"save-load-move", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     120000, SaveLoadMoveCase, 0},
    {"automap-roundtrip", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 120000, AutomapRoundtripCase, 0},
    {"oct-file", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     OctFileCase, 1},
    {"sight-threshold", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, SightThresholdCase, 1},
    {"split-stack", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     SplitStackCase, 1},
    {"party-movement", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, PartyMovementCase, 1},
    {"audio-semantics", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC,
     15000, AudioSemanticsCase, 1},
    {"mongen", RUNTIME_ENGINE_READY, FIXTURE_ENGINE_READY, RUNTIME_PR, RUNTIME_SEMANTIC, 15000,
     MonGenCase, 1},
    {"keyboard-menu", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     KeyboardMenuCase, 1},
    {"mouth-gap", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     MouthGapCase, 1},
    {"npc-dialogue", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     NpcDialogueCase, 1},
    {"lock-device", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     LockDeviceCase, 1},
    {"search-mode", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_SEMANTIC, 20000,
     SearchModeCase, 1},
    {"main-menu-startup", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR, RUNTIME_INTEGRATION,
     20000, MenuStartupCase, 0},
    {"main-menu-exit-auto-repeat", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_PR,
     RUNTIME_ACCEPTANCE, 20000, MenuExitCase, 0},
    {"main-menu-new-game", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_MAIN, RUNTIME_ACCEPTANCE,
     30000, CharacterReturnCase, 0},
    {"main-game-start", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_SINGLE_PARTY, RUNTIME_PR,
     RUNTIME_INTEGRATION, 60000, MainGameShortcutCase, 0},
    {"new-game-ui", RUNTIME_MAIN_MENU, FIXTURE_MAIN_MENU, RUNTIME_NIGHTLY, RUNTIME_ACCEPTANCE,
     120000, MainGameStartCase, 0},
    {"npc-state-reset", RUNTIME_MAIN_GAME, FIXTURE_MONASTERY_PARTY, RUNTIME_PR, RUNTIME_INTEGRATION,
     120000, NpcResetCase, 0},
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
    /* GE-Proton does not forward this console application's stderr to umu-run. */
    const char* log_path = getenv("WIZ8_RUNTIME_TEST_LOG");
    if (log_path != 0 && log_path[0] != 0) {
        freopen(log_path, "w", stderr);
        setvbuf(stderr, 0, _IONBF, 0);
    }
    const char* output_path = getenv("WIZ8_RUNTIME_TEST_OUTPUT");
    if (output_path != 0 && output_path[0] != 0) {
        freopen(output_path, "w", stdout);
        setvbuf(stdout, 0, _IONBF, 0);
    }
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
    RuntimeInstrumentationInitialize();
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
    fprintf(stderr, "runtime-test progress: main_game=%u timed_out=%u\n",
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
               "main_game_entered=%u case_passed=%u teardown=%u timed_out=%u\n",
               g_scenario, g_observation.engine_ready, g_observation.menu_seen,
               g_observation.menu_state, g_observation.region_set_enabled,
               g_observation.first_region, g_observation.last_region,
               g_observation.main_game_entered, g_observation.case_passed, teardown_ok ? 1 : 0,
               g_observation.timed_out);

    /* Batch mode already emitted one WIZ8_RUNTIME_TEST line per case from the
       driver; its verdict is the per-case results plus teardown. */
    const int result = driver_status == 0 && g_observation.case_passed && teardown_ok ? 0 : 1;
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
