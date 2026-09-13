#pragma once

#include <wchar.h>

int LayoutPortraitQuoteBubble(int surface_handle, unsigned char param_2, unsigned char param_3,
                              const wchar_t* text, unsigned int max_width, int param_6, int param_7,
                              int param_8, unsigned short* out_width, unsigned short* out_height,
                              unsigned int font_palette);
unsigned char ReleasePortraitQuoteBubble(int quote_handle);
const wchar_t* GetPortraitQuoteText(int quote_handle);
