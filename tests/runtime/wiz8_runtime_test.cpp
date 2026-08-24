#include "wiz8/regions.h"
#include "wiz8/screen_state.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/xstatus.h"

#include "english.h"
#include "FileMan.h"
#include "input.h"
#include "LibraryDataBase.h"
#include "shading.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {

extern int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                          LPSTR command_line, int show_command);
extern unsigned char gfProgramIsRunning;
extern unsigned short g_selected_item_0069c4b4;
extern HWND ghWindow;
extern bool g_teardown_done_650db4;

}

extern unsigned char g_music_playlist_active_65ba7e;
extern int g_music_playlist_weight_total_65ba80;
extern int g_music_playlist_track_count_65ba84;
extern int g_music_state_60aae8;
extern int g_music_state_60aaec;
extern int g_music_state_60aaf0;

struct RuntimeObservation {
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned char menu_seen;
    unsigned char shade_table_ok;
    unsigned char exit_observed;
    unsigned char transition_observed;
    unsigned char timed_out;
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

static LONG WINAPI ReportUnhandledException(EXCEPTION_POINTERS* exception)
{
    CONTEXT* context = exception->ContextRecord;
    const unsigned long* stack = (const unsigned long*)context->Esp;
    fprintf(stderr,
            "runtime-test exception: code=%08lx address=%p eip=%08lx esp=%08lx "
            "stack=%08lx,%08lx,%08lx,%08lx\n",
            exception->ExceptionRecord->ExceptionCode,
            exception->ExceptionRecord->ExceptionAddress,
            context->Eip, context->Esp,
            stack[0], stack[1], stack[2], stack[3]);
    fprintf(stderr, "runtime-test module-stack=");
    unsigned int found = 0;
    for (unsigned int index = 0; index < 256 && found < 24; ++index) {
        if (stack[index] >= 0x00400000 && stack[index] < 0x00500000) {
            fprintf(stderr, "%s%08lx@+%x", found ? "," : "",
                    stack[index], index * sizeof(*stack));
            ++found;
        }
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

static bool VerifyShadeTable(FLOAT coefficient)
{
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
                expected[index] = Get16BPPColor(FROMRGB(
                    (UINT8)(red * coefficient),
                    (UINT8)(green * coefficient),
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
            fprintf(stderr, "white-palette mismatch index=%u actual=%u\n",
                    index, White16BPPPalette[index]);
            return false;
        }
    }
    return true;
}

static bool WaitForMainMenu(unsigned int timeout_ms)
{
    unsigned int started = GetTickCount();
    while (GetTickCount() - started < timeout_ms) {
        if (*(volatile int*)&g_screen_state_0068ec78.id == 0 &&
            *(HWND volatile*)&ghWindow != NULL && g_region_sets[1].enabled) {
            return true;
        }
        Sleep(10);
    }
    return false;
}

static bool VerifyPatchPrecedence(void)
{
    for (int library_id = NUMBER_OF_LIBRARIES;
         library_id < gFileDataBase.usNumberOfLibraries; ++library_id) {
        LibraryHeaderStruct* library = &gFileDataBase.pLibraries[library_id];
        if (!library->fPatchLibrary) {
            continue;
        }
        for (unsigned int entry = 0; entry < library->usNumberOfEntries; ++entry) {
            char path[512];
            sprintf(path, "%s%s", library->sLibraryPath,
                    library->pFileHeader[entry].pFileName);
            if (GetLibraryIDFromFileName(path) == library_id) {
                HWFILE handle = FileOpen(path, FILE_ACCESS_READ | FILE_OPEN_EXISTING, 0);
                short selected_library = -1;
                unsigned int file_id = 0;
                if (handle != 0) {
                    GetLibraryAndFileIDFromLibraryFileHandle(
                        handle, &selected_library, &file_id);
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
        fprintf(
            stderr,
            "runtime-test timeout: state=%d window=%p regions=%u running=%u\n",
            g_screen_state_0068ec78.id,
            ghWindow,
            g_region_sets[1].enabled,
            gfProgramIsRunning);
        fflush(stderr);
        gfProgramIsRunning = 0;
        if (ghWindow != NULL) {
            PostMessage(ghWindow, WM_CLOSE, 0, 0);
        }
        return 2;
    }

    g_observation.menu_seen = 1;
    g_observation.menu_state = g_screen_state_0068ec78.id;
    g_observation.region_set_enabled = g_region_sets[1].enabled;
    g_observation.first_region = g_region_sets[1].first_region;
    g_observation.last_region = g_region_sets[1].last_region;
    g_observation.playlist_active = g_music_playlist_active_65ba7e;
    g_observation.playlist_tracks = g_music_playlist_track_count_65ba84;
    g_observation.playlist_weight = g_music_playlist_weight_total_65ba80;
    g_observation.playlist_pause_min = g_music_state_60aae8;
    g_observation.playlist_pause_max = g_music_state_60aaec;
    g_observation.playlist_pause_chance = g_music_state_60aaf0;
    g_observation.patch_catalog_count =
        gFileDataBase.usNumberOfLibraries - NUMBER_OF_LIBRARIES;
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
    g_observation.shade_table_ok =
        initial_table && changed_table && restored_table;
    fprintf(
        stderr,
        "runtime-test menu: scenario=%s state=%d regions=%u first=%u last=%u "
        "selected=%u playlist=%u tracks=%d weight=%d pause=%d..%d@%d "
        "patches=%d patch_precedence=%u physical_fallback=%u\n",
        g_scenario,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_selected_item_0069c4b4,
        g_observation.playlist_active,
        g_observation.playlist_tracks,
        g_observation.playlist_weight,
        g_observation.playlist_pause_min,
        g_observation.playlist_pause_max,
        g_observation.playlist_pause_chance,
        g_observation.patch_catalog_count,
        g_observation.patch_precedence_ok,
        g_observation.physical_fallback_ok);
    fflush(stderr);

    if (strcmp(g_scenario, "main-menu-startup") == 0) {
        gfProgramIsRunning = 0;
        return 0;
    }

    if (strcmp(g_scenario, "main-menu-new-game") == 0) {
        QueueEvent(KEY_DOWN, HOME, 0);
        QueueEvent(KEY_DOWN, DNARROW, 0);
        QueueEvent(KEY_DOWN, ENTER, 0);
        unsigned int started = GetTickCount();
        while (GetTickCount() - started < 5000) {
            if (*(volatile int*)&g_screen_state_0068ec78.id == 5) {
                g_observation.transition_observed = 1;
                gfProgramIsRunning = 0;
                return 0;
            }
            Sleep(10);
        }
        g_observation.timed_out = 1;
        PostMessage(ghWindow, WM_CLOSE, 0, 0);
        return 2;
    }

    QueueEvent(KEY_DOWN, KEY_END, 0);
    QueueEvent(KEY_DOWN, ENTER, 0);
    unsigned int started = GetTickCount();
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
    SetUnhandledExceptionFilter(ReportUnhandledException);
    if (argc != 3 || strcmp(argv[1], "--scenario") != 0 ||
        (strcmp(argv[2], "main-menu-startup") != 0 &&
         strcmp(argv[2], "main-menu-exit") != 0 &&
         strcmp(argv[2], "main-menu-new-game") != 0)) {
        fprintf(stderr, "usage: Wiz8RuntimeTest --scenario main-menu-startup|main-menu-exit|main-menu-new-game\n");
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
    WaitForSingleObject(driver, 35000);
    DWORD driver_status = 2;
    GetExitCodeThread(driver, &driver_status);
    CloseHandle(driver);

    if (IsWindow(ghWindow)) {
        DestroyWindow(ghWindow);
    }

    printf(
        "WIZ8_RUNTIME_TEST scenario=%s menu_seen=%u menu_state=%d "
        "regions_enabled=%u first_region=%u last_region=%u "
        "playlist_active=%u playlist_tracks=%d playlist_weight=%d "
        "playlist_pause_min=%d playlist_pause_max=%d playlist_pause_chance=%d "
        "patch_catalog_count=%d item_database_count=%u "
        "monster_database_count=%u npc_database_count=%u "
        "patch_precedence_ok=%u physical_fallback_ok=%u "
        "shade_table_ok=%u exit_observed=%u transition_observed=%u "
        "teardown=%u timed_out=%u\n",
        g_scenario,
        g_observation.menu_seen,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_observation.playlist_active,
        g_observation.playlist_tracks,
        g_observation.playlist_weight,
        g_observation.playlist_pause_min,
        g_observation.playlist_pause_max,
        g_observation.playlist_pause_chance,
        g_observation.patch_catalog_count,
        g_observation.item_database_count,
        g_observation.monster_database_count,
        g_observation.npc_database_count,
        g_observation.patch_precedence_ok,
        g_observation.physical_fallback_ok,
        g_observation.shade_table_ok,
        g_observation.exit_observed,
        g_observation.transition_observed,
        g_teardown_done_650db4 ? 1 : 0,
        g_observation.timed_out);

    const bool startup_ok =
        g_observation.menu_seen && g_observation.menu_state == 0 &&
        g_observation.region_set_enabled && g_observation.first_region == 0 &&
        g_observation.last_region == 6 && g_observation.playlist_active &&
        g_observation.playlist_tracks > 0 &&
        g_observation.patch_catalog_count > 0 &&
        g_observation.item_database_count > 0 &&
        g_observation.monster_database_count > 0 &&
        g_observation.npc_database_count > 0 &&
        g_observation.patch_precedence_ok &&
        g_observation.physical_fallback_ok && g_observation.shade_table_ok;
    const bool exit_ok =
        strcmp(g_scenario, "main-menu-startup") == 0 || g_observation.exit_observed;
    const bool transition_ok =
        strcmp(g_scenario, "main-menu-new-game") != 0 ||
        g_observation.transition_observed;
    const int result =
        driver_status == 0 && startup_ok &&
        (strcmp(g_scenario, "main-menu-new-game") == 0 || exit_ok) &&
        transition_ok && g_teardown_done_650db4 ? 0 : 1;
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), result);
    return result;
}
