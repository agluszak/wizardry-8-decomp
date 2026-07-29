#include "wiz8/wiz8_windows.h"
#include "Button System.h"
#include "Font.h"
#include "input.h"
#include "timer.h"

/*
 * The window procedure BringUpEngine hands to the renderer. Ghidra defines no
 * function at this address and the project lock could not be taken to create
 * one, so this is reconstructed from the canonical encoding and its two jump
 * tables rather than from decompiler output.
 *
 * WM_SIZING holds the window to 4:3. The original divides by 480 and by 640
 * through the usual reciprocal-multiply sequences, which is what (height * 640)
 * / 480 and (width * 480) / 640 compile to.
 */

extern "C" {

unsigned char g_flag_650dac;
unsigned char g_flag_6f0630;
extern unsigned int g_mswheel_roll_message;
int g_dword_650db0;
extern unsigned char g_flag_6505a9;
extern bool g_teardown_done_650db4;

extern void Function422970(int enable);
extern void Function422550(void);
extern unsigned char Function4277E0(void);
extern void Function4220B0(void);
extern void Function422050(void);
extern void Function4029F0(void);
extern float Function420B40(int value);
extern void ShutdownGameData(void);
extern void ShutdownDisplayList(void);
extern void Function408850(void);
extern bool ShutdownWizardryVideoSurfaceManager(void);
extern void ShutdownWizardryVideoObjectManager(void);
extern void ShutdownRenderer(void);
extern void NoOp(void);
extern void ShutdownVideoSurfaceState(void);

// FUNCTION: WIZ8 0x004011e0
long __stdcall WindowProc4011E0(void* window, int message,
                                unsigned int wparam, long lparam)
{
    RECT* rect;
    int right;
    int top;
    int width;
    int height;
    unsigned char move_left;

    if (g_flag_650dac) {
        return DefWindowProcA((HWND)window, message & 0xffff, wparam, lparam);
    }
    message &= 0xffff;
    if (message == g_mswheel_roll_message) {
        QueueEvent(0x800, wparam, lparam);
        return 0;
    }
    switch (message) {
    case WM_CREATE:
    case WM_MOVE:
        return 0;

    case WM_SIZE:
        if ((short)lparam == 0) {
            return 0;
        }
        if (((unsigned long)lparam >> 16) == 0) {
            return 0;
        }
        if (wparam == SIZE_RESTORED) {
            Function422550();
            return 0;
        }
        if (wparam != SIZE_MAXIMIZED) {
            return 0;
        }
        Function422970(1);
        return 0;

    case WM_DESTROY:
        if (!g_teardown_done_650db4) {
            g_teardown_done_650db4 = true;
            if (g_flag_6505a9) {
                ShutdownGameData();
            }
            ShutdownButtonSystem();
            ShutdownDisplayList();
            Function408850();
            DestroyEnglishTransTable();
            ShutdownFontManager();
            ShutdownClockManager();
            ShutdownWizardryVideoSurfaceManager();
            ShutdownWizardryVideoObjectManager();
            ShutdownRenderer();
            ShutdownInputManager();
            NoOp();
            NoOp();
            ShutdownVideoSurfaceState();
            NoOp();
        }
        ShowCursor(TRUE);
        PostQuitMessage(0);
        return 0;

    case WM_SETFOCUS:
        if (!Function4277E0()) {
            Function4220B0();
        }
        g_flag_6f0630 = 1;
        return 0;

    case WM_KILLFOCUS:
        if (!Function4277E0()) {
            Function422050();
        }
        g_flag_6f0630 = 0;
        FreeMouseCursor();
        g_dword_650db0 = 1;
        return 0;

    case WM_ACTIVATEAPP:
        if (wparam == 0) {
            if (!Function4277E0()) {
                Function422050();
            }
            Function420B40(1);
            g_dword_650db0 = 1;
            g_flag_6f0630 = 0;
            return 0;
        }
        if (wparam != 1) {
            return 0;
        }
        if (g_dword_650db0 != 1) {
            return 0;
        }
        if (!Function4277E0()) {
            Function4220B0();
            Function4029F0();
        }
        Function420B40(8);
        g_flag_6f0630 = 1;
        return 0;

    case WM_MOUSEMOVE:
        return 0;

    /* WM_MOUSEWHEEL; the VC6 headers only define it above this target's
       _WIN32_WINNT, and the canonical registers MSWHEEL_ROLLMSG for the same
       purpose on older shells. */
    case 0x020a:
        QueueEvent(0x800, wparam, lparam);
        return 0;

    case WM_SIZING:
        /* Holds the window to 4:3 against 640x480. The original reads the two
           edges it needs once, and both the top and the bottom group converge
           on one scaling tail rather than each carrying its own copy. */
        rect = (RECT*)lparam;
        move_left = 0;
        right = rect->right;
        top = rect->top;
        width = right - rect->left;
        height = rect->bottom - top;
        switch (wparam) {
        case WMSZ_LEFT:
            if (width >= 640) {
                goto scale_height;
            }
            rect->left = right - 640;
            break;
        case WMSZ_RIGHT:
            if (width >= 640) {
                goto scale_height;
            }
            rect->right = rect->left + 640;
            break;
        case WMSZ_TOPLEFT:
            move_left = 1;
            /* fall through */
        case WMSZ_TOP:
        case WMSZ_TOPRIGHT:
            if (height < 480) {
                rect->top = rect->bottom - 480;
                height = 480;
            }
            goto scale_width;
        case WMSZ_BOTTOMLEFT:
            move_left = 1;
            /* fall through */
        case WMSZ_BOTTOM:
        case WMSZ_BOTTOMRIGHT:
            if (height >= 480) {
                goto scale_width;
            }
            rect->bottom = top + 480;
            height = 480;
            goto scale_width;
        default:
            goto scale_height;
        }
        width = 640;
    scale_height:
        rect->bottom = top + (width * 480) / 640;
        return 0;

    scale_width:
        width = (height * 640) / 480;
        if (move_left) {
            rect->left = right - width;
        } else {
            rect->right = rect->left + width;
        }
        return 0;

    default:
        return DefWindowProcA((HWND)window, message, wparam, lparam);
    }
}

}
