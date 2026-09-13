#include "Types.h"
#include "Font.h"
#include "vobject.h"
#include "vsurface.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/PortraitQuote.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#pragma pack(push, 1)
struct W8PortraitQuoteBubble {
    UINT32 surface;
    UINT16 width;
    UINT16 height;
    unsigned char param_2;
    unsigned char param_3;
    unsigned char unknown_0a[2];
    UINT32 background_surface;
    UINT32 object;
    unsigned char unknown_14;
    unsigned char active;
    unsigned short unknown_16;
    UINT32 flags;
    wchar_t* text;
    UINT32 palette;
};
#pragma pack(pop)

static_assert(sizeof(W8PortraitQuoteBubble) == 0x24, "W8PortraitQuoteBubble_size");

static W8PortraitQuoteBubble* g_portrait_quotes[10];
static W8PortraitQuoteBubble* g_current_portrait_quote;

static void ReleasePortraitQuoteResources(W8PortraitQuoteBubble* quote)
{
    if (quote->active != 0) {
        DeleteVideoSurfaceFromIndex(quote->surface);
        DeleteVideoSurfaceFromIndex(quote->background_surface);
        DeleteVideoObjectFromIndex(quote->object);
        quote->active = 0;
    }
}

// FUNCTION: WIZ8 0x005cf6c0
int LayoutPortraitQuoteBubble(int surface_handle, unsigned char param_2, unsigned char param_3,
                              const wchar_t* text, unsigned int max_width, int param_6, int param_7,
                              int param_8, unsigned short* out_width, unsigned short* out_height,
                              unsigned int font_palette)
{
    (void)surface_handle;
    (void)param_6;
    (void)param_7;
    (void)param_8;
    if (text == 0) {
        return -1;
    }
    if (max_width > 0x27f) {
        return -1;
    }
    if (max_width < 0xb) {
        max_width = 10;
    }

    int handle = -1;
    for (int index = 0; index < 10; ++index) {
        if (g_portrait_quotes[index] == 0) {
            handle = index;
            break;
        }
    }
    if (handle == -1) {
        return -1;
    }

    W8PortraitQuoteBubble* quote =
        static_cast<W8PortraitQuoteBubble*>(calloc(1, sizeof(W8PortraitQuoteBubble)));
    if (quote == 0) {
        return -1;
    }
    quote->text = static_cast<wchar_t*>(malloc((wcslen(text) + 1) * sizeof(wchar_t)));
    if (quote->text == 0) {
        free(quote);
        return -1;
    }
    wcscpy(quote->text, text);

    unsigned int width = 0;
    unsigned int height = GetFontHeight(g_dialog_font_64fde8);
    const wchar_t* line = text;
    for (const wchar_t* cursor = text;; ++cursor) {
        if (*cursor == L'\n' || *cursor == L'\0') {
            size_t length = static_cast<size_t>(cursor - line);
            wchar_t* line_copy = static_cast<wchar_t*>(malloc((length + 1) * sizeof(wchar_t)));
            if (line_copy == 0) {
                free(quote->text);
                free(quote);
                return -1;
            }
            memcpy(line_copy, line, length * sizeof(wchar_t));
            line_copy[length] = L'\0';
            unsigned int line_width =
                static_cast<unsigned int>(StringPixLength(line_copy, g_dialog_font_64fde8));
            free(line_copy);
            if (line_width > width) {
                width = line_width;
            }
            if (*cursor == L'\0') {
                break;
            }
            height += GetFontHeight(g_dialog_font_64fde8);
            line = cursor + 1;
        }
    }

    unsigned int interior_width = width + 0x18;
    if (interior_width < max_width) {
        max_width = interior_width;
    } else {
        max_width = (max_width - param_6) - 0x17;
    }
    quote->width = max_width + param_6 * 2;
    quote->height = height + param_7 + 0x18 + param_8;
    if (quote->width > 0x15d) {
        quote->width = 0x15d;
    }
    if (quote->height >= 200) {
        free(quote->text);
        free(quote);
        return -1;
    }

    quote->palette = font_palette;
    quote->param_2 = param_2;
    quote->param_3 = param_3;
    quote->active = 1;
    g_portrait_quotes[handle] = quote;
    g_current_portrait_quote = quote;
    if (out_width != 0) {
        *out_width = static_cast<unsigned short>(quote->width);
    }
    if (out_height != 0) {
        *out_height = static_cast<unsigned short>(quote->height);
    }
    return handle;
}

// FUNCTION: WIZ8 0x005cffa0
unsigned char ReleasePortraitQuoteBubble(int quote_handle)
{
    if (quote_handle == -1 || quote_handle < 0 || quote_handle >= 10 ||
        g_portrait_quotes[quote_handle] == 0) {
        return 0;
    }
    W8PortraitQuoteBubble* quote = g_portrait_quotes[quote_handle];
    g_portrait_quotes[quote_handle] = 0;
    g_current_portrait_quote = quote;
    ReleasePortraitQuoteResources(quote);
    free(quote->text);
    free(quote);
    g_current_portrait_quote = 0;
    return 1;
}

// FUNCTION: WIZ8 0x005d0750
const wchar_t* GetPortraitQuoteText(int quote_handle)
{
    if (quote_handle == -1 || quote_handle < 0 || quote_handle >= 10 ||
        g_portrait_quotes[quote_handle] == 0) {
        return 0;
    }
    g_current_portrait_quote = g_portrait_quotes[quote_handle];
    return g_current_portrait_quote->text;
}
