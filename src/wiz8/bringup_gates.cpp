#include "wiz8/bringup_gates.h"
#include "wiz8/game_status.h"
#include "wiz8/wiz8_windows.h"

#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/input_hooks.h"
#include "wiz8/font_manager.h"
#include "wiz8/sound_man.h"
#include "wiz8/sgp_vsurface_private.h"
#include "Button System.h"
#include "Font.h"
#include "FileMan.h"
#include "RegInst.h"
#include "input.h"
#include "random.h"
#include "sgp.h"
#include "timer.h"
#include "vobject.h"
#include "vsurface.h"

#include <direct.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

/*
 * Gates called from InitializeStandardGamingPlatform at 0x00401570, in the
 * order the startup spine
 * records. Two of them the spine characterises from their imports and strings
 * and they are named accordingly; the rest it explicitly cannot, so they keep
 * address-derived names rather than invented meanings. Their globals are
 * likewise positional: the stores establish widths and initial values, nothing
 * establishes purpose.
 */

int g_dword_650df4;
int g_dword_650df8;
int g_dword_650dfc;
int g_dword_650e00;
bool g_flag_650e04;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_6598a8;
extern unsigned char g_flag_659711;
extern unsigned char g_fullscreen_603c39;
extern int g_dword_687595;
extern unsigned char g_byte_68de44;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_6598a8;
extern unsigned char g_flag_659711;
extern unsigned char g_fullscreen_603c39;
unsigned char g_flag_5ff5e8;

unsigned int g_mswheel_roll_message;
bool g_flag_6505a9;


/* Retail's shared success return, also used by the screen lifecycle table. */
// FUNCTION: WIZ8 0x005b1740
unsigned char ScreenLifecycleSuccess(void)
{
    return 1;
}

// FUNCTION: WIZ8 0x00404ba0
bool InitializeVideoSurfaceState(void)
{
    g_dword_650e00 = 0;
    g_dword_650df4 = 0;
    g_dword_650df8 = 0;
    g_dword_650dfc = 0;
    g_flag_650e04 = true;
    return true;
}

// FUNCTION: WIZ8 0x00428b80
int ReturnZero(void)
{
    return 0;
}

/* The initialized path occupies the 260 bytes before the mode at 0x603d74. */
// GLOBAL: WIZ8 0x00603c70
char g_video_config_file[260] = "3DVideo.CFG";

// FUNCTION: WIZ8 0x00427a30
void VideoSetConfigFile(const char* path)
{
    strcpy(g_video_config_file, path);
}

// FUNCTION: WIZ8 0x00427a60
char* VideoGetConfigFile(void)
{
    return g_video_config_file;
}

unsigned short* g_pointer_table_6ed440[0x400];
unsigned char g_flags_6ed040[0x400];
bool g_flag_650de4;
bool g_flag_5ff538;
unsigned char g_flag_6ef440;

void ShutdownHandler(void);
bool AddSubdirectoryToPath(const char* subdirectory);
extern "C" {
extern void GetRuntimeSettings(void);
extern unsigned int guiMouseWheelMsg;
}
extern unsigned char InitializeVideoManager(
    HINSTANCE instance, unsigned short show_command, void* window_proc);
extern long __stdcall WindowProc4011E0(
    void* window, int message, unsigned int wparam, long lparam);
extern unsigned char InitializeGame(void);
extern HWND g_window_6596cc;
bool g_shutdown_started_650db5;
bool g_teardown_done_650db4;
char g_shutdown_message_6505ac[0x100];
extern void MSYS_Shutdown(void);
extern int ReturnZero(void);



/* Stores the byte 0x004086D0's environment selection consults. */
/* Reports the byte at 0x00603C39; the only reader is 0x00421BB0. */
// FUNCTION: WIZ8 0x004229b0
unsigned char Function4229B0(void)
{
    return g_fullscreen_603c39;
}

/* Three latches, each set once and never cleared here. */
// FUNCTION: WIZ8 0x004229d0
void Function4229D0(void)
{
    g_flag_659711 = 1;
}

// FUNCTION: WIZ8 0x004277d0
void VideoInspectorEnable(void)
{
    g_flag_65970f = 1;
}

// FUNCTION: WIZ8 0x0042bc00
void Function42BC00(void)
{
    g_flag_6598a8 = 1;
}


/* Three more single-global writes, each in a different unit. The first stores a
   full dword and hands the same value back - it materializes the 1 in eax and
   stores through it - so its global is not the byte flag its one-bit use
   suggests, and it is not the void setter the other two are. */
// FUNCTION: WIZ8 0x00443a50
int Function443A50(void)
{
    g_status_685170.next_trigger_id_2356 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00482740
void Function482740(int value)
{
    g_dword_687595 = value;
}

// FUNCTION: WIZ8 0x005588e0
void Function5588E0(unsigned char value)
{
    g_byte_68de44 = value;
}

/* Clears the flag InitializeVideoSurfaceState raises. Both the window
   procedure's teardown and the shutdown handler reach it. */
// FUNCTION: WIZ8 0x00404bc0
void ShutdownVideoSurfaceState(void)
{
    g_flag_650e04 = false;
}

/* Empty in the shipped build: a single ret. InitializeStandardGamingPlatform still calls it. */
// FUNCTION: WIZ8 0x004023a0
void NoOp(void)
{
}

/* Clears the pointer table, then walks it releasing each entry. The walk can
   never see a live entry because the clear precedes it, but both are in the
   original and the compiler kept them, so both are reproduced. */
// FUNCTION: WIZ8 0x00404b00
void Function404B00(void)
{
    unsigned short** table;
    unsigned char* flags;
    unsigned short* entry;
    int remaining;

    memset(g_pointer_table_6ed440, 0, sizeof(g_pointer_table_6ed440));
    table = g_pointer_table_6ed440;
    flags = g_flags_6ed040;
    remaining = 0x400;
    do {
        entry = *table;
        *flags = 0;
        if (entry) {
            *entry = 0xffff;
            *table = 0;
        }
        ++flags;
        ++table;
        --remaining;
    } while (remaining);
    g_flag_650de4 = true;
    g_flag_5ff538 = true;
    g_flag_6ef440 = 0;
}

/* These are retained SGP globals, named by the vendored declaration surface. */
extern "C" HINSTANCE ghInstance;

/* The caller shifts the result right by ten and stores kilobytes. */
// FUNCTION: WIZ8 0x00404bd0
unsigned int QueryAvailableMemory(void)
{
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwAvailPhys;
}



/* Appends the subdirectory to the working directory and prepends the result to
   PATH, so plug-in DLLs load from the shipped subdirectory. Every string call
   here is inlined by VC6, which is why the body is mostly rep movs. */
// FUNCTION: WIZ8 0x00405740
bool AddSubdirectoryToPath(const char* subdirectory)
{
    char path[520];
    CHAR environment[520];
    unsigned int length;

    if (!subdirectory) {
        return false;
    }
    if (strlen(subdirectory) == 0) {
        return false;
    }
    _getcwd(path, 0x208);
    length = strlen(path);
    if (path[length != 0 ? length - 1 : 0] != '\\') {
        strcat(path, "\\");
    }
    strcat(path, subdirectory);
    if (GetEnvironmentVariableA("PATH", environment, 0x208) == 0) {
        return false;
    }
    strcat(environment, ";");
    strcat(environment, path);
    SetEnvironmentVariableA("PATH", environment);
    return true;
}

/* Registered with atexit as InitializeStandardGamingPlatform's first act. Guarded twice: a once
   flag so a second exit does nothing, and a separate teardown flag so the long
   release sequence runs at most once. The engine flag startup sets on
   success decides how much of it applies. Any message left in the buffer is
   shown before handing off. */
// FUNCTION: WIZ8 0x004017f0
void ShutdownHandler(void)
{
    unsigned char engine_up;

    if (g_shutdown_started_650db5) {
        return;
    }
    g_shutdown_started_650db5 = true;
    g_game_running = 0;
    DisableSoundManager();
    if (g_flag_6505a9) {
        GameloopExit(1);
    }
    if (!g_teardown_done_650db4) {
        engine_up = g_flag_6505a9;
        g_teardown_done_650db4 = true;
        if (engine_up) {
            ShutdownGame();
        }
        ShutdownButtonSystem();
        MSYS_Shutdown();
        DisableSoundManager();
        DestroyEnglishTransTable();
        ShutdownFontManager();
        ShutdownClockManager();
        ShutdownVideoSurfaceManager();
        ShutdownVideoObjectManager();
        ShutdownVideoManager();
        ShutdownInputManager();
        NoOp();
        NoOp();
        ShutdownVideoSurfaceState();
        NoOp();
    }
    ShowCursor(TRUE);
    if (strlen(g_shutdown_message_6505ac) != 0) {
        MessageBoxA(NULL, g_shutdown_message_6505ac, "Error", MB_ICONHAND);
    }
    ReturnZero();
}

/* A running instance is found by class and title both spelled "Wizardry 8"; it
   is raised and this one exits. Otherwise the video configuration file gates
   startup: absent, 3DSetup.EXE is spawned to write it and the check repeats,
   and still absent ends the run. The message loop ticks a frame whenever no
   message is waiting and the application is active, and waits otherwise. */
/* The shipped body allocates a 0xC8 frame and jumps straight to returning true.
   The CD check it guards - a sprintf of the insert-CD message and a MessageBoxA
   - sits between that jump and its target and is unreachable, which is the
   shape of a patch rather than of compiler output: the jump overwrites the
   first instruction after the prologue. All three GOG builds carry it
   identically, the retail image is protected so its addresses do not line up,
   and the demo has no insert-CD string at all, so the corpus holds no unpatched
   reference to recover the original check from.

   This therefore models the shipped behaviour rather than the shipped bytes,
   and is recorded structurally-strong: matching 87 bytes of retained-but-
   unreachable code is not something C can express, since the compiler would
   delete it. */
// FUNCTION: WIZ8 0x0042b830
bool CheckCdPresent(void)
{
    return true;
}

/* Retail 0x006EB708/0x006EB70C: the 10 ms timer driver's current and start
   ticks. */
// GLOBAL: WIZ8 0x006EB708
unsigned int g_dword_6eb708;
// GLOBAL: WIZ8 0x006EB70C
unsigned int g_dword_6eb70c;

/* A TIMERPROC: retail ends in `ret 0x10`, so it takes and cleans the four
   timer arguments even though it only reads the tick count. */
// FUNCTION: WIZ8 0x00406b70
void __stdcall Clock00406B70(
    HWND window, unsigned int message, unsigned int timer, unsigned long ticks)
{
    (void)window;
    (void)message;
    (void)timer;
    (void)ticks;
    unsigned int now = GetTickCount();
    if (now < g_dword_6eb708) {
        g_dword_6eb70c = now + (-1 - g_dword_6eb708);
        return;
    }
    g_dword_6eb70c = now - g_dword_6eb708;
}

// FUNCTION: WIZ8 0x00406ba0
unsigned char InitializeClockManager00406BA0(void)
{
    g_dword_6eb708 = GetTickCount();
    g_dword_6eb70c = g_dword_6eb708;
    SetTimer(g_window_6596cc, 1, 10, (TIMERPROC)Clock00406B70);
    return 1;
}

/* The retail startup spine. Each gate that fails returns straight out; the
   window procedure and shutdown handler this installs are what the live
   runtime tears down through. */
// FUNCTION: WIZ8 0x00401570
unsigned char InitializeStandardGamingPlatform(
    HINSTANCE instance, int show_command)
{
    FontTranslationTable* table;

    atexit(ShutdownHandler);
    InitializeRegistryKeys("Wizardry8", "Wizardry8key");
    AddSubdirectoryToPath("DLL");
    GetRuntimeSettings();
    Function404B00();
    if (!InitializeVideoSurfaceState()) {
        return 0;
    }
    if (!ScreenLifecycleSuccess()) {
        return 0;
    }
    NoOp();
    if (!InitializeInputManager00401EA0()) {
        return 0;
    }
    if (!InitializeVideoManager(
            instance, (unsigned short)show_command,
            (void*)WindowProc4011E0)) {
        return 0;
    }
    if (!InitializeVideoObjectManager()) {
        return 0;
    }
    if (!InitializeVideoSurfaceManager()) {
        return 0;
    }
    InitializeClockManager00406BA0();
    table = CreateDefaultFontTranslationTable();
    if (table == 0) {
        return 0;
    }
    if (!InitializeWiz8FontManager(8, table)) {
        return 0;
    }
    free(table);
    if (!InitializeWiz8SoundManager()) {
        return 0;
    }
    InitializeRandom();
    if (!InitializeGame()) {
        return 0;
    }
    guiMouseWheelMsg = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
    g_flag_6505a9 = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00401670
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    MSG message;
    HWND existing;

    existing = FindWindowExA(NULL, NULL, "Wizardry 8", "Wizardry 8");
    if (existing) {
        SetForegroundWindow(existing);
        ShowWindow(existing, 9);
        return 0;
    }
    ghInstance = hInstance;
    ProcessCommandLine(lpCmdLine);
    giStartMem = QueryAvailableMemory() >> 10;
    if (!FileExists(VideoGetConfigFile())) {
        _spawnl(0, "3DSetup.EXE", "3DSetup.EXE", VideoGetConfigFile(), NULL);
    }
    if (!FileExists(VideoGetConfigFile())) {
        return 0;
    }
    if (!CheckCdPresent()) {
        return 0;
    }
    ShowCursor(FALSE);
    if (!InitializeStandardGamingPlatform(hInstance, nShowCmd)) {
        return 0;
    }
    g_application_active = 1;
    g_game_running = 1;
    do {
        if (PeekMessageA(&message, NULL, 0, 0, 0)) {
            if (GetMessageA(&message, NULL, 0, 0) == 0) {
                return message.wParam;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else if (g_application_active == 0) {
            WaitMessage();
        } else {
            GameLoop();
            gfSGPInputReceived = 0;
        }
    } while (g_game_running);
    PostQuitMessage(0);
    return message.wParam;
}
