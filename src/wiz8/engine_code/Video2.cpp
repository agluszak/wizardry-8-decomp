#include "wiz8/engine_code/Video2.h"
#include "wiz8/bringup_gates.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.h"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/regions.h"
#include "wiz8/render_state.h"
#include "wiz8/sr_api.h"
#include "wiz8/startup_world.h"
#include "wiz8/surface2d.h"
#include "wiz8/utility.h"
#include "wiz8/virtual_file_stream.h"
#include "wiz8/wiz8_windows.h"
#include "surrender/srColorSurface.h"
#include "surrender/srConfig.h"
#include "surrender/srCore.h"
#include "surrender/srFilter.h"
#include "surrender/srExtension.h"
#include "surrender/srGERD.h"
#include "surrender/srImporter.h"
#include "surrender/srMaterial.h"
#include "surrender/srMeshModel.h"
#include "surrender/srModelInstance.h"
#include "surrender/srScene.h"
#include "surrender/srStatisticsManager.h"
#include "surrender/srStringTable.h"
#include "surrender/srTexture.h"
#include "surrender/srVertexProcessor.h"
#include "DirectDraw Calls.h"
#include "Types.h"
#include "himage.h"
#include "Font.h"
#include "input.h"
#include "mousesystem.h"
#include "sgp.h"
#include "soundman.h"
#include "vobject.h"
#include "vobject_blitters.h"

#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Video2-internal helpers. Their only recovered callers are in this unit, so
   they are declared here instead of the released Video2 header. */
srNode* Function424BA0(srTextureIFace* texture, float width, float height,
                       unsigned char positional_3);
void Function4229E0(void);
void FlushDirtyTiles00425B40(void);

/*
 * The renderer window and extension loading gate InitializeStandardGamingPlatform calls after the
 * input manager. Its callees and globals are almost all unidentified, so they
 * carry address-derived names; only the SR.DLL entry points and the window
 * handle have real ones.
 */

/* The released SGP video unit owns this platform handle.  Wiz8 replaces the
   released video manager but keeps the same source-defined interface. */
// GLOBAL: WIZ8 0x006596CC
HWND ghWindow;

// GLOBAL: WIZ8 0x603c38
unsigned char g_flag_603c38 = 1;
// GLOBAL: WIZ8 0x603c4c
unsigned char g_flag_603c4c = 1;
// GLOBAL: WIZ8 0x603c60
unsigned char g_flag_603c60 = 1;
// GLOBAL: WIZ8 0x603c6d
unsigned char g_flag_603c6d = 1;
// GLOBAL: WIZ8 0x603c68
int g_frame_reset_interval_603c68 = 50;
// GLOBAL: WIZ8 0x603c39
unsigned char g_fullscreen_603c39 = 1;
// GLOBAL: WIZ8 0x603c3a
unsigned char g_flush_pending_603c3a = 1;
// GLOBAL: WIZ8 0x603c3c
int g_screen_width_603c3c = 640;
// GLOBAL: WIZ8 0x603c40
int g_screen_height_603c40 = 480;
// GLOBAL: WIZ8 0x603c44
int g_screen_depth_603c44 = 16;
// GLOBAL: WIZ8 0x603c48
int g_pixel_format_603c48 = 9;
// GLOBAL: WIZ8 0x603d74
int g_renderer_mode_603d74;
// GLOBAL: WIZ8 0x65962c
srModelInstance* g_current_model_instance_65962c;
// GLOBAL: WIZ8 0x6596fc
int g_dword_6596fc;
// GLOBAL: WIZ8 0x659700
unsigned int g_tick_659700;
// GLOBAL: WIZ8 0x6596dc
int g_dword_6596dc;
// GLOBAL: WIZ8 0x6596e0
int g_dword_6596e0;
// GLOBAL: WIZ8 0x6596e8
unsigned char g_flags_6596e8[2];
// GLOBAL: WIZ8 0x654ac4
HINSTANCE g_instance_654ac4;
// GLOBAL: WIZ8 0x659620
unsigned short g_show_command_659620;
// GLOBAL: WIZ8 0x6595f8
WNDPROC g_window_proc_6595f8;
// GLOBAL: WIZ8 0x659710
unsigned char g_flag_659710;
// GLOBAL: WIZ8 0x65970e
unsigned char g_flag_65970e;
// GLOBAL: WIZ8 0x659711
unsigned char g_flag_659711;
// GLOBAL: WIZ8 0x65970f
unsigned char g_flag_65970f;
// GLOBAL: WIZ8 0x00659634
srGERD* g_gerd_659634;
// GLOBAL: WIZ8 0x65969c
LPDIRECTDRAW g_direct_draw_65969c;
// GLOBAL: WIZ8 0x6596a0
LPDIRECTDRAW2 g_direct_draw2_6596a0;
// GLOBAL: WIZ8 0x6596a4
LPDIRECTDRAWSURFACE g_primary_surface1_6596a4;
// GLOBAL: WIZ8 0x6596a8
LPDIRECTDRAWSURFACE2 g_primary_surface_6596a8;
// GLOBAL: WIZ8 0x6596ac
LPDIRECTDRAWSURFACE g_video_primary_surface1_6596ac;
// GLOBAL: WIZ8 0x6596b0
LPDIRECTDRAWSURFACE2 g_video_primary_surface2_6596b0;
// GLOBAL: WIZ8 0x659610
RECT g_window_rect_659610;

// GLOBAL: WIZ8 0x600088
unsigned int g_color_key_600088;
// GLOBAL: WIZ8 0x65963c
srModeler* g_modeler_65963c;
// GLOBAL: WIZ8 0x659640
srScene* g_scene_user_659640;
// GLOBAL: WIZ8 0x659644
srScene* g_scene_fullscreen_659644;
// GLOBAL: WIZ8 0x659648
srScene* g_scene_permanent_659648;
// GLOBAL: WIZ8 0x65964c
srScene* g_scene_prerender0_65964c;
// GLOBAL: WIZ8 0x659650
srScene* g_scene_prerender1_659650;
// GLOBAL: WIZ8 0x659654
srScene* g_scene_overlay0_659654;
// GLOBAL: WIZ8 0x659658
srScene* g_scene_overlay1_659658;
// GLOBAL: WIZ8 0x65965c
srScene* g_scene_square_65965c;
// GLOBAL: WIZ8 0x659660
srColorSurface* g_primary_color_surface_659660;
// GLOBAL: WIZ8 0x659664
class stSurface2D* g_surface_node_659664;
// GLOBAL: WIZ8 0x659670
srCamera* g_overlay_camera_659670;
// GLOBAL: WIZ8 0x659674
srCamera* g_square_camera_659674;
// GLOBAL: WIZ8 0x659688
srColorSurface* g_mouse_surface_659688;
// GLOBAL: WIZ8 0x65967c
srMaterial* g_blit_material_65967c;
// GLOBAL: WIZ8 0x654adc
srNode* g_surface_nodes_654adc[0x12c0];
// GLOBAL: WIZ8 0x6595dc
int g_surface_state_6595dc;
// GLOBAL: WIZ8 0x654ad8
int g_surface_state_654ad8;
// GLOBAL: WIZ8 0x6595e8
int g_viewport_left_6595e8;
// GLOBAL: WIZ8 0x6595ec
int g_viewport_top_6595ec;
// GLOBAL: WIZ8 0x6595f0
int g_viewport_right_6595f0;
// GLOBAL: WIZ8 0x6595f4
int g_viewport_bottom_6595f4;
// GLOBAL: WIZ8 0x00659AB4
W8World* g_world;
// GLOBAL: WIZ8 0x00659AB8
W8World* g_world_659ab8;
// GLOBAL: WIZ8 0x652da4
unsigned char g_flag_652da4;
// GLOBAL: WIZ8 0x5ebb1c
extern const float g_scale_x_5ebb1c = 1.0f / 640.0f;
// GLOBAL: WIZ8 0x5ebb20
extern const float g_scale_y_5ebb20 = 1.0f / 480.0f;

// GLOBAL: WIZ8 0x652ddc
unsigned char g_block_652ddc[0x12c0];
// GLOBAL: WIZ8 0x006596e4
unsigned int g_index_6596e4;
// GLOBAL: WIZ8 0x6596d8
int g_dword_6596d8;
// GLOBAL: WIZ8 0x006596ec
int g_dword_6596ec;
// GLOBAL: WIZ8 0x006596f0
int g_dword_6596f0;
// GLOBAL: WIZ8 0x659668
const int* g_value_659668;
// GLOBAL: WIZ8 0x65409c
unsigned int g_tick_65409c;
// GLOBAL: WIZ8 0x659704
float g_frames_per_second_659704;
// GLOBAL: WIZ8 0x659708
float g_seconds_per_frame_659708;

// GLOBAL: WIZ8 0x65a118
unsigned char* g_render_options_65a118;

// GLOBAL: WIZ8 0x659684
srScene* g_cursor_scene_659684;
// GLOBAL: WIZ8 0x65968c
srMeshModel* g_cursor_model_65968c;
// GLOBAL: WIZ8 0x659690
srTexture* g_cursor_texture_659690;
// GLOBAL: WIZ8 0x659694
srModelInstance* g_cursor_node_659694;
// GLOBAL: WIZ8 0x659698
unsigned int g_cursor_move_tick_659698;
// GLOBAL: WIZ8 0x654ad0
int g_cursor_width_654ad0;
// GLOBAL: WIZ8 0x654ad4
int g_cursor_height_654ad4;
// GLOBAL: WIZ8 0x6596b4
int g_cursor_image_width_6596b4;
// GLOBAL: WIZ8 0x6596b8
int g_cursor_image_height_6596b8;
// GLOBAL: WIZ8 0x6596bc
int g_cursor_hotspot_x_6596bc;
// GLOBAL: WIZ8 0x6596c0
int g_cursor_hotspot_y_6596c0;
// GLOBAL: WIZ8 0x6596c4
unsigned char g_system_cursor_visible_6596c4;

// FUNCTION: WIZ8 0x00428ab0
void AssertFailureHandler(const char* expression, const char* file, long line, const char* message)
{
    char text[2048];
    strcpy(text, "ERROR: You are viewing a message intended for the developers of "
                 "Wizardry 8. Please report the following information to technical "
                 "support. We apologize for this inconvenience.\n\n");
    if (message != 0 && *message != '\0') {
        _snprintf(text + strlen(text), 0x6d3,
                  "Debug assertion in module %s line %d failed:\n\n"
                  "Expression [ %s ] evaluates to false.\n\n%s\n",
                  file, line, expression, message);
    } else {
        _snprintf(text + strlen(text), 0x6d3,
                  "Debug assertion in module %s line %d failed:\n\n"
                  "Expression [ %s ] evaluates to false.\n",
                  file, line, expression);
    }
    g_pending_screen_state.id = -1;
    ShutdownWithErrorBox(text);
}

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
    InvalidateRegion(0, 0, 640, 480, 0);
}

/* Brings the renderer up, shows the window and, when the INSPECTOR switch was
   given, loads that extension from the DLL subdirectory before returning to the
   original working directory. Each gate that fails returns straight out with
   the callee's own false still in AL. */
// FUNCTION: WIZ8 0x00421bb0
unsigned char InitializeVideoManager(HINSTANCE instance, unsigned short show_command,
                                     void* window_proc)
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
        if (g_flag_006840bc) {
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
            InvalidateRegion(0, 0, 0x280, 0x1e0, 0);
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
    if (!InitializeStartupNavigation0044F060()) {
        return 0;
    }
    EnableAllRenderOptions();
    return InitializeMouseCursorScene();
}

// FUNCTION: WIZ8 0x00421dc0
void ShutdownVideoManager(void)
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
        gusAlphaMask = 0;
        gusRedMask = 0xf800;
        gusGreenMask = 0x07e0;
        g_color_key_600088 = 0x7bef;
    } else if (g_pixel_format_603c48 == 8) {
        gusAlphaMask = 0;
        gusRedMask = 0x7c00;
        gusGreenMask = 0x03e0;
        g_color_key_600088 = 0x3def;
    } else if (g_pixel_format_603c48 == 9) {
        gusAlphaMask = 0x8000;
        gusRedMask = 0x7c00;
        gusGreenMask = 0x03e0;
        g_color_key_600088 = 0x3def;
    } else {
        return;
    }

    gusBlueMask = 0x001f;
    gusRedShift = 8;
    for (bit = 0x8000; (gusRedMask & bit) == 0; bit >>= 1) {
        --gusRedShift;
    }
    gusGreenShift = 8;
    for (bit = 0x8000; (gusGreenMask & bit) == 0; bit >>= 1) {
        --gusGreenShift;
    }
    gusBlueShift = 8;
    for (bit = 0x8000; (0x001f & bit) == 0; bit >>= 1) {
        --gusBlueShift;
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
        ghWindow = CreateWindowExA(0, "Wizardry 8", "Wizardry 8", style, g_window_rect_659610.left,
                                   g_window_rect_659610.top,
                                   g_window_rect_659610.right - g_window_rect_659610.left,
                                   g_window_rect_659610.bottom - g_window_rect_659610.top, NULL,
                                   NULL, g_instance_654ac4, NULL);
    } else {
        style = WS_POPUP | WS_VISIBLE;
        ghWindow = CreateWindowExA(0, "Wizardry 8", "Wizardry 8", style, 0, 0,
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
    result = g_direct_draw_65969c->QueryInterface(IID_IDirectDraw2, (void**)&g_direct_draw2_6596a0);
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
    description.ddpfPixelFormat.dwRBitMask = gusRedMask;
    description.ddpfPixelFormat.dwGBitMask = gusGreenMask;
    description.ddpfPixelFormat.dwBBitMask = gusBlueMask;

    result = g_direct_draw2_6596a0->CreateSurface(&description, &g_primary_surface1_6596a4, NULL);
    if (FAILED(result)) {
        return 0;
    }
    result = g_primary_surface1_6596a4->QueryInterface(IID_IDirectDrawSurface2,
                                                       (void**)&g_primary_surface_6596a8);
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
    srConfig.set("DD_DIRECTX7", "DisablePrimaryHEL=1 DisableAttachedSecondaryDevices=1 "
                                "DisableDetachedSecondaryDevices=1 DisableNonDisplayDevices=1");
    srConfig.set("DD_DIRECTX6", "DisablePrimaryHEL=1 DisableAttachedSecondaryDevices=1 "
                                "DisableDetachedSecondaryDevices=1 DisableNonDisplayDevices=1");
    sprintf(driver_name, "srDD_%s", device);
    devices.addString(driver_name);
    g_gerd_659634 = srGERD::loadDevice(devices, 0);
    _chdir("..");
    if (!g_gerd_659634) {
        ShutdownWithErrorBox("Video device cannot be started. Please re-run 3DSetup.");
        return 0;
    }

    g_gerd_659634->createContext((unsigned long)ghWindow);
    Function422800();
    srAssertSetFunc(AssertFailureHandler);
    if (_strnicmp(sound_provider, "none", 4) != 0) {
        Sound3DSetProvider(sound_provider);
    }
    InitializeVirtualFileImageImporters();
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
        SetWindowPos(ghWindow, NULL, g_window_rect_659610.left, g_window_rect_659610.top,
                     g_window_rect_659610.right - g_window_rect_659610.left,
                     g_window_rect_659610.bottom - g_window_rect_659610.top, 0);
        error = g_gerd_659634->openWindow();
    } else {
        SetWindowLongA(ghWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(ghWindow, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), 0);
        mode = g_gerd_659634->getDisplayMode(g_screen_width_603c3c, g_screen_height_603c40,
                                             g_screen_depth_603c44);
        if (mode == -1) {
            ShutdownWithErrorBox("Video device does not support video resolution.");
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

// FUNCTION: WIZ8 0x00423390
IDirectDrawSurface2* BeginVideoPresentation(void)
{
    DDSURFACEDESC description;

    if (g_gerd_659634 != 0) {
        g_flush_pending_603c3a = 0;
        g_gerd_659634->closeWindow((srGERD::e_closeHint)1);
        g_gerd_659634->deleteContext();
    }
    if (g_direct_draw2_6596a0->SetCooperativeLevel(ghWindow, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) !=
        DD_OK) {
        NoOp();
        return 0;
    }
    if (g_direct_draw2_6596a0->SetDisplayMode(640, 480, 16, 0, 0) != DD_OK) {
        NoOp();
        return 0;
    }
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS;
    description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (g_direct_draw2_6596a0->CreateSurface(&description, &g_video_primary_surface1_6596ac, 0) !=
        DD_OK) {
        NoOp();
        return 0;
    }
    if (g_video_primary_surface1_6596ac->QueryInterface(
            IID_IDirectDrawSurface2, (void**)&g_video_primary_surface2_6596b0) != DD_OK) {
        NoOp();
        return 0;
    }
    return g_video_primary_surface2_6596b0;
}

// FUNCTION: WIZ8 0x004234A0
unsigned char FinishVideoPresentation(void)
{
    if (g_video_primary_surface1_6596ac != 0) {
        g_video_primary_surface1_6596ac->Release();
        g_video_primary_surface1_6596ac = 0;
    }
    if (g_video_primary_surface2_6596b0 != 0) {
        g_video_primary_surface2_6596b0->Release();
        g_video_primary_surface2_6596b0 = 0;
    }
    g_direct_draw2_6596a0->SetCooperativeLevel(ghWindow, DDSCL_NORMAL);
    g_gerd_659634->createContext((unsigned long)ghWindow);
    return Function422800();
}

/* WM_SIZE only rebuilds the SurRender output in windowed mode.  Full-screen
   startup receives the same Windows notification while the device already
   owns its configured 640x480 mode, so there is no resize operation to do. */
// FUNCTION: WIZ8 0x00422550
unsigned char VideoResizeWindow(void)
{
    if (g_fullscreen_603c39 || !ghWindow || !g_gerd_659634 || !g_flush_pending_603c3a) {
        return 0;
    }
    g_flush_pending_603c3a = 0;
    g_gerd_659634->closeWindow(static_cast<srGERD::e_closeHint>(1));
    if (g_gerd_659634->openWindow() == static_cast<srGERD::e_error>(3)) {
        return 0;
    }
    g_dword_6596f0 = 2;
    g_dword_6596ec = 2;
    ResetTransientRenderScenes();
    g_flush_pending_603c3a = 1;
    return 1;
}

// FUNCTION: WIZ8 0x00422970
void VideoFullScreen(unsigned char enabled)
{
    g_fullscreen_603c39 = enabled;
    if (ghWindow && g_gerd_659634 && g_flush_pending_603c3a) {
        g_flush_pending_603c3a = 0;
        g_gerd_659634->closeWindow(static_cast<srGERD::e_closeHint>(1));
        Function422800();
    }
}

// FUNCTION: WIZ8 0x00422f10
void ResetTransientRenderScenes(void)
{
    memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
    unsigned int active = g_index_6596e4;
    g_flags_6596e8[active ^ 1] = 0;
    g_flags_6596e8[active] = 0;
    g_dword_6596d8 = 0;
    PurgeInactiveSceneInstances(g_scene_prerender0_65964c);
    PurgeInactiveSceneInstances(g_scene_overlay0_659654);
    PurgeInactiveSceneInstances(g_scene_prerender1_659650);
    PurgeInactiveSceneInstances(g_scene_overlay1_659658);
    InvalidateRegion(0, 0, 640, 480, 0);
}

// FUNCTION: WIZ8 0x004277e0
unsigned char VideoInspectorIsEnabled(void)
{
    return g_flag_65970f;
}

// FUNCTION: WIZ8 0x00422050
void SuspendVideoManager(void)
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
        FreeMouseCursor();
    }
}

// FUNCTION: WIZ8 0x004220b0
unsigned char RestoreVideoManager(void)
{
    if (g_flag_659710) {
        return 1;
    }
    if (g_flag_006840bc) {
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
            InvalidateRegion(0, 0, 0x280, 0x1e0, 0);
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
void GetCurrentVideoSettings(unsigned short* height, unsigned short* width, unsigned char* depth)
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

void UpdateRenderElapsedTime00482140(void);

/* Saturate the three components of a renderer colour in place and return it.
   The reviewed body performs these three scalar saturations in order. */
// FUNCTION: WIZ8 0x004299b0
srVector3T<float>* __fastcall SaturateColor004299B0(srVector3T<float>* color)
{
    if (color->x <= 0.0f)
        color->x = 0.0f;
    else if (color->x >= 1.0f)
        color->x = 1.0f;
    if (color->y <= 0.0f)
        color->y = 0.0f;
    else if (color->y >= 1.0f)
        color->y = 1.0f;
    if (color->z <= 0.0f)
        color->z = 0.0f;
    else if (color->z >= 1.0f)
        color->z = 1.0f;
    return color;
}

/* Render one scene into either the full output or a logical 640x480 viewport.
   Scene fog follows the current world's static scene for ordinary overlays;
   the world passes retain their own fog by selecting preserve_fog. */
// FUNCTION: WIZ8 0x00427850
void Function427850(srScene* scene, srCamera* camera, const int* viewport, char preserve_fog)
{
    unsigned long width = g_gerd_659634->getWidth();
    unsigned long height = g_gerd_659634->getHeight();

    if (scene == 0 || scene->getChildCount() == 0) {
        return;
    }
    if (viewport != 0) {
        unsigned long left = viewport[0] * width / 640;
        unsigned long top = viewport[1] * height / 480;
        unsigned long viewport_width = (viewport[2] - viewport[0]) * width / 640;
        unsigned long viewport_height = (viewport[3] - viewport[1]) * height / 480;
        g_gerd_659634->setViewPort(left, top, viewport_width, viewport_height);
        g_gerd_659634->setScissor(left, top, viewport_width, viewport_height);
    }
    if (!preserve_fog) {
        srVector3T<float> fog;
        if (g_world == 0) {
            fog.SetZero();
        } else {
            g_world->static_scene->getFogColor(fog);
            SaturateColor004299B0(&fog);
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
void RenderFrame(void)
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

    clear_color.SetZero();
    SaturateColor004299B0(&clear_color);
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
            SaturateColor004299B0(&clear_color);
        }
    }

    g_gerd_659634->setClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
    SyncSystemCursor();
    RenderFastHelp();
    UpdateRegionHelp();
    FlushDirtyTiles00425B40();
    if (g_gerd_659634 != 0) {
        g_gerd_659634->resetStatistics();
    }
    srCore.getStatisticsManager()->reset();
    g_gerd_659634->beginFrame();
    UpdateRenderElapsedTime00482140();

    if (!g_monster_shadow_updates_enabled_0065970c) {
        if (g_flag_65970e) {
            goto clear_viewport;
        }
    } else if (g_flag_65970e || !g_render_flag_603c6c || g_world_659ab8 == 0) {
    clear_viewport: {
        unsigned long height = g_gerd_659634->getHeight();
        unsigned long width = g_gerd_659634->getWidth();
        g_gerd_659634->setScissor(
            g_viewport_left_6595e8 * width / 640, g_viewport_top_6595ec * height / 480,
            (g_viewport_right_6595f0 - g_viewport_left_6595e8) * width / 640,
            (g_viewport_bottom_6595f4 - g_viewport_top_6595ec) * height / 480);
        g_gerd_659634->clear(srFlags<srGERD::e_buffer>(3));
        g_gerd_659634->setScissor(0, 0, width, height);
    }
    }

    first_page = g_index_6596e4 ? g_scene_prerender0_65964c : g_scene_prerender1_659650;
    second_page = g_index_6596e4 ? g_scene_prerender1_659650 : g_scene_prerender0_65964c;
    Function427850(first_page, g_overlay_camera_659670, 0, 0);
    Function427850(second_page, g_overlay_camera_659670, 0, 0);
    g_gerd_659634->setTextureReduction(g_resident_texture_policy_659714);

    if (g_render_flag_603c6c && g_world_659ab8 != 0 && g_monster_shadow_updates_enabled_0065970c) {
        Function427850(g_world_659ab8->static_scene, g_world_659ab8->camera,
                       &g_viewport_left_6595e8, 0);
    }
    if (g_world != 0 && g_flag_65970d) {
        g_gerd_659634->setTextureReduction(g_resident_texture_policy_659714);
        if (!g_flag_603c38 ||
            g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0 < g_viewport_left_6595e8 ||
            g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4 < g_viewport_top_6595ec ||
            g_viewport_right_6595f0 < g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0 ||
            g_viewport_bottom_6595f4 < g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4) {
            Function427850(g_world->static_scene, g_world->camera, &g_viewport_left_6595e8, 1);
        } else {
            int half_width = (g_viewport_right_6595f0 - g_viewport_left_6595e8) / 2;
            int half_height = (g_viewport_bottom_6595f4 - g_viewport_top_6595ec) / 2;
            srGERD::Pick pick;
            pick.value_00 = static_cast<float>((g_cursor_hotspot_x_6596bc - half_width -
                                                g_viewport_left_6595e8 + g_cursor_width_654ad0)) /
                            static_cast<float>(half_width);
            pick.value_04 = -static_cast<float>(g_cursor_hotspot_y_6596c0 - half_height -
                                                g_viewport_top_6595ec + g_cursor_height_654ad4) /
                            static_cast<float>(half_height);
            pick.value_08 = 1.0f;
            pick.selected_model_0c = 0;
            pick.value_10 = 0;
            g_gerd_659634->setPickKey(0);
            g_gerd_659634->pushPick(pick);
            Function427850(g_world->static_scene, g_world->camera, &g_viewport_left_6595e8, 1);
            g_gerd_659634->popPick(pick);
            g_current_model_instance_65962c = pick.selected_model_0c;
            Function44D760(g_world);
        }
    }

    g_gerd_659634->setTextureReduction(0);
    if (g_flag_603c6d) {
        const int* overlay_viewport = g_value_659668;
        if (!g_flag_603c4c) {
            Function427850(g_scene_fullscreen_659644, g_overlay_camera_659670, overlay_viewport, 0);
        }
        Function427850(g_scene_overlay0_659654, g_overlay_camera_659670, 0, 0);
        Function427850(g_scene_user_659640, g_overlay_camera_659670, 0, 0);
        Function427850(g_scene_square_65965c, g_square_camera_659674, overlay_viewport, 0);
        if (g_flag_603c4c) {
            Function427850(g_scene_fullscreen_659644, g_overlay_camera_659670, overlay_viewport, 0);
        }
        if (g_flag_603c60) {
            Function427850(g_cursor_scene_659684, g_overlay_camera_659670, 0, 0);
        }
    }
    g_gerd_659634->endFrame();

    if (g_flag_659711) {
        Function4229E0();
        g_flag_659711 = 0;
    }
    if (g_flag_6596f4) {
        now = GetTickCount();
        if (now < g_tick_65409c || g_tick_65409c + g_frame_reset_interval_603c68 < now) {
            g_flag_659711 = 1;
            g_tick_65409c = now;
        }
    }

    next_page = g_index_6596e4 ^ 1;
    g_index_6596e4 = next_page;
    if (next_page == 0)
        g_dword_6596dc = 0;
    else
        g_dword_6596e0 = 0;
    g_flags_6596e8[next_page] = 0;
    ++g_dword_6596fc;
    retire_prerender = next_page ? g_scene_prerender1_659650 : g_scene_prerender0_65964c;
    retire_overlay = next_page ? g_scene_overlay1_659658 : g_scene_overlay0_659654;
    PurgeInactiveSceneInstances(retire_prerender);
    PurgeInactiveSceneInstances(retire_overlay);

    now = GetTickCount();
    elapsed = static_cast<float>(now - g_tick_659700);
    frames_per_second = static_cast<float>(g_dword_6596fc) / elapsed * 1000.0f;
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
// FUNCTION: WIZ8 0x00427440
void Function427440(void)
{
    if (g_gerd_659634 != 0) {
        g_gerd_659634->invalidateTextureCache();
    }
}

/* The light direction travels as three raw 32-bit words that the renderer and
   both static scenes consume as the fog colour. The renderer is flushed before
   the new colour lands. */
// FUNCTION: WIZ8 0x00427380
void PublishLightDirection(const EnvironmentColour* direction)
{
    const srVector3T<float>* color = reinterpret_cast<const srVector3T<float>*>(
        direction); // reinterpret-ok: the renderer consumes the light triple as a fog vector
    if (g_gerd_659634 != 0) {
        g_gerd_659634->flush();
        g_gerd_659634->setFogColor(*color);
    }
    if (g_world != 0) {
        g_world->static_scene->setFogColor(*color);
    }
    if (g_world_659ab8 != 0) {
        g_world_659ab8->static_scene->setFogColor(*color);
    }
}

// FUNCTION: WIZ8 0x00427230
void SetRendererOption4Enabled(char enabled)
{
    if (g_gerd_659634 != 0) {
        if ((!enabled && g_gerd_659634->isEnabled(static_cast<srGERD::e_enable>(4))) ||
            (enabled && !g_gerd_659634->isEnabled(static_cast<srGERD::e_enable>(4)))) {
            g_gerd_659634->toggle(static_cast<srGERD::e_enable>(4));
        }
    }
}

// FUNCTION: WIZ8 0x00428e20
int Function428E20(void)
{
    MEMORYSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPageFile - status.dwAvailPageFile;
}

// FUNCTION: WIZ8 0x00427260
unsigned char Function427260(void)
{
    srColorSurfaceIFace* surface = g_gerd_659634->lockBuffer();
    if (surface != 0) {
        g_gerd_659634->unlockBuffer();
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00427830
void Function427830(char enabled)
{
    g_flag_603c38 = enabled;
    if (enabled == 0) {
        g_current_model_instance_65962c = 0;
    }
}

/* Mouse cursor scene and rendering. */
namespace {

unsigned long float_bits(float value)
{
    union {
        float floating;
        unsigned long bits;
    } representation;
    representation.floating = value;
    return representation.bits;
}

} // namespace

// FUNCTION: WIZ8 0x00424EB0
static srModelInstance* MakePolygonBrush(srNode* parent, srColorSurfaceIFace* surface, double width,
                                         double height, float mapping_x, float mapping_y,
                                         float mapping_width, float mapping_height,
                                         unsigned char overlay)
{
    srMeshModel* model;
    srTextureMap* texture;
    stModelInstance2D* instance;
    srModeler::MappingInfo mapping;
    srVector3T<float> scale;
    srShader shader;

    model = SR_NEW(srMeshModel)(0L, 0L);
    if (!model) {
        return 0;
    }
    model->autoRelease();
    model->setName("Video2DMakePolygonBrush");

    g_modeler_65963c->createGrid(1, 1);
    mapping.unknown_00 = 0;
    mapping.unknown_04 = 1;
    mapping.unknown_08 = float_bits(mapping_width);
    mapping.unknown_0c = float_bits(mapping_height);
    mapping.unknown_10 = float_bits(mapping_x);
    mapping.unknown_14 = float_bits(mapping_y);
    g_modeler_65963c->planarMap(0, 0, mapping);
    scale.x = static_cast<float>(width);
    scale.y = static_cast<float>(height);
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    shader.value = overlay ? g_surface_state_654ad8 : g_surface_state_6595dc;
    if (!surface) {
        shader.value &= 0xffff7fff;
    } else {
        texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
        texture->autoRelease();
        texture->setName("Video2DMakePolygonBrush");
        texture->setSurfacePtr(surface);
        texture->setCorrection(static_cast<srTextureIFace::e_correction>(0));
        texture->setMagFilter(static_cast<srTextureIFace::e_filter>(3));
        texture->setMinFilter(static_cast<srTextureIFace::e_filter>(3));
        texture->setMipmap(static_cast<srTextureIFace::e_mipmap>(0));
        texture->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
        texture->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
        model->setMaterial(g_blit_material_65967c, 0, static_cast<srMeshModel::e_side>(0));
        model->setTexture(texture, 0, 0);
        texture->enableHint(static_cast<srTextureIFace::e_hint>(overlay ? 2 : 1));
        texture->enableHint(static_cast<srTextureIFace::e_hint>(3));
    }
    model->setShader(shader, 0);

    instance = new stModelInstance2D(parent);
    instance->setName("Video2DMakePolygonBrush");
    instance->SetModel0047F3A0(model);
    instance->configure2D(static_cast<short>(width * 640.0), static_cast<short>(height * 480.0));
    return instance;
}

static BOOLEAN BlitVideoObjectToColorSurface(UINT32 video_object, UINT16 region,
                                             srColorSurface* destination, UINT16 x, UINT16 y)
{
    HVOBJECT object;
    ETRLEObject properties;

    if (!GetVideoObject(&object, video_object)) {
        return FALSE;
    }
    if (!GetVideoObjectETRLEProperties(object, &properties, region)) {
        return FALSE;
    }
    return BltVideoObjectToBuffer(static_cast<UINT16*>(destination->getDataPtr()),
                                  destination->getPitch(), object, region, x, y,
                                  VO_BLT_SRCTRANSPARENCY, 0);
}

static void MoveSystemCursor(int x, int y)
{
    RECT client;
    POINT top_left;
    POINT bottom_right;

    if (!g_fullscreen_603c39) {
        GetClientRect(ghWindow, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(ghWindow, &top_left);
        ClientToScreen(ghWindow, &bottom_right);
        x += top_left.x;
        y += top_left.y;
    }
    SetCursorPos(x, y);
}

// FUNCTION: WIZ8 0x00427c90
BOOLEAN ResizeMouseCursorSurface(int width, int height)
{
    int extent;
    float mapping_scale;

    if (g_cursor_image_width_6596b4 == width && g_cursor_image_height_6596b8 == height) {
        return TRUE;
    }
    extent = width > height ? width : height;
    if (extent <= 16)
        extent = 16;
    else if (extent <= 32)
        extent = 32;
    else if (extent <= 64)
        extent = 64;
    else if (extent <= 128)
        extent = 128;
    else if (extent <= 256)
        extent = 256;
    else
        extent = -1;
    if (!g_mouse_surface_659688->resize(extent, extent)) {
        return FALSE;
    }
    if (g_cursor_node_659694) {
        g_cursor_node_659694->release();
    }
    if (g_cursor_texture_659690) {
        g_cursor_texture_659690->release();
    }
    mapping_scale = g_surface_scale_659680 / extent;
    g_cursor_node_659694 = MakePolygonBrush(
        g_cursor_scene_659684, g_mouse_surface_659688, static_cast<double>(extent) / 640.0,
        static_cast<double>(extent) / 480.0, mapping_scale, mapping_scale, 1.0f, 1.0f, 1);
    g_cursor_node_659694->setName("MouseResize");
    PositionMouseCursor(g_cursor_width_654ad0, g_cursor_height_654ad4, 0);
    g_cursor_model_65968c = static_cast<srMeshModel*>(g_cursor_node_659694->model());
    g_cursor_model_65968c->enableStartupControls();
    static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(0xc7c35000);
    g_cursor_model_65968c->enableStartupControls();
    g_cursor_model_65968c->setName("Mouse Cursor Mesh");
    g_cursor_texture_659690 = static_cast<srTexture*>(g_cursor_model_65968c->getTexture(0, 0));
    g_cursor_texture_659690->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
    g_cursor_texture_659690->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
    g_cursor_texture_659690->addReference();
    return TRUE;
}

/* Move the OS/system cursor when the hotspot changes, then resync the rendered
   cursor. Unlike SetMouseCursorFromVideoObject this path refreshes through
   SyncSystemCursor rather than PositionMouseCursor. */
// FUNCTION: WIZ8 0x00427f00
void SetMouseCursorHotspot(short hotspot_x, short hotspot_y)
{
    int x;
    int y;

    if (g_cursor_hotspot_x_6596bc != hotspot_x || g_cursor_hotspot_y_6596c0 != hotspot_y) {
        x = g_cursor_width_654ad0 - hotspot_x + g_cursor_hotspot_x_6596bc;
        y = g_cursor_height_654ad4 - hotspot_y + g_cursor_hotspot_y_6596c0;
        MoveSystemCursor(x, y);
        SyncSystemCursor();
        g_cursor_hotspot_x_6596bc = hotspot_x;
        g_cursor_hotspot_y_6596c0 = hotspot_y;
    }
}

// FUNCTION: WIZ8 0x00427ab0
BOOLEAN SetMouseCursorFromVideoObject(UINT32 video_object, UINT16 region, INT16 offset_x,
                                      INT16 offset_y)
{
    ETRLEObject properties;
    int x;
    int y;

    if (!gfVideoObjectsInit) {
        return FALSE;
    }
    if (!GetVideoObjectETRLEPropertiesFromIndex(video_object, &properties, region)) {
        return FALSE;
    }
    if (!ResizeMouseCursorSurface(properties.usWidth + 1, properties.usHeight + 1)) {
        return FALSE;
    }
    if (g_cursor_hotspot_x_6596bc != offset_x || g_cursor_hotspot_y_6596c0 != offset_y) {
        x = g_cursor_width_654ad0 - offset_x + g_cursor_hotspot_x_6596bc;
        y = g_cursor_height_654ad4 - offset_y + g_cursor_hotspot_y_6596c0;
        MoveSystemCursor(x, y);
        PositionMouseCursor(x, y, 1);
        g_cursor_hotspot_x_6596bc = offset_x;
        g_cursor_hotspot_y_6596c0 = offset_y;
    }
    g_cursor_image_width_6596b4 = properties.usWidth;
    g_cursor_image_height_6596b8 = properties.usHeight;
    g_mouse_surface_659688->fill(0);
    return BlitVideoObjectToColorSurface(video_object, region, g_mouse_surface_659688, 0, 0);
}

// FUNCTION: WIZ8 0x00427fc0
void BlitToMouseCursor(UINT32 video_object, UINT16 region, UINT16 x, UINT16 y)
{
    BlitVideoObjectToColorSurface(video_object, region, g_mouse_surface_659688, x, y);
}

// FUNCTION: WIZ8 0x00427ff0
void RefreshMouseCursorTexture(void)
{
    g_mouse_surface_659688->touch();
    g_cursor_texture_659690->invalidate();
}

/* Whether the tracked cursor position lies inside the render viewport. The
   two automap callers use it, but nothing names the viewport as automap-only
   state; placed here with the neighbouring cursor bodies. */
// FUNCTION: WIZ8 0x00428070
bool IsCursorInsideViewport(void)
{
    int x = g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0;
    int y = g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4;
    return x >= g_viewport_left_6595e8 && y >= g_viewport_top_6595ec &&
           x <= g_viewport_right_6595f0 && y <= g_viewport_bottom_6595f4;
}

// FUNCTION: WIZ8 0x00428140
void PositionMouseCursor(int width, int height, unsigned char reset_tick)
{
    srVector3T<double> location;

    if (g_mouse_surface_659688) {
        g_cursor_width_654ad0 = width < 641 ? width : 640;
        g_cursor_height_654ad4 = height < 481 ? height : 480;
        if (g_cursor_node_659694) {
            location.x = static_cast<double>(g_cursor_width_654ad0) / 640.0 +
                         static_cast<double>(g_mouse_surface_659688->getWidth()) / 1280.0;
            location.y = 1.0 - static_cast<double>(g_cursor_height_654ad4) / 480.0 -
                         static_cast<double>(g_mouse_surface_659688->getHeight()) / 960.0;
            location.z = 0.0;
            g_cursor_node_659694->setLocation(location);
            if (reset_tick) {
                g_cursor_move_tick_659698 = GetTickCount();
            }
        }
    }
}

/* The tracked cursor as viewport-relative 0..1 coordinates, or zero when it
   lies outside. Same tracked position and viewport as the query above. */
// FUNCTION: WIZ8 0x00428230
unsigned char GetCursorPositionInViewport(srVector3T<float>* position)
{
    int x = g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0;
    int y = g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4;
    if (x >= g_viewport_left_6595e8 && y >= g_viewport_top_6595ec && x <= g_viewport_right_6595f0 &&
        y <= g_viewport_bottom_6595f4) {
        position->x = static_cast<float>(x - g_viewport_left_6595e8) /
                      static_cast<float>(g_viewport_right_6595f0 - g_viewport_left_6595e8);
        position->y = static_cast<float>(y - g_viewport_top_6595ec) /
                      static_cast<float>(g_viewport_bottom_6595f4 - g_viewport_top_6595ec);
        position->z = 0.0f;
        return 1;
    }
    return 0;
}

/* Keep the rendered cursor synchronized with the OS cursor. In windowed mode
   the OS cursor is visible outside the client area and hidden while the game
   owns it; fullscreen coordinates are clamped to the 640x480 game surface. */
// FUNCTION: WIZ8 0x00428340
void SyncSystemCursor(void)
{
    POINT cursor;
    RECT client;
    POINT top_left;
    POINT bottom_right;

    GetCursorPos(&cursor);
    if (!g_fullscreen_603c39) {
        GetClientRect(ghWindow, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(ghWindow, &top_left);
        ClientToScreen(ghWindow, &bottom_right);
        if (cursor.x < top_left.x || cursor.x >= bottom_right.x || cursor.y < top_left.y ||
            cursor.y >= bottom_right.y) {
            if (g_system_cursor_visible_6596c4 != 1) {
                g_system_cursor_visible_6596c4 = 1;
                ShowCursor(TRUE);
            }
            return;
        }
        cursor.x -= top_left.x;
        cursor.y -= top_left.y;
        if (cursor.x != g_cursor_width_654ad0 || cursor.y != g_cursor_height_654ad4) {
            PositionMouseCursor(cursor.x, cursor.y, 1);
        }
        if (g_system_cursor_visible_6596c4 != 0) {
            g_system_cursor_visible_6596c4 = 0;
            ShowCursor(FALSE);
        }
        return;
    }

    if (cursor.x < 1)
        cursor.x = 0;
    else if (cursor.x >= 640)
        cursor.x = 640;
    if (cursor.y < 1)
        cursor.y = 0;
    else if (cursor.y >= 480)
        cursor.y = 480;
    if (cursor.x != g_cursor_width_654ad0 || cursor.y != g_cursor_height_654ad4) {
        PositionMouseCursor(cursor.x, cursor.y, 1);
        if (!g_fullscreen_603c39) {
            GetClientRect(ghWindow, &client);
            top_left.x = client.left;
            top_left.y = client.top;
            bottom_right.x = client.right;
            bottom_right.y = client.bottom;
            ClientToScreen(ghWindow, &top_left);
            ClientToScreen(ghWindow, &bottom_right);
            SetCursorPos(top_left.x + cursor.x, top_left.y + cursor.y);
        } else {
            SetCursorPos(cursor.x, cursor.y);
        }
    }
}

/* Fill a caller-supplied point with the sum of the two global pairs, after the
   refresh that recomputes them. Fifty callers reach this; what the two pairs
   mean is not established beyond the sum, so both keep address-qualified names. */
// FUNCTION: WIZ8 0x004284f0
void SGPMouseGetPos(POINT* point)
{
    if (point != 0) {
        SyncSystemCursor();
        point->x = g_cursor_hotspot_x_6596bc + g_cursor_width_654ad0;
        point->y = g_cursor_hotspot_y_6596c0 + g_cursor_height_654ad4;
    }
}

/* Creates the shipped 128x128 mouse polygon inside its dedicated scene. */
// FUNCTION: WIZ8 0x004285c0
unsigned char InitializeMouseCursorScene(void)
{
    srScene* cursor_scene = SR_NEW(srScene)(static_cast<srNode*>(0));
    cursor_scene->setAmbientLight(0.0f, 0.0f, 0.0f);
    cursor_scene->setFogColor(0.0f, 0.0f, 0.0f);
    g_cursor_scene_659684 = cursor_scene;
    g_cursor_scene_659684->setName("Mouse Cursor Scene");
    if (!g_mouse_surface_659688) {
        return 0;
    }
    g_mouse_surface_659688->fill(0);
    if (g_cursor_texture_659690) {
        g_cursor_texture_659690->release();
    }
    g_cursor_node_659694 = MakePolygonBrush(g_cursor_scene_659684, g_mouse_surface_659688, 0.2,
                                            0.26666666666666666, g_surface_scale_659680 / 128.0f,
                                            g_surface_scale_659680 / 128.0f, 1.0f, 1.0f, 1);
    if (g_cursor_node_659694) {
        g_cursor_node_659694->setName("MouseInit");
        g_cursor_model_65968c = static_cast<srMeshModel*>(g_cursor_node_659694->model());
        g_cursor_model_65968c->enableStartupControls();
        static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(0xc7c35000);
        g_cursor_texture_659690 = static_cast<srTexture*>(g_cursor_model_65968c->getTexture(0, 0));
        g_cursor_texture_659690->setWrapS(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->setWrapT(static_cast<srTextureIFace::e_wrap>(1));
        g_cursor_texture_659690->addReference();
        PositionMouseCursor(640, 480, 1);
    }
    return 1;
}

/* Dirty-rectangle tracking and surface updates. */
/*
 * Marks a rectangle dirty on the current page.
 *
 * The screen is tracked as an 80x60 grid of eight-pixel cells - 0x50 cells per
 * row, which is 640/8 - and this walks the cells a rectangle covers, handing
 * each to InvalidateDirtyTile004259B0. A page already marked whole is skipped outright, and a
 * rectangle that turns out to cover the whole 640x480 marks it whole.
 *
 * InvalidateDirtyTile004259B0 performs the per-cell write; the flag bits the caller passes
 * are only known by which bits they set.
 */

// GLOBAL: WIZ8 0x65970d
unsigned char g_flag_65970d;
// GLOBAL: WIZ8 0x6596ea
unsigned char g_flag_6596ea;
/* The initial full-screen invalidation runs before any 2D node occupies the
   tile table. A cell occupied by a 2D instance releases that instance and
   recursively invalidates the cells its extent covers; the flags the caller
   passes only mark this cell. */
// FUNCTION: WIZ8 0x004259b0
static void InvalidateDirtyTile004259B0(int cell, unsigned int flags)
{
    stModelInstance2D* node = static_cast<stModelInstance2D*>(g_surface_nodes_654adc[cell]);

    if (node != 0) {
        short position_x = node->right_16c;
        short position_y = node->bottom_16e;
        int columns = node->GetWidth00480EF0() >> 3;
        int rows = node->GetHeight00480F70() >> 3;

        for (int index = 0; index != 0x12c0; ++index) {
            if (g_surface_nodes_654adc[index] == node) {
                g_surface_nodes_654adc[index] = 0;
                g_block_652ddc[index] = 0;
            }
        }
        srMeshModel* model = static_cast<srMeshModel*>(node->model());
        if (model != 0) {
            srTextureIFace* texture = model->getTexture(0, 0);
            if (texture != 0) {
                texture->invalidate();
            }
        }
        node->release();
        int start = (position_y >> 3) * 0x50 + (position_x >> 3);
        for (int row = rows; row != 0; --row) {
            int row_cell = start;
            for (int column = columns; column != 0; --column) {
                InvalidateDirtyTile004259B0(row_cell, 0);
                ++row_cell;
            }
            start += 0x50;
        }
    }
    unsigned char state = g_block_652ddc[cell] | static_cast<unsigned char>(flags) | 0x40;
    g_block_652ddc[cell] = state;
    ++g_dword_6596d8;
    int bottom = (cell / 0x50) * 8 + 8;
    int top = (cell / 0x50) * 8;
    int right = (cell % 0x50) * 8 + 8;
    int left = (cell % 0x50) * 8;
    if (g_flag_65970d &&
        ((g_viewport_left_6595e8 <= left && left <= g_viewport_right_6595f0) ||
         (g_viewport_left_6595e8 <= right && right <= g_viewport_right_6595f0)) &&
        ((g_viewport_top_6595ec <= top && top <= g_viewport_bottom_6595f4) ||
         (g_viewport_top_6595ec <= bottom && bottom <= g_viewport_bottom_6595f4))) {
        g_block_652ddc[cell] = state | 3;
        g_flag_6596ea = 1;
    }
}

// FUNCTION: WIZ8 0x00422d50
void InvalidateRegion(int left, int top, int right, int bottom, unsigned int flags)
{
    unsigned char cell_flags;
    unsigned int clipped_left;
    unsigned int clipped_right;
    unsigned int x;

    cell_flags = 0;
    if (g_flags_6596e8[g_index_6596e4] == 0) {
        clipped_right = 0x280;
        /* The low clamp is a conditional expression because the original is
           branchless there and branches on the high one, and it is written
           <= 0 rather than < 1: the two are the same test and VC6 encodes them
           differently, setle against setl. */
        clipped_left = left <= 0 ? 0 : left;
        if ((int)clipped_left > 0x27f) {
            clipped_left = 0x280;
        }
        top = top <= 0 ? 0 : top;
        if (top > 0x1df) {
            top = 0x1e0;
        }
        /* Rounded up to the next cell boundary, with C's truncating division so
           a negative right edge collapses rather than wrapping. */
        right = ((right + 7) / 8) * 8;
        if (right <= 0 || right < 0x280) {
            clipped_right = right <= 0 ? 0 : right;
        }
        bottom = ((bottom + 7) / 8) * 8;
        if (bottom <= 0 || bottom < 0x1e0) {
            bottom = bottom <= 0 ? 0 : bottom;
        } else {
            bottom = 0x1e0;
        }
        if ((int)(clipped_right - clipped_left) > 0 && bottom - top > 0) {
            if (clipped_right - clipped_left == 0x280 && bottom - top == 0x1e0) {
                g_flags_6596e8[g_index_6596e4] = 1;
            }
            if (flags & 4) {
                cell_flags = 0x80;
            }
            if (flags & 1) {
                cell_flags = cell_flags | 2;
            }
            for (; top < bottom; top = top + 8) {
                if ((int)clipped_left < (int)clipped_right) {
                    x = clipped_left;
                    do {
                        InvalidateDirtyTile004259B0((int)x / 8 + (top / 8) * 0x50, cell_flags);
                        x = x + 8;
                    } while ((int)x < (int)clipped_right);
                }
            }
        }
    }
}

/* Invalidate each rectangle in a run; a flagged region cancels the rest. */
// FUNCTION: WIZ8 0x00422ec0
void Function422EC0(W8ScreenRect* rects, unsigned int count, int flags)
{
    unsigned int index;

    for (index = 0; index < count; ++index) {
        if (g_flags_6596e8[g_index_6596e4] != 0) {
            return;
        }
        InvalidateRegion(rects[index].left, rects[index].top, rects[index].right,
                         rects[index].bottom, flags);
    }
}

/* Coalesces dirty 8x8 cells into rectangular texture updates. Retail keeps the
   software surface locked for the complete batch and clears only the uploaded
   bit, preserving the lower per-cell state for the page lifecycle. */
// FUNCTION: WIZ8 0x00425b40
void FlushDirtyTiles00425B40(void)
{
    DDSURFACEDESC description;

    if (g_dword_6596d8 == 0) {
        return;
    }
    DDLockSurface(g_primary_surface_6596a8, 0, &description, 0, 0);
    for (int row = 0; row != 60; ++row) {
        int column = 0;
        while (column < 80) {
            int cell = row * 80 + column;
            if ((g_block_652ddc[cell] & 0x40) == 0) {
                ++column;
                continue;
            }

            int width = 0;
            while (column + width < 80 && (g_block_652ddc[cell + width] & 0x40) != 0) {
                ++width;
            }
            int height = 0;
            while (row + height < 60 && (g_block_652ddc[cell + height * 80] & 0x40) != 0) {
                ++height;
            }

            g_surface_node_659664->updateRectangle(g_gerd_659634, description.lpSurface,
                                                   description.lPitch, column * 8, row * 8,
                                                   (column + width) * 8, (row + height) * 8);
            for (int y = 0; y != height; ++y) {
                for (int x = 0; x != width; ++x) {
                    g_block_652ddc[cell + y * 80 + x] &= 0x3f;
                }
            }
            column += width;
        }
    }
    DDUnlockSurface(g_primary_surface_6596a8, 0);
    g_dword_6596d8 = 0;
}

/* Viewport. */
/*
 * Sets the viewport and rebuilds the camera view plane to match it.
 *
 * The srCamera and srGERD entry points are declared to produce exactly the
 * decorated names Wiz8.exe imports, taken from its import table: two
 * setViewPlane overloads, the const getViewPlane, and srGERD::flush. The
 * dllimport is not decoration - without it VC6 emits a direct call to the
 * thunk where the canonical has an indirect call through the import table.
 */

// FUNCTION: WIZ8 0x00426250
void SetViewport(int left, int top, int right, int bottom)
{
    float fractional_left;
    float fractional_top;
    float fractional_right;
    float fractional_bottom;
    srCamera::Rect view;
    srCamera::Rect plane;
    double depth;

    if (g_gerd_659634 != 0 && g_flush_pending_603c3a) {
        g_gerd_659634->flush();
    }
    fractional_left = (float)left * g_scale_x_5ebb1c;
    g_viewport_right_6595f0 = right + 1;
    g_viewport_left_6595e8 = left;
    g_viewport_bottom_6595f4 = bottom + 1;
    fractional_top = (float)top * g_scale_y_5ebb20;
    g_viewport_top_6595ec = top;
    fractional_right = (float)g_viewport_right_6595f0 * g_scale_x_5ebb1c;
    fractional_bottom = (float)g_viewport_bottom_6595f4 * g_scale_y_5ebb20;

    if (g_world != 0 && g_world->camera != 0) {
        g_world->camera->setViewPlane(3.14159265358979323846 * g_float_005ebcf8 * 85.0f,
                                      3.14159265358979323846 * g_float_005ebcf8 * 71.0f);
        g_world->camera->getViewPlane(view, depth);

        plane.left = (double)fractional_left * (view.right - view.left) + view.left;
        plane.right = (double)fractional_right * (view.right - view.left) + view.left;
        plane.bottom =
            (double)((g_double_005ebc30 - fractional_bottom) * (float)(view.top - view.bottom) +
                     (float)view.bottom);
        plane.top =
            (double)((g_double_005ebc30 - fractional_top) * (float)(view.top - view.bottom) +
                     (float)view.bottom);

        g_world->camera->setViewPlane(plane, 1.0);
        if (g_world_659ab8 != 0) {
            g_world_659ab8->camera->setViewPlane(plane, 1.0);
        }
        if (g_flag_652da4) {
            UpdateCameraView00450080(g_world->camera, 1);
        }
    }
}

// GLOBAL: WIZ8 0x00603c70
char g_video_config_file[260] = "3DVideo.CFG";

// FUNCTION: WIZ8 0x004229d0
void PrintScreen(void)
{
    g_flag_659711 = 1;
}

// FUNCTION: WIZ8 0x004277d0
void VideoInspectorEnable(void)
{
    g_flag_65970f = 1;
}

// GLOBAL: WIZ8 0x006548a0
INT32 g_help_box_width;
// GLOBAL: WIZ8 0x00654acc
INT32 g_help_box_height;

// GLOBAL: WIZ8 0x00654aac
int g_screen_transition_object_count_654aac;
// GLOBAL: WIZ8 0x00654ab4
srClass** g_screen_transition_objects_654ab4;

// FUNCTION: WIZ8 0x00429770
void VideoRemoveToolTip(void)
{
    int index;
    srClass* object;

    while (g_screen_transition_object_count_654aac != 0) {
        object = g_screen_transition_objects_654ab4[0];
        if (g_screen_transition_object_count_654aac > 0) {
            for (index = 0; index < g_screen_transition_object_count_654aac - 1; ++index) {
                g_screen_transition_objects_654ab4[index] =
                    g_screen_transition_objects_654ab4[index + 1];
            }
            --g_screen_transition_object_count_654aac;
        }
        object->release();
    }
}

// FUNCTION: WIZ8 0x00424A90
srNode* VideoMakePoster(srColorSurfaceIFace* surface, float width, float height,
                        unsigned char positional_3)
{
    srTextureIFace::e_hint hint;
    srTextureMap* texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
    texture->setMipmapBias(-8.0f);
    texture->autoRelease();
    texture->setName("VideoMakePoster");
    texture->setSurfacePtr(surface);
    texture->setWrapS(srTextureIFace::WRAP_POSITIONAL_1);
    texture->setWrapT(srTextureIFace::WRAP_POSITIONAL_1);
    if (positional_3 == 0) {
        hint = srTextureIFace::HINT_POSITIONAL_1;
    } else {
        hint = srTextureIFace::HINT_POSITIONAL_2;
    }
    texture->enableHint(hint);
    return Function424BA0(texture, width, height, positional_3);
}

void PresentMenuOverlayFrame(void)
{
    srNode::ProcessInfo process;

    FlushDirtyTiles00425B40();
    g_gerd_659634->beginFrame();
    process.renderer = g_gerd_659634;
    g_surface_node_659664->process(process, (srNode::e_processType)0);
    g_gerd_659634->flushRenderers();
    g_gerd_659634->endFrame();
}

// FUNCTION: WIZ8 0x00425570
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled)
{
    if (g_surface_node_659664) {
        g_surface_node_659664->setTextureHint2Enabled(enabled);
    }
}

// FUNCTION: WIZ8 0x00424040
unsigned char InitializeMouseSurface(void)
{
    srPixelConvert::e_surfaceType type;

    if (g_pixel_format_603c48 == 7) {
        type = srPixelConvert::SURFACE_RGB565;
    } else if (g_pixel_format_603c48 == 8) {
        type = srPixelConvert::SURFACE_RGB555;
    } else if (g_pixel_format_603c48 == 9) {
        type = srPixelConvert::SURFACE_ARGB1555;
    } else {
        return 0;
    }

    g_mouse_surface_659688 = SR_NEW(W8ColorSurface)(type, 128UL, 128UL);
    if (!g_mouse_surface_659688) {
        srAssertFail("psrMouseSurface", "C:\\Projects\\Wizardry 8\\Engine Code\\Video2.cpp", 0x635,
                     0);
    }
    g_mouse_surface_659688->setFilter(&srBoxFilter);
    g_mouse_surface_659688->fill(0);
    return 1;
}

// FUNCTION: WIZ8 0x00423500
unsigned char InitializeRendererSceneObjects(void)
{
    DDSURFACEDESC surface_description;
    srCamera::Rect view;
    srMaterial* material;
    srVector4T<float> material_value;
    char renderer_name[128];

    InitializeMouseSurface();
    g_modeler_65963c = new srModeler;
    g_scene_permanent_659648 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_permanent_659648->setName("2D Permanent Overlay Scene");
    g_scene_permanent_659648->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_permanent_659648->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_user_659640 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_user_659640->setName("2D User Overlay Scene");
    g_scene_user_659640->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_user_659640->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_fullscreen_659644 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_fullscreen_659644->setName("Full Screen Overlay Scene");
    g_scene_fullscreen_659644->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_fullscreen_659644->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay0_659654 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_overlay0_659654->setName("2D Overlay Scene (0)");
    g_scene_overlay0_659654->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay0_659654->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay1_659658 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_overlay1_659658->setName("2D Overlay Scene (1)");
    g_scene_overlay1_659658->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay1_659658->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_square_65965c = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_square_65965c->setName("2D Square Overlay Scene");
    g_scene_square_65965c->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_square_65965c->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender0_65964c = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_prerender0_65964c->setName("2D Pre-render Overlay Scene (0)");
    g_scene_prerender0_65964c->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender0_65964c->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender1_659650 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_prerender1_659650->setName("2D Pre-render Overlay Scene (1)");
    g_scene_prerender1_659650->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender1_659650->setFogColor(0.0f, 0.0f, 0.0f);

    g_overlay_camera_659670 = SR_NEW(srCamera)(static_cast<srNode*>(0));
    g_overlay_camera_659670->setName("2D Overlay Camera");
    g_overlay_camera_659670->setClipRange(0.01, 2.0);
    g_overlay_camera_659670->setLocation(0.0, 0.0, -1.0);
    g_overlay_camera_659670->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 1.0;
    g_overlay_camera_659670->setViewPlane(view, 1.0);
    g_overlay_camera_659670->setEnvironmentRange(0.0f, 0.0f);

    g_square_camera_659674 = SR_NEW(srCamera)(g_scene_square_65965c);
    g_square_camera_659674->setName("2D Square Overlay Camera");
    g_square_camera_659674->setClipRange(0.01, 2.0);
    g_square_camera_659674->setLocation(0.0, 0.0, -1.0);
    g_square_camera_659674->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 0.75;
    g_square_camera_659674->setViewPlane(view, 1.0);
    g_square_camera_659674->setEnvironmentRange(0.0f, 0.0f);

    material = SR_NEW(srMaterial);
    g_blit_material_65967c = material;
    material->setName("Blit Rect Material");
    material_value = 1.0f;
    material->setEmissive(material_value);
    material_value = 0.0f;
    material->setDiffuse(material_value);
    material->setSpecular(material_value);
    material->setOpacity(1.0);

    memset(g_surface_nodes_654adc, 0, sizeof(g_surface_nodes_654adc));
    memset(g_block_652ddc, 0, sizeof(g_block_652ddc));
    g_viewport_left_6595e8 = 0;
    g_viewport_top_6595ec = 0;
    g_viewport_right_6595f0 = 0;
    g_surface_state_6595dc = 0x100a017;
    g_surface_state_654ad8 = 0x100c0b7;
    g_dword_6596d8 = 0;
    g_viewport_bottom_6595f4 = 0;

    memset(&surface_description, 0, sizeof(surface_description));
    surface_description.dwSize = sizeof(surface_description);
    DDLockSurface(g_primary_surface_6596a8, 0, &surface_description, 0, 0);
    DDUnlockSurface(g_primary_surface_6596a8, 0);
    g_primary_color_surface_659660 = SR_NEW(W8ColorSurface)(
        srPixelConvert::SURFACE_ARGB1555, surface_description.lpSurface, 640UL, 480UL,
        static_cast<unsigned long>(surface_description.lPitch));
    if (!g_primary_color_surface_659660)
        return 0;

    g_surface_node_659664 =
        new stSurface2D(g_primary_color_surface_659660, 640, 480, g_scene_overlay0_659654, 128);
    if (!g_surface_node_659664)
        return 0;

    strncpy(renderer_name, g_gerd_659634->getName(), 127);
    renderer_name[127] = 0;
    _strupr(renderer_name);
    if (strstr(renderer_name, "GLIDE")) {
        g_surface_node_659664->enableRendererFlag(1);
    }
    g_renderer_mode_603d74 = strstr(renderer_name, "DIRECT3D") || strstr(renderer_name, "GLIDE") ||
                             strstr(renderer_name, "SOFTWARE2");
    return 1;
}

/* Zero a rectangle of the primary surface, one row at a time. The span is
   doubled because the surface holds sixteen-bit pixels, and the row clear is an
   ordinary memset that VC6 expands into a dword run with a byte remainder.

   Unlike the other lock site in this unit, the descriptor is not cleared before
   locking. That is the original's own sequence, reproduced. */
// FUNCTION: WIZ8 0x004263f0
void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom)
{
    DDSURFACEDESC surface_description;
    unsigned char* row;
    int rows;

    DDLockSurface(g_primary_surface_6596a8, 0, &surface_description, 0, 0);
    if (surface_description.lpSurface != 0) {
        if (top < bottom) {
            row = reinterpret_cast<unsigned char*>(surface_description.lpSurface) + left * 2 +
                  surface_description.lPitch * top;
            rows = bottom - top;
            do {
                memset(row, 0, (right - left) * 2);
                row = row + surface_description.lPitch;
                --rows;
            } while (rows != 0);
        }
        DDUnlockSurface(g_primary_surface_6596a8, 0);
    }
}

// FUNCTION: WIZ8 0x00426500
void PurgeInactiveSceneInstances(srScene* scene)
{
    srNode* node;

    if (!scene)
        return;
    node = scene->firstChild();
    while (node) {
        srNode* next = node->nextSibling();
        unsigned long class_id = node->getClassID();
        srModelInstance* instance = static_cast<srModelInstance*>(node);
        unsigned char display_state = 0;
        if (class_id == 0x10004) {
            display_state = static_cast<stModelInstance*>(node)->displayState();
        } else if (class_id == 0x10005) {
            display_state = static_cast<stModelInstance2D*>(node)->displayState();
        }
        if ((class_id == 0x10004 || class_id == 0x10005) && display_state != 3) {
            int index;
            for (index = 0; index != 0x12c0; ++index) {
                if (g_surface_nodes_654adc[index] == node) {
                    g_surface_nodes_654adc[index] = 0;
                    g_block_652ddc[index] = 0;
                }
            }
            if (instance->model()) {
                srMeshModel* model = static_cast<srMeshModel*>(instance->model());
                if (model) {
                    srTextureIFace* texture = model->getTexture(0, 0);
                    if (texture)
                        texture->invalidate();
                }
            }
            node->release();
        }
        node = next;
    }
}

// FUNCTION: WIZ8 0x00425820
void ClearNodeFlag(srNode* node)
{
    if (node) {
        node->setFlag(srNode::FLAG_POSITIONAL_0);
    }
}

// FUNCTION: WIZ8 0x00427810
srModelInstance* GetValue65962C(void)
{
    return g_current_model_instance_65962c;
}

// FUNCTION: WIZ8 0x00427820
void SetValue65962C(srModelInstance* value)
{
    g_current_model_instance_65962c = value;
}

// FUNCTION: WIZ8 0x00428010
unsigned char ClearFlag603C60(void)
{
    g_flag_603c60 = 0;
    return 1;
}

// FUNCTION: WIZ8 0x00428020
unsigned char SetFlag603C60(void)
{
    g_flag_603c60 = 1;
    return 1;
}

/* Release a renderer-owned object, leaving the renderer in 2D mode, or in
   the paired mode when its state byte says otherwise. The +0x160 flag lives
   past the 0x160-byte srModelInstance base, so the object is a larger
   derivative (stModelInstance-family shape); the filling producer is
   unrecovered, hence the offset read stays marked. */
// FUNCTION: WIZ8 0x004257F0
void ReleaseRendererObject004257F0(srClass* object)
{
    if ((reinterpret_cast<unsigned char*>(object)[0x160] & 1) !=
        0) { /* reinterpret-ok: unrecovered derivative tail past the srModelInstance base */
        g_dword_6596ec = 2;
    } else {
        g_dword_6596f0 = 2;
        g_dword_6596ec = 2;
    }
    object->release();
}

// FUNCTION: WIZ8 0x00428A90
void SetRendererMode6596EC(void)
{
    g_dword_6596ec = 2;
}

// FUNCTION: WIZ8 0x00428AA0
void SetRendererModePair(void)
{
    g_dword_6596f0 = 2;
    g_dword_6596ec = 2;
}

/* Install a texture (often an stTextureAnim) on the mouse-cursor mesh. A null
   argument reuses the current cursor texture. */
// FUNCTION: WIZ8 0x00429170
void SetMouseCursorTexture(srTextureIFace* texture)
{
    if (texture == 0) {
        texture = g_cursor_texture_659690;
    }
    g_cursor_model_65968c->setTexture(texture, 0, 0);
    if ((g_cursor_model_65968c->control_state_390 & 8) == 0) {
        unsigned long state = g_cursor_model_65968c->control_state_390;
        g_cursor_model_65968c->control_state_390 = state | 8;
        g_cursor_model_65968c->control_state_390 = state | 8;
    }
}

// FUNCTION: WIZ8 0x004291C0
unsigned char GetRendererModeByte(void)
{
    return (unsigned char)g_renderer_mode_603d74;
}

// FUNCTION: WIZ8 0x00429200
void SetValue659668(const int* value)
{
    g_value_659668 = value;
}

// FUNCTION: WIZ8 0x004297D0
bool HasScreenTransitionObjects(void)
{
    return g_screen_transition_object_count_654aac != 0;
}

// FUNCTION: WIZ8 0x004298E0
void SetFlag603C4C(unsigned char value)
{
    g_flag_603c4c = value;
}

// FUNCTION: WIZ8 0x004298F0
unsigned char HasEnoughFreeDiskSpace(void)
{
    FARPROC extended;
    LARGE_INTEGER available;
    LARGE_INTEGER capacity;
    LARGE_INTEGER free_bytes;
    DWORD sectors_per_cluster;
    DWORD bytes_per_sector;
    DWORD free_clusters;
    DWORD total_clusters;
    unsigned int megabytes;
    unsigned char enough;

    extended = GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetDiskFreeSpaceExA");
    if (extended != NULL) {
        GetDiskFreeSpaceExA(NULL, (PULARGE_INTEGER)&available, (PULARGE_INTEGER)&capacity,
                            (PULARGE_INTEGER)&free_bytes);
        megabytes = (unsigned int)(free_bytes.QuadPart / 0x100000);
        enough = megabytes >= 0x100;
        return enough;
    }
    GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters,
                      &total_clusters);
    megabytes = (unsigned int)((__int64)sectors_per_cluster * bytes_per_sector * free_clusters /
                               0x400 / 0x400);
    enough = megabytes >= 0x100;
    return enough;
}

// FUNCTION: WIZ8 0x00429AF0
void __fastcall ReleaseOwnedClass00429AF0(srClass** owner)
{
    if (*owner) {
        (*owner)->release();
    }
}

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

// FUNCTION: WIZ8 0x004229b0
unsigned char VideoIsFullScreen(void)
{
    return g_fullscreen_603c39;
}

// FUNCTION: WIZ8 0x00428b80
int VideoDumpMemoryLeaks(void)
{
    return 0;
}

/* The retail linker folds this trivial TRUE stub with other returns; its
   0x42B830 address sits past the proven Video2 hull, so its placement stays
   provisional. */
// FUNCTION: WIZ8 0x0042b830
BOOLEAN CheckCdPresent(void)
{
    return TRUE;
}

// FUNCTION: WIZ8 0x00427a70
void VideoGetClientRect(RECT* rect)
{
    GetClientRect(ghWindow, rect);
    ClientToScreen(ghWindow, (POINT*)rect);
    ClientToScreen(ghWindow, (POINT*)&rect->right);
}

// FUNCTION: WIZ8 0x00421f20
IDirectDrawSurface2* GetFrameBufferObject(void)
{
    return g_primary_surface_6596a8;
}

// FUNCTION: WIZ8 0x00421f40
unsigned char GetPrimaryRGBDistributionMasks(unsigned int* red, unsigned int* green,
                                             unsigned int* blue)
{
    *red = gusRedMask;
    *green = gusGreenMask;
    *blue = gusBlueMask;
    return 1;
}

// FUNCTION: WIZ8 0x00421fd0
void* LockMouseBuffer(unsigned int* pitch)
{
    *pitch = g_mouse_surface_659688->getPitch();
    return g_mouse_surface_659688->getDataPtr();
}

/* The retail linker folds this empty body with other empty C functions. */
void UnlockMouseBuffer(void) {}

/* The retail empty video-capture entry folds with the shared 0x4023a0 ret. */
void VideoCaptureToggle(void) {}

// FUNCTION: WIZ8 0x00421f30
IDirectDraw2* GetDirectDraw2Object(void)
{
    return g_direct_draw2_6596a0;
}

// GLOBAL: WIZ8 0x006596f4
unsigned char g_flag_6596f4;
// GLOBAL: WIZ8 0x00659724
int g_screenshot_index_659724;
// GLOBAL: WIZ8 0x00659728
int g_screenshot_page_659728;

// FUNCTION: WIZ8 0x004229e0
void Function4229E0(void)
{
    srSurfaceIOManager* surface_io_manager = srCore.getSurfaceIOManager();
    srExtension::load("JPEGImporter", 0);

    srColorSurfaceIFace* surface = g_gerd_659634->lockBuffer();
    int screenshot_index = g_screenshot_index_659724;
    if (surface != 0) {
        char filename[32];
        srSurfaceIOManager::ExportInfo options;
        options.unknown_00 = 0;
        options.unknown_04 = 1;
        options.option_string = 0;

        ++g_screenshot_index_659724;
        sprintf(filename, "Wiz8%5.5d.JPG", screenshot_index);
        if (g_flag_6596f4 == 0) {
            surface_io_manager->exportSurface(filename, *surface, options);
        } else {
            options.option_string = "QUALITY=0.35";
            PauseSharedGameTimers00439BC0();
            surface_io_manager->exportSurface(filename, *surface, options);
            ResumeSharedGameTimers00439CA0();
        }
        g_gerd_659634->unlockBuffer();
    }
    g_screenshot_page_659728 = (g_screenshot_page_659728 - 1) & 1;
}

/* Tooltip placement state. The left/top pair records the last position the
   tooltip builder used; the scale participates in the texture mapping. */
// GLOBAL: WIZ8 0x00654ab8
int g_help_box_x_654ab8;
// GLOBAL: WIZ8 0x00654abc
int g_help_box_y_654abc;
// GLOBAL: WIZ8 0x00654ab0
int g_screen_transition_object_capacity_654ab0;

// GLOBAL: WIZ8 0x005ebe88
double g_double_005ebe88 = 0.0020833333333333333;
// GLOBAL: WIZ8 0x005ebe90
double g_double_005ebe90 = 0.0015625;
// GLOBAL: WIZ8 0x005ebf40
double g_double_005ebf40 = 0.75;

/* Packs four normalized colour components into the surface byte order:
   red, green, blue, alpha from the high byte down. */
// FUNCTION: WIZ8 0x00429700
void __fastcall PackColour00429700(unsigned char* colour, double red, double green, double blue,
                                   double alpha)
{
    colour[3] = (int)(red * g_double_005ebf60);
    colour[2] = (int)(green * g_double_005ebf60);
    colour[1] = (int)(blue * g_double_005ebf60);
    colour[0] = (int)(alpha * g_double_005ebf60);
}

/* Places one tooltip node at a screen position in normalized coordinates.
   With positional set, the position is snapped to the renderer's pixel grid;
   the node keeps the screen x/y in its right/bottom extent fields. */
// FUNCTION: WIZ8 0x004255F0
void PositionToolTipNode(srNode* node, int x, int y, char positional)
{
    stModelInstance2D* instance = static_cast<stModelInstance2D*>(node);
    double position_x = (double)x * g_double_005ebe90;
    double position_y = (double)y * g_double_005ebe88;

    if (positional != 0 && g_gerd_659634 != 0) {
        double whole;
        long width = g_gerd_659634->getWidth();
        double fraction = modf((double)width * position_x, &whole);
        position_x -= fraction / (double)width;
        long height = g_gerd_659634->getHeight();
        fraction = modf((double)height * position_y, &whole);
        position_y -= fraction / (double)height;
    }

    int width = instance->GetWidth00480EF0() & 0xffff;
    double half_width = (double)width * g_double_005ebe90 * g_double_005ebe80;
    int height = instance->GetHeight00480F70() & 0xffff;
    double half_height = (double)height * g_double_005ebe88 * g_double_005ebe80;

    srVector3T<double> location;
    location.x = half_width + position_x;
    location.z = -0.0001;
    if ((instance->state_160 & 1U) == 0) {
        location.y = g_double_005ebc30 - (half_height + position_y);
        g_dword_6596f0 = 2;
    } else {
        location.y = g_double_005ebf40 - (half_height + position_y) * g_double_005ebf40;
    }
    node->setLocation(location);
    g_dword_6596ec = 2;
    instance->right_16c = (short)x;
    instance->bottom_16e = (short)y;
}

/* Positions every live tooltip object left to right starting at x, advancing
   the cursor by each node's scaled width. With no live objects, just record
   the requested position. */
// FUNCTION: WIZ8 0x00429210
void VideoPositionToolTip(INT32 x, INT32 y)
{
    if (g_screen_transition_object_count_654aac > 0) {
        INT32 offset = x;
        for (int index = 0; index < g_screen_transition_object_count_654aac; ++index) {
            srNode* node = static_cast<srNode*>(g_screen_transition_objects_654ab4[index]);
            PositionToolTipNode(node, offset, y, 1);
            offset += static_cast<stModelInstance2D*>(node)->GetWidth00480EF0() & 0xffff;
        }
        g_help_box_y_654abc = y;
        g_help_box_x_654ab8 = x;
        return;
    }
    g_help_box_x_654ab8 = x;
    g_help_box_y_654abc = y;
}

/* Copies the tooltip source rectangle into a size-rounded 16-bit surface,
   repeating its border one pixel outward, and reports the texture mapping
   scales for the resulting polygon brush. */
// FUNCTION: WIZ8 0x00428B90
unsigned char CopySurfaceWithBorder(srColorSurface* surface, int* rect, void* source,
                                    int source_pitch, float* scale_x, float* scale_y,
                                    float* mapping_x, float* mapping_y)
{
    if (surface == 0 || rect == 0 || source == 0 || source_pitch == 0 || scale_x == 0 ||
        scale_y == 0 || mapping_x == 0 || mapping_y == 0) {
        return 0;
    }
    int width = (rect[2] > 0x27f ? 0x280 : rect[2]) - rect[0];
    int height = (rect[3] > 0x1df ? 0x1e0 : rect[3]) - rect[1];
    UINT16* dest = (UINT16*)surface->getDataPtr();
    UINT32 dest_pitch = (UINT32)surface->getPitch();
    UINT16* src = (UINT16*)source;
    UINT32 src_pitch = (UINT32)source_pitch;
    int right = width + 1;
    int bottom = height + 1;

    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, 1, rect[0], rect[1], width, height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, 0, rect[0], rect[1], width, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 1, bottom, rect[0], rect[1] - 1 + height,
                    width, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, 1, rect[0], rect[1], 1, height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, 1, rect[0] - 1 + width, rect[1], 1,
                    height);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, 0, rect[0], rect[1], 1, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, 0, rect[0] - 1 + width, rect[1], 1, 1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, 0, bottom, rect[0], rect[1] - 1 + height, 1,
                    1);
    Blt16BPPTo16BPP(dest, dest_pitch, src, src_pitch, right, bottom, rect[0] - 1 + width,
                    rect[1] - 1 + height, 1, 1);

    float scale = 1.0f / (float)surface->getWidth();
    *scale_x = scale;
    *scale_x = scale * g_surface_scale_659680 + scale;
    scale = 1.0f / (float)surface->getHeight();
    *scale_y = scale;
    *scale_y = scale * g_surface_scale_659680 + scale;
    *mapping_x = (float)(rect[2] - rect[0]) / (float)surface->getWidth();
    *mapping_y = (float)(rect[3] - rect[1]) / (float)surface->getHeight();
    return 1;
}

/* Builds a polygon brush from a tooltip surface rectangle. The larger source
   extent is rounded up to the next power of two between 16 and 256, the copy
   repeats its border, and the node records the rectangle extents. */
// FUNCTION: WIZ8 0x00424280
srModelInstance* Video2DRectToPolygon(int* rect, void* source, int source_pitch, srNode* parent,
                                      unsigned char overlay)
{
    double left = (double)rect[0] * g_double_005ebe90;
    int extent = rect[2] - rect[0];
    double top = (double)rect[1] * g_double_005ebe88;
    int rect_height = rect[3] - rect[1];
    double width = (double)rect[2] * g_double_005ebe90 - left;
    double height = (double)rect[3] * g_double_005ebe88 - top;

    if (extent <= rect_height) {
        extent = rect_height;
    }
    if (extent < 0x10) {
        extent = 0x10;
    } else if (extent < 0x20) {
        if (extent != 0x10) {
            extent = 0x20;
        }
    } else if (extent < 0x40) {
        if (extent != 0x20) {
            extent = 0x40;
        }
    } else if (extent < 0x80) {
        if (extent != 0x40) {
            extent = 0x80;
        }
    } else {
        if (0x100 < extent) {
            return 0;
        }
        if (extent != 0x80) {
            extent = 0x100;
        }
    }

    srColorSurface* surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555,
                                                     (unsigned long)extent, (unsigned long)extent);
    if (surface == 0) {
        return 0;
    }
    surface->setFilter(&srBoxFilter);
    float scale_x;
    float scale_y;
    float mapping_x;
    float mapping_y;
    if (!CopySurfaceWithBorder(surface, rect, source, source_pitch, &scale_x, &scale_y, &mapping_x,
                               &mapping_y)) {
        surface->release();
        return 0;
    }
    surface->getDataPtr();
    srModelInstance* node = MakePolygonBrush(parent, surface, width, height, scale_x, scale_y,
                                             mapping_x, mapping_y, overlay);
    if (node != 0) {
        stModelInstance2D* instance = static_cast<stModelInstance2D*>(node);
        instance->state_160 = g_index_6596e4;
        instance->left_168 = (short)(rect[2] - rect[0]);
        instance->top_16a = (short)(rect[3] - rect[1]);
        instance->right_16c = (short)rect[0];
        instance->bottom_16e = (short)rect[1];
        srVector3T<double> location;
        location.x = width * g_double_005ebe80 + left;
        location.y = g_double_005ebc30 - (height * g_double_005ebe80 + top);
        location.z = -0.0001;
        node->setLocation(location);
        instance->setName("Video2DRectToPolygon");
    }
    return node;
}

/* Builds the help box: renders the text into an ARGB1555 surface, draws the
   border, converts the surface to a polygon brush, appends it to the live
   tooltip objects and positions them above the cursor. Only one tooltip is
   alive at a time. */
// FUNCTION: WIZ8 0x00429290
void VideoToolTip(UINT16* text)
{
    if (g_screen_transition_object_count_654aac != 0) {
        return;
    }
    srColorSurface* surface =
        SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, 0xfeUL, 0xfeUL);
    if (surface == 0) {
        return;
    }
    W8ControlsRect bounds;
    bounds.left = 0;
    bounds.top = 0;
    bounds.right = 0xfa;
    bounds.bottom = 0xfa;
    W8TextBuffer* buffer =
        new W8TextBuffer(&bounds, (const wchar_t*)text, g_font10arial_683668,
                         g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548, 4);
    if (buffer == 0) {
        return;
    }
    g_help_box_width = (int)buffer->m_maxLineWidth + 4;
    g_help_box_height = GetFontHeight(g_font10arial_683668) * buffer->m_lineCount + 2;
    surface->fill(0);
    void* data = surface->getDataPtr();
    unsigned char colour[4];
    for (int y = 0; y < g_help_box_height; ++y) {
        PackColour00429700(colour, 1.0, 0.0, 0.0, 0.0);
        surface->setHLine(0, y, g_help_box_width, *(unsigned long*)colour);
    }
    buffer->RenderText((int)data, (int)surface->getPitch(), 2, 1, 1);
    surface->setHLine(0, 0, g_help_box_width, 0xffed9954);
    surface->setHLine(0, g_help_box_height - 1, g_help_box_width, 0xffed9954);
    surface->setVLine(0, 0, g_help_box_height, 0xffed9954);
    surface->setVLine(g_help_box_width - 1, 0, g_help_box_height, 0xffed9954);

    int rect[4];
    rect[0] = 0;
    rect[1] = 0;
    rect[2] = 0xfe;
    rect[3] = 0xfe;
    srModelInstance* node =
        Video2DRectToPolygon(rect, data, (int)surface->getPitch(), g_cursor_scene_659684, 1);
    if (node != 0) {
        int count = g_screen_transition_object_count_654aac;
        bool append = true;
        if (g_screen_transition_object_capacity_654ab0 < count + 1) {
            srClass** objects = new srClass*[count + 1];
            if (objects == 0) {
                append = false;
            } else {
                for (int index = 0; index < count; ++index) {
                    objects[index] = g_screen_transition_objects_654ab4[index];
                }
                delete[] g_screen_transition_objects_654ab4;
                g_screen_transition_objects_654ab4 = objects;
                g_screen_transition_object_capacity_654ab0 = count + 1;
            }
        }
        if (append) {
            g_screen_transition_objects_654ab4[count] = static_cast<srClass*>(node);
            g_screen_transition_object_count_654aac = count + 1;
        }
    }

    int position_y = g_cursor_height_654ad4 - g_help_box_height;
    int position_x = g_cursor_width_654ad0;
    int offset = position_x;
    for (int index = 0; index < g_screen_transition_object_count_654aac; ++index) {
        srNode* object = static_cast<srNode*>(g_screen_transition_objects_654ab4[index]);
        PositionToolTipNode(object, offset, position_y, 1);
        offset += static_cast<stModelInstance2D*>(object)->GetWidth00480EF0() & 0xffff;
    }
    g_help_box_x_654ab8 = position_x;
    g_help_box_y_654abc = position_y;
    surface->release();
    delete buffer;
}

/* Renderer configuration helpers reached from the video device, not the
   persisted configuration block. */
// GLOBAL: WIZ8 0x659714
int g_resident_texture_policy_659714;

// FUNCTION: WIZ8 0x004266e0
void SetResidentTexturePolicy(int policy)
{
    if (policy != g_resident_texture_policy_659714) {
        g_gerd_659634->invalidateResidentTextures();
        g_gerd_659634->invalidateTextureCache();
        g_resident_texture_policy_659714 = policy;
    }
}

// FUNCTION: WIZ8 0x00428e60
unsigned int GetTotalPhysicalMemory(void)
{
    MEMORYSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys;
}

// FUNCTION: WIZ8 0x00429800
int GetRendererFamily(void)
{
    char name[128];
    if (!g_gerd_659634) {
        return -1;
    }
    strncpy(name, g_gerd_659634->getName(), sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    _strupr(name);
    if (strstr(name, "OPENGL"))
        return 0;
    if (strstr(name, "GLIDE"))
        return 2;
    if (strstr(name, "DIRECT3D"))
        return 1;
    return strstr(name, "SOFTWARE") ? 3 : 4;
}

// FUNCTION: WIZ8 0x004291d0
void SetDisplayGamma(float value)
{
    srVector3T<float> gamma;
    gamma = value;
    g_gerd_659634->setGamma(gamma);
}

/* Compiler-generated vtable and template emissions, grouped here as emission
   provenance. They are instantiation output from the SurRender headers, not
   authored Video2 bodies. */
// VTABLE: WIZ8 0x005EBE98
// class srClassSupport<srMeshModel, class srMeshModel, 0, 8208>

// TEMPLATE: WIZ8 0x00429B30
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassID

// TEMPLATE: WIZ8 0x00429B40
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassName

// TEMPLATE: WIZ8 0x00429B50
// srClassSupport<srMeshModel,srMeshModel,0,8208>::getClassNode

// TEMPLATE: WIZ8 0x00429BC0
// srClassSupport<srMeshModel,srMeshModel,0,8208>::clone

// SYNTHETIC: WIZ8 0x00424A50
// srClassSupport<srMeshModel,srMeshModel,0,8208>::`scalar deleting destructor'

/* CVDUMP includes the class tag on the repeated self-type argument in each
   vftable symbol below. These remain ordinary self-support instantiations. */
// VTABLE: WIZ8 0x005EBEEC
// class srClassSupport<srTextureMap, class srTextureMap, 0, 8465>

// SYNTHETIC: WIZ8 0x00424B70
// srClassSupport<srTextureMap,srTextureMap,0,8465>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429BE0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassID

// TEMPLATE: WIZ8 0x00429BF0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassName

// TEMPLATE: WIZ8 0x00429C00
// srClassSupport<srTextureMap,srTextureMap,0,8465>::getClassNode

// TEMPLATE: WIZ8 0x00429CA0
// srClassSupport<srTextureMap,srTextureMap,0,8465>::clone

// VTABLE: WIZ8 0x005EBDE0
// class srClassSupport<srMaterial, class srMaterial, 0, 8720>

// TEMPLATE: WIZ8 0x00429CC0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassID

// TEMPLATE: WIZ8 0x00429CD0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassName

// TEMPLATE: WIZ8 0x00429CE0
// srClassSupport<srMaterial,srMaterial,0,8720>::getClassNode

// TEMPLATE: WIZ8 0x00429D50
// srClassSupport<srMaterial,srMaterial,0,8720>::clone

// TEMPLATE: WIZ8 0x00429E80
// srClassSupport<srMaterialIFace,srClass,1,8704>::getClassID

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srCamera self-support template. */
// VTABLE: WIZ8 0x005EBE14
// class srClassSupport<srCamera, class srCamera, 0, 5120>

// TEMPLATE: WIZ8 0x0042A010
// srClassSupport<srCamera,srCamera,0,5120>::getClassID

// TEMPLATE: WIZ8 0x0042A020
// srClassSupport<srCamera,srCamera,0,5120>::getClassName

// TEMPLATE: WIZ8 0x0042A030
// srClassSupport<srCamera,srCamera,0,5120>::getClassNode

// TEMPLATE: WIZ8 0x0042A0A0
// srClassSupport<srCamera,srCamera,0,5120>::clone

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srScene self-support template. */
// VTABLE: WIZ8 0x005EBE48
// class srClassSupport<srScene, class srScene, 0, 4112>

// TEMPLATE: WIZ8 0x0042A0C0
// srClassSupport<srScene,srScene,0,4112>::getClassID

// TEMPLATE: WIZ8 0x0042A0D0
// srClassSupport<srScene,srScene,0,4112>::getClassName

// TEMPLATE: WIZ8 0x0042A0E0
// srClassSupport<srScene,srScene,0,4112>::getClassNode

// TEMPLATE: WIZ8 0x0042A150
// srClassSupport<srScene,srScene,0,4112>::clone

// VTABLE: WIZ8 0x005EBD10
// class srClassSupport<srColorSurface, class srColorSurface, 0, 12560>

// TEMPLATE: WIZ8 0x00429A40
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassID

// TEMPLATE: WIZ8 0x00429A50
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassName

// TEMPLATE: WIZ8 0x00429A60
// srClassSupport<srColorSurface,srColorSurface,0,12560>::getClassNode

// TEMPLATE: WIZ8 0x00429AD0
// srClassSupport<srColorSurface,srColorSurface,0,12560>::clone
