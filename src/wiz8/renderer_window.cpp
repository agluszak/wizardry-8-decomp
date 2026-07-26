#include "wiz8/wiz8_windows.h"

#include <direct.h>
#include <string.h>

/*
 * The renderer window and extension loading gate BringUpEngine calls after the
 * input manager. Its callees and globals are almost all unidentified, so they
 * carry address-derived names; only the SR.DLL entry points and the window
 * handle have real ones.
 */

/* Declared to produce exactly the decorated names Wiz8.exe imports:
   ?isWindowOpen@srGERD@@QBEHXZ and ?load@srExtension@@SAPAV1@PBD0@Z. The
   dllimport is not decoration: without it VC6 emits a five-byte direct call to
   the import thunk where the canonical has a six-byte indirect call through the
   import address table. */
class __declspec(dllimport) srGERD {
public:
    int isWindowOpen() const;
};

class __declspec(dllimport) srExtension {
public:
    static srExtension* load(const char* name, const char* path);
};

extern "C" {

extern void* ghWindow;
extern unsigned char g_flag_603c38;
extern int g_dword_65962c;
extern int g_dword_6596fc;
extern unsigned int g_tick_659700;
extern int g_dword_6596dc;
extern int g_dword_6596e0;
extern unsigned char g_flags_6596e8[2];
extern void* g_instance_654ac4;
extern unsigned short g_show_command_659620;
extern void* g_window_proc_6595f8;
extern unsigned char g_flag_659710;
extern unsigned char g_flag_6840bc;
extern unsigned char g_flag_65970f;
extern srGERD* g_gerd_659634;
extern void* g_surface_65964c;
extern void* g_surface_659650;
extern void* g_surface_659654;
extern void* g_surface_659658;
extern unsigned char g_block_652ddc[0x12c0];
extern unsigned int g_index_6596e4;
extern int g_dword_6596d8;
extern int g_dword_6596ec;
extern int g_dword_6596f0;

extern void Function4265C0(void);
extern unsigned char Function425EC0(void);
extern unsigned char Function426080(void);
extern unsigned char Function422240(void);
extern void Function423500(void);
extern void Function56AAB0(void);
extern unsigned char Function422800(void);
extern void Function426500(void* surface);
extern void Function422D50(int a, int b, int c, int d, int e);
extern void Function426250(int a, int b, int c, int d);
extern unsigned char Function44F060(void);
extern void Function47B5F0(void);
extern unsigned char Function4285C0(void);

/* Brings the renderer up, shows the window and, when the INSPECTOR switch was
   given, loads that extension from the DLL subdirectory before returning to the
   original working directory. Each gate that fails returns straight out with
   the callee's own false still in AL. */
// FUNCTION: WIZ8 0x00421BB0
unsigned char Function421BB0(void* instance, unsigned short show_command, void* window_proc)
{
    MEMORYSTATUS status;
    unsigned int active;

    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    g_flag_603c38 = 1;
    g_dword_65962c = 0;
    g_dword_6596fc = 0;
    g_tick_659700 = GetTickCount();
    g_dword_6596dc = 0;
    g_dword_6596e0 = 0;
    g_flags_6596e8[0] = 0;
    g_flags_6596e8[1] = 0;
    g_instance_654ac4 = instance;
    g_show_command_659620 = show_command;
    g_window_proc_6595f8 = window_proc;
    Function4265C0();
    if (!Function425EC0()) {
        return 0;
    }
    if (!Function426080()) {
        return 0;
    }
    if (!Function422240()) {
        return 0;
    }
    Function423500();
    if (!g_flag_659710) {
        if (g_flag_6840bc) {
            Function56AAB0();
        }
        if (ghWindow && g_gerd_659634) {
            g_flag_659710 = 1;
            ShowWindow((HWND)ghWindow, 9);
            if (g_gerd_659634->isWindowOpen() == 0) {
                if (!Function422800()) {
                    goto done;
                }
            }
            OpenIcon((HWND)ghWindow);
            SetFocus((HWND)ghWindow);
            memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
            active = g_index_6596e4;
            g_flags_6596e8[g_index_6596e4 ^ 1] = 0;
            g_dword_6596d8 = 0;
            g_flags_6596e8[active] = 0;
            Function426500(g_surface_65964c);
            Function426500(g_surface_659654);
            Function426500(g_surface_659650);
            Function426500(g_surface_659658);
            Function422D50(0, 0, 0x280, 0x1e0, 0);
            g_dword_6596f0 = 2;
            g_dword_6596ec = 2;
        }
    }
done:
    Function426250(0, 0, 0x280, 0x1e0);
    if (g_flag_65970f) {
        _chdir("DLL");
        srExtension::load("INSPECTOR", 0);
        _chdir(".");
    }
    if (!Function44F060()) {
        return 0;
    }
    Function47B5F0();
    return Function4285C0();
}

}
