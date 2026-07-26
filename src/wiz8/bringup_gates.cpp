#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"

#include <stdlib.h>

/*
 * Gates called from BringUpEngine at 0x00401570, in the order the startup spine
 * records. Two of them the spine characterises from their imports and strings
 * and they are named accordingly; the rest it explicitly cannot, so they keep
 * address-derived names rather than invented meanings. Their globals are
 * likewise positional: the stores establish widths and initial values, nothing
 * establishes purpose.
 */

extern "C" {

extern int g_dword_650dbc;
extern int g_dword_650dc0;
extern int g_dword_6ef4c0;
extern int g_dword_650df4;
extern int g_dword_650df8;
extern int g_dword_650dfc;
extern int g_dword_650e00;
extern bool g_flag_650e04;
extern int g_dword_650e24;
extern int g_dword_650e28;
extern bool g_flag_650e20;

extern char Function402E30(void);

/* Named in the startup spine. The rest are gates it records as
   uncharacterisable, and the callees below it that nothing yet identifies. */
extern void RegisterWindowClass(const char* window_class, const char* key_class);
extern void SetModuleSubdirectory(const char* subdirectory);
extern void GetRuntimeSettings(void);
extern unsigned char InitializeInputManager(void);
extern void InitializeClockManager(void);
extern void InitializeRandom(void);
extern void ShutdownHandler(void);
extern long __stdcall WindowProc4011E0(void* window, unsigned int message,
                                       unsigned int wparam, long lparam);
extern void Function404B00(void);
extern void Function4023A0(void);
extern unsigned char Function421BB0(void* instance, int show_command,
                                    long(__stdcall* window_proc)(void*, unsigned int,
                                                                 unsigned int, long));
extern void* Function407EC0(void);
extern unsigned char Function407D30(int count, void* buffer);
extern unsigned char Function4086D0(void);
extern unsigned char Function4E2F40(void);
extern unsigned int g_mswheel_roll_message;
extern bool g_flag_6505a9;

// FUNCTION: WIZ8 0x005B1740
/* Shared success stub. BringUpEngine uses it as a gate and the 62-entry frame
   dispatch table parks it in seventeen slots. */
unsigned char Function5B1740(int unused)
{
    return 1;
}

// FUNCTION: WIZ8 0x00402970
bool Function402970(void)
{
    bool ready;

    g_dword_650dc0 = 0;
    g_dword_650dbc = 0;
    g_dword_6ef4c0 = 0;
    ready = Function402E30() != 0;
    return ready;
}

// FUNCTION: WIZ8 0x00404BA0
bool Function404BA0(void)
{
    g_dword_650e00 = 0;
    g_dword_650df4 = 0;
    g_dword_650df8 = 0;
    g_dword_650dfc = 0;
    g_flag_650e04 = true;
    return true;
}

// FUNCTION: WIZ8 0x00405E60
bool Function405E60(void)
{
    g_dword_650e28 = 0;
    g_dword_650e24 = 0;
    g_flag_650e20 = true;
    return true;
}

// FUNCTION: WIZ8 0x00427A60
const char* GetVideoConfigFileName(void)
{
    return "3DVideo.CFG";
}

// FUNCTION: WIZ8 0x00401570
/* The window init sequence. Registers the window class, points module loading
   at the DLL subdirectory, then runs each bring-up gate in order and abandons
   the whole sequence the moment one reports failure. Like the smaller gates it
   returns the flag it raises at the end. */
bool BringUpEngine(void* instance, int show_command)
{
    void* buffer;

    atexit(ShutdownHandler);
    RegisterWindowClass("Wizardry8", "Wizardry8key");
    SetModuleSubdirectory("DLL");
    GetRuntimeSettings();
    Function404B00();
    if (!Function404BA0()) {
        return false;
    }
    if (!Function5B1740(0)) {
        return false;
    }
    Function4023A0();
    if (!InitializeInputManager()) {
        return false;
    }
    if (!Function421BB0(instance, show_command, WindowProc4011E0)) {
        return false;
    }
    if (!Function405E60()) {
        return false;
    }
    if (!Function402970()) {
        return false;
    }
    InitializeClockManager();
    buffer = Function407EC0();
    if (!buffer) {
        return false;
    }
    if (!Function407D30(8, buffer)) {
        return false;
    }
    free(buffer);
    if (!Function4086D0()) {
        return false;
    }
    InitializeRandom();
    if (!Function4E2F40()) {
        return false;
    }
    g_mswheel_roll_message = RegisterWindowMessageA("MSWHEEL_ROLLMSG");
    g_flag_6505a9 = true;
    return true;
}

// FUNCTION: WIZ8 0x00404BD0
/* The caller shifts the result right by ten and stores kilobytes. */
unsigned int QueryAvailableMemory(void)
{
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwAvailPhys;
}

}
