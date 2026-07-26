#include "wiz8/wiz8_windows.h"

#include "wiz8/gameplay_boundaries.h"

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

// FUNCTION: WIZ8 0x005B1740
/* Shared success stub. BringUpEngine uses it as a gate and the 62-entry frame
   dispatch table parks it in seventeen slots. */
unsigned char Function5B1740(void)
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
