#include "wiz8/wiz8_windows.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/cursor.h"
#include "wiz8/regions.h"
#include "wiz8/sgp_video.h"
#include "wiz8/sr_api.h"
#include "DirectDraw Calls.h"
#include "input.h"
#include "sgp.h"
#include "surrender/srConfig.h"
#include "surrender/srColorSurface.h"
#include "surrender/srGERD.h"
#include "surrender/srExtension.h"
#include "surrender/srMaterial.h"
#include "surrender/srCore.h"
#include "surrender/srMeshModel.h"
#include "surrender/srScene.h"
#include "surrender/srStatisticsManager.h"
#include "surrender/srStringTable.h"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Function56AAB0(void);
void Function56AA30(void);

/*
 * The renderer window and extension loading gate BringUpEngine calls after the
 * input manager. Its callees and globals are almost all unidentified, so they
 * carry address-derived names; only the SR.DLL entry points and the window
 * handle have real ones.
 */

extern "C" {

/* The released SGP video unit owns this platform handle.  Wiz8 replaces the
   released video manager but keeps the same source-defined interface. */
HWND ghWindow;

unsigned char g_flag_603c38 = 1;
unsigned char g_flag_603c4c = 1;
unsigned char g_flag_603c60 = 1;
unsigned char g_flag_603c6d = 1;
int g_frame_reset_interval_603c68 = 50;
unsigned char g_fullscreen_603c39 = 1;
unsigned char g_flush_pending_603c3a = 1;
int g_screen_width_603c3c = 640;
int g_screen_height_603c40 = 480;
int g_screen_depth_603c44 = 16;
int g_pixel_format_603c48 = 9;
int g_renderer_mode_603d74;
srModelInstance* g_current_model_instance_65962c;
int g_dword_6596fc;
unsigned int g_tick_659700;
int g_dword_6596dc;
int g_dword_6596e0;
unsigned char g_flags_6596e8[2];
HINSTANCE g_instance_654ac4;
unsigned short g_show_command_659620;
WNDPROC g_window_proc_6595f8;
unsigned char g_flag_659710;
unsigned char g_flag_65970e;
unsigned char g_flag_659711;
unsigned char g_flag_6596f4;
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
short g_red_shift_650f50;
short g_blue_shift_650f52;
short g_green_shift_650f54;
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
// GLOBAL: WIZ8 0x00659AB4
W8World* g_world;
W8World* g_world_659ab8;
unsigned char g_flag_652da4;
extern const float g_scale_x_5ebb1c = 1.0f / 640.0f;
extern const float g_scale_y_5ebb20 = 1.0f / 480.0f;
extern const float g_one_5ebc30 = 1.0f;

unsigned char g_block_652ddc[0x12c0];
unsigned int g_index_6596e4;
int g_dword_6596d8;
int g_dword_6596ec;
int g_dword_6596f0;
int g_value_659668;
unsigned int g_tick_65409c;
float g_frames_per_second_659704;
float g_seconds_per_frame_659708;
extern srClass* g_cursor_node_659694;

extern void Initialize16BitPixelFormatMasks(void);
extern unsigned char CreateWizardryWindow(void);
extern unsigned char InitializePrimaryDirectDrawSurface(void);
extern unsigned char InitializeVideoDevice(void);
extern unsigned char InitializeRendererSceneObjects(void);
extern void Function402750(void);
extern unsigned char Function422800(void);
extern void PurgeInactiveSceneInstances(srScene* scene);
extern void SetViewport(int left, int top, int right, int bottom);
extern unsigned char Function44F060(void);
extern "C" void EnableAllRenderOptions(void);
extern void Function47D5F0(void);
extern unsigned char Function4285C0(void);
extern void AssertFailureHandler(const char* expression, const char* file,
                                 long line, const char* message);

char* g_sound_provider_650e54;
unsigned char* g_render_options_65a118;

// FUNCTION: WIZ8 0x00421f70
PTR LockPrimarySurface(UINT32* pitch)
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
    PurgeInactiveSceneInstances(g_scene_prerender0_65964c);
    PurgeInactiveSceneInstances(g_scene_overlay0_659654);
    PurgeInactiveSceneInstances(g_scene_prerender1_659650);
    PurgeInactiveSceneInstances(g_scene_overlay1_659658);
    MarkScreenRectDirty(0, 0, 640, 480, 0);
}


/* Brings the renderer up, shows the window and, when the INSPECTOR switch was
   given, loads that extension from the DLL subdirectory before returning to the
   original working directory. Each gate that fails returns straight out with
   the callee's own false still in AL. */
// FUNCTION: WIZ8 0x00421bb0
unsigned char InitializeRenderer(void* instance, unsigned short show_command, void* window_proc)
{
    MEMORYSTATUS status;
    unsigned int active;

    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    g_flag_603c38 = 1;
    g_current_model_instance_65962c = 0;
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
            PurgeInactiveSceneInstances(g_scene_prerender0_65964c);
            PurgeInactiveSceneInstances(g_scene_overlay0_659654);
            PurgeInactiveSceneInstances(g_scene_prerender1_659650);
            PurgeInactiveSceneInstances(g_scene_overlay1_659658);
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

// FUNCTION: WIZ8 0x00421dc0
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
// FUNCTION: WIZ8 0x004265c0
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
// FUNCTION: WIZ8 0x00425ec0
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

// FUNCTION: WIZ8 0x004277e0
unsigned char Function4277E0(void)
{
    return g_flag_65970f;
}

// FUNCTION: WIZ8 0x00422050
void Function422050(void)
{
    if (g_flag_659710) {
        Function56AA30();
        g_flag_659710 = 0;
        if (g_gerd_659634) {
            g_flush_pending_603c3a = 0;
            g_gerd_659634->closeWindow((srGERD::e_closeHint)0);
        }
        if (!g_fullscreen_603c39) {
            GetWindowRect(ghWindow, &g_window_rect_659610);
        }
        ShowWindow(ghWindow, SW_MINIMIZE);
        Function402750();
    }
}

// FUNCTION: WIZ8 0x004220b0
unsigned char Function4220B0(void)
{
    if (g_flag_659710) {
        return 1;
    }
    if (g_flag_6840bc) {
        Function56AAB0();
    }
    if (ghWindow && g_gerd_659634) {
        g_flag_659710 = 1;
        ShowWindow(ghWindow, SW_RESTORE);
        if (g_gerd_659634->isWindowOpen() != 0 || Function422800()) {
            OpenIcon(ghWindow);
            SetFocus(ghWindow);
            memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
            g_dword_6596d8 = 0;
            g_flags_6596e8[g_index_6596e4 ^ 1] = 0;
            g_flags_6596e8[g_index_6596e4] = 0;
            PurgeInactiveSceneInstances(g_scene_prerender0_65964c);
            PurgeInactiveSceneInstances(g_scene_overlay0_659654);
            PurgeInactiveSceneInstances(g_scene_prerender1_659650);
            PurgeInactiveSceneInstances(g_scene_overlay1_659658);
            MarkScreenRectDirty(0, 0, 0x280, 0x1e0, 0);
            g_dword_6596f0 = 2;
            g_dword_6596ec = 2;
        }
    }
    return 0;
}

/* Releases the lock the frame tick takes on the primary surface. The second
   argument is the locked pointer SGP's wrapper wants; the original passes null
   because it unlocks the whole surface. */
// FUNCTION: WIZ8 0x00421fb0
void UnlockPrimarySurface(void)
{
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
}

/* The mode the engine falls back to: 640x480 at 16bpp, reported height first.
   Nothing here reads a configuration - the three constants are inline. */
// FUNCTION: WIZ8 0x00422af0
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
// FUNCTION: WIZ8 0x00421ff0
unsigned char ClearPrimarySurface(void)
{
    DDSURFACEDESC description;

    DDLockSurface(g_primary_surface_6596a8, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface_6596a8, NULL);
    return 1;
}

}

extern "C" void Function00428340(void);
extern "C" void Function40C0B0(void);
extern "C" void Function425B40(void);
extern "C" srScene* g_cursor_scene_659684;
extern "C" unsigned char g_render_flag_603c6c;
extern "C" int g_resident_texture_policy_659714;
extern unsigned char g_monster_shadow_updates_enabled_0065970c;
extern "C" unsigned char g_flag_65970d;
extern unsigned char g_trigger_action_active_006599c8;
extern srVector3T<float> g_trigger_action_scene_offset_006599ac;

extern void Function4229E0(void);
extern char Function44D760(W8World* world);
extern "C" void Function482140(void);

/* Clamp the three components of a renderer colour independently. The input is
   deliberately not treated as a generic vector operation: the reviewed body
   performs these three scalar saturations in order and returns its argument. */
// FUNCTION: WIZ8 0x004299b0
float* __fastcall Function4299B0(float* color)
{
    if (color[0] <= 0.0f) color[0] = 0.0f;
    else if (color[0] >= 1.0f) color[0] = 1.0f;
    if (color[1] <= 0.0f) color[1] = 0.0f;
    else if (color[1] >= 1.0f) color[1] = 1.0f;
    if (color[2] <= 0.0f) color[2] = 0.0f;
    else if (color[2] >= 1.0f) color[2] = 1.0f;
    return color;
}

/* Render one scene into either the full output or a logical 640x480 viewport.
   Scene fog follows the current world's static scene for ordinary overlays;
   the world passes retain their own fog by selecting preserve_fog. */
// FUNCTION: WIZ8 0x00427850
void Function427850(srScene* scene, srCamera* camera,
                    const int* viewport, char preserve_fog)
{
    unsigned long width = g_gerd_659634->getWidth();
    unsigned long height = g_gerd_659634->getHeight();

    if (scene == 0 || scene->getChildCount() == 0) {
        return;
    }
    if (viewport != 0) {
        unsigned long left = viewport[0] * width / 640;
        unsigned long top = viewport[1] * height / 480;
        unsigned long viewport_width =
            (viewport[2] - viewport[0]) * width / 640;
        unsigned long viewport_height =
            (viewport[3] - viewport[1]) * height / 480;
        g_gerd_659634->setViewPort(
            left, top, viewport_width, viewport_height);
        g_gerd_659634->setScissor(
            left, top, viewport_width, viewport_height);
    }
    if (!preserve_fog) {
        srVector3T<float> fog;
        if (g_world == 0) {
            fog.x = 0.0f;
            fog.y = 0.0f;
            fog.z = 0.0f;
        } else {
            g_world->static_scene->getFogColor(fog);
            Function4299B0(&fog.x);
        }
        scene->setFogColor(fog);
    }
    scene->render(*g_gerd_659634, camera);
    g_gerd_659634->flushRenderers();
    if (viewport != 0) {
        g_gerd_659634->setViewPort(0, 0, width, height);
        g_gerd_659634->setScissor(0, 0, width, height);
    }
}

/* The central renderer frame transaction. It owns the page flip, every scene
   pass, optional world picking, transient-node retirement, and frame timing;
   callers do not reproduce any subset of that lifecycle. */
// FUNCTION: WIZ8 0x00426790
void Function426790(void)
{
    srVector3T<float> clear_color;
    srVector3T<float> saved_world_position;
    srVector3T<float> shifted_world_position;
    unsigned int next_page;
    unsigned long now;
    float elapsed;
    float frames_per_second;
    srScene* first_page;
    srScene* second_page;
    srScene* retire_prerender;
    srScene* retire_overlay;

    clear_color.x = 0.0f;
    clear_color.y = 0.0f;
    clear_color.z = 0.0f;
    Function4299B0(&clear_color.x);
    if (!g_flag_659710) {
        return;
    }

    if (g_trigger_action_active_006599c8 && GetWorld() != 0) {
        GetCameraPosition(&saved_world_position);
        shifted_world_position.x =
            saved_world_position.x + g_trigger_action_scene_offset_006599ac.x;
        shifted_world_position.y =
            saved_world_position.y + g_trigger_action_scene_offset_006599ac.y;
        shifted_world_position.z =
            saved_world_position.z + g_trigger_action_scene_offset_006599ac.z;
        SetWorldScenePosition004511D0(GetWorld(), &shifted_world_position);
    }
    if (g_world != 0) {
        if (!IsSkyEnabled()) {
            GetWorldLightValue(g_world, reinterpret_cast<int*>(&clear_color));
        } else {
            g_world->static_scene->getFogColor(clear_color);
            Function4299B0(&clear_color.x);
        }
    }

    g_gerd_659634->setClearColor(
        clear_color.x, clear_color.y, clear_color.z, 1.0f);
    Function00428340();
    Function40C0B0();
    UpdateRegionHelp();
    Function425B40();
    if (g_gerd_659634 != 0) {
        g_gerd_659634->resetStatistics();
    }
    srCore.getStatisticsManager()->reset();
    g_gerd_659634->beginFrame();
    Function482140();

    if (!g_monster_shadow_updates_enabled_0065970c) {
        if (g_flag_65970e) {
            goto clear_viewport;
        }
    } else if (g_flag_65970e || !g_render_flag_603c6c || g_world_659ab8 == 0) {
clear_viewport:
        {
            unsigned long height = g_gerd_659634->getHeight();
            unsigned long width = g_gerd_659634->getWidth();
            g_gerd_659634->setScissor(
                g_viewport_left_6595e8 * width / 640,
                g_viewport_top_6595ec * height / 480,
                (g_viewport_right_6595f0 - g_viewport_left_6595e8) * width / 640,
                (g_viewport_bottom_6595f4 - g_viewport_top_6595ec) * height / 480);
            g_gerd_659634->clear(srFlags<srGERD::e_buffer>(3));
            g_gerd_659634->setScissor(0, 0, width, height);
        }
    }

    first_page = g_index_6596e4 ? g_scene_prerender0_65964c
                                : g_scene_prerender1_659650;
    second_page = g_index_6596e4 ? g_scene_prerender1_659650
                                 : g_scene_prerender0_65964c;
    Function427850(first_page, g_overlay_camera_659670, 0, 0);
    Function427850(second_page, g_overlay_camera_659670, 0, 0);
    g_gerd_659634->setTextureReduction(g_resident_texture_policy_659714);

    if (g_render_flag_603c6c && g_world_659ab8 != 0 &&
        g_monster_shadow_updates_enabled_0065970c) {
        Function427850(g_world_659ab8->static_scene, g_world_659ab8->camera,
                       &g_viewport_left_6595e8, 0);
    }
    if (g_world != 0 && g_flag_65970d) {
        g_gerd_659634->setTextureReduction(g_resident_texture_policy_659714);
        if (!g_flag_603c38 ||
            g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0 <
                g_viewport_left_6595e8 ||
            g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4 <
                g_viewport_top_6595ec ||
            g_viewport_right_6595f0 <
                g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0 ||
            g_viewport_bottom_6595f4 <
                g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4) {
            Function427850(g_world->static_scene, g_world->camera,
                           &g_viewport_left_6595e8, 1);
        } else {
            int half_width =
                (g_viewport_right_6595f0 - g_viewport_left_6595e8) / 2;
            int half_height =
                (g_viewport_bottom_6595f4 - g_viewport_top_6595ec) / 2;
            srGERD::Pick pick;
            pick.value_00 = static_cast<float>(
                (g_cursor_hotspot_x_6596bc - half_width -
                 g_viewport_left_6595e8 + g_cursor_width_654ad0)) /
                static_cast<float>(half_width);
            pick.value_04 = -static_cast<float>(
                g_cursor_hotspot_y_6596c0 - half_height -
                g_viewport_top_6595ec + g_cursor_height_654ad4) /
                static_cast<float>(half_height);
            pick.value_08 = 1.0f;
            pick.selected_model_0c = 0;
            pick.value_10 = 0;
            g_gerd_659634->setPickKey(0);
            g_gerd_659634->pushPick(pick);
            Function427850(g_world->static_scene, g_world->camera,
                           &g_viewport_left_6595e8, 1);
            g_gerd_659634->popPick(pick);
            g_current_model_instance_65962c = pick.selected_model_0c;
            Function44D760(g_world);
        }
    }

    g_gerd_659634->setTextureReduction(0);
    if (g_flag_603c6d) {
        const int* overlay_viewport =
            reinterpret_cast<const int*>(g_value_659668);
        if (!g_flag_603c4c) {
            Function427850(g_scene_fullscreen_659644,
                           g_overlay_camera_659670, overlay_viewport, 0);
        }
        Function427850(g_scene_overlay0_659654,
                       g_overlay_camera_659670, 0, 0);
        Function427850(g_scene_user_659640, g_overlay_camera_659670, 0, 0);
        Function427850(g_scene_square_65965c,
                       g_square_camera_659674, overlay_viewport, 0);
        if (g_flag_603c4c) {
            Function427850(g_scene_fullscreen_659644,
                           g_overlay_camera_659670, overlay_viewport, 0);
        }
        if (g_flag_603c60) {
            Function427850(g_cursor_scene_659684,
                           g_overlay_camera_659670, 0, 0);
        }
    }
    g_gerd_659634->endFrame();

    if (g_flag_659711) {
        Function4229E0();
        g_flag_659711 = 0;
    }
    if (g_flag_6596f4) {
        now = GetTickCount();
        if (now < g_tick_65409c ||
            g_tick_65409c + g_frame_reset_interval_603c68 < now) {
            g_flag_659711 = 1;
            g_tick_65409c = now;
        }
    }

    next_page = g_index_6596e4 ^ 1;
    g_index_6596e4 = next_page;
    if (next_page == 0) g_dword_6596dc = 0;
    else g_dword_6596e0 = 0;
    g_flags_6596e8[next_page] = 0;
    ++g_dword_6596fc;
    retire_prerender = next_page ? g_scene_prerender1_659650
                                 : g_scene_prerender0_65964c;
    retire_overlay = next_page ? g_scene_overlay1_659658
                               : g_scene_overlay0_659654;
    PurgeInactiveSceneInstances(retire_prerender);
    PurgeInactiveSceneInstances(retire_overlay);

    now = GetTickCount();
    elapsed = static_cast<float>(now - g_tick_659700);
    frames_per_second =
        static_cast<float>(g_dword_6596fc) / elapsed * 1000.0f;
    if (g_dword_6596fc > 50) {
        g_tick_659700 = GetTickCount();
        g_dword_6596fc = 0;
    }
    g_seconds_per_frame_659708 = 1.0f / frames_per_second;
    g_frames_per_second_659704 = frames_per_second;

    if (g_trigger_action_active_006599c8 && GetWorld() != 0) {
        SetWorldScenePosition004511D0(GetWorld(), &saved_world_position);
    }
}
