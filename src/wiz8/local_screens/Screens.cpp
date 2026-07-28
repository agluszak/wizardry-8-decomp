#include "wiz8/gameplay_boundaries.h"

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
extern int Function5493E0(int a, int b, int c);
extern int Function549390(int a, int b, int c);
extern void Function427AB0(int cursor);
extern void Function427FC0(int cursor);
extern void Function427FF0(void);

// FUNCTION: WIZ8 0x0055EF90
void Function55EF90(void)
{
    int surface;
    int frame;
    int cursor;

    if ((g_screen_state_0068ec78.id == 7 || g_screen_state_0068ec78.id == 6) && g_item_in_hand_shown_006874ca) {
        if (g_item_in_hand.item_id != -1) {
            g_item_in_hand_shown_006874ca = 1;
            surface = Function55CE80(g_item_in_hand.item_id);
            frame = Function5493E0(surface, 0, 0);
            cursor = Function549390(surface, 0, frame);
            Function427AB0(cursor);
            frame = Function5493E0(0, 0, 0);
            cursor = Function549390(0, 0, frame);
            Function427FC0(cursor);
            Function427FF0();
            g_cursor_state_00683fdb = 7;
            return;
        }
    }
    else if (g_cursor_state_00683fdb != -1) {
        frame = Function5493E0(0, 0, 0);
        cursor = Function549390(0, 0, frame);
        Function427AB0(cursor);
        Function427FF0();
        g_cursor_state_00683fdb = -1;
        g_dword_683fdf = 0;
        g_dword_683fe3 = 0;
    }
}

}
