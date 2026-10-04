#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/utility.h"
#include "Font.h"

/* Reconstructed logical owner; original translation-unit identity is unproven.
   Live query: 0x005D14D0/0x005D1640 sit in the gap between DialogInterface.cpp
   (upper 0x005CF580) and stMessageDialog.cpp (lower 0x005D2800). Retail
   Dialog Code\stListBox.cpp's hull is 0x005CCE70-0x005CD1E0: the six SGP
   button callbacks of W8ListBoxDialog, now defined in stListBox.cpp. The
   intervening helpers (0x005CC650/0x005CCB80/0x005CD2B0) are not in that hull,
   and neither are these. Retail emits a second one-slot table at 0x005EF89C
   holding the duplicate sdd emission 0x005D2560; the compared table at
   0x005EF898 (marked in the header) holds 0x005D2590, so only that copy
   pairs to the recompiled sdd. */

/* The NPC-dialogue translation unit's own copies of the 1/2/4 text-buffer
   layout masks; they sit immediately ahead of this file's vector vtable at
   0x005EF898 in retail data. */
// GLOBAL: WIZ8 0x005EF888
extern const unsigned int g_W8DialogTextAreaAlignLeft = 0x01;
// GLOBAL: WIZ8 0x005EF88C
extern const unsigned int g_W8DialogTextAreaAlignCenter = 0x02;
// GLOBAL: WIZ8 0x005EF890
extern const unsigned int g_W8DialogTextAreaAlignRight = 0x04;

// FUNCTION: WIZ8 0x005d1ab0
void W8DialogTextArea::SetFirstVisibleEntry(unsigned int index)
{
    if (m_all_lines.GetCount() != 0 && index <= static_cast<unsigned int>(m_all_lines.GetCount()) &&
        (static_cast<unsigned int>(m_first_visible_entry) != index || m_first_visible_line != 0)) {
        m_first_visible_entry = index;
        m_first_visible_line = 0;
        m_relayout_needed = 1;
        m_dirty = true;
    }
}

// FUNCTION: WIZ8 0x005d1640
void W8DialogTextArea::Configure(const W8ControlsRect* bounds, int font, unsigned int flags)
{
    m_bounds = *bounds;
    m_layout_initialized = true;
    m_relayout_needed = 1;
    m_behavior_flags = flags;
    m_font = font;
    for (int index = 0; index < m_all_lines.GetCount(); ++index) {
        W8DialogTextEntry* entry = *m_all_lines.GetAt(index);
        entry->m_pendingBounds = m_bounds;
    }
}

// FUNCTION: WIZ8 0x005d1900
void W8DialogTextArea::Draw(unsigned char force)
{
    unsigned int font_height = GetFontHeight(m_font);
    if (force || m_dirty || selection_dirty) {
        W8ControlsRect bounds;
        if (m_relayout_needed) {
            bounds.left = m_bounds.left;
            bounds.right = m_bounds.right;
            bounds.top = m_bounds.top - m_first_visible_line * font_height;
        }
        for (int index = m_first_visible_entry; index < m_visible_lines.GetCount(); ++index) {
            if (m_relayout_needed) {
                unsigned int height = GetLineHeight();
                bounds.bottom =
                    bounds.top + (*m_visible_lines.GetAt(index))->m_lineCount * height;
                (*m_visible_lines.GetAt(index))->SetLayoutBounds(&bounds, 0, 0);
                bounds.top = bounds.bottom + m_entry_spacing;
            }
            (*m_visible_lines.GetAt(index))->Draw(force || m_dirty);
        }
        m_relayout_needed = 0;
        m_dirty = false;
        selection_dirty = 0;
    }
}

// FUNCTION: WIZ8 0x005d1a30
void W8DialogTextArea::SetFirstVisibleLine(int requested_line)
{
    int position = 0;
    unsigned int font_height = GetFontHeight(m_font);
    for (unsigned int index = 0; index < static_cast<unsigned int>(m_visible_lines.GetCount());
         ++index) {
        for (unsigned int line = 0;
             line < (*m_visible_lines.GetAt(index))->m_lineCount +
                        static_cast<unsigned int>(m_entry_spacing) / font_height;
             ++line, ++position) {
            if (position == requested_line) {
                if (static_cast<unsigned int>(m_first_visible_entry) == index &&
                    static_cast<unsigned int>(m_first_visible_line) == line)
                    return;
                m_first_visible_entry = index;
                m_first_visible_line = line;
                m_relayout_needed = 1;
                m_dirty = true;
                return;
            }
        }
    }
}

// FUNCTION: WIZ8 0x005d1cb0
int W8DialogTextArea::GetTotalLineCount()
{
    int total = 0;
    for (int index = 0; index < m_visible_lines.GetCount(); ++index) {
        int lines = (*m_visible_lines.GetAt(index))->m_lineCount;
        total += lines + static_cast<unsigned int>(m_entry_spacing) / GetFontHeight(m_font);
    }
    if (total != 0) {
        return total - static_cast<unsigned int>(m_entry_spacing) / GetFontHeight(m_font);
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1d30
unsigned int W8DialogTextArea::GetLineHeight()
{
    if (!m_layout_initialized)
        return static_cast<unsigned int>(-1);
    unsigned int height = m_line_height_override;
    if (height == static_cast<unsigned int>(-1))
        height = GetFontHeight(m_font);
    return height;
}

// FUNCTION: WIZ8 0x005d1d60
void W8DialogTextArea::SetEntrySpacing(int lines)
{
    if (m_layout_initialized) {
        unsigned int font_height = GetFontHeight(m_font);
        m_relayout_needed = 1;
        m_entry_spacing = font_height * lines;
    }
}

// FUNCTION: WIZ8 0x005d1d90
void W8DialogTextArea::SetLineHeight(unsigned int height)
{
    m_line_height_override = height;
    m_relayout_needed = 1;
    /* Retail uses the visible count with the owning list, not visible entries. */
    for (int index = 0; index < m_visible_lines.GetCount(); ++index) {
        (*m_all_lines.GetAt(index))->SetLineHeight(m_line_height_override);
    }
}

// FUNCTION: WIZ8 0x005d1e80
unsigned char W8DialogTextArea::SelectEntry(int index)
{
    if (m_visible_lines.GetCount() != 0 && !(*m_visible_lines.GetAt(index))->m_selected) {
        (*m_visible_lines.GetAt(index))->SetSelected(1);
        m_selected_visible_entry = index;
        selection_dirty = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1ed0
unsigned char W8DialogTextArea::ClearSelection()
{
    if (m_visible_lines.GetCount() == 0) {
        m_selected_visible_entry = -1;
    } else if (m_selected_visible_entry != -1) {
        (*m_visible_lines.GetAt(m_selected_visible_entry))->SetSelected(0);
        selection_dirty = 1;
        m_selected_visible_entry = -1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d2120
unsigned char W8DialogTextArea::CopyEntryText(unsigned int index, wchar_t* output)
{
    if (index >= static_cast<unsigned int>(m_all_lines.GetCount()))
        return 0;
    (*m_all_lines.GetAt(index))->CopyTextTo(output);
    return 1;
}

// FUNCTION: WIZ8 0x005d1dd0
unsigned int W8DialogTextArea::HitTestEntry(int, int y)
{
    int position = 0;
    unsigned int font_height = GetFontHeight(m_font);
    unsigned int spacing_pixels = m_entry_spacing;
    int line_height = GetLineHeight();
    for (unsigned int index = m_first_visible_entry;
         index < static_cast<unsigned int>(m_visible_lines.GetCount()); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines.GetAt(index))->m_lineCount + spacing_pixels / font_height;
             ++line, ++position) {
            if (position == (y - m_bounds.top) / line_height)
                return index;
        }
    }
    return static_cast<unsigned int>(-1);
}

// FUNCTION: WIZ8 0x005d1f20
unsigned char W8DialogTextArea::UpdateSelectionFromPoint(int, int y)
{
    unsigned int position = 0;
    unsigned int spacing = static_cast<unsigned int>(m_entry_spacing) / GetFontHeight(m_font);
    if (!(m_behavior_flags & 2))
        return 0;
    int line_height = GetLineHeight();
    unsigned int target = (y - m_bounds.top) / line_height;
    if (target == static_cast<unsigned int>(m_selected_visible_entry))
        return 0;
    unsigned char changed = ClearSelection();
    for (unsigned int index = m_first_visible_entry;
         index < static_cast<unsigned int>(m_visible_lines.GetCount()); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines.GetAt(index))->m_lineCount + spacing;
             ++line, ++position) {
            if (position == target)
                return SelectEntry(index);
        }
    }
    return changed;
}

// FUNCTION: WIZ8 0x005d20a0
unsigned char W8DialogTextArea::ClearPointSelection()
{
    return (m_behavior_flags & 2) ? ClearSelection() : 0;
}

// FUNCTION: WIZ8 0x005d20f0
unsigned char W8DialogTextArea::CopyVisibleEntryText(unsigned int index, wchar_t* output)
{
    if (index >= static_cast<unsigned int>(m_visible_lines.GetCount()))
        return 0;
    (*m_visible_lines.GetAt(index))->CopyTextTo(output);
    return 1;
}

// FUNCTION: WIZ8 0x005d2150
unsigned char W8DialogTextArea::SetEntryState5D(int index)
{
    if (m_visible_lines.GetCount() != 0 && !(*m_visible_lines.GetAt(index))->m_state0) {
        W8DialogTextEntry* entry = *m_visible_lines.GetAt(index);
        if (entry->m_state0 != 1) {
            entry->m_state0 = 1;
            entry->SetGeometryDirty();
        }
        m_state_5d_entry = index;
        selection_dirty = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d21a0
unsigned char W8DialogTextArea::ClearEntryState5D()
{
    if (m_visible_lines.GetCount() == 0) {
        m_state_5d_entry = -1;
    } else if (m_state_5d_entry != -1) {
        W8DialogTextEntry* entry = *m_visible_lines.GetAt(m_state_5d_entry);
        if (entry->m_state0) {
            entry->m_state0 = 0;
            entry->SetGeometryDirty();
        }
        selection_dirty = 1;
        m_state_5d_entry = -1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d2420
W8DialogTextEntry* W8DialogTextArea::GetEntry(unsigned int index)
{
    if (index >= static_cast<unsigned int>(m_all_lines.GetCount()))
        return 0;
    return *m_all_lines.GetAt(index);
}

// FUNCTION: WIZ8 0x005d2440
int W8DialogTextArea::GetOwningEntryIndex(int visible_index)
{
    W8DialogTextEntry** visible = m_visible_lines.GetAt(visible_index);
    for (int index = 0; index < m_all_lines.GetCount(); ++index) {
        if (*m_all_lines.GetAt(index) == *visible)
            return index;
    }
    return -1;
}

// FUNCTION: WIZ8 0x005d24a0
void W8DialogTextArea::SetEntryState60(int index, bool state)
{
    W8DialogTextEntry* entry = *m_all_lines.GetAt(index);
    if (entry->m_state1 != state) {
        entry->m_state1 = state;
        entry->SetGeometryDirty();
    }
    selection_dirty = 1;
}

// FUNCTION: WIZ8 0x005d16c0
int W8DialogTextArea::AddEntry(const wchar_t* prefix, const wchar_t* text,
                               unsigned int prefix_palette, unsigned int text_palette,
                               unsigned char category)
{
    W8DialogTextEntry* entry;
    if (m_behavior_flags & 1) {
        entry = new W8DialogTextEntry(
            prefix, text, prefix_palette, text_palette, &m_bounds, m_font, category,
            g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft | g_dialog_text_layout_mask,
            m_behavior_flags & 4);
    } else {
        entry = new W8DialogTextEntry(prefix, text, prefix_palette, text_palette, &m_bounds, m_font,
                                      category, g_W8TextBufferAlignMiddle | g_W8TextBufferAlignLeft,
                                      m_behavior_flags & 4);
    }
    if (m_line_height_override != -1)
        entry->SetLineHeight(m_line_height_override);
    int index = m_all_lines.Add(entry);
    m_relayout_needed = 1;
    RebuildVisibleEntries();
    return index;
}

// FUNCTION: WIZ8 0x005d1820
void W8DialogTextArea::RemoveEntry(unsigned int index)
{
    if (m_all_lines.GetCount() != 0 && index < static_cast<unsigned int>(m_all_lines.GetCount())) {
        if ((*m_all_lines.GetAt(index))->m_state0)
            m_state_5d_entry = -1;
        if ((*m_all_lines.GetAt(index))->m_selected)
            m_selected_visible_entry = -1;
        m_all_lines.RemoveAtAndDelete(index);
        if (m_first_visible_entry != 0 &&
            static_cast<unsigned int>(m_all_lines.GetCount()) <=
                static_cast<unsigned int>(m_first_visible_entry) &&
            m_all_lines.GetCount() != 0) {
            SetFirstVisibleEntry(m_all_lines.GetCount() - 1);
        }
        m_relayout_needed = 1;
        m_dirty = true;
        RebuildVisibleEntries();
    }
}

// FUNCTION: WIZ8 0x005d21f0
void W8DialogTextArea::RebuildVisibleEntries()
{
    wchar_t text[200];
    wchar_t other[200];
    ClearEntryState5D();
    ClearSelection();
    m_visible_lines.Clear();
    for (int index = 0; index < m_all_lines.GetCount(); ++index) {
        if ((*m_all_lines.GetAt(index))->m_category ==
                static_cast<unsigned char>(m_category_filter) ||
            m_category_filter == -1) {
            if (!m_sorted) {
                m_visible_lines.Add(*m_all_lines.GetAt(index));
            } else {
                CopyEntryText(index, text);
                int position;
                for (position = 0; position < m_visible_lines.GetCount(); ++position) {
                    CopyVisibleEntryText(position, other);
                    if (CompareWideTextIgnoreAsciiCase(text, other) < 0)
                        break;
                }
                if (position == m_visible_lines.GetCount()) {
                    m_visible_lines.Add(*m_all_lines.GetAt(index));
                } else {
                    m_visible_lines.InsertAt(position, *m_all_lines.GetAt(index));
                }
            }
        }
    }
    m_first_visible_entry = 0;
    m_first_visible_line = 0;
}

// FUNCTION: WIZ8 0x005d2400
void W8DialogTextArea::SetCategoryFilter(signed char category)
{
    m_category_filter = category;
    selection_dirty = 1;
    m_relayout_needed = 1;
    RebuildVisibleEntries();
}

// FUNCTION: WIZ8 0x005d2480
void W8DialogTextArea::SetSorted(unsigned char sorted)
{
    m_sorted = sorted;
    selection_dirty = 1;
    m_relayout_needed = 1;
    RebuildVisibleEntries();
}

// FUNCTION: WIZ8 0x005d1ae0
unsigned char W8DialogTextArea::ScrollDown(unsigned char check_only)
{
    unsigned int spacing = static_cast<unsigned int>(m_entry_spacing) / GetFontHeight(m_font);
    int height = m_bounds.bottom - m_bounds.top;
    int visible_line = 1;
    for (unsigned int index = m_first_visible_entry;
         index < static_cast<unsigned int>(m_visible_lines.GetCount()); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines.GetAt(index))->m_lineCount + spacing;
             ++line, ++visible_line) {
            unsigned int line_height = -1;
            if (m_layout_initialized) {
                line_height = m_line_height_override;
                if (line_height == static_cast<unsigned int>(-1)) {
                    line_height = GetFontHeight(m_font);
                }
            }
            if (static_cast<int>(line_height * visible_line) > height) {
                W8TextBuffer* text = *m_visible_lines.GetAt(m_first_visible_entry);
                if (static_cast<unsigned int>(m_first_visible_line) <
                    text->m_lineCount - 1 + spacing) {
                    if (!check_only)
                        ++m_first_visible_line;
                } else {
                    if (static_cast<unsigned int>(m_first_visible_entry) >=
                        static_cast<unsigned int>(m_visible_lines.GetCount() - 1)) {
                        return 0;
                    }
                    if (!check_only) {
                        ++m_first_visible_entry;
                        m_first_visible_line = 0;
                    }
                }
                if (!check_only) {
                    m_dirty = true;
                    m_relayout_needed = 1;
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
    unsigned int spacing = static_cast<unsigned int>(m_entry_spacing) / GetFontHeight(m_font);
    if (m_all_lines.GetCount() == 0 || (m_first_visible_entry == 0 && m_first_visible_line == 0)) {
        return 0;
    }
    if (!check_only) {
        if (m_first_visible_line != 0) {
            --m_first_visible_line;
        } else {
            --m_first_visible_entry;
            W8TextBuffer* text = *m_visible_lines.GetAt(m_first_visible_entry);
            if (text->m_lineCount + spacing > 1) {
                m_first_visible_line = text->m_lineCount - 1 + spacing;
            }
        }
        m_dirty = true;
        m_relayout_needed = 1;
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d14d0
W8DialogTextArea::W8DialogTextArea()
{
    int invalid;

    invalid = -1;
    m_line_height_override = invalid;
    m_selected_visible_entry = invalid;
    m_state_5d_entry = invalid;
    m_category_filter = invalid;
    m_first_visible_entry = 0;
    m_first_visible_line = 0;
    m_font = 0;
    m_layout_initialized = false;
    m_dirty = false;
    selection_dirty = 0;
    m_entry_spacing = 0;
    m_behavior_flags = 0;
    m_relayout_needed = 0;
    m_sorted = 0;
}

// FUNCTION: WIZ8 0x005d1590
W8DialogTextArea::~W8DialogTextArea()
{
    int index;

    if (m_all_lines.GetCount() > 0) {
        for (index = m_all_lines.GetCount() - 1; index >= 0; --index) {
            m_all_lines.RemoveAtAndDelete(index);
        }
    }
}
