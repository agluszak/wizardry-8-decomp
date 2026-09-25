#include "wiz8/xstatus.h"
#ifdef WIZ8_RUNTIME_TESTS
#include "runtime_instrumentation.h"
#endif
#include "wiz8/engine_code/Video2.h"
#include "wiz8/layouts/screen_state.h"
#include "wiz8/local_code/Gameloop.h"
#include "wiz8/local_screens/Screens.h"
#include "wiz8/cursor.h"
#include "wiz8/engine_code/3dapi.h"
#include "wiz8/engine_code/Environment.h"
#include "wiz8/engine_code/GDCamera.h"
#include "wiz8/engine_code/Prop.h"
#include "wiz8/engine_code/Trigger.hpp"
#include "wiz8/engine_code/World.h"
#include "wiz8/engine_code/Octree.h"
#include "wiz8/engine_code/game_timer.h"
#include "wiz8/engine_code/stModelInstance.h"
#include "wiz8/engine_code/Level.h"
#include "wiz8/engine_code/stCube.h"
#include "wiz8/engine_code/stMeshModel.h"
#include "wiz8/float_constants.h"
#include "wiz8/fonts.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/local_code/Configuration.h"
#include "wiz8/local_screens/MainGameScreen.h"
#include "wiz8/regions.h"
#include "wiz8/engine_code/Quality.h"
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
#include "surrender/srModeler.h"
#include "surrender/srModelInstance.h"
#include "surrender/srPixelConvert.h"
#include "surrender/srPalette.h"
#include "surrender/srCamera.h"
#include "surrender/srTextureMap.h"
#include "wiz8/engine_code/stTextureAnim.h"
#include "surrender/srScene.h"
#include "surrender/srShader.h"
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
#include "vsurface.h"
#include "wiz8/local_code/ControlsRect.h"

#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiz8/engine_code/GameData.h"
#include "wiz8/engine_code/Monster.h"
#include "wiz8/engine_code/Octree.h"

/* Video2-internal helpers. Their only recovered callers are in this unit, so
   they are declared here instead of the released Video2 header. */
srModelInstance* Video2DRectToSquarePolygon(int* rect, void* source, int source_pitch,
                                            srNode* parent, unsigned char overlay);
srModelInstance* Video2DRectToPolygon(int* rect, void* source, int source_pitch, srNode* parent,
                                      unsigned char overlay);
unsigned char CopySurfaceWithBorder(srColorSurface* surface, int* rect, void* source,
                                    int source_pitch, float* scale_x, float* scale_y,
                                    float* mapping_x, float* mapping_y);
void SaveJpegScreenshot(void);
void FlushDirtyTiles(void);

/*
 * The renderer window and extension loading gate InitializeStandardGamingPlatform calls after the
 * input manager.
 */

/* The released SGP video unit owns this platform handle.  Wiz8 replaces the
   released video manager but keeps the same source-defined interface. */
// GLOBAL: WIZ8 0x006596CC
HWND ghWindow;

// GLOBAL: WIZ8 0x603c38
unsigned char g_world_pick_enabled = 1;
// GLOBAL: WIZ8 0x603c4c
unsigned char g_fullscreen_scene_last = 1;
// GLOBAL: WIZ8 0x603c60
bool g_cursor_scene_enabled = true;
// GLOBAL: WIZ8 0x603c6d
bool g_overlay_scenes_enabled = true;
// GLOBAL: WIZ8 0x603c68
int g_frame_reset_interval = 50;
// GLOBAL: WIZ8 0x603c39
unsigned char g_fullscreen = 1;
// GLOBAL: WIZ8 0x603c3a
bool g_flush_pending = true;
// GLOBAL: WIZ8 0x603c3c
int g_screen_width = 640;
// GLOBAL: WIZ8 0x603c40
int g_screen_height = 480;
// GLOBAL: WIZ8 0x603c44
int g_screen_depth = 16;
// GLOBAL: WIZ8 0x603c48
int g_pixel_format = 9;
// GLOBAL: WIZ8 0x603d74
int g_renderer_mode = 1;
// GLOBAL: WIZ8 0x65962c
srModelInstance* g_current_model_instance;
// GLOBAL: WIZ8 0x6596fc
int g_fps_frame_count;
// GLOBAL: WIZ8 0x659700
unsigned int g_fps_window_tick;
/* Per-page counters reset as each overlay page is retired; retail stores
   through g_active_page as an index, so this is one two-entry array. */
// GLOBAL: WIZ8 0x6596dc
int g_overlay_page_counters[2];
// GLOBAL: WIZ8 0x6596e8
unsigned char g_page_full_redraw[2];
// GLOBAL: WIZ8 0x654ac4
HINSTANCE g_instance_654ac4;
// GLOBAL: WIZ8 0x659620
unsigned short g_show_command;
// GLOBAL: WIZ8 0x6595f8
WNDPROC g_window_proc;
// GLOBAL: WIZ8 0x659710
bool g_video_active;
// GLOBAL: WIZ8 0x65970e
unsigned char g_world_blacked_out;
// GLOBAL: WIZ8 0x659711
bool g_screenshot_pending;
// GLOBAL: WIZ8 0x65970f
bool g_video_inspector_enabled;
// GLOBAL: WIZ8 0x00659634
srGERD* g_gerd;
// GLOBAL: WIZ8 0x65971c
srGERD* g_secondary_gerd;
// GLOBAL: WIZ8 0x65969c
LPDIRECTDRAW g_direct_draw;
// GLOBAL: WIZ8 0x6596a0
LPDIRECTDRAW2 g_direct_draw2;
// GLOBAL: WIZ8 0x6596a4
LPDIRECTDRAWSURFACE g_primary_surface1;
// GLOBAL: WIZ8 0x6596a8
LPDIRECTDRAWSURFACE2 g_primary_surface;
// GLOBAL: WIZ8 0x6596ac
LPDIRECTDRAWSURFACE g_video_primary_surface1;
// GLOBAL: WIZ8 0x6596b0
LPDIRECTDRAWSURFACE2 g_video_primary_surface2;
// GLOBAL: WIZ8 0x659610
RECT g_window_rect;

// GLOBAL: WIZ8 0x600088
unsigned int g_color_key = 0x3def;
// GLOBAL: WIZ8 0x65963c
srModeler* g_modeler_65963c;
// GLOBAL: WIZ8 0x659640
srScene* g_scene_user;
// GLOBAL: WIZ8 0x659644
srScene* g_scene_fullscreen;
// GLOBAL: WIZ8 0x659648
srScene* g_scene_permanent;
// GLOBAL: WIZ8 0x65964c
srScene* g_scene_prerender0;
// GLOBAL: WIZ8 0x659650
srScene* g_scene_prerender1;
// GLOBAL: WIZ8 0x659654
srScene* g_scene_overlay0;
// GLOBAL: WIZ8 0x659658
srScene* g_scene_overlay1;
// GLOBAL: WIZ8 0x65965c
srScene* g_scene_square;
// GLOBAL: WIZ8 0x659660
srColorSurface* g_primary_color_surface;
// GLOBAL: WIZ8 0x659664
class stSurface2D* g_surface_node;
// GLOBAL: WIZ8 0x659680
float g_surface_scale;
// GLOBAL: WIZ8 0x659670
srCamera* g_overlay_camera;
// GLOBAL: WIZ8 0x659674
srCamera* g_square_camera;
// GLOBAL: WIZ8 0x659688
srColorSurface* g_mouse_surface;
// GLOBAL: WIZ8 0x65967c
srMaterial* g_blit_material;
// GLOBAL: WIZ8 0x654adc
srNode* g_surface_nodes[0x12c0];
// GLOBAL: WIZ8 0x6595dc
int g_surface_state_6595dc;
// GLOBAL: WIZ8 0x654ad8
int g_surface_state_654ad8;
// GLOBAL: WIZ8 0x6595e8
W8ViewportRect g_viewport_6595e8;
// GLOBAL: WIZ8 0x00659AB4
W8World* g_world;
// GLOBAL: WIZ8 0x00659AB8
W8World* g_world_659ab8;
// GLOBAL: WIZ8 0x652da4
bool g_camera_sway_active;
// GLOBAL: WIZ8 0x5ebb1c
extern const float g_scale_x_5ebb1c = 1.0f / 640.0f;
// GLOBAL: WIZ8 0x5ebb20
extern const float g_scale_y_5ebb20 = 1.0f / 480.0f;

// GLOBAL: WIZ8 0x652ddc
unsigned char g_tile_dirty_flags[0x12c0];
// GLOBAL: WIZ8 0x006596e4
unsigned int g_active_page;
// GLOBAL: WIZ8 0x006596d4
int g_video_inspector_mode;
// GLOBAL: WIZ8 0x6596d8
int g_dirty_tile_count;
// GLOBAL: WIZ8 0x006596ec
int g_overlay_render_mode;
// GLOBAL: WIZ8 0x006596f0
int g_paired_render_mode;
// GLOBAL: WIZ8 0x659668
const int* g_overlay_viewport;
// GLOBAL: WIZ8 0x65966c
srClass* g_render_object_65966c;
// GLOBAL: WIZ8 0x659678
srClass* g_render_object_659678;
// GLOBAL: WIZ8 0x659720
HWND g_window_659720;
// GLOBAL: WIZ8 0x65409c
unsigned int g_last_capture_tick;
// GLOBAL: WIZ8 0x659704
float g_frames_per_second;
// GLOBAL: WIZ8 0x659708
float g_seconds_per_frame;

// GLOBAL: WIZ8 0x65a118
unsigned char* g_render_options_65a118;

// GLOBAL: WIZ8 0x659684
srScene* g_cursor_scene;
// GLOBAL: WIZ8 0x65968c
srMeshModel* g_cursor_model;
// GLOBAL: WIZ8 0x659690
srTexture* g_cursor_texture;
// GLOBAL: WIZ8 0x659694
srModelInstance* g_cursor_node_659694;
// GLOBAL: WIZ8 0x659698
unsigned int g_cursor_move_tick;
// GLOBAL: WIZ8 0x654ad0
int g_cursor_width;
// GLOBAL: WIZ8 0x654ad4
int g_cursor_height;
// GLOBAL: WIZ8 0x6596b4
int g_cursor_image_width;
// GLOBAL: WIZ8 0x6596b8
int g_cursor_image_height;
// GLOBAL: WIZ8 0x6596bc
int g_cursor_hotspot_x;
// GLOBAL: WIZ8 0x6596c0
int g_cursor_hotspot_y;
// GLOBAL: WIZ8 0x6596c4
bool g_system_cursor_visible;

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

    DDLockSurface(g_primary_surface, NULL, &description, 0, NULL);
    *pitch = description.lPitch;
    return description.lpSurface;
}

/* Clear the software-facing frame and retire every transient 2D overlay.
   The four scene walks are the same typed operation used during renderer
   bring-up; keeping the reset here avoids reproducing SurRender's node ABI at
   the menu call site. */
// FUNCTION: WIZ8 0x00422b10
void ResetVideoFrameState(void)
{
    DDSURFACEDESC description;
    unsigned int active;

    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    DDLockSurface(g_primary_surface, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface, NULL);
    memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
    memset(g_surface_nodes, 0, sizeof(g_surface_nodes));
    active = g_active_page;
    g_page_full_redraw[active ^ 1] = 0;
    g_page_full_redraw[active] = 0;
    g_dirty_tile_count = 0;
    PurgeInactiveSceneInstances(g_scene_prerender0);
    PurgeInactiveSceneInstances(g_scene_overlay0);
    PurgeInactiveSceneInstances(g_scene_prerender1);
    PurgeInactiveSceneInstances(g_scene_overlay1);
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
    g_world_pick_enabled = 1;
    g_current_model_instance = 0;
    g_fps_frame_count = 0;
    g_fps_window_tick = GetTickCount();
    g_overlay_page_counters[0] = 0;
    g_overlay_page_counters[1] = 0;
    g_page_full_redraw[0] = 0;
    g_page_full_redraw[1] = 0;
    g_instance_654ac4 = instance;
    g_show_command = show_command;
    g_window_proc = (WNDPROC)window_proc;
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
    if (!g_video_active) {
        if (gXStatus.world_update_blocked) {
            ResumeMainGameWorld();
        }
        if (ghWindow && g_gerd) {
            g_video_active = 1;
            ShowWindow(ghWindow, 9);
            if (g_gerd->isWindowOpen() == 0) {
                if (!OpenRendererWindow()) {
                    goto done;
                }
            }
            OpenIcon(ghWindow);
            SetFocus(ghWindow);
            memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
            active = g_active_page;
            g_page_full_redraw[g_active_page ^ 1] = 0;
            g_dirty_tile_count = 0;
            g_page_full_redraw[active] = 0;
            PurgeInactiveSceneInstances(g_scene_prerender0);
            PurgeInactiveSceneInstances(g_scene_overlay0);
            PurgeInactiveSceneInstances(g_scene_prerender1);
            PurgeInactiveSceneInstances(g_scene_overlay1);
            InvalidateRegion(0, 0, 0x280, 0x1e0, 0);
            g_paired_render_mode = 2;
            g_overlay_render_mode = 2;
        }
    }
done:
    SetViewport(0, 0, 0x280, 0x1e0);
    if (g_video_inspector_enabled) {
        _chdir("DLL");
        srExtension::load("INSPECTOR", 0);
        _chdir(".");
    }
    if (!InitializeStartupNavigation()) {
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
    ShutdownVideoScenes();
    ShutdownStartupNavigation();
    if (g_video_active) {
        PauseMainGameWorld();
        g_video_active = 0;
        if (g_gerd) {
            g_flush_pending = false;
            g_gerd->closeWindow(static_cast<srGERD::e_closeHint>(0));
        }
        if (!g_fullscreen) {
            GetWindowRect(ghWindow, &g_window_rect);
        }
        ShowWindow(ghWindow, SW_MINIMIZE);
        FreeMouseCursor();
    }
    if (g_primary_surface1) {
        g_primary_surface1->Release();
        g_primary_surface1 = 0;
    }
    if (g_primary_surface) {
        g_primary_surface->Release();
        g_primary_surface = 0;
    }
    if (g_direct_draw) {
        g_direct_draw->Release();
        g_direct_draw = 0;
    }
    if (g_direct_draw2) {
        g_direct_draw2->Release();
        g_direct_draw2 = 0;
    }
    if (ghWindow) {
        CloseWindow(ghWindow);
        ghWindow = 0;
    }
    if (g_window_659720) {
        CloseWindow(g_window_659720);
        g_window_659720 = 0;
    }
    if (g_gerd) {
        g_flush_pending = false;
        g_gerd->closeWindow(static_cast<srGERD::e_closeHint>(0));
        g_gerd->deleteContext();
        g_gerd = 0;
    }
    if (g_secondary_gerd) {
        g_secondary_gerd->closeWindow(static_cast<srGERD::e_closeHint>(0));
        g_secondary_gerd->deleteContext();
        g_secondary_gerd = 0;
    }
    srConfig.removeAll();
    srExit();
}

/* Releases the renderer scene graph and 2D objects created by the video
   startup sequence; runs from ShutdownVideoManager before the DirectDraw
   teardown. */
// FUNCTION: WIZ8 0x00423f30
void ShutdownVideoScenes(void)
{
    if (g_modeler_65963c) {
        delete g_modeler_65963c;
        g_modeler_65963c = 0;
    }
    if (g_scene_permanent) {
        g_scene_permanent->release();
        g_scene_permanent = 0;
    }
    if (g_scene_user) {
        g_scene_user->release();
        g_scene_user = 0;
    }
    if (g_scene_fullscreen) {
        g_scene_fullscreen->release();
        g_scene_fullscreen = 0;
    }
    if (g_scene_overlay0) {
        g_scene_overlay0->release();
        g_scene_overlay0 = 0;
    }
    if (g_scene_overlay1) {
        g_scene_overlay1->release();
        g_scene_overlay1 = 0;
    }
    if (g_scene_square) {
        g_scene_square->release();
        g_scene_square = 0;
    }
    if (g_scene_prerender0) {
        g_scene_prerender0->release();
        g_scene_prerender0 = 0;
    }
    if (g_scene_prerender1) {
        g_scene_prerender1->release();
        g_scene_prerender1 = 0;
    }
    if (g_render_object_65966c) {
        g_render_object_65966c->release();
        g_render_object_65966c = 0;
    }
    if (g_render_object_659678) {
        g_render_object_659678->release();
        g_render_object_659678 = 0;
    }
    if (g_blit_material) {
        g_blit_material->release();
        g_blit_material = 0;
    }
    if (g_mouse_surface) {
        g_mouse_surface->release();
        g_mouse_surface = 0;
    }
    if (g_primary_color_surface) {
        g_primary_color_surface->release();
        g_primary_color_surface = 0;
    }
}

/* Derives the 16-bit channel masks and their leading-bit positions from the
   renderer's reviewed pixel-format selector.  The startup configuration uses
   format 9, RGB 5:5:5 with the high bit reserved. */
// FUNCTION: WIZ8 0x004265c0
void Initialize16BitPixelFormatMasks(void)
{
    unsigned short bit;

    if (g_pixel_format == 7) {
        gusAlphaMask = 0;
        gusRedMask = 0xf800;
        gusGreenMask = 0x07e0;
        g_color_key = 0x7bef;
    } else if (g_pixel_format == 8) {
        gusAlphaMask = 0;
        gusRedMask = 0x7c00;
        gusGreenMask = 0x03e0;
        g_color_key = 0x3def;
    } else if (g_pixel_format == 9) {
        gusAlphaMask = 0x8000;
        gusRedMask = 0x7c00;
        gusGreenMask = 0x03e0;
        g_color_key = 0x3def;
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
    window_class.style = CS_VREDRAW | CS_HREDRAW | CS_NOCLOSE | CS_DBLCLKS;
    window_class.lpfnWndProc = g_window_proc;
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
    g_window_rect.left = GetSystemMetrics(SM_CXSCREEN) / 2 - extent / 2;
    extent = GetSystemMetrics(SM_CXSCREEN);
    if (extent > 640) {
        extent = 640;
    }
    g_window_rect.top = GetSystemMetrics(SM_CXSCREEN) / 2 - extent / 2;
    extent = GetSystemMetrics(SM_CXSCREEN);
    if (extent > 640) {
        extent = 640;
    }
    g_window_rect.right = g_window_rect.left + extent;
    extent = GetSystemMetrics(SM_CYSCREEN);
    if (extent > 480) {
        extent = 480;
    }
    g_window_rect.bottom = g_window_rect.top + extent;

    if (!g_fullscreen) {
        style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        AdjustWindowRect(&g_window_rect, style, FALSE);
        ghWindow = CreateWindowExA(0, "Wizardry 8", "Wizardry 8", style, g_window_rect.left,
                                   g_window_rect.top, g_window_rect.right - g_window_rect.left,
                                   g_window_rect.bottom - g_window_rect.top, NULL, NULL,
                                   g_instance_654ac4, NULL);
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

    result = DirectDrawCreate(NULL, &g_direct_draw, NULL);
    if (FAILED(result)) {
        return 0;
    }
    result = g_direct_draw->QueryInterface(
        IID_IDirectDraw2,
        // reinterpret-ok: COM QueryInterface returns the interface through void**
        reinterpret_cast<void**>(&g_direct_draw2));
    if (FAILED(result)) {
        return 0;
    }
    result = g_direct_draw2->SetCooperativeLevel(NULL, DDSCL_NORMAL);
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

    result = g_direct_draw2->CreateSurface(&description, &g_primary_surface1, NULL);
    if (FAILED(result)) {
        return 0;
    }
    result = g_primary_surface1->QueryInterface(
        IID_IDirectDrawSurface2,
        // reinterpret-ok: COM QueryInterface returns the interface through void**
        reinterpret_cast<void**>(&g_primary_surface));
    if (FAILED(result)) {
        return 0;
    }

    DDLockSurface(g_primary_surface, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface, NULL);
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

    if (g_gerd) {
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
            g_screen_width = atoi(line);
        }
        if (fgets(line, sizeof(line), config)) {
            g_screen_height = atoi(line);
        }
        if (fgets(line, sizeof(line), config)) {
            g_screen_depth = atoi(line);
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
    g_gerd = srGERD::loadDevice(devices, 0);
    _chdir("..");
    if (!g_gerd) {
        ShutdownWithErrorBox("Video device cannot be started. Please re-run 3DSetup.");
        return 0;
    }

    // reinterpret-ok: SurRender takes the window handle as an integer
    g_gerd->createContext(reinterpret_cast<unsigned long>(ghWindow));
    OpenRendererWindow();
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
unsigned char OpenRendererWindow(void)
{
    srGERD::e_error error;
    long mode;

    SetLastError(0);
    if (!g_fullscreen) {
        SetWindowLongA(ghWindow, GWL_STYLE,
                       WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
        SetWindowPos(ghWindow, NULL, g_window_rect.left, g_window_rect.top,
                     g_window_rect.right - g_window_rect.left,
                     g_window_rect.bottom - g_window_rect.top, 0);
        error = g_gerd->openWindow();
    } else {
        SetWindowLongA(ghWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowPos(ghWindow, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN),
                     GetSystemMetrics(SM_CYSCREEN), 0);
        mode = g_gerd->getDisplayMode(g_screen_width, g_screen_height, g_screen_depth);
        if (mode == -1) {
            ShutdownWithErrorBox("Video device does not support video resolution.");
            return 0;
        }
        SetEnvironmentVariableA("FX_GLIDE_NO_SPLASH", "1");
        error = g_gerd->openWindow(mode);
    }
    if (error != 0) {
        ShutdownWithErrorBox("Could not open video output device.");
        return 0;
    }
    g_flush_pending = true;
    return 1;
}

// FUNCTION: WIZ8 0x00423390
IDirectDrawSurface2* BeginVideoPresentation(void)
{
    DDSURFACEDESC description;

    if (g_gerd != 0) {
        g_flush_pending = false;
        g_gerd->closeWindow((srGERD::e_closeHint)1);
        g_gerd->deleteContext();
    }
    if (g_direct_draw2->SetCooperativeLevel(ghWindow, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) !=
        DD_OK) {
        NoOp();
        return 0;
    }
    if (g_direct_draw2->SetDisplayMode(640, 480, 16, 0, 0) != DD_OK) {
        NoOp();
        return 0;
    }
    memset(&description, 0, sizeof(description));
    description.dwSize = sizeof(description);
    description.dwFlags = DDSD_CAPS;
    description.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (g_direct_draw2->CreateSurface(&description, &g_video_primary_surface1, 0) != DD_OK) {
        NoOp();
        return 0;
    }
    if (g_video_primary_surface1->QueryInterface(
            IID_IDirectDrawSurface2,
            // reinterpret-ok: COM QueryInterface returns the interface through void**
            reinterpret_cast<void**>(&g_video_primary_surface2)) != DD_OK) {
        NoOp();
        return 0;
    }
    return g_video_primary_surface2;
}

// FUNCTION: WIZ8 0x004234A0
unsigned char FinishVideoPresentation(void)
{
    if (g_video_primary_surface1 != 0) {
        g_video_primary_surface1->Release();
        g_video_primary_surface1 = 0;
    }
    if (g_video_primary_surface2 != 0) {
        g_video_primary_surface2->Release();
        g_video_primary_surface2 = 0;
    }
    g_direct_draw2->SetCooperativeLevel(ghWindow, DDSCL_NORMAL);
    // reinterpret-ok: SurRender takes the window handle as an integer
    g_gerd->createContext(reinterpret_cast<unsigned long>(ghWindow));
    return OpenRendererWindow();
}

/* WM_SIZE only rebuilds the SurRender output in windowed mode.  Full-screen
   startup receives the same Windows notification while the device already
   owns its configured 640x480 mode, so there is no resize operation to do. */
// FUNCTION: WIZ8 0x00422550
unsigned char VideoResizeWindow(void)
{
    if (g_fullscreen || !ghWindow || !g_gerd || !g_flush_pending) {
        return 0;
    }
    g_flush_pending = false;
    g_gerd->closeWindow(static_cast<srGERD::e_closeHint>(1));
    if (g_gerd->openWindow() == static_cast<srGERD::e_error>(3)) {
        return 0;
    }
    g_paired_render_mode = 2;
    g_overlay_render_mode = 2;
    ResetTransientRenderScenes();
    g_flush_pending = true;
    return 1;
}

// FUNCTION: WIZ8 0x00422970
void VideoFullScreen(unsigned char enabled)
{
    g_fullscreen = enabled;
    if (ghWindow && g_gerd && g_flush_pending) {
        g_flush_pending = false;
        g_gerd->closeWindow(static_cast<srGERD::e_closeHint>(1));
        OpenRendererWindow();
    }
}

// FUNCTION: WIZ8 0x00422f10
void ResetTransientRenderScenes(void)
{
    memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
    unsigned int active = g_active_page;
    g_page_full_redraw[active ^ 1] = 0;
    g_page_full_redraw[active] = 0;
    g_dirty_tile_count = 0;
    PurgeInactiveSceneInstances(g_scene_prerender0);
    PurgeInactiveSceneInstances(g_scene_overlay0);
    PurgeInactiveSceneInstances(g_scene_prerender1);
    PurgeInactiveSceneInstances(g_scene_overlay1);
    InvalidateRegion(0, 0, 640, 480, 0);
}

/* Same dirty-block / transient-scene reset as ResetTransientRenderScenes, but
   the full-screen invalidate uses flag 1 (the all-bits redraw path). */
// FUNCTION: WIZ8 0x00423150
void ClearVideoDirtyBlocks(void)
{
    memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
    unsigned int active = g_active_page;
    g_page_full_redraw[active ^ 1] = 0;
    g_page_full_redraw[active] = 0;
    g_dirty_tile_count = 0;
    PurgeInactiveSceneInstances(g_scene_prerender0);
    PurgeInactiveSceneInstances(g_scene_overlay0);
    PurgeInactiveSceneInstances(g_scene_prerender1);
    PurgeInactiveSceneInstances(g_scene_overlay1);
    InvalidateRegion(0, 0, 640, 480, 1);
}

// FUNCTION: WIZ8 0x004277e0
unsigned char VideoInspectorIsEnabled(void)
{
    return g_video_inspector_enabled;
}

// FUNCTION: WIZ8 0x00422050
void SuspendVideoManager(void)
{
    if (g_video_active) {
        PauseMainGameWorld();
        g_video_active = 0;
        if (g_gerd) {
            g_flush_pending = false;
            g_gerd->closeWindow((srGERD::e_closeHint)0);
        }
        if (!g_fullscreen) {
            GetWindowRect(ghWindow, &g_window_rect);
        }
        ShowWindow(ghWindow, SW_MINIMIZE);
        FreeMouseCursor();
    }
}

// FUNCTION: WIZ8 0x004220b0
unsigned char RestoreVideoManager(void)
{
    if (g_video_active) {
        return 1;
    }
    if (gXStatus.world_update_blocked) {
        ResumeMainGameWorld();
    }
    if (ghWindow && g_gerd) {
        g_video_active = 1;
        ShowWindow(ghWindow, SW_RESTORE);
        if (g_gerd->isWindowOpen() != 0 || OpenRendererWindow()) {
            OpenIcon(ghWindow);
            SetFocus(ghWindow);
            memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
            g_dirty_tile_count = 0;
            g_page_full_redraw[g_active_page ^ 1] = 0;
            g_page_full_redraw[g_active_page] = 0;
            PurgeInactiveSceneInstances(g_scene_prerender0);
            PurgeInactiveSceneInstances(g_scene_overlay0);
            PurgeInactiveSceneInstances(g_scene_prerender1);
            PurgeInactiveSceneInstances(g_scene_overlay1);
            InvalidateRegion(0, 0, 0x280, 0x1e0, 0);
            g_paired_render_mode = 2;
            g_overlay_render_mode = 2;
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
    DDUnlockSurface(g_primary_surface, NULL);
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

    DDLockSurface(g_primary_surface, NULL, &description, 0, NULL);
    memset(description.lpSurface, 0, description.lPitch * 480);
    DDUnlockSurface(g_primary_surface, NULL);
    return 1;
}

void UpdateRenderElapsedTime(void);

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
#ifdef WIZ8_RUNTIME_TESTS
static RuntimeWorldRenderData ObserveWorldRenderState();
#endif

// FUNCTION: WIZ8 0x00427850
void RenderScene(srScene* scene, srCamera* camera, const int* viewport, char preserve_fog)
{
    unsigned long width = g_gerd->getWidth();
    unsigned long height = g_gerd->getHeight();

    if (scene == 0 || scene->getChildCount() == 0) {
        return;
    }
    if (viewport != 0) {
        unsigned long left = viewport[0] * width / 640;
        unsigned long top = viewport[1] * height / 480;
        unsigned long viewport_width = (viewport[2] - viewport[0]) * width / 640;
        unsigned long viewport_height = (viewport[3] - viewport[1]) * height / 480;
        g_gerd->setViewPort(left, top, viewport_width, viewport_height);
        g_gerd->setScissor(left, top, viewport_width, viewport_height);
#ifdef WIZ8_RUNTIME_TESTS
        if (g_world != 0 && scene == g_world->static_scene) {
            RuntimeWorldRenderData data = ObserveWorldRenderState();
            data.applied_viewport[0] = left;
            data.applied_viewport[1] = top;
            data.applied_viewport[2] = viewport_width;
            data.applied_viewport[3] = viewport_height;
            RuntimeObserveWorld(RUNTIME_WORLD_VIEWPORT_APPLIED, data);
        }
#endif
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
    scene->render(*g_gerd, camera);
    g_gerd->flushRenderers();
    if (viewport != 0) {
        g_gerd->setViewPort(0, 0, width, height);
        g_gerd->setScissor(0, 0, width, height);
    }
}

#ifdef WIZ8_RUNTIME_TESTS
static RuntimeWorldRenderData ObserveWorldRenderState()
{
    RuntimeWorldRenderData data;
    memset(&data, 0, sizeof(data));
    data.level = g_status.current_level;
    data.flags = (g_world_render_enabled ? 1UL : 0UL) | (g_world_blacked_out ? 2UL : 0UL) |
                 (g_video_active ? 4UL : 0UL) | (g_monster_shadow_updates_enabled ? 8UL : 0UL) |
                 (g_render_flag_603c6c ? 16UL : 0UL) | (g_world_pick_enabled ? 32UL : 0UL);
    data.scene_children = g_world->static_scene->getChildCount();
    if (g_world->octree != 0 && g_world->psrMeshes != 0) {
        for (unsigned long index = 0; index < g_world->octree->m_meshCount_1b4; ++index) {
            srModelInstance* instance = g_world->psrMeshes[index];
            if (instance != 0 && !instance->testFlag(srNode::FLAG_DISABLE)) {
                ++data.visible_meshes;
            }
        }
    }
    data.viewport[0] = g_viewport_6595e8.left;
    data.viewport[1] = g_viewport_6595e8.top;
    data.viewport[2] = g_viewport_6595e8.right;
    data.viewport[3] = g_viewport_6595e8.bottom;
    data.renderer_size[0] = g_gerd->getWidth();
    data.renderer_size[1] = g_gerd->getHeight();
    srVector3T<float> camera;
    GetCameraPosition(&camera);
    data.camera[0] = camera.x;
    data.camera[1] = camera.y;
    data.camera[2] = camera.z;
    return data;
}
#endif

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
    if (!g_video_active) {
        return;
    }

    if (g_trigger_action_active && GetWorld() != 0) {
        GetCameraPosition(&saved_world_position);
        shifted_world_position.x = saved_world_position.x + g_trigger_action_scene_offset.x;
        shifted_world_position.y = saved_world_position.y + g_trigger_action_scene_offset.y;
        shifted_world_position.z = saved_world_position.z + g_trigger_action_scene_offset.z;
        SetWorldScenePosition(GetWorld(), &shifted_world_position);
    }
    if (g_world != 0) {
        if (!IsSkyEnabled()) {
            EnvironmentColour ambient_light;
            GetWorldLightValue(g_world, &ambient_light);
            clear_color.x = ambient_light.red;
            clear_color.y = ambient_light.green;
            clear_color.z = ambient_light.blue;
        } else {
            clear_color.SetSaturated(g_world->static_scene->getFogColor());
        }
    }

    g_gerd->setClearColor(clear_color.x, clear_color.y, clear_color.z, 1.0f);
    SyncSystemCursor();
    RenderFastHelp();
    UpdateRegionHelp();
    FlushDirtyTiles();
    if (g_gerd != 0) {
        g_gerd->resetStatistics();
    }
    srCore.getStatisticsManager()->reset();
    g_gerd->beginFrame();
#ifdef WIZ8_RUNTIME_TESTS
    RuntimeObserve(RUNTIME_FRAME_BEGIN, g_fps_frame_count, g_current_screen_state.id,
                   g_video_active);
#endif
    UpdateRenderElapsedTime();

    if (!g_monster_shadow_updates_enabled) {
        if (g_world_blacked_out) {
            goto clear_viewport;
        }
    } else if (g_world_blacked_out || !g_render_flag_603c6c || g_world_659ab8 == 0) {
    clear_viewport: {
        unsigned long height = g_gerd->getHeight();
        unsigned long width = g_gerd->getWidth();
        g_gerd->setScissor(g_viewport_6595e8.left * width / 640,
                           g_viewport_6595e8.top * height / 480,
                           (g_viewport_6595e8.right - g_viewport_6595e8.left) * width / 640,
                           (g_viewport_6595e8.bottom - g_viewport_6595e8.top) * height / 480);
        g_gerd->clear(srFlags<srGERD::e_buffer>(3));
        g_gerd->setScissor(0, 0, width, height);
    }
    }

    first_page = g_active_page ? g_scene_prerender0 : g_scene_prerender1;
    second_page = g_active_page ? g_scene_prerender1 : g_scene_prerender0;
    RenderScene(first_page, g_overlay_camera, 0, 0);
    RenderScene(second_page, g_overlay_camera, 0, 0);
    g_gerd->setTextureReduction(g_resident_texture_policy);

    if (g_render_flag_603c6c && g_world_659ab8 != 0 && g_monster_shadow_updates_enabled) {
        RenderScene(g_world_659ab8->static_scene, g_world_659ab8->camera, &g_viewport_6595e8.left,
                    0);
    }
    if (g_world != 0 && g_world_render_enabled) {
#ifdef WIZ8_RUNTIME_TESTS
        RuntimeWorldRenderData world_observation = ObserveWorldRenderState();
        RuntimeObserveWorld(RUNTIME_WORLD_RENDER_BEGIN, world_observation);
#endif
        g_gerd->setTextureReduction(g_resident_texture_policy);
        if (!g_world_pick_enabled || g_cursor_hotspot_x + g_cursor_width < g_viewport_6595e8.left ||
            g_cursor_hotspot_y + g_cursor_height < g_viewport_6595e8.top ||
            g_viewport_6595e8.right < g_cursor_hotspot_x + g_cursor_width ||
            g_viewport_6595e8.bottom < g_cursor_hotspot_y + g_cursor_height) {
            RenderScene(g_world->static_scene, g_world->camera, &g_viewport_6595e8.left, 1);
        } else {
            int half_width = (g_viewport_6595e8.right - g_viewport_6595e8.left) / 2;
            int half_height = (g_viewport_6595e8.bottom - g_viewport_6595e8.top) / 2;
            srGERD::Pick pick;
            pick.position_00.x =
                (g_cursor_hotspot_x - half_width - g_viewport_6595e8.left + g_cursor_width) /
                static_cast<float>(half_width);
            pick.position_00.y = -static_cast<float>(g_cursor_hotspot_y - half_height -
                                                     g_viewport_6595e8.top + g_cursor_height) /
                                 half_height;
            pick.position_00.z = 1.0f;
            pick.selected_model_0c = 0;
            pick.value_10 = 0;
            g_gerd->setPickKey(0);
            g_gerd->pushPick(pick);
            RenderScene(g_world->static_scene, g_world->camera, &g_viewport_6595e8.left, 1);
            g_gerd->popPick(pick);
            g_current_model_instance = pick.selected_model_0c;
            ResolvePickedProp(g_world);
        }
#ifdef WIZ8_RUNTIME_TESTS
        RuntimeObserveWorld(RUNTIME_WORLD_RENDER_END, world_observation);
#endif
    }

    g_gerd->setTextureReduction(0);
#ifdef WIZ8_RUNTIME_TESTS
    if (getenv("WIZ8_RUNTIME_HIDE_OVERLAY") == 0)
#endif
        if (g_overlay_scenes_enabled) {
            const int* overlay_viewport = g_overlay_viewport;
            if (!g_fullscreen_scene_last) {
                RenderScene(g_scene_fullscreen, g_overlay_camera, overlay_viewport, 0);
            }
            RenderScene(g_scene_overlay0, g_overlay_camera, 0, 0);
            RenderScene(g_scene_user, g_overlay_camera, 0, 0);
            RenderScene(g_scene_square, g_square_camera, overlay_viewport, 0);
            if (g_fullscreen_scene_last) {
                RenderScene(g_scene_fullscreen, g_overlay_camera, overlay_viewport, 0);
            }
            if (g_cursor_scene_enabled) {
                RenderScene(g_cursor_scene, g_overlay_camera, 0, 0);
            }
        }
    g_gerd->endFrame();
#ifdef WIZ8_RUNTIME_TESTS
    RuntimeObserve(RUNTIME_FRAME_SUBMITTED, g_fps_frame_count, g_current_screen_state.id, 0);
#endif

    if (g_screenshot_pending) {
        SaveJpegScreenshot();
        g_screenshot_pending = 0;
    }
    if (g_auto_capture) {
        now = GetTickCount();
        if (now < g_last_capture_tick || g_last_capture_tick + g_frame_reset_interval < now) {
            g_screenshot_pending = 1;
            g_last_capture_tick = now;
        }
    }

    next_page = g_active_page ^ 1;
    g_active_page = next_page;
    g_overlay_page_counters[next_page] = 0;
    g_page_full_redraw[next_page] = 0;
    ++g_fps_frame_count;
    retire_prerender = next_page ? g_scene_prerender1 : g_scene_prerender0;
    retire_overlay = next_page ? g_scene_overlay1 : g_scene_overlay0;
    PurgeInactiveSceneInstances(retire_prerender);
    PurgeInactiveSceneInstances(retire_overlay);

    now = GetTickCount();
    elapsed = static_cast<float>(now - g_fps_window_tick);
    frames_per_second = g_fps_frame_count / elapsed * 1000.0f;
    if (g_fps_frame_count > 50) {
        g_fps_window_tick = GetTickCount();
        g_fps_frame_count = 0;
    }
    g_seconds_per_frame = 1.0f / frames_per_second;
    g_frames_per_second = frames_per_second;

    if (g_trigger_action_active && GetWorld() != 0) {
        SetWorldScenePosition(GetWorld(), &saved_world_position);
    }
}

/* Render the world into the locked frame buffer and copy the pixels onto a
   caller-owned color surface. The secondary render device (the second Voodoo
   board) takes over when present; `rect` selects a logical 640x480 viewport,
   otherwise the stored game viewport globals apply. SaveGame uses it for the
   SHOT screenshot; the automap uses it for its backdrop. */
// FUNCTION: WIZ8 0x00426f80
unsigned char RenderWorldToSurface00426F80(srColorSurface* target, W8ScreenRect* rect,
                                           char render_secondary)
{
    srGERD* gerd = g_secondary_gerd;
    EnvironmentColour clear_color;
    clear_color.red = 1.0f;
    clear_color.green = 1.0f;
    clear_color.blue = 1.0f;
    if (gerd == 0) {
        gerd = g_gerd;
    }
    gerd->beginFrame();
    gerd->setTextureReduction(g_resident_texture_policy);
    gerd->setScissor(0, 0, gerd->getWidth(), gerd->getHeight());
    if (rect != 0) {
        gerd->setViewPort(rect->left * gerd->getWidth() / 640, rect->top * gerd->getHeight() / 480,
                          (rect->right - rect->left) * gerd->getWidth() / 640,
                          (rect->bottom - rect->top) * gerd->getHeight() / 480);
    } else {
        gerd->setViewPort(
            g_viewport_6595e8.left * gerd->getWidth() / 640,
            g_viewport_6595e8.top * gerd->getHeight() / 480,
            (g_viewport_6595e8.right - g_viewport_6595e8.left) * gerd->getWidth() / 640,
            (g_viewport_6595e8.bottom - g_viewport_6595e8.top) * gerd->getHeight() / 480);
    }
    if (IsFogEnabled()) {
        GetLightDirection(&clear_color);
    } else {
        GetWorldLightValue(g_world, &clear_color);
    }
    gerd->setClearColor(clear_color.red, clear_color.green, clear_color.blue, 1.0f);
    if (g_inverted_depth_render != 0) {
        gerd->setClearDepth(0.0);
    }
    gerd->clear(srFlags<srGERD::e_buffer>(3));
    if (render_secondary != 0 && g_render_flag_603c6c != 0 && g_world_659ab8 != 0) {
        g_world_659ab8->static_scene->render(*gerd, g_world_659ab8->camera);
    }
    g_world->static_scene->render(*gerd, g_world->camera);
    gerd->endFrame();
    if (g_inverted_depth_render != 0) {
        gerd->setClearDepth(1.0);
    }
    gerd->flushRenderers();
    gerd->setTextureReduction(0);
    if (rect != 0) {
        gerd->setViewPort(0, 0, gerd->getWidth(), gerd->getHeight());
    }
    srColorSurfaceIFace* buffer = gerd->lockBuffer();
    if (buffer != 0) {
        target->setFilter(&srBoxFilter);
        target->copy(*buffer);
        gerd->unlockBuffer();
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00427440
void InvalidateRendererTextureCache(void)
{
    if (g_gerd != 0) {
        g_gerd->invalidateTextureCache();
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
    if (g_gerd != 0) {
        g_gerd->flush();
        g_gerd->setFogColor(*color);
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
    if (g_gerd != 0) {
        if ((!enabled && g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4)) ||
            (enabled && !g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4))) {
            g_gerd->toggle(srGERD::ENABLE_POSITIONAL_4);
        }
    }
}

/* The static scene's fog colour clamped into 0..1 for the caller; an empty
   world reports black. */
// FUNCTION: WIZ8 0x00427290
void GetWorldColour(EnvironmentColour* colour)
{
    if (g_world != 0) {
        const srVector3T<float> fog = g_world->static_scene->getFogColor();
        colour->red = fog.x;
        colour->green = fog.y;
        colour->blue = fog.z;
        if (colour->red <= 0.0f) {
            colour->red = 0.0f;
        } else if (colour->red >= 1.0f) {
            colour->red = 1.0f;
        }
        if (colour->green <= 0.0f) {
            colour->green = 0.0f;
        } else if (colour->green >= 1.0f) {
            colour->green = 1.0f;
        }
        if (colour->blue <= 0.0f) {
            colour->blue = 0.0f;
        } else if (colour->blue >= 1.0f) {
            colour->blue = 1.0f;
        }
        return;
    }
    colour->red = 0.0f;
    colour->green = 0.0f;
    colour->blue = 0.0f;
}

// FUNCTION: WIZ8 0x00428e20
int GetUsedPageFileBytes(void)
{
    MEMORYSTATUS status;
    memset(&status, 0, sizeof(status));
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPageFile - status.dwAvailPageFile;
}

// FUNCTION: WIZ8 0x00427260
bool RendererBufferIsLockable(void)
{
    srColorSurfaceIFace* surface = g_gerd->lockBuffer();
    if (surface != 0) {
        g_gerd->unlockBuffer();
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x00427830
void SetWorldModelPickingEnabled(char enabled)
{
    g_world_pick_enabled = enabled;
    if (enabled == 0) {
        g_current_model_instance = 0;
    }
}

/* Mouse cursor scene and rendering. */

// FUNCTION: WIZ8 0x00424EB0
srModelInstance* MakePolygonBrush(srNode* parent, srColorSurfaceIFace* surface, double width,
                                  double height, float mapping_x, float mapping_y,
                                  float mapping_width, float mapping_height, unsigned char overlay)
{
    srMeshModel* model;
    srTextureMap* texture;
    stModelInstance2D* instance;
    srVector3T<float> scale;
    srShader shader;

    model = SR_NEW(srMeshModel)(0L, 0L);
    if (!model) {
        return 0;
    }
    model->autoRelease();
    model->setName("Video2DMakePolygonBrush");

    g_modeler_65963c->createGrid(1, 1);
    srModeler::MappingInfo mapping(srModeler::AXIS_X, srModeler::AXIS_Y, mapping_width,
                                   mapping_height, mapping_x, mapping_y);
    g_modeler_65963c->planarMap(0, 0, mapping);
    scale.x = static_cast<float>(width);
    scale.y = static_cast<float>(height);
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    shader.value = overlay ? g_surface_state_654ad8 : g_surface_state_6595dc;
    if (!surface) {
        shader.value &= ~srShader::MASK_TEXTURING;
    } else {
        texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
        texture->autoRelease();
        texture->setName("Video2DMakePolygonBrush");
        texture->setSurfacePtr(surface);
        texture->setCorrection(srTextureIFace::CORRECTION_FASTEST);
        texture->setMagFilter(srTextureIFace::FILTER_BEST);
        texture->setMinFilter(srTextureIFace::FILTER_BEST);
        texture->setMipmap(srTextureIFace::MIPMAP_NONE);
        texture->setWrapS(srTextureIFace::WRAP_CLAMP);
        texture->setWrapT(srTextureIFace::WRAP_CLAMP);
        model->setMaterial(g_blit_material, 0, static_cast<srMeshModel::e_side>(0));
        model->setTexture(texture, 0, 0);
        texture->enableHint(overlay ? srTextureIFace::HINT_POSITIONAL_2
                                    : srTextureIFace::HINT_POSITIONAL_1);
        texture->enableHint(srTextureIFace::HINT_POSITIONAL_3);
    }
    model->setShader(shader, 0);

    instance = new stModelInstance2D(parent);
    instance->setName("Video2DMakePolygonBrush");
    instance->SetModel(model);
    instance->configure2D(static_cast<short>(width * 640.0), static_cast<short>(height * 480.0));
    return instance;
}

// FUNCTION: WIZ8 0x00425190
stModelInstance2D* CreateSpriteFromTexture(srTextureIFace* texture, double width, double height,
                                           char keep_aspect, char a5)
{
    srShader shader;
    srVector3T<float> scale;
    int w = static_cast<int>(width * 640.0);
    int h = static_cast<int>(height * 480.0);

    srMeshModel* model = SR_NEW(srMeshModel)(0L, 0L);
    if (!model) {
        return 0;
    }
    model->autoRelease();
    model->setName("Video2DMakePolygonBrush");

    float step = g_surface_scale * (g_float_005ebb38 / w);
    g_modeler_65963c->createGrid(1, 1);
    srModeler::MappingInfo mapping(srModeler::AXIS_X, srModeler::AXIS_Y,
                                   g_float_005ebb38 - (step + step),
                                   g_float_005ebb38 - (step + step), step, step);
    g_modeler_65963c->planarMap(0, 0, mapping);
    scale.x = static_cast<float>(width);
    scale.y = static_cast<float>(height);
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    shader.value = keep_aspect ? g_surface_state_654ad8 : g_surface_state_6595dc;
    if (!texture) {
        shader.value &= ~srShader::MASK_TEXTURING;
    } else {
        model->setMaterial(g_blit_material, 0, static_cast<srMeshModel::e_side>(0));
        model->setTexture(texture, 0, 0);
    }
    model->setShader(shader, 0);

    stModelInstance2D* instance = new stModelInstance2D(0);
    if (instance) {
        instance->render_state_164.left = static_cast<short>(w);
        instance->render_state_164.top = static_cast<short>(h);
        instance->setName("Video2DMakePolygonBrush");
        instance->SetModel(model);
        if (a5) {
            instance->overlay_scene_flag_160 |= 1;
        }
    }
    return instance;
}

// FUNCTION: WIZ8 0x00484A40
BOOLEAN BlitVideoObjectToColorSurface(UINT32 video_object, UINT16 region,
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

/* Same blit as BlitVideoObjectToColorSurface with an already-resolved handle.
   Returns TRUE whenever the ETRLE properties lookup succeeds; the buffer blit
   result is not forwarded. */
// FUNCTION: WIZ8 0x00484AC0
BOOLEAN BlitHVObjectToColorSurface(HVOBJECT object, UINT16 region, srColorSurface* destination,
                                   int x, int y)
{
    ETRLEObject properties;

    if (!GetVideoObjectETRLEProperties(object, &properties, region)) {
        return FALSE;
    }
    BltVideoObjectToBuffer(static_cast<UINT16*>(destination->getDataPtr()), destination->getPitch(),
                           object, region, x, y, VO_BLT_SRCTRANSPARENCY, 0);
    return TRUE;
}

static void MoveSystemCursor(int x, int y)
{
    RECT client;
    POINT top_left;
    POINT bottom_right;

    if (!g_fullscreen) {
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

    if (g_cursor_image_width == width && g_cursor_image_height == height) {
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
    if (!g_mouse_surface->resize(extent, extent)) {
        return FALSE;
    }
    if (g_cursor_node_659694) {
        g_cursor_node_659694->release();
    }
    if (g_cursor_texture) {
        g_cursor_texture->release();
    }
    mapping_scale = g_surface_scale / extent;
    g_cursor_node_659694 =
        MakePolygonBrush(g_cursor_scene, g_mouse_surface, extent / 640.0, extent / 480.0,
                         mapping_scale, mapping_scale, 1.0f, 1.0f, 1);
    g_cursor_node_659694->setName("MouseResize");
    PositionMouseCursor(g_cursor_width, g_cursor_height, 0);
    g_cursor_model = static_cast<srMeshModel*>(g_cursor_node_659694->model());
    g_cursor_model->enableStartupControls();
    static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(0xc7c35000);
    g_cursor_model->enableStartupControls();
    g_cursor_model->setName("Mouse Cursor Mesh");
    g_cursor_texture = static_cast<srTexture*>(g_cursor_model->getTexture(0, 0));
    g_cursor_texture->setWrapS(srTextureIFace::WRAP_CLAMP);
    g_cursor_texture->setWrapT(srTextureIFace::WRAP_CLAMP);
    g_cursor_texture->addReference();
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

    if (g_cursor_hotspot_x != hotspot_x || g_cursor_hotspot_y != hotspot_y) {
        x = g_cursor_width - hotspot_x + g_cursor_hotspot_x;
        y = g_cursor_height - hotspot_y + g_cursor_hotspot_y;
        MoveSystemCursor(x, y);
        SyncSystemCursor();
        g_cursor_hotspot_x = hotspot_x;
        g_cursor_hotspot_y = hotspot_y;
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
    if (g_cursor_hotspot_x != offset_x || g_cursor_hotspot_y != offset_y) {
        x = g_cursor_width - offset_x + g_cursor_hotspot_x;
        y = g_cursor_height - offset_y + g_cursor_hotspot_y;
        MoveSystemCursor(x, y);
        PositionMouseCursor(x, y, 1);
        g_cursor_hotspot_x = offset_x;
        g_cursor_hotspot_y = offset_y;
    }
    g_cursor_image_width = properties.usWidth;
    g_cursor_image_height = properties.usHeight;
    g_mouse_surface->fill(0);
    return BlitVideoObjectToColorSurface(video_object, region, g_mouse_surface, 0, 0);
}

// FUNCTION: WIZ8 0x00427fc0
void BlitToMouseCursor(UINT32 video_object, UINT16 region, UINT16 x, UINT16 y)
{
    BlitVideoObjectToColorSurface(video_object, region, g_mouse_surface, x, y);
}

// FUNCTION: WIZ8 0x00427ff0
void RefreshMouseCursorTexture(void)
{
    g_mouse_surface->touch();
    g_cursor_texture->invalidate();
}

/* Whether the tracked cursor position lies inside the render viewport. The
   two automap callers use it, but nothing names the viewport as automap-only
   state; placed here with the neighbouring cursor bodies. */
// FUNCTION: WIZ8 0x00427e70
bool ClearMouseSurface(void)
{
    g_mouse_surface->fill(0);
    return true;
}

/* Distinct from IsCursorInsideViewport: the whole cursor image must fit
   inside the viewport, not just the hotspot point. */
// FUNCTION: WIZ8 0x00428030
bool IsCursorImageInsideViewport(void)
{
    if (g_cursor_width >= g_viewport_6595e8.left && g_cursor_height >= g_viewport_6595e8.top &&
        g_cursor_width + g_cursor_image_width <= g_viewport_6595e8.right &&
        g_cursor_height + g_cursor_image_height <= g_viewport_6595e8.bottom) {
        return true;
    }
    return false;
}

// FUNCTION: WIZ8 0x00428070
bool IsCursorInsideViewport(void)
{
    int x = g_cursor_hotspot_x + g_cursor_width;
    int y = g_cursor_hotspot_y + g_cursor_height;
    return x >= g_viewport_6595e8.left && y >= g_viewport_6595e8.top &&
           x <= g_viewport_6595e8.right && y <= g_viewport_6595e8.bottom;
}

// FUNCTION: WIZ8 0x004280c0
void WarpSystemCursor(int x, int y)
{
    if (g_fullscreen) {
        SetCursorPos(x, y);
        return;
    }
    RECT client;
    GetClientRect(ghWindow, &client);
    // reinterpret-ok: Win32 ClientToScreen takes LPPOINT; RECT is two adjacent POINTs
    ClientToScreen(ghWindow, reinterpret_cast<LPPOINT>(&client));
    // reinterpret-ok: Win32 ClientToScreen takes LPPOINT; RECT is two adjacent POINTs
    ClientToScreen(ghWindow, reinterpret_cast<LPPOINT>(&client.right));
    SetCursorPos(client.left + x, client.top + y);
}

// FUNCTION: WIZ8 0x00428140
void PositionMouseCursor(int width, int height, unsigned char reset_tick)
{
    srVector3T<double> location;

    if (g_mouse_surface) {
        g_cursor_width = width < 641 ? width : 640;
        g_cursor_height = height < 481 ? height : 480;
        if (g_cursor_node_659694) {
            location.x = g_cursor_width / 640.0 + g_mouse_surface->getWidth() / 1280.0;
            location.y = 1.0 - g_cursor_height / 480.0 - g_mouse_surface->getHeight() / 960.0;
            location.z = 0.0;
            g_cursor_node_659694->setLocation(location);
            if (reset_tick) {
                g_cursor_move_tick = GetTickCount();
            }
        }
    }
}

/* Milliseconds since the cursor last moved (or was repositioned with the
   reset-tick arm). */
// FUNCTION: WIZ8 0x00428220
unsigned int GetMillisecondsSinceCursorMove(void)
{
    return GetTickCount() - g_cursor_move_tick;
}

/* The tracked cursor as viewport-relative 0..1 coordinates, or zero when it
   lies outside. Same tracked position and viewport as the query above. */
// FUNCTION: WIZ8 0x00428230
unsigned char GetCursorPositionInViewport(srVector3T<float>* position)
{
    int x = g_cursor_hotspot_x + g_cursor_width;
    int y = g_cursor_hotspot_y + g_cursor_height;
    if (x >= g_viewport_6595e8.left && y >= g_viewport_6595e8.top && x <= g_viewport_6595e8.right &&
        y <= g_viewport_6595e8.bottom) {
        position->x = (x - g_viewport_6595e8.left) /
                      static_cast<float>(g_viewport_6595e8.right - g_viewport_6595e8.left);
        position->y = (y - g_viewport_6595e8.top) /
                      static_cast<float>(g_viewport_6595e8.bottom - g_viewport_6595e8.top);
        position->z = 0.0f;
        return 1;
    }
    return 0;
}

/* The tracked cursor tip (hotspot + size) in pixel-scale units. DropHeldItem
   turns it into the toss direction. */
// FUNCTION: WIZ8 0x004282F0
void GetCursorScaledPosition(srVector3T<float>* position)
{
    position->x = (g_cursor_hotspot_x + g_cursor_width) * g_scale_x_5ebb1c;
    position->z = 0.0f;
    position->y = (g_cursor_hotspot_y + g_cursor_height) * g_scale_y_5ebb20;
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
    if (!g_fullscreen) {
        GetClientRect(ghWindow, &client);
        top_left.x = client.left;
        top_left.y = client.top;
        bottom_right.x = client.right;
        bottom_right.y = client.bottom;
        ClientToScreen(ghWindow, &top_left);
        ClientToScreen(ghWindow, &bottom_right);
        if (cursor.x < top_left.x || cursor.x >= bottom_right.x || cursor.y < top_left.y ||
            cursor.y >= bottom_right.y) {
            if (g_system_cursor_visible != 1) {
                g_system_cursor_visible = true;
                ShowCursor(TRUE);
            }
            return;
        }
        cursor.x -= top_left.x;
        cursor.y -= top_left.y;
        if (cursor.x != g_cursor_width || cursor.y != g_cursor_height) {
            PositionMouseCursor(cursor.x, cursor.y, 1);
        }
        if (g_system_cursor_visible != 0) {
            g_system_cursor_visible = false;
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
    if (cursor.x != g_cursor_width || cursor.y != g_cursor_height) {
        PositionMouseCursor(cursor.x, cursor.y, 1);
        if (!g_fullscreen) {
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
        point->x = g_cursor_hotspot_x + g_cursor_width;
        point->y = g_cursor_hotspot_y + g_cursor_height;
    }
}

// FUNCTION: WIZ8 0x00428520
bool IsCursorInRectangle(int left, int top, int right, int bottom)
{
    POINT point;

    SGPMouseGetPos(&point);
    return point.x >= left && point.x <= right && point.y >= top && point.y <= bottom;
}

/* The atom's packed mouse position plus the cursor hotspot, split into the
   screen-space x and y region input works in. */
// FUNCTION: WIZ8 0x00428580
int GetAtomCursorX(const InputAtom* atom)
{
    return static_cast<unsigned short>(atom->uiParam) + g_cursor_hotspot_x;
}

// FUNCTION: WIZ8 0x004285a0
int GetAtomCursorY(const InputAtom* atom)
{
    return static_cast<int>(atom->uiParam >> 16) + g_cursor_hotspot_y;
}

/* Creates the shipped 128x128 mouse polygon inside its dedicated scene. */
// FUNCTION: WIZ8 0x004285c0
unsigned char InitializeMouseCursorScene(void)
{
    srScene* cursor_scene = SR_NEW(srScene)(static_cast<srNode*>(0));
    cursor_scene->setAmbientLight(0.0f, 0.0f, 0.0f);
    cursor_scene->setFogColor(0.0f, 0.0f, 0.0f);
    g_cursor_scene = cursor_scene;
    g_cursor_scene->setName("Mouse Cursor Scene");
    if (!g_mouse_surface) {
        return 0;
    }
    g_mouse_surface->fill(0);
    if (g_cursor_texture) {
        g_cursor_texture->release();
    }
    g_cursor_node_659694 =
        MakePolygonBrush(g_cursor_scene, g_mouse_surface, 0.2, 0.26666666666666666,
                         g_surface_scale / 128.0f, g_surface_scale / 128.0f, 1.0f, 1.0f, 1);
    if (g_cursor_node_659694) {
        g_cursor_node_659694->setName("MouseInit");
        g_cursor_model = static_cast<srMeshModel*>(g_cursor_node_659694->model());
        g_cursor_model->enableStartupControls();
        static_cast<stModelInstance2D*>(g_cursor_node_659694)->setRenderDepth(0xc7c35000);
        g_cursor_texture = static_cast<srTexture*>(g_cursor_model->getTexture(0, 0));
        g_cursor_texture->setWrapS(srTextureIFace::WRAP_CLAMP);
        g_cursor_texture->setWrapT(srTextureIFace::WRAP_CLAMP);
        g_cursor_texture->addReference();
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
 * each to InvalidateDirtyTile. A page already marked whole is skipped outright, and a
 * rectangle that turns out to cover the whole 640x480 marks it whole.
 *
 * InvalidateDirtyTile performs the per-cell write; the flag bits the caller passes
 * are only known by which bits they set.
 */

// GLOBAL: WIZ8 0x65970d
unsigned char g_world_render_enabled;
// GLOBAL: WIZ8 0x6596ea
bool g_viewport_tiles_dirty;
/* The initial full-screen invalidation runs before any 2D node occupies the
   tile table. A cell occupied by a 2D instance releases that instance and
   recursively invalidates the cells its extent covers; the flags the caller
   passes only mark this cell. */
/* Retire a 2D node: clear every tile-table slot that references it, invalidate
   the texture its model still exposes, and release the node itself. */
// FUNCTION: WIZ8 0x00425950
void ReleaseSurfaceNode(srNode* node)
{
    if (node == 0) {
        return;
    }
    int index = 0;
    for (srNode** slot = g_surface_nodes; slot < g_surface_nodes + 0x12c0; ++slot, ++index) {
        if (*slot == node) {
            *slot = 0;
            g_tile_dirty_flags[index] = 0;
        }
    }
    srMeshModel* model = static_cast<srMeshModel*>(static_cast<stModelInstance2D*>(node)->model());
    if (model != 0) {
        srTextureIFace* texture = model->getTexture(0, 0);
        if (texture != 0) {
            texture->invalidate();
        }
    }
    node->release();
}

// FUNCTION: WIZ8 0x004259b0
static void InvalidateDirtyTile(int cell, unsigned int flags)
{
    stModelInstance2D* node = static_cast<stModelInstance2D*>(g_surface_nodes[cell]);

    if (node != 0) {
        short position_x = node->render_state_164.right;
        short position_y = node->render_state_164.bottom;
        int columns = node->GetWidth00480EF0() >> 3;
        int rows = node->GetHeight00480F70() >> 3;

        for (int index = 0; index != 0x12c0; ++index) {
            if (g_surface_nodes[index] == node) {
                g_surface_nodes[index] = 0;
                g_tile_dirty_flags[index] = 0;
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
                InvalidateDirtyTile(row_cell, 0);
                ++row_cell;
            }
            start += 0x50;
        }
    }
    unsigned char state = g_tile_dirty_flags[cell] | static_cast<unsigned char>(flags) | 0x40;
    g_tile_dirty_flags[cell] = state;
    ++g_dirty_tile_count;
    int bottom = (cell / 0x50) * 8 + 8;
    int top = (cell / 0x50) * 8;
    int right = (cell % 0x50) * 8 + 8;
    int left = (cell % 0x50) * 8;
    if (g_world_render_enabled &&
        ((g_viewport_6595e8.left <= left && left <= g_viewport_6595e8.right) ||
         (g_viewport_6595e8.left <= right && right <= g_viewport_6595e8.right)) &&
        ((g_viewport_6595e8.top <= top && top <= g_viewport_6595e8.bottom) ||
         (g_viewport_6595e8.top <= bottom && bottom <= g_viewport_6595e8.bottom))) {
        g_tile_dirty_flags[cell] = state | 3;
        g_viewport_tiles_dirty = 1;
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
    if (g_page_full_redraw[g_active_page] == 0) {
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
                g_page_full_redraw[g_active_page] = 1;
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
                        InvalidateDirtyTile(static_cast<int>(x) / 8 + (top / 8) * 0x50, cell_flags);
                        x = x + 8;
                    } while ((int)x < (int)clipped_right);
                }
            }
        }
    }
}

/* Invalidate each rectangle in a run; a flagged region cancels the rest. */
// FUNCTION: WIZ8 0x00422ec0
void InvalidateScreenRects(W8ScreenRect* rects, unsigned int count, int flags)
{
    unsigned int index;

    for (index = 0; index < count; ++index) {
        if (g_page_full_redraw[g_active_page] != 0) {
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
void FlushDirtyTiles(void)
{
    DDSURFACEDESC description;

    if (g_dirty_tile_count == 0) {
        return;
    }
    DDLockSurface(g_primary_surface, 0, &description, 0, 0);
    for (int row = 0; row != 60; ++row) {
        int column = 0;
        while (column < 80) {
            int cell = row * 80 + column;
            if ((g_tile_dirty_flags[cell] & 0x40) == 0) {
                ++column;
                continue;
            }

            int width = 0;
            while (column + width < 80 && (g_tile_dirty_flags[cell + width] & 0x40) != 0) {
                ++width;
            }
            int height = 0;
            while (row + height < 60 && (g_tile_dirty_flags[cell + height * 80] & 0x40) != 0) {
                ++height;
            }

            g_surface_node->updateRectangle(g_gerd, description.lpSurface, description.lPitch,
                                            column * 8, row * 8, (column + width) * 8,
                                            (row + height) * 8);
            for (int y = 0; y != height; ++y) {
                for (int x = 0; x != width; ++x) {
                    g_tile_dirty_flags[cell + y * 80 + x] &= 0x3f;
                }
            }
            column += width;
        }
    }
    DDUnlockSurface(g_primary_surface, 0);
    g_dirty_tile_count = 0;
}

/* Viewport. */
/* Scale a 640x480 design-space rect onto the GERD viewport and remember it;
   no-ops when the stored bounds already match. */
// FUNCTION: WIZ8 0x00425C90
void SetScaledViewport00425C90(int left, int top, int right, int bottom)
{
    if (left == g_viewport_6595e8.left && top == g_viewport_6595e8.top &&
        right == g_viewport_6595e8.right && bottom == g_viewport_6595e8.bottom) {
        return;
    }
    g_gerd->setViewPort(g_gerd->getWidth() * left / 640, g_gerd->getHeight() * top / 480,
                        g_gerd->getWidth() * (right - left) / 640,
                        g_gerd->getHeight() * (bottom - top) / 480);
    g_viewport_6595e8.left = left;
    g_viewport_6595e8.top = top;
    g_viewport_6595e8.right = right;
    g_viewport_6595e8.bottom = bottom;
}

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

    if (g_gerd != 0 && g_flush_pending) {
        g_gerd->flush();
    }
    fractional_left = left * g_scale_x_5ebb1c;
    g_viewport_6595e8.right = right + 1;
    g_viewport_6595e8.left = left;
    g_viewport_6595e8.bottom = bottom + 1;
    fractional_top = top * g_scale_y_5ebb20;
    g_viewport_6595e8.top = top;
    fractional_right = g_viewport_6595e8.right * g_scale_x_5ebb1c;
    fractional_bottom = g_viewport_6595e8.bottom * g_scale_y_5ebb20;

    if (g_world != 0 && g_world->camera != 0) {
        g_world->camera->setViewPlane(3.14159265358979323846 * g_float_005ebcf8 * 85.0f,
                                      3.14159265358979323846 * g_float_005ebcf8 * 71.0f);
        g_world->camera->getViewPlane(view, depth);

        plane.left = fractional_left * (view.right - view.left) + view.left;
        plane.right = fractional_right * (view.right - view.left) + view.left;
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
        if (g_camera_sway_active) {
            SetCameraSwayMode(g_world->camera, 1);
        }
    }
}

// GLOBAL: WIZ8 0x00603c70
char g_video_config_file[260] = "3DVideo.CFG";

// FUNCTION: WIZ8 0x004229d0
void PrintScreen(void)
{
    g_screenshot_pending = 1;
}

/* The debug stats readout drawn over the primary surface: a black band plus
   the frame-rate line always, the object/poly/vertex counters, texture and
   pagefile memory in mode 2, or the scaled camera position in mode 3. */
// FUNCTION: WIZ8 0x00427460
void DrawVideoInspector(int left, unsigned int top)
{
    DDSURFACEDESC description;
    srGERD::Statistics statistics;
    MEMORYSTATUS memory_status;
    srVector3T<float> position;
    unsigned int bottom;
    unsigned char* row;
    int rows;
    /* Retail left the band height uninitialised for inspector modes outside
       1..3 and read that storage into bottom; deterministic zero models
       that defect path. */
    int height = 0;

    if (g_video_inspector_mode == 1) {
        height = 0xb;
    } else if (g_video_inspector_mode == 2) {
        height = 0x9a;
    } else if (g_video_inspector_mode == 3) {
        height = 0x2c;
    }
    bottom = top + height;
    DDLockSurface(g_primary_surface, NULL, &description, 0, NULL);
    if (description.lpSurface != 0) {
        if (top < bottom) {
            row = static_cast<unsigned char*>(description.lpSurface) + description.lPitch * top +
                  left * 2;
            for (rows = bottom - top; rows != 0; --rows) {
                memset(row, 0, 0x226);
                row += description.lPitch;
            }
        }
        DDUnlockSurface(g_primary_surface, NULL);
    }
    InvalidateRegion(left, top, left + 0x113, bottom, 0);
    if (g_gerd != 0) {
        g_gerd->getStatistics(statistics);
        SetFont(g_smfnt_font);
        SetFontObjectPalette16BPP(g_smfnt_font, g_font_state_palettes[5]);
        gprintfDirty(left, top, L"FR: %4.1f", g_frames_per_second);
        if (g_video_inspector_mode == 2) {
            gprintfDirty(left, top + 0xa, L"OC: %d", g_world->level->m_positional_13c);
            gprintfDirty(left, top + 0x14, L"PI: %d", statistics.value_34);
            gprintfDirty(left, top + 0x1e, L"PO: %d", statistics.value_20);
            gprintfDirty(left, top + 0x28, L"VI: %d", statistics.value_3c);
            gprintfDirty(left, top + 0x32, L"VO: %d", statistics.value_24);
            gprintfDirty(left, top + 0x3c, L"DD: %d", statistics.value_68);
            gprintfDirty(left, top + 0x46, L"TC: %d", statistics.texture_binds_4c);
            gprintfDirty(left, top + 0x50, L"TT: %d", statistics.value_08, statistics.value_0c);
            gprintfDirty(left, top + 0x5a, L"RM: %dK", g_gerd->getResidentTextureMemUsed() >> 10);
            gprintfDirty(left, top + 0x64, L"TM: %dK", g_gerd->getTextureCacheUsed());
            gprintfDirty(left, top + 0x6e, L"DR: %3d", GetCameraYawAndRotation(0));
            memset(&memory_status, 0, sizeof(memory_status));
            memory_status.dwLength = sizeof(memory_status);
            GlobalMemoryStatus(&memory_status);
            gprintfDirty(left, top + 0x78, L"MU: %dK",
                         (memory_status.dwTotalPageFile - memory_status.dwAvailPageFile) >> 10);
            gprintfDirty(left, top + 0x82, L"MM: %dK",
                         static_cast<unsigned int>(g_decompressed_mesh_bytes) >> 10);
            return;
        }
        if (g_video_inspector_mode == 3) {
            float scaled;

            GetCameraPosition(&position);
            if (gfKeyState[0x70] != 0) {
                position.x = position.x * g_world_cursor_scale;
                scaled = position.y * g_world_cursor_scale;
                position.y = position.z * g_world_cursor_scale;
                position.z = scaled;
            }
            gprintfDirty(left, top + 0xa, L" X: %.2f", position.x);
            gprintfDirty(left, top + 0x14, L" Y: %.2f", position.y);
            gprintfDirty(left, top + 0x1e, L" Z: %.2f", position.z);
        }
    }
}

// FUNCTION: WIZ8 0x004277d0
void VideoInspectorEnable(void)
{
    g_video_inspector_enabled = 1;
}

// GLOBAL: WIZ8 0x006548a0
INT32 g_help_box_width;
// GLOBAL: WIZ8 0x00654acc
INT32 g_help_box_height;

// GLOBAL: WIZ8 0x00654aac
int g_screen_transition_object_count;
// GLOBAL: WIZ8 0x00654ab4
srClass** g_screen_transition_objects;

// FUNCTION: WIZ8 0x00429770
void VideoRemoveToolTip(void)
{
    int index;
    srClass* object;

    while (g_screen_transition_object_count != 0) {
        object = g_screen_transition_objects[0];
        if (g_screen_transition_object_count > 0) {
            for (index = 0; index < g_screen_transition_object_count - 1; ++index) {
                g_screen_transition_objects[index] = g_screen_transition_objects[index + 1];
            }
            --g_screen_transition_object_count;
        }
        object->release();
    }
}

// FUNCTION: WIZ8 0x00424A40
srShader::srShader()
{
    value = 0x0100241b;
}

// FUNCTION: WIZ8 0x00424A90
srNode* VideoMakePoster(srColorSurfaceIFace* surface, float width, float height,
                        unsigned char additive)
{
    srTextureIFace::e_hint hint;
    srTextureMap* texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
    texture->setMipmapBias(-8.0f);
    texture->autoRelease();
    texture->setName("VideoMakePoster");
    texture->setSurfacePtr(surface);
    texture->setWrapS(srTextureIFace::WRAP_CLAMP);
    texture->setWrapT(srTextureIFace::WRAP_CLAMP);
    if (additive == 0) {
        hint = srTextureIFace::HINT_POSITIONAL_1;
    } else {
        hint = srTextureIFace::HINT_POSITIONAL_2;
    }
    texture->enableHint(hint);
    return MakePosterQuad00424BA0(texture, width, height, additive);
}

void PresentMenuOverlayFrame(void)
{
    srNode::ProcessInfo process;

    FlushDirtyTiles();
    g_gerd->beginFrame();
    process.renderer = g_gerd;
    g_surface_node->process(process, (srNode::e_processType)0);
    g_gerd->flushRenderers();
    g_gerd->endFrame();
}

// FUNCTION: WIZ8 0x00425570
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled)
{
    if (g_surface_node) {
        g_surface_node->setTextureHint2Enabled(enabled);
    }
}

// FUNCTION: WIZ8 0x00424040
unsigned char InitializeMouseSurface(void)
{
    srPixelConvert::e_surfaceType type;

    if (g_pixel_format == 7) {
        type = srPixelConvert::SURFACE_RGB565;
    } else if (g_pixel_format == 8) {
        type = srPixelConvert::SURFACE_RGB555;
    } else if (g_pixel_format == 9) {
        type = srPixelConvert::SURFACE_ARGB1555;
    } else {
        return 0;
    }

    g_mouse_surface = SR_NEW(W8ColorSurface)(type, 128UL, 128UL);
    if (!g_mouse_surface) {
        srAssertFail("psrMouseSurface", "C:\\Projects\\Wizardry 8\\Engine Code\\Video2.cpp", 0x635,
                     0);
    }
    g_mouse_surface->setFilter(&srBoxFilter);
    g_mouse_surface->fill(0);
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
    g_scene_permanent = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_permanent->setName("2D Permanent Overlay Scene");
    g_scene_permanent->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_permanent->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_user = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_user->setName("2D User Overlay Scene");
    g_scene_user->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_user->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_fullscreen = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_fullscreen->setName("Full Screen Overlay Scene");
    g_scene_fullscreen->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_fullscreen->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay0 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_overlay0->setName("2D Overlay Scene (0)");
    g_scene_overlay0->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay0->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_overlay1 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_overlay1->setName("2D Overlay Scene (1)");
    g_scene_overlay1->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_overlay1->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_square = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_square->setName("2D Square Overlay Scene");
    g_scene_square->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_square->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender0 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_prerender0->setName("2D Pre-render Overlay Scene (0)");
    g_scene_prerender0->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender0->setFogColor(0.0f, 0.0f, 0.0f);

    g_scene_prerender1 = SR_NEW(srScene)(static_cast<srNode*>(0));
    g_scene_prerender1->setName("2D Pre-render Overlay Scene (1)");
    g_scene_prerender1->setAmbientLight(0.0f, 0.0f, 0.0f);
    g_scene_prerender1->setFogColor(0.0f, 0.0f, 0.0f);

    g_overlay_camera = SR_NEW(srCamera)(static_cast<srNode*>(0));
    g_overlay_camera->setName("2D Overlay Camera");
    g_overlay_camera->setClipRange(0.01, 2.0);
    g_overlay_camera->setLocation(0.0, 0.0, -1.0);
    g_overlay_camera->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 1.0;
    g_overlay_camera->setViewPlane(view, 1.0);
    g_overlay_camera->setEnvironmentRange(0.0f, 0.0f);

    g_square_camera = SR_NEW(srCamera)(g_scene_square);
    g_square_camera->setName("2D Square Overlay Camera");
    g_square_camera->setClipRange(0.01, 2.0);
    g_square_camera->setLocation(0.0, 0.0, -1.0);
    g_square_camera->setRotation(0.0, 0.0, 0.0);
    view.left = 0.0;
    view.bottom = 0.0;
    view.right = 1.0;
    view.top = 0.75;
    g_square_camera->setViewPlane(view, 1.0);
    g_square_camera->setEnvironmentRange(0.0f, 0.0f);

    material = SR_NEW(srMaterial);
    g_blit_material = material;
    material->setName("Blit Rect Material");
    material_value = 1.0f;
    material->setEmissive(material_value);
    material_value = 0.0f;
    material->setDiffuse(material_value);
    material->setSpecular(material_value);
    material->setOpacity(1.0);

    memset(g_surface_nodes, 0, sizeof(g_surface_nodes));
    memset(g_tile_dirty_flags, 0, sizeof(g_tile_dirty_flags));
    g_viewport_6595e8.left = 0;
    g_viewport_6595e8.top = 0;
    g_viewport_6595e8.right = 0;
    g_surface_state_6595dc = 0x100a017;
    g_surface_state_654ad8 = 0x100c0b7;
    g_dirty_tile_count = 0;
    g_viewport_6595e8.bottom = 0;

    memset(&surface_description, 0, sizeof(surface_description));
    surface_description.dwSize = sizeof(surface_description);
    DDLockSurface(g_primary_surface, 0, &surface_description, 0, 0);
    DDUnlockSurface(g_primary_surface, 0);
    g_primary_color_surface = SR_NEW(W8ColorSurface)(
        srPixelConvert::SURFACE_ARGB1555, surface_description.lpSurface, 640UL, 480UL,
        static_cast<unsigned long>(surface_description.lPitch));
    if (!g_primary_color_surface)
        return 0;

    g_surface_node = new stSurface2D(g_primary_color_surface, 640, 480, g_scene_overlay0, 128);
    if (!g_surface_node)
        return 0;

    strncpy(renderer_name, g_gerd->getName(), 127);
    renderer_name[127] = 0;
    _strupr(renderer_name);
    if (strstr(renderer_name, "GLIDE")) {
        g_surface_node->enableRendererFlag(1);
    }
    g_renderer_mode = strstr(renderer_name, "DIRECT3D") || strstr(renderer_name, "GLIDE") ||
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

    DDLockSurface(g_primary_surface, 0, &surface_description, 0, 0);
    if (surface_description.lpSurface != 0) {
        if (top < bottom) {
            row = static_cast<unsigned char*>(surface_description.lpSurface) + left * 2 +
                  surface_description.lPitch * top;
            rows = bottom - top;
            do {
                memset(row, 0, (right - left) * 2);
                row = row + surface_description.lPitch;
                --rows;
            } while (rows != 0);
        }
        DDUnlockSurface(g_primary_surface, 0);
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
                if (g_surface_nodes[index] == node) {
                    g_surface_nodes[index] = 0;
                    g_tile_dirty_flags[index] = 0;
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
        node->setFlag(srNode::FLAG_DISABLE);
    }
}

// FUNCTION: WIZ8 0x00427810
srModelInstance* GetPickedModelInstance(void)
{
    return g_current_model_instance;
}

// FUNCTION: WIZ8 0x00427820
void SetPickedModelInstance(srModelInstance* value)
{
    g_current_model_instance = value;
}

// FUNCTION: WIZ8 0x00428010
unsigned char DisableCursorScene(void)
{
    g_cursor_scene_enabled = 0;
    return 1;
}

// FUNCTION: WIZ8 0x00428020
unsigned char EnableCursorScene(void)
{
    g_cursor_scene_enabled = 1;
    return 1;
}

/* Release an srClass, leaving the renderer in 2D mode, or in the paired
   mode when overlay_scene_flag_160 bit 0 says otherwise. Recovered callers pass
   stModelInstance2D sprites (highlight / formation board). */
// FUNCTION: WIZ8 0x004257F0
void ReleaseObject(srClass* object)
{
    if ((static_cast<stModelInstance2D*>(object)->overlay_scene_flag_160 & 1) != 0) {
        g_overlay_render_mode = 2;
    } else {
        g_paired_render_mode = 2;
        g_overlay_render_mode = 2;
    }
    object->release();
}

/* Rotate a 2D sprite node by `degrees` about z and dirty the renderer mode
   word its overlay_scene_flag_160 bit 0 selects. */
// FUNCTION: WIZ8 0x00425840
void RotateNodeInDegrees(srNode* node, int degrees)
{
    node->setRotation(0.0, 0.0, 3.141592653589793 * g_float_005ebcf8 * degrees);
    if ((static_cast<stModelInstance2D*>(node)->overlay_scene_flag_160 & 1) != 0) {
        g_overlay_render_mode = 2;
    } else {
        g_paired_render_mode = 2;
        g_overlay_render_mode = 2;
    }
}

// FUNCTION: WIZ8 0x00428A90
void SetOverlayRenderMode(void)
{
    g_overlay_render_mode = 2;
}

// FUNCTION: WIZ8 0x00428AA0
void SetRendererModePair(void)
{
    g_paired_render_mode = 2;
    g_overlay_render_mode = 2;
}

/* Install a texture (often an stTextureAnim) on the mouse-cursor mesh. A null
   argument reuses the current cursor texture. */
// FUNCTION: WIZ8 0x00429170
void SetMouseCursorTexture(srTextureIFace* texture)
{
    if (texture == 0) {
        texture = g_cursor_texture;
    }
    g_cursor_model->setTexture(texture, 0, 0);
    if ((g_cursor_model->control_state_390 & 8) == 0) {
        unsigned long state = g_cursor_model->control_state_390;
        g_cursor_model->control_state_390 = state | 8;
        g_cursor_model->control_state_390 = state | 8;
    }
}

// FUNCTION: WIZ8 0x004291C0
unsigned char GetRendererModeByte(void)
{
    return static_cast<unsigned char>(g_renderer_mode);
}

// FUNCTION: WIZ8 0x00429200
void SetOverlayViewport(const int* value)
{
    g_overlay_viewport = value;
}

// FUNCTION: WIZ8 0x004297D0
bool HasScreenTransitionObjects(void)
{
    return g_screen_transition_object_count != 0;
}

// FUNCTION: WIZ8 0x004297e0
void SetSurfaceScale(float scale)
{
    g_surface_node->setScale(scale);
    g_surface_scale = scale;
}

// FUNCTION: WIZ8 0x004298E0
void SetFullscreenSceneLast(unsigned char value)
{
    g_fullscreen_scene_last = value;
}

// FUNCTION: WIZ8 0x004298F0
bool HasEnoughFreeDiskSpace(void)
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
    bool enough;

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
void __fastcall ReleaseOwnedClass(srClass** owner)
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
    return g_fullscreen;
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
    return g_primary_surface;
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
    *pitch = g_mouse_surface->getPitch();
    return g_mouse_surface->getDataPtr();
}

/* The retail linker folds this empty body with other empty C functions. */
void UnlockMouseBuffer(void) {}

/* The retail empty video-capture entry folds with the shared 0x4023a0 ret. */
void VideoCaptureToggle(void) {}

// FUNCTION: WIZ8 0x00421f30
IDirectDraw2* GetDirectDraw2Object(void)
{
    return g_direct_draw2;
}

// GLOBAL: WIZ8 0x006596f4
unsigned char g_auto_capture;
// GLOBAL: WIZ8 0x00659724
int g_screenshot_index;
// GLOBAL: WIZ8 0x00659728
int g_screenshot_page;

// FUNCTION: WIZ8 0x004229e0
void SaveJpegScreenshot(void)
{
    srSurfaceIOManager* surface_io_manager = srCore.getSurfaceIOManager();
    srExtension::load("JPEGImporter", 0);

    srColorSurfaceIFace* surface = g_gerd->lockBuffer();
    int screenshot_index = g_screenshot_index;
    if (surface != 0) {
        char filename[32];
        srSurfaceIOManager::ExportInfo options;
        options.unknown_00 = 0;
        options.unknown_04 = 1;
        options.option_string = 0;

        ++g_screenshot_index;
        sprintf(filename, "Wiz8%5.5d.JPG", screenshot_index);
        if (g_auto_capture == 0) {
            surface_io_manager->exportSurface(filename, *surface, options);
        } else {
            options.option_string = "QUALITY=0.35";
            PauseSharedGameTimers();
            surface_io_manager->exportSurface(filename, *surface, options);
            ResumeSharedGameTimers();
        }
        g_gerd->unlockBuffer();
    }
    g_screenshot_page = (g_screenshot_page - 1) & 1;
}

/* Tooltip placement state. The left/top pair records the last position the
   tooltip builder used; the scale participates in the texture mapping. */
// GLOBAL: WIZ8 0x00654ab8
int g_help_box_x;
// GLOBAL: WIZ8 0x00654abc
int g_help_box_y;
// GLOBAL: WIZ8 0x00654ab0
int g_screen_transition_object_capacity;

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
    double position_x = x * g_double_005ebe90;
    double position_y = y * g_double_005ebe88;

    if (positional != 0 && g_gerd != 0) {
        double whole;
        long width = g_gerd->getWidth();
        double fraction = modf(width * position_x, &whole);
        position_x -= fraction / width;
        long height = g_gerd->getHeight();
        fraction = modf(height * position_y, &whole);
        position_y -= fraction / height;
    }

    int width = instance->GetWidth00480EF0() & 0xffff;
    double half_width = width * g_double_005ebe90 * g_double_005ebe80;
    int height = instance->GetHeight00480F70() & 0xffff;
    double half_height = height * g_double_005ebe88 * g_double_005ebe80;

    srVector3T<double> location;
    location.x = half_width + position_x;
    location.z = -0.0001;
    if ((instance->overlay_scene_flag_160 & 1U) == 0) {
        location.y = g_double_005ebc30 - (half_height + position_y);
        g_paired_render_mode = 2;
    } else {
        location.y = g_double_005ebf40 - (half_height + position_y) * g_double_005ebf40;
    }
    node->setLocation(location);
    g_overlay_render_mode = 2;
    instance->render_state_164.right = (short)x;
    instance->render_state_164.bottom = (short)y;
}

/* Builds a square power-of-two polygon brush from a surface rectangle. The
   larger source extent (including a one-pixel border) is rounded up to 16..256,
   then CopySurfaceWithBorder / MakePolygonBrush install it on the square
   overlay scene. */
// FUNCTION: WIZ8 0x00424560
srModelInstance* Video2DRectToSquarePolygon(int* rect, void* source, int source_pitch,
                                            srNode* parent, unsigned char overlay)
{
    int extent = (rect[3] - rect[1]) + 2;
    int width_extent = (rect[2] - rect[0]) + 2;
    double width = rect[2] * g_double_005ebe90 - rect[0] * g_double_005ebe90;
    if (extent < width_extent) {
        extent = width_extent;
    }
    if (extent <= 0x100) {
        unsigned long size;
        if (extent > 0x80) {
            size = 0x100;
        } else if (extent > 0x40) {
            size = 0x80;
        } else if (extent > 0x20) {
            size = 0x40;
        } else {
            size = ((extent <= 0x10) - 1 & 0x10) + 0x10;
        }

        srColorSurface* surface =
            SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, size, size);
        surface->autoRelease();
        surface->fill(0);
        surface->setFilter(&srBoxFilter);

        float scale_x;
        float scale_y;
        float mapping_x;
        float mapping_y;
        if (CopySurfaceWithBorder(surface, rect, source, source_pitch, &scale_x, &scale_y,
                                  &mapping_x, &mapping_y)) {
            srModelInstance* node = MakePolygonBrush(parent, surface, width, width, scale_x,
                                                     scale_y, mapping_x, mapping_y, overlay);
            node->setName("Video2DRectToSquarePolygon");
            stModelInstance2D* instance = static_cast<stModelInstance2D*>(node);
            instance->render_state_164.display_state = static_cast<unsigned char>(g_active_page);
            instance->overlay_scene_flag_160 |= 1;
            instance->render_state_164.left = static_cast<short>(size);
            instance->render_state_164.top = static_cast<short>(size);
            PositionToolTipNode(node, rect[0], rect[1], 0);
            return node;
        }
        surface->release();
    }
    return 0;
}

/* Locks a video surface (or negative target id), copies the requested rectangle
   into either the user overlay polygon path or the square overlay path, and
   records the resulting 2D instance extents. */
// FUNCTION: WIZ8 0x004253F0
stModelInstance2D* CreateSpriteFromVideoSurface(int target, const W8ControlsRect* bounds, int a3,
                                                int /*a4*/, char a5)
{
    HVSURFACE surface;
    unsigned short width;
    unsigned short height;
    int source_rect[4];
    UINT32 pitch;
    BYTE* pixels;
    srModelInstance* node;
    stModelInstance2D* instance;
    char mode = static_cast<char>(a3);

    if (!GetVideoSurface(&surface, static_cast<UINT32>(target))) {
        return 0;
    }
    if (bounds == 0) {
        source_rect[0] = 0;
        source_rect[1] = 0;
        source_rect[2] = surface->usWidth;
        source_rect[3] = surface->usHeight;
        width = surface->usWidth;
        height = surface->usHeight;
    } else {
        width = static_cast<unsigned short>(static_cast<short>(bounds->right) -
                                            static_cast<short>(bounds->left));
        height = static_cast<unsigned short>(static_cast<short>(bounds->bottom) -
                                             static_cast<short>(bounds->top));
        source_rect[0] = bounds->left;
        source_rect[1] = bounds->top;
        source_rect[2] = bounds->right;
        source_rect[3] = bounds->bottom;
        if (width > surface->usWidth) {
            return 0;
        }
        if (height > surface->usHeight) {
            return 0;
        }
    }
    if (width > 0x100 || height > 0x100) {
        return 0;
    }
    pixels = LockVideoSurface(static_cast<UINT32>(target), &pitch);
    if (pixels == 0) {
        return 0;
    }
    if (mode != 0) {
        node = Video2DRectToSquarePolygon(source_rect, pixels, static_cast<int>(pitch),
                                          g_scene_square, a5);
    } else {
        node = Video2DRectToPolygon(source_rect, pixels, static_cast<int>(pitch), g_scene_user, a5);
        g_paired_render_mode = 2;
    }
    g_overlay_render_mode = 2;
    UnLockVideoSurface(static_cast<UINT32>(target));
    instance = static_cast<stModelInstance2D*>(node);
    if (instance != 0) {
        if (mode != 0) {
            unsigned short extent = width;
            if (width <= height) {
                extent = height;
            }
            instance->render_state_164.top = extent;
            if (width <= height) {
                width = height;
            }
        } else {
            instance->render_state_164.top = height;
        }
        instance->render_state_164.left = width;
    }
    /* Retail writes display_state even when the node factory returned null. */
    instance->render_state_164.display_state = 3;
    return instance;
}

/* Builds a solid-color quad sprite. Width/height are pixel counts; color becomes
   the material emissive. Radar blip templates are the observed callers. */
// FUNCTION: WIZ8 0x00424790
stModelInstance2D* CreateColoredPolygonSprite(int width, int height, const srVector4T<float>* color,
                                              char a4)
{
    double scale_x = width * g_double_005ebe90;
    double scale_y = height * g_double_005ebe88;
    srMeshModel* model = SR_NEW(srMeshModel)(0L, 0L);
    model->autoRelease();

    g_modeler_65963c->createGrid(1, 1);
    srVector3T<float> scale;
    scale.x = static_cast<float>(scale_x);
    scale.y = static_cast<float>(scale_y);
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    srMaterial* material = SR_NEW(srMaterial)();
    material->autoRelease();
    material->setEmissive(*color);
    srVector4T<float> zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;
    zero.w = 0.0f;
    material->setDiffuse(zero);
    material->setSpecular(zero);
    material->parms.shininess = 1.0f;
    material->parms.diffuse.w = 1.0f;
    material->dirty_74 = 1;
    model->setMaterial(material, 0, static_cast<srMeshModel::e_side>(0));

    stModelInstance2D* instance = new stModelInstance2D(g_scene_user);
    instance->setName("Video2DPolyColored");
    instance->SetModel(model);
    instance->setRotation(0.0, 0.0, 0.0);

    srShader shader;
    shader.value = 0x2417;
    model->setShader(shader, 0);

    instance->render_state_164.left = static_cast<short>(width);
    instance->render_state_164.top = static_cast<short>(height);
    instance->render_state_164.display_state = 3;
    if (a4 != 0) {
        instance->setParent(g_scene_fullscreen, 1);
    }
    return instance;
}

// FUNCTION: WIZ8 0x004255c0
stModelInstance2D* CreateSpriteFromSurface(unsigned int image, const W8ControlsRect* rect, int mode,
                                           int arg_4, int arg_5)
{
    return CreateSpriteFromVideoSurface(image, rect, mode, arg_4, arg_5);
}

// FUNCTION: WIZ8 0x004257D0
void Position2DNodeUnsnapped(srNode* node, int x, int y)
{
    PositionToolTipNode(node, x, y, 0);
}

// FUNCTION: WIZ8 0x004264F0
void SetModelInstance2DDisplayState(stModelInstance2D* object, unsigned char state)
{
    object->render_state_164.display_state = state;
}

/* Positions every live tooltip object left to right starting at x, advancing
   the cursor by each node's scaled width. With no live objects, just record
   the requested position. */
// FUNCTION: WIZ8 0x00429210
void VideoPositionToolTip(INT32 x, INT32 y)
{
    if (g_screen_transition_object_count > 0) {
        INT32 offset = x;
        for (int index = 0; index < g_screen_transition_object_count; ++index) {
            srNode* node = static_cast<srNode*>(g_screen_transition_objects[index]);
            PositionToolTipNode(node, offset, y, 1);
            offset += static_cast<stModelInstance2D*>(node)->GetWidth00480EF0() & 0xffff;
        }
        g_help_box_y = y;
        g_help_box_x = x;
        return;
    }
    g_help_box_x = x;
    g_help_box_y = y;
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

    float scale = 1.0f / surface->getWidth();
    *scale_x = scale;
    *scale_x = scale * g_surface_scale + scale;
    scale = 1.0f / surface->getHeight();
    *scale_y = scale;
    *scale_y = scale * g_surface_scale + scale;
    *mapping_x = (rect[2] - rect[0]) / static_cast<float>(surface->getWidth());
    *mapping_y = (rect[3] - rect[1]) / static_cast<float>(surface->getHeight());
    return 1;
}

/* Builds a polygon brush from a tooltip surface rectangle. The larger source
   extent is rounded up to the next power of two between 16 and 256, the copy
   repeats its border, and the node records the rectangle extents. */
// FUNCTION: WIZ8 0x00424280
srModelInstance* Video2DRectToPolygon(int* rect, void* source, int source_pitch, srNode* parent,
                                      unsigned char overlay)
{
    double left = rect[0] * g_double_005ebe90;
    int extent = rect[2] - rect[0];
    double top = rect[1] * g_double_005ebe88;
    int rect_height = rect[3] - rect[1];
    double width = rect[2] * g_double_005ebe90 - left;
    double height = rect[3] * g_double_005ebe88 - top;

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
        instance->render_state_164.display_state = static_cast<unsigned char>(g_active_page);
        instance->render_state_164.left = (short)(rect[2] - rect[0]);
        instance->render_state_164.top = (short)(rect[3] - rect[1]);
        instance->render_state_164.right = (short)rect[0];
        instance->render_state_164.bottom = (short)rect[1];
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
    if (g_screen_transition_object_count != 0) {
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
        new W8TextBuffer(&bounds, text, g_font10arial,
                         g_W8TextBufferLayoutMask005ED558 | g_W8TextBufferLayoutMask005ED548, 4);
    if (buffer == 0) {
        return;
    }
    g_help_box_width = (int)buffer->m_maxLineWidth + 4;
    g_help_box_height = GetFontHeight(g_font10arial) * buffer->m_lineCount + 2;
    surface->fill(0);
    void* data = surface->getDataPtr();
    unsigned char colour[4];
    for (int y = 0; y < g_help_box_height; ++y) {
        PackColour00429700(colour, 1.0, 0.0, 0.0, 0.0);
        surface->setHLine(0, y, g_help_box_width, *(unsigned long*)colour);
    }
    buffer->RenderText(static_cast<unsigned char*>(data),
                       static_cast<unsigned int>(surface->getPitch()), 2, 1, 1);
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
        Video2DRectToPolygon(rect, data, static_cast<int>(surface->getPitch()), g_cursor_scene, 1);
    if (node != 0) {
        int count = g_screen_transition_object_count;
        bool append = true;
        if (g_screen_transition_object_capacity < count + 1) {
            srClass** objects = new srClass*[count + 1];
            if (objects == 0) {
                append = false;
            } else {
                for (int index = 0; index < count; ++index) {
                    objects[index] = g_screen_transition_objects[index];
                }
                delete[] g_screen_transition_objects;
                g_screen_transition_objects = objects;
                g_screen_transition_object_capacity = count + 1;
            }
        }
        if (append) {
            g_screen_transition_objects[count] = static_cast<srClass*>(node);
            g_screen_transition_object_count = count + 1;
        }
    }

    int position_y = g_cursor_height - g_help_box_height;
    int position_x = g_cursor_width;
    int offset = position_x;
    for (int index = 0; index < g_screen_transition_object_count; ++index) {
        srNode* object = static_cast<srNode*>(g_screen_transition_objects[index]);
        PositionToolTipNode(object, offset, position_y, 1);
        offset += static_cast<stModelInstance2D*>(object)->GetWidth00480EF0() & 0xffff;
    }
    g_help_box_x = position_x;
    g_help_box_y = position_y;
    surface->release();
    delete buffer;
}

/* Renderer configuration helpers reached from the video device, not the
   persisted configuration block. */
// GLOBAL: WIZ8 0x659714
int g_resident_texture_policy;

// FUNCTION: WIZ8 0x004266e0
void SetResidentTexturePolicy(int policy)
{
    if (policy != g_resident_texture_policy) {
        g_gerd->invalidateResidentTextures();
        g_gerd->invalidateTextureCache();
        g_resident_texture_policy = policy;
    }
}

// GLOBAL: WIZ8 0x659718
unsigned char g_swap_interval_enabled;

// FUNCTION: WIZ8 0x00426710
void SetSwapInterval(unsigned char enabled)
{
    g_swap_interval_enabled = enabled;
    g_gerd->setSwapInterval(enabled ? 1 : 0);
}

// FUNCTION: WIZ8 0x00426740
void SetTextureCacheSize(unsigned long bytes)
{
    if (bytes > 0x7fffff && bytes != g_gerd->getTextureCacheSize()) {
        g_gerd->invalidateResidentTextures();
        g_gerd->invalidateTextureCache();
        g_gerd->setTextureCacheSize(bytes);
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

/* Build an stTextureAnim whose frames are consecutive VObject subimages starting
   at start_frame. use_argb1555 selects SURFACE_ARGB1555 versus SURFACE_RGB555 for
   each frame surface. The setName string is the retail identity. */
// FUNCTION: WIZ8 0x00428E90
stTextureAnim* VideoVObjectToTextureAnim(HVOBJECT object, unsigned short start_frame,
                                         unsigned short frame_count, char use_argb1555)
{
    ETRLEObject properties;
    unsigned short frame;
    unsigned short end_frame;
    unsigned long extent;
    unsigned short largest;

    if (!gfVideoObjectsInit) {
        return 0;
    }

    stTextureAnim* animation = SR_NEW(stTextureAnim)();
    animation->autoRelease();
    animation->setName("VideoVObjecttoTextureAnim");
    animation->setMipmapBias(-8.0f);

    frame = start_frame;
    end_frame = static_cast<unsigned short>(start_frame + frame_count);
    while (frame < end_frame) {
        if (GetVideoObjectETRLEProperties(object, &properties, frame)) {
            largest = properties.usHeight;
            if (properties.usHeight < properties.usWidth) {
                largest = properties.usWidth;
            }
            if (largest < 0x101) {
                if (largest < 0x81) {
                    if (largest < 0x41) {
                        if (largest < 0x21) {
                            extent = ((largest < 0x11) - 1 & 0x10) + 0x10;
                        } else {
                            extent = 0x40;
                        }
                    } else {
                        extent = 0x80;
                    }
                } else {
                    extent = 0x100;
                }
            } else {
                extent = static_cast<unsigned long>(-1);
            }

            srColorSurface* surface;
            if (!use_argb1555) {
                surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_RGB555, extent, extent);
            } else {
                surface = SR_NEW(W8ColorSurface)(srPixelConvert::SURFACE_ARGB1555, extent, extent);
            }
            surface->setName("VideoVObjecttoTextureAnim:srColorSurface");
            surface->autoRelease();
            surface->fill(0);
            if (BlitHVObjectToColorSurface(object, frame, surface, 0, 0)) {
                srTextureMap* texture = SR_NEW(srTextureMap)(static_cast<srColorSurfaceIFace*>(0));
                texture->autoRelease();
                texture->setName("VideoVObjecttoTextureAnim");
                texture->setMipmapBias(-8.0f);
                texture->setSurfacePtr(surface);
                texture->setCorrection(srTextureIFace::CORRECTION_FASTEST);
                texture->setWrapS(srTextureIFace::WRAP_CLAMP);
                texture->setWrapT(srTextureIFace::WRAP_CLAMP);
                texture->setMagFilter(srTextureIFace::FILTER_NONE);
                texture->setMinFilter(srTextureIFace::FILTER_NONE);
                texture->enableHint(srTextureIFace::HINT_POSITIONAL_3);
                texture->setMipmap(srTextureIFace::MIPMAP_NONE);
                texture->enableHint(use_argb1555 ? srTextureIFace::HINT_POSITIONAL_2
                                                 : srTextureIFace::HINT_POSITIONAL_1);
                animation->AddTexture(texture);
            }
        }
        ++frame;
    }
    return animation;
}

// FUNCTION: WIZ8 0x00429800
int GetRendererFamily(void)
{
    char name[128];
    if (!g_gerd) {
        return -1;
    }
    strncpy(name, g_gerd->getName(), sizeof(name) - 1);
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
    g_gerd->setGamma(gamma);
}

/* The option-4-suppressed variant of the render probe: with the positional
   option forced off, draw the node between the dynamic scene's bracketing
   passes, restore the option and return the sampled statistic. */
// FUNCTION: WIZ8 0x00428830
unsigned int MeasureNodeRender00428830(srNode* node)
{
    srGERD::Statistics statistics;
    srNode::ProcessInfo process;

    if (g_gerd != 0 && g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4)) {
        g_gerd->toggle(srGERD::ENABLE_POSITIONAL_4);
    }
    g_gerd->flushRenderers();
    g_gerd->resetStatistics();
    g_gerd->beginFrame();
    srNode::lockSceneGraph();
    process.renderer = g_gerd;
    g_world->dynamic_scene->process(process, static_cast<srNode::e_processType>(1));
    node->process(process, static_cast<srNode::e_processType>(0));
    g_world->dynamic_scene->process(process, static_cast<srNode::e_processType>(2));
    srNode::unlockSceneGraph();
    g_gerd->endFrame();
    g_gerd->flushRenderers();
    g_gerd->getStatistics(statistics);
    if (g_gerd != 0 && !g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4)) {
        g_gerd->toggle(srGERD::ENABLE_POSITIONAL_4);
    }
    return static_cast<unsigned int>(statistics.value_10);
}

/* Open the render-probe pass: force renderer option 4 off, reset the frame
   to a solid blue clear and prime the scissor before the measured draw. */
// FUNCTION: WIZ8 0x00428910
void BeginRenderProbe(void)
{
    if (g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4)) {
        g_gerd->toggle(srGERD::ENABLE_POSITIONAL_4);
    }
    g_gerd->setClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    g_gerd->setAmbientLight(1.0f, 1.0f, 1.0f, 1.0f);
    g_gerd->beginFrame();
    g_gerd->setScissor(0, 0, g_gerd->getWidth(), g_gerd->getHeight());
    g_gerd->clear(srFlags<srGERD::e_buffer>(3));
    g_gerd->endFrame();
}

/* Draw the node between the dynamic scene's bracketing passes and return the
   renderer statistic the probe samples. */
// FUNCTION: WIZ8 0x004289e0
unsigned int MeasureNodeRender004289E0(srNode* node)
{
    srGERD::Statistics statistics;
    srNode::ProcessInfo process;

    g_gerd->flushRenderers();
    g_gerd->resetStatistics();
    g_gerd->beginFrame();
    srNode::lockSceneGraph();
    process.renderer = g_gerd;
    g_world->dynamic_scene->process(process, static_cast<srNode::e_processType>(1));
    node->process(process, static_cast<srNode::e_processType>(0));
    g_world->dynamic_scene->process(process, static_cast<srNode::e_processType>(2));
    srNode::unlockSceneGraph();
    g_gerd->endFrame();
    g_gerd->flushRenderers();
    g_gerd->getStatistics(statistics);
    return static_cast<unsigned int>(statistics.value_10);
}

/* Close the render-probe pass: restore renderer option 4 and present. */
// FUNCTION: WIZ8 0x004289c0
void EndRenderProbe(void)
{
    if (!g_gerd->isEnabled(srGERD::ENABLE_POSITIONAL_4)) {
        g_gerd->toggle(srGERD::ENABLE_POSITIONAL_4);
    }
    g_gerd->flipFrame();
}

/* Compiler-generated vtable and template emissions, grouped here as emission
   provenance. They are instantiation output from the SurRender headers, not
   authored Video2 bodies. */
// VTABLE: WIZ8 0x005EBE98
// class srClientSupport<srMeshModel,8208>

// TEMPLATE: WIZ8 0x00429B30
// srClientSupport<srMeshModel,8208>::getClassID

// TEMPLATE: WIZ8 0x00429B40
// srClientSupport<srMeshModel,8208>::getClassName

// TEMPLATE: WIZ8 0x00429B50
// srClientSupport<srMeshModel,8208>::getClassNode

// TEMPLATE: WIZ8 0x00429BC0
// srClientSupport<srMeshModel,8208>::clone

/* 0x00424A50 calls the imported ~srMeshModel: it is the local
   ??_GsrMeshModel thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00424A50
// srMeshModel::`scalar deleting destructor'

/* CVDUMP includes the class tag on the repeated self-type argument in each
   vftable symbol below. These remain ordinary self-support instantiations. */
// VTABLE: WIZ8 0x005EBEEC
// class srClientSupport<srTextureMap,8465>

/* 0x00424B70 calls the imported ~srTextureMap: it is the local
   ??_GsrTextureMap thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00424B70
// srTextureMap::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429BE0
// srClientSupport<srTextureMap,8465>::getClassID

// TEMPLATE: WIZ8 0x00429BF0
// srClientSupport<srTextureMap,8465>::getClassName

// TEMPLATE: WIZ8 0x00429C00
// srClientSupport<srTextureMap,8465>::getClassNode

// TEMPLATE: WIZ8 0x00429CA0
// srClientSupport<srTextureMap,8465>::clone

/* The constructor at 0x00429D70 registers class 8720 under the 8704 class
   node owned by srMaterialIFace through srMaterial::sGetClassNode: the
   client layer is srClientSupport<srMaterial,8720>, whose canonical class
   already chains that registration. */
// VTABLE: WIZ8 0x005EBDE0
// class srClientSupport<srMaterial,8720>

// TEMPLATE: WIZ8 0x00429D70
// srClientSupport<srMaterial,8720>::srClientSupport

// SYNTHETIC: WIZ8 0x0042A230
// srClientSupport<srMaterial,8720>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429CC0
// srClientSupport<srMaterial,8720>::getClassID

// TEMPLATE: WIZ8 0x00429CD0
// srClientSupport<srMaterial,8720>::getClassName

// TEMPLATE: WIZ8 0x00429CE0
// srClientSupport<srMaterial,8720>::getClassNode

// TEMPLATE: WIZ8 0x00429D50
// srClientSupport<srMaterial,8720>::clone

/* 0x00423E50 calls the imported ~srMaterial: it is the local ??_GsrMaterial
   thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00423e50
// srMaterial::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x00429E80
// srClassSupport<srMaterialIFace,srClass,1,8704>::getClassID

// TEMPLATE: WIZ8 0x00429EE0
// srClassSupport<srMaterialIFace,srClass,1,8704>::clone

// TEMPLATE: WIZ8 0x0042A1A0
// srClassSupport<srMaterialIFace,srClass,1,8704>::~srClassSupport

// SYNTHETIC: WIZ8 0x0042A170
// srClassSupport<srMaterialIFace,srClass,1,8704>::`scalar deleting destructor'

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srCamera self-support template. */
// VTABLE: WIZ8 0x005EBE14
// class srClientSupport<srCamera,5120>

// TEMPLATE: WIZ8 0x0042A010
// srClientSupport<srCamera,5120>::getClassID

// TEMPLATE: WIZ8 0x0042A020
// srClientSupport<srCamera,5120>::getClassName

// TEMPLATE: WIZ8 0x0042A030
// srClientSupport<srCamera,5120>::getClassNode

// TEMPLATE: WIZ8 0x0042A0A0
// srClientSupport<srCamera,5120>::clone

/* 0x00423E80 calls the imported ~srCamera: it is the local ??_GsrCamera
   thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00423e80
// srCamera::`scalar deleting destructor'

/* CVDUMP includes the class tag on the repeated self-type argument in the
   vftable symbol.  It is still the ordinary srScene self-support template. */
// VTABLE: WIZ8 0x005EBE48
// class srClientSupport<srScene,4112>

// TEMPLATE: WIZ8 0x0042A0C0
// srClientSupport<srScene,4112>::getClassID

// TEMPLATE: WIZ8 0x0042A0D0
// srClientSupport<srScene,4112>::getClassName

// TEMPLATE: WIZ8 0x0042A0E0
// srClientSupport<srScene,4112>::getClassNode

// TEMPLATE: WIZ8 0x0042A150
// srClientSupport<srScene,4112>::clone

/* 0x00423EB0 calls the imported ~srScene: it is the local ??_GsrScene
   thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00423eb0
// srScene::`scalar deleting destructor'

/* 0x00423EE0 calls the imported ~srModeler: it is the local ??_GsrModeler
   thunk emitted for the g_modeler delete, not a support-class deleting
   destructor. */
// SYNTHETIC: WIZ8 0x00423EE0
// srModeler::`scalar deleting destructor'

/* 0x004229C0 is a bare JMP to RenderFrame: a tail-jump thunk with no
   distinct source entity. */
// SYNTHETIC: WIZ8 0x004229C0
// RenderFrame (tail-jump thunk)

// VTABLE: WIZ8 0x005EBD10
// class srClientSupport<srColorSurface,12560>

// TEMPLATE: WIZ8 0x00429A40
// srClientSupport<srColorSurface,12560>::getClassID

// TEMPLATE: WIZ8 0x00429A50
// srClientSupport<srColorSurface,12560>::getClassName

// TEMPLATE: WIZ8 0x00429A60
// srClientSupport<srColorSurface,12560>::getClassNode

// TEMPLATE: WIZ8 0x00429AD0
// srClientSupport<srColorSurface,12560>::clone

/* 0x00423F00 calls the imported ~srColorSurface: it is the local
   ??_GsrColorSurface thunk, not the support-class deleting destructor. */
// SYNTHETIC: WIZ8 0x00423f00
// srColorSurface::`scalar deleting destructor'

// FUNCTION: WIZ8 0x00424BA0
srNode* MakePosterQuad00424BA0(srTextureIFace* texture, float width, float height,
                               unsigned char additive)
{
    srShader shader;
    srPtr<srPalette> palette;
    srTextureIFace::Dimensions dimensions;
    srPixelConvert::PixelFormat format;
    format.flags = 0;
    dimensions.width = 64;
    dimensions.height = 64;
    palette = srCore.getPalette();
    srFilter* filter = srCore.getFilter();
    srPixelConvert::mapPixelFormat(static_cast<srPixelConvert::e_surfaceType>(0xb), format);

    stMeshModel* model = SR_NEW(stMeshModel)(0, 0);
    if (model == 0) {
        return 0;
    }
    float extent_w = g_float_005ebb38 / dimensions.width * g_surface_scale;
    float extent_h = g_float_005ebb38 / dimensions.height * g_surface_scale;
    model->autoRelease();
    model->setName("VideoMakePoster");
    texture->getDimensions(dimensions);
    g_modeler_65963c->createGrid(1, 1);
    srModeler::MappingInfo mapping(srModeler::AXIS_X, srModeler::AXIS_Y,
                                   g_float_005ebb38 - extent_w, g_float_005ebb38 - extent_h,
                                   extent_w, extent_h);
    g_modeler_65963c->planarMap(0, 0, mapping);
    srVector3T<float> scale;
    scale.x = width;
    scale.y = height;
    scale.z = 1.0f;
    g_modeler_65963c->scale(scale);
    g_modeler_65963c->convert(*model, 1);
    g_modeler_65963c->discard();

    shader.value = 0x100a013;
    if (additive != 0) {
        shader.value = 0x100c0b3;
        model->setControlMask(0x40);
    }
    model->setMaterial(g_blit_material, 0, static_cast<srMeshModel::e_side>(0));
    model->setTexture(texture, 0, 0);
    srShader shader_copy;
    shader_copy.CopyValue(&shader.value);
    model->setShader(shader_copy, 0);

    stModelInstance* instance = SR_NEW(stModelInstance)(static_cast<srNode*>(0));
    instance->setName("VideoMakePoster");
    if (instance != 0) {
        instance->setModel(model);
    }
    instance->render_flags_178 |= 0x10;
    return instance;
}

/* Exception-scope cleanup for the small MakePosterQuad local whose managed
   pointer lives at +0x08 (the palette slot). */
struct Video2PosterQuadInfo {
    unsigned long width;
    unsigned long height;
    srClass* pointer_08;
};

// FUNCTION: WIZ8 0x00424EA0
void __fastcall ReleaseOwnedMember(Video2PosterQuadInfo* object)
{
    if (object->pointer_08 != 0) {
        object->pointer_08->release();
    }
}

// FUNCTION: WIZ8 0x00425590
void DrawColorSurface(srColorSurface* surface, int x, int y)
{
    g_primary_color_surface->blit(x, y, *surface, 0, 0, surface->getWidth(), surface->getHeight());
}

// FUNCTION: WIZ8 0x00425DA0
void SetScaledViewport00425DA0(int left, int top, int right, int bottom)
{
    if (left == g_viewport_6595e8.left && top == g_viewport_6595e8.top &&
        right == g_viewport_6595e8.right && bottom == g_viewport_6595e8.bottom) {
        return;
    }
    if (left == g_viewport_6595e8.left) {
        if (top == g_viewport_6595e8.top && right == g_viewport_6595e8.right &&
            bottom == g_viewport_6595e8.bottom) {
            goto store;
        }
    }
    g_gerd->setViewPort(g_gerd->getWidth() * left / 640, g_gerd->getHeight() * top / 480,
                        g_gerd->getWidth() * (right - left) / 640,
                        g_gerd->getHeight() * (bottom - top) / 480);
store:
    g_viewport_6595e8.left = left;
    g_viewport_6595e8.top = top;
    g_viewport_6595e8.right = right;
    g_viewport_6595e8.bottom = bottom;
}

// FUNCTION: WIZ8 0x00426490
void DrawBufferLine(long x0, long y0, long x1, long y1, unsigned long* pixel)
{
    if (x0 != 0 && y0 != 0 && x1 != 0 && y1 != 0) {
        srColorSurfaceIFace* surface = g_gerd->lockBuffer();
        if (surface != 0) {
            surface->setLine(x0, y0, x1, y1, *pixel);
            g_gerd->unlockBuffer();
        }
    }
}

// FUNCTION: WIZ8 0x004273F0
void GetScaledViewportBounds(float* left_top, float* right_bottom)
{
    left_top[0] = g_viewport_6595e8.left * g_scale_x_5ebb1c;
    left_top[1] = g_viewport_6595e8.top * g_scale_y_5ebb20;
    right_bottom[0] = g_viewport_6595e8.right * g_scale_x_5ebb1c;
    right_bottom[1] = g_viewport_6595e8.bottom * g_scale_y_5ebb20;
}

// FUNCTION: WIZ8 0x004277F0
void SetPickKey(void* key)
{
    if (g_gerd) {
        g_gerd->setPickKey(
            reinterpret_cast<unsigned long>(key)); // reinterpret-ok: opaque pick token
    }
}

/* Compiler emissions between Video2.cpp's authored bodies and Levels.cpp's
   first anchor: sr refcounted-pointer and class-support template bodies plus
   their deleting destructors. */

// TEMPLATE: WIZ8 0x00429B00
// srPtr assignment emission: release the held interface, addref and store the new one

// SYNTHETIC: WIZ8 0x0042A360
// srVertexProcessor::~srVertexProcessor trivial body

// SYNTHETIC: WIZ8 0x0042B890
// srVertexProcessor scalar deleting destructor

/* srVertexProcessor::MaterialInfo's inline ctor emitted out-of-line inside
   srMaterial's locally-compiled constructor; the primary is in
   srVertexProcessor.h. */
// SYNTHETIC: WIZ8 0x00424A80
// srVertexProcessor::MaterialInfo::MaterialInfo (Video2.cpp emission)
