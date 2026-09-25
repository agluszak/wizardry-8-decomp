/* Modified for the Wizardry 8 reconstruction, 2026-09-10.
   Reconstructed Wizardry Video2 interface and product C linkage.
   Distributed under the accompanying SFI Source Code license agreement. */
#ifndef WIZ8_VIDEO2_H
#define WIZ8_VIDEO2_H

#include <windows.h>
#include <ddraw.h>
#include <process.h>

#include "Local.h"
#include "Debug.h"
#include "Types.h"
#include "DirectDraw Calls.h"
#include "VSurface.h"
#include "Mutex Manager.h"

#define BUFFER_READY 0x00
#define BUFFER_BUSY 0x01
#define BUFFER_DIRTY 0x02
#define BUFFER_DISABLED 0x03

#define MAX_CURSOR_WIDTH 64
#define MAX_CURSOR_HEIGHT 64
#define VIDEO_NO_CURSOR 0xFFFF

#ifdef __cplusplus
extern "C" { // C-LINKAGE: the SGP video manager interface that src/sgp/*.c references
#endif

extern HWND ghWindow;

extern BOOLEAN InitializeVideoManager(HINSTANCE hInstance, UINT16 usCommandShow, void* WindowProc);
extern void ShutdownVideoManager(void);
extern void ShutdownVideoScenes(void);
extern void SuspendVideoManager(void);
extern BOOLEAN RestoreVideoManager(void);
extern void GetCurrentVideoSettings(UINT16* usWidth, UINT16* usHeight, UINT8* ubBitDepth);
extern void InvalidateRegion(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom, UINT32 uiFlags);
extern LPDIRECTDRAW2 GetDirectDraw2Object(void);
extern LPDIRECTDRAWSURFACE2 GetFrameBufferObject(void);
extern PTR LockPrimarySurface(UINT32* uiPitch);
extern void UnlockPrimarySurface(void);
extern PTR LockMouseBuffer(UINT32* uiPitch);
extern void UnlockMouseBuffer(void);
extern BOOLEAN GetPrimaryRGBDistributionMasks(UINT32* RedBitMask, UINT32* GreenBitMask,
                                              UINT32* BblueBitMask);
extern void PrintScreen(void);

void VideoCaptureToggle(void);

enum { INVAL_SRC_TRANS = 1, PIXEL_DEPTH = 16, SCREEN_WIDTH = 640, SCREEN_HEIGHT = 480 };

BOOLEAN VideoIsFullScreen(void);
void VideoFullScreen(BOOLEAN enabled);
BOOLEAN VideoResizeWindow(void);
void VideoInspectorEnable(void);
BOOLEAN VideoInspectorIsEnabled(void);
CHAR8* VideoGetConfigFile(void);
void VideoSetConfigFile(const CHAR8* path);
int VideoDumpMemoryLeaks(void);
BOOLEAN CheckCdPresent(void);
void VideoGetClientRect(RECT* rect);
void VideoToolTip(UINT16* text);
extern INT32 g_help_box_width;
extern INT32 g_help_box_height;
/* DisplayFastHelp in mousesystem.c and the product region code both access
   these fields inline; no separate getter bodies occur at those call sites. */
static __inline INT32 VideoGetToolTipWidth(void)
{
    return g_help_box_width;
}
static __inline INT32 VideoGetToolTipHeight(void)
{
    return g_help_box_height;
}
void VideoPositionToolTip(INT32 x, INT32 y);
void VideoRemoveToolTip(void);

void SGPMouseGetPos(POINT* point);

#ifdef __cplusplus
}

/* Tooltip ownership query: nonzero while a VideoToolTip object is alive. */
bool HasScreenTransitionObjects(void); /* 0x004297D0 */
/* 0x00422EC0: invalidate each rectangle in a run. */
struct W8ScreenRect;
void InvalidateScreenRects(W8ScreenRect* rects, unsigned int count, int flags);

#endif

#ifdef __cplusplus
/* Renderer state and helpers with only product C++ consumers. The C block
   above is the SGP video-manager surface the SGP C translation units
   reference; these stay ordinary C++ linkage because no C unit names them. */
extern unsigned char g_auto_capture;
extern int g_cursor_image_height;
/* 0x00652DA4: set while the swaying camera view is active; see
   SetCameraSwayMode in 3dapi.cpp. */
extern bool g_camera_sway_active;
extern int g_screenshot_index;
extern int g_screenshot_page;
void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom);
#ifdef __cplusplus

extern const float g_scale_x_5ebb1c;
extern const float g_scale_y_5ebb20;

class srColorSurface;
class srColorSurfaceIFace;
class srModelInstance;
class srNode;
class srTextureIFace;
class stModelInstance2D;
template <class T> class srVector3T;
template <class T> class srVector4T;
srNode* MakePosterQuad00424BA0(srTextureIFace* texture, float width, float height,
                               unsigned char additive);
/* 0x00425190: build a 2D marker model instance over a texture. */
stModelInstance2D* CreateSpriteFromTexture(srTextureIFace* texture, double width, double height,
                                           char keep_aspect, char a5);
/* 0x00426F80: render the world into a caller-owned color surface through a
   scissored viewport, then blit the locked frame buffer onto the target. */
unsigned char RenderWorldToSurface00426F80(srColorSurface* target, W8ScreenRect* rect,
                                           char render_secondary);
void SetPickKey(void* key);

struct W8ControlsRect;
/* 0x00424790: build a solid-color quad sprite; width/height are pixel counts
   and `color` becomes the material's emissive vector. MGSRadarMap's blip
   templates are its observed callers. */
void SetFullscreenSceneLast(unsigned char value); /* 0x004298E0 */
stModelInstance2D* CreateColoredPolygonSprite(int width, int height, const srVector4T<float>* color,
                                              char a4);
/* 0x004253F0: the render-target sprite factory CreateSpriteFromSurface wraps; the
   radar overlay is created through it directly. */
stModelInstance2D* CreateSpriteFromVideoSurface(int target, const W8ControlsRect* bounds, int a3,
                                                int a4, char a5);
/* 0x004255C0: wrap the sprite-surface factory - image is a video surface
   handle (or a negative target id), rect an optional source rectangle. */
stModelInstance2D* CreateSpriteFromSurface(unsigned int image, const W8ControlsRect* rect, int mode,
                                           int arg_4, int arg_5);
/* 0x004257D0: position a 2D node without pixel snapping. */
void Position2DNodeUnsnapped(srNode* node, int x, int y);
/* 0x004264F0: write the display-state byte of a 2D model instance. */
void SetModelInstance2DDisplayState(stModelInstance2D* object, unsigned char state);
/* 0x00425820: set FLAG_DISABLE on a node; the overlay drawers call it to hide
   the highlight sprite before redrawing the panel contents. */
void ClearNodeFlag(srNode* node);
/* 0x00425840: rotate an srNode in degrees and invalidate the renderer mode. */
void RotateNodeInDegrees(srNode* node, int degrees);
/* 0x00425950: drop a 2D node from the surface-tile table, invalidate its
   model's texture, and release the node. */
void ReleaseSurfaceNode(srNode* node);
/* 0x00427E70: surface-lock helper used while installing a drag cursor. */
bool ClearMouseSurface(void);
/* 0x004255F0: place a 2D node at a screen position in normalized
   coordinates; positional snaps to the renderer's pixel grid. */
void PositionToolTipNode(srNode* node, int x, int y, char positional);
/* 0x00427460: the debug stats readout - frame rate always, the full counter
   block in inspector mode 2, or the scaled camera position in mode 3. */
void DrawVideoInspector(int left, unsigned int top);
/* 0x00428830: the option-4-suppressed render probe - draws the node between
   the dynamic scene's bracketing passes and returns the sampled statistic. */
unsigned int MeasureNodeRender00428830(srNode* node);
/* 0x00424EB0: build the 2D polygon-brush model instance over a surface;
   RenderAutomapMarkers' item-marker factory calls it cross-TU. */
srModelInstance* MakePolygonBrush(srNode* parent, srColorSurfaceIFace* surface, double width,
                                  double height, float mapping_x, float mapping_y,
                                  float mapping_width, float mapping_height, unsigned char overlay);
/* 0x00484A40: blit one frame of a video object into a color surface. */
BOOLEAN BlitVideoObjectToColorSurface(UINT32 video_object, UINT16 region,
                                      srColorSurface* destination, UINT16 x, UINT16 y);
/* 0x00484AC0: same blit with an already-resolved HVOBJECT. */
BOOLEAN BlitHVObjectToColorSurface(HVOBJECT object, UINT16 region, srColorSurface* destination,
                                   int x, int y);
class stTextureAnim;
/* 0x00428E90: build an stTextureAnim whose frames are VObject subimages. */
stTextureAnim* VideoVObjectToTextureAnim(HVOBJECT object, unsigned short start_frame,
                                         unsigned short frame_count, char use_argb1555);
/* 0x00428A90: mark the primary renderer mode word dirty. */
void SetOverlayRenderMode(void);
/* 0x00428AA0: mark both renderer mode words dirty. */
void SetRendererModePair(void);
/* 0x00428910 / 0x004289C0 / 0x004289E0: the render-probe bracket the region
   link builder uses to count a mesh's drawn faces — begin the probe pass,
   draw the node and return its covered-face count, then end the pass. */
void BeginRenderProbe(void);
unsigned int MeasureNodeRender004289E0(srNode* node);
void EndRenderProbe(void);

#endif

void WarpSystemCursor(int x, int y); /* 0x004280C0: fullscreen-safe */

#ifdef __cplusplus

class srColorSurface;
class srCamera;
class srClass;
class srModeler;
class srGERD;
class srMaterial;
class srModelInstance;
class srNode;
class srScene;
class stSurface2D;
struct EnvironmentColour;
template <class T> class srVector3T;

extern int g_pixel_format;
extern unsigned char g_fullscreen;
extern bool g_screenshot_pending;
extern bool g_video_inspector_enabled;
extern bool g_cursor_scene_enabled;
extern unsigned char g_fullscreen_scene_last;
extern const int* g_overlay_viewport;
extern srModeler* g_modeler_65963c;
extern srScene* g_scene_user;
extern srScene* g_scene_fullscreen;
extern srScene* g_scene_permanent;
extern srScene* g_scene_prerender0;
extern srScene* g_scene_prerender1;
extern srScene* g_scene_overlay0;
extern srScene* g_scene_overlay1;
extern srScene* g_scene_square;
extern srColorSurface* g_primary_color_surface;
void DrawColorSurface(srColorSurface* surface, int x, int y);
srNode* VideoMakePoster(srColorSurfaceIFace* surface, float width, float height,
                        unsigned char additive); /* 0x00424A90 */
extern srCamera* g_overlay_camera;
extern srCamera* g_square_camera;
extern bool g_texture_cache_enabled;
extern srGERD* g_gerd;
/* Secondary renderer device preferred by the offscreen world-render path. */
extern srGERD* g_secondary_gerd;
extern LPDIRECTDRAWSURFACE2 g_primary_surface;
extern stSurface2D* g_surface_node;
extern srMaterial* g_blit_material;
extern srColorSurface* g_mouse_surface;
extern srNode* g_surface_nodes[0x12c0];
extern unsigned char g_block_652ddc[0x12c0];
extern IDirectDraw2* g_direct_draw2;
extern IDirectDrawSurface* g_video_primary_surface1;
extern IDirectDrawSurface2* g_video_primary_surface2;
extern srModelInstance* g_current_model_instance;
extern int g_renderer_mode;
extern int g_overlay_render_mode;
extern int g_paired_render_mode;
extern float g_surface_scale;
extern int g_surface_state_6595dc;
extern int g_surface_state_654ad8;
struct W8ViewportRect {
    int left;
    int top;
    int right;
    int bottom;
};
static_assert(sizeof(W8ViewportRect) == 16, "W8ViewportRect_size");
extern W8ViewportRect g_viewport_6595e8;
extern int g_dirty_tile_count;
extern int g_resident_texture_policy;
extern unsigned char g_world_render_enabled;
extern unsigned char g_world_blacked_out;

void SetResidentTexturePolicy(int policy);
void SetSurfaceScale(float scale);
void SetTextureCacheSize(unsigned long bytes);
void SetSwapInterval(unsigned char enabled);
unsigned char GetRendererModeByte(void);
void SetViewport(int left, int top, int right, int bottom);
/* Scale a 640x480 design-space rect onto the GERD surface and remember it;
   no-ops when the stored bounds already match. */

void SetScaledViewport00425DA0(int left, int top, int right, int bottom);
/* 0x00425C90: same scaled-viewport update; the automap installs its viewport
   through it. */
void SetScaledViewport00425C90(int left, int top, int right, int bottom);
/* Read the stored pixel viewport back out in normalized 0..1 scale. */
void GetScaledViewportBounds(float* left_top, float* right_bottom);
/* Lock the primary GERD buffer and emit one debug wireframe line. */
void DrawBufferLine(long x0, long y0, long x1, long y1, unsigned long* pixel);
unsigned char InitializeRendererSceneObjects(void);
void PurgeInactiveSceneInstances(srScene* scene);
void ResetVideoFrameState(void);
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled);
unsigned char ClearPrimarySurface(void);
void ResetTransientRenderScenes(void);
void ClearVideoDirtyBlocks(void); /* 0x00423150 */
void RenderScene(srScene* scene, srCamera* camera, const int* viewport, char preserve_fog);
void RenderFrame(void);
IDirectDrawSurface2* BeginVideoPresentation(void);
unsigned char FinishVideoPresentation(void);
void PublishLightDirection(const EnvironmentColour* direction);
void GetWorldColour(EnvironmentColour* colour); /* 0x00427290 */
srVector3T<float>* __fastcall SaturateColor004299B0(srVector3T<float>* color);
void ReleaseObject(srClass* object);
void Initialize16BitPixelFormatMasks(void);
unsigned char CreateWizardryWindow(void);
unsigned char InitializePrimaryDirectDrawSurface(void);
unsigned char InitializeVideoDevice(void);
unsigned char OpenRendererWindow(void);
void InvalidateRendererTextureCache(void);
void AssertFailureHandler(const char* expression, const char* file, long line, const char* message);
unsigned char DisableCursorScene(void);
unsigned char EnableCursorScene(void);
void SetOverlayViewport(const int* value);
void SetWorldModelPickingEnabled(char enabled);
bool RendererBufferIsLockable(void);
void SetRendererOption4Enabled(char enabled);
bool HasEnoughFreeDiskSpace(void);
int GetUsedPageFileBytes(void);
srModelInstance* GetPickedModelInstance(void);
void SetPickedModelInstance(srModelInstance* value);
bool IsCursorInsideViewport(void);      /* 0x00428070 */
bool IsCursorImageInsideViewport(void); /* 0x00428030 */

#endif

#endif

extern unsigned char* g_render_options_65a118;
void SetDisplayGamma(float value);
unsigned int GetTotalPhysicalMemory(void);
int GetRendererFamily(void);

#endif
