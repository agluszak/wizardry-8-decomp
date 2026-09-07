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
    if (m_up_button != -1) {
        RemoveButton(m_up_button);
        m_up_button = -1;
    }
    if (m_up_image != -1) {
        UnloadButtonImage(m_up_image);
        m_up_image = -1;
    }
    if (m_down_button != -1) {
        RemoveButton(m_down_button);
        m_down_button = -1;
    }
    if (m_down_image != -1) {
        UnloadButtonImage(m_down_image);
        m_down_image = -1;
    }
    if (m_thumb_button != -1) {
        RemoveButton(m_thumb_button);
        m_thumb_button = -1;
    }
    if (m_thumb_image != -1) {
        UnloadButtonImage(m_thumb_image);
        m_thumb_image = -1;
    }
    if (m_track_button != -1) {
        RemoveButton(m_track_button);
        m_track_button = -1;
    }
    if (m_track_image != -1) {
        UnloadButtonImage(m_track_image);
        m_track_image = -1;
    }
    m_on_scroll = 0;
    m_initialized = 0;
}

// FUNCTION: WIZ8 0x005e1000
void W8DialogScrollBar::UpdateThumb()
{
    if (m_initialized && m_entry_count != -1 && m_first_visible_entry != -1) {
        int x = m_track_bounds[0] +
                (m_track_bounds[2] - m_track_bounds[0] - GetButtonWidth(m_thumb_button)) / 2;
        int offset = 0;
        if (m_entry_count > m_view_height / m_entry_height) {
            offset = ((m_track_bounds[3] - m_track_bounds[1] -
                       GetButtonHeight(m_thumb_button)) * m_first_visible_entry) /
                     (m_entry_count - m_view_height / m_entry_height);
        }
        SetButtonPosition(m_thumb_button, static_cast<short>(x),
                          static_cast<short>(m_track_bounds[1] + offset));
        m_dirty = 1;
    }
}

// FUNCTION: WIZ8 0x005e10b0
void W8DialogScrollBar::Draw(unsigned char force)
{
    if (m_initialized && m_visible && (force || m_dirty)) {
        DrawButton(m_track_button);
        DrawButton(m_up_button);
        DrawButton(m_down_button);
        DrawButton(m_thumb_button);
        MarkScreenRectDirty(
            GetButtonX(m_track_button), GetButtonY(m_track_button),
            GetButtonX(m_track_button) + GetButtonWidth(m_track_button),
            GetButtonY(m_track_button) + GetButtonHeight(m_track_button), 0);
        m_dirty = 0;
    }
}

// FUNCTION: WIZ8 0x005e1170
void W8DialogScrollBar::ScrollUp()
{
    if (m_first_visible_entry != 0) {
        --m_first_visible_entry;
        if (m_on_scroll) {
            m_on_scroll(this, m_first_visible_entry);
        }
        UpdateThumb();
    }
}

// FUNCTION: WIZ8 0x005e11a0
void W8DialogScrollBar::ScrollDown()
{
    int visible_entries = m_view_height / m_entry_height;
    if (visible_entries < m_entry_count &&
        m_first_visible_entry < m_entry_count - visible_entries) {
        ++m_first_visible_entry;
        if (m_on_scroll) {
            m_on_scroll(this, m_first_visible_entry);
        }
        UpdateThumb();
    }
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
    m_initialized = 0;
    m_visible = 0;
    unknown_024 = 0;
    m_entry_count = 1;
    m_first_visible_entry = 0;
    m_entry_height = -1;
    m_view_height = -1;
    m_dirty = 0;
    m_track_bounds[0] = 0;
    m_track_bounds[1] = 0;
    m_track_bounds[2] = 0;
    m_track_bounds[3] = 0;
    m_up_image = -1;
    m_up_button = -1;
    m_down_image = -1;
    m_down_button = -1;
    m_thumb_image = -1;
    m_thumb_button = -1;
    m_track_image = -1;
    m_track_button = -1;
    m_on_scroll = 0;
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
