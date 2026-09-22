#include "wiz8/wiz8_windows.h"

#include "Types.h"
#include "Font.h"
#include "himage.h"
#include "vobject.h"
#include "vobject_blitters.h"
#include "vsurface.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/PortraitQuote.h"
#include "wiz8/engine_code/Video2.h"
#include "wiz8/fonts.h"
#include "wiz8/local_screens/OptionsScreen.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* The NPC quote bubble record, allocated with operator new inside
   LayoutPortraitQuoteBubble. Offset 0 is the rendered text surface; 0x0c and
   0x10 are the popup background surface and border object the bubble is
   composited from. The byte at 0x14 is set once those two resources exist;
   the byte at 0x15 is set once the whole bubble has been created. */
struct W8PortraitQuoteBubble {
    UINT32 surface;                    /* 0x00 */
    unsigned short width;              /* 0x04 */
    unsigned short height;             /* 0x06 */
    unsigned char background_index_08; /* 0x08 */
    unsigned char object_index_09;     /* 0x09 */
    unsigned char unknown_0a[2];
    UINT32 background_surface;      /* 0x0c */
    UINT32 object;                  /* 0x10 */
    unsigned char has_resources_14; /* 0x14 */
    unsigned char created_15;       /* 0x15 */
    unsigned char unknown_16[2];
    UINT32 flags;   /* 0x18: bit 0 selects the flat fill */
    wchar_t* text;  /* 0x1c */
    UINT32 palette; /* 0x20 */
};

static_assert(sizeof(W8PortraitQuoteBubble) == 0x24, "W8PortraitQuoteBubble_size");

static W8PortraitQuoteBubble* g_portrait_quotes[10];
static W8PortraitQuoteBubble* g_current_portrait_quote;

/* The bubble artwork file tables, indexed by the edge and background selector
   arguments of LayoutPortraitQuoteBubble. */
// GLOBAL: WIZ8 0x0064f550
static const char* g_quote_bubble_edges_64f550[] = {
    "data\\NPC Interaction\\npc_PopUp_edge.sti",
};
// GLOBAL: WIZ8 0x0064f554
static const char* g_quote_bubble_backgrounds_64f554[] = {
    "data\\NPC Interaction\\npc_popup_back.pcx",
};

/* Flags staged for the next laid-out bubble; LayoutPortraitQuoteBubble copies
   them into the record and clears the staging value. Bit 0 selects the flat
   white fill instead of the background artwork. */
// GLOBAL: WIZ8 0x0069c5c8
static unsigned int g_quote_bubble_flags_69c5c8;

int MeasureWrappedText(int arg_1, int arg_2, unsigned int wrap_width, int arg_4, int font,
                       int colour, const wchar_t* text, int arg_8, int arg_9, int arg_10,
                       unsigned int* out_edge);
int DrawWrappedText(int x, int y, unsigned int wrap_width, int arg_4, int font,
                    unsigned char colour, const wchar_t* text, int arg_8, int arg_9, int arg_10);

// FUNCTION: WIZ8 0x005d0590
static int DrawWrappedTextLine(UINT16* text, int x, int top, int width, int font,
                               unsigned char foreground, unsigned char background, bool dirty,
                               unsigned int flags)
{
    short draw_x = static_cast<short>(x);
    short draw_y = static_cast<short>(top);

    if (flags & 0x20) {
        return 1;
    }
    if (flags == 0) {
        flags = 1;
    }
    if (flags & 1) {
        draw_x = static_cast<short>(x);
        draw_y = static_cast<short>(top);
    } else if (flags & 2) {
        VarFindFontCenterCoordinates(static_cast<short>(x), static_cast<short>(top),
                                     static_cast<short>(width), GetFontHeight(font), font, &draw_x,
                                     &draw_y, text);
    } else if (flags & 4) {
        VarFindFontRightCoordinates(static_cast<short>(x), static_cast<short>(top),
                                    static_cast<short>(width), GetFontHeight(font), font, &draw_x,
                                    &draw_y, text);
    }

    SetFont(font);
    SetFontForeground(foreground);
    SetFontBackground(background);
    if (flags & 8) {
        ShadowVideoSurfaceRect(0xfffffff2, static_cast<unsigned short>(draw_x - 1),
                               static_cast<unsigned short>(draw_y - 1),
                               static_cast<unsigned short>(draw_x - 1) +
                                   static_cast<unsigned short>(StringPixLength(text, font)) + 1,
                               static_cast<unsigned short>(draw_y - 1) + GetFontHeight(font) + 1);
    }
    if (dirty) {
        gprintfDirty(draw_x, draw_y, text);
    }
    mprintf(draw_x, draw_y, text);
    if (flags & 0x20) {
        SetFontShadow(2);
    }
    if (flags & 0x10) {
        InvalidateRegion(draw_x, draw_y, draw_x + StringPixLength(text, font),
                         draw_y + GetFontHeight(font), 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005d0770
int DrawWrappedText(int x, int y, unsigned int wrap_width, int line_spacing, int font,
                    unsigned char colour, const wchar_t* text, int background, int dirty, int flags)
{
    wchar_t line[0x80];
    wchar_t word[0x40];
    unsigned int position = 0;
    unsigned int word_length = 0;
    unsigned int line_width = 0;
    unsigned int remaining_width = wrap_width;
    unsigned char active_colour = colour;
    int draw_x = x;
    int draw_y = y;
    int active_font = font;
    int line_count = 1;
    short section = 1;
    bool alternate = false;

    memset(line, 0, sizeof(line));
    memset(word, 0, sizeof(word));
    GetFontHeight(font);

    for (;;) {
        wchar_t ch = text[position];
        if (ch != L' ' && ch != L'\0') {
            word[word_length++] = ch;
        } else if ((word[0] < 0xb2 || word[0] > 0xb5) && word[0] != L'\n') {
            word[word_length] = L'\0';
            unsigned int word_width = StringPixLength(
                reinterpret_cast<UINT16*>(word), // reinterpret-ok: SGP wide-text API boundary
                active_font);
            word[word_length++] = L' ';
            word[word_length] = L'\0';
            if ((wrap_width & 0xffff) < word_width + line_width) {
                if (flags & 0x20) {
                    SetFontShadow(0);
                }
                DrawWrappedTextLine(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    draw_x, draw_y, remaining_width, active_font, active_colour,
                    static_cast<unsigned char>(background), section != 0, flags);
                if (flags & 0x20) {
                    SetFontShadow(2);
                }
                draw_y += GetFontHeight(active_font) + (line_spacing & 0xff);
                ++line_count;
                wcscpy(line, word);
                line_width = StringPixLength(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
                remaining_width = wrap_width;
                draw_x = x;
            } else {
                line_width += StringPixLength(
                    reinterpret_cast<UINT16*>(word), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
                wcscat(line, word);
            }
            word_length = 0;
        } else {
            if (word[0] == L'\n') {
                if (flags & 0x20) {
                    SetFontShadow(0);
                }
                DrawWrappedTextLine(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    draw_x, draw_y, remaining_width, active_font, active_colour,
                    static_cast<unsigned char>(background), section != 0, flags);
                if (flags & 0x20) {
                    SetFontShadow(2);
                }
                draw_y += GetFontHeight(active_font) + (line_spacing & 0xff);
                ++line_count;
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                word_length = 0;
                line_width = 0;
                remaining_width = wrap_width;
                draw_x = x;
            } else if (word[0] == 0xb2) {
                if (flags & 0x20) {
                    SetFontShadow(0);
                }
                DrawWrappedTextLine(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    draw_x, draw_y, remaining_width, active_font, active_colour,
                    static_cast<unsigned char>(background), section != 0, flags);
                if (flags & 0x20) {
                    SetFontShadow(2);
                }
                if (alternate) {
                    int span =
                        StringPixLength(reinterpret_cast<UINT16*>(
                                            line), // reinterpret-ok: SGP wide-text API boundary
                                        active_font);
                    remaining_width -= line_width;
                    memset(line, 0, sizeof(line));
                    memset(word, 0, sizeof(word));
                    alternate = false;
                    word_length = 0;
                    draw_x += span;
                    active_font = font;
                } else {
                    int span =
                        StringPixLength(reinterpret_cast<UINT16*>(
                                            line), // reinterpret-ok: SGP wide-text API boundary
                                        active_font);
                    active_font = g_font12point1_683648;
                    remaining_width -= line_width;
                    memset(line, 0, sizeof(line));
                    memset(word, 0, sizeof(word));
                    SetFontShadow(0);
                    alternate = true;
                    word_length = 0;
                    draw_x += span;
                }
            } else if (word[0] == 0xb3) {
                if (section == 2) {
                    if (flags & 0x20) {
                        SetFontShadow(0);
                    }
                    DrawWrappedTextLine(reinterpret_cast<UINT16*>(
                                            line), // reinterpret-ok: SGP wide-text API boundary
                                        draw_x, draw_y, remaining_width, active_font, active_colour,
                                        static_cast<unsigned char>(background), true, flags);
                    if (flags & 0x20) {
                        SetFontShadow(2);
                    }
                    section = 1;
                    draw_y += GetFontHeight(active_font) + (line_spacing & 0xff);
                    ++line_count;
                    memset(line, 0, sizeof(line));
                    word_length = 0;
                    memset(word, 0, sizeof(word));
                    line_width = 0;
                    draw_x = x;
                } else {
                    section = 2;
                    memset(word, 0, sizeof(word));
                    word_length = 0;
                    memset(line, 0, sizeof(line));
                    line_width = 0;
                }
            } else if (word[0] == 0xb4 || word[0] == 0xb5) {
                if (flags & 0x20) {
                    SetFontShadow(0);
                }
                DrawWrappedTextLine(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    draw_x, draw_y, remaining_width, active_font, active_colour,
                    static_cast<unsigned char>(background), section != 0, flags);
                if (flags & 0x20) {
                    SetFontShadow(2);
                }
                if (word[0] == 0xb4 && word[1] != L' ' && word[1] < 0x100) {
                    active_colour = static_cast<unsigned char>(word[1]);
                }
                int span = StringPixLength(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
                remaining_width -= line_width;
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                if (word[0] == 0xb5) {
                    active_colour = colour;
                }
                word_length = 0;
                draw_x += span;
            }
        }

        ++position;
        if (ch == L'\0') {
            wcscat(line, &g_wchar_00689b34);
            if (flags & 0x20) {
                SetFontShadow(0);
            }
            DrawWrappedTextLine(
                reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                draw_x, draw_y, remaining_width, active_font, active_colour,
                static_cast<unsigned char>(background), section != 0, flags);
            if (flags & 0x20) {
                SetFontShadow(2);
            }
            return (GetFontHeight(font) + (line_spacing & 0xff)) * line_count;
        }
    }
}

// FUNCTION: WIZ8 0x005d0050
int MeasureWrappedText(int x, int y, unsigned int wrap_width, int line_spacing, int font,
                       int alternate_font, const wchar_t* text, int arg_8, int arg_9, int arg_10,
                       unsigned int* out_edge)
{
    wchar_t line[0x140];
    wchar_t word[0x140];
    unsigned int position = 0;
    unsigned int word_length = 0;
    unsigned int line_width = 0;
    int line_count = 1;
    short section = 1;
    bool alternate = false;
    int active_font = font;

    memset(line, 0, sizeof(line));
    memset(word, 0, sizeof(word));
    GetFontHeight(font);

    for (;;) {
        wchar_t ch = text[position];
        if (ch != L' ' && ch != L'\0') {
            if (ch == L'\n') {
                GetFontHeight(active_font);
                if (line_count == 1 && out_edge != 0) {
                    *out_edge = 0xffffffff;
                }
                ++line_count;
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                line_width = 0;
                word_length = 0;
            } else {
                word[word_length++] = ch;
            }
        } else if ((word[0] < 0xb2 || word[0] > 0xb5) && word[0] != L'\n') {
            word[word_length] = L'\0';
            unsigned int word_width = StringPixLength(
                reinterpret_cast<UINT16*>(word), // reinterpret-ok: SGP wide-text API boundary
                active_font);
            word[word_length++] = L' ';
            word[word_length] = L'\0';
            unsigned int next_width = word_width + line_width;
            if ((wrap_width & 0xffff) < next_width) {
                GetFontHeight(active_font);
                if (line_count == 1 && out_edge != 0) {
                    *out_edge = next_width;
                }
                ++line_count;
                wcscpy(line, word);
                line_width = StringPixLength(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
            } else {
                line_width += StringPixLength(
                    reinterpret_cast<UINT16*>(word), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
                wcscat(line, word);
            }
            word_length = 0;
        } else {
            switch (word[0]) {
            case L'\n':
                GetFontHeight(active_font);
                if (line_count == 1 && out_edge != 0) {
                    *out_edge = 0xffffffff;
                }
                ++line_count;
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                line_width = 0;
                word_length = 0;
                break;
            case 0xb2:
                if (alternate) {
                    StringPixLength(reinterpret_cast<UINT16*>(
                                        line), // reinterpret-ok: SGP wide-text API boundary
                                    active_font);
                    memset(line, 0, sizeof(line));
                    memset(word, 0, sizeof(word));
                    alternate = false;
                    word_length = 0;
                    active_font = font;
                } else {
                    StringPixLength(reinterpret_cast<UINT16*>(
                                        line), // reinterpret-ok: SGP wide-text API boundary
                                    active_font);
                    active_font = g_font12point1_683648;
                    memset(line, 0, sizeof(line));
                    memset(word, 0, sizeof(word));
                    SetFontShadow(0);
                    alternate = true;
                    word_length = 0;
                }
                break;
            case 0xb3:
                if (section == 2) {
                    GetFontHeight(active_font);
                    if (line_count == 1 && out_edge != 0) {
                        *out_edge = 0xffffffff;
                    }
                    ++line_count;
                    line_width = 0;
                    section = 1;
                } else {
                    section = 2;
                    line_width = 0;
                }
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                word_length = 0;
                break;
            case 0xb4:
            case 0xb5:
                StringPixLength(
                    reinterpret_cast<UINT16*>(line), // reinterpret-ok: SGP wide-text API boundary
                    active_font);
                memset(line, 0, sizeof(line));
                memset(word, 0, sizeof(word));
                word_length = 0;
                break;
            }
        }

        ++position;
        if (ch == L'\0') {
            return (GetFontHeight(font) + (line_spacing & 0xff)) * line_count;
        }
    }
}

// FUNCTION: WIZ8 0x005cf620
unsigned char DrawPortraitQuoteBubble(int quote_handle, short x, short y, unsigned int surface)
{
    W8PortraitQuoteBubble* bubble;
    unsigned int blt_flags;

    if (quote_handle == -1) {
        return 0;
    }
    bubble = g_portrait_quotes[quote_handle];
    if (bubble == 0) {
        return 0;
    }
    if ((bubble->flags & 1) == 0) {
        blt_flags = 4;
    } else {
        blt_flags = 6;
    }
    g_current_portrait_quote = bubble;
    BltVideoSurface(surface, bubble->surface, 0, x, y, blt_flags, 0);
    if (surface == 0xfffffff2) {
        InvalidateRegion(x, y, static_cast<short>(x + bubble->width),
                         static_cast<short>(y + bubble->height), 0);
    }
    return 1;
}

// FUNCTION: WIZ8 0x005cf6c0
int LayoutPortraitQuoteBubble(int quote_handle, unsigned char background_index,
                              unsigned char edge_index, const wchar_t* text, unsigned int max_width,
                              int margin_x, int margin_top, int margin_bottom,
                              unsigned short* out_width, unsigned short* out_height,
                              unsigned int font_palette)
{
    W8PortraitQuoteBubble* bubble;
    W8PortraitQuoteBubble** slot;
    VSURFACE_DESC surface_desc;
    VSURFACE_DESC text_desc;
    VOBJECT_DESC object_desc;
    wchar_t line[0x800];
    const wchar_t* read;
    wchar_t* write;
    size_t remaining;
    size_t length;
    int position;
    short line_width;
    int index;
    unsigned int max_line;
    unsigned int right_edge;
    int text_height;
    unsigned int height;
    int width_px;
    int height_px;
    /* Retail left these unset on the nonzero-background/no-palette path and
       still consumed them; deterministic defaults model that defect path. */
    unsigned char colour = 0;
    unsigned char foreground = 0;
    unsigned short count;
    unsigned short x;
    unsigned short y;
    UINT16 fill;
    UINT16* pixels;
    UINT8* source;
    UINT32 pitch;
    UINT32 source_pitch;
    HVOBJECT object;
    HVSURFACE source_surface;
    SGPRect rect;

    if (static_cast<unsigned short>(max_width) >= 0x280) {
        return -1;
    }
    if (static_cast<unsigned short>(max_width) <= 0xa) {
        max_width = 10;
    }
    if (quote_handle == -1) {
        bubble = new W8PortraitQuoteBubble;
        g_current_portrait_quote = bubble;
        surface_desc.fCreateFlags = VSURFACE_CREATE_FROMFILE | VSURFACE_SYSTEM_MEM_USAGE;
        strcpy(surface_desc.ImageFile, g_quote_bubble_backgrounds_64f554[background_index]);
        if (!AddVideoSurface(&surface_desc, &bubble->background_surface)) {
            delete bubble;
            return -1;
        }
        object_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
        strcpy(object_desc.ImageFile, g_quote_bubble_edges_64f550[edge_index]);
        if (!AddVideoObject(&object_desc, &g_current_portrait_quote->object)) {
            delete bubble;
            return -1;
        }
        g_current_portrait_quote->has_resources_14 = 1;
        g_current_portrait_quote->background_index_08 = background_index;
        g_current_portrait_quote->object_index_09 = edge_index;
    } else {
        bubble = g_portrait_quotes[quote_handle];
        g_current_portrait_quote = bubble;
        if (background_index != bubble->background_index_08 ||
            edge_index != bubble->object_index_09 || bubble->has_resources_14 == 0) {
            if (bubble != 0 && bubble->has_resources_14 != 0) {
                DeleteVideoSurfaceFromIndex(bubble->background_surface);
                DeleteVideoObjectFromIndex(g_current_portrait_quote->object);
                g_current_portrait_quote->has_resources_14 = 0;
            }
            surface_desc.fCreateFlags = VSURFACE_CREATE_FROMFILE | VSURFACE_SYSTEM_MEM_USAGE;
            strcpy(surface_desc.ImageFile, g_quote_bubble_backgrounds_64f554[background_index]);
            if (!AddVideoSurface(&surface_desc, &g_current_portrait_quote->background_surface)) {
                return -1;
            }
            object_desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
            strcpy(object_desc.ImageFile, g_quote_bubble_edges_64f550[edge_index]);
            if (!AddVideoObject(&object_desc, &g_current_portrait_quote->object)) {
                return -1;
            }
            g_current_portrait_quote->has_resources_14 = 1;
            g_current_portrait_quote->background_index_08 = background_index;
            g_current_portrait_quote->object_index_09 = edge_index;
        }
    }
    g_current_portrait_quote->text = static_cast<wchar_t*>(malloc(wcslen(text) * 2 + 2));
    wcscpy(g_current_portrait_quote->text, text);
    g_current_portrait_quote->flags = g_quote_bubble_flags_69c5c8;
    position = 0;
    g_quote_bubble_flags_69c5c8 = 0;
    max_line = 0xffffffff;
    remaining = wcslen(text);
    memset(line, 0, sizeof(line));
    if (remaining > 0) {
        read = text;
        do {
            if (*read != L'\n') {
                line[position] = *read;
                ++position;
            } else {
                line_width = 0;
                length = wcslen(line);
                if (static_cast<int>(length) > 0) {
                    write = line;
                    do {
                        wchar_t ch = *write;
                        if ((static_cast<unsigned short>(ch) < 0xb2 ||
                             static_cast<unsigned short>(ch) > 0xb5) &&
                            static_cast<unsigned short>(ch) > 10) {
                            line_width += StringPixLengthArg(g_font12point1_683648, 1, write);
                        }
                        ++write;
                    } while (--length != 0);
                }
                if (line_width > static_cast<int>(max_line)) {
                    max_line = line_width;
                }
                position = 0;
                memset(line, 0, sizeof(line));
            }
            ++read;
        } while (--remaining != 0);
    }
    line_width = 0;
    length = wcslen(line);
    if (static_cast<int>(length) > 0) {
        write = line;
        do {
            wchar_t ch = *write;
            if ((static_cast<unsigned short>(ch) < 0xb2 ||
                 static_cast<unsigned short>(ch) > 0xb5) &&
                static_cast<unsigned short>(ch) > 10) {
                line_width += StringPixLengthArg(g_font12point1_683648, 1, write);
            }
            ++write;
        } while (--length != 0);
    }
    if (line_width > static_cast<int>(max_line)) {
        max_line = line_width;
    }
    if (static_cast<int>(max_line & 0xffff) < static_cast<int>((max_width & 0xffff) - 0x18)) {
        max_width = max_line + 0x18;
        max_line = max_line + 1;
    } else {
        max_line = (max_width - margin_x) - 0x17;
        right_edge = 0xffffffff;
        MeasureWrappedText(0, 0, max_line, 2, g_font12point1_683648, 0xd0, text, 0, 0, 1,
                           &right_edge);
        if (right_edge != 0xffffffff && static_cast<int>(right_edge - (max_line & 0xffff)) < 0x14) {
            max_line = right_edge;
            max_width = right_edge + 0x18;
        }
    }
    text_height =
        MeasureWrappedText(0, 0, max_line, 2, g_font12point1_683648, 0xd0, text, 0, 0, 1, 0);
    height = text_height + margin_top + 0x18 + margin_bottom;
    max_width = max_width + margin_x * 2;
    if (static_cast<unsigned short>(max_width) >= 0x15e) {
        max_width = 0x15d;
    }
    if (static_cast<unsigned short>(height) < 200) {
        memset(&text_desc, 0, sizeof(text_desc));
        text_desc.fCreateFlags = VSURFACE_CREATE_DEFAULT | VSURFACE_SYSTEM_MEM_USAGE;
        text_desc.usWidth = static_cast<unsigned short>(max_width);
        text_desc.usHeight = static_cast<unsigned short>(height);
        text_desc.ubBitDepth = 0x10;
        if (!AddVideoSurface(&text_desc, &bubble->surface)) {
            return 0;
        }
        bubble->palette = font_palette;
        bubble->created_15 = 1;
        bubble->width = static_cast<unsigned short>(max_width);
        bubble->height = static_cast<unsigned short>(height);
        rect.iLeft = 0;
        rect.iTop = 0;
        width_px = max_width & 0xffff;
        height_px = height & 0xffff;
        *out_width = static_cast<unsigned short>(max_width);
        *out_height = static_cast<unsigned short>(height);
        rect.iRight = width_px;
        rect.iBottom = height_px;
        if (bubble->flags & 1) {
            SetVideoSurfaceTransparency(bubble->surface, 0xffff);
            pixels = reinterpret_cast<UINT16*>( // reinterpret-ok: raw locked pixel memory
                LockVideoSurface(bubble->surface, &pitch));
            fill = Get16BPPColor(0xffff);
            count = static_cast<unsigned short>(height * max_width);
            for (x = 0; x < count; ++x) {
                pixels[x] = fill;
            }
            UnLockVideoSurface(bubble->surface);
        } else {
            GetVideoSurface(&source_surface, bubble->background_surface);
            pixels = reinterpret_cast<UINT16*>( // reinterpret-ok: raw locked pixel memory
                LockVideoSurface(bubble->surface, &pitch));
            source = LockVideoSurface(bubble->background_surface, &source_pitch);
            Blt8BPPDataSubTo16BPPBuffer(pixels, pitch, source_surface, source, source_pitch, 0, 0,
                                        &rect);
            UnLockVideoSurface(bubble->background_surface);
            UnLockVideoSurface(bubble->surface);
        }
        GetVideoObject(&object, bubble->object);
        for (x = 0x10; static_cast<int>(x) < width_px - 0x10; x = x + 0x10) {
            BltVideoObject(bubble->surface, object, 1, x, 0, 2, 0);
            BltVideoObject(bubble->surface, object, 6, x, height_px - 0x10, 2, 0);
        }
        for (y = 0x10; static_cast<int>(y) < height_px - 0x10; y = y + 0x10) {
            BltVideoObject(bubble->surface, object, 3, 0, y, 2, 0);
            BltVideoObject(bubble->surface, object, 4, width_px - 8, y, 2, 0);
        }
        BltVideoObject(bubble->surface, object, 0, 0, 0, 2, 0);
        BltVideoObject(bubble->surface, object, 2, width_px - 0x10, 0, 2, 0);
        BltVideoObject(bubble->surface, object, 5, 0, height_px - 0x10, 2, 0);
        BltVideoObject(bubble->surface, object, 7, width_px - 0x10, height_px - 0x10, 2, 0);
        if (background_index == 0) {
            colour = 0xd0;
            foreground = 0;
        }
        if (bubble->palette != 0xffffffff) {
            colour = static_cast<unsigned char>(bubble->palette);
        }
        SetFont(g_font12point1_683648);
        SetFontForeground(foreground);
        SetFontDestBuffer(bubble->surface, 0, 0, width_px, height_px, 0);
        DrawWrappedText(margin_x + 0xc, margin_top + 0xc, max_line, 2, g_font12point1_683648,
                        colour, text, 0, 0, 1);
        SetFontDestBuffer(-14, 0, 0, 0x280, 0x1e0, 0);
        SetFontForeground(2);
        if (quote_handle == -1 && bubble != 0) {
            index = 0;
            slot = g_portrait_quotes;
            do {
                if (*slot == 0) {
                    g_portrait_quotes[index] = bubble;
                    if (index != -1) {
                        g_current_portrait_quote = bubble;
                    }
                    return index;
                }
                ++slot;
                ++index;
            } while (slot < g_portrait_quotes + 10);
            return -1;
        }
        if (quote_handle == -1) {
            return -1;
        }
        if (g_portrait_quotes[quote_handle] != 0) {
            g_current_portrait_quote = g_portrait_quotes[quote_handle];
        }
        return quote_handle;
    }
    if (quote_handle != -1) {
        return -1;
    }
    delete bubble;
    return -1;
}

// FUNCTION: WIZ8 0x005cffa0
unsigned char ReleasePortraitQuoteBubble(int quote_handle)
{
    W8PortraitQuoteBubble* quote;

    if (quote_handle != -1 && (quote = g_portrait_quotes[quote_handle]) != 0 &&
        (g_current_portrait_quote = quote) != 0) {
        if (g_current_portrait_quote->created_15 != 0) {
            for (int index = 0; index < 10; ++index) {
                if (g_portrait_quotes[index] == quote) {
                    g_portrait_quotes[index] = 0;
                    index = 10;
                }
            }
            DeleteVideoSurfaceFromIndex(quote->surface);
            free(g_current_portrait_quote->text);
            if (g_current_portrait_quote != 0 && g_current_portrait_quote->has_resources_14 != 0) {
                DeleteVideoSurfaceFromIndex(g_current_portrait_quote->background_surface);
                DeleteVideoObjectFromIndex(g_current_portrait_quote->object);
                g_current_portrait_quote->has_resources_14 = 0;
            }
            delete g_current_portrait_quote;
            g_current_portrait_quote = 0;
        }
        return 1;
    }
    return 0;
}

// FUNCTION: WIZ8 0x005d0750
const wchar_t* GetPortraitQuoteText(int quote_handle)
{
    W8PortraitQuoteBubble* quote;

    if (quote_handle != -1 && (quote = g_portrait_quotes[quote_handle]) != 0) {
        g_current_portrait_quote = quote;
        return quote->text;
    }
    return 0;
}
