#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"

#include <direct.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>

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

extern void GetRuntimeSettings(void);
extern unsigned char InitializeInputManager(void);
extern void InitializeClockManager(void);
extern void InitializeRandom(void);

extern long __stdcall WindowProc4011E0(void* window, unsigned int message,
                                       unsigned int wparam, long lparam);


extern unsigned char Function421BB0(void* instance, int show_command,
                                    long(__stdcall* window_proc)(void*, unsigned int,
                                                                 unsigned int, long));
extern void* Function407EC0(void);

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

extern unsigned short* g_pointer_table_6ed440[0x400];
extern unsigned char g_flags_6ed040[0x400];
extern bool g_flag_650de4;
extern bool g_flag_5ff538;
extern unsigned char g_flag_6ef440;

void ShutdownHandler(void);
bool SetModuleSubdirectory(const char* subdirectory);
extern bool g_shutdown_started_650db5;
extern bool g_teardown_done_650db4;
extern char g_shutdown_message_6505ac[];
extern void Function408850(void);
extern void Function4E34B0(int flag);
extern void Function4E3290(void);
extern void Function40CF90(void);
extern void Function40B450(void);
extern void Function407E70(void);
extern void Function407E30(void);
extern void Function406BD0(void);
extern void Function402990(void);
extern void Function405E80(void);
extern void Function421DC0(void);
extern void Function401F70(void);
extern void Function404BC0(void);
extern void Function428B80(void);

// FUNCTION: WIZ8 0x004023A0
/* Empty in the shipped build: a single ret. BringUpEngine still calls it. */
void Function4023A0(void)
{
}

// FUNCTION: WIZ8 0x00404B00
/* Clears the pointer table, then walks it releasing each entry. The walk can
   never see a live entry because the clear precedes it, but both are in the
   original and the compiler kept them, so both are reproduced. */
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

extern unsigned char FileExists(const char* path);

extern unsigned char Function42B830(void);
extern void Function4E3340(void);
extern void* g_instance_6f062c;
extern unsigned int g_available_kilobytes;
extern unsigned char g_run_flag_6f0628;
extern unsigned char gfApplicationActive;
extern unsigned char gfSGPInputReceived;

// FUNCTION: WIZ8 0x00404BD0
/* The caller shifts the result right by ten and stores kilobytes. */
unsigned int QueryAvailableMemory(void)
{
    MEMORYSTATUS status;

    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwAvailPhys;
}


/* An eight-byte node: a code and a payload word. 0x00407D30 builds a head node
   pointing at a copy of the caller's, which is the only structure either
   establishes. */
struct W8BindingNode {
    unsigned short code;                  /* 0x00 */
    unsigned char unknown_02[2];
    void* payload;                        /* 0x04 */
};

extern void Function422AF0(unsigned int* first, unsigned int* second, unsigned int* third);
extern int g_dword_5ff5f0;
extern int g_dword_5ff5f4;
extern int g_dword_5ff5f8;
extern int g_dword_5ff5fc;
extern int g_dword_5ff600;
extern int g_dword_5ff604;
extern int g_dword_5ff608;
extern int g_dword_5ff60c;
extern unsigned char g_flag_650e38;
extern W8BindingNode* g_binding_head_6eb704;
extern unsigned char g_block_6eb6a0[0x64];

extern void Function4086C0(int enable);
extern void Function4277D0(void);
extern void Function427A30(const char* path);
extern void Function422970(int enable);
extern void Function42BC00(void);
extern unsigned char gfLoadAtStartup;
extern unsigned char gfUsingBoundsChecker;
extern unsigned char gfCapturingVideo;
extern char* gzStringDataOverride;

// FUNCTION: WIZ8 0x00401950
/* Switches are matched by prefix with _strnicmp, so each comparison carries its
   own length. The line is copied first because strtok writes through it, and
   the delimiter set is copied to a stack word rather than used in place. */
void ProcessCommandLine(char* pCommandLine)
{
    char delimiters[4];
    char* copy;
    char* token;

    *(unsigned int*)delimiters = *(unsigned int*)"\t =";
    copy = (char*)malloc(strlen(pCommandLine) + 1);
    if (!copy) {
        return;
    }
    memcpy(copy, pCommandLine, strlen(pCommandLine) + 1);
    token = strtok(copy, delimiters);
    while (token) {
        if (_strnicmp(token, "/NOSOUND", 8) == 0) {
            Function4086C0(0);
        } else if (_strnicmp(token, "/INSPECTOR", 10) == 0) {
            Function4277D0();
        } else if (_strnicmp(token, "/VIDEOCFG", 9) == 0) {
            token = strtok(NULL, delimiters);
            Function427A30(token);
        } else if (_strnicmp(token, "/LOAD", 5) == 0) {
            gfLoadAtStartup = 1;
        } else if (_strnicmp(token, "/WINDOW", 7) == 0) {
            Function422970(0);
        } else if (_strnicmp(token, "/BC", 7) == 0) {
            gfUsingBoundsChecker = 1;
        } else if (_strnicmp(token, "/CAPTURE", 7) == 0) {
            gfCapturingVideo = 1;
        } else if (_strnicmp(token, "/NOOCT", 6) == 0) {
            Function42BC00();
        } else if (_strnicmp(token, "/STRINGDATA", 0xb) == 0) {
            token = strtok(NULL, delimiters);
            gzStringDataOverride = (char*)malloc(strlen(token) + 1);
            memcpy(gzStringDataOverride, token, strlen(token) + 1);
        }
        token = strtok(NULL, delimiters);
    }
    free(copy);
}

// FUNCTION: WIZ8 0x00407D30
/* Publishes the three values 0x00422AF0 reports, narrowed to their field
   widths, then - when given a source node - allocates a head and a copy of it.
   Either allocation failing abandons the whole thing, leaking the first. */
bool Function407D30(unsigned short code, W8BindingNode* source)
{
    unsigned int first;
    unsigned int second;
    unsigned int third;
    W8BindingNode* copy;

    g_dword_5ff5f0 = -1;
    g_dword_5ff5f4 = -15;
    g_dword_5ff5f8 = 0;
    Function422AF0(&first, &second, &third);
    g_dword_5ff600 = 0;
    g_dword_5ff604 = 0;
    g_dword_5ff608 = first & 0xffff;
    g_dword_5ff60c = second & 0xffff;
    g_dword_5ff5fc = third & 0xff;
    g_flag_650e38 = 0;
    if (!source) {
        return false;
    }
    g_binding_head_6eb704 = (W8BindingNode*)malloc(8);
    if (!g_binding_head_6eb704) {
        return false;
    }
    copy = (W8BindingNode*)malloc(8);
    if (!copy) {
        return false;
    }
    g_binding_head_6eb704->payload = copy;
    g_binding_head_6eb704->code = code;
    copy->code = source->code;
    copy->payload = source->payload;
    memset(g_block_6eb6a0, 0, sizeof(g_block_6eb6a0));
    return true;
}

// FUNCTION: WIZ8 0x00405740
/* Appends the subdirectory to the working directory and prepends the result to
   PATH, so plug-in DLLs load from the shipped subdirectory. Every string call
   here is inlined by VC6, which is why the body is mostly rep movs. */
bool SetModuleSubdirectory(const char* subdirectory)
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

// FUNCTION: WIZ8 0x004017F0
/* Registered with atexit as BringUpEngine's first act. Guarded twice: a once
   flag so a second exit does nothing, and a separate teardown flag so the long
   release sequence runs at most once. The engine flag BringUpEngine sets on
   success decides how much of it applies. Any message left in the buffer is
   shown before handing off. */
void ShutdownHandler(void)
{
    unsigned char engine_up;

    if (g_shutdown_started_650db5) {
        return;
    }
    g_shutdown_started_650db5 = true;
    g_run_flag_6f0628 = 0;
    Function408850();
    if (g_flag_6505a9) {
        Function4E34B0(1);
    }
    if (!g_teardown_done_650db4) {
        engine_up = g_flag_6505a9;
        g_teardown_done_650db4 = true;
        if (engine_up) {
            Function4E3290();
        }
        Function40CF90();
        Function40B450();
        Function408850();
        Function407E70();
        Function407E30();
        Function406BD0();
        Function402990();
        Function405E80();
        Function421DC0();
        Function401F70();
        Function4023A0();
        Function4023A0();
        Function404BC0();
        Function4023A0();
    }
    ShowCursor(TRUE);
    if (strlen(g_shutdown_message_6505ac) != 0) {
        MessageBoxA(NULL, g_shutdown_message_6505ac, "Error", MB_ICONHAND);
    }
    Function428B80();
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
    if (!Function407D30(8, (W8BindingNode*)buffer)) {
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

// FUNCTION: WIZ8 0x00401670
/* A running instance is found by class and title both spelled "Wizardry 8"; it
   is raised and this one exits. Otherwise the video configuration file gates
   startup: absent, 3DSetup.EXE is spawned to write it and the check repeats,
   and still absent ends the run. The message loop ticks a frame whenever no
   message is waiting and the application is active, and waits otherwise. */
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
    g_instance_6f062c = hInstance;
    ProcessCommandLine(lpCmdLine);
    g_available_kilobytes = QueryAvailableMemory() >> 10;
    if (!FileExists(GetVideoConfigFileName())) {
        _spawnl(0, "3DSetup.EXE", "3DSetup.EXE", GetVideoConfigFileName(), NULL);
    }
    if (!FileExists(GetVideoConfigFileName())) {
        return 0;
    }
    if (!Function42B830()) {
        return 0;
    }
    ShowCursor(FALSE);
    if (!BringUpEngine(hInstance, nShowCmd)) {
        return 0;
    }
    gfApplicationActive = 1;
    g_run_flag_6f0628 = 1;
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
            Function4E3340();
            gfSGPInputReceived = 0;
        }
    } while (g_run_flag_6f0628);
    PostQuitMessage(0);
    return message.wParam;
}

}
