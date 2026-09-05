/* Wizardry's thin public adapters around the released SGP video-object and
   video-surface managers.  The implementation bodies remain the vendored SGP
   source; these names preserve the first-party callers' recovered ABI. */

#include "wiz8/wiz8_windows.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/render_state.h"
#include "wiz8/sgp_video.h"
#include "Font.h"
#include "himage.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "vsurface.h"

/* Exact relocation-masked match in every comparable Wizardry build. The body
   is owned by the pinned SFI SGP vsurface.c source, not a Wizardry restatement. */
// LIBRARY: WIZ8 0x00402FA0
// ColorFillVideoSurfaceArea

// LIBRARY: WIZ8 0x004045B0
// InternalShadowVideoSurfaceRect

// LIBRARY: WIZ8 0x004048A0
// ShadowVideoSurfaceRect

/* The FileMan existence checks are likewise owned by the pinned SFI SGP
   FileMan.c oracle. The recovery exports pair them exactly (fopen/fclose
   with the library fallback), so retain their linked identities here. */
// LIBRARY: WIZ8 0x00404BF0
// FileExists

// LIBRARY: WIZ8 0x00404C40
// FileExistsNoDB

// LIBRARY: WIZ8 0x00404C80
// FileOpen

// LIBRARY: WIZ8 0x00404FB0
// FileWrite

// LIBRARY: WIZ8 0x00405030
// FileSeek

// LIBRARY: WIZ8 0x004051D0
// DirectoryExists

// LIBRARY: WIZ8 0x004051F0
// MakeFileManDirectory

// LIBRARY: WIZ8 0x00405200
// GetExecutableDirectory

// LIBRARY: WIZ8 0x00405270
// GetFileFirst

// LIBRARY: WIZ8 0x00405300
// GetFileNext

// LIBRARY: WIZ8 0x00405350
// GetFileClose

// LIBRARY: WIZ8 0x00405390
// W32toSGPFileFind

// LIBRARY: WIZ8 0x004054D0
// FileCopy

// LIBRARY: WIZ8 0x004054F0
// FileGetAttributes

// LIBRARY: WIZ8 0x00405550
// FileClearAttributes

// LIBRARY: WIZ8 0x00408AD0
// SoundPlayStreamedFile

// LIBRARY: WIZ8 0x00408EF0
// SoundIsPlaying

// LIBRARY: WIZ8 0x00408F70
// SoundStop

// LIBRARY: WIZ8 0x00409140
// SoundSetFadeVolume

// LIBRARY: WIZ8 0x00409210
// SoundSetVolume

/* Wizardry polls the released SGP stream manager from the frame, load-screen,
   and ambient-sound paths.  Keep the linked body in soundman.c rather than
   restating it as a first-party bridge. */
// LIBRARY: WIZ8 0x004095B0
// SoundServiceStreams

// LIBRARY: WIZ8 0x0040A9A0
// SoundSetMusic

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

// LIBRARY: WIZ8 0x004124A0
// Blt16BPPBufferShadowRect

// LIBRARY: WIZ8 0x00412570
// Blt16BPPBufferShadowRectAlternateTable

/* Wizardry extends the LibraryDataBase record layouts and patch selection,
   while these four released SGP bodies remain exact in the retail image. */
// LIBRARY: WIZ8 0x00412B10
// ShutDownFileDatabase

// LIBRARY: WIZ8 0x00413680
// CreateRealFileHandle

// LIBRARY: WIZ8 0x00413730
// GetLibraryAndFileIDFromLibraryFileHandle

// LIBRARY: WIZ8 0x00413D00
// CompareDirEntryFileNames

/* The FileMan implementation is likewise owned by the pinned SFI SGP oracle.
   The match image currently consumes it through the runtime-shared boundary,
   so retain its original linked identity without cloning its declaration or
   implementation into Wizardry-owned source. */
// LIBRARY: WIZ8 0x00405630
// GetFileManFileTime

/* The stack/list containers are likewise owned by the pinned SFI SGP
   Container.c oracle (release-mode DbgMessage calls compile out). Retain
   their linked identities without restating them. */
// LIBRARY: WIZ8 0x00405970
// CreateStack

// LIBRARY: WIZ8 0x004059B0
// CreateList

// LIBRARY: WIZ8 0x00405A00
// Push

// LIBRARY: WIZ8 0x00405A70
// Pop

// LIBRARY: WIZ8 0x00405AC0
// PeekStack

// LIBRARY: WIZ8 0x00405B00
// DeleteStack

// LIBRARY: WIZ8 0x00405B20
// PeekList

// LIBRARY: WIZ8 0x00405B90
// StoreListNode

// LIBRARY: WIZ8 0x00405C00
// StackSize

// LIBRARY: WIZ8 0x00405C10
// AddtoList

/* The random-number helpers are likewise owned by the pinned SFI SGP
   Random.c oracle (Wizardry inlines the Random call inside Chance). */
// LIBRARY: WIZ8 0x0040EF80
// InitializeRandom

// LIBRARY: WIZ8 0x0040EFA0
// Random

// LIBRARY: WIZ8 0x0040EFE0
// Chance

/* The DirectDraw wrappers are likewise owned by the pinned SFI SGP DirectDraw
   Calls.c oracle, recognizable by their __FILE__ fingerprints and NoOp calls. */
// LIBRARY: WIZ8 0x0040F0B0
// DDCreateSurface

// LIBRARY: WIZ8 0x0040F100
// DDLockSurface

// LIBRARY: WIZ8 0x0040F150
// DDUnlockSurface

// LIBRARY: WIZ8 0x0040F180
// DDGetSurfaceDescription

// LIBRARY: WIZ8 0x0040F1C0
// DDReleaseSurface

// LIBRARY: WIZ8 0x0040F210
// DDRestoreSurface

// LIBRARY: WIZ8 0x0040F230
// DDBltFastSurface

// LIBRARY: WIZ8 0x0040F290
// DDBltSurface

// LIBRARY: WIZ8 0x0040F300
// DDCreatePalette

// LIBRARY: WIZ8 0x0040F340
// DDSetPaletteEntries

// LIBRARY: WIZ8 0x0040F380
// DDGetPaletteEntries

// LIBRARY: WIZ8 0x0040F3C0
// DDReleasePalette

// LIBRARY: WIZ8 0x0040F3E0
// DDSetSurfaceColorKey

/* These are the released Font.c string-width and font-height operations. */
// LIBRARY: WIZ8 0x004068E0
// SetObjectShade

// LIBRARY: WIZ8 0x00406DE0
// GetFontObjectPalette16BPP

// LIBRARY: WIZ8 0x00406DF0
// GetFontObject

// LIBRARY: WIZ8 0x00407010
// StringPixLength

// LIBRARY: WIZ8 0x00407090
// SaveFontSettings

// LIBRARY: WIZ8 0x00407140
// RestoreFontSettings

// LIBRARY: WIZ8 0x004071F0
// GetFontHeight

// LIBRARY: WIZ8 0x00407220
// SetFontDestBuffer

// LIBRARY: WIZ8 0x00407A10
// gprintf_buffer

// LIBRARY: WIZ8 0x00407B80
// mprintf_buffer

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

extern "C" unsigned char VideoIsFullScreen(void)
{
    return g_fullscreen_603c39;
}

extern "C" void VideoGetClientRect(RECT* bounds)
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
