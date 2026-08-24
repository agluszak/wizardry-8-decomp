#include "wiz8/dirty_tiles.h"
#include "wiz8/wiz8_windows.h"
#include "wiz8/sr_api.h"
#include "wiz8/video_object_catalog.h"
#include "wiz8/utility.h"

#include <stdlib.h>
#include <string.h>
#include "vobject.h"
#include "vobject_blitters.h"
#include "vsurface.h"

/*
 * Local Code\VideoObjectManager.cpp, named by the three assertions this body
 * embeds at lines 45, 62 and 221.
 *
 * Two tables are indexed here and both are established by this body alone. The
 * slot table is eight bytes per entry: a first-frame index at +0x00 and a
 * signed vertical offset at +0x04, which is read with movsx. The frame table is
 * 0x3c bytes per entry - the decompiler shows the index scaled by 0xf dwords -
 * whose first 0x30 bytes are the path it loads from, then a mode selector, a
 * loaded flag and the surface it draws. The path is what fixes the base at
 * 0x0062C430: reading the blit alone suggests 0x0062C460, because the first
 * fields it touches are the mode and the surface.
 */

#pragma pack(push, 1)

typedef struct W8VideoObjectSlot {
    int first_frame;                      /* 0x00 */
    short y_offset;                       /* 0x04: signed; added to the caller's y */
    unsigned char unknown_06[2];
} W8VideoObjectSlot;                      /* 0x08 */

typedef struct W8VideoFrame {
    /* 0x00: the record opens with its own path, which is why the table base is
       0x0062C430 and not the 0x0062C460 the blit's first two reads suggest.
       0x00549090 builds the load path from here. */
    char path[0x30];
    int mode;                             /* 0x30: zero selects the second blitter */
    unsigned char loaded;                 /* 0x34: cleared until the frame is loaded */
    unsigned char unknown_35[3];
    unsigned int handle;                  /* 0x38: released SGP object/surface handle */
} W8VideoFrame;                           /* 0x3c */

#pragma pack(pop)

static_assert(sizeof(W8VideoObjectSlot) == 8, "W8VideoObjectSlot_size_must_be_8");
static_assert(sizeof(W8VideoFrame) == 0x3c, "W8VideoFrame_size_must_be_0x3c");

extern "C" {

extern unsigned char g_video_objects_ready_650e20;
W8VideoObjectSlot g_video_slots_6448c8[494];
/* The cleanup loop bounds the complete table at 0x00644900: 0x184d0 bytes,
   or 1658 records.  Only the first 566 catalog entries are attributed so far. */
W8VideoFrame g_video_frames_62c430[1658];


/* The two loaders consume the released SGP object and surface request records.
   Their 0x6c and 0x70 sizes account exactly for this function's 0xe0-byte pair
   of stack objects. */
extern char Function405EF0(VOBJECT_DESC* request, unsigned int* handle);
extern char Function402A70(VSURFACE_DESC* request, unsigned int* handle);

void Function549090(int object, int frame);
extern char Function405FF0(int object, unsigned int handle, short y,
                           int a, int b, int c, int d);
extern char Function402ED0(int object, unsigned int handle, short y,
                           int a, int b, int c, int d);

#define VIDEO_OBJECT_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\VideoObjectManager.cpp"

// FUNCTION: WIZ8 0x00549250
void ReleaseLoadedVideoFrames(void)
{
    W8VideoFrame* frame;
    char released;

    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x9c, 0);
    }
    frame = g_video_frames_62c430;
    do {
        if (frame->loaded) {
            if (frame->mode == 0) {
                released = DeleteVideoObject((HVOBJECT)frame->handle);
            } else {
                released = DeleteVideoSurface((HVSURFACE)frame->handle);
            }
            if (!released) {
                srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x85, 0);
            }
            frame->handle = 0;
            frame->loaded = 0;
        }
        ++frame;
    } while (&frame->handle < (unsigned int*)(g_video_frames_62c430 + 1658));
}

/* The complete checked-in catalog remains part of wiz8-a69.  These are the
   exact records consumed by startup, the default cursor and the main menu. */
void InitializeMenuVideoObjectCatalog(void)
{
    static const char* const font_palette_paths[15] = {
        "Data\\Fonts\\PaletteRed.sti",
        "Data\\Fonts\\PaletteGreen.sti",
        "Data\\Fonts\\PalettePurple.sti",
        "Data\\Fonts\\PaletteBlue.sti",
        "Data\\Fonts\\PaletteOrange.sti",
        "Data\\Fonts\\PaletteYellow.sti",
        "Data\\Fonts\\PalettePink.sti",
        "Data\\Fonts\\PaletteBrown.sti",
        "Data\\Fonts\\PaletteWhite.sti",
        "Data\\Fonts\\PaletteRust.sti",
        "Data\\Fonts\\PaletteBronze.sti",
        "Data\\Fonts\\PaletteGray.sti",
        "Data\\Fonts\\PaletteBeige.sti",
        "Data\\Fonts\\PaletteOptGreen.sti",
        "Data\\Fonts\\PaletteOptWhite.sti"
    };
    static const char* const party_generation_paths[10] = {
        "Data\\Party Generation\\back.sti",
        "Data\\Party Generation\\listboxes.sti",
        "Data\\Party Generation\\detailbox.sti",
        "Data\\Party Generation\\bottombuttons.sti",
        "Data\\Party Generation\\listbuttons.sti",
        "Data\\Party Generation\\scrollbuttons.sti",
        "Data\\Party Generation\\portraitborder.sti",
        "Data\\Party Generation\\portraithighlight.sti",
        "Data\\Party Generation\\options_back.sti",
        "Data\\Party Generation\\import_addon.sti"
    };
    unsigned int index;

    strcpy(g_video_frames_62c430[0].path, "Data\\Cursors\\2D-Cursors.sti");

    g_video_slots_6448c8[0xe8].first_frame = 0x1c5;
    g_video_slots_6448c8[0xe9].first_frame = 0x1c6;
    g_video_slots_6448c8[0xea].first_frame = 0x1c7;
    g_video_slots_6448c8[0xeb].first_frame = 0x1c8;
    g_video_slots_6448c8[0xec].first_frame = 0x1c9;
    g_video_slots_6448c8[0xed].first_frame = 0x1ca;

    strcpy(g_video_frames_62c430[0x1c5].path,
           "Data\\Options\\intro_bg_text.sti");
    g_video_frames_62c430[0x1c5].mode = 1;
    strcpy(g_video_frames_62c430[0x1c6].path,
           "Data\\Options\\intro_bg_credits.sti");
    g_video_frames_62c430[0x1c6].mode = 1;
    strcpy(g_video_frames_62c430[0x1c7].path,
           "Data\\Options\\introtext_normal.sti");
    strcpy(g_video_frames_62c430[0x1c8].path,
           "Data\\Options\\introtext_depressed.sti");
    strcpy(g_video_frames_62c430[0x1c9].path,
           "Data\\Options\\introtext_highlight.sti");
    strcpy(g_video_frames_62c430[0x1ca].path,
           "Data\\Options\\introtext_unavailable.sti");

    /* Object 0x1e5 is the 15-frame palette catalog consumed by the font
       initializer.  The paths are initialized data at frame records
       0x227..0x235 in the retail image. */
    g_video_slots_6448c8[0x1e5].first_frame = 0x227;
    for (index = 0; index != 15; ++index) {
        strcpy(g_video_frames_62c430[0x227 + index].path,
               font_palette_paths[index]);
    }

    /* The contiguous object range 0xfa..0x103 is the Party Generation screen
       catalog consumed by the recovered state-5 controller.  Retail maps it
       one-for-one onto frame records 0x1d7..0x1e0; all ten are ETRLE objects. */
    for (index = 0; index != 10; ++index) {
        g_video_slots_6448c8[0xfa + index].first_frame = 0x1d7 + index;
        strcpy(g_video_frames_62c430[0x1d7 + index].path,
               party_generation_paths[index]);
    }

    /* Object 0x1b6 is the flat 8-bit fill surface used by state-5 panels.
       Retail maps it to frame 0x262 and loads it through the surface path. */
    g_video_slots_6448c8[0x1b6].first_frame = 0x262;
    strcpy(g_video_frames_62c430[0x262].path,
           "Data\\Main Interface\\basic_fill_texture.pcx");
    g_video_frames_62c430[0x262].mode = 1;
}

// FUNCTION: WIZ8 0x00548f90
void Function548F90(int target, int object, int frame, short y,
                    int a5, int a6, int a7, int a8)
{
    W8VideoObjectSlot* slot;
    short row;
    unsigned int surface;
    char ok;

    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x2d, 0);
    }
    Function549090(object, frame);
    /* The slot address is held; the frame index is not. The original recomputes
       first_frame + frame for each of the two frame reads rather than keeping
       it, and the vertical offset is added to the caller's row in sixteen bits -
       both are shorts and the original adds them as such. */
    slot = &g_video_slots_6448c8[object];
    row = slot->y_offset + y;
    surface = g_video_frames_62c430[slot->first_frame + frame].handle;
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames_62c430[slot->first_frame + frame].mode == 0) {
        ok = Function405FF0(target, surface, row, a5, a6, a7, a8);
    } else {
        ok = Function402ED0(target, surface, row, a5, a6, a7, 0);
    }
    if (!ok) {
        srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x3e, 0);
    }
}


/* Loads one frame's surface the first time it is drawn. The path comes out of
   the frame record itself, and the mode picks which loader receives it. A
   failure does not return - it formats the path and the mode into the
   assertion's message. */
// FUNCTION: WIZ8 0x00549090
void Function549090(int object, int frame)
{
    VOBJECT_DESC request_a;
    VSURFACE_DESC request_b;
    W8VideoFrame* record;
    unsigned int handle;
    char loaded_ok;

    /* Two nested checks, both in the original: the assertion does not return,
       so the inner one is reachable only when it is compiled out. */
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0x4c, 0);
        if (!g_video_objects_ready_650e20) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xd4, 0);
        }
    }
    record = &g_video_frames_62c430[g_video_slots_6448c8[object].first_frame + frame];
    if (record->loaded == 0) {
        if (!g_video_objects_ready_650e20) {
            srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
        }
        if (record->mode == 0) {
            request_a.fCreateFlags = VOBJECT_CREATE_FROMFILE;
            strcpy(request_a.ImageFile, record->path);
            loaded_ok = Function405EF0(&request_a, &handle);
        } else {
            request_b.fCreateFlags = VSURFACE_CREATE_FROMFILE;
            strcpy(request_b.ImageFile, record->path);
            loaded_ok = Function402A70(&request_b, &handle);
        }
        if (loaded_ok == 0) {
            if (!g_video_objects_ready_650e20) {
                srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
            }
            srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x68,
                         FormatString("LoadVideoObject: ERROR - Add %s failed, type %d",
                                      record->path, record->mode));
        }
        record->handle = handle;
        record->loaded = 1;
    }
}

/* Copies the loaded frame's released-SGP palette into an owned 256-entry
   table.  The allocation is intentionally retained when the source API says
   the object has no palette, matching the shipped failure path. */
// FUNCTION: WIZ8 0x005492e0
unsigned short* Function5492E0(int object, int frame)
{
    unsigned short* palette;

    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xaf, 0);
    }
    palette = (unsigned short*)malloc(0x200);
    if (!palette) {
        return 0;
    }
    Function549090(object, frame);
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xe6, 0);
    }
    if (!CopyVideoObjectPalette16BPP(
            g_video_frames_62c430[
                g_video_slots_6448c8[object].first_frame + frame].handle,
            palette)) {
        return 0;
    }
    return palette;
}

// FUNCTION: WIZ8 0x00549390
unsigned int GetCatalogVideoObjectHandle(int object, int frame)
{
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xe6, 0);
    }
    Function549090(object, frame);
    return g_video_frames_62c430[
        g_video_slots_6448c8[object].first_frame + frame].handle;
}

// FUNCTION: WIZ8 0x005493e0
short GetCatalogVideoObjectYOffset(int object)
{
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xf2, 0);
    }
    return g_video_slots_6448c8[object].y_offset;
}

/* Mark the complete rectangle covered by one catalog image. ETRLE-backed
   objects supply subimage dimensions through the canonical SGP object API;
   surface-backed objects expose the dimensions on the canonical SGP surface
   record returned by GetVideoSurface. */
// FUNCTION: WIZ8 0x005494f0
void Function5494F0(int object, int frame, int image,
                    int left, int top, int flags)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;
    unsigned short width = 0;
    unsigned short height = 0;

    Function549090(object, frame);
    slot = &g_video_slots_6448c8[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames_62c430[slot->first_frame + frame];
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xdd, 0);
    }
    if (record->mode == 0) {
        GetVideoObjectETRLESubregionProperties(
            record->handle, subimage, &width, &height);
    }
    else {
        HVSURFACE surface;
        if (GetVideoSurface(&surface, record->handle)) {
            height = surface->usHeight;
            width = surface->usWidth;
        }
    }

    if (width != 0 && height != 0) {
        MarkScreenRectDirty(
            left, top, left + width, top + height, flags);
    }
}

/* Draw a catalog video object, then mark the area it covered. The vertical
   argument is truncated to a short for the draw and passed whole to the mark,
   and the seventh reaches the mark only as whether it equals two. */
// FUNCTION: WIZ8 0x00549600
void Function549600(int target, int object, int frame, int y,
                    int a5, int a6, int a7, int a8)
{
    Function548F90(target, object, frame, (short)y, a5, a6, a7, a8);
    Function5494F0(object, frame, y, a5, a6, a7 == 2);
}

/* Loads the selected catalog frame and returns the dimensions of one of its
   ETRLE subimages. Surface-backed records have no ETRLE table, so the retail
   body intentionally leaves the caller's outputs untouched for them. */
// FUNCTION: WIZ8 0x00549660
void Function549660(int object, int frame, int image,
                    short* width, short* height)
{
    W8VideoObjectSlot* slot;
    W8VideoFrame* record;
    short subimage;

    Function549090(object, frame);
    slot = &g_video_slots_6448c8[object];
    subimage = slot->y_offset + image;
    record = &g_video_frames_62c430[slot->first_frame + frame];
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xdd, 0);
    }
    if (record->mode == 0) {
        GetVideoObjectETRLESubregionProperties(
            record->handle, subimage,
            (unsigned short*)width, (unsigned short*)height);
    }
}

/* Copy a rectangle from one catalog-owned 8-bit surface into a 16-bit target.
   The source rectangle has the destination's extent and begins at the two
   caller-provided source coordinates. Both surfaces remain locked for exactly
   the pinned SGP conversion call. */
// FUNCTION: WIZ8 0x005497c0
unsigned char Function5497C0(int target, int left, int top,
                             int right, int bottom, int object,
                             int source_x, int source_y)
{
    SGPRect source_rect;
    HVSURFACE source_surface;
    unsigned int target_pitch;
    unsigned int source_pitch;
    unsigned int source_handle;
    unsigned char* target_pixels;
    unsigned char* source_pixels;

    source_rect.iLeft = source_x;
    source_rect.iTop = source_y;
    source_rect.iRight = source_x + right - left;
    source_rect.iBottom = source_y + bottom - top;

    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0x19d, 0);
    }
    Function549090(object, 0);
    source_handle =
        g_video_frames_62c430[g_video_slots_6448c8[object].first_frame].handle;
    GetVideoSurface(&source_surface, source_handle);
    target_pixels = LockVideoSurface(target, &target_pitch);
    source_pixels = LockVideoSurface(source_handle, &source_pitch);
    Blt8BPPDataSubTo16BPPBuffer(
        reinterpret_cast<unsigned short*>(target_pixels), target_pitch,
        source_surface, source_pixels, source_pitch, left, top, &source_rect);
    UnLockVideoSurface(target);
    UnLockVideoSurface(source_handle);
    return 1;
}

}
