#include "wiz8/gameplay_boundaries.h"
#include "wiz8/cursor.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/video_object_catalog.h"
#include "Container.h"

#include <string.h>

/*
 * Local Screens\Screens.cpp.
 *
 * Keeps the held-item cursor in step with the screen. On the two screens that
 * allow an item in hand it installs that item's cursor; anywhere else it puts
 * the default cursor back and forgets what was held. The screen ids and the
 * cursor ids are the numbers the original uses, and nothing here names them.
 */

extern "C" {
extern unsigned char g_flag_68edac;
extern void* g_stack_68eda8;

extern char Function5D1AE0(unsigned int command);
extern char Function5D1C00(unsigned int command);
extern void RequestRedraw(unsigned int mask);

class W8ScreenCommandTarget0055EBB0 {
public:
    virtual void vslot0();
    virtual void vslot1(int value);

    int Function55EBB0(unsigned int command);
    int Function55EBE0(unsigned int command);
};

/* Run the first screen command predicate and reset this target through its
   second virtual slot when command zero succeeds. */
// FUNCTION: WIZ8 0x0055EBB0
int W8ScreenCommandTarget0055EBB0::Function55EBB0(unsigned int command)
{
    if (Function5D1AE0(command) != 0) {
        if (static_cast<char>(command) == 0) {
            vslot1(0);
        }
        return 1;
    }
    return 0;
}

/* The parallel path using the second command predicate. */
// FUNCTION: WIZ8 0x0055EBE0
int W8ScreenCommandTarget0055EBB0::Function55EBE0(unsigned int command)
{
    if (Function5D1C00(command) != 0) {
        if (static_cast<char>(command) == 0) {
            vslot1(0);
        }
        return 1;
    }
    return 0;
}

/* Return the requested screen id, falling back to the state at the top of the
   return stack when there is no explicit pending state. */
// FUNCTION: WIZ8 0x0055EC10
int Function55EC10(void)
{
    W8ScreenStateRuntime state;

    if (g_dword_68ed10.id != -1) {
        return g_dword_68ed10.id;
    }
    if (PeekStack(g_stack_68eda8, &state)) {
        return state.id;
    }
    return -1;
}

/* Whether a transition is pending either explicitly or through the frame's
   transition flag. */
// FUNCTION: WIZ8 0x0055EC70
unsigned char Function55EC70(void)
{
    if (g_dword_68ed10.id == -1 && g_flag_68edac == 0) {
        return 0;
    }
    return 1;
}

/* Route one redraw bit to the active camp or main-game screen state. */
// FUNCTION: WIZ8 0x0055EE30
void Function55EE30(unsigned char bit)
{
    if (g_screen_state_0068ec78.id == W8_SCREEN_CAMP) {
        g_camp_screen_0069c0f4->redraw_flags |= 0x100;
    }
    else if (g_screen_state_0068ec78.id == W8_SCREEN_MAIN_GAME) {
        RequestRedraw(1 << (bit & 31));
    }
}

// FUNCTION: WIZ8 0x0055ef90
void UpdateHeldItemCursor(void)
{
    int object;

    if ((g_screen_state_0068ec78.id == 7 || g_screen_state_0068ec78.id == 6) &&
        g_status_685170.item_in_cursor) {
        if (g_status_685170.item_in_hand_235b.item_id != -1) {
            g_status_685170.item_in_cursor = 1;
            object = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                g_status_685170.item_in_hand_235b.item_id);
            SetMouseCursorFromVideoObject(
                GetCatalogVideoObjectHandle(object, 0), 0, 0,
                GetCatalogVideoObjectYOffset(object));
            BlitToMouseCursor(
                GetCatalogVideoObjectHandle(0, 0), 0, 0,
                GetCatalogVideoObjectYOffset(0));
            RefreshMouseCursorTexture();
            g_cursor_state_00683fdb = 7;
            return;
        }
    }
    else if (g_cursor_state_00683fdb != -1) {
        SetMouseCursorFromVideoObject(
            GetCatalogVideoObjectHandle(0, 0), 0, 0,
            GetCatalogVideoObjectYOffset(0));
        RefreshMouseCursorTexture();
        g_cursor_state_00683fdb = -1;
        g_dword_683fdf = 0;
        g_dword_683fe3 = 0;
    }
}

/* Empty the item-in-hand record and restore the normal cursor.  The held item
   is the 0x0c-byte record embedded in gXStatus at 0x006874CB; the byte directly
   before it is the cursor-visible flag. */
// FUNCTION: WIZ8 0x0055f1e0
void ClearHeldItemDisplay(void)
{
    memset(&g_status_685170.item_in_hand_235b, 0,
           sizeof(g_status_685170.item_in_hand_235b));
    g_status_685170.item_in_cursor = 0;
    g_status_685170.item_in_hand_235b.item_id = -1;

    if (g_cursor_state_00683fdb != -1) {
        SetMouseCursorFromVideoObject(
            GetCatalogVideoObjectHandle(0, 0), 0, 0,
            GetCatalogVideoObjectYOffset(0));
        RefreshMouseCursorTexture();
        g_cursor_state_00683fdb = -1;
        g_dword_683fdf = 0;
        g_dword_683fe3 = 0;
    }
}

}
