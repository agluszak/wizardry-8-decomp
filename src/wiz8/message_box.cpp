#include "wiz8/gameplay_boundaries.h"

#include <string.h>

// FUNCTION: WIZ8 0x00528a80
void AddMessageBoxLine(int type, W8WideChar* text, void* extra)
{
    W8MessageBoxLine* line = new W8MessageBoxLine;
    int new_capacity;
    int index;

    memset(line, 0, sizeof(W8MessageBoxLine));
    line->unknown_00 = -1;
    line->type = type;
    line->text = text;
    line->extra = extra;
    line->sequence = g_message_sequence;

    new_capacity = g_message_box_line_count + 1;
    if (new_capacity > g_message_box_line_capacity) {
        W8MessageBoxLine** old_lines = g_message_box_lines;

        g_message_box_lines = new W8MessageBoxLine*[new_capacity];
        if (g_message_box_lines == 0) {
            g_message_box_lines = old_lines;
            return;
        }
        g_message_box_line_capacity = new_capacity;
        for (index = 0; index < g_message_box_line_count; ++index) {
            g_message_box_lines[index] = old_lines[index];
        }
        delete[] old_lines;
    }

    g_message_box_lines[g_message_box_line_count] = line;
    ++g_message_box_line_count;
}
