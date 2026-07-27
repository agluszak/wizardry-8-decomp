#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/*
 * Local Code\VideoObjectManager.cpp, named by the three assertions this body
 * embeds at lines 45, 62 and 221.
 *
 * Two tables are indexed here and both are established by this body alone. The
 * slot table is eight bytes per entry: a first-frame index at +0x00 and a
 * signed vertical offset at +0x04, which is read with movsx. The frame table is
 * 0x3c bytes per entry - the decompiler shows the index scaled by 0xf dwords -
 * with a mode selector at +0x00 and the surface it draws at +0x08. Nothing here
 * establishes any other field of either, so the remainder stays opaque.
 */

#pragma pack(push, 1)

typedef struct W8VideoObjectSlot {
    int first_frame;                      /* 0x00 */
    short y_offset;                       /* 0x04: signed; added to the caller's y */
    unsigned char unknown_06[2];
} W8VideoObjectSlot;                      /* 0x08 */

typedef struct W8VideoFrame {
    int mode;                             /* 0x00: zero selects the second blitter */
    unsigned char unknown_04[4];
    void* surface;                        /* 0x08 */
    unsigned char unknown_0c[0x30];
} W8VideoFrame;                           /* 0x3c */

#pragma pack(pop)

typedef char W8VideoObjectSlot_size_must_be_8[sizeof(W8VideoObjectSlot) == 8 ? 1 : -1];
typedef char W8VideoFrame_size_must_be_0x3c[sizeof(W8VideoFrame) == 0x3c ? 1 : -1];

extern "C" {

extern unsigned char g_video_objects_ready_650e20;
extern W8VideoObjectSlot g_video_slots_6448c8[];
extern W8VideoFrame g_video_frames_62c460[];

extern void Function549090(int object, int frame);
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
    surface = g_video_frames_62c460[slot->first_frame + frame].surface;
    if (!g_video_objects_ready_650e20) {
        srAssertFail("VideoObjectsInitialized()", VIDEO_OBJECT_MANAGER_CPP, 0xdd, 0);
    }
    if (g_video_frames_62c460[slot->first_frame + frame].mode == 0) {
        ok = Function405FF0(object, surface, row, a5, a6, a7, a8);
    } else {
        ok = Function402ED0(object, surface, row, a5, a6, a7, 0);
    }
    if (!ok) {
        srAssertFail("fReturnCode", VIDEO_OBJECT_MANAGER_CPP, 0x3e, 0);
    }
}

}
