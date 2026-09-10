#pragma once
#include "wiz8/screen_state.h"

#include "wiz8/vector.h"

struct W8CreditLine {
    unsigned int flags;
    int pixel_width;
    int line_height;
    wchar_t* primary;
    wchar_t* secondary;
};
static_assert(sizeof(W8CreditLine) == 0x14, "W8CreditLine_size");

unsigned char ReadWideTextLine004CEED0(
    int handle, wchar_t* destination, int capacity, unsigned char* more);
