#pragma once

#include <wchar.h>

int LayoutPortraitQuoteBubble(int quote_handle, unsigned char background_index,
                              unsigned char edge_index, const wchar_t* text, unsigned int max_width,
                              int margin_x, int margin_top, int margin_bottom,
                              unsigned short* out_width, unsigned short* out_height,
                              unsigned int font_palette);
/* 0x005CF620: blit the bubble's rendered surface to `surface` at (x, y). */
unsigned char DrawPortraitQuoteBubble(int quote_handle, short x, short y, unsigned int surface);
unsigned char ReleasePortraitQuoteBubble(int quote_handle);
const wchar_t* GetPortraitQuoteText(int quote_handle);
