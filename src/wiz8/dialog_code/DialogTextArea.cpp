#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/dialog_code/DialogTextArea.h"
#include "wiz8/utility.h"
#include "Font.h"

/* Reconstructed logical owner; original translation-unit identity is unproven. */

// VTABLE: WIZ8 0x005ef89c W8GrowableVector<W8DialogTextEntry*>
// class W8GrowableVector<W8DialogTextEntry*>

// SYNTHETIC: WIZ8 0x005d2560
// W8GrowableVector<W8DialogTextEntry*>::`scalar deleting destructor'

// TEMPLATE: WIZ8 0x005d2540
// W8GrowableVector<W8DialogTextEntry*>::~W8GrowableVector<W8DialogTextEntry*>

// SYNTHETIC: WIZ8 0x005d2590
// W8GrowableVector<W8DialogTextEntry*>::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d1ab0
void W8DialogTextArea::SetFirstVisibleEntry(unsigned int index)
{
    if (m_all_lines_01c.count != 0 && index <= static_cast<unsigned int>(m_all_lines_01c.count) &&
        (static_cast<unsigned int>(m_first_visible_entry) != index || m_first_visible_line != 0)) {
        m_first_visible_entry = index;
        m_first_visible_line = 0;
        m_relayout_needed = 1;
        unknown_03d = 1;
    }
}

// FUNCTION: WIZ8 0x005d1640
void W8DialogTextArea::Configure(const W8ControlsRect* bounds, int font, unsigned int flags)
{
    m_bounds.left = bounds->left;
    m_bounds.top = bounds->top;
    m_bounds.right = bounds->right;
    m_bounds.bottom = bounds->bottom;
    m_layout_initialized = 1;
    m_relayout_needed = 1;
    m_behavior_flags = flags;
    m_font = font;
    for (int index = 0; index < m_all_lines_01c.count; ++index) {
        W8DialogTextEntry* entry = *m_all_lines_01c.GetAt(index);
        entry->m_pendingBounds.left = m_bounds.left;
        entry->m_pendingBounds.top = m_bounds.top;
        entry->m_pendingBounds.right = m_bounds.right;
        entry->m_pendingBounds.bottom = m_bounds.bottom;
    }
}

// FUNCTION: WIZ8 0x005d1900
void W8DialogTextArea::Draw(unsigned char force)
{
    unsigned int font_height = GetFontHeight(m_font);
    if (force || unknown_03d || unknown_03e) {
        W8ControlsRect bounds;
        if (m_relayout_needed) {
            bounds.left = m_bounds.left;
            bounds.right = m_bounds.right;
            bounds.top = m_bounds.top - m_first_visible_line * font_height;
        }
        for (int index = m_first_visible_entry; index < m_visible_lines_02c.count; ++index) {
            if (m_relayout_needed) {
                unsigned int height = GetLineHeight();
                bounds.bottom =
                    bounds.top + (*m_visible_lines_02c.GetAt(index))->m_lineCount * height;
                (*m_visible_lines_02c.GetAt(index))->SetLayoutBounds(&bounds, 0, 0);
                bounds.top = bounds.bottom + m_entry_spacing;
            }
            (*m_visible_lines_02c.GetAt(index))->Draw(force || unknown_03d);
        }
        m_relayout_needed = 0;
        unknown_03d = 0;
        unknown_03e = 0;
    }
}

// FUNCTION: WIZ8 0x005d1a30
void W8DialogTextArea::SetFirstVisibleLine(int requested_line)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
    /* Retail compiled this comparison with VC6's mixed-sign operands; the
   signedness is part of the recovered body and changing it would change
   the compare and branch. Suppress only this diagnostic here. */
    int position = 0;
    unsigned int font_height = GetFontHeight(m_font);
    for (unsigned int index = 0; index < static_cast<unsigned int>(m_visible_lines_02c.count);
         ++index) {
        for (unsigned int line = 0;
             line < (*m_visible_lines_02c.GetAt(index))->m_lineCount +
                        static_cast<unsigned int>(m_entry_spacing) / font_height;
             ++line, ++position) {
            if (position == requested_line) {
                if (m_first_visible_entry == index && m_first_visible_line == line)
                    return;
                m_first_visible_entry = index;
                m_first_visible_line = line;
                m_relayout_needed = 1;
                unknown_03d = 1;
                return;
            }
        }
    }
#pragma clang diagnostic pop
}

// FUNCTION: WIZ8 0x005d1cb0
int W8DialogTextArea::GetTotalLineCount()
{
    int total = 0;
    for (int index = 0; index < m_visible_lines_02c.count; ++index) {
        int lines = (*m_visible_lines_02c.GetAt(index))->m_lineCount;
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
    for (int index = 0; index < m_visible_lines_02c.count; ++index) {
        (*m_all_lines_01c.GetAt(index))->SetLineHeight(m_line_height_override);
    }
}

// FUNCTION: WIZ8 0x005d1e80
unsigned char W8DialogTextArea::SelectEntry(int index)
{
    if (m_visible_lines_02c.count != 0 && !(*m_visible_lines_02c.GetAt(index))->m_selected) {
        (*m_visible_lines_02c.GetAt(index))->SetSelected(1);
        m_selected_visible_entry = index;
        unknown_03e = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1ed0
unsigned char W8DialogTextArea::ClearSelection()
{
    if (m_visible_lines_02c.count == 0) {
        m_selected_visible_entry = -1;
    } else if (m_selected_visible_entry != -1) {
        (*m_visible_lines_02c.GetAt(m_selected_visible_entry))->SetSelected(0);
        unknown_03e = 1;
        m_selected_visible_entry = -1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d2120
unsigned char W8DialogTextArea::CopyEntryText(unsigned int index, wchar_t* output)
{
    if (index >= static_cast<unsigned int>(m_all_lines_01c.count))
        return 0;
    (*m_all_lines_01c.GetAt(index))->CopyTextTo(output);
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
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines_02c.GetAt(index))->m_lineCount + spacing_pixels / font_height;
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
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines_02c.GetAt(index))->m_lineCount + spacing;
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
    if (index >= static_cast<unsigned int>(m_visible_lines_02c.count))
        return 0;
    (*m_visible_lines_02c.GetAt(index))->CopyTextTo(output);
    return 1;
}

// FUNCTION: WIZ8 0x005d2150
unsigned char W8DialogTextArea::SetEntryState5D(int index)
{
    if (m_visible_lines_02c.count != 0 && !(*m_visible_lines_02c.GetAt(index))->m_state_5d) {
        W8DialogTextEntry* entry = *m_visible_lines_02c.GetAt(index);
        if (entry->m_state_5d != 1) {
            entry->m_state_5d = 1;
            entry->m_geometryDirty = 1;
        }
        m_state_5d_entry = index;
        unknown_03e = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d21a0
unsigned char W8DialogTextArea::ClearEntryState5D()
{
    if (m_visible_lines_02c.count == 0) {
        m_state_5d_entry = -1;
    } else if (m_state_5d_entry != -1) {
        W8DialogTextEntry* entry = *m_visible_lines_02c.GetAt(m_state_5d_entry);
        if (entry->m_state_5d) {
            entry->m_state_5d = 0;
            entry->m_geometryDirty = 1;
        }
        unknown_03e = 1;
        m_state_5d_entry = -1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d2420
W8DialogTextEntry* W8DialogTextArea::GetEntry(unsigned int index)
{
    if (index >= static_cast<unsigned int>(m_all_lines_01c.count))
        return 0;
    return *m_all_lines_01c.GetAt(index);
}

// FUNCTION: WIZ8 0x005d2440
int W8DialogTextArea::GetOwningEntryIndex(int visible_index)
{
    W8DialogTextEntry** visible = m_visible_lines_02c.GetAt(visible_index);
    for (int index = 0; index < m_all_lines_01c.count; ++index) {
        if (*m_all_lines_01c.GetAt(index) == *visible)
            return index;
    }
    return -1;
}

// FUNCTION: WIZ8 0x005d24a0
void W8DialogTextArea::SetEntryState60(int index, unsigned char state)
{
    W8DialogTextEntry* entry = *m_all_lines_01c.GetAt(index);
    if (entry->m_state_60 != state) {
        entry->m_state_60 = state;
        entry->m_geometryDirty = 1;
    }
    unknown_03e = 1;
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
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548 |
                g_dialog_text_layout_mask_69c5d0,
            m_behavior_flags & 4);
    } else {
        entry = new W8DialogTextEntry(
            prefix, text, prefix_palette, text_palette, &m_bounds, m_font, category,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548,
            m_behavior_flags & 4);
    }
    if (m_line_height_override != -1)
        entry->SetLineHeight(m_line_height_override);
    int index = m_all_lines_01c.Add(entry);
    m_relayout_needed = 1;
    RebuildVisibleEntries();
    return index;
}

// FUNCTION: WIZ8 0x005d1820
void W8DialogTextArea::RemoveEntry(unsigned int index)
{
    if (m_all_lines_01c.count != 0 && index < static_cast<unsigned int>(m_all_lines_01c.count)) {
        if ((*m_all_lines_01c.GetAt(index))->m_state_5d)
            m_state_5d_entry = -1;
        if ((*m_all_lines_01c.GetAt(index))->m_selected)
            m_selected_visible_entry = -1;
        delete m_all_lines_01c.RemoveAt(index);
        if (m_first_visible_entry != 0 &&
            static_cast<unsigned int>(m_all_lines_01c.count) <=
                static_cast<unsigned int>(m_first_visible_entry) &&
            m_all_lines_01c.count != 0) {
            SetFirstVisibleEntry(m_all_lines_01c.count - 1);
        }
        m_relayout_needed = 1;
        unknown_03d = 1;
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
    m_visible_lines_02c.Clear();
    for (int index = 0; index < m_all_lines_01c.count; ++index) {
        if ((*m_all_lines_01c.GetAt(index))->m_category ==
                static_cast<unsigned char>(m_category_filter) ||
            m_category_filter == -1) {
            if (!m_sorted) {
                m_visible_lines_02c.Add(*m_all_lines_01c.GetAt(index));
            } else {
                CopyEntryText(index, text);
                int position;
                for (position = 0; position < m_visible_lines_02c.count; ++position) {
                    CopyVisibleEntryText(position, other);
                    if (CompareWideTextIgnoreAsciiCase00402920(text, other) < 0)
                        break;
                }
                if (position == m_visible_lines_02c.count) {
                    m_visible_lines_02c.Add(*m_all_lines_01c.GetAt(index));
                } else {
                    m_visible_lines_02c.InsertAt(position, *m_all_lines_01c.GetAt(index));
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
    unknown_03e = 1;
    m_relayout_needed = 1;
    RebuildVisibleEntries();
}

// FUNCTION: WIZ8 0x005d2480
void W8DialogTextArea::SetSorted(unsigned char sorted)
{
    m_sorted = sorted;
    unknown_03e = 1;
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
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = m_first_visible_line;
             line < (*m_visible_lines_02c.GetAt(index))->m_lineCount + spacing;
             ++line, ++visible_line) {
            unsigned int line_height = -1;
            if (m_layout_initialized) {
                line_height = m_line_height_override;
                if (line_height == static_cast<unsigned int>(-1)) {
                    line_height = GetFontHeight(m_font);
                }
            }
            if (static_cast<int>(line_height * visible_line) > height) {
                W8TextBuffer* text = *m_visible_lines_02c.GetAt(m_first_visible_entry);
                if (static_cast<unsigned int>(m_first_visible_line) <
                    text->m_lineCount - 1 + spacing) {
                    if (!check_only)
                        ++m_first_visible_line;
                } else {
                    if (static_cast<unsigned int>(m_first_visible_entry) >=
                        static_cast<unsigned int>(m_visible_lines_02c.count - 1)) {
                        return 0;
                    }
                    if (!check_only) {
                        ++m_first_visible_entry;
                        m_first_visible_line = 0;
                    }
                }
                if (!check_only) {
                    unknown_03d = 1;
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
    if (m_all_lines_01c.count == 0 || (m_first_visible_entry == 0 && m_first_visible_line == 0)) {
        return 0;
    }
    if (!check_only) {
        if (m_first_visible_line != 0) {
            --m_first_visible_line;
        } else {
            --m_first_visible_entry;
            W8TextBuffer* text = *m_visible_lines_02c.GetAt(m_first_visible_entry);
            if (text->m_lineCount + spacing > 1) {
                m_first_visible_line = text->m_lineCount - 1 + spacing;
            }
        }
        unknown_03d = 1;
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
    m_layout_initialized = 0;
    unknown_03d = 0;
    unknown_03e = 0;
    m_entry_spacing = 0;
    m_behavior_flags = 0;
    m_relayout_needed = 0;
    m_sorted = 0;
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
