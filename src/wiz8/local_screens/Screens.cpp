#include "wiz8/gameplay_boundaries.h"
#include "wiz8/cursor.h"
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

extern int Function55CE80(int item_id);
// FUNCTION: WIZ8 0x0055ef90
void Function55EF90(void)
{
    int object;
    short y_offset;
    unsigned int handle;

    if ((g_screen_state_0068ec78.id == 7 || g_screen_state_0068ec78.id == 6) && g_item_in_hand_shown_006874ca) {
        if (g_item_in_hand.item_id != -1) {
            g_item_in_hand_shown_006874ca = 1;
            object = Function55CE80(g_item_in_hand.item_id);
            y_offset = GetCatalogVideoObjectYOffset(object, 0, 0);
            handle = GetCatalogVideoObjectHandle(object, 0, y_offset);
            SetMouseCursorFromVideoObject(handle, 0, 0, y_offset);
            y_offset = GetCatalogVideoObjectYOffset(0, 0, 0);
            handle = GetCatalogVideoObjectHandle(0, 0, y_offset);
            BlitToMouseCursor(handle, 0, 0, y_offset);
            RefreshMouseCursorTexture();
            g_cursor_state_00683fdb = 7;
            return;
        }
    }
    else if (g_cursor_state_00683fdb != -1) {
        y_offset = GetCatalogVideoObjectYOffset(0, 0, 0);
        handle = GetCatalogVideoObjectHandle(0, 0, y_offset);
        SetMouseCursorFromVideoObject(handle, 0, 0, y_offset);
        RefreshMouseCursorTexture();
        g_cursor_state_00683fdb = -1;
        g_dword_683fdf = 0;
        g_dword_683fe3 = 0;
    }
}

}
