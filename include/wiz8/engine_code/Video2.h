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
extern unsigned char g_flag_6596f4;
/* 0x00652DA4: set while the swaying camera view is active; see
   SetCameraSwayMode in 3dapi.cpp. */
extern unsigned char g_camera_sway_active_652da4;
extern int g_screenshot_index_659724;
extern int g_screenshot_page_659728;
void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom);
#ifdef __cplusplus

extern const float g_scale_x_5ebb1c;
extern const float g_scale_y_5ebb20;

class srColorSurface;
class srNode;
class srTextureIFace;
class stModelInstance2D;
template <class T> class srVector3T;
template <class T> class srVector4T;
/* 0x00425190: build a 2D marker model instance over a texture. */
stModelInstance2D* Function425190(srTextureIFace* texture, double width, double height,
                                  char keep_aspect, char a5);
/* 0x00426F80: render the world into a caller-owned color surface through a
   scissored viewport, then blit the locked frame buffer onto the target. */
unsigned char Function426F80(srColorSurface* target, W8ScreenRect* rect, char render_secondary);

struct W8ControlsRect;
/* 0x00424790: build a solid-color quad sprite; width/height are pixel counts
   and `color` becomes the material's emissive vector. MGSRadarMap's blip
   templates are its observed callers. */
stModelInstance2D* Function424790(int width, int height, const srVector4T<float>* color, char a4);
/* 0x004253F0: the render-target sprite factory CreateSpriteFromSurface wraps; the
   radar overlay is created through it directly. */
stModelInstance2D* Function4253F0(int target, const W8ControlsRect* bounds, int a3, int a4,
                                  char a5);
/* 0x004255C0: wrap the sprite-surface factory - image is a video surface
   handle (or a negative target id), rect an optional source rectangle. */
stModelInstance2D* CreateSpriteFromSurface(unsigned int image, const W8ControlsRect* rect, int mode,
                                           int arg_4, int arg_5);
/* 0x004257D0: position a 2D node without pixel snapping. */
void Position2DNodeUnsnapped004257D0(srNode* node, int x, int y);
/* 0x004264F0: write the display-state byte of a 2D model instance. */
void SetModelInstance2DDisplayState004264F0(stModelInstance2D* object, unsigned char state);
/* 0x00425840: rotate an srNode in degrees and invalidate the renderer mode. */
void Function425840(srNode* node, int degrees);
/* 0x00427E70: surface-lock helper used while installing a drag cursor. */
int Function427E70(void);
/* 0x004255F0: place a 2D node at a screen position in normalized
   coordinates; positional snaps to the renderer's pixel grid. */
void PositionToolTipNode(srNode* node, int x, int y, char positional);
/* 0x00428AA0: mark both renderer mode words dirty. */
void SetRendererModePair(void);
/* 0x004215E0: point-visibility query the radar item loop consults. */
bool HasCameraLineOfSight(const srVector3T<float>* position);
/* 0x00428910 / 0x004289C0 / 0x004289E0: the render-probe bracket the region
   link builder uses to count a mesh's drawn faces — begin the probe pass,
   draw the node and return its covered-face count, then end the pass. */
void BeginRenderProbe00428910(void);
unsigned int MeasureNodeRender004289E0(srNode* node);
void EndRenderProbe004289C0(void);

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

extern int g_pixel_format_603c48;
extern unsigned char g_fullscreen_603c39;
extern unsigned char g_flag_659711;
extern unsigned char g_flag_65970f;
extern unsigned char g_flag_603c60;
extern unsigned char g_flag_603c4c;
extern const int* g_value_659668;
extern srModeler* g_modeler_65963c;
extern srScene* g_scene_user_659640;
extern srScene* g_scene_fullscreen_659644;
extern srScene* g_scene_permanent_659648;
extern srScene* g_scene_prerender0_65964c;
extern srScene* g_scene_prerender1_659650;
extern srScene* g_scene_overlay0_659654;
extern srScene* g_scene_overlay1_659658;
extern srScene* g_scene_square_65965c;
extern srColorSurface* g_primary_color_surface_659660;
void DrawColorSurface00425590(srColorSurface* surface, int x, int y);
extern srCamera* g_overlay_camera_659670;
extern srCamera* g_square_camera_659674;
extern unsigned char g_flag_65beaf;
extern srGERD* g_gerd_659634;
extern LPDIRECTDRAWSURFACE2 g_primary_surface_6596a8;
extern stSurface2D* g_surface_node_659664;
extern srMaterial* g_blit_material_65967c;
extern srColorSurface* g_mouse_surface_659688;
extern srNode* g_surface_nodes_654adc[0x12c0];
extern unsigned char g_block_652ddc[0x12c0];
extern IDirectDraw2* g_direct_draw2_6596a0;
extern IDirectDrawSurface* g_video_primary_surface1_6596ac;
extern IDirectDrawSurface2* g_video_primary_surface2_6596b0;
extern srModelInstance* g_current_model_instance_65962c;
extern int g_renderer_mode_603d74;
extern int g_dword_6596ec;
extern int g_dword_6596f0;
extern float g_surface_scale_659680;
extern int g_surface_state_6595dc;
extern int g_surface_state_654ad8;
extern int g_viewport_left_6595e8;
extern int g_viewport_top_6595ec;
extern int g_viewport_right_6595f0;
extern int g_viewport_bottom_6595f4;
extern int g_dword_6596d8;
extern int g_resident_texture_policy_659714;
extern unsigned char g_flag_65970d;

void SetResidentTexturePolicy(int policy);
void SetSurfaceScale004297E0(float scale);
void SetTextureCacheSize00426740(unsigned long bytes);
void SetSwapInterval00426710(unsigned char enabled);
unsigned char GetRendererModeByte(void);
void SetViewport(int left, int top, int right, int bottom);
/* Scale a 640x480 design-space rect onto the GERD surface and remember it;
   no-ops when the stored bounds already match. */
void SetScaledViewport00425DA0(int left, int top, int right, int bottom);
unsigned char InitializeRendererSceneObjects(void);
void PurgeInactiveSceneInstances(srScene* scene);
void ResetVideoFrameState00422B10(void);
void SetPrimarySurfaceTextureHint2Enabled(unsigned char enabled);
unsigned char ClearPrimarySurface(void);
void ResetTransientRenderScenes(void);
void RenderScene(srScene* scene, srCamera* camera, const int* viewport, char preserve_fog);
void RenderFrame(void);
IDirectDrawSurface2* BeginVideoPresentation(void);
unsigned char FinishVideoPresentation(void);
void PublishLightDirection(const EnvironmentColour* direction);
srVector3T<float>* __fastcall SaturateColor004299B0(srVector3T<float>* color);
void ReleaseObject004257F0(srClass* object);
void Initialize16BitPixelFormatMasks(void);
unsigned char CreateWizardryWindow(void);
unsigned char InitializePrimaryDirectDrawSurface(void);
unsigned char InitializeVideoDevice(void);
unsigned char OpenRendererWindow(void);
void InvalidateRendererTextureCache(void);
void AssertFailureHandler(const char* expression, const char* file, long line, const char* message);
unsigned char ClearFlag603C60(void);
unsigned char SetFlag603C60(void);
void SetValue659668(const int* value);
void SetWorldModelPickingEnabled(char enabled);
unsigned char RendererBufferIsLockable(void);
void SetRendererOption4Enabled(char enabled);
unsigned char HasEnoughFreeDiskSpace(void);
int GetUsedPageFileBytes(void);
srModelInstance* GetValue65962C(void);
void SetValue65962C(srModelInstance* value);

#endif

#endif

extern unsigned char* g_render_options_65a118;
void SetRendererMode6596EC(void); /* 0x00428A90 */
void SetDisplayGamma(float value);
unsigned int GetTotalPhysicalMemory(void);
int GetRendererFamily(void);

#endif
