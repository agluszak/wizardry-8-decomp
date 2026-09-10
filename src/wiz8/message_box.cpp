#include "wiz8/message_box.h"

#include <string.h>

// GLOBAL
W8MessageBoxLine** g_message_box_lines;
// GLOBAL
int g_message_box_line_count;
// GLOBAL: WIZ8 0x0068c4c4
int g_message_box_line_capacity;
// GLOBAL
int g_message_sequence;
