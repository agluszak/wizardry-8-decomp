#include "wiz8/gameplay_boundaries.h"
#include "wiz8/screen_state.h"
#include "wiz8/wiz8_windows.h"

#include "english.h"
#include "input.h"
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

struct RuntimeObservation {
    int menu_state;
    unsigned int region_set_enabled;
    unsigned int first_region;
    unsigned int last_region;
    unsigned char menu_seen;
    unsigned char shade_table_ok;
    unsigned char exit_observed;
    unsigned char timed_out;
};

static RuntimeObservation g_observation;
static const char* g_scenario;

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
    const bool initial_table = VerifyShadeTable((FLOAT)0.66);
    SetShadeTablePercent((FLOAT)0.50);
    const bool changed_table = VerifyShadeTable((FLOAT)0.50);
    SetShadeTablePercent((FLOAT)0.66);
    const bool restored_table = VerifyShadeTable((FLOAT)0.66);
    g_observation.shade_table_ok =
        initial_table && changed_table && restored_table;
    fprintf(
        stderr,
        "runtime-test menu: scenario=%s state=%d regions=%u first=%u last=%u selected=%u\n",
        g_scenario,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_selected_item_0069c4b4);
    fflush(stderr);

    if (strcmp(g_scenario, "main-menu-startup") == 0) {
        gfProgramIsRunning = 0;
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
        "regions_enabled=%u first_region=%u last_region=%u "
        "shade_table_ok=%u exit_observed=%u teardown=%u timed_out=%u\n",
        g_scenario,
        g_observation.menu_seen,
        g_observation.menu_state,
        g_observation.region_set_enabled,
        g_observation.first_region,
        g_observation.last_region,
        g_observation.shade_table_ok,
        g_observation.exit_observed,
        g_teardown_done_650db4 ? 1 : 0,
        g_observation.timed_out);

    const bool startup_ok =
        g_observation.menu_seen && g_observation.menu_state == 0 &&
        g_observation.region_set_enabled && g_observation.first_region == 0 &&
        g_observation.last_region == 6 && g_observation.shade_table_ok;
    const bool exit_ok =
        strcmp(g_scenario, "main-menu-startup") == 0 || g_observation.exit_observed;
    const int result =
        driver_status == 0 && startup_ok && exit_ok && g_teardown_done_650db4 ? 0 : 1;
    fflush(stdout);
    TerminateProcess(GetCurrentProcess(), result);
    return result;
}
