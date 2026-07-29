#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <stdlib.h>
#include <string.h>
#include "vobject.h"

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

typedef char W8VideoObjectSlot_size_must_be_8[sizeof(W8VideoObjectSlot) == 8 ? 1 : -1];
typedef char W8VideoFrame_size_must_be_0x3c[sizeof(W8VideoFrame) == 0x3c ? 1 : -1];

extern "C" {

extern unsigned char g_video_objects_ready_650e20;
W8VideoObjectSlot g_video_slots_6448c8[494];
W8VideoFrame g_video_frames_62c430[566];


/* Two loaders, chosen by the frame's mode. Each takes a request whose first
   field is 0x40 and whose path follows it inline, and hands back a surface.
   The two path buffers are not the same size - 104 and 108 - which the frame
   proves: the original reserves 0xe0, and only 4+104 plus 4+108 plus the
   surface pointer adds up to it. The decompiler shows the second as 96 because
   that is as much of it as this body touches. */
typedef struct W8VideoLoadRequestA {
    int kind;
    char path[104];
} W8VideoLoadRequestA;

typedef struct W8VideoLoadRequestB {
    int kind;
    char path[108];
} W8VideoLoadRequestB;

extern char Function405EF0(W8VideoLoadRequestA* request, unsigned int* handle);
extern char Function402A70(W8VideoLoadRequestB* request, unsigned int* handle);

void Function549090(int object, int frame);
extern char Function405FF0(int object, unsigned int handle, short y,
                           int a, int b, int c, int d);
extern char Function402ED0(int object, unsigned int handle, short y,
                           int a, int b, int c, int d);

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
}

#define VIDEO_OBJECT_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\VideoObjectManager.cpp"

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
    W8VideoLoadRequestA request_a;
    W8VideoLoadRequestB request_b;
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
            request_a.kind = 0x40;
            strcpy(request_a.path, record->path);
            loaded_ok = Function405EF0(&request_a, &handle);
        } else {
            request_b.kind = 0x40;
            strcpy(request_b.path, record->path);
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
void* Function549390(int object, int frame)
{
    Function549090(object, frame);
    return (void*)g_video_frames_62c430[
        g_video_slots_6448c8[object].first_frame + frame].handle;
}

// FUNCTION: WIZ8 0x005493e0
short Function5493E0(int object, int fallback)
{
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP,
                     0xf2, 0);
        return g_video_slots_6448c8[fallback].y_offset;
    }
    return g_video_slots_6448c8[object].y_offset;
}

}
