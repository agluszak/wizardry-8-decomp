#include "Types.h"
#include "mousesystem.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/game_status.h"
#include "wiz8/engine_code/GameData.h"
#include "wiz8/render_state.h"
#include "wiz8/sound_man.h"
#include "Button System.h"
#include "Font.h"
#include "input.h"
#include "wiz8/sgp_input_private.h"
#include "wiz8/sgp_private.h"
#include "timer.h"
#include "vsurface.h"


/*
 * The product window procedure InitializeStandardGamingPlatform hands to the
 * renderer. It retains the released Wizardry message handling shape, but its
 * WM_DESTROY path omits released sound-stream servicing.
 *
 * WM_SIZING holds the window to 4:3. The original divides by 480 and by 640
 * through the usual reciprocal-multiply sequences, which is what (height * 640)
 * / 480 and (width * 480) / 640 compile to.
 */

// GLOBAL: WIZ8 0x00650DB0
static int fRestore;


// FUNCTION: WIZ8 0x004011e0
extern "C" INT32 FAR PASCAL WindowProcedure(
    HWND window, UINT16 message, WPARAM wparam, LPARAM lparam)
{
    RECT* rect;
    int right;
    int top;
    int width;
    int height;
    unsigned char move_left;

    if (gfIgnoreMessages) {
        return DefWindowProcA(window, message & 0xffff, wparam, lparam);
    }
    message &= 0xffff;
    if (message == guiMouseWheelMsg) {
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
            VideoResizeWindow();
            return 0;
        }
        if (wparam != SIZE_MAXIMIZED) {
            return 0;
        }
        VideoFullScreen(1);
        return 0;

    case WM_DESTROY:
        if (!g_sgp_shutdown_reentered) {
            g_sgp_shutdown_reentered = true;
            if (gfGameInitialized) {
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
        PostQuitMessage(0);
        return 0;

    case WM_SETFOCUS:
        if (!VideoInspectorIsEnabled()) {
            RestoreVideoManager();
        }
        gfApplicationActive = 1;
        return 0;

    case WM_KILLFOCUS:
        if (!VideoInspectorIsEnabled()) {
            SuspendVideoManager();
        }
        gfApplicationActive = 0;
        FreeMouseCursor();
        fRestore = 1;
        return 0;

    case WM_ACTIVATEAPP:
        if (wparam == 0) {
            if (!VideoInspectorIsEnabled()) {
                SuspendVideoManager();
            }
            MoveTimer(1);
            fRestore = 1;
            gfApplicationActive = 0;
            return 0;
        }
        if (wparam != 1) {
            return 0;
        }
        if (fRestore != 1) {
            return 0;
        }
        if (!VideoInspectorIsEnabled()) {
            RestoreVideoManager();
            RestoreVideoSurfaces();
        }
        MoveTimer(8);
        gfApplicationActive = 1;
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
        return DefWindowProcA(window, message, wparam, lparam);
    }


}
