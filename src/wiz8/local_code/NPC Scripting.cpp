#include "wiz8/character.h"
#include "wiz8/npc_interaction.h"
#include "wiz8/npc_state.h"
#include "wiz8/message_box.h"

#include <string.h>

// GLOBAL: WIZ8 0x0068c4f4
unsigned char g_flag_68c4f4;
// GLOBAL: WIZ8 0x0068c4fa
unsigned char g_flag_68c4fa;
// GLOBAL: WIZ8 0x0068c500
unsigned char g_flag_68c500;

/* Local Code\NPC Scripting.cpp. The NPC-scripting flag gates the scripted
   monster state; the four accessors below are its only owners. */

// FUNCTION: WIZ8 0x00529560
void SetFlag68C4F4(void)
{
    g_flag_68c4f4 = 1;
}
// FUNCTION: WIZ8 0x00529BC0
void SetFlag68C4F7(void)
{
    g_flag_68c4f7 = 1;
}
// FUNCTION: WIZ8 0x00529BD0
void ClearFlag68C4F7(void)
{
    g_flag_68c4f7 = 0;
}
// FUNCTION: WIZ8 0x0052A070
unsigned char GetFlag68C4FA(void)
{
    return g_flag_68c4fa;
}
// FUNCTION: WIZ8 0x0052A1A0
void SetFlag68C500(unsigned char value)
{
    g_flag_68c500 = value;
}

// FUNCTION: WIZ8 0x00525DD0
unsigned char Function525DD0(void)
{
    return g_flag_68c4a0 != 0 || g_flag_68c4f6 != 0;
}

// FUNCTION: WIZ8 0x00525DF0
unsigned char Function525DF0(unsigned char require_group_entry)
{
    if (g_flag_68c4f7 != 0) {
        return 0;
    }
    if (g_flag_68c4a0 == 0 && g_flag_68c4f6 == 0 && g_value_68c4c0 == 0) {
        return 0;
    }
    if (g_npc_state_68c4ac == 0) {
        return 0;
    }
    if (GetNpcGroupEntry(g_npc_state_68c4ac) != 0 && require_group_entry == 0) {
        return 0;
    }
    return 1;
}

// FUNCTION: WIZ8 0x00525E50
bool IsValue68C4C0Clear(void)
{
    return g_value_68c4c0 == 0;
}

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
