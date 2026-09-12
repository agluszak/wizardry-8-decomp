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
extern int g_screenshot_index_659724;
extern int g_screenshot_page_659728;
void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom);
/* 0x004048A0: fill one rectangle of the target surface through the
   locked primary-surface blitter. */
void Function4048A0(int target, int left, int top, int right, int bottom);

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
extern unsigned char g_monster_shadow_updates_enabled_0065970c;
extern unsigned char g_flag_65970d;

void SetResidentTexturePolicy(int policy);
void SetSurfaceScale004297E0(float scale);
void SetTextureCacheSize00426740(unsigned long bytes);
void SetSwapInterval00426710(unsigned char enabled);
unsigned char GetRendererModeByte(void);
void SetViewport(int left, int top, int right, int bottom);
unsigned char InitializeRendererSceneObjects(void);
void PurgeInactiveSceneInstances(srScene* scene);
void Function422B10(void);
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
