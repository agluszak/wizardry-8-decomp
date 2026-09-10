#include "Types.h"
#include "mousesystem.h"
#include "wiz8/sgp-compat/gameloop.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/game_status.h"
#include "wiz8/wiz8_windows.h"

#include "wiz8/render_state.h"
#include "wiz8/screen_state.h"
#include "wiz8/input_hooks.h"
#include "wiz8/sgp_input_private.h"
#include "wiz8/sgp_private.h"
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

extern unsigned char g_flag_65970f;
// GLOBAL: WIZ8 0x006598a8
unsigned char g_flag_6598a8;
extern unsigned char g_flag_659711;
extern unsigned char g_fullscreen_603c39;
// GLOBAL: WIZ8 0x0068de44
unsigned char g_byte_68de44;
unsigned char g_flag_5ff5e8;



/* Retail's shared success return, also used by the screen lifecycle table. */
// FUNCTION: WIZ8 0x005b1740
unsigned char ScreenLifecycleSuccess(void)
{
    return 1;
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

// GLOBAL: WIZ8 0x00650DB4
bool g_sgp_shutdown_reentered;
// GLOBAL: WIZ8 0x00650DB5
static bool fAlreadyExiting;

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
    g_status_685170.game_time_days = value;
}

// FUNCTION: WIZ8 0x005588e0
void Function5588E0(unsigned char value)
{
    g_byte_68de44 = value;
}

/* Empty in the shipped build: a single ret. InitializeStandardGamingPlatform still calls it. */
// FUNCTION: WIZ8 0x004023a0
void NoOp(void)
{
}



/* Appends the subdirectory to the working directory and prepends the result to
   PATH, so plug-in DLLs load from the shipped subdirectory. Every string call
   here is inlined by VC6, which is why the body is mostly rep movs. */
// FUNCTION: WIZ8 0x00405740
BOOLEAN AddSubdirectoryToPath(CHAR8* subdirectory)
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

/* Wizardry's exit override omits released sound-stream servicing and shares
   its teardown guard with the product window procedure. */
// FUNCTION: WIZ8 0x004017f0
extern "C" void SGPExit(void)
{
    unsigned char engine_up;

    if (fAlreadyExiting) {
        return;
    }
    fAlreadyExiting = true;
    gfProgramIsRunning = 0;
    ShutdownSoundManager();
    if (gfGameInitialized) {
        GameloopExit(1);
    }
    if (!g_sgp_shutdown_reentered) {
        engine_up = gfGameInitialized;
        g_sgp_shutdown_reentered = true;
        if (engine_up) {
            ShutdownGame();
        }
        ShutdownButtonSystem();
        MSYS_Shutdown();
        ShutdownSoundManager();
        DestroyEnglishTransTable();
        ShutdownFontManager();
        ShutdownClockManager();
        ShutdownVideoSurfaceManager();
        ShutdownVideoObjectManager();
        ShutdownVideoManager();
        ShutdownInputManager();
        NoOp();
        NoOp();
        ShutdownMemoryManager();
        NoOp();
    }
    ShowCursor(TRUE);
    if (strlen(gzErrorMsg) != 0) {
        MessageBoxA(NULL, gzErrorMsg, "Error", MB_ICONHAND);
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
    giStartMem = MemGetFree() >> 10;
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
    gfApplicationActive = 1;
    gfProgramIsRunning = 1;
    do {
        if (PeekMessageA(&message, NULL, 0, 0, 0)) {
            if (GetMessageA(&message, NULL, 0, 0) == 0) {
                return message.wParam;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else if (gfApplicationActive == 0) {
            WaitMessage();
        } else {
            GameLoop();
            gfSGPInputReceived = 0;
        }
    } while (gfProgramIsRunning);
    PostQuitMessage(0);
    return message.wParam;
}
