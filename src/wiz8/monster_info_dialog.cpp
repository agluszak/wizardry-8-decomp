#include "wiz8/monster_info_dialog.h"
#include "wiz8/local_code/Controls.h"
#include "Button System.h"
#include "Font.h"
#include "wiz8/dirty_tiles.h"

#include <new>

extern int g_dword_69ca28;

// FUNCTION: WIZ8 0x005db4e0
void W8DialogButton::Draw()
{
    if (unknown_038 && m_resource_01c != -1) {
        int left = GetButtonX(m_resource_01c);
        int top = GetButtonY(m_resource_01c);
        DrawButton(m_resource_01c);
        MarkScreenRectDirty(left, top,
                            left + GetButtonWidth(m_resource_01c),
                            top + GetButtonHeight(m_resource_01c), 0);
        unknown_038 = 0;
    }
}

// FUNCTION: WIZ8 0x005db550
void W8DialogButton::SetPosition(int x, int y)
{
    if (m_resource_01c != -1 &&
        (x != GetButtonX(m_resource_01c) || y != GetButtonY(m_resource_01c))) {
        SetButtonPosition(m_resource_01c, static_cast<short>(x), static_cast<short>(y));
        unknown_038 = 1;
    }
}

// FUNCTION: WIZ8 0x005db5a0
int W8DialogButton::GetWidth()
{
    return m_resource_01c != -1 ? GetButtonWidth(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5c0
int W8DialogButton::GetHeight()
{
    return m_resource_01c != -1 ? GetButtonHeight(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db5e0
int W8DialogButton::GetX()
{
    return m_resource_01c != -1 ? GetButtonX(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db600
int W8DialogButton::GetY()
{
    return m_resource_01c != -1 ? GetButtonY(m_resource_01c) : 0;
}

// FUNCTION: WIZ8 0x005db8d0
void W8DialogButton::SetEnabled(unsigned char enabled)
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    unknown_035 = enabled;
    if (button) {
        if (enabled) {
            if (!(button->uiFlags & BUTTON_ENABLED)) {
                button->uiFlags |= BUTTON_ENABLED;
                unknown_038 = 1;
            }
        }
        else if (button->uiFlags & BUTTON_ENABLED) {
            button->uiFlags &= ~BUTTON_ENABLED;
            unknown_038 = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db920
unsigned char W8DialogButton::IsEnabled()
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    return button ? static_cast<unsigned char>(button->uiFlags & BUTTON_ENABLED) : 0;
}

// FUNCTION: WIZ8 0x005db950
void W8DialogButton::SetPressed(unsigned char pressed)
{
    GUI_BUTTON* button = GetButtonPtr(m_resource_01c);
    if (button) {
        if (pressed) {
            if (!(button->uiFlags & BUTTON_CLICKED_ON)) {
                button->uiFlags |= BUTTON_CLICKED_ON;
                unknown_038 = 1;
            }
        }
        else if (button->uiFlags & BUTTON_CLICKED_ON) {
            button->uiFlags &= ~BUTTON_CLICKED_ON;
            unknown_038 = 1;
        }
    }
}

// FUNCTION: WIZ8 0x005db9a0
unsigned char W8DialogButton::IsPressed()
{
    return static_cast<unsigned char>(GetButtonPtr(m_resource_01c)->uiFlags & BUTTON_CLICKED_ON);
}

// FUNCTION: WIZ8 0x005d1ab0
void W8DialogTextArea::SetFirstVisibleEntry(unsigned int index)
{
    if (m_all_lines_01c.count != 0 &&
        index <= static_cast<unsigned int>(m_all_lines_01c.count) &&
        (static_cast<unsigned int>(unknown_014) != index || unknown_018 != 0)) {
        unknown_014 = index;
        unknown_018 = 0;
        unknown_054 = 1;
        unknown_03d = 1;
    }
}

// FUNCTION: WIZ8 0x005e0e00
void W8DialogScrollBar::DestroyControls()
{
    if (unknown_02c != -1) {
        RemoveButton(unknown_02c);
        unknown_02c = -1;
    }
    if (unknown_028 != -1) {
        UnloadButtonImage(unknown_028);
        unknown_028 = -1;
    }
    if (unknown_034 != -1) {
        RemoveButton(unknown_034);
        unknown_034 = -1;
    }
    if (unknown_030 != -1) {
        UnloadButtonImage(unknown_030);
        unknown_030 = -1;
    }
    if (unknown_03c != -1) {
        RemoveButton(unknown_03c);
        unknown_03c = -1;
    }
    if (unknown_038 != -1) {
        UnloadButtonImage(unknown_038);
        unknown_038 = -1;
    }
    if (unknown_044 != -1) {
        RemoveButton(unknown_044);
        unknown_044 = -1;
    }
    if (unknown_040 != -1) {
        UnloadButtonImage(unknown_040);
        unknown_040 = -1;
    }
    unknown_048 = 0;
    unknown_000 = 0;
}

// FUNCTION: WIZ8 0x005d1ae0
unsigned char W8DialogTextArea::ScrollDown(unsigned char check_only)
{
    unsigned int spacing = static_cast<unsigned int>(unknown_040) /
                           GetFontHeight(unknown_010);
    int height = m_bottom_00c - m_top_004;
    int visible_line = 1;
    for (unsigned int index = unknown_014;
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = unknown_018;
             line < (*m_visible_lines_02c.GetAt(index))->m_lineCount + spacing;
             ++line, ++visible_line) {
            unsigned int line_height = -1;
            if (unknown_03c) {
                line_height = unknown_048;
                if (line_height == static_cast<unsigned int>(-1)) {
                    line_height = GetFontHeight(unknown_010);
                }
            }
            if (static_cast<int>(line_height * visible_line) > height) {
                W8TextBuffer005ED5B8* text =
                    *m_visible_lines_02c.GetAt(unknown_014);
                if (static_cast<unsigned int>(unknown_018) <
                    text->m_lineCount - 1 + spacing) {
                    if (!check_only) ++unknown_018;
                }
                else {
                    if (static_cast<unsigned int>(unknown_014) >=
                        static_cast<unsigned int>(m_visible_lines_02c.count - 1)) {
                        return 0;
                    }
                    if (!check_only) {
                        ++unknown_014;
                        unknown_018 = 0;
                    }
                }
                if (!check_only) {
                    unknown_03d = 1;
                    unknown_054 = 1;
                }
                return 1;
            }
        }
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1c00
unsigned char W8DialogTextArea::ScrollUp(unsigned char check_only)
{
    unsigned int spacing = static_cast<unsigned int>(unknown_040) /
                           GetFontHeight(unknown_010);
    if (m_all_lines_01c.count == 0 ||
        (unknown_014 == 0 && unknown_018 == 0)) {
        return 0;
    }
    if (!check_only) {
        if (unknown_018 != 0) {
            --unknown_018;
        }
        else {
            --unknown_014;
            W8TextBuffer005ED5B8* text = *m_visible_lines_02c.GetAt(unknown_014);
            if (text->m_lineCount + spacing > 1) {
                unknown_018 = text->m_lineCount - 1 + spacing;
            }
        }
        unknown_03d = 1;
        unknown_054 = 1;
    }
    return 1;
}

/* Dialog Code\MonsterInfoDialog.cpp defines no assertions, so unlike Octree or
   Monster this class yields no member names. Only offsets are established here,
   by byte-exact ports; the fields keep positional names. The 0x58 subobject is
   the first of the three the reviewed complete destructor tears down. */

// FUNCTION: WIZ8 0x005e0c40
W8DialogScrollBar::W8DialogScrollBar()
{
    unknown_000 = 0;
    unknown_001 = 0;
    unknown_024 = 0;
    unknown_004 = 1;
    unknown_008 = 0;
    unknown_00c = -1;
    unknown_010 = -1;
    unknown_002 = 0;
    unknown_014[0] = 0;
    unknown_014[1] = 0;
    unknown_014[2] = 0;
    unknown_014[3] = 0;
    unknown_028 = -1;
    unknown_02c = -1;
    unknown_030 = -1;
    unknown_034 = -1;
    unknown_038 = -1;
    unknown_03c = -1;
    unknown_040 = -1;
    unknown_044 = -1;
    unknown_048 = 0;
}

// FUNCTION: WIZ8 0x005db1b0
W8DialogButton::W8DialogButton()
{
    m_resource_018 = -1;
    m_resource_01c = -1;
    unknown_024 = 0;
    unknown_028 = 0;
    unknown_02c = 0;
    unknown_030 = 0;
    unknown_034 = 0;
    unknown_035 = 1;
    unknown_036 = 0;
    unknown_037 = 0;
    unknown_020 = -1;
    unknown_038 = 1;
    unknown_039 = 0;
    unknown_03a = 0;
    unknown_03b = 0;
    unknown_03c = 0;
    unknown_040 = 0;
    unknown_004 = -1;
    unknown_008 = -1;
    unknown_00c = -1;
    unknown_010 = -1;
    unknown_014 = -1;
    unknown_044 = g_dword_69ca28;
}

// SYNTHETIC: WIZ8 0x005db210
// W8DialogButton::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005db260
W8DialogButton::~W8DialogButton()
{
    if (m_resource_018 != -1) {
        UnloadButtonImage(m_resource_018);
        m_resource_018 = -1;
    }
    if (m_resource_01c != -1) {
        RemoveButton(m_resource_01c);
        m_resource_01c = -1;
    }
}

// VTABLE: WIZ8 0x005ef89c W8GrowableVector<W8TextBuffer005ED5B8*>
// class W8GrowableVector<W8TextBuffer005ED5B8*>

// SYNTHETIC: WIZ8 0x005d2560
// W8GrowableVector<W8TextBuffer005ED5B8*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005d2540
// W8GrowableVector<W8TextBuffer005ED5B8*>::~W8GrowableVector<W8TextBuffer005ED5B8*>

// SYNTHETIC: WIZ8 0x005d2590
// W8GrowableVector<W8TextBuffer005ED5B8*>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d14d0
W8DialogTextArea::W8DialogTextArea()
{
    int invalid;

    invalid = -1;
    unknown_048 = invalid;
    unknown_04c = invalid;
    unknown_050 = invalid;
    unknown_055 = invalid;
    unknown_014 = 0;
    unknown_018 = 0;
    unknown_010 = 0;
    unknown_03c = 0;
    unknown_03d = 0;
    unknown_03e = 0;
    unknown_040 = 0;
    unknown_044 = 0;
    unknown_054 = 0;
    unknown_056 = 0;
}

// FUNCTION: WIZ8 0x005d1590
W8DialogTextArea::~W8DialogTextArea()
{
    int index;

    if (m_all_lines_01c.GetCount() > 0) {
        for (index = m_all_lines_01c.GetCount() - 1; index >= 0; --index) {
            delete m_all_lines_01c.RemoveAt(index);
        }
    }
}

// Primary vtable slot 12.
// FUNCTION: WIZ8 0x005d6e60
void W8MonsterInfoDialog::OnRightButtonUp()
{
    if (m_field_50) {
        m_field_41 = 0;
    }
}

// Primary vtable slot 2.
// FUNCTION: WIZ8 0x005dbde0
void W8MonsterInfoDialog::DestroyControls()
{
    m_scroll_bar_58.DestroyControls();
    W8DialogBase::DestroyControls();
}
