#include "wiz8/gameplay_boundaries.h"
#include "wiz8/cursor.h"
#include "wiz8/item_video_object_vector.h"
#include "wiz8/video_object_catalog.h"

/*
 * Local Screens\Screens.cpp.
 *
 * Keeps the held-item cursor in step with the screen. On the two screens that
 * allow an item in hand it installs that item's cursor; anywhere else it puts
 * the default cursor back and forgets what was held. The screen ids and the
 * cursor ids are the numbers the original uses, and nothing here names them.
 */

extern "C" {
extern unsigned char g_item_in_hand_shown_006874ca;
extern int g_cursor_state_00683fdb;
extern int g_dword_683fdf;
extern int g_dword_683fe3;

// FUNCTION: WIZ8 0x0055ef90
void UpdateHeldItemCursor(void)
{
    int object;

    if ((g_screen_state_0068ec78.id == 7 || g_screen_state_0068ec78.id == 6) && g_item_in_hand_shown_006874ca) {
        if (g_item_in_hand.item_id != -1) {
            g_item_in_hand_shown_006874ca = 1;
            object = g_item_video_objects_68ec68.GetOrCreateVideoObject(
                g_item_in_hand.item_id);
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

}
