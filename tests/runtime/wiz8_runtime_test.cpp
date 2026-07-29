#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"
#include "wiz8/wiz8_windows.h"

#include "english.h"
#include "input.h"

#include <stdio.h>
#include <string.h>

extern "C" {

extern int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                          LPSTR command_line, int show_command);
extern unsigned char gfProgramIsRunning;
extern unsigned short g_selected_item_0069c4b4;
extern HWND ghWindow;
extern bool g_teardown_done_650db4;

}

struct RuntimeObservation {
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned short selected_item;
    unsigned char menu_seen;
    unsigned char exit_observed;
    unsigned char timed_out;
};

static RuntimeObservation g_observation;
static const char* g_scenario;

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
    g_observation.selected_item = g_selected_item_0069c4b4;
    fprintf(
        stderr,
        "runtime-test menu: scenario=%s state=%d regions=%u first=%u last=%u selected=%u\n",
        g_scenario,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_observation.selected_item);
    fflush(stderr);

    if (strcmp(g_scenario, "main-menu-startup") == 0) {
        PostMessage(ghWindow, WM_CLOSE, 0, 0);
        return 0;
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
    if (argc != 3 || strcmp(argv[1], "--scenario") != 0 ||
        (strcmp(argv[2], "main-menu-startup") != 0 &&
         strcmp(argv[2], "main-menu-exit") != 0)) {
        fprintf(stderr, "usage: Wiz8RuntimeTest --scenario main-menu-startup|main-menu-exit\n");
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
        "regions_enabled=%u first_region=%u last_region=%u selected_item=%u "
        "exit_observed=%u teardown=%u timed_out=%u\n",
        g_scenario,
        g_observation.menu_seen,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_observation.selected_item,
        g_observation.exit_observed,
        g_teardown_done_650db4 ? 1 : 0,
        g_observation.timed_out);

    const bool startup_ok =
        g_observation.menu_seen && g_observation.menu_state == 0 &&
        g_observation.region_set_enabled && g_observation.first_region == 0 &&
        g_observation.last_region == 6 && g_observation.selected_item == 0;
    const bool exit_ok =
        strcmp(g_scenario, "main-menu-startup") == 0 || g_observation.exit_observed;
    return driver_status == 0 && startup_ok && exit_ok && g_teardown_done_650db4 ? 0 : 1;
}
