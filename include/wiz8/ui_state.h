#ifndef WIZ8_UI_STATE_H
#define WIZ8_UI_STATE_H

#include "wiz8/layouts/gameplay_databases.h"

struct W8MessageBoxLine {
    int unknown_00;                       /* initialized to -1 */
    int unknown_04;
    int unknown_08;
    int type;                             /* 0x0c: message category */
    W8WideChar* text;                     /* 0x10 */
    int unknown_14;
    int unknown_18;
    void* extra;                          /* 0x1c: caller-owned payload */
    int sequence;                         /* 0x20: running global counter */
};                                      /* 0x24 */

extern "C" {

extern W8MessageBoxLine** g_message_box_lines;
extern int g_message_box_line_count;
extern int g_message_box_line_capacity;
extern int g_message_sequence;
extern unsigned char* g_main_game_screen_0068f2d4;

}

#endif
