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

#define BUFFER_READY          0x00
#define BUFFER_BUSY           0x01
#define BUFFER_DIRTY          0x02
#define BUFFER_DISABLED       0x03

#define MAX_CURSOR_WIDTH      64
#define MAX_CURSOR_HEIGHT     64
#define VIDEO_NO_CURSOR				0xFFFF

#ifdef __cplusplus
extern "C" {
#endif

extern HWND										ghWindow;

extern BOOLEAN              InitializeVideoManager(HINSTANCE hInstance, UINT16 usCommandShow, void *WindowProc);
extern void                 ShutdownVideoManager(void);
extern void                 SuspendVideoManager(void);
extern BOOLEAN              RestoreVideoManager(void);
extern void                 GetCurrentVideoSettings(UINT16 *usWidth, UINT16 *usHeight, UINT8 *ubBitDepth);
extern void                 InvalidateRegion(INT32 iLeft, INT32 iTop, INT32 iRight, INT32 iBottom, UINT32 uiFlags);
extern LPDIRECTDRAW2        GetDirectDraw2Object(void);
extern LPDIRECTDRAWSURFACE2 GetFrameBufferObject(void);
extern PTR                  LockPrimarySurface(UINT32 *uiPitch);
extern void                 UnlockPrimarySurface(void);
extern PTR                  LockMouseBuffer(UINT32 *uiPitch);
extern void                 UnlockMouseBuffer(void);
extern BOOLEAN              GetPrimaryRGBDistributionMasks(UINT32 *RedBitMask, UINT32 *GreenBitMask, UINT32 *BblueBitMask);
extern void                 PrintScreen(void);
extern unsigned char        g_flag_6596f4;
extern int                  g_screenshot_index_659724;
extern int                  g_screenshot_page_659728;

void												VideoCaptureToggle( void );

enum {
    INVAL_SRC_TRANS = 1,
    PIXEL_DEPTH = 16,
    SCREEN_WIDTH = 640,
    SCREEN_HEIGHT = 480
};

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
static __inline INT32 VideoGetToolTipWidth(void) { return g_help_box_width; }
static __inline INT32 VideoGetToolTipHeight(void) { return g_help_box_height; }
void VideoPositionToolTip(INT32 x, INT32 y);
void VideoRemoveToolTip(void);

void SGPMouseGetPos(POINT* point);
void NoOct(void);
void ClearSurfaceRect(int left, unsigned int top, int right, unsigned int bottom);

#ifdef __cplusplus
}

class srNode;
class srTextureIFace;
class srColorSurfaceIFace;
class srColorSurface;
class srModelInstance;
srNode* Function424BA0(srTextureIFace* texture, float width, float height,
    unsigned char positional_3);
void Function4229E0(void);
void Function4257F0(int value);
void __fastcall PackColour00429700(
    unsigned char* colour, double red, double green, double blue, double alpha);
void Function4255F0(srNode* node, int x, int y, char positional);
unsigned char Function428B90(
    srColorSurface* surface, int* rect, void* source, int source_pitch,
    float* scale_x, float* scale_y, float* mapping_x, float* mapping_y);
srModelInstance* Function424280(
    int* rect, void* source, int source_pitch, srNode* parent, unsigned char overlay);
srModelInstance* MakePolygonBrush(
    srNode* parent, srColorSurfaceIFace* surface,
    double width, double height,
    float mapping_x, float mapping_y,
    float mapping_width, float mapping_height,
    unsigned char overlay);
#endif


float* RotateMatrixAroundAxis0042B910(
    float* matrix, double sine, double cosine, float* axis);

#endif
