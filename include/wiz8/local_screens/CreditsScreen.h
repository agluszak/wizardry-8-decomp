#pragma once

#include "wiz8/vector.h"

struct W8CreditLine {
    unsigned int flags;
    int pixel_width;
    int line_height;
    wchar_t* primary;
    wchar_t* secondary;
};
static_assert(sizeof(W8CreditLine) == 0x14, "W8CreditLine_size");

unsigned char CreditsScreenEnter005BC130(void);
unsigned char CreditsScreenLeave005BC420(int leaving);
void CreditsScreenFrame005BC530(void);
