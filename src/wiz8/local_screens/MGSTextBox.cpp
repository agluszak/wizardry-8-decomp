#include "wiz8/gameplay_boundaries.h"
#include "wiz8/local_code/Controls.h"

/*
 * Local Screens\MGSTextBox.cpp.
 *
 * The message box on the main game screen. Everything it reads and writes
 * lives on the level runtime block at 0x0068EDCC, which is the same object the
 * item manager and the movement rules reach through - the text state simply
 * occupies a different part of it.
 */

/* The redraw the text box asks for whenever anything it shows changes. */
enum { W8_REDRAW_TEXT_BOX = 0x800 };

extern void RequestRedraw(int mask);                                    /* 0x00562A50 */
extern unsigned char g_text_box_mode_0069b7b8;
extern int g_text_box_value_0064bd54;
extern int g_text_line_cursor_00686905;
/* 0x00689B17: one entry per line, how many messages that line holds. */
extern const int g_text_line_counts[];
extern void ScrollTextBoxTo(int line);                                  /* 0x0058BBC0 */
/* 0x0068F2D4: the screen the text box belongs to; its two panels sit at 0x0c
   and 0x14. */
extern void RedrawTextBoxBody(void);                                    /* 0x00588E60 */

/* How many lines the text box can still be scrolled through. */
// FUNCTION: WIZ8 0x0058fb30
int GetTextBoxScrollRange(void)
{
    return g_level_block->scroll_bottom - g_level_block->scroll_top;
}

/* One entry of the second slot table. */
// FUNCTION: WIZ8 0x0058fa60
int GetTextSlot1E8(int index)
{
    return g_level_block->text_slots_1e8[index];
}

/* Empty one entry of either slot table and ask for a redraw. The two bodies
   differ only in which table they clear, which is what pairs them. */
// FUNCTION: WIZ8 0x0058f960
void ClearTextSlot1D8(int index)
{
    g_level_block->text_slots_1d8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058fa30
void ClearTextSlot1E8(int index)
{
    g_level_block->text_slots_1e8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* Ask for the text box to be redrawn without changing anything. */
// FUNCTION: WIZ8 0x0058aa00
void RedrawTextBox(void)
{
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* The value the screen keeps beside the text. */
// FUNCTION: WIZ8 0x0058aa10
int GetTextBoxValue2E8(void)
{
    return g_level_block->value_2e8;
}

/* Whether the line the cursor is on has anything on it. */
// FUNCTION: WIZ8 0x0058b940
bool CurrentTextLineHasContent(void)
{
    return g_level_block->text_lines[g_text_line_cursor_00686905] != 0;
}

/* Scroll so the line the cursor is on is the last of eight showing, or to the
   top when it would fit anyway. */
// FUNCTION: WIZ8 0x0058b910
void ScrollTextBoxToCursor(void)
{
    if (g_text_line_counts[g_text_line_cursor_00686905] > 8) {
        ScrollTextBoxTo(g_text_line_counts[g_text_line_cursor_00686905] - 8 + 1);
        return;
    }
    ScrollTextBoxTo(0);
}

/* Whichever byte the open dialogue exposes at 0x2d, or nothing when no
   dialogue is open - the flag has to be up before the pointer is read. */
// FUNCTION: WIZ8 0x0058d7c0
unsigned char GetOpenDialogueFlag(void)
{
    if (g_level_block->dialogue_open != 0 && g_level_block->dialogue_owner != 0) {
        return g_level_block->dialogue_owner[0x2d];
    }
    return 0;
}

/* The text box's mode, and the setter that also records a value when one is
   given - passing -1 leaves the value alone. */
// FUNCTION: WIZ8 0x005905e0
unsigned char GetTextBoxMode(void)
{
    return g_text_box_mode_0069b7b8;
}

// FUNCTION: WIZ8 0x005905c0
void SetTextBoxMode(unsigned char mode, int value)
{
    g_text_box_mode_0069b7b8 = mode;
    if (value != -1) {
        g_text_box_value_0064bd54 = value;
    }
}

/* Redraw the whole text box: its frame, its body, and its frame again on top -
   the second panel is drawn after the body rather than with the first. The two
   panels are Local Code\\Controls.cpp's Controls, and the null rectangle is how
   that class spells "all of it"; the screen holds them at 0x0c and 0x14. */
// FUNCTION: WIZ8 0x0058a8c0
void RedrawTextBoxComplete(void)
{
    unsigned char* screen = g_main_game_screen_0068f2d4;

    (*(Controls**)(screen + 0xc))->Invalidate(0);
    RedrawTextBoxBody();
    (*(Controls**)(screen + 0x14))->Invalidate(0);
}

extern unsigned int ClockIsTicking(int clock);
/* 0x0068F2E0: one clock per message, nine dwords apart, in per-line runs of
   0x15e. */
extern int g_message_clocks[];
extern unsigned char g_flag_00683f99;
extern void Function558810(void);
extern void Function558720(int arg_1);
extern void ShowNotice(int channel, const void* text, int a, int b, int c);

/* The last message on the current line whose clock has stopped, searched from
   the newest backwards - so the first one found is the most recent finished
   message rather than the oldest. */
// FUNCTION: WIZ8 0x0058d760
int FindStoppedTextLine(void)
{
    int index = g_text_line_counts[g_text_line_cursor_00686905];

    if (index == 0) {
        return -1;
    }
    while (--index >= 0) {
        if (ClockIsTicking(
                g_message_clocks[(index + g_text_line_cursor_00686905 * 0x15e) * 9]) == 0) {
            return index;
        }
    }
    return -1;
}

/* Hand one key to the text box's own handler. A key that moved the selection
   is followed by the two calls that settle it; a key that did not is not. */
// FUNCTION: WIZ8 0x0058a8f0
char TextBoxHandleKey(const void* event)
{
    char* panel = *(char**)(g_main_game_screen_0068f2d4 + 0xc);
    int before = *(int*)(panel + 0x78);
    char* handler = *(char**)(panel + 0x74);
    char handled;

    handled = (*(char(**)(char*, unsigned short))(*(char***)handler + 0x48 / 4))(
        handler, *(const unsigned short*)((const char*)event + 8));

    if (handled != 0 && *(int*)(panel + 0x78) != before) {
        Function558810();
        Function558720(3);
    }
    return handled;
}

/* Record what the Knock Knock spell is aimed at. Casting it anywhere the
   overlay is not up says so and records nothing - the message is the
   function's own name in the player's words. */
// FUNCTION: WIZ8 0x0058a9c0
void SetKnockKnockTarget(int target)
{
    unsigned char* screen = g_main_game_screen_0068f2d4;

    if (g_flag_00683f99 == 0) {
        ShowNotice(0xc, L"You can't cast Knock Knock here!", -1, -1, 0);
        return;
    }
    *(int*)(screen + 0x14c) = target;
    *(int*)(*(char**)(screen + 0x10) + 0x68) = target;
    *(unsigned char*)(*(char**)(screen + 0xc) + 0x140) = 1;
}
