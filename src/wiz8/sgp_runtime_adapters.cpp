/* Wizardry's thin public adapters around the released SGP video-object and
   video-surface managers.  The implementation bodies remain the vendored SGP
   source; these names preserve the first-party callers' recovered ABI. */

#include "wiz8/wiz8_windows.h"

extern "C" {

struct W8VideoObjectRequest {
    unsigned int flags;
    char path[104];
};

struct W8VideoSurfaceRequest {
    unsigned int flags;
    char path[108];
};

extern unsigned char AddStandardVideoObject(
    W8VideoObjectRequest* request, unsigned int* index);
extern unsigned char AddStandardVideoSurface(
    W8VideoSurfaceRequest* request, unsigned int* index);
extern unsigned char BltVideoObjectFromIndex(
    unsigned int destination, unsigned int source, unsigned short region,
    int x, int y, unsigned int flags, void* effects);
extern unsigned char BltVideoSurface(
    unsigned int destination, unsigned int source, unsigned short region,
    int x, int y, unsigned int flags, void* effects);
extern HWND ghWindow;
extern unsigned char g_fullscreen_603c39;
extern IDirectDraw2* g_direct_draw2_6596a0;
extern IDirectDrawSurface2* g_primary_surface_6596a8;
extern unsigned short g_red_mask_650f4a;
extern unsigned short g_green_mask_650f4c;
extern unsigned short g_blue_mask_650f4e;
extern unsigned short g_alpha_mask_650f48;
extern unsigned short g_red_shift_650f50;
extern unsigned short g_green_shift_650f54;
extern unsigned short g_blue_shift_650f52;
extern int g_screen_width_603c3c;
extern int g_screen_height_603c40;
extern int g_screen_depth_603c44;
extern void* LockPrimarySurface(int* pitch);
extern void UnlockPrimarySurface(void);
extern void Function422D50(int left, int top, int right, int bottom, int flags);
extern unsigned char gbPixelDepth;
extern unsigned short gusAlphaMask;
extern unsigned short gusRedMask;
extern unsigned short gusGreenMask;
extern unsigned short gusBlueMask;
extern short gusRedShift;
extern short gusGreenShift;
extern short gusBlueShift;
extern unsigned int guiTranslucentMask;

void InitializeSGPPixelFormat(void)
{
    gbPixelDepth = static_cast<unsigned char>(g_screen_depth_603c44);
    gusAlphaMask = g_alpha_mask_650f48;
    gusRedMask = g_red_mask_650f4a;
    gusGreenMask = g_green_mask_650f4c;
    gusBlueMask = g_blue_mask_650f4e;
    gusRedShift = static_cast<short>(g_red_shift_650f50);
    gusGreenShift = static_cast<short>(g_green_shift_650f54);
    gusBlueShift = static_cast<short>(g_blue_shift_650f52);
    guiTranslucentMask = gusGreenMask == 0x03e0 ? 0x3def : 0x7bef;
}

unsigned char VideoIsFullScreen(void)
{
    return g_fullscreen_603c39;
}

void VideoGetClientRect(RECT* bounds)
{
    POINT origin;

    GetClientRect(ghWindow, bounds);
    origin.x = bounds->left;
    origin.y = bounds->top;
    ClientToScreen(ghWindow, &origin);
    OffsetRect(bounds, origin.x, origin.y);
}

BOOL SGPMouseGetPos(LPPOINT position)
{
    return GetCursorPos(position);
}

IDirectDraw2* GetDirectDraw2Object(void)
{
    return g_direct_draw2_6596a0;
}

IDirectDrawSurface2* GetFrameBufferObject(void)
{
    return g_primary_surface_6596a8;
}

IDirectDrawSurface2* GetMouseBufferObject(void)
{
    return g_primary_surface_6596a8;
}

void* LockFrameBuffer(unsigned int* pitch)
{
    int value;
    void* pixels = LockPrimarySurface(&value);
    *pitch = value;
    return pixels;
}

void UnlockFrameBuffer(void)
{
    UnlockPrimarySurface();
}

void* LockMouseBuffer(unsigned int* pitch)
{
    return LockFrameBuffer(pitch);
}

void UnlockMouseBuffer(void)
{
    UnlockFrameBuffer();
}

unsigned char GetPrimaryRGBDistributionMasks(
    unsigned int* red, unsigned int* green, unsigned int* blue)
{
    *red = g_red_mask_650f4a;
    *green = g_green_mask_650f4c;
    *blue = g_blue_mask_650f4e;
    return 1;
}

void GetCurrentVideoSettings(
    unsigned short* width, unsigned short* height, unsigned char* depth)
{
    *width = static_cast<unsigned short>(g_screen_width_603c3c);
    *height = static_cast<unsigned short>(g_screen_height_603c40);
    *depth = static_cast<unsigned char>(g_screen_depth_603c44);
}

void InvalidateRegion(int left, int top, int right, int bottom)
{
    Function422D50(left, top, right, bottom, 0);
}

unsigned char Function405EF0(W8VideoObjectRequest* request, void** handle)
{
    unsigned int index;
    unsigned char ok = AddStandardVideoObject(request, &index);
    *handle = reinterpret_cast<void*>(index);
    return ok;
}

unsigned char Function402A70(W8VideoSurfaceRequest* request, void** handle)
{
    unsigned int index;
    unsigned char ok = AddStandardVideoSurface(request, &index);
    *handle = reinterpret_cast<void*>(index);
    return ok;
}

unsigned char Function405FF0(int destination, void* source, short region,
                             int x, int y, int flags, int effects)
{
    return BltVideoObjectFromIndex(
        destination, reinterpret_cast<unsigned int>(source),
        static_cast<unsigned short>(region), x, y,
        static_cast<unsigned int>(flags), reinterpret_cast<void*>(effects));
}

unsigned char Function402ED0(int destination, void* source, short region,
                             int x, int y, int flags, int effects)
{
    return BltVideoSurface(
        destination, reinterpret_cast<unsigned int>(source),
        static_cast<unsigned short>(region), x, y,
        static_cast<unsigned int>(flags), reinterpret_cast<void*>(effects));
}

}
