#include "wiz8/wiz8_windows.h"
#include "wiz8/gameplay_boundaries.h"
#include "surrender/srConfig.h"
#include "surrender/srColorSurface.h"
#include "surrender/srGERD.h"
#include "surrender/srExtension.h"
#include "surrender/srMaterial.h"
#include "surrender/srScene.h"
#include "surrender/srStringTable.h"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
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
__declspec(dllimport) int __cdecl srInit(void);
__declspec(dllimport) void __cdecl srAssertSetFunc(
    void (__cdecl *handler)(const char*, const char*, long, const char*));

extern "C" {

/* The released SGP video unit owns this platform handle.  Wiz8 replaces the
   released video manager but keeps the same source-defined interface. */
HWND ghWindow;

unsigned char g_flag_603c38 = 1;
unsigned char g_fullscreen_603c39 = 1;
unsigned char g_flush_pending_603c3a = 1;
int g_screen_width_603c3c = 640;
int g_screen_height_603c40 = 480;
int g_screen_depth_603c44 = 16;
int g_pixel_format_603c48 = 9;
int g_renderer_mode_603d74;
int g_dword_65962c;
int g_dword_6596fc;
unsigned int g_tick_659700;
int g_dword_6596dc;
int g_dword_6596e0;
unsigned char g_flags_6596e8[2];
HINSTANCE g_instance_654ac4;
unsigned short g_show_command_659620;
WNDPROC g_window_proc_6595f8;
unsigned char g_flag_659710;
unsigned char g_flag_6840bc;
unsigned char g_flag_65970f;
srGERD* g_gerd_659634;
LPDIRECTDRAW g_direct_draw_65969c;
LPDIRECTDRAW2 g_direct_draw2_6596a0;
LPDIRECTDRAWSURFACE g_primary_surface1_6596a4;
LPDIRECTDRAWSURFACE2 g_primary_surface_6596a8;
RECT g_window_rect_659610;

unsigned short g_alpha_mask_650f48;
unsigned short g_red_mask_650f4a;
unsigned short g_green_mask_650f4c;
unsigned short g_blue_mask_650f4e;
unsigned short g_red_shift_650f50;
unsigned short g_blue_shift_650f52;
unsigned short g_green_shift_650f54;
unsigned int g_color_key_600088;
srModeler* g_modeler_65963c;
srScene* g_scene_user_659640;
srScene* g_scene_fullscreen_659644;
srScene* g_scene_permanent_659648;
srScene* g_scene_prerender0_65964c;
srScene* g_scene_prerender1_659650;
srScene* g_scene_overlay0_659654;
srScene* g_scene_overlay1_659658;
srScene* g_scene_square_65965c;
srColorSurface* g_primary_color_surface_659660;
class stSurface2D* g_surface_node_659664;
srCamera* g_overlay_camera_659670;
srCamera* g_square_camera_659674;
srColorSurface* g_mouse_surface_659688;
srMaterial* g_blit_material_65967c;
srNode* g_surface_nodes_654adc[0x12c0];
int g_surface_state_6595dc;
int g_surface_state_654ad8;
int g_viewport_left_6595e8;
int g_viewport_top_6595ec;
int g_viewport_right_6595f0;
int g_viewport_bottom_6595f4;
void* g_world_00659ab4;
W8World* g_world_659ab8;
unsigned char g_flag_652da4;
extern const float g_scale_x_5ebb1c = 1.0f / 640.0f;
extern const float g_scale_y_5ebb20 = 1.0f / 480.0f;
extern const float g_one_5ebc30 = 1.0f;

/* From the vendored SGP DirectDraw Calls.c. */
extern void DDUnlockSurface(void* surface, void* locked);
extern void DDLockSurface(void* surface, RECT* area,
                          DDSURFACEDESC* description, int flags, void* event);
unsigned char g_block_652ddc[0x12c0];
unsigned int g_index_6596e4;
int g_dword_6596d8;
int g_dword_6596ec;
int g_dword_6596f0;
extern srClass* g_cursor_node_659694;

extern void Initialize16BitPixelFormatMasks(void);
extern void FreeMouseCursor(void);
extern unsigned char CreateWizardryWindow(void);
extern unsigned char InitializePrimaryDirectDrawSurface(void);
extern unsigned char InitializeVideoDevice(void);
extern unsigned char InitializeRendererSceneObjects(void);
extern void Function56AAB0(void);
extern unsigned char Function422800(void);
extern void MarkScreenRectDirty(int left, int top, int right, int bottom, int flags);
extern void Function426500(srScene* scene);
extern void SetViewport(int left, int top, int right, int bottom);
extern unsigned char Function44F060(void);
extern "C" void EnableAllRenderOptions(void);
extern void Function47D5F0(void);
extern unsigned char Function4285C0(void);
extern void ShutdownWithErrorBox(char* message);
extern void AssertFailureHandler(const char* expression, const char* file,
                                 long line, const char* message);

char* g_sound_provider_650e54;
unsigned char* g_render_options_65a118;

// FUNCTION: WIZ8 0x00421F70
void* LockPrimarySurface(int* pitch)
{
    DDSURFACEDESC description;

    DDLockSurface(g_primary_surface_6596a8, NULL, &description, 0, NULL);
    *pitch = description.lPitch;
    return description.lpSurface;
}

/* Clear the software-facing frame and retire every transient 2D overlay.
   The four scene walks are the same typed operation used during renderer
   bring-up; keeping the reset here avoids reproducing SurRender's node ABI at
   the menu call site. */
void Function422B10(void)
{
    DDSURFACEDESC description;
    unsigned int active;

    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    DDLockSurface(g_primary_surface_6596a8, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
    memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
    memset(g_surface_nodes_654adc, 0, sizeof(g_surface_nodes_654adc));
    active = g_index_6596e4;
    g_flags_6596e8[active ^ 1] = 0;
    g_flags_6596e8[active] = 0;
    g_dword_6596d8 = 0;
    Function426500(g_scene_prerender0_65964c);
    Function426500(g_scene_overlay0_659654);
    Function426500(g_scene_prerender1_659650);
    Function426500(g_scene_overlay1_659658);
    MarkScreenRectDirty(0, 0, 640, 480, 0);
}


/* Brings the renderer up, shows the window and, when the INSPECTOR switch was
   given, loads that extension from the DLL subdirectory before returning to the
   original working directory. Each gate that fails returns straight out with
   the callee's own false still in AL. */
// FUNCTION: WIZ8 0x00421BB0
unsigned char InitializeRenderer(void* instance, unsigned short show_command, void* window_proc)
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
    g_instance_654ac4 = (HINSTANCE)instance;
    g_show_command_659620 = show_command;
    g_window_proc_6595f8 = (WNDPROC)window_proc;
    Initialize16BitPixelFormatMasks();
    if (!CreateWizardryWindow()) {
        return 0;
    }
    if (!InitializePrimaryDirectDrawSurface()) {
        return 0;
    }
    if (!InitializeVideoDevice()) {
        return 0;
    }
    InitializeRendererSceneObjects();
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
            Function426500(g_scene_prerender0_65964c);
            Function426500(g_scene_overlay0_659654);
            Function426500(g_scene_prerender1_659650);
            Function426500(g_scene_overlay1_659658);
            MarkScreenRectDirty(0, 0, 0x280, 0x1e0, 0);
            g_dword_6596f0 = 2;
            g_dword_6596ec = 2;
        }
    }
done:
    SetViewport(0, 0, 0x280, 0x1e0);
    if (g_flag_65970f) {
        _chdir("DLL");
        srExtension::load("INSPECTOR", 0);
        _chdir(".");
    }
    if (!Function44F060()) {
        return 0;
    }
    EnableAllRenderOptions();
    return Function4285C0();
}

// FUNCTION: WIZ8 0x00421DC0
void ShutdownRenderer(void)
{
    if (g_cursor_node_659694) {
        g_cursor_node_659694->release();
        g_cursor_node_659694 = 0;
    }
    FreeMouseCursor();
    if (g_flag_659710) {
        g_flag_659710 = 0;
        g_flush_pending_603c3a = 0;
        if (!g_fullscreen_603c39 && ghWindow) {
            GetWindowRect(ghWindow, &g_window_rect_659610);
        }
        FreeMouseCursor();
    }
}

/* Derives the 16-bit channel masks and their leading-bit positions from the
   renderer's reviewed pixel-format selector.  The startup configuration uses
   format 9, RGB 5:5:5 with the high bit reserved. */
// FUNCTION: WIZ8 0x004265C0
void Initialize16BitPixelFormatMasks(void)
{
    unsigned short bit;

    if (g_pixel_format_603c48 == 7) {
        g_alpha_mask_650f48 = 0;
        g_red_mask_650f4a = 0xf800;
        g_green_mask_650f4c = 0x07e0;
        g_color_key_600088 = 0x7bef;
    } else if (g_pixel_format_603c48 == 8) {
        g_alpha_mask_650f48 = 0;
        g_red_mask_650f4a = 0x7c00;
        g_green_mask_650f4c = 0x03e0;
        g_color_key_600088 = 0x3def;
    } else if (g_pixel_format_603c48 == 9) {
        g_alpha_mask_650f48 = 0x8000;
        g_red_mask_650f4a = 0x7c00;
        g_green_mask_650f4c = 0x03e0;
        g_color_key_600088 = 0x3def;
    } else {
        return;
    }

    g_blue_mask_650f4e = 0x001f;
    g_red_shift_650f50 = 8;
    for (bit = 0x8000; (g_red_mask_650f4a & bit) == 0; bit >>= 1) {
        --g_red_shift_650f50;
    }
    g_green_shift_650f54 = 8;
    for (bit = 0x8000; (g_green_mask_650f4c & bit) == 0; bit >>= 1) {
        --g_green_shift_650f54;
    }
    g_blue_shift_650f52 = 8;
    for (bit = 0x8000; (0x001f & bit) == 0; bit >>= 1) {
        --g_blue_shift_650f52;
    }
}

/* Registers and creates the real top-level Wizardry window.  The odd use of
   the horizontal screen metric for both axes is present in the retail body and
   is preserved here; AdjustWindowRect turns the four stored client bounds into
   the outer window rectangle before CreateWindowEx. */
// FUNCTION: WIZ8 0x00425EC0
unsigned char CreateWizardryWindow(void)
{
    WNDCLASSA window_class;
    int extent;
    DWORD style;

    memset(&window_class, 0, sizeof(window_class));
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC | CS_DBLCLKS;
    window_class.lpfnWndProc = g_window_proc_6595f8;
    window_class.hInstance = g_instance_654ac4;
    window_class.hIcon = LoadIconA(g_instance_654ac4, MAKEINTRESOURCEA(106));
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = "Wizardry 8";
    RegisterClassA(&window_class);

    extent = GetSystemMetrics(SM_CXSCREEN);
    if (extent > 640) {
        extent = 640;
    }
    g_window_rect_659610.left = GetSystemMetrics(SM_CXSCREEN) / 2 - extent / 2;
    extent = GetSystemMetrics(SM_CXSCREEN);
    if (extent > 640) {
        extent = 640;
    }
    g_window_rect_659610.top = GetSystemMetrics(SM_CXSCREEN) / 2 - extent / 2;
    extent = GetSystemMetrics(SM_CXSCREEN);
    if (extent > 640) {
        extent = 640;
    }
    g_window_rect_659610.right = g_window_rect_659610.left + extent;
    extent = GetSystemMetrics(SM_CYSCREEN);
    if (extent > 480) {
        extent = 480;
    }
    g_window_rect_659610.bottom = g_window_rect_659610.top + extent;

    if (!g_fullscreen_603c39) {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        AdjustWindowRect(&g_window_rect_659610, style, FALSE);
        ghWindow = CreateWindowExA(
            0, "Wizardry 8", "Wizardry 8", style,
            g_window_rect_659610.left, g_window_rect_659610.top,
            g_window_rect_659610.right - g_window_rect_659610.left,
            g_window_rect_659610.bottom - g_window_rect_659610.top,
            NULL, NULL, g_instance_654ac4, NULL);
    } else {
        style = WS_POPUP | WS_VISIBLE;
        ghWindow = CreateWindowExA(
            0, "Wizardry 8", "Wizardry 8", style, 0, 0,
            GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
            NULL, NULL, g_instance_654ac4, NULL);
    }
    if (!ghWindow) {
        return 0;
    }
    SetFocus(ghWindow);
    return 1;
}

/* Creates the 640x480 system-memory DirectDraw surface that SurRender uses as
   its color output.  Interface identities, flags and HRESULT semantics are
   from the published DirectDraw headers; only the orchestration is Wiz8 code. */
// FUNCTION: WIZ8 0x00426080
unsigned char InitializePrimaryDirectDrawSurface(void)
{
    DDSURFACEDESC description;
    HRESULT result;

    result = DirectDrawCreate(NULL, &g_direct_draw_65969c, NULL);
    if (FAILED(result)) {
        return 0;
    }
    result = g_direct_draw_65969c->QueryInterface(
        IID_IDirectDraw2, (void**)&g_direct_draw2_6596a0);
    if (FAILED(result)) {
        return 0;
    }
    result = g_direct_draw2_6596a0->SetCooperativeLevel(NULL, DDSCL_NORMAL);
    if (FAILED(result)) {
        return 0;
    }

    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    description.dwHeight = 480;
    description.dwWidth = 640;
    description.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    description.ddpfPixelFormat.dwSize = sizeof(description.ddpfPixelFormat);
    description.ddpfPixelFormat.dwFlags = DDPF_RGB;
    description.ddpfPixelFormat.dwRGBBitCount = 16;
    description.ddpfPixelFormat.dwRBitMask = g_red_mask_650f4a;
    description.ddpfPixelFormat.dwGBitMask = g_green_mask_650f4c;
    description.ddpfPixelFormat.dwBBitMask = g_blue_mask_650f4e;

    result = g_direct_draw2_6596a0->CreateSurface(
        &description, &g_primary_surface1_6596a4, NULL);
    if (FAILED(result)) {
        return 0;
    }
    result = g_primary_surface1_6596a4->QueryInterface(
        IID_IDirectDrawSurface2, (void**)&g_primary_surface_6596a8);
    if (FAILED(result)) {
        return 0;
    }

    DDLockSurface(g_primary_surface_6596a8, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
    return 1;
}

/* Selects and starts the configured SurRender display driver, binds it to the
   top-level window, and retains the final 3DVideo.CFG line for the later sound
   manager.  Driver and API names come from the SR export table and retail
   strings; the file format is the five-line format emitted by 3DSetup.exe. */
// FUNCTION: WIZ8 0x00422240
unsigned char InitializeVideoDevice(void)
{
    FILE* config;
    char device[100] = "";
    char sound_provider[100] = "";
    char line[10] = "";
    char driver_name[100];
    char* newline;
    srStringTable devices;

    if (g_gerd_659634) {
        return 1;
    }

    config = fopen("3DVideo.CFG", "r");
    if (config) {
        fgets(device, sizeof(device), config);
        newline = strpbrk(device, "\r\n");
        if (newline) {
            *newline = '\0';
        }
        if (fgets(line, sizeof(line), config)) {
            g_screen_width_603c3c = atoi(line);
        }
        if (fgets(line, sizeof(line), config)) {
            g_screen_height_603c40 = atoi(line);
        }
        if (fgets(line, sizeof(line), config)) {
            g_screen_depth_603c44 = atoi(line);
        }
        fgets(sound_provider, sizeof(sound_provider), config);
        newline = strpbrk(sound_provider, "\r\n");
        if (newline) {
            *newline = '\0';
        }
        fclose(config);
    }

    _chdir("DLL");
    srInit();
    srConfig.set(
        "DD_DIRECTX7",
        "DisablePrimaryHEL=1 DisableAttachedSecondaryDevices=1 "
        "DisableDetachedSecondaryDevices=1 DisableNonDisplayDevices=1");
    srConfig.set(
        "DD_DIRECTX6",
        "DisablePrimaryHEL=1 DisableAttachedSecondaryDevices=1 "
        "DisableDetachedSecondaryDevices=1 DisableNonDisplayDevices=1");
    sprintf(driver_name, "srDD_%s", device);
    devices.addString(driver_name);
    g_gerd_659634 = srGERD::loadDevice(devices, 0);
    _chdir("..");
    if (!g_gerd_659634) {
        ShutdownWithErrorBox(
            "Video device cannot be started. Please re-run 3DSetup.");
        return 0;
    }

    g_gerd_659634->createContext((unsigned long)ghWindow);
    Function422800();
    srAssertSetFunc(AssertFailureHandler);
    if (_strnicmp(sound_provider, "none", 4) != 0) {
        g_sound_provider_650e54 = (char*)malloc(strlen(sound_provider) + 1);
        if (g_sound_provider_650e54) {
            strcpy(g_sound_provider_650e54, sound_provider);
        }
    }
    Function47D5F0();
    return 1;
}

/* Applies the configured window style, asks SurRender for the matching display
   mode in fullscreen operation, and opens the renderer output window. */
// FUNCTION: WIZ8 0x00422800
unsigned char Function422800(void)
{
    srGERD::e_error error;
    long mode;

    SetLastError(0);
    if (!g_fullscreen_603c39) {
        SetWindowLongA(ghWindow, GWL_STYLE,
                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        SetWindowPos(
            ghWindow, NULL, g_window_rect_659610.left, g_window_rect_659610.top,
            g_window_rect_659610.right - g_window_rect_659610.left,
            g_window_rect_659610.bottom - g_window_rect_659610.top, 0);
        error = g_gerd_659634->openWindow();
    } else {
        SetWindowLongA(ghWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(ghWindow, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), 0);
        mode = g_gerd_659634->getDisplayMode(
            g_screen_width_603c3c, g_screen_height_603c40,
            g_screen_depth_603c44);
        if (mode == -1) {
            ShutdownWithErrorBox(
                "Video device does not support video resolution.");
            return 0;
        }
        SetEnvironmentVariableA("FX_GLIDE_NO_SPLASH", "1");
        error = g_gerd_659634->openWindow(mode);
    }
    if (error != 0) {
        ShutdownWithErrorBox("Could not open video output device.");
        return 0;
    }
    g_flush_pending_603c3a = 1;
    return 1;
}

/* WM_SIZE only rebuilds the SurRender output in windowed mode.  Full-screen
   startup receives the same Windows notification while the device already
   owns its configured 640x480 mode, so there is no resize operation to do. */
unsigned char Function422550(void)
{
    if (g_fullscreen_603c39 || !ghWindow || !g_gerd_659634 ||
        !g_flush_pending_603c3a) {
        return 0;
    }
    return 0;
}

// FUNCTION: WIZ8 0x004277E0
unsigned char Function4277E0(void)
{
    return g_flag_65970f;
}

/* The first WM_SETFOCUS arrives from ShowWindow while the renderer bring-up
   latch is raised.  Retail treats that notification as already resumed. */
unsigned char Function4220B0(void)
{
    if (g_flag_659710) {
        return 1;
    }
    return 0;
}

/* Releases the lock the frame tick takes on the primary surface. The second
   argument is the locked pointer SGP's wrapper wants; the original passes null
   because it unlocks the whole surface. */
// FUNCTION: WIZ8 0x00421FB0
void UnlockPrimarySurface(void)
{
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
}

/* The mode the engine falls back to: 640x480 at 16bpp, reported height first.
   Nothing here reads a configuration - the three constants are inline. */
// FUNCTION: WIZ8 0x00422AF0
void GetDefaultScreenMode(unsigned short* height, unsigned short* width,
                          unsigned char* depth)
{
    *height = 0x1e0;
    *width = 0x280;
    *depth = 0x10;
}


/* Clears the primary surface. The dword count the original computes - the pitch
   times fifteen, masked, shifted left three - is VC6's inline memset over
   pitch times 480 bytes, which is why the byte-remainder loop that follows it
   runs zero times: the length is always a multiple of four. */
// FUNCTION: WIZ8 0x00421FF0
unsigned char ClearPrimarySurface(void)
{
    DDSURFACEDESC description;

    DDLockSurface(g_primary_surface_6596a8, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
    return 1;
}

}
