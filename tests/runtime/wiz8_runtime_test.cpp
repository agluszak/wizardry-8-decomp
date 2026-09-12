/* Enable the pinned SDK's SendInput declarations for the harness only. */
#define _WIN32_WINNT 0x0500

#include "wiz8/regions.h"
#include "wiz8/combat_state.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/local_screens/CharacterScreen.h"
#include "wiz8/local_screens/MainMenuScreen.h"
#include "wiz8/local_screens/PartySelectionScreen.h"
#include "wiz8/local_code/GameplayCode.h"
#include "wiz8/local_code/GameplayDatabase.h"
#include "wiz8/music_playlist.h"
#include "wiz8/screen_state.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/xstatus.h"
#include "wiz8_crash_report.h"

struct SightSemanticResult;
bool RunSightSemanticTests(SightSemanticResult* result);
void PrintSightSemanticResults(const SightSemanticResult* result);

#include "english.h"
#include "FileMan.h"
#include "input.h"
#include "LibraryDataBase.h"
#include "shading.h"
#include "sgp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/game_status.h"

extern "C" {

extern int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line,
                          int show_command);
}

struct RuntimeObservation {
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned char menu_seen;
    unsigned char shade_table_ok;
    unsigned char exit_observed;
    unsigned char transition_observed;
    unsigned char character_entered;
    unsigned char character_returned;
    unsigned char final_page_entered;
    unsigned char final_page_redrawn;
    unsigned char character_committed;
    unsigned char character_in_party;
    unsigned char main_game_entered;
    unsigned char return_observed;
    unsigned char timed_out;
    int character_page_start;
    int character_page_after;
    unsigned char tooltip_shown;
    unsigned char tooltip_removed;
    unsigned char skill_tooltip_shown;
    unsigned char skill_tooltip_removed;
    unsigned char skill_interacted;
    unsigned char playlist_active;
    int playlist_tracks;
    int playlist_weight;
    int playlist_pause_min;
    int playlist_pause_max;
    int playlist_pause_chance;
    int patch_catalog_count;
    unsigned int item_database_count;
    unsigned int monster_database_count;
    unsigned int npc_database_count;
    unsigned char patch_precedence_ok;
    unsigned char physical_fallback_ok;
};

static RuntimeObservation g_observation;
static const char* g_scenario;

/* The whole in-process scenario must finish inside this budget; the Python
   runner's outer kill is larger so this report always wins. */
static const DWORD kScenarioBudgetMs = 120000;

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

static DWORD FailScenario()
{
    g_observation.timed_out = 1;
    if (ghWindow != NULL) {
        PostMessage(ghWindow, WM_CLOSE, 0, 0);
    }
    return 2;
}

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
    bool dismissed_intro = false;
    while (GetTickCount() - started < timeout_ms) {
        if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_MENU &&
            *(HWND volatile*)&ghWindow != NULL && g_region_sets[1].enabled) {
            return true;
        }
        // State zero also exists before input initialization clears the queue.
        // Wait until startup finishes before posting the intro-dismiss events.
        if (!dismissed_intro && gfGameInitialized && gfApplicationActive &&
            *(volatile int*)&g_current_screen_state.id == W8_SCREEN_INTRO) {
            SendScenarioKey(VK_ESCAPE);
            SendScenarioKey(VK_ESCAPE);
            dismissed_intro = true;
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

static DWORD WINAPI DriveScenario(void*)
{
    if (!WaitForMainMenu(30000)) {
        g_observation.timed_out = 1;
        fprintf(stderr, "runtime-test timeout: state=%d window=%p regions=%u running=%u\n",
                g_current_screen_state.id, ghWindow, g_region_sets[1].enabled, gfProgramIsRunning);
        fflush(stderr);
        gfProgramIsRunning = 0;
        if (ghWindow != NULL) {
            PostMessage(ghWindow, WM_CLOSE, 0, 0);
        }
        return 2;
    }

    g_observation.menu_seen = 1;
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

    if (strcmp(g_scenario, "main-menu-startup") == 0) {
        gfProgramIsRunning = 0;
        return 0;
    }

    if (strcmp(g_scenario, "main-menu-new-game") == 0 ||
        strcmp(g_scenario, "main-game-start") == 0 || strcmp(g_scenario, "new-game-entry") == 0) {
        SendScenarioKey(VK_PRIOR, KEYEVENTF_EXTENDEDKEY);
        SendScenarioKey(VK_DOWN, KEYEVENTF_EXTENDEDKEY);
        SendScenarioKey(VK_RETURN);
        /* Entering is only complete after GameLoop clears the pending state and
           the controller has built and enabled the mode-zero character panel.
           The panel's shared region-set slot is the readiness marker. */
        unsigned int started = GetTickCount();
        while (GetTickCount() - started < 5000) {
            unsigned int region_set =
                *(volatile unsigned int*)&g_state5_character_region_set_69c4f0;
            if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION &&
                *(volatile int*)&g_pending_screen_state.id == -1 && region_set != 0 &&
                *(volatile unsigned int*)&g_region_sets[region_set].enabled) {
                g_observation.transition_observed = 1;
                break;
            }
            Sleep(10);
        }
        if (!g_observation.transition_observed) {
            g_observation.timed_out = 1;
            PostMessage(ghWindow, WM_CLOSE, 0, 0);
            return 2;
        }

        /* The left action panel registers its controls in creation order, so
           the first region in its live set is "Create Character". Click the
           centre of that region's current bounds rather than a fixed pixel. */
        unsigned int left_action_set =
            *(volatile unsigned int*)&g_state5_left_action_region_set_69c504;
        if (left_action_set == 0 || left_action_set >= g_region_set_count) {
            g_observation.timed_out = 1;
            PostMessage(ghWindow, WM_CLOSE, 0, 0);
            return 2;
        }
        unsigned int create_region =
            *(volatile unsigned int*)&g_region_sets[left_action_set].first_region;
        if (create_region >= g_region_count) {
            g_observation.timed_out = 1;
            PostMessage(ghWindow, WM_CLOSE, 0, 0);
            return 2;
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
                break;
            }
            Sleep(10);
        }
        if (!g_observation.character_entered) {
            return FailScenario();
        }

        /* Walk the creation pages through their real controls. The first
           profession record is a non-caster, so the spell page is skipped. */
        W8CharacterScreen* screen = *(W8CharacterScreen* volatile*)&g_character_screen_0069c2e8;
        if (screen == 0 || screen->m_page_index_00c != 0 || screen->m_pages_1b0c[0] == 0) {
            return FailScenario();
        }
        W8CharacterPage005EF778* stats_page =
            static_cast<W8CharacterPage005EF778*>(screen->m_pages_1b0c[0]);
        W8CharacterCreationState* creation = &screen->m_creation_state_187c;

        /* Take the first profession, race and sex record. */
        ClickControl(stats_page->m_profession_row_07c->m_increment_020);
        started = GetTickCount();
        while (stats_page->m_profession_row_07c->m_value_004 == -1) {
            if (GetTickCount() - started > 3000)
                return FailScenario();
            Sleep(10);
        }
        ClickControl(stats_page->m_race_row_080->m_increment_020);
        started = GetTickCount();
        while (stats_page->m_race_row_080->m_value_004 == -1) {
            if (GetTickCount() - started > 3000)
                return FailScenario();
            Sleep(10);
        }
        ClickControl(stats_page->m_gender_row_084->m_increment_020);
        started = GetTickCount();
        while (stats_page->m_gender_row_084->m_value_004 == -1) {
            if (GetTickCount() - started > 3000)
                return FailScenario();
            Sleep(10);
        }

        /* The row callback enables the attribute entries once profession and
           race exist. */
        started = GetTickCount();
        while (!stats_page->m_entries_04c.data[0]->m_enabled_03a) {
            if (GetTickCount() - started > 3000)
                return FailScenario();
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
            return FailScenario();
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
                return FailScenario();
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
            return FailScenario();
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
            return FailScenario();
        }

        /* The final page copies both name fields into its text-input buffers at
           activation, and only enables Next while both are non-empty. Pre-fill
           the character before the page opens so the real Next control is
           enabled, the same state a user reaches by typing a name. */
        if (strcmp(g_scenario, "main-game-start") == 0 ||
            strcmp(g_scenario, "new-game-entry") == 0) {
            if (screen->m_character_018.name[0] == 0) {
                wcscpy(screen->m_character_018.name, L"Probe");
            }
            if (screen->m_character_018.name_part_2[0] == 0) {
                wcscpy(screen->m_character_018.name_part_2, L"Probe");
            }
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
                if (final_page->m_prepared_06c == 0) {
                    g_observation.final_page_redrawn = 1;
                    break;
                }
            }
            Sleep(10);
        }
        if (!g_observation.final_page_entered) {
            return FailScenario();
        }

        g_observation.character_page_start = 0;
        g_observation.character_page_after = screen->m_page_index_00c;

        if (strcmp(g_scenario, "main-game-start") == 0 ||
            strcmp(g_scenario, "new-game-entry") == 0) {
            /* Commit the character through the final page's own next control.
               A broken AdvancePage callback must fail the scenario rather than
               be papered over by calling it directly. */
            ClickControl(screen->m_next_1af8);
            started = GetTickCount();
            while (GetTickCount() - started < 5000) {
                if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_PARTY_SELECTION &&
                    *(volatile int*)&g_pending_screen_state.id == -1) {
                    g_observation.character_committed = 1;
                    break;
                }
                Sleep(10);
            }
            if (!g_observation.character_committed) {
                return FailScenario();
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
                return FailScenario();
            }

            if (strcmp(g_scenario, "main-game-start") == 0) {
                /* The product's start control requires a six-member party, so
                   the remaining members are fixture copies; they still enter
                   through the recovered party-add path rather than by writing
                   occupied flags directly. */
                int populated = -1;
                for (int roster_slot = 0; roster_slot < 8; ++roster_slot) {
                    if (g_status_685170.buffers.party_rows[roster_slot].occupied &&
                        g_status_685170.buffers.characters[roster_slot].hp_current != 0) {
                        populated = roster_slot;
                        break;
                    }
                }
                if (populated < 0) {
                    return FailScenario();
                }
                while (CountActiveCharacters() < 6) {
                    int before = CountActiveCharacters();
                    if (AddCharacterToParty(&g_status_685170.buffers.characters[populated], -1) <
                            0 ||
                        CountActiveCharacters() <= before) {
                        return FailScenario();
                    }
                }
                unsigned int bottom_set =
                    *(volatile unsigned int*)&g_state5_bottom_action_region_set_69c508;
                for (int click = 0; click < 6; ++click) {
                    int start_region = RegionWithHelpText(bottom_set, 0x6cb);
                    if (start_region < 0) {
                        return FailScenario();
                    }
                    ClickRegion(start_region);
                    started = GetTickCount();
                    while (GetTickCount() - started < 700) {
                        if (*(volatile int*)&g_pending_screen_state.id != -1 ||
                            *(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                            break;
                        }
                        Sleep(10);
                    }
                    if (*(volatile int*)&g_pending_screen_state.id != -1 ||
                        *(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME) {
                        break;
                    }
                }
            } else {
                /* Direct product new-game entry is the low-level bring-up
                   path; the behavioral scenario must not call it. */
                RunNewGameOpeningSequence(1, 0);
            }

            started = GetTickCount();
            while (GetTickCount() - started < 30000) {
                if (*(volatile int*)&g_current_screen_state.id == W8_SCREEN_MAIN_GAME &&
                    *(volatile int*)&g_pending_screen_state.id == -1) {
                    g_observation.main_game_entered = 1;
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
                return FailScenario();
            }
            gfProgramIsRunning = 0;
            return 0;
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
            return FailScenario();
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

int main(int argc, char** argv)
{
    W8SetCrashContextWriter(WriteRuntimeTestContext);
    if (argc != 3 || strcmp(argv[1], "--scenario") != 0) {
        fprintf(stderr,
                "usage: Wiz8RuntimeTest --scenario "
                "main-menu-startup|main-menu-exit-auto-repeat|main-menu-new-game|main-game-start|"
                "new-game-entry|sight-threshold\n");
        return 64;
    }

    if (strcmp(argv[2], "sight-threshold") == 0) {
        SightSemanticResult sight_result;

        if (!RunSightSemanticTests(&sight_result)) {
            PrintSightSemanticResults(&sight_result);
            return 1;
        }
        PrintSightSemanticResults(&sight_result);
        return 0;
    }

    if (strcmp(argv[2], "main-menu-startup") != 0 &&
        strcmp(argv[2], "main-menu-exit-auto-repeat") != 0 &&
        strcmp(argv[2], "main-game-start") != 0 && strcmp(argv[2], "new-game-entry") != 0 &&
        strcmp(argv[2], "main-menu-new-game") != 0) {
        fprintf(stderr,
                "usage: Wiz8RuntimeTest --scenario "
                "main-menu-startup|main-menu-exit-auto-repeat|main-menu-new-game|main-game-start|"
                "new-game-entry|sight-threshold\n");
        return 64;
    }

    g_scenario = argv[2];
    memset(&g_observation, 0, sizeof(g_observation));
    g_observation.menu_state = -1;
    HANDLE driver = CreateThread(NULL, 0, DriveScenario, NULL, 0, NULL);
    if (driver == NULL) {
        fprintf(stderr, "could not start in-process scenario driver\n");
        return 70;
    }

    char command_line[] = "";
    WinMain(GetModuleHandle(NULL), NULL, command_line, SW_SHOWNORMAL);
    /* The in-process driver owns each scenario's budget and reports its own
       timeout; this wait must stay below the runner's outer subprocess timeout
       (RUNTIME_SCENARIO_TIMEOUT_SECONDS in tools/wiz8decomp/runtime.py). */
    WaitForSingleObject(driver, kScenarioBudgetMs);
    DWORD driver_status = 2;
    GetExitCodeThread(driver, &driver_status);
    CloseHandle(driver);

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

    printf("WIZ8_RUNTIME_TEST scenario=%s menu_seen=%u menu_state=%d "
           "regions_enabled=%u first_region=%u last_region=%u "
           "playlist_active=%u playlist_tracks=%d playlist_weight=%d "
           "playlist_pause_min=%d playlist_pause_max=%d playlist_pause_chance=%d "
           "patch_catalog_count=%d item_database_count=%u "
           "monster_database_count=%u npc_database_count=%u "
           "patch_precedence_ok=%u physical_fallback_ok=%u "
           "shade_table_ok=%u exit_observed=%u transition_observed=%u "
           "character_entered=%u character_returned=%u "
           "final_page_entered=%u final_page_redrawn=%u "
           "character_committed=%u character_in_party=%u main_game_entered=%u "
           "return_observed=%u teardown=%u timed_out=%u "
           "character_page_start=%d character_page_after=%d "
           "tooltip_shown=%u tooltip_removed=%u "
           "skill_tooltip_shown=%u skill_tooltip_removed=%u "
           "skill_interacted=%u\n",
           g_scenario, g_observation.menu_seen, g_observation.menu_state,
           g_observation.region_set_enabled, g_observation.first_region, g_observation.last_region,
           g_observation.playlist_active, g_observation.playlist_tracks,
           g_observation.playlist_weight, g_observation.playlist_pause_min,
           g_observation.playlist_pause_max, g_observation.playlist_pause_chance,
           g_observation.patch_catalog_count, g_observation.item_database_count,
           g_observation.monster_database_count, g_observation.npc_database_count,
           g_observation.patch_precedence_ok, g_observation.physical_fallback_ok,
           g_observation.shade_table_ok, g_observation.exit_observed,
           g_observation.transition_observed, g_observation.character_entered,
           g_observation.character_returned, g_observation.final_page_entered,
           g_observation.final_page_redrawn, g_observation.character_committed,
           g_observation.character_in_party, g_observation.main_game_entered,
           g_observation.return_observed, teardown_ok ? 1 : 0, g_observation.timed_out,
           g_observation.character_page_start, g_observation.character_page_after,
           g_observation.tooltip_shown, g_observation.tooltip_removed,
           g_observation.skill_tooltip_shown, g_observation.skill_tooltip_removed,
           g_observation.skill_interacted);

    const bool startup_ok =
        g_observation.menu_seen && g_observation.menu_state == W8_SCREEN_MAIN_MENU &&
        g_observation.region_set_enabled && g_observation.first_region == 1 &&
        g_observation.last_region == 6 && g_observation.playlist_active &&
        g_observation.playlist_tracks > 0 && g_observation.patch_catalog_count > 0 &&
        g_observation.item_database_count > 0 && g_observation.monster_database_count > 0 &&
        g_observation.npc_database_count > 0 && g_observation.patch_precedence_ok &&
        g_observation.physical_fallback_ok && g_observation.shade_table_ok;
    const bool exit_ok =
        strcmp(g_scenario, "main-menu-startup") == 0 || g_observation.exit_observed;
    const bool character_flow = strcmp(g_scenario, "main-menu-new-game") == 0 ||
                                strcmp(g_scenario, "main-game-start") == 0 ||
                                strcmp(g_scenario, "new-game-entry") == 0;
    const bool transition_ok =
        !character_flow ||
        (g_observation.transition_observed && g_observation.character_entered &&
         g_observation.final_page_entered && g_observation.final_page_redrawn &&
         (strcmp(g_scenario, "main-game-start") == 0 || strcmp(g_scenario, "new-game-entry") == 0
              ? (g_observation.character_committed && g_observation.main_game_entered)
              : (g_observation.character_returned && g_observation.return_observed)));
    const bool character_ok =
        !character_flow ||
        (g_observation.character_page_start == 0 &&
         g_observation.character_page_after > g_observation.character_page_start &&
         g_observation.tooltip_shown && g_observation.tooltip_removed);
    const bool skills_ok =
        !character_flow || (g_observation.skill_tooltip_shown &&
                            g_observation.skill_tooltip_removed && g_observation.skill_interacted);
    const bool gameplay_ok =
        (strcmp(g_scenario, "main-game-start") != 0 && strcmp(g_scenario, "new-game-entry") != 0) ||
        (g_observation.character_committed && g_observation.character_in_party &&
         g_observation.main_game_entered);
    const int result = driver_status == 0 && startup_ok && (character_flow || exit_ok) &&
                               transition_ok && character_ok && skills_ok && gameplay_ok &&
                               teardown_ok
                           ? 0
                           : 1;
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), result);
    return result;
}
