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
        (static_cast<unsigned int>(unknown_014) != index || unknown_018 != 0)) {
        unknown_014 = index;
        unknown_018 = 0;
        unknown_054 = 1;
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
    unknown_03c = 1;
    unknown_054 = 1;
    unknown_044 = flags;
    unknown_010 = font;
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
    unsigned int font_height = GetFontHeight(unknown_010);
    if (force || unknown_03d || unknown_03e) {
        W8ControlsRect bounds;
        if (unknown_054) {
            bounds.left = m_bounds.left;
            bounds.right = m_bounds.right;
            bounds.top = m_bounds.top - unknown_018 * font_height;
        }
        for (int index = unknown_014; index < m_visible_lines_02c.count; ++index) {
            if (unknown_054) {
                unsigned int height = GetLineHeight();
                bounds.bottom =
                    bounds.top + (*m_visible_lines_02c.GetAt(index))->m_lineCount * height;
                (*m_visible_lines_02c.GetAt(index))->SetLayoutBounds(&bounds, 0, 0);
                bounds.top = bounds.bottom + unknown_040;
            }
            (*m_visible_lines_02c.GetAt(index))->Draw(force || unknown_03d);
        }
        unknown_054 = 0;
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
    unsigned int font_height = GetFontHeight(unknown_010);
    for (unsigned int index = 0; index < static_cast<unsigned int>(m_visible_lines_02c.count);
         ++index) {
        for (unsigned int line = 0; line < (*m_visible_lines_02c.GetAt(index))->m_lineCount +
                                               static_cast<unsigned int>(unknown_040) / font_height;
             ++line, ++position) {
            if (position == requested_line) {
                if (unknown_014 == index && unknown_018 == line)
                    return;
                unknown_014 = index;
                unknown_018 = line;
                unknown_054 = 1;
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
        total += lines + static_cast<unsigned int>(unknown_040) / GetFontHeight(unknown_010);
    }
    if (total != 0) {
        return total - static_cast<unsigned int>(unknown_040) / GetFontHeight(unknown_010);
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1d30
unsigned int W8DialogTextArea::GetLineHeight()
{
    if (!unknown_03c)
        return static_cast<unsigned int>(-1);
    unsigned int height = unknown_048;
    if (height == static_cast<unsigned int>(-1))
        height = GetFontHeight(unknown_010);
    return height;
}

// FUNCTION: WIZ8 0x005d1d60
void W8DialogTextArea::SetEntrySpacing(int lines)
{
    if (unknown_03c) {
        unsigned int font_height = GetFontHeight(unknown_010);
        unknown_054 = 1;
        unknown_040 = font_height * lines;
    }
}

// FUNCTION: WIZ8 0x005d1d90
void W8DialogTextArea::SetLineHeight(unsigned int height)
{
    unknown_048 = height;
    unknown_054 = 1;
    /* Retail uses the visible count with the owning list, not visible entries. */
    for (int index = 0; index < m_visible_lines_02c.count; ++index) {
        (*m_all_lines_01c.GetAt(index))->SetLineHeight(unknown_048);
    }
}

// FUNCTION: WIZ8 0x005d1e80
unsigned char W8DialogTextArea::SelectEntry(int index)
{
    if (m_visible_lines_02c.count != 0 && !(*m_visible_lines_02c.GetAt(index))->m_selected) {
        (*m_visible_lines_02c.GetAt(index))->SetSelected(1);
        unknown_04c = index;
        unknown_03e = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d1ed0
unsigned char W8DialogTextArea::ClearSelection()
{
    if (m_visible_lines_02c.count == 0) {
        unknown_04c = -1;
    } else if (unknown_04c != -1) {
        (*m_visible_lines_02c.GetAt(unknown_04c))->SetSelected(0);
        unknown_03e = 1;
        unknown_04c = -1;
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
    unsigned int font_height = GetFontHeight(unknown_010);
    unsigned int spacing_pixels = unknown_040;
    int line_height = GetLineHeight();
    for (unsigned int index = unknown_014;
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = unknown_018;
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
    unsigned int spacing = static_cast<unsigned int>(unknown_040) / GetFontHeight(unknown_010);
    if (!(unknown_044 & 2))
        return 0;
    int line_height = GetLineHeight();
    unsigned int target = (y - m_bounds.top) / line_height;
    if (target == static_cast<unsigned int>(unknown_04c))
        return 0;
    unsigned char changed = ClearSelection();
    for (unsigned int index = unknown_014;
         index < static_cast<unsigned int>(m_visible_lines_02c.count); ++index) {
        for (unsigned int line = unknown_018;
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
    return (unknown_044 & 2) ? ClearSelection() : 0;
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
        unknown_050 = index;
        unknown_03e = 1;
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d21a0
unsigned char W8DialogTextArea::ClearEntryState5D()
{
    if (m_visible_lines_02c.count == 0) {
        unknown_050 = -1;
    } else if (unknown_050 != -1) {
        W8DialogTextEntry* entry = *m_visible_lines_02c.GetAt(unknown_050);
        if (entry->m_state_5d) {
            entry->m_state_5d = 0;
            entry->m_geometryDirty = 1;
        }
        unknown_03e = 1;
        unknown_050 = -1;
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
    if (unknown_044 & 1) {
        entry = new W8DialogTextEntry(
            prefix, text, prefix_palette, text_palette, &m_bounds, unknown_010, category,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548 |
                g_dialog_text_layout_mask_69c5d0,
            unknown_044 & 4);
    } else {
        entry = new W8DialogTextEntry(
            prefix, text, prefix_palette, text_palette, &m_bounds, unknown_010, category,
            g_W8TextBufferLayoutMask005ED554 | g_W8TextBufferLayoutMask005ED548, unknown_044 & 4);
    }
    if (unknown_048 != -1)
        entry->SetLineHeight(unknown_048);
    int index = m_all_lines_01c.Add(entry);
    unknown_054 = 1;
    RebuildVisibleEntries();
    return index;
}

// FUNCTION: WIZ8 0x005d1820
void W8DialogTextArea::RemoveEntry(unsigned int index)
{
    if (m_all_lines_01c.count != 0 && index < static_cast<unsigned int>(m_all_lines_01c.count)) {
        if ((*m_all_lines_01c.GetAt(index))->m_state_5d)
            unknown_050 = -1;
        if ((*m_all_lines_01c.GetAt(index))->m_selected)
            unknown_04c = -1;
        delete m_all_lines_01c.RemoveAt(index);
        if (unknown_014 != 0 &&
            static_cast<unsigned int>(m_all_lines_01c.count) <=
                static_cast<unsigned int>(unknown_014) &&
            m_all_lines_01c.count != 0) {
            SetFirstVisibleEntry(m_all_lines_01c.count - 1);
        }
        unknown_054 = 1;
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
                static_cast<unsigned char>(unknown_055) ||
            unknown_055 == -1) {
            if (!unknown_056) {
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
    unknown_014 = 0;
    unknown_018 = 0;
}

// FUNCTION: WIZ8 0x005d2400
void W8DialogTextArea::SetCategoryFilter(signed char category)
{
    unknown_055 = category;
    unknown_03e = 1;
    unknown_054 = 1;
    RebuildVisibleEntries();
}

// FUNCTION: WIZ8 0x005d2480
void W8DialogTextArea::SetSorted(unsigned char sorted)
{
    unknown_056 = sorted;
    unknown_03e = 1;
    unknown_054 = 1;
    RebuildVisibleEntries();
}

// FUNCTION: WIZ8 0x005d1ae0
unsigned char W8DialogTextArea::ScrollDown(unsigned char check_only)
{
    unsigned int spacing = static_cast<unsigned int>(unknown_040) / GetFontHeight(unknown_010);
    int height = m_bounds.bottom - m_bounds.top;
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
                W8TextBuffer* text = *m_visible_lines_02c.GetAt(unknown_014);
                if (static_cast<unsigned int>(unknown_018) < text->m_lineCount - 1 + spacing) {
                    if (!check_only)
                        ++unknown_018;
                } else {
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
    unsigned int spacing = static_cast<unsigned int>(unknown_040) / GetFontHeight(unknown_010);
    if (m_all_lines_01c.count == 0 || (unknown_014 == 0 && unknown_018 == 0)) {
        return 0;
    }
    if (!check_only) {
        if (unknown_018 != 0) {
            --unknown_018;
        } else {
            --unknown_014;
            W8TextBuffer* text = *m_visible_lines_02c.GetAt(unknown_014);
            if (text->m_lineCount + spacing > 1) {
                unknown_018 = text->m_lineCount - 1 + spacing;
            }
        }
        unknown_03d = 1;
        unknown_054 = 1;
    }
    return 1;
}

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
