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
/* 0x004048A0: fill one rectangle of the target surface through the
   locked primary-surface blitter. */
void Function4048A0(int target, int left, int top, int right, int bottom);

#ifdef __cplusplus

extern const float g_scale_x_5ebb1c;
extern const float g_scale_y_5ebb20;

class srColorSurface;
class srNode;
class srTextureIFace;
class stModelInstance2D;
/* 0x00425190: build a 2D marker model instance over a texture. */
stModelInstance2D* Function425190(srTextureIFace* texture, double width, double height,
                                  char keep_aspect, char a5);
/* 0x00426F80: render the world into a caller-owned color surface through a
   scissored viewport, then blit the locked frame buffer onto the target. */
unsigned char Function426F80(srColorSurface* target, W8ScreenRect* rect, char render_secondary);

struct W8ControlsRect;
/* 0x004255C0: wrap the sprite-surface factory - image is a video surface
   handle (or a negative target id), rect an optional source rectangle. */
stModelInstance2D* Function4255C0(unsigned int image, const W8ControlsRect* rect, char mode,
                                int arg_4, int arg_5);
/* 0x004257D0: position a 2D node without pixel snapping. */
void Function4257D0(srNode* node, int x, int y);
/* 0x004264F0: write the display-state byte of a 2D model instance. */
void Function4264F0(stModelInstance2D* object, unsigned char state);
/* 0x00425840: rotate an srNode in degrees and invalidate the renderer mode. */
void Function425840(srNode* node, int degrees);
/* 0x00427E70: surface-lock helper used while installing a drag cursor. */
int Function427E70(void);
/* 0x004255F0: place a 2D node at a screen position in normalized
   coordinates; positional snaps to the renderer's pixel grid. */
void PositionToolTipNode(srNode* node, int x, int y, char positional);
/* 0x00428AA0: mark both renderer mode words dirty. */
void SetRendererModePair(void);

#endif

void Function4280C0(int x, int y); /* 0x004280C0: warp the system cursor, fullscreen-safe */

#endif
