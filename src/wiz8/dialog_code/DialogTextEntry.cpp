#include "wiz8/local_code/ControlsRect.h"
#include "wiz8/local_code/TextBuffer.h"
#include "wiz8/dialog_code/DialogTextEntry.h"
#include "wiz8/dirty_tiles.h"
#include "wiz8/screen_state.h"
#include "wiz8/fonts.h"
#include "wiz8/utility.h"
#include "Font.h"

/* Retail initializer 0x005D1010 copies the Controls layout constant. */
// GLOBAL: WIZ8 0x0069c5d0
unsigned int g_dialog_text_layout_mask_69c5d0 = g_W8TextBufferLayoutMask005ED560;

// FUNCTION: WIZ8 0x005d1020
W8DialogTextEntry::~W8DialogTextEntry()
{
}

// SYNTHETIC: WIZ8 0x005d1030
// W8DialogTextEntry::`scalar deleting destructor'

// FUNCTION: WIZ8 0x005d1050
W8DialogTextEntry::W8DialogTextEntry(
    const wchar_t* prefix, const wchar_t* text,
    unsigned int prefix_palette, unsigned int text_palette,
    const W8ControlsRect* bounds, int font, unsigned char category,
    unsigned int layout_mode, unsigned char shorten)
{
    m_prefix_palette = prefix_palette;
    m_font = font;
    m_lineCount = 0;
    m_geometryDirty = 1;
    m_text_palette = text_palette;
    m_selected = 0;
    m_state_5d = 0;
    m_state_60 = 0;
    m_category = category;
    SetLayoutBounds(bounds, 1, 1);
    SetLayoutMode(layout_mode);
    m_shorten = shorten;
    m_prefix_length = prefix ? wcslen(prefix) + 2 : 0;
    m_buffer = new wchar_t[wcslen(text) + m_prefix_length + 1];
    if (m_prefix_length != 0) {
        wcscpy(m_buffer, prefix);
        wcscat(m_buffer, L": ");
    } else {
        wcscpy(m_buffer, L"");
    }
    wcscat(m_buffer, text);
    UpdateLayout();
}

// FUNCTION: WIZ8 0x005d1170
void W8DialogTextEntry::Draw(unsigned char force)
{
    int width = m_layoutBounds.right - m_layoutBounds.left;
    int prefix_remaining = m_prefix_length;
    if (m_buffer == 0 || (force == 0 && m_geometryDirty == 0)) {
        return;
    }
    wchar_t* copy = new wchar_t[wcslen(m_buffer) + 5];
    wcscpy(copy, m_buffer);
    if (m_shorten) {
        ShortenTextToWidth00577410(copy, m_buffer, width - 5, m_font);
    }
    SetFont(m_font);
    unsigned short* palette = g_font_state_palettes_68ee1c[5];
    if (!m_state_5d) {
        if (m_state_60) {
            palette = g_font_state_palettes_68ee1c[1];
        } else if (m_selected) {
            palette = g_font_state_palettes_68ee1c[8];
        } else {
            palette = g_colour_68ee08;
            if (m_prefix_length == 0) {
                if (m_text_palette < 15) {
                    palette = g_font_state_palettes_68ee1c[m_text_palette];
                }
            } else if (m_prefix_palette < 15) {
                palette = g_font_state_palettes_68ee1c[m_prefix_palette];
            }
        }
    }
    SetFontObjectPalette16BPP(m_font, palette);
    SetFontDestBuffer(-14, m_pendingBounds.left, m_pendingBounds.top,
                      m_pendingBounds.right, m_pendingBounds.bottom, 0);
    int y = GetVerticalPosition();
    wchar_t* line = copy;
    size_t span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    while (line[span] != L'\0') {
        line[span] = L'\0';
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        if (prefix_remaining > 0) {
            if (prefix_remaining < static_cast<int>(span)) {
                wchar_t saved = line[prefix_remaining];
                line[prefix_remaining] = L'\0';
                mprintf(x, y, (unsigned short*)L"%s", line);
                x += StringPixLength((unsigned short*)line, m_font);
                line[prefix_remaining] = saved;
                if (!m_selected) {
                    SetFontObjectPalette16BPP(m_font, m_text_palette < 15
                        ? g_font_state_palettes_68ee1c[m_text_palette] : g_colour_68ee08);
                }
                line += prefix_remaining;
                span -= prefix_remaining;
            }
            prefix_remaining -= span;
        }
        mprintf(x, y, (unsigned short*)L"%s", line);
        y += GetLineHeight();
        line[span] = L'\n';
        if (m_layoutBounds.bottom <= y) {
            goto done;
        }
        line += span + 1;
        span = wcscspn(line, g_W8LineBreakCharacters00617C90);
    }
    {
        int x = GetHorizontalPosition(StringPixLength((unsigned short*)line, m_font));
        if (prefix_remaining > 0 && prefix_remaining < static_cast<int>(span)) {
            wchar_t saved = line[prefix_remaining];
            line[prefix_remaining] = L'\0';
            mprintf(x, y, (unsigned short*)L"%s", line);
            x += StringPixLength((unsigned short*)line, m_font);
            line[prefix_remaining] = saved;
            if (!m_selected) {
                SetFontObjectPalette16BPP(m_font, m_text_palette < 15
                    ? g_font_state_palettes_68ee1c[m_text_palette] : g_colour_68ee08);
            }
            line += prefix_remaining;
        }
        mprintf(x, y, (unsigned short*)L"%s", line);
    }
done:
    MarkScreenRectDirty(m_layoutBounds.left, m_layoutBounds.top,
                        m_layoutBounds.right, m_layoutBounds.bottom, 0);
    SetFontDestBuffer(-14, 0, 0, 640, 480, 0);
    m_geometryDirty = 0;
    delete[] copy;
}

// FUNCTION: WIZ8 0x005d14b0
void W8DialogTextEntry::SetSelected(unsigned char selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        m_geometryDirty = 1;
    }
}
