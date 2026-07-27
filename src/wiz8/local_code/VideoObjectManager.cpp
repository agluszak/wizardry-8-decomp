#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

#include <string.h>

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
    void* surface;                        /* 0x38 */
} W8VideoFrame;                           /* 0x3c */

#pragma pack(pop)

typedef char W8VideoObjectSlot_size_must_be_8[sizeof(W8VideoObjectSlot) == 8 ? 1 : -1];
typedef char W8VideoFrame_size_must_be_0x3c[sizeof(W8VideoFrame) == 0x3c ? 1 : -1];

extern "C" {

extern unsigned char g_video_objects_ready_650e20;
extern W8VideoObjectSlot g_video_slots_6448c8[];
extern W8VideoFrame g_video_frames_62c430[];


/* Two loaders, chosen by the frame's mode. Each takes a request whose first
   field is 0x40 and whose path follows it inline, and hands back a surface. */
typedef struct W8VideoLoadRequestA {
    int kind;
    char path[104];
} W8VideoLoadRequestA;

typedef struct W8VideoLoadRequestB {
    int kind;
    char path[96];
} W8VideoLoadRequestB;

extern char Function405EF0(W8VideoLoadRequestA* request, void** surface);
extern char Function402A70(W8VideoLoadRequestB* request, void** surface);

void Function549090(int object, int frame);
extern char Function405FF0(int object, void* surface, short y, int a, int b, int c, int d);
extern char Function402ED0(int object, void* surface, short y, int a, int b, int c, int d);

#define VIDEO_OBJECT_MANAGER_CPP "C:\\Projects\\Wizardry 8\\Local Code\\VideoObjectManager.cpp"

// FUNCTION: WIZ8 0x00548F90
void Function548F90(int target, int object, int frame, short y,
                    int a5, int a6, int a7, int a8)
{
    W8VideoObjectSlot* slot;
    short row;
    void* surface;
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
    surface = g_video_frames_62c430[slot->first_frame + frame].surface;
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames_62c430[slot->first_frame + frame].mode == 0) {
        ok = Function405FF0(object, surface, row, a5, a6, a7, a8);
    } else {
        ok = Function402ED0(object, surface, row, a5, a6, a7, 0);
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
    void* surface;
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
            loaded_ok = Function405EF0(&request_a, &surface);
        } else {
            request_b.kind = 0x40;
            strcpy(request_b.path, record->path);
            loaded_ok = Function402A70(&request_b, &surface);
        }
        if (loaded_ok == 0) {
            if (!g_video_objects_ready_650e20) {
                srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
            }
            srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x68,
                         FormatString("LoadVideoObject: ERROR - Add %s failed, type %d",
                                      record->path, record->mode));
        }
        record->surface = surface;
        record->loaded = 1;
    }
}

}
