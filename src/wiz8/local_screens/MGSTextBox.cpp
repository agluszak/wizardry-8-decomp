#include "wiz8/gameplay_boundaries.h"

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

extern W8LevelRuntimeBlock* g_level_block;
extern void RequestRedraw(int mask);                                    /* 0x00562A50 */
extern unsigned char g_text_box_mode_0069b7b8;
extern int g_text_box_value_0064bd54;
extern int g_text_line_cursor_00686905;
/* 0x00689B17: one byte per line, how many entries that line holds. */
extern const unsigned char g_text_line_counts[];
extern void ScrollTextBoxTo(int line);                                  /* 0x0058BBC0 */
/* 0x0068F2D4: the screen the text box belongs to; its two panels sit at 0x0c
   and 0x14. */
extern unsigned char* g_main_game_screen_0068f2d4;
extern void RedrawTextBoxBody(void);                                    /* 0x00588E60 */

/* How many lines the text box can still be scrolled through. */
// FUNCTION: WIZ8 0x0058FB30
int GetTextBoxScrollRange(void)
{
    return g_level_block->scroll_bottom - g_level_block->scroll_top;
}

/* One entry of the second slot table. */
// FUNCTION: WIZ8 0x0058FA60
int GetTextSlot1E8(int index)
{
    return g_level_block->text_slots_1e8[index];
}

/* Empty one entry of either slot table and ask for a redraw. The two bodies
   differ only in which table they clear, which is what pairs them. */
// FUNCTION: WIZ8 0x0058F960
void ClearTextSlot1D8(int index)
{
    g_level_block->text_slots_1d8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

// FUNCTION: WIZ8 0x0058FA30
void ClearTextSlot1E8(int index)
{
    g_level_block->text_slots_1e8[index] = -1;
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* Ask for the text box to be redrawn without changing anything. */
// FUNCTION: WIZ8 0x0058AA00
void RedrawTextBox(void)
{
    RequestRedraw(W8_REDRAW_TEXT_BOX);
}

/* The value the screen keeps beside the text. */
// FUNCTION: WIZ8 0x0058AA10
int GetTextBoxValue2E8(void)
{
    return g_level_block->value_2e8;
}

/* Whether the line the cursor is on has anything on it. */
// FUNCTION: WIZ8 0x0058B940
bool CurrentTextLineHasContent(void)
{
    return g_level_block->text_lines[g_text_line_cursor_00686905] != 0;
}

/* Scroll so the line the cursor is on is the last of eight showing, or to the
   top when it would fit anyway. */
// FUNCTION: WIZ8 0x0058B910
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
// FUNCTION: WIZ8 0x0058D7C0
unsigned char GetOpenDialogueFlag(void)
{
    if (g_level_block->dialogue_open != 0 && g_level_block->dialogue_owner != 0) {
        return g_level_block->dialogue_owner[0x2d];
    }
    return 0;
}

/* The text box's mode, and the setter that also records a value when one is
   given - passing -1 leaves the value alone. */
// FUNCTION: WIZ8 0x005905E0
unsigned char GetTextBoxMode(void)
{
    return g_text_box_mode_0069b7b8;
}

// FUNCTION: WIZ8 0x005905C0
void SetTextBoxMode(unsigned char mode, int value)
{
    g_text_box_mode_0069b7b8 = mode;
    if (value != -1) {
        g_text_box_value_0064bd54 = value;
    }
}

/* One panel of the main game screen, reached only for its redraw slot. */
class W8ScreenPanel {
public:
    virtual ~W8ScreenPanel();
    virtual void Redraw(int full_redraw);
};

/* Redraw the whole text box: its frame, its body, and its frame again on top -
   the second panel is drawn after the body rather than with the first. */
// FUNCTION: WIZ8 0x0058A8C0
void RedrawTextBoxComplete(void)
{
    unsigned char* screen = g_main_game_screen_0068f2d4;

    (*(W8ScreenPanel**)(screen + 0xc))->Redraw(0);
    RedrawTextBoxBody();
    (*(W8ScreenPanel**)(screen + 0x14))->Redraw(0);
}
