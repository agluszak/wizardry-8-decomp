#include "wiz8/wiz8_windows.h"

#include <ddraw.h>

#include "wiz8/gameplay_boundaries.h"

/*
 * Fills a rectangle on one of the managed surfaces.
 *
 * The four negative ids name the surfaces the module owns directly; anything
 * else is looked up in the list at 0x00650DBC. The rectangle is clipped against
 * the current clip rect before the blit, and an empty result is not an error -
 * it simply reports nothing drawn.
 */

extern "C" {

typedef struct W8ManagedSurface {
    void* surface;                        /* 0x00 */
    int id;                               /* 0x04 */
    struct W8ManagedSurface* next;        /* 0x08 */
} W8ManagedSurface;

typedef struct W8SurfaceSlot {
    unsigned char unknown_00[2];
    /* 0x02: zero-extended into the effects block alongside the caller's value.
       Which of the two the blit treats as the fill is not settled here. */
    unsigned short word_02;
    unsigned char unknown_04[4];
    IDirectDrawSurface2* surface;         /* 0x08: the blit target */
} W8SurfaceSlot;

extern W8SurfaceSlot* g_surface_650dd4;
extern W8SurfaceSlot* g_surface_650dd8;
extern W8SurfaceSlot* g_primary_surface_view_650ddc;
extern W8SurfaceSlot* g_surface_650de0;
extern W8ManagedSurface* g_surface_list_650dbc;

extern void GetClipRect(int* rect);
extern int DDBltSurface(IDirectDrawSurface2* target, RECT* area,
                        IDirectDrawSurface2* source, RECT* source_area,
                        unsigned int flags, DDBLTFX* effects);

// FUNCTION: WIZ8 0x00402FA0
unsigned char FillSurfaceRect(int surface_id, int left, int top, int right, int bottom,
                              int colour)
{
    RECT area;
    DDBLTFX effects;
    int clip[4];
    W8SurfaceSlot* slot;
    W8ManagedSurface* entry;

    slot = g_surface_650dd4;
    if (surface_id != -0x10 && (slot = g_surface_650dd8, surface_id != -0xf)
        && (slot = g_primary_surface_view_650ddc, surface_id != -0xe)
        && (slot = g_surface_650de0, entry = g_surface_list_650dbc, surface_id != -0xd)) {
        for (; entry != 0; entry = entry->next) {
            if (entry->id == surface_id) {
                slot = (W8SurfaceSlot*)entry->surface;
                goto found;
            }
        }
        return 0;
    }

found:
    /* Nested rather than flattened into early returns: the original tests each
       edge inside the previous one, so a rejection falls all the way out to the
       single failure return at the end. */
    GetClipRect(clip);
    if (top < clip[0]) {
        top = clip[0];
    }
    if (top <= clip[2]) {
        if (clip[2] < bottom) {
            bottom = clip[2];
        }
        if (clip[0] <= bottom) {
            if (left < clip[1]) {
                left = clip[1];
            }
            if (left <= clip[3]) {
                if (clip[3] < right) {
                    right = clip[3];
                }
                if (clip[1] <= right && top < bottom && left < right) {
                    effects.dwSize = 100;
                    effects.dwDDFX = 0;
                    effects.dwROP = 0;
                    effects.dwDDROP = 0;
                                    effects.dwFillColor = colour & 0xffff;
                    area.left = top;
                    area.top = left;
                    area.right = bottom;
                    area.bottom = right;
                    DDBltSurface(slot->surface, &area, 0, 0, 0x400, &effects);
                    return 1;
                }
            }
        }
    }
    return 0;
}


}
