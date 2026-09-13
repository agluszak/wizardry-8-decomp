#include "Types.h"
#include "Font.h"
#include "vobject.h"
#include "vsurface.h"
#include "wiz8/dialog_code/DialogInterface.h"
#include "wiz8/dialog_code/PortraitQuote.h"

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* The NPC quote bubble record, allocated with operator new inside
   LayoutPortraitQuoteBubble. Offset 0 is the rendered text surface; 0x0c and
   0x10 are the popup background surface and border object the bubble is
   composited from. The byte at 0x14 is set once those two resources exist;
   the byte at 0x15 is set once the whole bubble has been created. */
#pragma pack(push, 1)
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
#pragma pack(pop)

static_assert(sizeof(W8PortraitQuoteBubble) == 0x24, "W8PortraitQuoteBubble_size");

static W8PortraitQuoteBubble* g_portrait_quotes[10];
static W8PortraitQuoteBubble* g_current_portrait_quote;

/* LayoutPortraitQuoteBubble (0x005cf6c0) is unrecovered; the runtime build
   reaches it through the generated stub trap. */

// FUNCTION: WIZ8 0x005cffa0
unsigned char ReleasePortraitQuoteBubble(int quote_handle)
{
    W8PortraitQuoteBubble* quote;

    if (quote_handle != -1 && (quote = g_portrait_quotes[quote_handle]) != 0) {
        g_current_portrait_quote = quote;
        if (quote->created_15 != 0) {
            for (int index = 0; index < 10; ++index) {
                if (g_portrait_quotes[index] == quote) {
                    g_portrait_quotes[index] = 0;
                    index = 10;
                }
            }
            DeleteVideoSurfaceFromIndex(quote->surface);
            free(quote->text);
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
