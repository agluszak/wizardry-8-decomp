/* Wizardry's thin public adapters around the released SGP video-object and
   video-surface managers.  The implementation bodies remain the vendored SGP
   source; these names preserve the first-party callers' recovered ABI. */

#include "wiz8/wiz8_windows.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/render_state.h"
#include "wiz8/sgp_video.h"
#include "Font.h"
#include "himage.h"
#include "sgp.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "vsurface.h"

/* Exact relocation-masked match in every comparable Wizardry build. The body
   is owned by the pinned SFI SGP vsurface.c source, not a Wizardry restatement. */
// LIBRARY: WIZ8 0x00402FA0
// ColorFillVideoSurfaceArea

// LIBRARY: WIZ8 0x0040ABF0
// Sound3DPlay

/* These bodies are retained byte-for-byte from the pinned SFI SGP shading.c
   oracle; Wizardry's source model owns only their linked addresses. */
// LIBRARY: WIZ8 0x00413d60
// BuildShadeTable

// LIBRARY: WIZ8 0x00413e80
// SetShadeTablePercent

/* Released SGP flat-surface palette conversion used by VideoObjectManager. */
// LIBRARY: WIZ8 0x00411730
// Blt8BPPDataSubTo16BPPBuffer

/* The FileMan implementation is likewise owned by the pinned SFI SGP oracle.
   The match image currently consumes it through the runtime-shared boundary,
   so retain its original linked identity without cloning its declaration or
   implementation into Wizardry-owned source. */
// LIBRARY: WIZ8 0x00405630
// GetFileManFileTime

/* These are the released Font.c string-width and font-height operations. */
// LIBRARY: WIZ8 0x00407010
// StringPixLength

// LIBRARY: WIZ8 0x004071F0
// GetFontHeight

extern "C" {

extern unsigned char g_fullscreen_603c39;
extern unsigned short g_red_mask_650f4a;
extern unsigned short g_green_mask_650f4c;
extern unsigned short g_blue_mask_650f4e;
extern unsigned short g_alpha_mask_650f48;
extern int g_screen_width_603c3c;
extern int g_screen_height_603c40;
extern int g_screen_depth_603c44;
unsigned char InitializeWiz8FontManager(
    unsigned short pixel_depth, FontTranslationTable* translation)
{
    if (!InitializeFontManager(pixel_depth, translation)) {
        return 0;
    }
    return SetFontDestBuffer(
        0xfffffff2u, 0, 0, g_screen_width_603c3c, g_screen_height_603c40, 0);
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
    UINT32 value;
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
    MarkScreenRectDirty(left, top, right, bottom, 0);
}

unsigned char Function405EF0(VOBJECT_DESC* request, unsigned int* handle)
{
    return AddStandardVideoObject(request, handle);
}

unsigned char Function402A70(VSURFACE_DESC* request, unsigned int* handle)
{
    return AddStandardVideoSurface(request, handle);
}

unsigned char Function405FF0(int destination, unsigned int source, short region,
                             int x, int y, int flags, int effects)
{
    return BltVideoObjectFromIndex(
        destination, source, static_cast<unsigned short>(region), x, y,
        static_cast<unsigned int>(flags), reinterpret_cast<blt_fx*>(effects));
}

unsigned char Function402ED0(int destination, unsigned int source, short region,
                             int x, int y, int flags, int effects)
{
    return BltVideoSurface(
        destination, source, static_cast<unsigned short>(region), x, y,
        static_cast<unsigned int>(flags), reinterpret_cast<blt_vs_fx*>(effects));
}

}
